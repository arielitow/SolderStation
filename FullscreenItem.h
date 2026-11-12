#pragma once
/**
 * @file FullscreenItem.h
 * @brief A menu entry that hands full 128×64 buffer control to a draw callback.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include "MenuCore.h"
#include <functional>

// =============================================================
//  FULLSCREEN ITEM
// =============================================================
/**
 * @brief A menu entry that gives you complete, unconstrained control of the display.
 *
 * Unlike DisplayScreen, no title bar or any other chrome is drawn.
 * Your draw callback receives the raw U8G2 reference after clearBuffer()
 * has been called — paint whatever you need across the full 128×64.
 *
 * The encoder button (or BACK) returns to the previous screen unless
 * the optional event callback consumes the event first.
 *
 * ── Overload 1: simple draw only ─────────────────────────────
 * @code
 *   FullscreenItem liveView("Live View", [](U8G2& scr) {
 *       scr.drawStr(0, 0, "raw 128x64");
 *   });
 * @endcode
 *
 * ── Overload 2: draw + context-aware event handling ───────────
 * Use when you need to push/pop from within the event handler.
 * @code
 *   FullscreenItem scope("Osciloscopio",
 *       [](U8G2& scr) { drawWaveform(scr); },
 *       [](MenuEvent ev, MenuContext& ctx) -> bool {
 *           if (ev == MenuEvent::UP)   { zoomIn();  return true; }
 *           if (ev == MenuEvent::DOWN) { zoomOut(); return true; }
 *           return false;  // let BACK/SELECT pop normally
 *       }
 *   );
 * @endcode
 *
 * ── Overload 3: draw + event + enter/exit ─────────────────────
 * Use for screens that need display-mode switching or resource setup.
 * @code
 *   FullscreenItem scope("Osciloscopio",
 *       [](U8G2& scr) { drawWaveform(scr); },
 *       [](MenuEvent ev, MenuContext& ctx) -> bool { ... return false; },
 *       []() { modoV(); },   // onEnter: switch to vertical display mode
 *       []() { modoH(); }    // onExit:  restore horizontal display mode
 *   );
 * @endcode
 *
 * The exitCb always fires when the screen pops via the default BACK/SELECT
 * logic. If the eventCb consumes the event (returns true) and also calls
 * ctx.pop() itself, exitCb will NOT fire — handle cleanup inside your
 * eventCb in that case.
 */
class FullscreenItem : public MenuItem, public MenuScreen {
public:
    // Callback type aliases for readability
    using DrawCb    = std::function<void(U8G2&)>;
    using EventCb   = std::function<bool(MenuEvent, MenuContext&)>;
    using VoidCb    = std::function<void()>;

    // ── Overload 1: draw only ────────────────────────────────

    /**
     * @brief Fullscreen item with a draw callback only.
     *
     * BACK or SELECT (when in VIEW_ONLY context) pops the screen.
     *
     * @param label   Name shown in the parent list.
     * @param drawCb  Called every frame with the full U8G2 buffer.
     *                clearBuffer() has already been called. You must
     *                call sendBuffer() yourself at the end. May be nullptr.
     */
    FullscreenItem(const char* label, DrawCb drawCb = nullptr)
        : MenuItem(label), _drawCb(drawCb) {}

    // ── Overload 2: draw + event ─────────────────────────────

    /**
     * @brief Fullscreen item with draw and event callbacks.
     *
     * @param label    Name shown in the parent list.
     * @param drawCb   Called every frame. May be nullptr.
     * @param eventCb  Called on every input event before the default pop
     *                 logic. Return true to consume the event (suppresses
     *                 the default BACK/SELECT → pop behaviour). Return false
     *                 to let the default logic run. May be nullptr.
     */
    FullscreenItem(const char* label, DrawCb drawCb, EventCb eventCb)
        : MenuItem(label), _drawCb(drawCb), _eventCb(eventCb) {}

    // ── Overload 3: draw + event + enter/exit ────────────────

    /**
     * @brief Fullscreen item with all callbacks.
     *
     * @param label    Name shown in the parent list.
     * @param drawCb   Called every frame. May be nullptr.
     * @param eventCb  Event interceptor; return true to consume. May be nullptr.
     * @param enterCb  Called once when the screen is pushed. Use to switch
     *                 display orientation, start streaming, etc. May be nullptr.
     * @param exitCb   Called once just before the screen is popped via the
     *                 default BACK/SELECT logic. Use to restore display mode
     *                 or stop streaming. May be nullptr.
     */
    FullscreenItem(const char* label,
                   DrawCb  drawCb,
                   EventCb eventCb,
                   VoidCb  enterCb,
                   VoidCb  exitCb)
        : MenuItem(label),
          _drawCb(drawCb), _eventCb(eventCb),
          _enterCb(enterCb), _exitCb(exitCb) {}

    // ── Setters (post-construction) ───────────────────────────

    /** @brief Replace the draw callback after construction. May be nullptr. */
    void setDrawCallback (DrawCb  cb) { _drawCb  = cb; }

    /** @brief Replace the event callback after construction. May be nullptr. */
    void setEventCallback(EventCb cb) { _eventCb = cb; }

    /** @brief Replace the enter callback after construction. May be nullptr. */
    void setEnterCallback(VoidCb  cb) { _enterCb = cb; }

    /** @brief Replace the exit callback after construction. May be nullptr. */
    void setExitCallback (VoidCb  cb) { _exitCb  = cb; }

    // ── MenuScreen ────────────────────────────────────────────

    /** @brief Returns FULLSCREEN (caller owns the entire buffer). */
    ScreenMode screenMode() const override { return ScreenMode::FULLSCREEN; }

    /**
     * @brief Fires the enter callback when the screen is pushed.
     * @param ctx  The active MenuContext (not forwarded — use a lambda capture
     *             if you need ctx inside enterCb).
     */
    void onEnter(MenuContext& ctx) override {
        if (_enterCb) _enterCb();
    }

    /**
     * @brief Calls the draw callback with the raw U8G2 reference.
     *
     * Note: unlike normal screens, you are responsible for calling
     * scr.clearBuffer() and scr.sendBuffer() inside your drawCb.
     * @param scr  U8G2 display reference.
     */
    void onDraw(U8G2& scr) override {
        if (_drawCb) _drawCb(scr);
    }

    /**
     * @brief Passes events to eventCb first; fires exitCb and pops on BACK/SELECT.
     *
     * If eventCb returns true the event is consumed and no further action is taken.
     * @param ev   The input event.
     * @param ctx  The active MenuContext.
     */
    void onEvent(MenuEvent ev, MenuContext& ctx) override {
        if (_eventCb && _eventCb(ev, ctx)) return;  // consumed
        if (ev == MenuEvent::BACK || ev == MenuEvent::SELECT) {
            if (_exitCb) _exitCb();
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
    DrawCb  _drawCb;    ///< Raw full-buffer draw callback.
    EventCb _eventCb;   ///< Optional event interceptor.
    VoidCb  _enterCb;   ///< Called once on push.
    VoidCb  _exitCb;    ///< Called once before pop (default logic only).
};
