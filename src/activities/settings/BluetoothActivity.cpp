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

  requestUpdate();
}

void BluetoothActivity::loop() {
  // Poll BLE state changes and re-render when state changes
  if (ble_.isEnabled()) {
    ble_.update();
    int currentState = static_cast<int>(ble_.getState());
    if (currentState != lastRenderedState_) {
      requestUpdate();
    }
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    SETTINGS.saveToFile();
    onComplete_();
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    if (!ble_.isEnabled()) {
      // Enable and initialise BLE
      SETTINGS.bluetoothEnabled = 1;
      ble_.begin();
      ble_.startScan(15);
      SETTINGS.saveToFile();
    } else if (ble_.isConnected()) {
      // Disconnect
      ble_.disconnect();
    } else if (ble_.getState() == BlePageTurner::State::Idle) {
      // Start scanning
      ble_.startScan(15);
    } else if (ble_.getState() == BlePageTurner::State::Scanning) {
      // Stop scanning
      ble_.stopScan();
    }
    requestUpdate();
    return;
  }

  // Long press Back to disable BLE entirely
  if (ble_.isEnabled() && mappedInput.isPressed(MappedInputManager::Button::Back) &&
      mappedInput.getHeldTime() >= 1500) {
    ble_.end();
    SETTINGS.bluetoothEnabled = 0;
    SETTINGS.saveToFile();
    onComplete_();
    return;
  }
}

void BluetoothActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  auto metrics = UITheme::getInstance().getMetrics();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_BT_PAGE_TURNER), "");

  const int contentY = metrics.topPadding + metrics.headerHeight + 20;
  const int lineHeight = 28;
  const int leftMargin = 20;
  int y = contentY;

  // Status section
  renderer.setFont(UI_12_FONT_ID);

  const char* statusText = "";
  const auto state = ble_.isEnabled() ? ble_.getState() : BlePageTurner::State::Disabled;

  switch (state) {
    case BlePageTurner::State::Disabled:
      statusText = tr(STR_BT_DISABLED);
      break;
    case BlePageTurner::State::Idle:
      statusText = tr(STR_BT_IDLE);
      break;
    case BlePageTurner::State::Scanning:
      statusText = tr(STR_BT_SCANNING);
      break;
    case BlePageTurner::State::Connecting:
      statusText = tr(STR_BT_CONNECTING);
      break;
    case BlePageTurner::State::Connected:
      statusText = tr(STR_BT_CONNECTED);
      break;
  }

  // Draw status label
  std::string statusLine = std::string(tr(STR_BT_STATUS)) + statusText;
  renderer.drawText(leftMargin, y, statusLine.c_str());
  y += lineHeight;

  // Draw device name if connected or was connected
  if (state == BlePageTurner::State::Connected || !ble_.getDeviceName().empty()) {
    std::string deviceLine = std::string(tr(STR_BT_DEVICE)) + ble_.getDeviceName();
    renderer.drawText(leftMargin, y, deviceLine.c_str());
    y += lineHeight;
  }

  y += lineHeight;

  // Instructions
  renderer.setFont(UI_10_FONT_ID);
  if (state == BlePageTurner::State::Disabled) {
    renderer.drawText(leftMargin, y, tr(STR_BT_PRESS_TO_ENABLE));
    y += lineHeight;
    renderer.drawText(leftMargin, y, tr(STR_BT_INSTRUCTION));
  } else if (state == BlePageTurner::State::Idle) {
    renderer.drawText(leftMargin, y, tr(STR_BT_PRESS_TO_SCAN));
    y += lineHeight;
    renderer.drawText(leftMargin, y, tr(STR_BT_HOLD_TO_DISABLE));
  } else if (state == BlePageTurner::State::Scanning) {
    renderer.drawText(leftMargin, y, tr(STR_BT_SCAN_IN_PROGRESS));
    y += lineHeight;
    renderer.drawText(leftMargin, y, tr(STR_BT_PRESS_TO_STOP));
  } else if (state == BlePageTurner::State::Connected) {
    renderer.drawText(leftMargin, y, tr(STR_BT_PRESS_TO_DISCONNECT));
    y += lineHeight;
    renderer.drawText(leftMargin, y, tr(STR_BT_HOLD_TO_DISABLE));
  } else if (state == BlePageTurner::State::Connecting) {
    renderer.drawText(leftMargin, y, tr(STR_BT_CONNECTING));
  }

  // Draw button hints
  const char* confirmLabel = "";
  switch (state) {
    case BlePageTurner::State::Disabled:
      confirmLabel = tr(STR_BT_ENABLE);
      break;
    case BlePageTurner::State::Idle:
      confirmLabel = tr(STR_BT_SCAN);
      break;
    case BlePageTurner::State::Scanning:
      confirmLabel = tr(STR_CANCEL);
      break;
    case BlePageTurner::State::Connected:
      confirmLabel = tr(STR_BT_DISCONNECT);
      break;
    case BlePageTurner::State::Connecting:
      confirmLabel = "";
      break;
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), confirmLabel, "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  lastRenderedState_ = static_cast<int>(state);

  renderer.displayBuffer();
}
