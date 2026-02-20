#include "LyraReloadedTheme.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Utf8.h>

#include <cmath>
#include <cstdint>
#include <string>

#include "Battery.h"
#include "CrossPointSettings.h"
#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/icons/book.h"
#include "components/icons/book24.h"
#include "components/icons/cover.h"
#include "components/icons/file24.h"
#include "components/icons/folder.h"
#include "components/icons/folder24.h"
#include "components/icons/hotspot.h"
#include "components/icons/image24.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/text24.h"
#include "components/icons/transfer.h"
#include "components/icons/wifi.h"
#include "fontIds.h"

// ============================================================================
//  Internal constants
// ============================================================================
namespace {
constexpr int kBatteryPercentSpacing = 4;
constexpr int kHPad = 10;            // horizontal padding inside selection pills
constexpr int kCornerRadius = 10;    // Apple-style generous rounding
constexpr int kCardRadius = 12;      // card container radius
constexpr int kPillRadius = 18;      // pill button radius
constexpr int kTopHintButtonY = 345;
constexpr int kPopupMarginX = 20;
constexpr int kPopupMarginY = 14;
constexpr int kMaxSubtitleWidth = 110;
constexpr int kMaxListValueWidth = 200;
constexpr int kMainMenuIconSize = 32;
constexpr int kListIconSize = 24;
constexpr int kCircleProgressRadius = 22;  // outer radius of progress ring
constexpr int kCircleProgressStroke = 4;   // ring stroke width
constexpr int kCoverGap = 10;              // gap between cover tiles

int sCoverWidth = 0;  // cached cover aspect-derived width

// Icon lookup helper — identical to Lyra
const uint8_t* iconForName(UIIcon icon, int size) {
  if (size == 24) {
    switch (icon) {
      case UIIcon::Folder: return Folder24Icon;
      case UIIcon::Text:   return Text24Icon;
      case UIIcon::Image:  return Image24Icon;
      case UIIcon::Book:   return Book24Icon;
      case UIIcon::File:   return File24Icon;
      default: return nullptr;
    }
  } else if (size == 32) {
    switch (icon) {
      case UIIcon::Folder:   return FolderIcon;
      case UIIcon::Book:     return BookIcon;
      case UIIcon::Recent:   return RecentIcon;
      case UIIcon::Settings: return Settings2Icon;
      case UIIcon::Transfer: return TransferIcon;
      case UIIcon::Library:  return LibraryIcon;
      case UIIcon::Wifi:     return WifiIcon;
      case UIIcon::Hotspot:  return HotspotIcon;
      default: return nullptr;
    }
  }
  return nullptr;
}
}  // namespace

// ============================================================================
//  Dark mode helpers
// ============================================================================

bool lyraReloaded_isDarkMode() {
  return SETTINGS.darkMode != 0;
}

// fg / bg follow e-ink convention:
//   Normal mode : fg=true (black pixels), bg=false (white pixels)
//   Dark mode   : fg=false (white pixels), bg=true (black pixels)
bool LyraReloadedTheme::fg() const { return !lyraReloaded_isDarkMode(); }
bool LyraReloadedTheme::bg() const { return lyraReloaded_isDarkMode(); }
Color LyraReloadedTheme::fgColor() const { return lyraReloaded_isDarkMode() ? Color::White : Color::Black; }
Color LyraReloadedTheme::bgColor() const { return lyraReloaded_isDarkMode() ? Color::Black : Color::White; }
Color LyraReloadedTheme::selColor() const { return lyraReloaded_isDarkMode() ? Color::DarkGray : Color::LightGray; }

// ============================================================================
//  Circle drawing helpers
// ============================================================================

void LyraReloadedTheme::drawCircleRing(const GfxRenderer& renderer, int cx, int cy,
                                        int outerR, int innerR, bool state) const {
  const int outerSq = outerR * outerR;
  const int innerSq = innerR * innerR;
  for (int dy = -outerR; dy <= outerR; ++dy) {
    for (int dx = -outerR; dx <= outerR; ++dx) {
      int dSq = dx * dx + dy * dy;
      if (dSq <= outerSq && dSq >= innerSq) {
        renderer.drawPixel(cx + dx, cy + dy, state);
      }
    }
  }
}

