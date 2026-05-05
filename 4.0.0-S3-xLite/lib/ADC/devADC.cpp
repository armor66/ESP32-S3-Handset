#include "targets.h"
//my@ #if defined(TARGET_TX)
#include "common.h"
#include "devADC.h"

//my@
/********************************************************************************/
#include "config.h" //?
#include <driver/adc.h>
#include <driver/gpio.h>
#include "handset.h"

#define CHANNELS            4
#define SAMPLES             4
#define TIMES               CHANNELS*SAMPLES      //channels * samples per channel
#define GET_UNIT(x) ((x>>3) & 0x1)
//CONFIG_IDF_TARGET_ESP32S3
#define ADC_RESULT_BYTE     4
#define ADC_CONV_LIMIT_EN   0
#define ADC_CONV_MODE       ADC_CONV_SINGLE_UNIT_1      //ADC_CONV_BOTH_UNIT
#define ADC_OUTPUT_TYPE     ADC_DIGI_OUTPUT_FORMAT_TYPE2

// #define BUFFER              TIMES*ADC_RESULT_BYTE
// static const uint8_t channels_to_convert = 2;   //6
static uint16_t adc1_chan_mask = BIT(0) | BIT(1) | BIT(3) | BIT(4); // | BIT(5) | BIT(6), ADC1_CHANNEL_5 , ADC1_CHANNEL_6
static uint16_t adc2_chan_mask = 0;
static adc1_channel_t channel[CHANNELS] = {ADC1_CHANNEL_0, ADC1_CHANNEL_1, ADC1_CHANNEL_3, ADC1_CHANNEL_4};
// static adc2_channel_t adc2channel[2] = {(ADC2_CHANNEL_8 | 1 << 3), (ADC2_CHANNEL_9 | 1 << 3)};
static const uint16_t bufferSize = CHANNELS * SAMPLES * ADC_RESULT_BYTE;
uint8_t analogReadBuffer[bufferSize] = { 0 };
bool adcStatus = false;
bool permitArming = false;
bool adcNoData = false;
int8_t model_Match = 0;
int8_t adcCounter = 0;
int16_t adcBytes = 0;
int16_t adcRaw = 0;

// #define ADC_THROTTLE_REVERSED false
uint32_t roll_min, roll_ctr, roll_max;
uint32_t pitch_min, pitch_ctr, pitch_max;
uint32_t thrtl_min, thrtl_ctr, thrtl_max;
uint32_t yaw_min, yaw_ctr, yaw_max;

uint32_t raw_roll, raw_pitch, raw_throttle, raw_yaw;
uint32_t rollMin_tmp, rollCtr_tmp, rollMax_tmp,
      pitchMin_tmp, pitchCtr_tmp, pitchMax_tmp,
      thrtlMin_tmp, thrtlMax_tmp, yawMin_tmp, yawCtr_tmp, yawMax_tmp;

bool roll_rev, pitch_rev, thrtl_rev, yaw_rev;
// crsf uses a reduced range, and BF expects to see it.
const static uint32_t MAX_OUT = 1811;
const static uint32_t MID_OUT =  992;
const static uint32_t MIN_OUT =  172;
bool calibrated = false;    //get true after config.Load() only
bool handsetArmed = false;

