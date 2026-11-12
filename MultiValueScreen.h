#pragma once
/**
 * @file MultiValueScreen.h
 * @brief A screen with multiple individually-editable integer fields + OK/Cancelar.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include "MenuCore.h"
#include "MenuHelpers.h"
#include <functional>

// =============================================================
//  MULTI VALUE SCREEN
// =============================================================
/**
 * @brief A screen with multiple labelled integer value boxes and OK/Cancelar.
 *
 * Each field is displayed as a bordered box with a short label. The
 * highlighted field is filled (inverted text). SELECT enters a field to
 * edit it with the encoder; SELECT again exits edit mode. Navigating past
 * the last field moves to the OK / Cancelar button row.
 *
 * Changes are held in temporary copies and committed to the real variables
 * only when the user selects OK. BACK or Cancelar discards all changes.
 *
 * Supports up to MULTI_VAL_MAX_FIELDS fields (default 8). Fields that
 * exceed three visible slots get a scrollbar.
 *
 * ── Interaction model ─────────────────────────────────────────
 *  Field navigation:
 *    UP / DOWN        → move between fields.
 *    DOWN past last   → move to button row.
 *    SELECT on field  → enter edit mode.
 *    BACK anywhere    → discard and pop.
 *
 *  Edit mode (active field):
 *    UP / DOWN        → increment / decrement value.
 *    SELECT or BACK   → exit edit mode, stay on field.
 *
 *  Button row:
 *    UP               → return to last field.
 *    DOWN             → toggle OK / Cancelar.
 *    SELECT on OK     → commit all fields, fire onConfirm, pop.
 *    SELECT on Cancel → discard, pop.
 *
 * @code
 *   MultiValueScreen sweep("Barrido");
 *   sweep.addField("Inicio", freqStart, 100,    500000, 100,  "%d Hz");
 *   sweep.addField("Fin",    freqStop,  100,    500000, 1000, "%d Hz");
 *   sweep.addField("Puntos", nPoints,   10,     500,    10,   "%d");
 *   sweep.setOnConfirm([](){ startSweep(); });
 *
 *   root.add(&sweep, iconSweep);
 * @endcode
 */
class MultiValueScreen : public MenuItem, public MenuScreen {
public:
    /**
     * @brief Construct a multi-value screen.
     *
     * @param title      Screen title and parent-list label.
     * @param onConfirm  Optional callback fired when the user selects OK.
     *                   All field values have already been committed when
     *                   this fires. May be nullptr; can also be set later
     *                   via setOnConfirm().
     */
    explicit MultiValueScreen(const char* title,
                              std::function<void()> onConfirm = nullptr)
        : MenuItem(title), _count(0), _cursor(0),
          _editing(false), _onConfirm(onConfirm) {}

    /**
     * @brief Add an editable integer field to the screen.
     *
     * Fields appear in the order they are added. Up to MULTI_VAL_MAX_FIELDS
     * fields are supported; additional calls are silently ignored.
     *
     * @param label  Short label shown to the left of the value box.
     * @param value  Reference to the integer variable to edit. Only written
     *               on OK.
     * @param minVal Minimum value (inclusive).
     * @param maxVal Maximum value (inclusive).
     * @param step   Increment / decrement step.
     * @param fmt    printf format string for the displayed value, e.g. "%d Hz".
     *               Defaults to "%d".
     * @param boxW   Value box width in pixels. 0 (default) = auto-sized from
     *               the wider of the min and max value strings. Set manually
     *               if the auto size looks wrong for your format string.
     */
    void addField(const char* label, int& value,
                  int minVal, int maxVal, int step,
                  const char* fmt = "%d",
                  uint8_t boxW = 0)
    {
        if (_count >= MULTI_VAL_MAX_FIELDS) return;
        Field& f = _fields[_count++];
        f.label  = label;
        f.value  = &value;
        f.temp   = value;
        f.minVal = minVal;
        f.maxVal = maxVal;
        f.step   = step;
        f.fmt    = fmt;
        f.boxW   = boxW;
    }

    /**
     * @brief Set or replace the OK confirmation callback after construction.
     * @param cb  Called (with no arguments) when the user confirms. May be nullptr.
     */
    void setOnConfirm(std::function<void()> cb) { _onConfirm = cb; }

