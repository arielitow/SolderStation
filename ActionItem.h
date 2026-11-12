#pragma once
/**
 * @file ActionItem.h
 * @brief A menu list entry that executes a callback when selected.
 *
 * Do not include this file directly in your sketch.
 * Include MenuFramework.h instead.
 */

#include "MenuCore.h"
#include <functional>

// =============================================================
//  ACTION ITEM
// =============================================================
/**
 * @brief A menu entry that runs a callback when the user selects it.
 *
 * Two constructor overloads are provided:
 *
 *  - Simple void callback — use when you only need to trigger an action:
 *    @code
 *      ActionItem reset("Reset ADC", []{ resetADC(); });
 *    @endcode
 *
 *  - Context-aware callback — use when you need to push a screen or
 *    navigate programmatically after the action:
 *    @code
 *      ActionItem go("Go to screen", [](MenuContext& ctx){
 *          ctx.push(&myScreen);
 *      });
 *    @endcode
 *
 * Both callbacks are called (if set) in onSelect(). Combining them in
 * one instance is intentional — you can trigger an action AND push a
 * screen in the same item.
 */
class ActionItem : public MenuItem {
public:
    /**
     * @brief Construct with a simple void callback.
     *
     * @param label  Text shown in the parent list.
     * @param cb     Callback invoked on SELECT. May be nullptr.
     */
    ActionItem(const char* label, std::function<void()> cb)
        : MenuItem(label), _cb(cb) {}

    /**
     * @brief Construct with a context-aware callback.
     *
     * Use this overload when you need to push a screen or call
     * ctx.pop() / ctx.popToRoot() as part of the action.
     *
     * @param label  Text shown in the parent list.
     * @param cb     Callback invoked on SELECT, receiving the active
     *               MenuContext. May be nullptr.
     */
    ActionItem(const char* label, std::function<void(MenuContext&)> cb)
        : MenuItem(label), _cbCtx(cb) {}

    /**
     * @brief Executes the configured callback(s) when this item is selected.
     *
     * Called automatically by the parent menu — do not call directly.
     * @param ctx  The active MenuContext.
     */
    void onSelect(MenuContext& ctx) override {
        if (_cb)    _cb();
        if (_cbCtx) _cbCtx(ctx);
    }

private:
    std::function<void()>           _cb;     ///< Simple callback (no ctx).
    std::function<void(MenuContext&)> _cbCtx; ///< Context-aware callback.
};