// Draw a partial circle arc from 12-o'clock clockwise to `percent`%.
// The coordinate system: angle 0 = 12 o'clock, increases clockwise.
void LyraReloadedTheme::drawCircleProgress(const GfxRenderer& renderer, int cx, int cy,
                                            int outerR, int innerR, int percent,
                                            bool state) const {
  if (percent <= 0) return;
  if (percent > 100) percent = 100;

  const int outerSq = outerR * outerR;
  const int innerSq = innerR * innerR;
  // Convert percent to angle in degrees (0-360), starting from top, clockwise
  const double endAngle = percent * 360.0 / 100.0;

  for (int dy = -outerR; dy <= outerR; ++dy) {
    for (int dx = -outerR; dx <= outerR; ++dx) {
      int dSq = dx * dx + dy * dy;
      if (dSq > outerSq || dSq < innerSq) continue;

      // Compute angle from 12-o'clock clockwise.
      // atan2 gives angle from positive-x axis counter-clockwise.
      // We want angle from negative-y axis clockwise.
      double angle = atan2(static_cast<double>(dx), static_cast<double>(-dy));
      if (angle < 0) angle += 2.0 * M_PI;
      double angleDeg = angle * 180.0 / M_PI;

      if (angleDeg <= endAngle) {
        renderer.drawPixel(cx + dx, cy + dy, state);
      }
    }
  }
}

// ============================================================================
//  Battery
// ============================================================================

static void drawBatteryIconReloaded(const GfxRenderer& renderer, int x, int y,
                                     int battWidth, int rectHeight,
                                     uint16_t percentage, bool state) {
  // Rounded battery outline
  renderer.drawLine(x + 2, y, x + battWidth - 4, y, state);
  renderer.drawLine(x + 2, y + rectHeight - 1, x + battWidth - 4, y + rectHeight - 1, state);
  renderer.drawLine(x, y + 2, x, y + rectHeight - 3, state);
  renderer.drawLine(x + battWidth - 2, y + 1, x + battWidth - 2, y + rectHeight - 2, state);
  // Round corners
  renderer.drawPixel(x + 1, y + 1, state);
  renderer.drawPixel(x + 1, y + rectHeight - 2, state);
  renderer.drawPixel(x + battWidth - 3, y + 1, state);
  renderer.drawPixel(x + battWidth - 3, y + rectHeight - 2, state);
  // Tip
  renderer.drawPixel(x + battWidth - 1, y + 3, state);
  renderer.drawPixel(x + battWidth - 1, y + rectHeight - 4, state);
  renderer.drawLine(x + battWidth, y + 4, x + battWidth, y + rectHeight - 5, state);

  // Segmented fill (3 bars)
  if (percentage > 10) renderer.fillRect(x + 2, y + 2, 3, rectHeight - 4, state);
  if (percentage > 40) renderer.fillRect(x + 6, y + 2, 3, rectHeight - 4, state);
  if (percentage > 70) renderer.fillRect(x + 10, y + 2, 3, rectHeight - 4, state);
}

void LyraReloadedTheme::drawBatteryLeft(const GfxRenderer& renderer, Rect rect,
                                         const bool showPercentage) const {
  const uint16_t percentage = battery.readPercentage();
  const int y = rect.y + 6;
  const int battWidth = LyraReloadedMetrics::values.batteryWidth;

  if (showPercentage) {
    const auto txt = std::to_string(percentage) + "%";
    renderer.drawText(SMALL_FONT_ID, rect.x + kBatteryPercentSpacing + battWidth, rect.y,
                      txt.c_str(), fg());
  }
  drawBatteryIconReloaded(renderer, rect.x, y, battWidth, rect.height, percentage, fg());
}

void LyraReloadedTheme::drawBatteryRight(const GfxRenderer& renderer, Rect rect,
                                          const bool showPercentage) const {
  const uint16_t percentage = battery.readPercentage();
  const int y = rect.y + 6;
  const int battWidth = LyraReloadedMetrics::values.batteryWidth;

  if (showPercentage) {
    const auto txt = std::to_string(percentage) + "%";
    const int tw = renderer.getTextWidth(SMALL_FONT_ID, txt.c_str());
    const int th = renderer.getTextHeight(SMALL_FONT_ID);
    renderer.fillRect(rect.x - tw - kBatteryPercentSpacing, rect.y, tw, th, bg());
    renderer.drawText(SMALL_FONT_ID, rect.x - tw - kBatteryPercentSpacing, rect.y,
                      txt.c_str(), fg());
  }
  drawBatteryIconReloaded(renderer, rect.x, y, battWidth, rect.height, percentage, fg());
}

