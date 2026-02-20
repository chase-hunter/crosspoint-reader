#include "BootActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <string>

#include "fontIds.h"

namespace {
void drawCrossPointLogo(const GfxRenderer& renderer, int x, int y, int size) {
  const int center = size / 2;

  // Outer frame – clean double border with rounded corners
  renderer.drawRoundedRect(x, y, size, size, 2, 6, true);
  renderer.drawRoundedRect(x + 5, y + 5, size - 10, size - 10, 1, 4, true);

  // Cross bars with gap at center (crosshair / reticle style)
  const int barW = 5;
  const int barInset = 22;
  const int gapR = 12;

  // Vertical bar – top half
  renderer.fillRect(x + center - barW / 2, y + barInset, barW, center - barInset - gapR);
  // Vertical bar – bottom half
  renderer.fillRect(x + center - barW / 2, y + center + gapR, barW, center - barInset - gapR);
  // Horizontal bar – left half
  renderer.fillRect(x + barInset, y + center - barW / 2, center - barInset - gapR, barW);
  // Horizontal bar – right half
  renderer.fillRect(x + center + gapR, y + center - barW / 2, center - barInset - gapR, barW);

  // Center point – filled square at the intersection
  const int ptSize = 11;
  renderer.fillRect(x + center - ptSize / 2, y + center - ptSize / 2, ptSize, ptSize);
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
  drawCrossPointLogo(renderer, logoX, logoY, logoSize);

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
