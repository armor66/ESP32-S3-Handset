#pragma once

#include "targets.h"
#include "elrs_eeprom.h"
#include "options.h"
#include "common.h"

// #if defined(PLATFORM_ESP32)
#include <nvs_flash.h>
#include <nvs.h>
// #endif

// CONFIG_MAGIC is ORed with CONFIG_VERSION in the version field
#define CONFIG_MAGIC_MASK   (0b11U << 30)
#define TX_CONFIG_MAGIC     (0b01U << 30)
#define RX_CONFIG_MAGIC     (0b10U << 30)

#define TX_CONFIG_VERSION   8U
#define RX_CONFIG_VERSION   11U

//my@ #if defined(TARGET_TX)

#define CONFIG_TX_BUTTON_ACTION_CNT 2
#define CONFIG_TX_MODEL_CNT         64

typedef enum {
    HT_OFF,
    HT_ON,
    HT_AUX1_UP,
    HT_AUX1_DN,
    HT_AUX2_UP,
    HT_AUX2_DN,
    HT_AUX3_UP,
    HT_AUX3_DN,
    HT_AUX4_UP,
    HT_AUX4_DN,
    HT_AUX5_UP,
    HT_AUX5_DN,
    HT_AUX6_UP,
    HT_AUX6_DN,
    HT_AUX7_UP,
    HT_AUX7_DN,
    HT_AUX8_UP,
    HT_AUX8_DN,
} headTrackingEnable_t;

typedef enum {
    HT_START_EDGETX,
    HT_START_AUX1,
    HT_START_AUX2,
    HT_START_AUX3,
    HT_START_AUX4,
    HT_START_AUX5,
    HT_START_AUX6,
    HT_START_AUX7,
    HT_START_AUX8,
    HT_START_AUX9,
    HT_START_AUX10,
} headTrackingStart_t;

typedef struct {
    uint32_t    rate:5,
                tlm:4,
                power:3,
                switchMode:2,
                boostChannel:3, // dynamic power boost AUX channel
                dynamicPower:1,
                modelMatch:1,
                txAntenna:2,    // FUTURE: Which TX antenna to use, 0=Auto
                ptrStartChannel:4,
                ptrEnableChannel:5,
                linkMode:2;
} model_config_t;

typedef struct {
    uint8_t     pressType:1,    // 0 short, 1 long
                count:3,        // 1-8 click count for short, .5sec hold count for long
                action:4;       // action to execute
} button_action_t;

typedef union {
    struct {
        uint8_t color;                  // RRRGGGBB
        button_action_t actions[CONFIG_TX_BUTTON_ACTION_CNT];
        uint8_t unused;
    } val;
    uint32_t raw;
} tx_button_color_t;

typedef enum {
    BACKPACK_TELEM_MODE_OFF,
    BACKPACK_TELEM_MODE_ESPNOW,
    BACKPACK_TELEM_MODE_WIFI,
    BACKPACK_TELEM_MODE_BLUETOOTH,
} telem_mode_t;

typedef struct {
    uint32_t        version;
    uint8_t         vtxBand;    // 0=Off, else band number
    uint8_t         vtxChannel; // 0=Ch1 -> 7=Ch8
    uint8_t         vtxPower;   // 0=Do not set, else power number
    uint8_t         vtxPitmode; // Off/On/AUX1^/AUX1v/etc
    uint8_t         powerFanThreshold:4; // Power level to enable fan if present
    model_config_t  model_config[CONFIG_TX_MODEL_CNT];
    uint8_t         fanMode;            // some value used by thermal?
    uint8_t         motionMode:2,       // bool, but space for 2 more modes
                    dvrStopDelay:3,
                    backpackDisable:1,  // bool, disable backpack via EN pin if available
                    backpackTlmMode:2;  // 0=Off, 1=Fwd tlm via espnow, 2=fwd tlm via wifi 3=(FUTURE) bluetooth
    uint8_t         dvrStartDelay:3,
                    dvrAux:5;
    tx_button_color_t buttonColors[2];  // FUTURE: TX RGB color / mode (sets color of TX, can be a static color or standard)
                                        // FUTURE: Model RGB color / mode (sets LED color mode on the model, but can be second TX led color too)
                                        // FUTURE: Custom button actions
    // extreme values
    uint16_t        roll_Min;           //"roll_M"
    uint16_t        roll_Max;
    uint16_t        pitchMin;           //"pitchM"
    uint16_t        pitchMax;
    uint16_t        thrtlMin;           //"thrtlM"
    uint16_t        thrtlMax;
    uint16_t        yaw__Min;           //"yaw__M"
    uint16_t        yaw__Max:     12,   //LSB?
                    calEnd_points: 4;   //just index to know if calibration occures
    // central values
    uint16_t        roll_Ctr;           //"roll_pitch"
    uint16_t        pitchCtr;
    uint16_t        yaw__Ctr;           //"yawCtr"
    uint8_t         reverse_gmbls: 4,   //LSB?
                    // roll_Rev: 1,
                    // pitchRev: 1,
                    // thrtlRev: 1,
                    // yaw__Rev: 1,
                    calCentr_vals: 4;   //just index to know if calibration occures
    // value with 0x3F mask
    uint8_t         modelMatch_VL: 6,   //LSB?
                    modelMatch_ST: 1;   //MSB?
} tx_config_t;

class TxConfig
{
public:
    TxConfig();
    void Load();
    uint32_t Commit();