// ============================================================================
//  Header — minimal, Apple-style hairline divider
// ============================================================================

void LyraReloadedTheme::drawHeader(const GfxRenderer& renderer, Rect rect,
                                    const char* title, const char* subtitle) const {
  // Clear header area
  renderer.fillRect(rect.x, rect.y, rect.width, rect.height, bg());

  const bool showBatt =
      SETTINGS.hideBatteryPercentage != CrossPointSettings::HIDE_BATTERY_PERCENTAGE::HIDE_ALWAYS;
  const int batteryX = rect.x + rect.width - 14 - LyraReloadedMetrics::values.batteryWidth;
  drawBatteryRight(
      renderer,
      Rect{batteryX, rect.y + 5, LyraReloadedMetrics::values.batteryWidth,
           LyraReloadedMetrics::values.batteryHeight},
      showBatt);

  int maxTitleW =
      rect.width - LyraReloadedMetrics::values.contentSidePadding * 2 -
      (subtitle ? kMaxSubtitleWidth : 0);

  if (title) {
    auto trunc = renderer.truncatedText(UI_12_FONT_ID, title, maxTitleW, EpdFontFamily::BOLD);
    renderer.drawText(UI_12_FONT_ID, rect.x + LyraReloadedMetrics::values.contentSidePadding,
                      rect.y + LyraReloadedMetrics::values.batteryBarHeight + 2,
                      trunc.c_str(), fg(), EpdFontFamily::BOLD);
  }

  if (subtitle) {
    auto trunc = renderer.truncatedText(SMALL_FONT_ID, subtitle, kMaxSubtitleWidth);
    int sw = renderer.getTextWidth(SMALL_FONT_ID, trunc.c_str());
    renderer.drawText(SMALL_FONT_ID,
                      rect.x + rect.width - LyraReloadedMetrics::values.contentSidePadding - sw,
                      rect.y + LyraReloadedMetrics::values.batteryBarHeight + 8,
                      trunc.c_str(), fg());
  }

  // Hairline separator (Apple-style thin divider)
  renderer.drawLine(rect.x + LyraReloadedMetrics::values.contentSidePadding,
                    rect.y + rect.height - 1,
                    rect.x + rect.width - LyraReloadedMetrics::values.contentSidePadding,
                    rect.y + rect.height - 1, fg());
}

// ============================================================================
//  Sub-header
// ============================================================================

void LyraReloadedTheme::drawSubHeader(const GfxRenderer& renderer, Rect rect,
                                       const char* label,
                                       const char* rightLabel) const {
  int currentX = rect.x + LyraReloadedMetrics::values.contentSidePadding;
  int rightSpace = LyraReloadedMetrics::values.contentSidePadding;

  if (rightLabel) {
    auto trunc = renderer.truncatedText(SMALL_FONT_ID, rightLabel, kMaxListValueWidth);
    int rw = renderer.getTextWidth(SMALL_FONT_ID, trunc.c_str());
    renderer.drawText(SMALL_FONT_ID,
                      rect.x + rect.width - LyraReloadedMetrics::values.contentSidePadding - rw,
                      rect.y + 7, trunc.c_str(), fg());
    rightSpace += rw + kHPad;
  }

  auto trunc = renderer.truncatedText(
      UI_10_FONT_ID, label,
      rect.width - LyraReloadedMetrics::values.contentSidePadding - rightSpace);
  renderer.drawText(UI_10_FONT_ID, currentX, rect.y + 6, trunc.c_str(), fg());

  // Hairline
  renderer.drawLine(rect.x + LyraReloadedMetrics::values.contentSidePadding,
                    rect.y + rect.height - 1,
                    rect.x + rect.width - LyraReloadedMetrics::values.contentSidePadding,
                    rect.y + rect.height - 1, fg());
}

// ============================================================================
//  Tab bar — iOS-style segmented control with rounded pill selection
// ============================================================================