    // ── MenuScreen ────────────────────────────────────────────

    /** @brief Returns NAVIGATE. */
    ScreenMode screenMode() const override { return ScreenMode::NAVIGATE; }

    /**
     * @brief Resets all temporary field values and navigation state on open.
     * @param ctx  The active MenuContext.
     */
    void onEnter(MenuContext&) override {
        for (uint8_t i = 0; i < _count; i++)
            _fields[i].temp = *_fields[i].value;
        _cursor  = 0;
        _offset  = 0;
        _editing = false;
        _btnSel  = 0;
        _onBtn   = false;
    }

    /**
     * @brief Draws all visible fields with their labels, value boxes, and buttons.
     * @param scr  U8G2 display reference.
     */
    void onDraw(U8G2& scr) override {
        _beginDraw(scr);
        _menuDrawTitleBar(scr, _label);

        const uint8_t cY   = _menuContentY() - 2;
        const uint8_t btnH = MENU_BTNROW_H;
        const uint8_t btnY = MENU_SCREEN_H - btnH;

        const uint8_t VISIBLE_SLOTS = 3;
        const uint8_t totalH  = btnY - cY;
        const uint8_t slot    = totalH / VISIBLE_SLOTS;
        const uint8_t boxH    = slot - 1;
        const uint8_t visible = (_count < VISIBLE_SLOTS) ? _count : VISIBLE_SLOTS;

        // Scrollbar (only when more fields than fit on screen)
        if (_count > VISIBLE_SLOTS) {
            float barH = (float)totalH * VISIBLE_SLOTS / _count;
            float barY = cY + (float)totalH * _offset / _count;
            scr.drawLine(MENU_SCROLLBAR_W, cY, MENU_SCROLLBAR_W, btnY - 1);
            scr.drawBox(0, (uint8_t)barY, MENU_SCROLLBAR_W - 1, (uint8_t)barH);
            if (_offset > 0)
                _menuDrawArrowUp(scr, _ARROW_CX, cY + 1);
            if (_offset + VISIBLE_SLOTS < _count)
                _menuDrawArrowDown(scr, _ARROW_CX, btnY - 5);
        }

        scr.setFont(MENU_FONT_NORMAL);

        for (uint8_t i = 0; i < visible; i++) {
            uint8_t idx  = _offset + i;
            if (idx >= _count) break;
            uint8_t rowY    = cY + i * slot;
            bool    sel     = (!_onBtn && _cursor == idx);
            bool    editing = (sel && _editing);

            char buf[20];
            snprintf(buf, sizeof(buf), _fields[idx].fmt, _fields[idx].temp);

            // Determine value-box width (auto or fixed)
            uint8_t bw;
            if (_fields[idx].boxW > 0) {
                bw = _fields[idx].boxW;
            } else {
                char bufMin[20], bufMax[20];
                snprintf(bufMin, sizeof(bufMin), _fields[idx].fmt, _fields[idx].minVal);
                snprintf(bufMax, sizeof(bufMax), _fields[idx].fmt, _fields[idx].maxVal);
                uint8_t wMin = scr.getStrWidth(bufMin);
                uint8_t wMax = scr.getStrWidth(bufMax);
                bw = ((wMin > wMax) ? wMin : wMax) + 6;
            }

            uint8_t bx = MENU_SCREEN_W - bw - 2;
            uint8_t by = rowY + 1;

            // Field label
            scr.setDrawColor(1);
            scr.setFont(sel ? MENU_FONT_BOLD : MENU_FONT_NORMAL);
            uint8_t lx = (_count > VISIBLE_SLOTS) ? MENU_TEXT_X + 1 : MENU_TEXT_X;
            scr.setCursor(lx, rowY + 1);
            scr.print(_fields[idx].label);
            if (editing) {
                scr.setCursor(lx - 5, rowY + 1);
                scr.print(">");
            }

            // Value box
            if (sel) {
                scr.setDrawColor(1);
                scr.drawBox(bx, by, bw, boxH);
                scr.setDrawColor(0);
                scr.setFont(editing ? MENU_FONT_BOLD : MENU_FONT_NORMAL);
            } else {
                scr.setDrawColor(1);
                scr.drawFrame(bx, by, bw, boxH);
                scr.setFont(MENU_FONT_NORMAL);
            }
            uint8_t tw = scr.getStrWidth(buf);
            scr.setCursor(bx + (bw - tw) / 2, by + 1);
            scr.print(buf);
            scr.setDrawColor(1);
        }

        // OK / Cancelar buttons
        const uint8_t halfW = MENU_SCREEN_W / 2;
        _menuDrawBtn(scr, MENU_STR_OK,     2,         btnY, halfW - 4, MENU_BTNROW_H,
                     _onBtn && _btnSel == 0);
        _menuDrawBtn(scr, MENU_STR_CANCEL, halfW + 2, btnY, halfW - 4, MENU_BTNROW_H,
                     _onBtn && _btnSel == 1);
        _endDraw(scr);
    }