void endPointsCalibration(void)
{
    rollMin_tmp = rollMax_tmp = pitchMin_tmp = pitchMax_tmp = 
      thrtlMin_tmp = thrtlMax_tmp = yawMin_tmp = yawMax_tmp = 1900;
    // raw_roll = raw_pitch = raw_throttle = raw_yaw = 1800; commented due to new "adc_*" vars in getChannelValues()
    
    while(gpio_get_level(GPIO_NUM_21))  // set/right button
    {
        if (raw_roll < rollMin_tmp) rollMin_tmp = raw_roll;
        if (raw_roll > rollMax_tmp) rollMax_tmp = raw_roll;

        if (raw_pitch < pitchMin_tmp) pitchMin_tmp = raw_pitch;
        if (raw_pitch > pitchMax_tmp) pitchMax_tmp = raw_pitch;

        if (raw_throttle < thrtlMin_tmp) thrtlMin_tmp = raw_throttle;
        if (raw_throttle > thrtlMax_tmp) thrtlMax_tmp = raw_throttle;

        if (raw_yaw < yawMin_tmp) yawMin_tmp = raw_yaw;
        if (raw_yaw > yawMax_tmp) yawMax_tmp = raw_yaw;
    }
    // config.Load();   in config.cpp ->TxConfig::Commit() ->ENDPOINTS_CHANGED
}
void centrValsCalibration(void)
{
    while(gpio_get_level(GPIO_NUM_21))  // set/right button
    {
        rollCtr_tmp = raw_roll;
        pitchCtr_tmp = raw_pitch;
        yawCtr_tmp = raw_yaw;
    }
    // config.Load();   in config.cpp ->TxConfig::Commit() ->CENTRVALS_CHANGED
}

inline uint32_t scaleADCtoCRSF(uint16_t min, uint16_t centre, uint16_t max, 
                        uint16_t adcValue, bool reversed)
{
    if(calibrated)  //get true at the end of config.Load() only
    {
        roll_min = config.GetCalibratedValue(ROLL_MIN);
        roll_ctr = config.GetCalibratedValue(ROLL_CTR);
        roll_max = config.GetCalibratedValue(ROLL_MAX);

        pitch_min = config.GetCalibratedValue(PITCH_MIN);
        pitch_ctr = config.GetCalibratedValue(PITCH_CTR);
        pitch_max = config.GetCalibratedValue(PITCH_MAX);
        
        thrtl_min = config.GetCalibratedValue(THRTL_MIN);
        thrtl_max = config.GetCalibratedValue(THRTL_MAX);
        thrtl_ctr = (thrtl_min + thrtl_max) / 2;

        yaw_min = config.GetCalibratedValue(YAW_MIN);
        yaw_ctr = config.GetCalibratedValue(YAW_CTR);
        yaw_max = config.GetCalibratedValue(YAW_MAX);

        uint8_t reverse_gimbals = config.GetReverse_gmbls();  //GetCalibratedValue(REV_IND);
        
        roll_rev = reverse_gimbals & 0x1;
        pitch_rev = reverse_gimbals >> 1 & 0x1;
        thrtl_rev = reverse_gimbals >> 2 & 0x1;
        yaw_rev = reverse_gimbals >> 3 & 0x1;
        
        calibrated = false;
    }

   uint32_t crsfVal;
   if (adcValue <= min) crsfVal = MIN_OUT;

   else if (adcValue >= max) crsfVal = MAX_OUT;
    // high half scaling
   else if (adcValue > centre) crsfVal = MID_OUT + (adcValue - centre) * (MID_OUT - MIN_OUT) / (max - centre);
    // low half scaling
   else crsfVal = MID_OUT - ((centre - adcValue) * (MAX_OUT - MID_OUT) / (centre - min));

   if (reversed) crsfVal = MID_OUT * 2 - crsfVal;

   return crsfVal;
}

static inline uint32_t scaleYawData(uint16_t raw_adc_yaw) {
   uint32_t yaw = scaleADCtoCRSF(yaw_min, yaw_ctr, yaw_max, raw_adc_yaw, yaw_rev);
   return yaw;
}
static inline uint32_t scaleThrottleData(uint16_t raw_adc_throttle) {
   uint32_t throttle = scaleADCtoCRSF(thrtl_min, thrtl_ctr, thrtl_max, raw_adc_throttle, thrtl_rev);
   return throttle;
}
static inline uint32_t scalePitchData(uint16_t raw_adc_pitch) {
   uint32_t pitch = scaleADCtoCRSF(pitch_min, pitch_ctr, pitch_max, raw_adc_pitch, pitch_rev);
   return pitch;
}
static inline uint32_t scaleRollData(uint16_t raw_adc_roll) {
   uint32_t roll = scaleADCtoCRSF(roll_min, roll_ctr, roll_max, raw_adc_roll, roll_rev);
   return roll;
}

