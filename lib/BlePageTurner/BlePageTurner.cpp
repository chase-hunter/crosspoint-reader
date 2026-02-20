#include "BlePageTurner.h"

#include <Logging.h>

// USB HID keyboard usage page keycodes (from USB HID Usage Tables 1.12)
namespace HidKeycode {
constexpr uint8_t NONE = 0x00;
constexpr uint8_t ENTER = 0x28;
constexpr uint8_t ESCAPE = 0x29;
constexpr uint8_t SPACE = 0x2C;
constexpr uint8_t PAGE_UP = 0x4B;
constexpr uint8_t PAGE_DOWN = 0x4E;
constexpr uint8_t RIGHT_ARROW = 0x4F;
constexpr uint8_t LEFT_ARROW = 0x50;
constexpr uint8_t DOWN_ARROW = 0x51;
constexpr uint8_t UP_ARROW = 0x52;
constexpr uint8_t F5 = 0x3E;  // Some presenters use F5
}  // namespace HidKeycode

// HID service UUID
static const NimBLEUUID HID_SERVICE_UUID("1812");
// HID Report characteristic UUID
static const NimBLEUUID HID_REPORT_UUID("2A4D");
// HID Report Map characteristic UUID (for debugging)
static const NimBLEUUID HID_REPORT_MAP_UUID("2A4B");

// Singleton pointer for static callback
static BlePageTurner* s_instance = nullptr;

BlePageTurner::~BlePageTurner() {
  end();
}

void BlePageTurner::begin() {
  if (state_.load() != State::Disabled) {
    return;  // Already initialised
  }

  s_instance = this;

  LOG_DBG("BLE", "Initializing NimBLE stack");
  NimBLEDevice::init("CrossPoint");

  // Set power level (ESP32-C3 supports limited levels)
  NimBLEDevice::setPower(ESP_PWR_LVL_P3);

  // Enable bonding/encryption for HID devices
  NimBLEDevice::setSecurityAuth(true, true, true);  // bonding, MITM, SC
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

  state_.store(State::Idle, std::memory_order_release);
  LOG_DBG("BLE", "NimBLE stack initialized");
}

void BlePageTurner::end() {
  if (state_.load() == State::Disabled) {
    return;
  }

  disconnect();
  stopScan();

  NimBLEDevice::deinit(true);
  s_instance = nullptr;
  state_.store(State::Disabled, std::memory_order_release);
  LOG_DBG("BLE", "NimBLE stack deinitialized");
}

void BlePageTurner::startScan(uint32_t durationSeconds) {
  const auto currentState = state_.load();
  if (currentState == State::Disabled || currentState == State::Connected) {
    return;
  }

  LOG_DBG("BLE", "Starting BLE scan for %lu seconds", durationSeconds);

  connectionPending_ = false;
  discoveredDevices_.clear();

  auto* scan = NimBLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(this, false);
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(99);
  scan->start(durationSeconds, false);

  state_.store(State::Scanning, std::memory_order_release);
}

void BlePageTurner::stopScan() {
  auto* scan = NimBLEDevice::getScan();
  if (scan->isScanning()) {
    scan->stop();
  }

  const auto currentState = state_.load();
  if (currentState == State::Scanning) {
    state_.store(State::ScanComplete, std::memory_order_release);
    LOG_DBG("BLE", "Scan stopped, %d device(s) found", static_cast<int>(discoveredDevices_.size()));
  }
}

void BlePageTurner::dismissScanResults() {
  if (state_.load() == State::ScanComplete) {
    state_.store(State::Idle, std::memory_order_release);
  }
}

void BlePageTurner::connectToDeviceByIndex(size_t index) {
  if (index >= discoveredDevices_.size()) {
    LOG_ERR("BLE", "Invalid device index: %d", static_cast<int>(index));
    return;
  }
  pendingConnectionIndex_ = index;
  deviceName_ = discoveredDevices_[index].name;
  displayPasskey_.store(0, std::memory_order_release);
  connectionPending_ = true;
  state_.store(State::Connecting, std::memory_order_release);
}

void BlePageTurner::disconnect() {
  if (client_ && client_->isConnected()) {
    client_->disconnect();
  }
  client_ = nullptr;
  const auto currentState = state_.load();
  if (currentState == State::Connected || currentState == State::Connecting) {
    state_.store(State::Idle, std::memory_order_release);
  }
}

