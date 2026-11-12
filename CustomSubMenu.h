#pragma once
/**
 * @file CustomSubMenu.h
 * @brief A scrollable list submenu that holds any MenuItem* entries.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include "MenuCore.h"
#include "MenuHelpers.h"

// =============================================================
//  CUSTOM SUBMENU
// =============================================================
/**
 * @brief A scrollable list of MenuItem* entries that opens as its own screen.
 *
 * Can hold up to 16 items of any MenuItem subtype (ActionItem, ToggleItem,
 * nested CustomSubMenu, ValueEditScreen, etc.).
 *
 * An implicit "Volver" (back) entry is always appended at the bottom of
 * the list. The encoder button or BACK event also returns to the parent.
 *
 * @code
 *   CustomSubMenu settings("Configuración");
 *   settings.add(&brightnessEdit);
 *   settings.add(&uartToggle);
 *   settings.add(&resetAction);
 *
 *   root.add(&settings, iconSettings);
 * @endcode
 */
class CustomSubMenu : public MenuItem, public MenuScreen {
public:
    /**
     * @brief Construct a submenu.
     * @param title  Title shown in the title bar when the submenu is open,
     *               and as the label in the parent list.
     */
    explicit CustomSubMenu(const char* title)
        : MenuItem(title), _count(0), _cursor(0), _offset(0) {}

    /**
     * @brief Add an item to the list.
     *
     * Items are displayed in the order they are added.
     * The submenu does not take ownership — do not destroy items while
     * the menu is in use.
     *
     * @param item  Pointer to any MenuItem subtype. Must outlive this menu.
     */
    void add(MenuItem* item) {
        if (_count < _MAX) _items[_count++] = item;
    }

    // ── MenuScreen ────────────────────────────────────────────

    /** @brief Returns NAVIGATE (encoder scrolls list, button = SELECT). */
    ScreenMode screenMode() const override { return ScreenMode::NAVIGATE; }

    /**
     * @brief Resets the cursor and scroll offset when the menu is opened.
     * @param ctx  The active MenuContext.
     */
    void onEnter(MenuContext& ctx) override {
        _cursor = 0;
        _offset = 0;
    }

    /**
     * @brief Draws the scrollable list with title bar, scrollbar, and arrows.
     * @param scr  U8G2 display reference.
     */
    void onDraw(U8G2& scr) override {
        _beginDraw(scr);
        _menuDrawTitleBar(scr, _label);

        const uint8_t contentY = _menuContentY();
        const uint8_t listH    = MENU_SCREEN_H - contentY;
        const uint8_t rowH     = listH / MENU_MAX_VISIBLE;
        // total = user items + 1 implicit "Volver"
        const uint8_t total   = _count + 1;
        const uint8_t visible = (total < MENU_MAX_VISIBLE) ? total : MENU_MAX_VISIBLE;

        // Scrollbar
        if (total > 0) {
            float barH = (float)(MENU_SCREEN_H - contentY) * visible / total;
            float barY = contentY + (float)(MENU_SCREEN_H - contentY) * _offset / total;
            scr.drawLine(MENU_SCROLLBAR_W, contentY,
                         MENU_SCROLLBAR_W, MENU_SCREEN_H - 1);
            scr.drawBox(0, (uint8_t)barY, MENU_SCROLLBAR_W - 1, (uint8_t)barH);
        }

        // Scroll arrows
        if (_offset > 0)
            _menuDrawArrowUp(scr, _ARROW_CX, contentY);
        if (_offset + MENU_MAX_VISIBLE < total)
            _menuDrawArrowDown(scr, _ARROW_CX, MENU_SCREEN_H - 5);

        // List rows
        for (uint8_t i = 0; i < visible; i++) {
            uint8_t idx  = _offset + i;
            uint8_t rowY = contentY + i * rowH - 1;
            bool    sel  = (idx == _cursor);

            if (idx < _count)
                _items[idx]->drawInList(scr, MENU_TEXT_X, rowY, rowH + 1, sel);
            else
                _drawVolverItem(scr, MENU_TEXT_X, rowY, rowH + 1, sel);
        }
        _endDraw(scr);
    }

    /**
     * @brief Handles UP/DOWN scrolling, SELECT, and BACK navigation.
     * @param ev   The input event.
     * @param ctx  The active MenuContext.
     */
    void onEvent(MenuEvent ev, MenuContext& ctx) override {
        const uint8_t total = _count + 1;
        switch (ev) {
        case MenuEvent::UP:
            if (_cursor > 0) {
                _cursor--;
                if (_cursor < _offset) _offset = _cursor;
            }
            break;
        case MenuEvent::DOWN:
            if (_cursor + 1 < total) {
                _cursor++;
                if (_cursor >= _offset + MENU_MAX_VISIBLE)
                    _offset = _cursor - MENU_MAX_VISIBLE + 1;
            }
            break;
        case MenuEvent::SELECT:
            if (_cursor < _count)
                _items[_cursor]->onSelect(ctx);
            else
                ctx.pop();   // "Volver" selected
            break;
        case MenuEvent::BACK:
            ctx.pop();
            break;
        default: break;
        }
    }

    // ── MenuItem ──────────────────────────────────────────────

    /**
     * @brief Pushes this submenu onto the stack when selected in a parent list.
     * @param ctx  The active MenuContext.
     */
    void onSelect(MenuContext& ctx) override { ctx.push(this); }

private:
    static constexpr uint8_t _MAX = 16;
    MenuItem* _items[_MAX] = {};
    uint8_t   _count;
    uint8_t   _cursor;
    uint8_t   _offset;

    /** Draw the implicit "Volver" row at the bottom of the list. */
    static void _drawVolverItem(U8G2& scr, uint8_t x, uint8_t y,
                                uint8_t rowH, bool selected)
    {
        if (selected) {
            scr.setDrawColor(1);
            scr.drawBox(x - 1, y, MENU_SCREEN_W - x, rowH);
            scr.setDrawColor(0);
            scr.setFont(MENU_FONT_BOLD);
        } else {
            scr.setDrawColor(1);
            scr.setFont(MENU_FONT_NORMAL);
        }
        scr.setCursor(x + 1, y + 1);
        scr.print(MENU_STR_BACK);
        scr.setDrawColor(1);
    }
};
