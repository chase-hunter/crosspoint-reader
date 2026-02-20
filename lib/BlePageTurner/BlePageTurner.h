#pragma once

#include <NimBLEDevice.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

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
    Disabled = 0,  // BLE not initialised
    Idle,          // Initialised, not scanning or connected
    Scanning,      // Actively scanning for devices
    Connecting,    // Connection attempt in progress
    Connected,     // Page turner is connected and ready
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

  /// Start scanning for HID devices. Non-blocking.
  void startScan(uint32_t durationSeconds = 10);

  /// Stop an in-progress scan.
  void stopScan();

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

 private:
  // NimBLEClientCallbacks
  void onConnect(NimBLEClient* client) override;
  void onDisconnect(NimBLEClient* client) override;

  // NimBLEAdvertisedDeviceCallbacks
  void onResult(NimBLEAdvertisedDevice* device) override;

  // HID report notification handler
  static void onHidReport(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify);

  // Internal helpers
  bool connectToDevice(NimBLEAdvertisedDevice* device);
  bool subscribeToHidReports(NimBLEClient* client);
  Event translateKeycode(uint8_t keycode) const;

  // State
  std::atomic<State> state_{State::Disabled};
  std::atomic<Event> pendingEvent_{Event::None};

  NimBLEClient* client_ = nullptr;
  NimBLEAdvertisedDevice* targetDevice_ = nullptr;
  std::string deviceName_;
  bool scanComplete_ = false;
  bool connectionPending_ = false;
};
