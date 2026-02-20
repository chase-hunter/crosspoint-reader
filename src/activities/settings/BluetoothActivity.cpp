#include "BluetoothActivity.h"

#include <BlePageTurner.h>
#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

void BluetoothActivity::onEnter() {
  Activity::onEnter();

  // If BLE is not yet initialised but the setting is enabled, initialise it now
  if (SETTINGS.bluetoothEnabled && !ble_.isEnabled()) {
    ble_.begin();
  }

  selectedDeviceIndex_ = 0;
  requestUpdate();
}

void BluetoothActivity::loop() {
  const auto state = ble_.isEnabled() ? ble_.getState() : BlePageTurner::State::Disabled;

  // Poll BLE state changes and re-render when state or device count changes
  if (ble_.isEnabled()) {
    ble_.update();
    const int currentState = static_cast<int>(ble_.getState());
    const int deviceCount = static_cast<int>(ble_.getDiscoveredDevices().size());
    if (currentState != lastRenderedState_ || deviceCount != lastDeviceCount_) {
      requestUpdate();
    }
  }

  // --- Back button (all states) ---
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    // If entering a PIN, dismiss and go back to device list
    if (state == BlePageTurner::State::PinEntry) {
      ble_.dismissPinEntry();
      for (int i = 0; i < 6; i++) pinDigits_[i] = 0;
      pinCursorPos_ = 0;
      requestUpdate();
      return;
    }
    // If viewing scan results, dismiss them and go back to idle
    if (state == BlePageTurner::State::ScanComplete) {
      ble_.dismissScanResults();
      requestUpdate();
      return;
    }
    SETTINGS.saveToFile();
    onComplete_();
    return;
  }

  // --- Long press Back to disable BLE entirely ---
  if (ble_.isEnabled() && mappedInput.isPressed(MappedInputManager::Button::Back) &&
      mappedInput.getHeldTime() >= 1500) {
    ble_.end();
    SETTINGS.bluetoothEnabled = 0;
    SETTINGS.saveToFile();
    onComplete_();
    return;
  }

  // --- State-specific input handling ---
  if (state == BlePageTurner::State::Disabled) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
      SETTINGS.bluetoothEnabled = 1;
      ble_.begin();
      ble_.startScan(15);
      SETTINGS.saveToFile();
      requestUpdate();
    }
  } else if (state == BlePageTurner::State::Idle) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
      ble_.startScan(15);
      selectedDeviceIndex_ = 0;
      requestUpdate();
    }
  } else if (state == BlePageTurner::State::Scanning) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
      ble_.stopScan();
      requestUpdate();
    }
  } else if (state == BlePageTurner::State::ScanComplete) {
    const auto& devices = ble_.getDiscoveredDevices();
    const int listSize = static_cast<int>(devices.size());

    if (listSize > 0) {
      // Confirm selects the highlighted device
      if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
        for (int i = 0; i < 6; i++) pinDigits_[i] = 0;
        pinCursorPos_ = 0;
        ble_.connectToDeviceByIndex(static_cast<size_t>(selectedDeviceIndex_));
        requestUpdate();
        return;
      }

      // Navigate the device list with Up/Down
      buttonNavigator_.onNext([this, listSize] {
        selectedDeviceIndex_ = ButtonNavigator::nextIndex(selectedDeviceIndex_, listSize);
        requestUpdate();
      });
      buttonNavigator_.onPrevious([this, listSize] {
        selectedDeviceIndex_ = ButtonNavigator::previousIndex(selectedDeviceIndex_, listSize);
        requestUpdate();
      });

      // Right button rescans
      if (mappedInput.wasPressed(MappedInputManager::Button::Right)) {
        ble_.startScan(15);
        selectedDeviceIndex_ = 0;
        requestUpdate();
      }
    } else {
      // No devices found - Confirm rescans
      if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
        ble_.startScan(15);
        selectedDeviceIndex_ = 0;
        requestUpdate();
      }
    }
  } else if (state == BlePageTurner::State::PinEntry) {
    // Up/Down to change digit value
    if (mappedInput.wasPressed(MappedInputManager::Button::Up)) {
      pinDigits_[pinCursorPos_] = (pinDigits_[pinCursorPos_] + 1) % 10;
      requestUpdate();
    }
    if (mappedInput.wasPressed(MappedInputManager::Button::Down)) {
      pinDigits_[pinCursorPos_] = (pinDigits_[pinCursorPos_] + 9) % 10;
      requestUpdate();
    }
    // Left: move cursor backwards
    if (mappedInput.wasPressed(MappedInputManager::Button::Left)) {
      if (pinCursorPos_ > 0) {
        pinCursorPos_--;
        requestUpdate();
      }
    }
    // Confirm: advance cursor or submit PIN on last digit
    if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
      if (pinCursorPos_ < 5) {
        pinCursorPos_++;
        requestUpdate();
      } else {
        // Submit PIN: compute numeric value and connect
        uint32_t pin = 0;
        for (int i = 0; i < 6; i++) {
          pin = pin * 10 + static_cast<uint32_t>(pinDigits_[i]);
        }
        BlePageTurner::setSecurityPasskey(pin);
        ble_.connectPendingDevice();
        requestUpdate();
      }
    }
    // Right: skip PIN and connect without passkey
    if (mappedInput.wasPressed(MappedInputManager::Button::Right)) {
      BlePageTurner::setSecurityPasskey(0);
      ble_.connectPendingDevice();
      requestUpdate();
    }
  } else if (state == BlePageTurner::State::Connected) {
    if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
      ble_.disconnect();
      requestUpdate();
    }
  }
}

void BluetoothActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto state = ble_.isEnabled() ? ble_.getState() : BlePageTurner::State::Disabled;

  auto metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_BT_PAGE_TURNER), "");

  switch (state) {
    case BlePageTurner::State::Disabled:
      renderDisabled();
      break;
    case BlePageTurner::State::Scanning:
      renderScanning();
      break;
    case BlePageTurner::State::ScanComplete:
      renderDeviceList();
      break;
    case BlePageTurner::State::PinEntry:
      renderPinEntry();
      break;
    case BlePageTurner::State::Connecting:
      renderConnecting();
      break;
    case BlePageTurner::State::Connected:
      renderConnected();
      break;
    case BlePageTurner::State::Idle:
      renderDisabled();  // Same layout — prompts user to scan
      break;
  }

  lastRenderedState_ = static_cast<int>(state);
  lastDeviceCount_ = static_cast<int>(ble_.getDiscoveredDevices().size());

  renderer.displayBuffer();
}

void BluetoothActivity::renderDisabled() const {
  const auto pageHeight = renderer.getScreenHeight();
  const auto lineHeight = renderer.getLineHeight(UI_10_FONT_ID);
  const auto top = (pageHeight - lineHeight * 3) / 2;

  if (!ble_.isEnabled()) {
    renderer.drawCenteredText(UI_12_FONT_ID, top, tr(STR_BT_DISABLED), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_10_FONT_ID, top + lineHeight + 15, tr(STR_BT_PRESS_TO_ENABLE));
    renderer.drawCenteredText(UI_10_FONT_ID, top + lineHeight * 2 + 25, tr(STR_BT_INSTRUCTION));

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_BT_ENABLE), "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  } else {
    // Idle state — BLE is on but not scanning
    renderer.drawCenteredText(UI_12_FONT_ID, top, tr(STR_BT_IDLE), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_10_FONT_ID, top + lineHeight + 15, tr(STR_BT_PRESS_TO_SCAN));
    renderer.drawCenteredText(UI_10_FONT_ID, top + lineHeight * 2 + 25, tr(STR_BT_HOLD_TO_DISABLE));

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_BT_SCAN), "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }
}