bool BlePageTurner::update() {
  // Handle deferred connection request from the UI
  if (connectionPending_) {
    connectionPending_ = false;
    if (pendingConnectionIndex_ < discoveredDevices_.size()) {
      const auto& dev = discoveredDevices_[pendingConnectionIndex_];

      if (connectToAddress(NimBLEAddress(dev.address))) {
        state_.store(State::Connected, std::memory_order_release);
        LOG_INF("BLE", "Connected to page turner: %s", deviceName_.c_str());
      } else {
        state_.store(State::ScanComplete, std::memory_order_release);
        LOG_ERR("BLE", "Failed to connect to page turner");
      }
    }
  }

  // Check if scan finished naturally
  if (state_.load() == State::Scanning) {
    auto* scan = NimBLEDevice::getScan();
    if (!scan->isScanning()) {
      state_.store(State::ScanComplete, std::memory_order_release);
      LOG_DBG("BLE", "Scan complete, %d device(s) found",
              static_cast<int>(discoveredDevices_.size()));
    }
  }

  // Check for disconnection
  if (state_.load() == State::Connected && client_ && !client_->isConnected()) {
    state_.store(State::Idle, std::memory_order_release);
    LOG_INF("BLE", "Page turner disconnected");
    client_ = nullptr;
  }

  return pendingEvent_.load(std::memory_order_relaxed) != Event::None;
}

BlePageTurner::Event BlePageTurner::consumeEvent() {
  return pendingEvent_.exchange(Event::None, std::memory_order_acq_rel);
}

// --- NimBLE Callbacks ---

void BlePageTurner::onResult(NimBLEAdvertisedDevice* device) {
  LOG_DBG("BLE", "Found device: %s  RSSI: %d", device->getName().c_str(), device->getRSSI());

  // Check if this device advertises the HID service
  if (device->isAdvertisingService(HID_SERVICE_UUID)) {
    std::string name = device->getName();
    std::string addr = device->getAddress().toString();
    LOG_INF("BLE", "HID device found: %s (%s)", name.c_str(), addr.c_str());

    if (name.empty()) {
      name = addr;
    }

    // Avoid duplicates (same address)
    for (const auto& d : discoveredDevices_) {
      if (d.address == addr) {
        return;
      }
    }

    discoveredDevices_.push_back({name, addr, device->getRSSI()});
  }
}

void BlePageTurner::onConnect(NimBLEClient* client) {
  LOG_DBG("BLE", "Client connected callback");
}

void BlePageTurner::onDisconnect(NimBLEClient* client) {
  LOG_INF("BLE", "Client disconnected callback");
  // State update handled in update()
}

uint32_t BlePageTurner::onPassKeyRequest() {
  // Generate a random 6-digit passkey and display it on the e-reader screen.
  // The user must type this passkey on the Bluetooth keyboard to complete pairing.
  uint32_t passkey = esp_random() % 1000000;
  displayPasskey_.store(passkey, std::memory_order_release);
  LOG_INF("BLE", "Passkey generated for display: %06lu", static_cast<unsigned long>(passkey));

  // Notify the UI to refresh so the passkey is shown while connect() blocks
  if (renderCallback_) {
    renderCallback_();
  }

  return passkey;
}

bool BlePageTurner::onConfirmPIN(uint32_t pin) {
  LOG_INF("BLE", "Numeric comparison PIN: %06lu - auto confirming", static_cast<unsigned long>(pin));
  return true;
}

bool BlePageTurner::connectToAddress(const NimBLEAddress& address) {
  LOG_DBG("BLE", "Connecting to %s", address.toString().c_str());

  client_ = NimBLEDevice::createClient();
  client_->setClientCallbacks(this, false);
  client_->setConnectionParams(12, 12, 0, 400);  // min/max interval, latency, timeout
  client_->setConnectTimeout(10);                 // 10 seconds

  if (!client_->connect(address)) {
    LOG_ERR("BLE", "Connection failed");
    NimBLEDevice::deleteClient(client_);
    client_ = nullptr;
    return false;
  }

  LOG_DBG("BLE", "Connected, discovering services...");

  // Update connection parameters for low power (HID doesn't need fast polling)
  client_->updateConnParams(24, 48, 4, 400);  // 30-60ms interval, latency 4

  if (!subscribeToHidReports(client_)) {
    LOG_ERR("BLE", "Failed to subscribe to HID reports");
    client_->disconnect();
    NimBLEDevice::deleteClient(client_);
    client_ = nullptr;
    return false;
  }

  return true;
}