void LyraReloadedTheme::drawTabBar(const GfxRenderer& renderer, Rect rect,
                                    const std::vector<TabInfo>& tabs,
                                    bool selected) const {
  const auto& m = LyraReloadedMetrics::values;

  // Draw a rounded background track (frosted gray) spanning the whole bar
  if (selected) {
    renderer.fillRectDither(rect.x + m.contentSidePadding, rect.y,
                            rect.width - m.contentSidePadding * 2, rect.height,
                            selColor());
  }

  int currentX = rect.x + m.contentSidePadding + kHPad;

  for (const auto& tab : tabs) {
    const int tw = renderer.getTextWidth(UI_10_FONT_ID, tab.label);

    if (tab.selected) {
      if (selected) {
        // Active pill
        renderer.fillRoundedRect(currentX - kHPad / 2, rect.y + 2, tw + kHPad,
                                 rect.height - 4, (rect.height - 4) / 2, fgColor());
      } else {
        // Underline indicator
        renderer.fillRectDither(currentX - kHPad / 2, rect.y, tw + kHPad, rect.height - 3,
                                selColor());
        renderer.drawLine(currentX - kHPad / 2, rect.y + rect.height - 3,
                          currentX - kHPad / 2 + tw + kHPad, rect.y + rect.height - 3, 2,
                          fg());
      }
    }

    renderer.drawText(UI_10_FONT_ID, currentX, rect.y + 7, tab.label,
                      !(tab.selected && selected));

    currentX += tw + m.tabSpacing + kHPad;
  }

  // Bottom hairline
  renderer.drawLine(rect.x + m.contentSidePadding, rect.y + rect.height - 1,
                    rect.x + rect.width - m.contentSidePadding, rect.y + rect.height - 1,
                    fg());
}

// ============================================================================
//  List — rounded selection pill, icons, scroll indicator
// ============================================================================

void LyraReloadedTheme::drawList(const GfxRenderer& renderer, Rect rect, int itemCount,
                                  int selectedIndex,
                                  const std::function<std::string(int)>& rowTitle,
                                  const std::function<std::string(int)>& rowSubtitle,
                                  const std::function<UIIcon(int)>& rowIcon,
                                  const std::function<std::string(int)>& rowValue,
                                  bool highlightValue) const {
  const auto& m = LyraReloadedMetrics::values;
  int rowHeight = (rowSubtitle) ? m.listWithSubtitleRowHeight : m.listRowHeight;
  int pageItems = rect.height / rowHeight;

  const int totalPages = (itemCount + pageItems - 1) / pageItems;

  // --- Scroll bar ---
  if (totalPages > 1) {
    const int scrollH = rect.height;
    const int barH = (scrollH * pageItems) / itemCount;
    const int page = selectedIndex / pageItems;
    const int barY = rect.y + ((scrollH - barH) * page) / (totalPages - 1);
    const int barX = rect.x + rect.width - m.scrollBarRightOffset;
    // Track
    renderer.drawLine(barX, rect.y, barX, rect.y + scrollH, fg());
    // Thumb (filled pill)
    renderer.fillRect(barX - m.scrollBarWidth, barY, m.scrollBarWidth, barH, fg());
  }

  int contentW =
      rect.width - (totalPages > 1 ? (m.scrollBarWidth + m.scrollBarRightOffset) : 1);

  // --- Selection highlight (rounded pill) ---
  if (selectedIndex >= 0) {
    renderer.fillRoundedRect(m.contentSidePadding,
                             rect.y + selectedIndex % pageItems * rowHeight,
                             contentW - m.contentSidePadding * 2, rowHeight, kCornerRadius,
                             selColor());
  }

  int textX = rect.x + m.contentSidePadding + kHPad;
  int textW = contentW - m.contentSidePadding * 2 - kHPad * 2;
  int iconSize = 0;
  if (rowIcon) {
    iconSize = (rowSubtitle) ? kMainMenuIconSize : kListIconSize;
    textX += iconSize + kHPad;
    textW -= iconSize + kHPad;
  }

  const auto pageStart = selectedIndex / pageItems * pageItems;
  int iconYOff = (rowSubtitle) ? 16 : 10;

  for (int i = pageStart; i < itemCount && i < pageStart + pageItems; ++i) {
    const int itemY = rect.y + (i % pageItems) * rowHeight;
    int rowTextW = textW;

    // Value column
    int valW = 0;
    std::string valText;
    if (rowValue) {
      valText = rowValue(i);
      valText = renderer.truncatedText(UI_10_FONT_ID, valText.c_str(), kMaxListValueWidth);
      valW = renderer.getTextWidth(UI_10_FONT_ID, valText.c_str()) + kHPad;
      rowTextW -= valW;
    }

    // Row title
    auto name = rowTitle(i);
    auto item = renderer.truncatedText(UI_10_FONT_ID, name.c_str(), rowTextW);
    renderer.drawText(UI_10_FONT_ID, textX, itemY + 8, item.c_str(), fg());

    // Row icon
    if (rowIcon) {
      const uint8_t* bmp = iconForName(rowIcon(i), iconSize);
      if (bmp) {
        renderer.drawIcon(bmp, rect.x + m.contentSidePadding + kHPad,
                          itemY + iconYOff, iconSize, iconSize);
      }
    }

    // Subtitle
    if (rowSubtitle) {
      auto sub = rowSubtitle(i);
      auto truncSub = renderer.truncatedText(SMALL_FONT_ID, sub.c_str(), rowTextW);
      renderer.drawText(SMALL_FONT_ID, textX, itemY + 30, truncSub.c_str(), fg());
    }

    // Value (with optional highlight pill)
    if (!valText.empty()) {
      if (i == selectedIndex && highlightValue) {
        renderer.fillRoundedRect(
            contentW - m.contentSidePadding - kHPad - valW, itemY,
            valW + kHPad, rowHeight, kCornerRadius, fgColor());
      }
      renderer.drawText(UI_10_FONT_ID,
                        rect.x + contentW - m.contentSidePadding - valW, itemY + 8,
                        valText.c_str(), !(i == selectedIndex && highlightValue));
    }

    // Hairline separator between rows (except last visible)
    if (i < pageStart + pageItems - 1 && i < itemCount - 1) {
      int lineY = itemY + rowHeight - 1;
      renderer.drawLine(rect.x + m.contentSidePadding + kHPad, lineY,
                        rect.x + contentW - m.contentSidePadding, lineY, fg());
    }
  }
}

