#pragma once
/**
 * @file ValueListScreen.h
 * @brief A spin-box screen that cycles through a fixed array of values.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include "MenuCore.h"
#include "MenuHelpers.h"
#include <functional>

// =============================================================
//  VALUE LIST SCREEN  – spin-box / discrete list
// =============================================================
/**
 * @brief A full-screen spin-box that steps through a fixed array of options.
 *
 * Useful for selecting from a discrete set of values such as baud rates,
 * sample rates, SSID names, or enum choices. The encoder cycles through
 * the array; the button (or SELECT) confirms the selection and pops.
 *
 * @tparam T  Type of each option: int, uint32_t, const char*, or any type
 *            that can be formatted with a printf-style format string.
 *
 * ── Basic usage ───────────────────────────────────────────────
 * @code
 *   const uint32_t bauds[] = {9600, 57600, 115200, 921600};
 *
 *   ValueListScreen<uint32_t> baudScreen(
 *       "Baudrate", bauds, 4, "%lu bps",
 *       [](uint32_t v){ Serial.begin(v); });
 * @endcode
 *
 * ── Show neighbouring values (scroll wheel feel) ──────────────
 * Pass showNeighbors = true to display the previous and next values
 * in a smaller font above and below the selected value:
 * @code
 *   ValueListScreen<uint32_t> baudScreen(
 *       "Baudrate", bauds, 4, "%lu bps",
 *       [](uint32_t v){ Serial.begin(v); },
 *       0,      // initial index
 *       true);  // showNeighbors
 * @endcode
 *
 * ── Reading the selection back ────────────────────────────────
 * @code
 *   uint8_t idx = baudScreen.currentIndex();
 *   baudScreen.setIndex(2);  // programmatic selection
 * @endcode
 */
template<typename T>
class ValueListScreen : public MenuItem, public MenuScreen {
public:
    /**
     * @brief Construct a value list screen.
     *
     * @param title        Screen title shown in the title bar, and label in
     *                     the parent list.
     * @param options      Pointer to the options array. Must outlive this object
     *                     (use a global or static array).
     * @param count        Number of elements in options[].
     * @param valueFmt     printf-style format string for displaying each option,
     *                     e.g. "%lu bps" or "%s".
     * @param onConfirm    Called with the selected value when the user confirms.
     *                     May be nullptr.
     * @param initIndex    Zero-based index of the initially selected option.
     *                     Defaults to 0.
     * @param showNeighbors  If true, the previous and next options are drawn
     *                     above and below the selected value in normal-weight
     *                     font. The selected value is always bold and centred.
     */
    ValueListScreen(const char* title,
                    const T* options, uint8_t count,
                    const char* valueFmt,
                    std::function<void(T)> onConfirm = nullptr,
                    uint8_t initIndex = 0,
                    bool showNeighbors = false)
        : MenuItem(title),
          _options(options), _count(count), _idx(initIndex),
          _valueFmt(valueFmt), _onConfirm(onConfirm),
          _showNeighbors(showNeighbors) {}

    // ── MenuScreen ────────────────────────────────────────────

    /** @brief Returns NAVIGATE (encoder cycles options, button = confirm). */
    ScreenMode screenMode() const override { return ScreenMode::NAVIGATE; }

    /** @brief No state to reset on enter. */
    void onEnter(MenuContext&) override {}

    /**
     * @brief Draws the spin-box with up/down arrows and the current value.
     * @param scr  U8G2 display reference.
     */
    void onDraw(U8G2& scr) override {
        _beginDraw(scr);
        _menuDrawTitleBar(scr, _label);

        const uint8_t cY         = _menuContentY();
        const uint8_t arrowUpY   = cY + 1;
        const uint8_t arrowDownY = MENU_SCREEN_H - 4;
        const uint8_t cx         = MENU_SCREEN_W / 2;
        const uint8_t midY = arrowUpY + 3 + (arrowDownY - arrowUpY - 3) / 2 - 4;

        char buf[24];

        if (_showNeighbors) {
            const uint8_t rowSpacing = 12;

            // Previous option (above, normal font)
            if (_idx > 0) {
                snprintf(buf, sizeof(buf), _valueFmt, _options[_idx - 1]);
                scr.setFont(MENU_FONT_NORMAL);
                uint8_t tw = scr.getStrWidth(buf);
                scr.setCursor((MENU_SCREEN_W - tw) / 2, midY - rowSpacing);
                scr.print(buf);
            }

            // Selected option (centre, bold)
            snprintf(buf, sizeof(buf), _valueFmt, _options[_idx]);
            scr.setFont(MENU_FONT_BOLD);
            uint8_t tw = scr.getStrWidth(buf);
            scr.setCursor((MENU_SCREEN_W - tw) / 2, midY);
            scr.print(buf);

            // Next option (below, normal font)
            if (_idx + 1 < _count) {
                snprintf(buf, sizeof(buf), _valueFmt, _options[_idx + 1]);
                scr.setFont(MENU_FONT_NORMAL);
                tw = scr.getStrWidth(buf);
                scr.setCursor((MENU_SCREEN_W - tw) / 2, midY + rowSpacing);
                scr.print(buf);
            }
        } else {
            snprintf(buf, sizeof(buf), _valueFmt, _options[_idx]);
            scr.setFont(MENU_FONT_BOLD);
            uint8_t tw = scr.getStrWidth(buf);
            scr.setCursor((MENU_SCREEN_W - tw) / 2, midY);
            scr.print(buf);
        }

        _menuDrawArrowUp  (scr, cx, arrowUpY);
        _menuDrawArrowDown(scr, cx, arrowDownY);
        _endDraw(scr);
    }

    /**
     * @brief Handles encoder turns (cycle options) and button press (confirm + pop).
     * @param ev   The input event.
     * @param ctx  The active MenuContext.
     */
    void onEvent(MenuEvent ev, MenuContext& ctx) override {
        switch (ev) {
        case MENU_VAL_INC:
            if (_idx + 1 < _count) _idx++;
            break;
        case MENU_VAL_DEC:
            if (_idx > 0) _idx--;
            break;
        case MenuEvent::SELECT:
        case MenuEvent::BACK:
            if (_onConfirm) _onConfirm(_options[_idx]);
            ctx.pop();
            break;
        default: break;
        }
    }

    // ── MenuItem ──────────────────────────────────────────────

    /**
     * @brief Pushes this screen onto the stack when selected in a parent list.
     * @param ctx  The active MenuContext.
     */
    void onSelect(MenuContext& ctx) override { ctx.push(this); }

    // ── Accessors ─────────────────────────────────────────────

    /**
     * @brief Returns the zero-based index of the currently selected option.
     */
    uint8_t currentIndex() const { return _idx; }

    /**
     * @brief Programmatically select an option by index.
     *
     * Useful for restoring a previously saved selection on startup.
     * Does nothing if i is out of range.
     * @param i  Zero-based index to select.
     */
    void setIndex(uint8_t i) { if (i < _count) _idx = i; }

private:
    const T*    _options;       ///< Pointer to the options array (not owned).
    uint8_t     _count;         ///< Number of options.
    uint8_t     _idx;           ///< Index of the currently selected option.
    const char* _valueFmt;      ///< printf format string for display.
    std::function<void(T)> _onConfirm; ///< Called on confirm with selected value.
    bool        _showNeighbors; ///< If true, draw prev/next values as context.
};
