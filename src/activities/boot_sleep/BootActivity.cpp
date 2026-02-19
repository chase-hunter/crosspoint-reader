#include "BootActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <string>

#include "fontIds.h"

namespace {
void drawChMonogram(const GfxRenderer& renderer, int x, int y, int size) {
  const int frame = size;
  const int frameInset = 10;
  const int glyphX = x + frameInset;
  const int glyphY = y + frameInset;
  const int glyphW = frame - frameInset * 2;
  const int glyphH = frame - frameInset * 2;
  const int stroke = 8;

  renderer.drawRect(x, y, frame, frame);

  // "C"
  renderer.fillRect(glyphX, glyphY, stroke, glyphH);
  renderer.fillRect(glyphX, glyphY, glyphW / 2, stroke);
  renderer.fillRect(glyphX, glyphY + glyphH - stroke, glyphW / 2, stroke);

  // "H"
  const int hX = glyphX + glyphW / 2 + 6;
  const int hW = glyphW / 2 - 6;
  renderer.fillRect(hX, glyphY, stroke, glyphH);
  renderer.fillRect(hX + hW - stroke, glyphY, stroke, glyphH);
  renderer.fillRect(hX, glyphY + glyphH / 2 - stroke / 2, hW, stroke);
}
}  // namespace

void BootActivity::onEnter() {
  Activity::onEnter();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const int logoSize = 120;
  const int logoX = (pageWidth - logoSize) / 2;
  const int logoY = (pageHeight - logoSize) / 2;

  renderer.clearScreen();
  drawChMonogram(renderer, logoX, logoY, logoSize);

  const std::string bootTitle = "CrossPoint - github.com/chase-hunter";
  const auto title = renderer.truncatedText(UI_10_FONT_ID, bootTitle.c_str(), pageWidth - 20, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 + 70, title.c_str(), true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight / 2 + 95, tr(STR_BOOTING));
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 30, CROSSPOINT_VERSION);
  renderer.displayBuffer();
}