// ============================================================================
//  Button hints — pill-shaped, bottom of screen
// ============================================================================

void LyraReloadedTheme::drawButtonHints(GfxRenderer& renderer, const char* btn1,
                                         const char* btn2, const char* btn3,
                                         const char* btn4) const {
  const auto origOri = renderer.getOrientation();
  renderer.setOrientation(GfxRenderer::Orientation::Portrait);

  const int pageH = renderer.getScreenHeight();
  constexpr int btnW = 78;
  constexpr int smallH = 8;
  constexpr int allocH = LyraReloadedMetrics::values.buttonHintsHeight;
  constexpr int drawnH = 26;
  constexpr int radius = drawnH / 2;  // full pill
  constexpr int topPad = (allocH - drawnH) / 2;
  constexpr int positions[] = {58, 146, 254, 342};
  const char* labels[] = {btn1, btn2, btn3, btn4};

  for (int i = 0; i < 4; ++i) {
    const int x = positions[i];
    if (labels[i] && labels[i][0] != '\0') {
      const int y = pageH - allocH + topPad;
      renderer.fillRoundedRect(x, y, btnW, drawnH, radius, bgColor());
      renderer.drawRoundedRect(x, y, btnW, drawnH, 2, radius, fg());
      const int tw = renderer.getTextWidth(SMALL_FONT_ID, labels[i]);
      const int lh = renderer.getLineHeight(SMALL_FONT_ID);
      renderer.drawText(SMALL_FONT_ID, x + (btnW - tw) / 2, y + (drawnH - lh) / 2,
                        labels[i], fg());
    } else {
      // Tiny placeholder pill
      renderer.fillRoundedRect(x, pageH - smallH, btnW, smallH, smallH / 2, bgColor());
      renderer.drawRoundedRect(x, pageH - smallH, btnW, smallH, 1, smallH / 2, fg());
    }
  }

  renderer.setOrientation(origOri);
}

// ============================================================================
//  Side button hints — rounded outlines
// ============================================================================

