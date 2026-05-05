#pragma once

#include "targets.h"
#include "device.h"

enum adc_reading {
    ADC_JOYSTICK,
    ADC_PA_PDET,
    ADC_MAX_DEVICES
};

extern int getADCReading(adc_reading reading);
extern device_t ADC_device;

extern bool adcNoData;
extern int8_t adcCounter;
extern int16_t adcBytes;
extern int16_t adcRaw;

void endPointsCalibration(void);    //call in menu.cpp->displayValueIndex()
void centrValsCalibration(void);