// Initialize continous DMA mode ADC1 Channels 0, 1, 3, 4
bool InitializeContinousADC()
{
  adc_digi_init_config_t adc_dma_config =
  {
      .max_store_buf_size = bufferSize,             //bufferSize,  TIMES*ADC_RESULT_BYTE
      .conv_num_each_intr = TIMES*ADC_RESULT_BYTE,  //can't explain why should multiply it but it works
      .adc1_chan_mask = adc1_chan_mask,
      .adc2_chan_mask = adc2_chan_mask,
  };

  esp_err_t ret = adc_digi_initialize(&adc_dma_config);
  if (ret != ESP_OK) return false;

  adc_digi_configuration_t dig_cfg =
  {
      .conv_limit_en = ADC_CONV_LIMIT_EN,   //should be only "0"?
      .conv_limit_num = ADC_CONV_LIMIT_EN,  //has no affect due to the previous
      .sample_freq_hz = 25000,  //20000->890uS SOC_ADC_SAMPLE_FREQ_THRES_HIGH INT16_MAX
      .conv_mode = ADC_CONV_MODE,
      .format = ADC_OUTPUT_TYPE,
  };
/*SOC_ADC_PATT_LEN_MAX (24) two pattern table, each contains 12 items. Each item takes 1 byte*/
  adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};

  dig_cfg.pattern_num = CHANNELS;
  for (int i = 0; i < CHANNELS; i++) {
        uint8_t unit = GET_UNIT(channel[i]);
        uint8_t ch = channel[i] & 0x7;
        adc_pattern[i].atten = ADC_ATTEN_DB_12;
        adc_pattern[i].channel = ch;
        adc_pattern[i].unit = unit;
        adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;   // (12)
  }

  dig_cfg.adc_pattern = adc_pattern;

  ret = adc_digi_controller_configure(&dig_cfg);
  if (ret != ESP_OK) return false;

  return true;
}
/********************************************************************************/
//@

#define ADC_READING_PERIOD_MS 1 //20

static volatile int analogReadings[ADC_MAX_DEVICES];

int getADCReading(adc_reading reading)
{
    return analogReadings[reading];
}

static int start()
{
//my@ #if defined(GPIO_PIN_JOYSTICK)
//     if (GPIO_PIN_JOYSTICK != UNDEF_PIN)
//     {
//         return DURATION_IMMEDIATELY;
//     }
// #endif
//     return DURATION_NEVER;
/*Two position switches or buttons, active is pulled down(GND)*/
/*right switches*/
    // gpio_reset_pin(GPIO_NUM_15);
    // gpio_set_direction(GPIO_NUM_15, GPIO_MODE_INPUT);
    // gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);
    // gpio_reset_pin(GPIO_NUM_16);
    // gpio_set_direction(GPIO_NUM_16, GPIO_MODE_INPUT);
    // gpio_set_pull_mode(GPIO_NUM_16, GPIO_PULLUP_ONLY);
    gpio_reset_pin(GPIO_NUM_17);
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_17, GPIO_PULLUP_ONLY);
/*left button*/
    gpio_reset_pin(GPIO_NUM_6);
    gpio_set_direction(GPIO_NUM_6, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_6, GPIO_PULLUP_ONLY);
/*right button*/
    gpio_reset_pin(GPIO_NUM_7);
    gpio_set_direction(GPIO_NUM_7, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_7, GPIO_PULLUP_ONLY);
/*left switches*/
    gpio_reset_pin(GPIO_NUM_0);
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
    gpio_reset_pin(GPIO_NUM_41);
    gpio_set_direction(GPIO_NUM_41, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_41, GPIO_PULLUP_ONLY);
    gpio_reset_pin(GPIO_NUM_42);
    gpio_set_direction(GPIO_NUM_42, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_42, GPIO_PULLUP_ONLY);
