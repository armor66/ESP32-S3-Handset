#include "devAnalogVbat.h"

//my@ #if defined(USE_ANALOG_VBAT)
#include <Arduino.h>
#include "CRSF.h"
#include "telemetry.h"
#include "median.h"
#include "logging.h"

// Sample 5x samples over 500ms (unless SlowUpdate)
#define VBAT_SMOOTH_CNT         5
// my@ #if defined(DEBUG_VBAT_ADC)
// #define VBAT_SAMPLE_INTERVAL    20U // faster updates in debug mode
// #else
#define VBAT_SAMPLE_INTERVAL    100U
//my@ #endif

typedef uint16_t vbatAnalogStorage_t;
static MedianAvgFilter<vbatAnalogStorage_t, VBAT_SMOOTH_CNT>vbatSmooth;
static uint8_t vbatUpdateScale;
static bool permit_halt = false;
static bool buzzer_once = true;
//extern variables:
bool screen_on = true;
uint8_t screen_counter = 0;

#if defined(PLATFORM_ESP32)
#include "esp_adc_cal.h"
static esp_adc_cal_characteristics_t *vbatAdcUnitCharacterics;
#endif

/* Shameful externs */
extern Telemetry telemetry;

/**
 * @brief: Enable SlowUpdate mode to reduce the frequency Vbat telemetry is sent
 ***/
void Vbat_enableSlowUpdate(bool enable)
{
    vbatUpdateScale = enable ? 2 : 1;
}

static int start()
{
    if (GPIO_ANALOG_VBAT == UNDEF_PIN)
    {
        return DURATION_NEVER;
    }
    vbatUpdateScale = 1;
#if defined(PLATFORM_ESP32)
    analogReadResolution(12);

    int atten = hardware_int(HARDWARE_vbat_atten);
    if (atten != -1)
    {
        // if the configured value is higher than the max item (11dB, it indicates to use cal_characterize)
        bool useCal = atten > ADC_11db;
        if (useCal)
        {
            atten -= (ADC_11db + 1);

            vbatAdcUnitCharacterics = new esp_adc_cal_characteristics_t();
            int8_t channel = digitalPinToAnalogChannel(GPIO_ANALOG_VBAT);
            adc_unit_t unit = (channel > (SOC_ADC_MAX_CHANNEL_NUM - 1)) ? ADC_UNIT_2 : ADC_UNIT_1;
            esp_adc_cal_characterize(unit, (adc_atten_t)atten, ADC_WIDTH_BIT_12, 3300, vbatAdcUnitCharacterics);
        }
        analogSetPinAttenuation(GPIO_ANALOG_VBAT, (adc_attenuation_t)atten);
    }
#endif
    return VBAT_SAMPLE_INTERVAL;
}

int32_t handset_vbat = 0;
static void reportVbat()
{
    uint32_t adc = vbatSmooth.calc();
#if defined(PLATFORM_ESP32) && !defined(DEBUG_VBAT_ADC)
    if (vbatAdcUnitCharacterics)
        adc = esp_adc_cal_raw_to_voltage(adc, vbatAdcUnitCharacterics);
#endif

    // For negative offsets, anything between abs(OFFSET) and 0 is considered 0
    if (ANALOG_VBAT_OFFSET < 0 && adc <= -ANALOG_VBAT_OFFSET)
        handset_vbat = 0;
    else
        handset_vbat = ((int32_t)adc - ANALOG_VBAT_OFFSET) * 100 / ANALOG_VBAT_SCALE;

    //my@ CRSF_MK_FRAME_T(crsf_sensor_battery_t) crsfbatt = { 0 };
    // // Values are MSB first (BigEndian)
    // crsfbatt.p.voltage = htobe16((uint16_t)vbat);
    // // No sensors for current, capacity, or remaining available

    // CRSF::SetHeaderAndCrc((uint8_t *)&crsfbatt, CRSF_FRAMETYPE_BATTERY_SENSOR, CRSF_FRAME_SIZE(sizeof(crsf_sensor_battery_t)), CRSF_ADDRESS_CRSF_TRANSMITTER);
    // telemetry.AppendTelemetryPackage((uint8_t *)&crsfbatt);
}
#include "handset.h"    //might be worth move to devScreen.cpp
static int timeout()
{
/*when GPIO_MODE_OUTPUT gpio_get_level(pin), always returns "0"*/
    if(!gpio_get_level(GPIO_NUM_47) && !handset->IsArmed()) //PWR_BTN pressed & not armed
    {
        if(screen_counter < 20) { screen_counter++; }    //two seconds delay
        else
        {
            if(permit_halt)
            {
                digitalWrite(GPIO_PIN_TFT_BL, 0);
                gpio_set_level(GPIO_NUM_48, 0);
                permit_halt = false;    //merely for test and
                screen_counter = 0;     //more reliable halt
            }
            else
            {
                digitalWrite(GPIO_PIN_TFT_BL, 1);
                gpio_set_level(GPIO_NUM_48, 1);
                if(buzzer_once)
                {
                    buzzer_once = false;
                    gpio_set_level(GPIO_NUM_45, 0); //BUZZER ON
                }
            }
        }
    }
    else if(handset->IsArmed())
    {
        if(screen_on)
        {
            if(screen_counter < 100) { screen_counter++; }    //10 seconds delay
            else
            {
                digitalWrite(GPIO_PIN_TFT_BL, 0);
                screen_on = false;
                screen_counter = 0;
            }
        }
    }
    else    //if(gpio_get_level(GPIO_NUM_47)) and not armed
    {
        screen_counter = 0;
        permit_halt = true;
        if(!screen_on)
        {
            digitalWrite(GPIO_PIN_TFT_BL, 1);
            screen_on = true; 
        }
    }
    // if (GPIO_ANALOG_VBAT == UNDEF_PIN || telemetry.GetCrsfBatterySensorDetected())
    // {
    //     return DURATION_NEVER;
    // }

    uint32_t adc = analogRead(GPIO_ANALOG_VBAT);
#if defined(PLATFORM_ESP32) && defined(DEBUG_VBAT_ADC)
    // When doing DEBUG_VBAT_ADC, every value is adjusted (for logging)
    // in normal mode only the final value is adjusted to save CPU cycles
    if (vbatAdcUnitCharacterics)
        adc = esp_adc_cal_raw_to_voltage(adc, vbatAdcUnitCharacterics);
    DBGLN("$ADC,%u", adc);
#endif
vbatSmooth.add(adc);
    unsigned int idx = vbatSmooth.add(adc);
    if (idx == 0)   reportVbat();   // && connectionState == connected
    return VBAT_SAMPLE_INTERVAL * vbatUpdateScale;
}

device_t AnalogVbat_device = {
    .initialize = nullptr,
    .start = start,
    .event = nullptr,
    .timeout = timeout,
};

//my@ #endif /* if USE_ANALOG_VCC */