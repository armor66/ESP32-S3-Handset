#include "devEncoder.h"

static Encoder encoder1;

#define ENCODER_PERIOD_MS   1;

int16_t handsetEncoder = 0;

Encoder* Encoder::_instance = nullptr;

// Quadrature state table — much more reliable than simple CLK/DT check
// Each detent of KY-040 goes through a full quadrature cycle (4 states)
// We track the full cycle and only emit a step when a complete detent is detected
static const int8_t QUAD_TABLE[] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

// void Encoder::init() {
// }

void IRAM_ATTR Encoder::_isrEncoder() {
    if (!_instance) return;

    uint8_t newState = (digitalRead(GPIO_ENC_CL) << 1) | digitalRead(GPIO_ENC_DT);
    uint8_t idx = (_instance->_lastQuadState << 2) | newState;
    int8_t delta = QUAD_TABLE[idx & 0x0F];
    _instance->_lastQuadState = newState;

    _instance->_quadAccum += delta;

    // Emit step every 2 state changes (half-step — works on most KY-040)
    if (_instance->_quadAccum >= 2) {
        (ChannelData[7] > 1792)? (ChannelData[7] = 1792): ChannelData[7]+=25;
        _instance->_rotCounter++;
        _instance->_quadAccum = 0;
    } else if (_instance->_quadAccum <= -2) {
        (ChannelData[7] < 191)? (ChannelData[7] = 191): ChannelData[7]-=25;
        _instance->_rotCounter--;
        _instance->_quadAccum = 0;
    }
}

// bool Encoder::_readSW() { return !digitalRead(GPIO_ENC_SW); } // Active low

// void Encoder::update() {
// }

EncoderEvent Encoder::getEvent() {
    EncoderEvent e = _pendingEvent;
    _pendingEvent = EncoderEvent::NONE;
    return e;
}

static int start()
{
    ChannelData[7] = 1003;
    encoder1.init();
    return DURATION_IMMEDIATELY;
}

static int timeout()
{
    extern volatile bool busyTransmitting;
    static bool fullWait = true;

    if (fullWait && busyTransmitting && connectionState < MODE_STATES) return DURATION_IMMEDIATELY;
    fullWait = false;

    if (!busyTransmitting && connectionState < MODE_STATES) return DURATION_IMMEDIATELY;

    encoder1.update();

    fullWait = true;
    return ENCODER_PERIOD_MS;
}

device_t Encoder_device = {
    .initialize = nullptr,
    .start = start,
    .event = nullptr,
    .timeout = timeout,
};