void BluetoothActivity::renderScanning() const {
  const auto pageHeight = renderer.getScreenHeight();
  const auto lineHeight = renderer.getLineHeight(UI_10_FONT_ID);
  const auto top = (pageHeight - lineHeight * 2) / 2;

  renderer.drawCenteredText(UI_12_FONT_ID, top, tr(STR_BT_SCAN_IN_PROGRESS), true, EpdFontFamily::BOLD);

  const int deviceCount = static_cast<int>(ble_.getDiscoveredDevices().size());
  char countBuf[32];
  snprintf(countBuf, sizeof(countBuf), "%s%d", tr(STR_BT_DEVICES_FOUND), deviceCount);
  renderer.drawCenteredText(UI_10_FONT_ID, top + lineHeight + 15, countBuf);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_CANCEL), "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void BluetoothActivity::renderDeviceList() const {
  const auto& devices = ble_.getDiscoveredDevices();
  auto metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  if (devices.empty()) {
    // No devices found
    const auto lineHeight = renderer.getLineHeight(UI_10_FONT_ID);
    const auto top = (pageHeight - lineHeight * 2) / 2;
    renderer.drawCenteredText(UI_12_FONT_ID, top, tr(STR_BT_NO_DEVICES), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_10_FONT_ID, top + lineHeight + 15, tr(STR_BT_PRESS_TO_RESCAN));

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_RETRY), "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    return;
  }

  // Draw scrollable device list using the theme's drawList
  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing * 2;

  GUI.drawList(
      renderer, Rect{0, contentTop, pageWidth, contentHeight}, static_cast<int>(devices.size()),
      selectedDeviceIndex_,
      [&devices](int index) -> std::string { return devices[index].name; },  // rowTitle
      nullptr,                                                                // rowSubtitle
      nullptr,                                                                // rowIcon
      [&devices](int index) -> std::string {                                  // rowValue (RSSI)
        char buf[16];
        snprintf(buf, sizeof(buf), "%d dBm", devices[index].rssi);
        return buf;
      });

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void BluetoothActivity::renderPinEntry() const {
  auto metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const auto lineHeight = renderer.getLineHeight(UI_10_FONT_ID);
  const auto digitHeight = renderer.getLineHeight(UI_12_FONT_ID);

  // Device name
  int y = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing * 2;
  renderer.drawCenteredText(UI_10_FONT_ID, y, ble_.getDeviceName().c_str());
  y += lineHeight + metrics.verticalSpacing;

  // Instruction
  renderer.drawCenteredText(UI_10_FONT_ID, y, tr(STR_BT_ENTER_PIN));
  y += lineHeight + metrics.verticalSpacing * 3;

  // Draw PIN digit boxes
  const int digitW = renderer.getTextWidth(UI_12_FONT_ID, "0", EpdFontFamily::BOLD);
  const int pad = 6;
  const int boxW = digitW + pad * 2;
  const int boxH = digitHeight + pad * 2;
  const int gap = 6;
  const int totalW = 6 * boxW + 5 * gap;
  const int startX = (pageWidth - totalW) / 2;

  for (int i = 0; i < 6; i++) {
    char digit[2] = {static_cast<char>('0' + pinDigits_[i]), '\0'};
    const int bx = startX + i * (boxW + gap);

    if (i == pinCursorPos_) {
      // Selected: filled box with white text
      renderer.fillRect(bx, y, boxW, boxH);
      renderer.drawText(UI_12_FONT_ID, bx + pad, y + pad, digit, false, EpdFontFamily::BOLD);
    } else {
      // Normal: outlined box with black text
      renderer.drawRect(bx, y, boxW, boxH);
      renderer.drawText(UI_12_FONT_ID, bx + pad, y + pad, digit, true, EpdFontFamily::BOLD);
    }
  }

  y += boxH + metrics.verticalSpacing * 2;

  // Skip instruction
  renderer.drawCenteredText(UI_10_FONT_ID, y, tr(STR_BT_SKIP_PIN));

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_CONFIRM), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void BluetoothActivity::renderConnecting() const {
  const auto pageHeight = renderer.getScreenHeight();
  const auto lineHeight = renderer.getLineHeight(UI_10_FONT_ID);
  const auto top = (pageHeight - lineHeight * 2) / 2;

  renderer.drawCenteredText(UI_12_FONT_ID, top, tr(STR_BT_CONNECTING), true, EpdFontFamily::BOLD);

  if (!ble_.getDeviceName().empty()) {
    renderer.drawCenteredText(UI_10_FONT_ID, top + lineHeight + 15, ble_.getDeviceName().c_str());
  }
}

void BluetoothActivity::renderConnected() const {
  const auto pageHeight = renderer.getScreenHeight();
  const auto lineHeight = renderer.getLineHeight(UI_10_FONT_ID);
  const auto top = (pageHeight - lineHeight * 3) / 2;

  renderer.drawCenteredText(UI_12_FONT_ID, top, tr(STR_BT_CONNECTED), true, EpdFontFamily::BOLD);

  std::string deviceLine = std::string(tr(STR_BT_DEVICE)) + ble_.getDeviceName();
  renderer.drawCenteredText(UI_10_FONT_ID, top + lineHeight + 15, deviceLine.c_str());

  renderer.drawCenteredText(UI_10_FONT_ID, top + lineHeight * 2 + 25, tr(STR_BT_HOLD_TO_DISABLE));

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_BT_DISCONNECT), "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}
