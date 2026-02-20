#pragma once

#include "components/themes/BaseTheme.h"

class GfxRenderer;

// =============================================================================
// Lyra Reloaded — A modern, Apple-inspired UI theme for CrossPoint Reader.
//
// Design principles:
//   - Clean, spacious layouts with generous whitespace
//   - Rounded card containers and pill-shaped interactive elements
//   - Circular reading progress indicators on the home screen
//   - Dark mode support (inverted palette for e-ink)
//   - Frosted-glass-style separators using dithered gray
//   - Minimal header with thin hairline dividers
//   - Grid-style home menu with centered icons
// =============================================================================

// Lyra Reloaded metrics — tuned for a modern, airy layout
namespace LyraReloadedMetrics {
constexpr ThemeMetrics values = {
    .batteryWidth = 16,
    .batteryHeight = 12,
    .topPadding = 4,
    .batteryBarHeight = 36,
    .headerHeight = 72,
    .verticalSpacing = 14,
    .contentSidePadding = 24,
    .listRowHeight = 44,
    .listWithSubtitleRowHeight = 64,
    .menuRowHeight = 58,
    .menuSpacing = 10,
    .tabSpacing = 6,
    .tabBarHeight = 38,
    .scrollBarWidth = 3,
    .scrollBarRightOffset = 4,
    .homeTopPadding = 48,
    .homeCoverHeight = 200,
    .homeCoverTileHeight = 260,
    .homeRecentBooksCount = 3,
    .buttonHintsHeight = 38,
    .sideButtonHintsWidth = 28,
    .progressBarHeight = 14,
    .bookProgressBarHeight = 3,
    .keyboardKeyWidth = 31,
    .keyboardKeyHeight = 50,
    .keyboardKeySpacing = 0,
    .keyboardBottomAligned = true,
    .keyboardCenteredText = true,
};
}  // namespace LyraReloadedMetrics

// ---------------------------------------------------------------------------
// Helper: is dark mode enabled?
// Reads `CrossPointSettings::darkMode` at call time.
// ---------------------------------------------------------------------------
bool lyraReloaded_isDarkMode();

class LyraReloadedTheme : public BaseTheme {
 public:
  // ---- Drawing API overrides -----------------------------------------------
  void drawBatteryLeft(const GfxRenderer& renderer, Rect rect,
                       bool showPercentage = true) const override;
  void drawBatteryRight(const GfxRenderer& renderer, Rect rect,
                        bool showPercentage = true) const override;
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title,
                  const char* subtitle) const override;
  void drawSubHeader(const GfxRenderer& renderer, Rect rect,
                     const char* label,
                     const char* rightLabel = nullptr) const override;
  void drawTabBar(const GfxRenderer& renderer, Rect rect,
                  const std::vector<TabInfo>& tabs,
                  bool selected) const override;
  void drawList(const GfxRenderer& renderer, Rect rect, int itemCount,
                int selectedIndex,
                const std::function<std::string(int index)>& rowTitle,
                const std::function<std::string(int index)>& rowSubtitle,
                const std::function<UIIcon(int index)>& rowIcon,
                const std::function<std::string(int index)>& rowValue,
                bool highlightValue) const override;
  void drawButtonHints(GfxRenderer& renderer, const char* btn1,
                       const char* btn2, const char* btn3,
                       const char* btn4) const override;
  void drawSideButtonHints(const GfxRenderer& renderer, const char* topBtn,
                           const char* bottomBtn) const override;
  void drawButtonMenu(
      GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
      const std::function<std::string(int index)>& buttonLabel,
      const std::function<UIIcon(int index)>& rowIcon) const override;
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect,
                           const std::vector<RecentBook>& recentBooks,
                           const int selectorIndex, bool& coverRendered,
                           bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;
  void drawProgressBar(const GfxRenderer& renderer, Rect rect, size_t current,
                       size_t total) const override;
  void drawReadingProgressBar(const GfxRenderer& renderer,
                              size_t bookProgress) const override;
  Rect drawPopup(const GfxRenderer& renderer,
                 const char* message) const override;
  void fillPopupProgress(const GfxRenderer& renderer, const Rect& layout,
                         const int progress) const override;
  void drawHelpText(const GfxRenderer& renderer, Rect rect,
                    const char* label) const override;
  void drawTextField(const GfxRenderer& renderer, Rect rect,
                     const int textWidth) const override;
  void drawKeyboardKey(const GfxRenderer& renderer, Rect rect,
                       const char* label, const bool isSelected) const override;

 private:
  // --- Helpers --------------------------------------------------------------
  void drawCircleRing(const GfxRenderer& renderer, int cx, int cy,
                      int outerR, int innerR, bool state) const;
  void drawCircleProgress(const GfxRenderer& renderer, int cx, int cy,
                          int outerR, int innerR, int percent,
                          bool state) const;
  void drawEmptyRecents(const GfxRenderer& renderer, Rect rect) const;

  // Dark-mode aware foreground / background helpers.
  // fg = true  → draw in foreground colour; fg = false → background colour.
  bool fg() const;   // foreground state value  (false = black pixel on normal)
  bool bg() const;   // background state value
  Color fgColor() const;
  Color bgColor() const;
  Color selColor() const;  // selection highlight colour
};
