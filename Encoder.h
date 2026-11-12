// Encoder.h — PCNT-based quadrature encoder driver for ESP32 / ESP32-S3
// Single-header, drop-in replacement. No ISR needed.
#pragma once

#include <Arduino.h>
#include "driver/pcnt.h"
#include "esp_timer.h"

class Encoder {
public:
    // ── Types ────────────────────────────────────────────────

    /** Callback type for long-press events. */
    typedef void (*longpress_cb_t)();

    /**
     * Direction of the last rotation query.
     * Matches the sign of getRotation(): UP = positive, DOWN = negative.
     */
    typedef enum {
        ENC_IDLE =  0,
        ENC_DOWN = -1,
        ENC_UP   =  1
    } mov_t;

    /**
     * Electrical pulses per mechanical detent of your encoder.
     *
     *  ONE_PER_DETENT — common EC11-style encoders: one full A/B cycle per
     *                   click. The PCNT sees 4 edges per detent (both edges
     *                   on both channels). We count all 4 and divide by 4,
     *                   giving exactly ±1 per detent with no dropped steps.
     *
     *  TWO_PER_DETENT — encoders that produce two full A/B cycles per
     *                   click (e.g. some Alps or Bourns models). Same PCNT
     *                   setup; divide by 2 instead.
     */
    typedef enum {
        ONE_PER_DETENT = 2,   // divisor stored directly in the enum value
        TWO_PER_DETENT = 4
    } enc_type_t;

    // ── Constructor ──────────────────────────────────────────

    /**
     * @param pinA    Encoder channel A (CLK). Any GPIO with input capability.
     * @param pinB    Encoder channel B (DT).  Any GPIO with input capability.
     * @param pinBtn  Button pin (active LOW, internal pull-up enabled), or
     *                255 to disable button support entirely.
     * @param type    Pulses per detent — ONE_PER_DETENT for standard EC11-
     *                style encoders, TWO_PER_DETENT for encoders that produce
     *                two electrical cycles per mechanical click.
     * @param unit    PCNT hardware unit (0–7). Change if another peripheral
     *                already claims PCNT_UNIT_0.
     */
    Encoder(uint8_t pinA, uint8_t pinB,
            uint8_t pinBtn  = 255,
            enc_type_t type = ONE_PER_DETENT,
            pcnt_unit_t unit = PCNT_UNIT_0)
        : _pinA(pinA), _pinB(pinB), _pinBtn(pinBtn),
          _type(type), _pcntUnit(unit)
    {}

    // ── Lifecycle ────────────────────────────────────────────

    /**
     * Initialise GPIO and PCNT hardware. Call once from setup() or the
     * task that owns this encoder before any other method.
     */
    void begin() {
        pinMode(_pinA, INPUT_PULLUP);
        pinMode(_pinB, INPUT_PULLUP);

        if (_pinBtn != 255) {
            pinMode(_pinBtn, INPUT_PULLUP);
            _lastBtnState = (digitalRead(_pinBtn) == LOW);
        }

        _initPCNT();
    }

    // ── Rotation ─────────────────────────────────────────────

    /**
     * Returns the number of detents rotated since the last call.
     * Positive = clockwise (UP), negative = counter-clockwise (DOWN).
     * Returns 0 if the encoder has not moved.
     *
     * Safe to call from any task or the main loop — PCNT is a hardware
     * counter so no shared state beyond the atomic register read.
     */
    int getRotation() {
        int16_t raw = 0;
        pcnt_get_counter_value(_pcntUnit, &raw);
        if (raw == 0) return 0;

        // Accumulate into a 32-bit running total to survive rapid rotation
        // that might exceed the ±1 detent we expect between calls.
        _accumulator += raw;
        pcnt_counter_clear(_pcntUnit);

        // Extract whole detents, leave sub-detent remainder for next call.
        // This prevents the integer-division-to-zero bug at slow speeds.
        int divisor = static_cast<int>(_type);
        int detents = _accumulator / divisor;
        _accumulator %= divisor;   // keep remainder

        return detents;
    }

    /**
     * Returns the direction of the last movement, or ENC_IDLE.
     * Consumes the movement (same as calling getRotation()).
     */
    mov_t getMov() {
        int v = getRotation();
        if (v > 0) return ENC_UP;
        if (v < 0) return ENC_DOWN;
        return ENC_IDLE;
    }

    // ── Button ───────────────────────────────────────────────

    /**
     * Debounced button query. Returns true exactly once per press event
     * (on the falling edge, i.e. when the button is first pushed down).
     *
     * Also drives long-press detection internally — call this regularly
     * (every loop iteration or from the menu update task) to ensure the
     * long-press callback fires on time.
     *
     * Returns false if no button pin was configured (pinBtn == 255).
     */
    bool getButton() {
        if (_pinBtn == 255) return false;

        bool pressed = (digitalRead(_pinBtn) == LOW);
        int64_t now  = esp_timer_get_time();  // microseconds

        if (pressed != _lastBtnState) {
            if ((now - _lastBtnTime) > _DEBOUNCE_US) {
                _lastBtnTime  = now;
                _lastBtnState = pressed;

                if (pressed) {
                    // Falling edge — button just pushed down
                    _btnPressStart  = now;
                    _longpressFired = false;
                    return true;
                }
                // Rising edge — button released, no short-press event here
            }
        }

        // Long-press: fire callback once while held past threshold
        if (_longpressEnabled && pressed && !_longpressFired && _longpressCb) {
            int64_t heldMs = (now - _btnPressStart) / 1000;
            if (heldMs >= (int64_t)_longpressMs) {
                _longpressFired = true;
                _longpressCb();
            }
        }

        return false;
    }

