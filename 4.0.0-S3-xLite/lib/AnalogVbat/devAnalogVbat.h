#pragma once

#include "common.h"
#include "device.h"

void Vbat_enableSlowUpdate(bool enable);

//my@
extern bool screen_on;
extern uint8_t screen_counter;
extern int32_t handset_vbat;
// extern int32_t handset_slider;

extern device_t AnalogVbat_device;
