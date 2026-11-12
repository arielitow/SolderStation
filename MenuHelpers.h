#pragma once
/**
 * @file MenuHelpers.h
 * @brief Shared drawing helpers used internally by all screen types.
 *
 * All functions are `inline` (C++11-safe) so they can be included
 * from multiple headers without duplicate-symbol errors.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include "MenuCore.h"

// ─────────────────────────────────────────────────────────────
// X-centre for scroll arrows in the text area.
// Placed after the scrollbar, centred across the remaining width.
// ─────────────────────────────────────────────────────────────
/// X pixel centre used when drawing UP/DOWN scroll arrows.
#define _ARROW_CX  (MENU_TEXT_X + (MENU_SCREEN_W - MENU_TEXT_X) / 2)

/**
 * @brief Draw the standard title bar at the top of a screen.
 *
 * Renders the title string in MENU_FONT_BOLD starting at (3, 1),
 * then draws a horizontal rule one pixel below the font ascent.
 * Call this at the start of onDraw() after _beginDraw().
 *
 * @param scr    U8G2 display reference.
 * @param title  Null-terminated title string.
 */
inline void _menuDrawTitleBar(U8G2& scr, const char* title) {
    scr.setFont(MENU_FONT_BOLD);
    scr.setDrawColor(1);
    scr.setCursor(3, 1);
    scr.print(title);
    uint8_t lineY = scr.getAscent() + 3;
    scr.drawHLine(0, lineY, MENU_SCREEN_W);
}

/**
 * @brief Returns the Y pixel coordinate where content starts below the title bar.
 *
 * Computed from the bold font ascent + padding + rule + gap.
 * If you change MENU_FONT_BOLD, update this value to match.
 *
 * @return uint8_t  Y coordinate of the first usable content pixel.
 */
inline uint8_t _menuContentY() {
    // ascent of helvB10 ≈ 10px + 3px padding + 1px rule + 1px gap = 15,
    // tuned to 14 for the TRIASCAN display layout.
    return 14;
}

/**
 * @brief Draw a small solid upward-pointing arrow (scroll indicator).
 *
 * The arrow is 5px wide and 3px tall, with the tip at (cx, y)
 * and the base two pixels below.
 *
 * @param scr  U8G2 display reference.
 * @param cx   Horizontal centre of the arrow in pixels.
 * @param y    Y coordinate of the arrow tip.
 */
inline void _menuDrawArrowUp(U8G2& scr, uint8_t cx, uint8_t y) {
    scr.drawHLine(cx,     y,     1);  // tip
    scr.drawHLine(cx - 1, y + 1, 3);
    scr.drawHLine(cx - 2, y + 2, 5); // base
}

/**
 * @brief Draw a small solid downward-pointing arrow (scroll indicator).
 *
 * Mirror of _menuDrawArrowUp: 5px base at top, 1px tip at bottom.
 *
 * @param scr  U8G2 display reference.
 * @param cx   Horizontal centre of the arrow in pixels.
 * @param y    Y coordinate of the top (widest) row of the arrow.
 */
inline void _menuDrawArrowDown(U8G2& scr, uint8_t cx, uint8_t y) {
    scr.drawHLine(cx - 2, y,     5); // base
    scr.drawHLine(cx - 1, y + 1, 3);
    scr.drawHLine(cx,     y + 2, 1); // tip
}

/**
 * @brief Draw an OK / Cancelar style button.
 *
 * Renders a filled (selected) or framed (idle) rectangle with the
 * label centred inside. Used by ValueEditConfirmScreen and
 * MultiValueScreen.
 *
 * @param scr   U8G2 display reference.
 * @param txt   Button label string.
 * @param x     Left edge of the button in pixels.
 * @param y     Top edge of the button in pixels.
 * @param w     Button width in pixels.
 * @param h     Button height in pixels.
 * @param sel   True to draw filled (selected), false for outline only.
 */
inline void _menuDrawBtn(U8G2& scr, const char* txt,
                         uint8_t x, uint8_t y,
                         uint8_t w, uint8_t h, bool sel)
{
    if (sel) {
        scr.setDrawColor(1);
        scr.drawBox(x, y, w, h);
        scr.setDrawColor(0);
    } else {
        scr.setDrawColor(1);
        scr.drawFrame(x, y, w, h);
    }
    scr.setFont(MENU_FONT_BOLD);
    uint8_t tw = scr.getStrWidth(txt);
    scr.setCursor(x + (w - tw) / 2, y + 1);
    scr.print(txt);
    scr.setDrawColor(1);
}
