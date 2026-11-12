#pragma once
/**
 * @file DisplayScreen.h
 * @brief A read-only display screen with a title bar and flexible callbacks.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include "MenuCore.h"
#include "MenuHelpers.h"
#include <functional>

// =============================================================
//  DISPLAY SCREEN
// =============================================================
/**
 * @brief A VIEW_ONLY screen that draws a title bar and delegates
 *        content rendering to a callback.
 *
 * The encoder button or BACK returns to the previous screen.
 * Three constructor overloads are available — pick the one that
 * matches your needs:
 *
 * ── Overload 1: simple draw callback ──────────────────────────
 * Basic usage. No access to MenuContext inside the callback.
 * @code
 *   DisplayScreen meas("Medición", [](U8G2& scr) {
 *       scr.setCursor(10, 20);
 *       scr.print(myValue);
 *   });
 * @endcode
 *
 * ── Overload 2: context-aware draw callback ───────────────────
 * Use when you need to auto-pop (e.g. after connecting to WiFi)
 * or push another screen from inside the draw loop.
 * @code
 *   DisplayScreen status("Estado", [](U8G2& scr, MenuContext& ctx) {
 *       if (done) { ctx.pop(); return; }
 *       scr.print("Procesando...");
 *   });
 * @endcode
 *
 * ── Overload 3: full bells and whistles ───────────────────────
 * Adds enter and exit callbacks alongside the context-aware draw.
 * Use for screens that need setup/teardown (e.g. start a timer on
 * enter, stop it on exit).
 * @code
 *   DisplayScreen live("Live",
 *       [](U8G2& scr, MenuContext& ctx) { drawLiveData(scr); },
 *       [](MenuContext& ctx) { startSampling(); },   // onEnter
 *       [](MenuContext& ctx) { stopSampling();  }    // onExit
 *   );
 * @endcode
 *
 * All three overloads support an optional BACK/SELECT event handler.
 * Return true from it to suppress the default pop behaviour:
 * @code
 *   DisplayScreen confirm("Confirmar",
 *       [](U8G2& scr, MenuContext& ctx) { scr.print("Seguro?"); },
 *       nullptr,  // no onEnter
 *       nullptr,  // no onExit
 *       [](MenuEvent ev, MenuContext& ctx) -> bool {
 *           if (ev == MenuEvent::SELECT) { doThing(); ctx.pop(); return true; }
 *           return false;  // BACK pops normally
 *       }
 *   );
 * @endcode
 */
class DisplayScreen : public MenuItem, public MenuScreen {
public:
    // Callback type aliases for readability
    using DrawCb    = std::function<void(U8G2&)>;
    using DrawCtxCb = std::function<void(U8G2&, MenuContext&)>;
    using CtxCb     = std::function<void(MenuContext&)>;
    using EventCb   = std::function<bool(MenuEvent, MenuContext&)>;

    // ── Overload 1: simple draw callback ─────────────────────

    /**
     * @brief Basic display screen with a simple draw callback.
     *
     * @param title   Title shown in the title bar and in the parent list.
     * @param drawCb  Called every frame to render content below the title
     *                bar. Receives only the U8G2 reference. May be nullptr
     *                to show an empty screen.
     */
    explicit DisplayScreen(const char* title, DrawCb drawCb = nullptr)
        : MenuItem(title), _drawCb(drawCb) {}

    // ── Overload 2: context-aware draw ───────────────────────

    /**
     * @brief Display screen with a draw callback that receives MenuContext.
     *
     * Use this overload when you need to call ctx.pop(), ctx.push(), or
     * ctx.popToRoot() from inside the draw loop (e.g. auto-return after
     * an async operation completes).
     *
     * @param title      Title shown in the title bar and in the parent list.
     * @param drawCtxCb  Called every frame; receives U8G2 and MenuContext.
     *                   May be nullptr.
     */
    DisplayScreen(const char* title, DrawCtxCb drawCtxCb)
        : MenuItem(title), _drawCtxCb(drawCtxCb) {}

    // ── Overload 3: full callbacks ────────────────────────────

