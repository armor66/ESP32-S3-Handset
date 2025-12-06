#pragma once

#include "common.h"
#include "device.h"

//my@ #if defined(USE_ANALOG_VBAT)
void Vbat_enableSlowUpdate(bool enable);

extern device_t AnalogVbat_device;
//my@ #endif
extern int32_t handset_vbat;