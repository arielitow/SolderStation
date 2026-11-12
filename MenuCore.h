#pragma once
/**
 * @file MenuCore.h
 * @brief Core types for the MenuFramework library.
 *
 * Defines the fundamental building blocks that every other module
 * depends on:
 *   - MenuEvent      – input event enum
 *   - MenuScreen     – mixin base for screens
 *   - MenuItem       – base for anything that appears in a list
 *   - MenuContext    – navigation stack + input dispatcher
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <functional>

#ifdef MENU_USE_ENCODER
  #include <Encoder.h>
#endif

// ─────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────
class MenuContext;
class MenuItem;

// =============================================================
//  MENU EVENT
// =============================================================
/**
 * @brief Input events dispatched by MenuContext to the active screen.
 *
 * You never create these manually — MenuContext::update() produces
 * them from your encoder or button state and passes them to the
 * current screen's onEvent().
 */
enum class MenuEvent : int8_t {
    NONE   =  0,  ///< No event this frame.
    UP     =  1,  ///< Encoder CCW or Up button.
    DOWN   = -1,  ///< Encoder CW  or Down button.
    SELECT =  2,  ///< Encoder button press or OK button.
    BACK   =  3   ///< Back / cancel button.
};

// =============================================================
//  SCREEN BASE CLASS
// =============================================================
/**
 * @brief Mixin base for menu items that render a full screen.
 *
 * Inherit from both MenuItem and MenuScreen to create a screen type.
 * MenuContext calls onEnter() once when pushed, then onDraw() and
 * onEvent() every loop while the screen is on top of the stack.
 *
 * ScreenMode controls how the encoder button is interpreted:
 *   - NAVIGATE   : button = SELECT (scroll / choose items).
 *   - VIEW_ONLY  : button = BACK   (nothing to interact with).
 *   - FULLSCREEN : caller owns the entire display buffer.
 */
class MenuScreen {
public:
    /**
     * @brief Controls how the encoder button is interpreted.
     */
    enum class ScreenMode {
        NAVIGATE,    ///< Encoder scrolls items; button press = SELECT.
        VIEW_ONLY,   ///< Static content; button press = BACK.
        FULLSCREEN,  ///< Raw buffer access; no chrome drawn around content.
    };

    /**
     * @brief Returns this screen's interaction mode.
     *
     * MenuContext uses this to decide whether the encoder button
     * fires SELECT or BACK.
     */
    virtual ScreenMode screenMode() const = 0;

    /**
     * @brief Called once when this screen is pushed onto the stack.
     *
     * Use to reset state, start timers, switch display orientation, etc.
     * @param ctx  The active MenuContext.
     */
    virtual void onEnter(MenuContext& ctx) {}

    /**
     * @brief Called every loop() while this screen is on top.
     *
     * Responsible for its own clearBuffer() / sendBuffer() cycle
     * (use _beginDraw / _endDraw helpers for normal screens).
     * @param scr  The U8G2 display reference.
     */
    virtual void onDraw(U8G2& scr) = 0;

    /**
     * @brief Called with every input event while this screen is on top.
     *
     * @param ev   The event that occurred this frame.
     * @param ctx  The active MenuContext; call ctx.pop() to go back.
     */
    virtual void onEvent(MenuEvent ev, MenuContext& ctx) = 0;

protected:
    /**
     * @brief Call at the start of onDraw() for normal (non-fullscreen) screens.
     *
     * Clears the display buffer so drawing starts on a blank slate.
     */
    static void _beginDraw(U8G2& scr) { scr.clearBuffer(); }

    /**
     * @brief Call at the end of onDraw() for normal (non-fullscreen) screens.
     *
     * Flushes the buffer to the physical display.
     */
    static void _endDraw(U8G2& scr) { scr.sendBuffer(); }
};

// =============================================================
//  MENU ITEM BASE CLASS
// =============================================================
/**
 * @brief Abstract base for every entry that can appear in a menu list.
 *
 * Subclass this (along with MenuScreen if the item opens a screen)
 * to create custom item types.
 *
 * The framework calls onSelect() when the user activates this item,
 * and drawInList() to render it inside a parent submenu.
 */
class MenuItem {
public:
    /**
     * @param label  Text shown in the parent list. Must remain valid
     *               for the lifetime of this object (use string literals
     *               or a persistent buffer).
     */
    explicit MenuItem(const char* label) : _label(label) {}
    virtual ~MenuItem() = default;

    /** @brief Returns the label string shown in the parent list. */
    const char* label() const { return _label; }

    /**
     * @brief Called when this item is activated (encoder click / OK).
     *
     * For items that open a screen, implement this as ctx.push(this).
     * For action items, run your callback here.
     * @param ctx  The active MenuContext.
     */
    virtual void onSelect(MenuContext& ctx) = 0;

    /**
     * @brief Renders this item inside a parent list at the given position.
     *
     * The default implementation draws a highlight box when selected,
     * then prints _label. Override to draw custom content (icons, values…).
     *
     * @param scr       U8G2 display reference.
     * @param x         Left edge of the text area in pixels.
     * @param y         Top edge of the item row in pixels.
     * @param rowH      Row height in pixels.
     * @param selected  True if this item is currently highlighted.
     */
    virtual void drawInList(U8G2& scr, uint8_t x, uint8_t y,
                            uint8_t rowH, bool selected) const
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
        scr.print(_label);
        scr.setDrawColor(1);
    }

