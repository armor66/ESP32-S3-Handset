#pragma once

#include "common.h"
#include "device.h"
// /*add lib_deps = madhephaestus/ESP32Encoder@^0.11.8 in common.ini*/
// #include <ESP32Encoder.h>
#include <Arduino.h>

extern int16_t handsetEncoder;
extern device_t Encoder_device;

// Encoder Timing (ms)
// =============================================================================
#define ENC_DEBOUNCE_MS      55
#define ENC_CLICK_MS         300    // Max time for a single click
#define ENC_DOUBLE_CLICK_MS  1000    //300 Max time between clicks for double-click
#define ENC_LONG_PRESS_MS    2500    // Long press for shutdown
#define ENC_LONG_PRESS_SHORT 1200   // Short long press (stopwatch reset, easter egg)

enum class EncoderEvent {
    NONE,
    ROTATE_CW,
    ROTATE_CCW,
    CLICK,
    DOUBLE_CLICK,
    LONG_PRESS,        // ~3s — shutdown
    LONG_PRESS_SHORT   // ~1.5s — context-specific (stopwatch reset, easter egg)
};

class Encoder {

private:
    static void IRAM_ATTR _isrEncoder();
    static Encoder* _instance;

    // Quadrature state tracking
    volatile uint8_t _lastQuadState = 0;
    volatile int8_t _quadAccum = 0;
    volatile int _rotCounter = 0;

    // Button state
    bool _pressed = false;
    bool _lastPressed = false;
    bool _lastRawPressed = false;
    uint32_t _lastDebounceTime = 0;

    uint32_t _pressStart = 0;
    uint32_t _lastClickTime = 0;
    bool _longPressFired = false;
    bool _shortLongPressFired = false;
    bool _waitingDoubleClick = false;

    EncoderEvent _pendingEvent = EncoderEvent::NONE;

    // bool _readSW();

public:
    // void init();
    // void update();
    EncoderEvent getEvent();

    bool _readSW() { return !digitalRead(GPIO_ENC_SW); } // Active low

    void init()
    {
    _instance = this;

    pinMode(GPIO_ENC_CL, INPUT_PULLUP);
    pinMode(GPIO_ENC_DT, INPUT_PULLUP);
    pinMode(GPIO_ENC_SW, INPUT_PULLUP);    

    // Read initial state
    _lastQuadState = (digitalRead(GPIO_ENC_CL) << 1) | digitalRead(GPIO_ENC_DT);

    attachInterrupt(digitalPinToInterrupt(GPIO_ENC_CL), _isrEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(GPIO_ENC_DT), _isrEncoder, CHANGE);
    }

    void update()
    {
        uint32_t now = millis();

    // --- Rotation ---
    noInterrupts();
    int rot = _rotCounter;
    _rotCounter = 0;
    interrupts();

    if (rot > 0) {
        _pendingEvent = EncoderEvent::ROTATE_CW;
        // (ChannelData[10] > 1792)? (ChannelData[10] = 1792): ChannelData[10]+=10;
        handsetEncoder++;
        return;
    } else if (rot < 0) {
        _pendingEvent = EncoderEvent::ROTATE_CCW;
        // (ChannelData[10] < 191)? (ChannelData[10] = 191): ChannelData[10]-=10;
        handsetEncoder--;
        return;
    }

    // --- Button ---
    bool rawPressed = _readSW();

    // Software debounce
    if (rawPressed != _lastRawPressed) {
        _lastDebounceTime = now;
    }
    _lastRawPressed = rawPressed;

    if ((now - _lastDebounceTime) < ENC_DEBOUNCE_MS) {
        return; // Still bouncing
    }

    _pressed = rawPressed;

    // Button just pressed
    if (_pressed && !_lastPressed) {
        _pressStart = now;
        _longPressFired = false;
        _shortLongPressFired = false;
    }

    // Button held — check long presses
    if (_pressed && _pressStart > 0) {
        uint32_t held = now - _pressStart;

        if (held >= ENC_LONG_PRESS_MS && !_longPressFired) {
            _longPressFired = true;
            _pendingEvent = EncoderEvent::LONG_PRESS;
            _lastClickTime = 0;
            _lastPressed = _pressed;
            return;
        }

        if (held >= ENC_LONG_PRESS_SHORT && !_shortLongPressFired && !_longPressFired) {
            _shortLongPressFired = true;
            _pendingEvent = EncoderEvent::LONG_PRESS_SHORT;
            _lastPressed = _pressed;
            return;
        }
    }

    // Button just released (short press)
    if (!_pressed && _lastPressed) {
        uint32_t held = now - _pressStart;

        if (!_longPressFired && !_shortLongPressFired && held < ENC_CLICK_MS) {
            if (_waitingDoubleClick && (now - _lastClickTime) < ENC_DOUBLE_CLICK_MS) {
                // Second click within window → double click
                _pendingEvent = EncoderEvent::DOUBLE_CLICK;
                ChannelData[7] = 1003;
                handsetEncoder = 0;
                _waitingDoubleClick = false;
                _lastClickTime = 0;
            } else {
                // First click — defer until double-click window expires
                _lastClickTime = now;
                _waitingDoubleClick = true;
            }
        }
    }

    // Emit deferred single click after double-click window expires
    if (_waitingDoubleClick && !_pressed && (now - _lastClickTime) >= ENC_DOUBLE_CLICK_MS) {
        _pendingEvent = EncoderEvent::CLICK;
        _waitingDoubleClick = false;
        _lastClickTime = 0;
    }

    _lastPressed = _pressed;
    }
};