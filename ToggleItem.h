#pragma once
/**
 * @file ToggleItem.h
 * @brief A menu list entry that flips between two labels on each press.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include "MenuCore.h"
#include <functional>

// =============================================================
//  TOGGLE ITEM
// =============================================================
/**
 * @brief A list entry that cycles between two text labels and fires a callback.
 *
 * No submenu is opened. The label in the list updates immediately to
 * reflect the new state, and the optional callback receives the new
 * boolean state (true = second / "on" label active).
 *
 * @code
 *   ToggleItem uart("UART - Apagado", "UART - Encendido",
 *                   [](bool on){ sendUART = on; });
 * @endcode
 *
 * You can also initialise the toggle in its "on" state and/or read
 * the current state back at any time:
 * @code
 *   uart.setState(true);        // force on
 *   bool isOn = uart.state();   // read current state
 * @endcode
 */
class ToggleItem : public MenuItem {
public:
    /**
     * @brief Construct a toggle item.
     *
     * @param labelOff  Label shown when state is false (initial state).
     * @param labelOn   Label shown when state is true.
     * @param cb        Optional callback invoked on each toggle, receiving
     *                  the new state (true = labelOn active). May be nullptr.
     */
    ToggleItem(const char* labelOff, const char* labelOn,
               std::function<void(bool)> cb = nullptr)
        : MenuItem(labelOff),
          _labelOff(labelOff), _labelOn(labelOn),
          _state(false), _cb(cb) {}

    /**
     * @brief Flip the toggle state and fire the callback.
     *
     * Called automatically by the parent menu — do not call directly.
     * @param ctx  The active MenuContext (not used, but required by the interface).
     */
    void onSelect(MenuContext& ctx) override {
        _state = !_state;
        _label = _state ? _labelOn : _labelOff;
        if (_cb) _cb(_state);
    }

    /**
     * @brief Returns the current toggle state.
     * @return true if the "on" label is active, false otherwise.
     */
    bool state() const { return _state; }

    /**
     * @brief Programmatically set the toggle state without firing the callback.
     *
     * Useful for restoring a saved state on startup.
     * @param s  The desired state (true = labelOn, false = labelOff).
     */
    void setState(bool s) {
        _state = s;
        _label = _state ? _labelOn : _labelOff;
    }

private:
    const char* _labelOff;              ///< Label shown when state == false.
    const char* _labelOn;               ///< Label shown when state == true.
    bool        _state;                 ///< Current toggle state.
    std::function<void(bool)> _cb;      ///< Callback fired on each toggle.
};
