#include "MappedInputManager.h"

#include <BlePageTurner.h>

#include "CrossPointSettings.h"

namespace {
using ButtonIndex = uint8_t;

struct SideLayoutMap {
  ButtonIndex pageBack;
  ButtonIndex pageForward;
};

// Order matches CrossPointSettings::SIDE_BUTTON_LAYOUT.
constexpr SideLayoutMap kSideLayouts[] = {
    {HalGPIO::BTN_UP, HalGPIO::BTN_DOWN},
    {HalGPIO::BTN_DOWN, HalGPIO::BTN_UP},
};
}  // namespace

bool MappedInputManager::mapButton(const Button button, bool (HalGPIO::*fn)(uint8_t) const) const {
  const auto sideLayout = static_cast<CrossPointSettings::SIDE_BUTTON_LAYOUT>(SETTINGS.sideButtonLayout);
  const auto& side = kSideLayouts[sideLayout];

  switch (button) {
    case Button::Back:
      // Logical Back maps to user-configured front button.
      return (gpio.*fn)(SETTINGS.frontButtonBack);
    case Button::Confirm:
      // Logical Confirm maps to user-configured front button.
      return (gpio.*fn)(SETTINGS.frontButtonConfirm);
    case Button::Left:
      // Logical Left maps to user-configured front button.
      return (gpio.*fn)(SETTINGS.frontButtonLeft);
    case Button::Right:
      // Logical Right maps to user-configured front button.
      return (gpio.*fn)(SETTINGS.frontButtonRight);
    case Button::Up:
      // Side buttons remain fixed for Up/Down.
      return (gpio.*fn)(HalGPIO::BTN_UP);
    case Button::Down:
      // Side buttons remain fixed for Up/Down.
      return (gpio.*fn)(HalGPIO::BTN_DOWN);
    case Button::Power:
      // Power button bypasses remapping.
      return (gpio.*fn)(HalGPIO::BTN_POWER);
    case Button::PageBack:
      // Reader page navigation uses side buttons and can be swapped via settings.
      return (gpio.*fn)(side.pageBack);
    case Button::PageForward:
      // Reader page navigation uses side buttons and can be swapped via settings.
      return (gpio.*fn)(side.pageForward);
  }

  return false;
}

void MappedInputManager::update() {
  gpio.update();

  // Reset BLE virtual button state
  blePageForward = false;
  blePageBack = false;
  bleConfirm = false;
  bleBack = false;
  bleEventThisFrame = false;

  // Poll BLE page turner for events
  if (blePageTurner && blePageTurner->isConnected()) {
    blePageTurner->update();
    auto event = blePageTurner->consumeEvent();
    if (event != BlePageTurner::Event::None) {
      bleEventThisFrame = true;
      switch (event) {
        case BlePageTurner::Event::PageForward:
          blePageForward = true;
          break;
        case BlePageTurner::Event::PageBack:
          blePageBack = true;
          break;
        case BlePageTurner::Event::Confirm:
          bleConfirm = true;
          break;
        case BlePageTurner::Event::Back:
          bleBack = true;
          break;
        default:
          break;
      }
    }
  } else if (blePageTurner) {
    // Still call update to handle connection state transitions
    blePageTurner->update();
  }
}

bool MappedInputManager::wasPressed(const Button button) const {
  // Check BLE virtual buttons first (they act as press events)
  switch (button) {
    case Button::PageForward:
      if (blePageForward) return true;
      break;
    case Button::PageBack:
      if (blePageBack) return true;
      break;
    case Button::Confirm:
      if (bleConfirm) return true;
      break;
    case Button::Back:
      if (bleBack) return true;
      break;
    default:
      break;
  }
  return mapButton(button, &HalGPIO::wasPressed);
}

bool MappedInputManager::wasReleased(const Button button) const {
  // BLE events are instantaneous, so they trigger both press and release in the same frame.
  // This ensures they work regardless of whether the activity checks wasPressed or wasReleased.
  switch (button) {
    case Button::PageForward:
      if (blePageForward) return true;
      break;
    case Button::PageBack:
      if (blePageBack) return true;
      break;
    case Button::Confirm:
      if (bleConfirm) return true;
      break;
    case Button::Back:
      if (bleBack) return true;
      break;
    default:
      break;
  }
  return mapButton(button, &HalGPIO::wasReleased);
}

bool MappedInputManager::isPressed(const Button button) const { return mapButton(button, &HalGPIO::isPressed); }

bool MappedInputManager::wasAnyPressed() const { return gpio.wasAnyPressed() || bleEventThisFrame; }

bool MappedInputManager::wasAnyReleased() const { return gpio.wasAnyReleased() || bleEventThisFrame; }

unsigned long MappedInputManager::getHeldTime() const { return gpio.getHeldTime(); }

MappedInputManager::Labels MappedInputManager::mapLabels(const char* back, const char* confirm, const char* previous,
                                                         const char* next) const {
  // Build the label order based on the configured hardware mapping.
  auto labelForHardware = [&](uint8_t hw) -> const char* {
    // Compare against configured logical roles and return the matching label.
    if (hw == SETTINGS.frontButtonBack) {
      return back;
    }
    if (hw == SETTINGS.frontButtonConfirm) {
      return confirm;
    }
    if (hw == SETTINGS.frontButtonLeft) {
      return previous;
    }
    if (hw == SETTINGS.frontButtonRight) {
      return next;
    }
    return "";
  };

  return {labelForHardware(HalGPIO::BTN_BACK), labelForHardware(HalGPIO::BTN_CONFIRM),
          labelForHardware(HalGPIO::BTN_LEFT), labelForHardware(HalGPIO::BTN_RIGHT)};
}

int MappedInputManager::getPressedFrontButton() const {
  // Scan the raw front buttons in hardware order.
  // This bypasses remapping so the remap activity can capture physical presses.
  if (gpio.wasPressed(HalGPIO::BTN_BACK)) {
    return HalGPIO::BTN_BACK;
  }
  if (gpio.wasPressed(HalGPIO::BTN_CONFIRM)) {
    return HalGPIO::BTN_CONFIRM;
  }
  if (gpio.wasPressed(HalGPIO::BTN_LEFT)) {
    return HalGPIO::BTN_LEFT;
  }
  if (gpio.wasPressed(HalGPIO::BTN_RIGHT)) {
    return HalGPIO::BTN_RIGHT;
  }
  return -1;
}