protected:
    const char* _label;  ///< Text shown in the parent list.
};

// =============================================================
//  MENU CONTEXT
// =============================================================
/**
 * @brief Owns the navigation stack and drives input → screen dispatch.
 *
 * Create one instance globally, passing your U8G2 display.
 * Push the root menu in setup(), then call update() and draw()
 * every loop().
 *
 * @code
 *   MenuContext ctx(u8g2);
 *
 *   void setup() {
 *       ctx.push(&root);
 *   }
 *   void loop() {
 *       ctx.update(encoder);   // or ctx.update(up, dn, ok, back)
 *       ctx.draw();
 *   }
 * @endcode
 */
class MenuContext {
public:
    /**
     * @param display  Your U8G2 display instance. Must outlive this object.
     */
    explicit MenuContext(U8G2& display) : _disp(display), _depth(0) {}

    // ── Stack operations ──────────────────────────────────────

    /**
     * @brief Push a screen onto the navigation stack.
     *
     * Calls the screen's onEnter() before the next draw cycle.
     * Does nothing if the stack is already at MENU_STACK_DEPTH.
     * @param screen  The screen to push. Must outlive this context.
     */
    void push(MenuScreen* screen) {
        if (_depth < MENU_STACK_DEPTH) {
            _stack[_depth++] = screen;
            screen->onEnter(*this);
        }
    }

    /**
     * @brief Pop the current screen, returning to the previous one.
     *
     * Does nothing if already at the root (depth == 1).
     */
    void pop() {
        if (_depth > 1) _depth--;
    }

    /**
     * @brief Jump all the way back to the root screen in one step.
     *
     * Useful for a "home" button or after completing a deep workflow.
     */
    void popToRoot() { if (_depth > 1) _depth = 1; }

    /**
     * @brief Returns the screen currently on top of the stack, or nullptr.
     */
    MenuScreen* current() { return _depth ? _stack[_depth - 1] : nullptr; }

    // ── Input: encoder ────────────────────────────────────────

#ifdef MENU_USE_ENCODER
    /**
     * @brief Poll an Encoder object and dispatch navigation events.
     *
     * Only available when MENU_USE_ENCODER is defined before including
     * MenuFramework.h. CW rotation = DOWN, CCW = UP. Button press fires
     * SELECT on NAVIGATE screens and BACK on VIEW_ONLY / FULLSCREEN screens.
     *
     * @param enc  Your Encoder instance.
     */
    void update(Encoder& enc) {
        MenuEvent ev = MenuEvent::NONE;

        Encoder::mov_t mov = enc.getMov();
        if      (mov == Encoder::ENC_UP)   ev = MenuEvent::DOWN;
        else if (mov == Encoder::ENC_DOWN)  ev = MenuEvent::UP;

        if (ev != MenuEvent::NONE) _dispatch(ev);

        if (enc.getButton()) {
            MenuScreen* s = current();
            if (s && s->screenMode() != MenuScreen::ScreenMode::NAVIGATE)
                _dispatch(MenuEvent::BACK);
            else
                _dispatch(MenuEvent::SELECT);
        }
    }
#endif

    /**
     * @brief Dispatch input from four discrete buttons.
     *
     * Pass `true` on the frame a button is freshly pressed (after your
     * own debouncing). Each bool should be true for exactly one loop().
     *
     * @param btnUp    Up / CCW button.
     * @param btnDown  Down / CW button.
     * @param btnOk    OK / Select button.
     * @param btnBack  Back / Cancel button.
     */
    void update(bool btnUp, bool btnDown, bool btnOk, bool btnBack) {
        if (btnUp)   _dispatch(MenuEvent::UP);
        if (btnDown) _dispatch(MenuEvent::DOWN);
        if (btnOk)   _dispatch(MenuEvent::SELECT);
        if (btnBack) _dispatch(MenuEvent::BACK);
    }

    // ── Draw ──────────────────────────────────────────────────

    /**
     * @brief Render the current screen.
     *
     * Call once per loop(), after update(). Each screen manages its own
     * buffer clear/flush — this method is a pure dispatcher.
     */
    void draw() {
        MenuScreen* s = current();
        if (!s) return;
        _prepareFont();
        s->onDraw(_disp);
    }

    /**
     * @brief Direct access to the underlying U8G2 display.
     *
     * Use for one-off drawing outside the menu loop, e.g. a splash screen.
     */
    U8G2& display() { return _disp; }

private:
    U8G2&       _disp;
    MenuScreen* _stack[MENU_STACK_DEPTH];
    uint8_t     _depth;

    void _dispatch(MenuEvent ev) {
        if (current()) current()->onEvent(ev, *this);
    }

    /** Set a consistent font baseline before handing off to a screen. */
    void _prepareFont() {
        _disp.setFont(MENU_FONT_NORMAL);
        _disp.setFontRefHeightExtendedText();
        _disp.setDrawColor(1);
        _disp.setFontPosTop();
        _disp.setFontDirection(0);
    }
};