bool BlePageTurner::subscribeToHidReports(NimBLEClient* client) {
  auto* hidService = client->getService(HID_SERVICE_UUID);
  if (!hidService) {
    LOG_ERR("BLE", "HID service not found");
    return false;
  }

  LOG_DBG("BLE", "HID service found, looking for report characteristics...");

  // Subscribe to all HID Report characteristics (there may be multiple - keyboard, consumer, etc.)
  auto* characteristics = hidService->getCharacteristics(true);
  if (!characteristics) {
    LOG_ERR("BLE", "No characteristics found");
    return false;
  }

  int subscribed = 0;
  for (auto* chr : *characteristics) {
    if (chr->getUUID() == HID_REPORT_UUID) {
      if (chr->canNotify()) {
        if (chr->subscribe(true, onHidReport)) {
          subscribed++;
          LOG_DBG("BLE", "Subscribed to HID report characteristic (handle: %d)", chr->getHandle());
        }
      }
    }
  }

  if (subscribed == 0) {
    LOG_ERR("BLE", "No HID report characteristics could be subscribed");
    return false;
  }

  LOG_INF("BLE", "Subscribed to %d HID report characteristic(s)", subscribed);
  return true;
}

void BlePageTurner::onHidReport(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length,
                                bool isNotify) {
  if (!s_instance || length < 1) {
    return;
  }

  // HID keyboard reports are typically 8 bytes:
  //   [0] modifier keys (Ctrl, Shift, Alt, etc.)
  //   [1] reserved (0x00)
  //   [2..7] up to 6 keycodes
  //
  // Some page turners send shorter reports (e.g. consumer control)
  // or use different report formats. We handle common patterns:

  if (length >= 3) {
    // Standard keyboard report
    // We only care about key down (non-zero keycode), not key up (all zeros)
    for (size_t i = 2; i < length && i < 8; i++) {
      if (data[i] != HidKeycode::NONE) {
        Event evt = s_instance->translateKeycode(data[i]);
        if (evt != Event::None) {
          s_instance->pendingEvent_.store(evt, std::memory_order_release);
          LOG_DBG("BLE", "HID keycode 0x%02X -> event %d", data[i], static_cast<int>(evt));
          return;
        }
      }
    }
  } else if (length >= 2) {
    // Consumer control report (some remotes): 2 bytes, little-endian usage code
    // Common codes: 0x00B5 = Next, 0x00B6 = Previous, 0x00CD = Play/Pause
    uint16_t usage = data[0] | (data[1] << 8);
    Event evt = Event::None;
    switch (usage) {
      case 0x00B5:  // Next Track
      case 0x0095:  // Help / Next
        evt = Event::PageForward;
        break;
      case 0x00B6:  // Previous Track
        evt = Event::PageBack;
        break;
      case 0x00CD:  // Play/Pause → treat as next page
        evt = Event::PageForward;
        break;
      default:
        break;
    }
    if (evt != Event::None) {
      s_instance->pendingEvent_.store(evt, std::memory_order_release);
      LOG_DBG("BLE", "HID consumer usage 0x%04X -> event %d", usage, static_cast<int>(evt));
    }
  } else if (length == 1) {
    // Single byte report: some very simple page turners
    // Treat non-zero as page forward (most common use case)
    if (data[0] != 0) {
      Event evt = s_instance->translateKeycode(data[0]);
      if (evt != Event::None) {
        s_instance->pendingEvent_.store(evt, std::memory_order_release);
      }
    }
  }
}

BlePageTurner::Event BlePageTurner::translateKeycode(uint8_t keycode) const {
  switch (keycode) {
    // Forward page controls
    case HidKeycode::RIGHT_ARROW:
    case HidKeycode::DOWN_ARROW:
    case HidKeycode::PAGE_DOWN:
    case HidKeycode::SPACE:
    case HidKeycode::F5:
      return Event::PageForward;

    // Back page controls
    case HidKeycode::LEFT_ARROW:
    case HidKeycode::UP_ARROW:
    case HidKeycode::PAGE_UP:
      return Event::PageBack;

    // Enter → confirm
    case HidKeycode::ENTER:
      return Event::Confirm;

    // Escape → back
    case HidKeycode::ESCAPE:
      return Event::Back;

    default:
      LOG_DBG("BLE", "Unhandled HID keycode: 0x%02X", keycode);
      return Event::None;
  }
}
