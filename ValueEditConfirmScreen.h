#pragma once
/**
 * @file ValueEditConfirmScreen.h
 * @brief A value editor with explicit OK / Cancelar buttons.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include "MenuCore.h"
#include "MenuHelpers.h"
#include <functional>

// =============================================================
//  VALUE EDIT CONFIRM SCREEN
// =============================================================
/**
 * @brief A full-screen value editor that only commits the change on OK.
 *
 * A temporary working copy of the value is edited. The original variable
 * is updated only when the user explicitly selects OK. Selecting Cancelar
 * or pressing BACK discards all changes.
 *
 * This is the safer alternative to ValueEditScreen when changes have
 * side-effects that are expensive or destructive to reverse.
 *
 * @tparam T  Numeric type: int, float, uint32_t, etc.
 *
 * ── Interaction model ─────────────────────────────────────────
 *  State: NAV_VALUE  → cursor is on the value / bar area.
 *    SELECT           → enter EDIT_VALUE mode (start turning encoder).
 *    DOWN             → move cursor to button row (NAV_BTN).
 *    BACK             → discard and pop.
 *
 *  State: EDIT_VALUE → encoder changes the working copy.
 *    UP / DOWN        → increment / decrement value.
 *    SELECT or BACK   → exit edit mode, return to NAV_VALUE.
 *
 *  State: NAV_BTN    → cursor is on the button row.
 *    UP               → move back to value area (NAV_VALUE).
 *    DOWN             → toggle between OK and Cancelar.
 *    SELECT on OK     → commit value, fire onConfirm, pop.
 *    SELECT on Cancel → discard, pop.
 *    BACK             → discard, pop.
 *
 * @code
 *   ValueEditConfirmScreen<int> cutoff(
 *       "Frec. corte", freqVal, 100, 500000, 100,
 *       true,    // show bar
 *       "%d Hz", // label format
 *       [](int v){ setFs(v); });
 * @endcode
 */
template<typename T>
class ValueEditConfirmScreen : public MenuItem, public MenuScreen {
public:
    /**
     * @brief Construct a value editor with OK / Cancelar buttons.
     *
     * @param title      Screen title and parent-list label.
     * @param value      Reference to the variable to edit. Only written on OK.
     * @param minVal     Minimum allowed value (inclusive).
     * @param maxVal     Maximum allowed value (inclusive).
     * @param step       Encoder step size.
     * @param showBar    If true, draws a progress bar.
     * @param valueFmt   printf format string for the numeric label, e.g. "%d Hz".
     *                   Pass nullptr to hide.
     * @param onConfirm  Called with the confirmed value when the user selects OK.
     *                   May be nullptr.
     */
    ValueEditConfirmScreen(const char* title, T& value,
                           T minVal, T maxVal, T step,
                           bool showBar = true,
                           const char* valueFmt = nullptr,
                           std::function<void(T)> onConfirm = nullptr)
        : MenuItem(title),
          _value(value), _temp(value),
          _minVal(minVal), _maxVal(maxVal), _step(step),
          _showBar(showBar), _valueFmt(valueFmt),
          _onConfirm(onConfirm),
          _focus(NAV_VALUE), _btnSel(0) {}

    // ── MenuScreen ────────────────────────────────────────────

    /** @brief Returns NAVIGATE. */
    ScreenMode screenMode() const override { return ScreenMode::NAVIGATE; }

    /**
     * @brief Resets the working copy and focus state on each open.
     * @param ctx  The active MenuContext.
     */
    void onEnter(MenuContext&) override {
        _temp   = _value;
        _focus  = NAV_VALUE;
        _btnSel = 0;
    }