/*BUZZER/(DSView probe for test purpose)*/
    gpio_reset_pin(GPIO_NUM_45);
    gpio_set_direction(GPIO_NUM_45, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(GPIO_NUM_45, GPIO_FLOATING);
    gpio_set_level(GPIO_NUM_45, 0);  //BUZZER OFF
/*Hold power*/
    gpio_reset_pin(GPIO_NUM_48);
    gpio_set_direction(GPIO_NUM_48, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(GPIO_NUM_48, GPIO_FLOATING);
    gpio_set_level(GPIO_NUM_48, 0);
/*Power button*/
    gpio_reset_pin(GPIO_NUM_47);
    gpio_set_direction(GPIO_NUM_47, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_47, GPIO_PULLUP_ONLY);

    adcStatus = InitializeContinousADC();
    return DURATION_IMMEDIATELY;
}
//my@
static int getChannelValues()   //should takes within 1mS
{
/*DSView time chart in conjunction with RxEN(which starts pattern)
 and TxEN(to make sure that adc conversion occurs while transmitting):*/
    // gpio_set_level(GPIO_NUM_45, 1);  //for DSView conversion start

    uint8_t counter = 0;

//  uint32_t raw_slider;        //raw_sw_b, raw_sw_c;
    //theese vars solely to separate raw_* values and get proper calibration
    uint32_t adc_roll, adc_pitch, adc_throttle, adc_yaw, adc_slider;
    adc_roll = adc_pitch = adc_throttle = adc_yaw = adc_slider = 0;

    // Reset values in buffer
    memset(analogReadBuffer, 0x7F, bufferSize);
    // Start ADC
    adc_digi_start();
    // Read ADC values into buffer
    uint32_t numBytesRead = 0;
    // Period of packet rate 500Hz and F500(has no advantage) is 2mS therefore the timeout is the same
    adc_digi_read_bytes(analogReadBuffer, bufferSize, &numBytesRead, 2);     //ADC_MAX_DELAY
    // Stop ADC
    adc_digi_stop();
    
    // gpio_set_level(GPIO_NUM_45, 0);  //for DSView conversion stop

    // uint32_t adc_pitchMax = 0;
    // uint32_t adc_pitchMin = 4096;
    // uint32_t adc_pitchSum = 0;

    /*.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/include/hal/include/hal/adc_types.h*/
    for (uint16_t i = 0; i < numBytesRead; i+=ADC_RESULT_BYTE)
    {
        adc_digi_output_data_t* pData = (adc_digi_output_data_t*)&analogReadBuffer[i];
        switch(pData->type2.channel)
        {
            case ADC1_CHANNEL_0:
                adc_throttle += pData->type2.data;
                break;
            case ADC1_CHANNEL_1:
                adc_yaw += pData->type2.data;
                break;
            case ADC1_CHANNEL_3:
                adc_roll += pData->type2.data;
                break;
            case ADC1_CHANNEL_4:
                adc_pitch += pData->type2.data;
                // adc_pitch = pData->type2.data;
                // adc_pitchSum += adc_pitch;
                ++counter;  //it may drop by 1, that is why
                break;
            // case ADC1_CHANNEL_5:
                // adc_slider += pData->type2.data;
                // ++counter;  //it may drop by 1, that is why
                // break;
            default:
                adcNoData = true;
                break;
        }
    }
    // gpio_set_level(GPIO_NUM_45, 1);     //for DSView
    //raw values used for calibration also
    raw_roll = adc_roll/SAMPLES;
    raw_pitch = adc_pitch/counter;    //(adc_pitchSum - adc_pitchMax - adc_pitchMin)/2;  
    raw_throttle = adc_throttle/SAMPLES;
    raw_yaw = adc_yaw/SAMPLES;
//  raw_slider = adc_slider;

    if(adcNoData) {
        ChannelData[0] = 992;
        ChannelData[1] = 992;
        ChannelData[2] = 172;
        ChannelData[3] = 992;
    }
    else {
        ChannelData[0] = scaleRollData(raw_roll);
        ChannelData[1] = scalePitchData(raw_pitch);
        ChannelData[2] = scaleThrottleData(raw_throttle);
        ChannelData[3] = scaleYawData(raw_yaw);
    //  ChannelData[8] = raw_slider;
    }

/*arm switch*/
    if(gpio_get_level(GPIO_NUM_0)) //change status when not armed
    {
        ((ChannelData[2] > 220) || (connectionState != connected) || !connectionHasModelMatch)? permitArming = false: permitArming = true;
    }
    (!gpio_get_level(GPIO_NUM_0) && permitArming)? handsetArmed = true: handsetArmed = false;
    handset->SetArmed(handsetArmed);
    // gpio_get_level(GPIO_NUM_0)? ChannelData[4] = 172: ChannelData[4] = 1812;
/*right 2-pos switch*/
    gpio_get_level(GPIO_NUM_17)? ChannelData[5] = 172: ChannelData[5] = 1812;

/*left 3-pos switches*/
    if(!gpio_get_level(GPIO_NUM_41))        { ChannelData[6] = 172; }
    else if(!gpio_get_level(GPIO_NUM_42))   { ChannelData[6] = 1812;}
    else ChannelData[6] = 992;
/*right 3-pos switches*/
    // if(!gpio_get_level(GPIO_NUM_15))        { ChannelData[7] = 172; }
    // else if(!gpio_get_level(GPIO_NUM_16))   { ChannelData[7] = 1812;}
    // else ChannelData[7] = 992;

/*left button*/
    gpio_get_level(GPIO_NUM_6)? ChannelData[8] = 172: ChannelData[8] = 1812;
/*right button*/
    gpio_get_level(GPIO_NUM_7)? ChannelData[9] = 172: ChannelData[9] = 1812;
/*power button*/
    gpio_get_level(GPIO_NUM_47)? ChannelData[11] = 172: ChannelData[11] = 1812;

/*fake values on telemetry screen*/
    adcCounter = counter;
    adcRaw = ChannelData[1];
    // gpio_set_level(GPIO_NUM_45, 0);     //for DSView
    // ChannelData[8] = ChannelData[10] = 172;
    return numBytesRead;    //permitArming;
}
//@
static int timeout()
{
    extern volatile bool busyTransmitting;
    static bool fullWait = true;

    // if called because of a full-timeout and the main loop is transmitting then we will
    // leave the fullWait flag true and return with an immediate timeout so we can wait for
    // the main loop to finish transmitting, which will pop us into the next state.
    if (fullWait && busyTransmitting && connectionState < MODE_STATES) return DURATION_IMMEDIATELY;
    fullWait = false;

    // If the main loop is NOT transmitting then return with an immediate timeout until it transitions
    // to transmitting
    if (!busyTransmitting && connectionState < MODE_STATES) return DURATION_IMMEDIATELY;

    // If we reach this point we are assured that the main loop has just transitioned from
    // not transmitting to transmitting, so it's safe to read the ADC
//my@ #if defined(GPIO_PIN_JOYSTICK)
//     if (GPIO_PIN_JOYSTICK != UNDEF_PIN)
//     {
//         analogReadings[ADC_JOYSTICK] = analogRead(GPIO_PIN_JOYSTICK);
//     }
// #endif
// #if defined(GPIO_PIN_PA_PDET)
//     if (GPIO_PIN_PA_PDET != UNDEF_PIN)
//     {
//         analogReadings[ADC_PA_PDET] = analogReadMilliVolts(GPIO_PIN_PA_PDET);
//     }
// #endif

    if(adcStatus) adcBytes = getChannelValues();    //my@

    fullWait = true;
    return ADC_READING_PERIOD_MS;
}

device_t ADC_device = {
    .initialize = nullptr,
    .start = start,
    .event = nullptr,
    .timeout = timeout,
};
//my@ #endif
