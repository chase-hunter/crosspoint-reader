#pragma once

#include <functional>
#include <string>

#include "activities/Activity.h"

class BlePageTurner;

/**
 * Activity for scanning, pairing, and managing a BLE page turner.
 *
 * Shows the current BLE state and allows the user to:
 *   - Start scanning for devices
 *   - View the connected device name
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
  BlePageTurner& ble_;
  const std::function<void()> onComplete_;
  int lastRenderedState_ = -1;
};
