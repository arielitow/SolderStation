#pragma once
/**
 * @file RootMenu.h
 * @brief The root scrolling menu — the permanent bottom of the navigation stack.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include "MenuCore.h"
#include "MenuHelpers.h"

// =============================================================
//  ROOT MENU
// =============================================================
/**
 * @brief The main scrolling menu that sits at the bottom of the navigation stack.
 *
 * Behaviour is identical to CustomSubMenu with two differences:
 *  1. BACK is silently ignored — there is nowhere further back to go.
 *  2. Each entry can have an associated 64×64 XBM bitmap shown in a
 *     panel on the right side of the display while that entry is selected.
 *
 * Push this onto the MenuContext in setup() and never pop it.
 *
 * @code
 *   // Declare 64×64 PROGMEM bitmaps (XBM byte order, LSB first):
 *   static const uint8_t iconMeas[] PROGMEM = { … };
 *   static const uint8_t iconBrillo[] PROGMEM = { … };
 *
 *   RootMenu root;
 *
 *   void setup() {
 *       root.add(&measScreen,    iconMeas);    // bitmap shown when selected
 *       root.add(&brightnessEdit, iconBrillo);
 *       root.add(&uartToggle,    nullptr);     // no icon for this entry
 *       ctx.push(&root);
 *   }
 * @endcode
 *
 * Icons can also be assigned or overridden after add() via setIcon():
 * @code
 *   root.setIcon(2, iconBaud);   // zero-based index
 * @endcode
 */
class RootMenu : public MenuScreen {
public:
    /** @brief Construct an empty root menu. */
    RootMenu() : _cursor(0), _offset(0), _count(0) {}

    /**
     * @brief Add an item to the root list (up to 16 items).
     *
     * Items are shown in the order they are added.
     * The menu does not take ownership — do not destroy items while
     * the menu is in use.
     *
     * @param item    Pointer to any MenuItem subtype. Must outlive this menu.
     * @param bitmap  Optional 64×64 XBM bitmap shown in the right-hand icon
     *                panel when this entry is highlighted. Must be stored in
     *                PROGMEM (use the PROGMEM keyword). Pass nullptr for no icon.
     *
     * Declare bitmaps as:
     * @code
     *   static const uint8_t myIcon[] PROGMEM = { … };
     * @endcode
     */
    void add(MenuItem* item, const uint8_t* bitmap = nullptr) {
        if (_count < _MAX) {
            _items[_count]   = item;
            _bitmaps[_count] = bitmap;
            _count++;
        }
    }

    /**
     * @brief Assign or replace the icon bitmap for an already-added item.
     *
     * @param index   Zero-based index of the item (order it was added via add()).
     * @param bitmap  PROGMEM XBM bitmap pointer, or nullptr to clear the icon.
     */
    void setIcon(uint8_t index, const uint8_t* bitmap) {
        if (index < _count) _bitmaps[index] = bitmap;
    }

    // ── MenuScreen ────────────────────────────────────────────

    /** @brief Returns NAVIGATE (encoder scrolls list, button = SELECT). */
    ScreenMode screenMode() const override { return ScreenMode::NAVIGATE; }

    /** @brief No state to reset on enter (root is never re-entered). */
    void onEnter(MenuContext&) override {}

    /**
     * @brief Draws the split list/icon layout.
     *
     * Left side: scrollable item list with scrollbar and arrows.
     * Right side: 64×64 XBM bitmap for the currently selected item (if set).
     * @param scr  U8G2 display reference.
     */
    void onDraw(U8G2& scr) override {
        _beginDraw(scr);
        const uint8_t visible = (_count < MENU_MAX_VISIBLE) ? _count : MENU_MAX_VISIBLE;

        // ── Right-side icon panel ─────────────────────────────
        scr.setDrawColor(1);
        scr.drawVLine(MENU_ICON_SEP_X, 0, MENU_SCREEN_H);

        if (_cursor < _count && _bitmaps[_cursor] != nullptr) {
            scr.drawXBMP(MENU_ICON_X, 0,
                         MENU_ICON_SIZE, MENU_ICON_SIZE,
                         _bitmaps[_cursor]);
        }

        // ── Left-side item list ───────────────────────────────
        const uint8_t slot   = MENU_SCREEN_H / MENU_MAX_VISIBLE;
        const uint8_t boxH   = slot - 3;
        const uint8_t topPad = (MENU_SCREEN_H - visible * boxH) / 2;
        const uint8_t listArrowCX = MENU_TEXT_X + (MENU_LIST_W - MENU_TEXT_X) / 2;

        // Scrollbar
        if (_count > 0) {
            float barH = (float)MENU_SCREEN_H * visible / _count;
            float barY = (float)MENU_SCREEN_H * _offset  / _count;
            scr.drawLine(MENU_SCROLLBAR_W, 0,
                         MENU_SCROLLBAR_W, MENU_SCREEN_H - 1);
            scr.drawBox(0, (uint8_t)barY, MENU_SCROLLBAR_W - 1, (uint8_t)barH);
        }

        // Scroll arrows inside the padding zones
        if (_offset > 0)
            _menuDrawArrowUp(scr, listArrowCX, topPad / 2 - 2);
        if (_offset + MENU_MAX_VISIBLE < _count)
            _menuDrawArrowDown(scr, listArrowCX, MENU_SCREEN_H - topPad / 2 - 2);

        // Item rows (no gap between them, block vertically centred)
        for (uint8_t i = 0; i < visible; i++) {
            uint8_t idx  = _offset + i;
            uint8_t boxY = topPad + i * boxH;
            bool    sel  = (idx == _cursor);

            if (sel) {
                scr.setDrawColor(1);
                scr.drawBox(MENU_TEXT_X - 1, boxY,
                            MENU_LIST_W - MENU_TEXT_X + 1, boxH);
                scr.setDrawColor(0);
                scr.setFont(MENU_FONT_BOLD);
            } else {
                scr.setDrawColor(1);
                scr.setFont(MENU_FONT_NORMAL);
            }
            scr.setCursor(MENU_TEXT_X + 1, boxY + 1);
            scr.print(_items[idx]->label());
            scr.setDrawColor(1);
        }
        _endDraw(scr);
    }

    /**
     * @brief Handles UP/DOWN scrolling and SELECT. BACK is silently ignored.
     * @param ev   The input event.
     * @param ctx  The active MenuContext.
     */
    void onEvent(MenuEvent ev, MenuContext& ctx) override {
        switch (ev) {
        case MenuEvent::UP:
            if (_cursor > 0) {
                _cursor--;
                if (_cursor < _offset) _offset = _cursor;
            }
            break;
        case MenuEvent::DOWN:
            if (_cursor + 1 < _count) {
                _cursor++;
                if (_cursor >= _offset + MENU_MAX_VISIBLE)
                    _offset = _cursor - MENU_MAX_VISIBLE + 1;
            }
            break;
        case MenuEvent::SELECT:
            if (_count > 0) _items[_cursor]->onSelect(ctx);
            break;
        case MenuEvent::BACK:
            // Root menu: nowhere to go. Intentionally ignored.
            break;
        default: break;
        }
    }

private:
    static constexpr uint8_t _MAX = 16;
    MenuItem*       _items[_MAX]   = {};
    const uint8_t*  _bitmaps[_MAX] = {};  ///< PROGMEM XBM bitmap per entry.
    uint8_t         _count;
    uint8_t         _cursor;
    uint8_t         _offset;
};
