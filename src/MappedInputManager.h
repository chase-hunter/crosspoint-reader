#pragma once

#include <HalGPIO.h>

class BlePageTurner;

class MappedInputManager {
 public:
  enum class Button { Back, Confirm, Left, Right, Up, Down, Power, PageBack, PageForward };

  struct Labels {
    const char* btn1;
    const char* btn2;
    const char* btn3;
    const char* btn4;
  };

  explicit MappedInputManager(HalGPIO& gpio) : gpio(gpio) {}

  void update();
  bool wasPressed(Button button) const;
  bool wasReleased(Button button) const;
  bool isPressed(Button button) const;
  bool wasAnyPressed() const;
  bool wasAnyReleased() const;
  unsigned long getHeldTime() const;
  Labels mapLabels(const char* back, const char* confirm, const char* previous, const char* next) const;
  // Returns the raw front button index that was pressed this frame (or -1 if none).
  int getPressedFrontButton() const;

  /// Set BLE page turner source for virtual button events.
  void setBlePageTurner(BlePageTurner* ble) { blePageTurner = ble; }

  /// Returns true if a BLE event was consumed this frame.
  bool hadBleEvent() const { return bleEventThisFrame; }

 private:
  HalGPIO& gpio;
  BlePageTurner* blePageTurner = nullptr;

  // Virtual button state from BLE (set during update(), consumed by wasPressed/wasReleased)
  bool blePageForward = false;
  bool blePageBack = false;
  bool bleConfirm = false;
  bool bleBack = false;
  bool bleEventThisFrame = false;

  bool mapButton(Button button, bool (HalGPIO::*fn)(uint8_t) const) const;
};