    // Getters
    uint8_t GetRate() const { return m_model->rate; }
    uint8_t GetTlm() const { return m_model->tlm; }
    uint8_t GetPower() const { return m_model->power; }
    bool GetDynamicPower() const { return m_model->dynamicPower; }
    uint8_t GetCalEnd_points() const { return m_config.calEnd_points; } //my@
    uint8_t GetCalCenrt_vals() const { return m_config.calCentr_vals; } //my@
    uint8_t GetReverse_gmbls() const { return m_config.reverse_gmbls; } //my@
    uint8_t GetModelMatch_VL() const { return m_config.modelMatch_VL; } //my@
    bool GetModelMatch_ST() const { return m_config.modelMatch_ST; }    //my@
    uint8_t GetBoostChannel() const { return m_model->boostChannel; }
    uint8_t GetSwitchMode() const { return m_model->switchMode; }
    uint8_t GetAntennaMode() const { return m_model->txAntenna; }
    uint8_t GetLinkMode() const { return m_model->linkMode; }
    bool GetModelMatch() const { return m_model->modelMatch; }
    bool     IsModified() const { return m_modified != 0; }
    uint8_t  GetVtxBand() const { return m_config.vtxBand; }
    uint8_t  GetVtxChannel() const { return m_config.vtxChannel; }
    uint8_t  GetVtxPower() const { return m_config.vtxPower; }
    uint8_t  GetVtxPitmode() const { return m_config.vtxPitmode; }
    uint8_t GetPowerFanThreshold() const { return m_config.powerFanThreshold; }
    uint8_t  GetFanMode() const { return m_config.fanMode; }
    uint8_t  GetMotionMode() const { return m_config.motionMode; }
    uint8_t  GetDvrAux() const { return m_config.dvrAux; }
    uint8_t  GetDvrStartDelay() const { return m_config.dvrStartDelay; }
    uint8_t  GetDvrStopDelay() const { return m_config.dvrStopDelay; }
    bool     GetBackpackDisable() const { return m_config.backpackDisable; }
    uint8_t  GetBackpackTlmMode() const { return m_config.backpackTlmMode; }
    tx_button_color_t const *GetButtonActions(uint8_t button) const { return &m_config.buttonColors[button]; }
    model_config_t const &GetModelConfig(uint8_t model) const { return m_config.model_config[model]; }
    uint8_t GetPTRStartChannel() const { return m_model->ptrStartChannel; }
    uint8_t GetPTREnableChannel() const { return m_model->ptrEnableChannel; }

    uint32_t GetCalibratedValue(uint8_t itemIndex);   //my@

    // Setters
    void SetRate(uint8_t rate);
    void SetTlm(uint8_t tlm);
    void SetPower(uint8_t power);
    void SetDynamicPower(bool dynamicPower);
    void SetCalEnd_points(uint8_t calEnd_points);   //my@
    void SetCalCenrt_vals(uint8_t calCentr_vals);   //my@
    void SetReverse_gmbls(uint8_t reverse_gmbls);   //my@
    void SetModelMatch_VL(uint8_t modelMatch_VL);   //my@
    void SetModelMatch_ST(bool modelMatch_ST);      //my@
    void SetBoostChannel(uint8_t boostChannel);
    void SetSwitchMode(uint8_t switchMode);
    void SetAntennaMode(uint8_t txAntenna);
    void SetLinkMode(uint8_t linkMode);
    void SetModelMatch(bool modelMatch);
    void SetDefaults(bool commit);
    void SetStorageProvider(ELRS_EEPROM *eeprom);
    void SetVtxBand(uint8_t vtxBand);
    void SetVtxChannel(uint8_t vtxChannel);
    void SetVtxPower(uint8_t vtxPower);
    void SetVtxPitmode(uint8_t vtxPitmode);
    void SetPowerFanThreshold(uint8_t powerFanThreshold);
    void SetFanMode(uint8_t fanMode);
    void SetMotionMode(uint8_t motionMode);
    void SetDvrAux(uint8_t dvrAux);
    void SetDvrStartDelay(uint8_t dvrStartDelay);
    void SetDvrStopDelay(uint8_t dvrStopDelay);
    void SetButtonActions(uint8_t button, tx_button_color_t actions[2]);
    void SetBackpackDisable(bool backpackDisable);
    void SetBackpackTlmMode(uint8_t mode);
    void SetPTRStartChannel(uint8_t ptrStartChannel);
    void SetPTREnableChannel(uint8_t ptrEnableChannel);

    // State setters
    bool SetModelId(uint8_t modelId);

private:
// #if !defined(PLATFORM_ESP32)
//     void UpgradeEepromV5ToV6();
//     void UpgradeEepromV6ToV7();
//     void UpgradeEepromV7ToV8();
// #endif

    tx_config_t m_config;
    ELRS_EEPROM *m_eeprom;
    uint32_t     m_modified;
    model_config_t *m_model;
    uint8_t     m_modelId;
// #if defined(PLATFORM_ESP32)
    // nvs_handle  handle;
    nvs_handle_t  handle;
// #endif
};

extern TxConfig config;

//my@ #endif
typedef enum
{
    ROLL_MIN,
    ROLL_MAX,
    PITCH_MIN,
    PITCH_MAX,
    THRTL_MIN,
    THRTL_MAX,
    YAW_MIN,
    YAW_MAX,
    ROLL_CTR,
    PITCH_CTR,
    YAW_CTR,
    REV_IND
} calibrated_item_index_t;