void LyraReloadedTheme::drawSideButtonHints(const GfxRenderer& renderer,
                                             const char* topBtn,
                                             const char* bottomBtn) const {
  const int sw = renderer.getScreenWidth();
  constexpr int btnW = LyraReloadedMetrics::values.sideButtonHintsWidth;
  constexpr int btnH = 76;
  const char* labels[] = {topBtn, bottomBtn};
  const int x = sw - btnW;

  for (int i = 0; i < 2; ++i) {
    if (labels[i] && labels[i][0] != '\0') {
      int y = kTopHintButtonY + i * (btnH + 6);
      renderer.drawRoundedRect(x, y, btnW, btnH, 1, kCornerRadius, true, false, true, false,
                               fg());
      const int tw = renderer.getTextWidth(SMALL_FONT_ID, labels[i]);
      renderer.drawTextRotated90CW(SMALL_FONT_ID, x, y + (btnH + tw) / 2, labels[i]);
    }
  }
}

// ============================================================================
//  Recent books — covers with circular progress rings
// ============================================================================

void LyraReloadedTheme::drawRecentBookCover(
    GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
    const int selectorIndex, bool& coverRendered, bool& coverBufferStored,
    bool& bufferRestored, std::function<bool()> storeCoverBuffer) const {
  const auto& m = LyraReloadedMetrics::values;
  const int tileW = rect.width - 2 * m.contentSidePadding;
  const int tileH = rect.height;
  const int tileY = rect.y;
  const bool hasBooks = !recentBooks.empty();
  const int bookCount = std::min(static_cast<int>(recentBooks.size()), m.homeRecentBooksCount);

  if (!hasBooks) {
    drawEmptyRecents(renderer, rect);
    return;
  }

  // ---------- Layout: up to 3 covers side by side with a circular progress ring ----------
  const int singleCoverW = (tileW - kCoverGap * (bookCount - 1)) / bookCount;
  const int coverH = m.homeCoverHeight;

  if (!coverRendered) {
    for (int b = 0; b < bookCount; ++b) {
      const RecentBook& book = recentBooks[b];
      int coverX = m.contentSidePadding + b * (singleCoverW + kCoverGap);

      bool hasCover = false;
      if (!book.coverBmpPath.empty()) {
        const std::string path = UITheme::getCoverThumbPath(book.coverBmpPath, coverH);
        FsFile file;
        if (Storage.openFileForRead("HOME", path, file)) {
          Bitmap bitmap(file);
          if (bitmap.parseHeaders() == BmpReaderError::Ok) {
            int imgW = bitmap.getWidth();
            int imgH = bitmap.getHeight();
            // Scale to fit inside singleCoverW × coverH
            int drawW = singleCoverW;
            int drawH = coverH;
            if (imgW > 0 && imgH > 0) {
              float imgR = static_cast<float>(imgW) / imgH;
              float boxR = static_cast<float>(singleCoverW) / coverH;
              if (imgR > boxR) {
                drawH = static_cast<int>(singleCoverW / imgR);
              } else {
                drawW = static_cast<int>(coverH * imgR);
              }
            }
            int cx = coverX + (singleCoverW - drawW) / 2;
            int cy = tileY + (coverH - drawH) / 2;
            renderer.drawBitmap(bitmap, cx, cy, drawW, drawH);
            hasCover = true;
          }
          file.close();
        }
      }

      if (!hasCover) {
        // Card placeholder with icon
        renderer.drawRoundedRect(coverX, tileY, singleCoverW, coverH, 1, kCardRadius, fg());
        renderer.fillRect(coverX + 1, tileY + coverH / 3, singleCoverW - 2,
                          coverH * 2 / 3 - 1, fg());
        renderer.drawIcon(CoverIcon, coverX + (singleCoverW - 32) / 2,
                          tileY + (coverH - 32) / 2, 32, 32);
      }

      // ---- Circular progress ring ----
      // Compute reading progress (0–100).  RecentBook does not store progress
      // directly, so we show a placeholder 0%, but the EpubReaderActivity updates
      // `RecentBook` at runtime.  When the bookProgress field is eventually added
      // this will light up seamlessly.
      int progress = 0;  // placeholder; will hook to RecentBook.progress

      const int circleX = coverX + singleCoverW - kCircleProgressRadius - 4;
      const int circleY = tileY + coverH + 8 + kCircleProgressRadius;
      const int outerR = kCircleProgressRadius;
      const int innerR = outerR - kCircleProgressStroke;

      // Track ring (light gray)
      drawCircleRing(renderer, circleX, circleY, outerR, innerR, fg());
      // Progress arc overlay
      if (progress > 0) {
        drawCircleProgress(renderer, circleX, circleY, outerR, innerR, progress, fg());
      }

      // Percentage text centered in ring
      std::string pctTxt = std::to_string(progress) + "%";
      int ptw = renderer.getTextWidth(SMALL_FONT_ID, pctTxt.c_str());
      int pth = renderer.getLineHeight(SMALL_FONT_ID);
      renderer.drawText(SMALL_FONT_ID, circleX - ptw / 2, circleY - pth / 2,
                        pctTxt.c_str(), fg());

      // Book title next to the progress ring
      int titleX = coverX;
      int titleMaxW = singleCoverW - kCircleProgressRadius * 2 - 12;
      if (titleMaxW < 40) titleMaxW = singleCoverW;
      auto title = renderer.truncatedText(SMALL_FONT_ID, book.title.c_str(), titleMaxW);
      renderer.drawText(SMALL_FONT_ID, titleX, tileY + coverH + 8, title.c_str(), fg());
    }

    coverBufferStored = storeCoverBuffer();
    coverRendered = true;
  }

  // ---- Selection highlight ----
  for (int b = 0; b < bookCount; ++b) {
    bool bookSelected = (selectorIndex == b);
    if (bookSelected) {
      int coverX = m.contentSidePadding + b * (singleCoverW + kCoverGap);
      renderer.drawRoundedRect(coverX - 2, tileY - 2, singleCoverW + 4,
                               tileH + 4, 2, kCardRadius + 2, fg());
    }
  }
}