    /**
     * @brief Draws the value area, progress bar, and OK/Cancelar buttons.
     * @param scr  U8G2 display reference.
     */
    void onDraw(U8G2& scr) override {
        _beginDraw(scr);
        _menuDrawTitleBar(scr, _label);

        const uint8_t cY   = _menuContentY();
        const uint8_t btnY = MENU_SCREEN_H - MENU_BTNROW_H;

        // Value area highlight (NAV_VALUE or EDIT_VALUE)
        bool valueHL = (_focus == NAV_VALUE || _focus == EDIT_VALUE);
        if (valueHL) {
            scr.setDrawColor(1);
            scr.drawFrame(1, cY, MENU_SCREEN_W - 2, btnY - 4 - cY);
        }
        if (_focus == EDIT_VALUE) {
            scr.setFont(MENU_FONT_NORMAL);
            scr.setCursor(3, cY + 1);
            scr.print("*");
        }

        // Numeric label
        if (_valueFmt) {
            char buf[24];
            snprintf(buf, sizeof(buf), _valueFmt, _temp);
            scr.setFont(MENU_FONT_BOLD);
            uint8_t tw = scr.getStrWidth(buf);
            scr.setCursor((MENU_SCREEN_W - tw) / 2, cY + 2);
            scr.print(buf);
        }

        // Progress bar
        if (_showBar) {
            const uint8_t barX = 10;
            const uint8_t barW = MENU_SCREEN_W - 20;
            const uint8_t barY = cY + (_valueFmt ? 14 : 6);
            const uint8_t barH = 7;

            float   ratio = (float)(_temp - _minVal) / (float)(_maxVal - _minVal);
            uint8_t fill  = (uint8_t)(ratio * (barW - 2));

            scr.setFont(MENU_FONT_NORMAL);
            scr.setCursor(barX - 6, barY); scr.print('<');
            scr.setCursor(barX + barW + 1, barY); scr.print('>');
            scr.drawFrame(barX, barY, barW, barH);
            scr.drawBox(barX + 1, barY + 1, fill, barH - 2);
        }

        // Separator above buttons
        scr.drawHLine(0, btnY - 2, MENU_SCREEN_W);

        // OK / Cancelar buttons
        const uint8_t halfW = MENU_SCREEN_W / 2;
        _menuDrawBtn(scr, MENU_STR_OK,     2,         btnY, halfW - 4, MENU_BTNROW_H,
                     _focus == NAV_BTN && _btnSel == 0);
        _menuDrawBtn(scr, MENU_STR_CANCEL, halfW + 2, btnY, halfW - 4, MENU_BTNROW_H,
                     _focus == NAV_BTN && _btnSel == 1);
        _endDraw(scr);
    }

    /**
     * @brief Handles the three-state focus machine (NAV_VALUE / EDIT_VALUE / NAV_BTN).
     * @param ev   The input event.
     * @param ctx  The active MenuContext.
     */
    void onEvent(MenuEvent ev, MenuContext& ctx) override {
        switch (_focus) {

        case NAV_VALUE:
            if (ev == MenuEvent::SELECT)   { _focus = EDIT_VALUE; }
            else if (ev == MenuEvent::DOWN){ _focus = NAV_BTN; _btnSel = 0; }
            else if (ev == MenuEvent::BACK){ ctx.pop(); }
            break;

        case EDIT_VALUE:
            if (ev == MENU_VAL_INC) {
                _temp += _step;
                if (_temp > _maxVal) _temp = _maxVal;
            } else if (ev == MENU_VAL_DEC) {
                _temp -= _step;
                if (_temp < _minVal) _temp = _minVal;
            } else if (ev == MenuEvent::SELECT) {
                _focus = NAV_VALUE;
            } else if (ev == MenuEvent::BACK) {
                _temp  = _value;   // discard working copy
                _focus = NAV_VALUE;
            }
            break;

        case NAV_BTN:
            if (ev == MenuEvent::UP) {
                _focus = NAV_VALUE;
            } else if (ev == MenuEvent::DOWN) {
                _btnSel = (_btnSel == 0) ? 1 : 0;
            } else if (ev == MenuEvent::SELECT) {
                if (_btnSel == 0) {        // OK
                    _value = _temp;
                    if (_onConfirm) _onConfirm(_value);
                }
                ctx.pop();
            } else if (ev == MenuEvent::BACK) {
                ctx.pop();
            }
            break;
        }
    }

    // ── MenuItem ──────────────────────────────────────────────

    /**
     * @brief Pushes this screen onto the stack when selected in a parent list.
     * @param ctx  The active MenuContext.
     */
    void onSelect(MenuContext& ctx) override { ctx.push(this); }

private:
    enum Focus : uint8_t { NAV_VALUE, EDIT_VALUE, NAV_BTN };

    T&          _value;     ///< Reference to the real variable (written on OK only).
    T           _temp;      ///< Working copy edited while the screen is open.
    T           _minVal;
    T           _maxVal;
    T           _step;
    bool        _showBar;
    const char* _valueFmt;
    std::function<void(T)> _onConfirm;
    Focus       _focus;
    uint8_t     _btnSel;    ///< 0 = OK highlighted, 1 = Cancelar highlighted.
};