    /**
     * @brief Handles field navigation, edit mode, and button row interaction.
     * @param ev   The input event.
     * @param ctx  The active MenuContext.
     */
    void onEvent(MenuEvent ev, MenuContext& ctx) override {
        if (_editing) {
            // Edit mode: encoder changes the temporary value
            if (ev == MENU_VAL_INC) {
                _fields[_cursor].temp += _fields[_cursor].step;
                if (_fields[_cursor].temp > _fields[_cursor].maxVal)
                    _fields[_cursor].temp = _fields[_cursor].maxVal;
            } else if (ev == MENU_VAL_DEC) {
                _fields[_cursor].temp -= _fields[_cursor].step;
                if (_fields[_cursor].temp < _fields[_cursor].minVal)
                    _fields[_cursor].temp = _fields[_cursor].minVal;
            } else if (ev == MenuEvent::SELECT || ev == MenuEvent::BACK) {
                _editing = false;
            }
            return;
        }

        if (_onBtn) {
            // Button row navigation
            if (ev == MenuEvent::UP) {
                _onBtn  = false;
                _cursor = _count - 1;
                if (_cursor >= _offset + 3) _offset = _cursor - 2;
            } else if (ev == MenuEvent::DOWN) {
                _btnSel = (_btnSel == 0) ? 1 : 0;
            } else if (ev == MenuEvent::SELECT) {
                if (_btnSel == 0) {  // OK: commit all fields
                    for (uint8_t i = 0; i < _count; i++)
                        *_fields[i].value = _fields[i].temp;
                    if (_onConfirm) _onConfirm();
                }
                ctx.pop();
            } else if (ev == MenuEvent::BACK) {
                ctx.pop();
            }
            return;
        }

        // Field navigation
        if (ev == MenuEvent::SELECT) {
            _editing = true;
        } else if (ev == MenuEvent::DOWN) {
            if (_cursor + 1 < _count) {
                _cursor++;
                if (_cursor >= _offset + 3) _offset = _cursor - 2;
            } else {
                _onBtn  = true;
                _btnSel = 0;
            }
        } else if (ev == MenuEvent::UP) {
            if (_cursor > 0) {
                _cursor--;
                if (_cursor < _offset) _offset = _cursor;
            }
        } else if (ev == MenuEvent::BACK) {
            ctx.pop();
        }
    }

    // ── MenuItem ──────────────────────────────────────────────

    /**
     * @brief Pushes this screen onto the stack when selected in a parent list.
     * @param ctx  The active MenuContext.
     */
    void onSelect(MenuContext& ctx) override { ctx.push(this); }

private:
    /** Internal representation of a single editable field. */
    struct Field {
        const char* label  = nullptr; ///< Short label shown to the left.
        int*        value  = nullptr; ///< Pointer to the real variable.
        int         temp   = 0;       ///< Working copy edited on-screen.
        int         minVal = 0;
        int         maxVal = 100;
        int         step   = 1;
        const char* fmt    = "%d";    ///< printf format string.
        uint8_t     boxW   = 0;       ///< Box width in px; 0 = auto.
    };

    Field   _fields[MULTI_VAL_MAX_FIELDS];
    uint8_t _count;
    uint8_t _cursor;
    uint8_t _offset  = 0;
    bool    _editing;
    bool    _onBtn   = false;   ///< True when cursor is on the button row.
    uint8_t _btnSel  = 0;       ///< 0 = OK, 1 = Cancelar.
    std::function<void()> _onConfirm;
};
