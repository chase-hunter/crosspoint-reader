#pragma once

#include <functional>
#include <string>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class BlePageTurner;

/**
 * Activity for scanning, pairing, and managing a BLE page turner.
 *
 * Shows the current BLE state and allows the user to:
 *   - Enable/disable Bluetooth
 *   - Scan for nearby HID devices
 *   - Browse a scrollable list of discovered devices
 *   - Select a device to pair with
 *   - Disconnect the current device
 */
class BluetoothActivity final : public Activity {
 public:
  explicit BluetoothActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, BlePageTurner& ble,
                             const std::function<void()>& onComplete)
      : Activity("Bluetooth", renderer, mappedInput), ble_(ble), onComplete_(onComplete) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void renderDisabled() const;
  void renderScanning() const;
  void renderDeviceList() const;
  void renderConnecting() const;
  void renderConnected() const;

  BlePageTurner& ble_;
  const std::function<void()> onComplete_;
  int lastRenderedState_ = -1;
  int lastDeviceCount_ = -1;
  int selectedDeviceIndex_ = 0;
  ButtonNavigator buttonNavigator_;
};