    /**
     * @brief Display screen with context-aware draw, enter, exit, and event callbacks.
     *
     * @param title      Title shown in the title bar and in the parent list.
     * @param drawCtxCb  Called every frame; receives U8G2 and MenuContext.
     *                   May be nullptr.
     * @param enterCb    Called once when the screen is pushed onto the stack.
     *                   Use to start timers, begin async operations, etc.
     *                   May be nullptr.
     * @param exitCb     Called once just before the screen is popped (whether
     *                   by BACK, SELECT, or a ctx.pop() inside drawCtxCb).
     *                   Use to stop timers, clean up resources, etc.
     *                   May be nullptr.
     * @param eventCb    Optional input handler called before the default BACK/
     *                   SELECT pop logic. Return true to consume the event and
     *                   suppress the default behaviour; false to let it run.
     *                   May be nullptr.
     */
    DisplayScreen(const char* title,
                  DrawCtxCb drawCtxCb,
                  CtxCb     enterCb,
                  CtxCb     exitCb,
                  EventCb   eventCb = nullptr)
        : MenuItem(title),
          _drawCtxCb(drawCtxCb),
          _enterCb(enterCb),
          _exitCb(exitCb),
          _eventCb(eventCb) {}

    // ── Setters (post-construction) ───────────────────────────

    /**
     * @brief Replace or set the simple draw callback after construction.
     * @param cb  New draw callback. Clears any context-aware draw callback.
     */
    void setDrawCallback(DrawCb cb)    { _drawCb = cb; _drawCtxCb = nullptr; }

    /**
     * @brief Replace or set the context-aware draw callback after construction.
     * @param cb  New draw callback. Clears any simple draw callback.
     */
    void setDrawCallback(DrawCtxCb cb) { _drawCtxCb = cb; _drawCb = nullptr; }

    /**
     * @brief Set or replace the enter callback after construction.
     * @param cb  Called once when the screen is pushed. May be nullptr.
     */
    void setEnterCallback(CtxCb cb)  { _enterCb  = cb; }

    /**
     * @brief Set or replace the exit callback after construction.
     * @param cb  Called once just before the screen is popped. May be nullptr.
     */
    void setExitCallback(CtxCb cb)   { _exitCb   = cb; }

    /**
     * @brief Set or replace the event callback after construction.
     * @param cb  Called on BACK/SELECT before default pop logic. May be nullptr.
     */
    void setEventCallback(EventCb cb) { _eventCb  = cb; }

    // ── MenuScreen ────────────────────────────────────────────

    /** @brief Returns VIEW_ONLY (encoder button = BACK). */
    ScreenMode screenMode() const override { return ScreenMode::VIEW_ONLY; }

    /**
     * @brief Stores the context reference and fires the enter callback.
     * @param ctx  The active MenuContext.
     */
    void onEnter(MenuContext& ctx) override {
        _ctx = &ctx;
        if (_enterCb) _enterCb(ctx);
    }

    /**
     * @brief Clears the buffer, draws the title bar, then calls the draw callback.
     * @param scr  U8G2 display reference.
     */
    void onDraw(U8G2& scr) override {
        _beginDraw(scr);
        _menuDrawTitleBar(scr, _label);
        if (_drawCb    && _ctx) _drawCb(scr);
        if (_drawCtxCb && _ctx) _drawCtxCb(scr, *_ctx);
        _endDraw(scr);
    }

    /**
     * @brief Handles BACK and SELECT: fires eventCb first, then pops if not consumed.
     *
     * If an exitCb is set, it fires before the pop.
     * @param ev   The input event.
     * @param ctx  The active MenuContext.
     */
    void onEvent(MenuEvent ev, MenuContext& ctx) override {
        if (_eventCb && _eventCb(ev, ctx)) return;  // consumed
        if (ev == MenuEvent::BACK || ev == MenuEvent::SELECT) {
            if (_exitCb) _exitCb(ctx);
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
    DrawCb    _drawCb;       ///< Simple draw callback (no ctx).
    DrawCtxCb _drawCtxCb;    ///< Context-aware draw callback.
    CtxCb     _enterCb;      ///< Called once on push.
    CtxCb     _exitCb;       ///< Called once before pop.
    EventCb   _eventCb;      ///< Optional event interceptor.
    MenuContext* _ctx = nullptr; ///< Stored on onEnter for use in onDraw.
};
