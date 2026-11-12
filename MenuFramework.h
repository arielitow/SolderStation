#pragma once
/**
 * @file MenuFramework.h
 * @brief Umbrella include for the MenuFramework library.
 *
 * Include this single header in your sketch. All screen types,
 * item types, and the MenuContext are pulled in automatically.
 *
 * Configurado para TRIASCAN (OLED 0.96", tiene varios cambios en
 * los layouts de menu y fonts para que se vea bien).
 *
 * ============================================================
 * QUICK START
 * ============================================================
 *
 *  1. #include <MenuFramework.h>  (or "MenuFramework.h" if local)
 *  2. Create a U8G2 instance and a MenuContext, passing it the display.
 *  3. Build your menu tree with the item types below.
 *  4. In loop(), call ctx.update(encoder) or ctx.update(up,dn,ok,back),
 *     then ctx.draw().
 *
 * ============================================================
 * OPTIONAL: Encoder support
 * ============================================================
 *  Define MENU_USE_ENCODER before this include to enable the
 *  Encoder-based update() overload. Encoder.h must be available:
 *
 *    #define MENU_USE_ENCODER
 *    #include <Encoder.h>
 *    #include <MenuFramework.h>
 *
 * ============================================================
 * SCREEN LAYOUT — adjust for your display
 * ============================================================
 */

/// Display width in pixels.
#define MENU_SCREEN_W       128
/// Display height in pixels.
#define MENU_SCREEN_H       64

/// Maximum list items visible at once in a scrolling list.
#define MENU_MAX_VISIBLE    3

/// Height in pixels of the OK / Cancelar button row.
#define MENU_BTNROW_H       12

/// Width in pixels of the sidebar scrollbar.
#define MENU_SCROLLBAR_W    3

/// Left margin where list text starts (after scrollbar + gap).
#define MENU_TEXT_X         (MENU_SCROLLBAR_W + 2)

// ── Root-menu icon panel ──────────────────────────────────────
/// Size (width and height) of the per-entry bitmap on the root menu.
#define MENU_ICON_SIZE      64
/// X coordinate of the vertical separator between list and icon area.
#define MENU_ICON_SEP_X     (MENU_SCREEN_W - MENU_ICON_SIZE - 2)
/// X coordinate where the icon bitmap is drawn.
#define MENU_ICON_X         (MENU_ICON_SEP_X + 2)
/// Width available to the root-menu item list (left of separator).
#define MENU_LIST_W         MENU_ICON_SEP_X

// ============================================================
// STRINGS — change to suit your language / style
// ============================================================
#define MENU_STR_OK       "OK"
#define MENU_STR_CANCEL   "Cancelar"
#define MENU_STR_BACK     "Volver"

// ============================================================
// FONTS
// ============================================================
/// Normal-weight font used for list items and labels.
#define MENU_FONT_NORMAL    u8g2_font_helvR10_tf
/// Bold font used for titles and the selected item.
#define MENU_FONT_BOLD      u8g2_font_helvB10_tf

// ============================================================
// VALUE EDIT DIRECTION
// Swap MENU_VAL_INC / MENU_VAL_DEC if increment feels backwards.
// Does NOT affect list scrolling direction.
// ============================================================
/// Event that increments a value (clockwise / right).
#define MENU_VAL_INC  MenuEvent::DOWN
/// Event that decrements a value (counter-clockwise / left).
#define MENU_VAL_DEC  MenuEvent::UP

// ============================================================
// NAVIGATION STACK DEPTH
// ============================================================
/// Maximum number of nested screens on the stack.
#define MENU_STACK_DEPTH    8

/// Maximum number of editable fields in a MultiValueScreen.
#define MULTI_VAL_MAX_FIELDS 8

// ============================================================
// CORE INCLUDES
// ============================================================
#include <Arduino.h>
#include <U8g2lib.h>
#include <functional>

#ifdef MENU_USE_ENCODER
  #include <Encoder.h>
#endif

// ── Library modules (order matters: dependencies first) ──────
#include "MenuCore.h"
#include "MenuHelpers.h"
#include "ActionItem.h"
#include "ToggleItem.h"
#include "CustomSubMenu.h"
#include "DisplayScreen.h"
#include "FullscreenItem.h"
#include "ValueEditScreen.h"
#include "ValueListScreen.h"
#include "ValueEditConfirmScreen.h"
#include "MultiValueScreen.h"
#include "RootMenu.h"
