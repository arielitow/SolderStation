#pragma once
/**
 * @file ValueEditScreen.h
 * @brief A full-screen continuous-range value editor driven by the encoder.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include "MenuCore.h"
#include "MenuHelpers.h"
#include <functional>

// =============================================================
//  VALUE EDIT SCREEN  – continuous range
// =============================================================
/**
 * @brief A full-screen editor for a single numeric variable.
 *
 * The encoder increments or decrements the value in fixed steps between
 * a minimum and maximum. An optional progress bar and a printf-style
 * numeric label can be shown. The encoder button (or SELECT) confirms
 * the value and returns to the previous screen.
 *
 * The variable is edited live — changes take effect immediately via the
 * onChange callback. If you need a working-copy / cancel pattern, use
 * ValueEditConfirmScreen instead.
 *
 * @tparam T  Numeric type: int, float, uint32_t, etc.
 *
 * ── Basic usage ───────────────────────────────────────────────
 * @code
 *   int brightness = 200;
 *
 *   ValueEditScreen<int> brillo("Brillo", brightness, 0, 255, 5,
 *       true,    // show progress bar
 *       "%d",    // value format label
 *       [](int v){ ledcWrite(PWM_CH, v); });  // onChange
 * @endcode
 *
 * ── Float, no bar, no label ───────────────────────────────────
 * @code
 *   float gain = 1.0f;
 *
 *   ValueEditScreen<float> gainEdit("Ganancia", gain,
 *       0.f, 2.f, 0.1f,
 *       false, nullptr);
 * @endcode
 *
 * ── With onConfirm callback ───────────────────────────────────
 * @code
 *   ValueEditScreen<int> freq("Frecuencia", freqVal,
 *       100, 500000, 100,
 *       true, "%d Hz",
 *       [](int v){ applyFreq(v); },   // onChange  (live)
 *       [](int v){ saveFreq(v); });   // onConfirm (on button press)
 * @endcode
 */
template<typename T>
class ValueEditScreen : public MenuItem, public MenuScreen {
public:
    /**
     * @brief Construct a value editor.
     *
     * @param title      Screen title shown in the title bar, and label in
     *                   the parent list.
     * @param value      Reference to the variable to edit. The variable is
     *                   modified live as the encoder turns.
     * @param minVal     Minimum allowed value (inclusive).
     * @param maxVal     Maximum allowed value (inclusive).
     * @param step       Amount added or subtracted per encoder tick.
     * @param showBar    If true, draws a horizontal progress bar.
     * @param valueFmt   printf-style format string for the numeric label shown
     *                   above the bar, e.g. "%d" or "%.1f dB". Pass nullptr
     *                   to hide the label.
     * @param onChange   Called every time the value changes. Receives the new
     *                   value. Use for live preview (e.g. update LED brightness).
     *                   May be nullptr.
     * @param onConfirm  Called once when the user confirms (encoder button /
     *                   SELECT). Receives the final value. May be nullptr.
     */
    ValueEditScreen(const char* title, T& value,
                    T minVal, T maxVal, T step,
                    bool showBar = true,
                    const char* valueFmt = nullptr,
                    std::function<void(T)> onChange  = nullptr,
                    std::function<void(T)> onConfirm = nullptr)
        : MenuItem(title),
          _value(value), _minVal(minVal), _maxVal(maxVal), _step(step),
          _showBar(showBar), _valueFmt(valueFmt),
          _onChange(onChange), _onConfirm(onConfirm) {}

    // ── MenuScreen ────────────────────────────────────────────

    /** @brief Returns NAVIGATE (encoder turns = value change, button = confirm). */
    ScreenMode screenMode() const override { return ScreenMode::NAVIGATE; }

    /** @brief No state to reset on enter. */
    void onEnter(MenuContext&) override {}

    /**
     * @brief Draws the title bar, optional numeric label, and optional progress bar.
     * @param scr  U8G2 display reference.
     */
    void onDraw(U8G2& scr) override {
        _beginDraw(scr);
        _menuDrawTitleBar(scr, _label);

        const uint8_t cY = _menuContentY();

        if (_valueFmt) {
            char buf[24];
            snprintf(buf, sizeof(buf), _valueFmt, _value);
            scr.setFont(MENU_FONT_BOLD);
            uint8_t tw = scr.getStrWidth(buf);
            scr.setCursor((MENU_SCREEN_W - tw) / 2, cY + 2);
            scr.print(buf);
        }

        if (_showBar) {
            const uint8_t barX = 10;
            const uint8_t barW = MENU_SCREEN_W - 20;
            const uint8_t barY = cY + (_valueFmt ? 14 : 10);
            const uint8_t barH = 8;

            float   ratio = (float)(_value - _minVal) / (float)(_maxVal - _minVal);
            uint8_t fill  = (uint8_t)(ratio * (barW - 2));

            scr.drawFrame(barX, barY, barW, barH);

            scr.setFont(MENU_FONT_NORMAL);
            scr.setCursor(barX - 6, barY);
            scr.print('<');
            scr.setCursor(barX + barW + 1, barY);
            scr.print('>');

            scr.drawBox(barX + 1, barY + 1, fill, barH - 2);
        }
        _endDraw(scr);
    }

    /**
     * @brief Handles encoder turns (value change) and button press (confirm + pop).
     * @param ev   The input event.
     * @param ctx  The active MenuContext.
     */
    void onEvent(MenuEvent ev, MenuContext& ctx) override {
        switch (ev) {
        case MENU_VAL_INC:
            _value += _step;
            if (_value > _maxVal) _value = _maxVal;
            if (_onChange) _onChange(_value);
            break;
        case MENU_VAL_DEC:
            _value -= _step;
            if (_value < _minVal) _value = _minVal;
            if (_onChange) _onChange(_value);
            break;
        case MenuEvent::SELECT:
        case MenuEvent::BACK:
            if (_onConfirm) _onConfirm(_value);
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

private:
    T&          _value;     ///< Reference to the variable being edited.
    T           _minVal;
    T           _maxVal;
    T           _step;
    bool        _showBar;
    const char* _valueFmt;
    std::function<void(T)> _onChange;
    std::function<void(T)> _onConfirm;
};
