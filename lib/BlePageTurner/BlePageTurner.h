#pragma once

#include <NimBLEDevice.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

/**
 * BLE HID Host for page turner remotes.
 *
 * Scans for BLE HID devices (page turners, presenters, keyboards), connects,
 * and translates HID keyboard reports into virtual button events that can be
 * consumed by MappedInputManager.
 *
 * Typical HID page turners send standard keyboard keycodes:
 *   - Right Arrow / Page Down / Enter / Space  → "next page"
 *   - Left Arrow  / Page Up                    → "previous page"
 *
 * This class runs on the NimBLE task and notifies the main loop via atomic
 * flags so no additional FreeRTOS task or mutex is needed.
 */
class BlePageTurner : public NimBLEClientCallbacks,
                      public NimBLEAdvertisedDeviceCallbacks {
 public:
  /// Information about a discovered BLE HID device.
  struct DiscoveredDevice {
    std::string name;
    std::string address;
    int rssi = 0;
  };

  // Virtual button events produced by the page turner
  enum class Event : uint8_t {
    None = 0,
    PageForward,
    PageBack,
    Confirm,
    Back,
  };

  // Connection state
  enum class State : uint8_t {
    Disabled = 0,   // BLE not initialised
    Idle,           // Initialised, not scanning or connected
    Scanning,       // Actively scanning for devices
    ScanComplete,   // Scan finished, results available for selection
    Connecting,     // Connection attempt in progress
    Connected,      // Page turner is connected and ready
  };

  BlePageTurner() = default;
  ~BlePageTurner();

  // Delete copy
  BlePageTurner(const BlePageTurner&) = delete;
  BlePageTurner& operator=(const BlePageTurner&) = delete;

  /// Initialise the NimBLE stack. Call once from setup().
  void begin();

  /// Tear down BLE. Disconnects and deinitialises the stack.
  void end();

  /// Start scanning for HID devices. Non-blocking. Discovered devices are
  /// collected in a list and can be retrieved after scanning completes.
  void startScan(uint32_t durationSeconds = 10);

  /// Stop an in-progress scan. Transitions to ScanComplete so the user can
  /// still see any devices found so far.
  void stopScan();

  /// Select a device and begin connecting by its index in the discovered list.
  void connectToDeviceByIndex(size_t index);

  /// Disconnect any connected device.
  void disconnect();

  /// Call from loop() to process events. Returns true if any virtual button was pressed this frame.
  bool update();

  /// Consume the latest event. Returns Event::None if no event pending.
  Event consumeEvent();

  /// Current connection state.
  State getState() const { return state_.load(std::memory_order_relaxed); }

  /// Name of the connected (or last connected) device.
  const std::string& getDeviceName() const { return deviceName_; }

  /// Whether BLE has been initialised.
  bool isEnabled() const { return state_.load(std::memory_order_relaxed) != State::Disabled; }

  /// Whether a page turner is currently connected.
  bool isConnected() const { return state_.load(std::memory_order_relaxed) == State::Connected; }

  /// Get the pairing passkey displayed during connection (0 if none).
  uint32_t getDisplayPasskey() const { return displayPasskey_.load(std::memory_order_relaxed); }

  /// Set a callback to be invoked when a screen refresh is needed
  /// (e.g. when a passkey is generated during the blocking connect call).
  void setRenderCallback(std::function<void()> cb) { renderCallback_ = std::move(cb); }

  /// Get the list of devices found during the last scan.
  const std::vector<DiscoveredDevice>& getDiscoveredDevices() const { return discoveredDevices_; }

  /// Clear the discovered device list.
  void clearDiscoveredDevices() { discoveredDevices_.clear(); }

  /// Dismiss the scan results and return to Idle.
  void dismissScanResults();

 private:
  // NimBLEClientCallbacks
  void onConnect(NimBLEClient* client) override;
  void onDisconnect(NimBLEClient* client) override;
  uint32_t onPassKeyRequest() override;
  bool onConfirmPIN(uint32_t pin) override;
  void onAuthenticationComplete(ble_gap_conn_desc* desc) override;

  // NimBLEAdvertisedDeviceCallbacks
  void onResult(NimBLEAdvertisedDevice* device) override;

  // HID report notification handler
  static void onHidReport(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify);

  // Internal helpers
  bool connectToAddress(const NimBLEAddress& address);
  bool subscribeToHidReports(NimBLEClient* client);
  Event translateKeycode(uint8_t keycode) const;

  // State
  std::atomic<State> state_{State::Disabled};
  std::atomic<Event> pendingEvent_{Event::None};
  std::atomic<uint32_t> displayPasskey_{0};

  NimBLEClient* client_ = nullptr;
  std::string deviceName_;
  bool connectionPending_ = false;
  size_t pendingConnectionIndex_ = 0;

  // Discovered devices from the most recent scan
  std::vector<DiscoveredDevice> discoveredDevices_;

  // Optional callback for requesting a screen refresh
  std::function<void()> renderCallback_;
};