void LyraReloadedTheme::drawEmptyRecents(const GfxRenderer& renderer,
                                          Rect rect) const {
  constexpr int pad = 48;
  renderer.drawText(UI_12_FONT_ID, rect.x + pad,
                    rect.y + rect.height / 2 - renderer.getLineHeight(UI_12_FONT_ID) - 2,
                    tr(STR_NO_OPEN_BOOK), fg(), EpdFontFamily::BOLD);
  renderer.drawText(UI_10_FONT_ID, rect.x + pad, rect.y + rect.height / 2 + 2,
                    tr(STR_START_READING), fg());
}

// ============================================================================
//  Button menu — rounded cards with icon + label, Apple Settings-style
// ============================================================================

void LyraReloadedTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount,
                                        int selectedIndex,
                                        const std::function<std::string(int)>& buttonLabel,
                                        const std::function<UIIcon(int)>& rowIcon) const {
  const auto& m = LyraReloadedMetrics::values;
  for (int i = 0; i < buttonCount; ++i) {
    int tileW = rect.width - m.contentSidePadding * 2;
    Rect tile{rect.x + m.contentSidePadding,
              rect.y + i * (m.menuRowHeight + m.menuSpacing), tileW, m.menuRowHeight};

    const bool sel = (selectedIndex == i);

    if (sel) {
      renderer.fillRoundedRect(tile.x, tile.y, tile.width, tile.height,
                               kPillRadius, selColor());
    }

    // Subtle card outline for unselected items
    if (!sel) {
      renderer.drawRoundedRect(tile.x, tile.y, tile.width, tile.height, 1,
                               kPillRadius, fg());
    }

    std::string lbl = buttonLabel(i);
    int textX = tile.x + 18;
    const int lh = renderer.getLineHeight(UI_12_FONT_ID);
    const int textY = tile.y + (m.menuRowHeight - lh) / 2;

    if (rowIcon) {
      const uint8_t* bmp = iconForName(rowIcon(i), kMainMenuIconSize);
      if (bmp) {
        renderer.drawIcon(bmp, textX, textY + 3, kMainMenuIconSize, kMainMenuIconSize);
        textX += kMainMenuIconSize + kHPad + 2;
      }
    }

    renderer.drawText(UI_12_FONT_ID, textX, textY, lbl.c_str(), fg());
  }
}

// ============================================================================
//  Progress bars
// ============================================================================

void LyraReloadedTheme::drawProgressBar(const GfxRenderer& renderer, Rect rect,
                                         size_t current, size_t total) const {
  if (total == 0) return;
  int pct = static_cast<int>((static_cast<uint64_t>(current) * 100) / total);

  // Rounded track
  const int radius = rect.height / 2;
  renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height, 1, radius, fg());

  int fillW = (rect.width - 4) * pct / 100;
  if (fillW > 0) {
    renderer.fillRoundedRect(rect.x + 2, rect.y + 2, fillW, rect.height - 4,
                             (rect.height - 4) / 2, fgColor());
  }

  const auto pctText = std::to_string(pct) + "%";
  renderer.drawCenteredText(UI_10_FONT_ID, rect.y + rect.height + 12, pctText.c_str(), fg());
}