    // ── Long-press configuration ─────────────────────────────

    /**
     * Register a long-press callback and the hold duration that triggers it.
     * @param cb      Function to call when the button is held long enough.
     * @param timeMs  Hold duration in milliseconds (default 800).
     */
    void setLongpress(longpress_cb_t cb, uint32_t timeMs = 800) {
        _longpressCb = cb;
        _longpressMs = timeMs;
    }

    void enableLongpress()  { _longpressEnabled = true;  }
    void disableLongpress() { _longpressEnabled = false; }

    // ── Utility ──────────────────────────────────────────────

    /** Reset the accumulated step count without returning it. */
    void clearRotation() {
        pcnt_counter_clear(_pcntUnit);
        _accumulator = 0;
    }

private:
    // ── Pins & config ────────────────────────────────────────
    uint8_t     _pinA;
    uint8_t     _pinB;
    uint8_t     _pinBtn;
    enc_type_t  _type;
    pcnt_unit_t _pcntUnit;

    // ── Rotation accumulator ─────────────────────────────────
    // Holds sub-detent remainder between getRotation() calls so that
    // slow movement (raw=1 or 2) is not silently dropped by integer division.
    int32_t _accumulator = 0;

    // ── Button state ─────────────────────────────────────────
    bool    _lastBtnState   = false;
    int64_t _lastBtnTime    = 0;
    int64_t _btnPressStart  = 0;
    bool    _longpressFired = false;

    static constexpr int64_t _DEBOUNCE_US = 5000;  // 5 ms

    // ── Long-press ───────────────────────────────────────────
    longpress_cb_t _longpressCb      = nullptr;
    bool           _longpressEnabled = false;
    uint32_t       _longpressMs      = 800;

    // ── PCNT init ────────────────────────────────────────────

    /**
     * Configure two PCNT channels on the same unit for full quadrature
     * decoding (X4 mode): both edges on both A and B are counted, with
     * the opposite channel controlling direction. This gives 4 hardware
     * counts per detent on a ONE_PER_DETENT encoder, which getRotation()
     * divides back to ±1 per click — no dropped steps at any speed.
     *
     * Channel 0: A is pulse input, B is direction control.
     * Channel 1: B is pulse input, A is direction control.
     */
    void _initPCNT() {
        // ── Channel 0: count edges on A, B controls direction ──
        pcnt_config_t ch0 = {};
        ch0.pulse_gpio_num = (int)_pinA;
        ch0.ctrl_gpio_num  = (int)_pinB;
        ch0.unit           = _pcntUnit;
        ch0.channel        = PCNT_CHANNEL_0;
        ch0.pos_mode       = PCNT_COUNT_INC;   // A rising  → +1
        ch0.neg_mode       = PCNT_COUNT_DEC;   // A falling → -1
        ch0.hctrl_mode     = PCNT_MODE_KEEP;   // B HIGH: keep direction
        ch0.lctrl_mode     = PCNT_MODE_REVERSE;// B LOW:  reverse direction
        ch0.counter_h_lim  =  32767;
        ch0.counter_l_lim  = -32768;
        pcnt_unit_config(&ch0);

        // ── Channel 1: count edges on B, A controls direction ──
        pcnt_config_t ch1 = {};
        ch1.pulse_gpio_num = (int)_pinB;
        ch1.ctrl_gpio_num  = (int)_pinA;
        ch1.unit           = _pcntUnit;
        ch1.channel        = PCNT_CHANNEL_1;
        ch1.pos_mode       = PCNT_COUNT_DEC;   // B rising  → -1 (opposite
        ch1.neg_mode       = PCNT_COUNT_INC;   // B falling → +1  polarity)
        ch1.hctrl_mode     = PCNT_MODE_KEEP;   // A HIGH: keep direction
        ch1.lctrl_mode     = PCNT_MODE_REVERSE;// A LOW:  reverse direction
        ch1.counter_h_lim  =  32767;
        ch1.counter_l_lim  = -32768;
        pcnt_unit_config(&ch1);

        // Glitch filter: ignore pulses shorter than ~1 µs (80 APB cycles
        // at 80 MHz). Eliminates contact bounce without masking real edges.
        // Raise this value (up to 1023) if you still see spurious counts.
        pcnt_set_filter_value(_pcntUnit, 80);
        pcnt_filter_enable(_pcntUnit);

        pcnt_counter_pause(_pcntUnit);
        pcnt_counter_clear(_pcntUnit);
        pcnt_counter_resume(_pcntUnit);
    }
};
