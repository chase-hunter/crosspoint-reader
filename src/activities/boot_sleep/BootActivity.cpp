#include "BootActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <string>

#include "fontIds.h"

namespace {
void drawChMonogram(const GfxRenderer& renderer, int x, int y, int size) {
  const int frame = size;
  const int outerStroke = 2;
  const int frameInset = 14;
  const int glyphX = x + frameInset;
  const int glyphY = y + frameInset;
  const int glyphW = frame - frameInset * 2;
  const int glyphH = frame - frameInset * 2;
  const int stroke = 7;

  // Outer and inner frame for a tech-style emblem
  renderer.drawRect(x, y, frame, frame);
  renderer.drawRect(x + outerStroke + 3, y + outerStroke + 3, frame - (outerStroke + 3) * 2,
                    frame - (outerStroke + 3) * 2);

  // Corner accents
  renderer.fillRect(x + 5, y + 5, 10, 3);
  renderer.fillRect(x + frame - 15, y + frame - 8, 10, 3);

  // Stylized "C"
  const int cW = glyphW / 2 - 2;
  renderer.fillRect(glyphX, glyphY, stroke, glyphH);
  renderer.fillRect(glyphX, glyphY, cW, stroke);
  renderer.fillRect(glyphX, glyphY + glyphH - stroke, cW, stroke);

  // Stylized "H" with stronger center bridge
  const int hX = glyphX + glyphW / 2 + 4;
  const int hW = glyphW / 2 - 4;
  renderer.fillRect(hX, glyphY, stroke, glyphH);
  renderer.fillRect(hX + hW - stroke, glyphY, stroke, glyphH);
  renderer.fillRect(hX, glyphY + glyphH / 2 - stroke / 2, hW, stroke + 1);

  // Diagonal cut accent for a flashier mark
  renderer.drawLine(glyphX + 4, glyphY + glyphH - 10, hX + hW - 2, glyphY + 8);
  renderer.drawLine(glyphX + 6, glyphY + glyphH - 8, hX + hW, glyphY + 10);
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

  const std::string titleLine1 = "CrossPoint Reworked";
  const std::string titleLine2 = "github.com/chase-hunter";
  const auto line1 = renderer.truncatedText(UI_10_FONT_ID, titleLine1.c_str(), pageWidth - 20, EpdFontFamily::BOLD);
  const auto line2 = renderer.truncatedText(SMALL_FONT_ID, titleLine2.c_str(), pageWidth - 20, EpdFontFamily::BOLD);

  const int line1Y = pageHeight / 2 + 70;
  const int line2Y = line1Y + 22;
  const int line3Y = line2Y + 18;

  renderer.drawCenteredText(UI_10_FONT_ID, line1Y, line1.c_str(), true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, line2Y, line2.c_str(), true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, line3Y, tr(STR_BOOTING));
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 30, CROSSPOINT_VERSION);
  renderer.displayBuffer();
}