void LyraReloadedTheme::drawReadingProgressBar(const GfxRenderer& renderer,
                                                size_t bookProgress) const {
  int mt, mr, mb, ml;
  renderer.getOrientedViewableTRBL(&mt, &mr, &mb, &ml);

  const int maxW = renderer.getScreenWidth() - ml - mr;
  const int barY = renderer.getScreenHeight() - mb - LyraReloadedMetrics::values.bookProgressBarHeight;
  const int fillW = maxW * bookProgress / 100;
  renderer.fillRect(ml, barY, fillW, LyraReloadedMetrics::values.bookProgressBarHeight, fg());
}

// ============================================================================
//  Popup — rounded card overlay
// ============================================================================

Rect LyraReloadedTheme::drawPopup(const GfxRenderer& renderer,
                                   const char* message) const {
  constexpr int y = 120;
  constexpr int outline = 3;
  const int tw = renderer.getTextWidth(UI_12_FONT_ID, message);
  const int th = renderer.getLineHeight(UI_12_FONT_ID);
  const int w = tw + kPopupMarginX * 2;
  const int h = th + kPopupMarginY * 2;
  const int x = (renderer.getScreenWidth() - w) / 2;

  // Shadow (dark ring)
  renderer.fillRoundedRect(x - outline, y - outline, w + outline * 2,
                           h + outline * 2, kCardRadius + outline, bgColor());
  // Card background
  renderer.fillRoundedRect(x, y, w, h, kCardRadius, fgColor());

  const int textX = x + (w - tw) / 2;
  const int textY = y + kPopupMarginY - 2;
  renderer.drawText(UI_12_FONT_ID, textX, textY, message, bg());
  renderer.displayBuffer();

  return Rect{x, y, w, h};
}

void LyraReloadedTheme::fillPopupProgress(const GfxRenderer& renderer,
                                           const Rect& layout,
                                           const int progress) const {
  constexpr int barH = 4;
  const int barW = layout.width - kPopupMarginX * 2;
  const int barX = layout.x + (layout.width - barW) / 2;
  const int barY = layout.y + layout.height - kPopupMarginY / 2 - barH / 2 - 1;

  int fillW = barW * progress / 100;
  renderer.fillRect(barX, barY, fillW, barH, bg());
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}

// ============================================================================
//  Help text
// ============================================================================

void LyraReloadedTheme::drawHelpText(const GfxRenderer& renderer, Rect rect,
                                      const char* label) const {
  const auto& m = LyraReloadedMetrics::values;
  auto trunc = renderer.truncatedText(SMALL_FONT_ID, label,
                                      rect.width - m.contentSidePadding * 2);
  renderer.drawCenteredText(SMALL_FONT_ID, rect.y, trunc.c_str(), fg());
}

// ============================================================================
//  Text field & keyboard
// ============================================================================

void LyraReloadedTheme::drawTextField(const GfxRenderer& renderer, Rect rect,
                                       const int textWidth) const {
  int lineY = rect.y + rect.height + renderer.getLineHeight(UI_12_FONT_ID) +
              LyraReloadedMetrics::values.verticalSpacing;
  int lineW = textWidth + kHPad * 2;
  renderer.drawLine(rect.x + (rect.width - lineW) / 2, lineY,
                    rect.x + (rect.width + lineW) / 2, lineY, 2, fg());
}

void LyraReloadedTheme::drawKeyboardKey(const GfxRenderer& renderer, Rect rect,
                                         const char* label,
                                         const bool isSelected) const {
  if (isSelected) {
    renderer.fillRoundedRect(rect.x, rect.y, rect.width, rect.height,
                             kCornerRadius, fgColor());
  }

  const int tw = renderer.getTextWidth(UI_12_FONT_ID, label);
  const int textX = rect.x + (rect.width - tw) / 2;
  const int textY = rect.y + (rect.height - renderer.getLineHeight(UI_12_FONT_ID)) / 2;
  renderer.drawText(UI_12_FONT_ID, textX, textY, label, !isSelected);
}
