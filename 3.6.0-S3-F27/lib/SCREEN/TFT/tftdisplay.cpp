//my@ #ifdef HAS_TFT_SCREEN

#include <Arduino_GFX_Library.h>
#include "Pragma_Sans36pt7b.h"
#include "Pragma_Sans37pt7b.h"
#include "Pragma_Sans314pt7b.h"

#include "tftdisplay.h"

#include "logos.h"
#include "options.h"
#include "logging.h"
#include "common.h"
#include "CRSF.h"
//my@
#include "devAnalogVbat.h"
#include "devADC.h"
#include "config.h"

#include "WiFi.h"
extern WiFiMode_t wifiMode;

/*matches with display.cpp -> *Display::main_menu_strings[][2],
  which colored here in TFTDisplay::displayMainMenu */
const uint16_t *main_menu_icons[] = {
    elrs_rate,
    elrs_switch,
    elrs_antenna,
    elrs_power,
    elrs_ratio,
    elrs_motion,
    elrs_fan,
    elrs_joystick,
    elrs_bind,
    elrs_wifimode,
    elrs_vtx,

    elrs_power,
    elrs_power,

    elrs_vtx,
    elrs_vtx,
    elrs_vtx,
    elrs_vtx,
    elrs_vtx,

    elrs_updatefw,
    elrs_rxwifi,
    elrs_backpack,
    elrs_vrxwifi,

    elrs_joystick,
    elrs_fan,
    elrs_fan,
    elrs_fan,
    elrs_motion,
    elrs_motion,
    elrs_motion,
};

// Hex color code to 16-bit rgb:
// color = 0x96c76f
// rgb_hex = ((((color&0xFF0000)>>16)&0xf8)<<8) + ((((color&0x00FF00)>>8)&0xfc)<<3) + ((color&0x0000FF)>>3)
constexpr uint16_t elrs_banner_bgColor[] = {
    0x4315, // MSG_NONE          => #4361AA (ELRS blue)
    0x9E2D, // MSG_CONNECTED     => #9FC76F (ELRS green)
    0xAA08, // MSG_ARMED         => #AA4343 (red)
    0xF501, // MSG_MISMATCH      => #F0A30A (amber)
    0xF800  // MSG_ERROR         => #F0A30A (very red)
};

#define SCREEN_X    160
#define SCREEN_Y    95

#define SCREEN_SMALL_FONT_SIZE      8
#define SCREEN_SMALL_FONT           Pragma_Sans36pt7b

#define SCREEN_NORMAL_FONT_SIZE     16
#define SCREEN_NORMAL_FONT          Pragma_Sans37pt7b

#define SCREEN_LARGE_FONT_SIZE      26
#define SCREEN_LARGE_FONT           Pragma_Sans314pt7b

//ICON SIZE Definition
#define SCREEN_LARGE_ICON_SIZE      60

//GAP Definition
#define SCREEN_CONTENT_GAP          10
#define SCREEN_FONT_GAP             5

//INIT LOGO PAGE Definition
#define INIT_PAGE_LOGO_X            SCREEN_X
#define INIT_PAGE_LOGO_Y            53
#define INIT_PAGE_FONT_PADDING      3
#define INIT_PAGE_FONT_START_X      SCREEN_FONT_GAP
#define INIT_PAGE_FONT_START_Y      INIT_PAGE_LOGO_Y + (SCREEN_Y - INIT_PAGE_LOGO_Y - SCREEN_NORMAL_FONT_SIZE)/2

//IDLE PAGE Definition
#define IDLE_PAGE_START_X   SCREEN_CONTENT_GAP
#define IDLE_PAGE_START_Y   2

#define IDLE_PAGE_STAT_START_X  SCREEN_X/2 // is centered, so no GAP
#define IDLE_PAGE_STAT_Y_GAP    0   //(SCREEN_Y - SCREEN_NORMAL_FONT_SIZE * 3)/4

#define IDLE_PAGE_RATE_START_Y  IDLE_PAGE_START_Y + IDLE_PAGE_STAT_Y_GAP
#define IDLE_PAGE_RATIO_START_Y IDLE_PAGE_RATE_START_Y + SCREEN_LARGE_FONT_SIZE + IDLE_PAGE_STAT_Y_GAP
#define IDLE_PAGE_POWER_START_Y IDLE_PAGE_RATIO_START_Y + SCREEN_NORMAL_FONT_SIZE + IDLE_PAGE_STAT_Y_GAP
#define IDLE_PAGE_STATE_START_Y IDLE_PAGE_POWER_START_Y + SCREEN_NORMAL_FONT_SIZE + IDLE_PAGE_STAT_Y_GAP + 1   //my@
#define IDLE_PAGE_STATE1_START_Y IDLE_PAGE_STATE_START_Y + SCREEN_NORMAL_FONT_SIZE - 1  //my@

//MAIN PAGE Definition
#define MAIN_PAGE_ICON_START_X  SCREEN_CONTENT_GAP
#define MAIN_PAGE_ICON_START_Y  (SCREEN_Y -  SCREEN_LARGE_ICON_SIZE)/2

#define MAIN_PAGE_WORD_START_X  MAIN_PAGE_ICON_START_X + SCREEN_LARGE_ICON_SIZE
#define MAIN_PAGE_WORD_START_Y1 (SCREEN_Y -  SCREEN_NORMAL_FONT_SIZE*2 - SCREEN_FONT_GAP)/2
#define MAIN_PAGE_WORD_START_Y2 MAIN_PAGE_WORD_START_Y1 + SCREEN_NORMAL_FONT_SIZE + SCREEN_FONT_GAP

//Sub Function Definiton
#define SUB_PAGE_VALUE_START_X  SCREEN_CONTENT_GAP
#define SUB_PAGE_VALUE_START_Y  (SCREEN_Y - SCREEN_LARGE_FONT_SIZE - SCREEN_NORMAL_FONT_SIZE - SCREEN_CONTENT_GAP)/2

#define SUB_PAGE_TIPS_START_X  SCREEN_CONTENT_GAP
#define SUB_PAGE_TIPS_START_Y  SCREEN_Y - SCREEN_NORMAL_FONT_SIZE - SCREEN_CONTENT_GAP

//Sub WIFI Mode & Bind Confirm Definiton
#define SUB_PAGE_ICON_START_X  0
#define SUB_PAGE_ICON_START_Y  (SCREEN_Y -  SCREEN_LARGE_ICON_SIZE)/2

#define SUB_PAGE_WORD_START_X   SUB_PAGE_ICON_START_X + SCREEN_LARGE_ICON_SIZE
#define SUB_PAGE_WORD_START_Y1  (SCREEN_Y -  SCREEN_NORMAL_FONT_SIZE*3 - SCREEN_FONT_GAP*2)/2
#define SUB_PAGE_WORD_START_Y2  SUB_PAGE_WORD_START_Y1 + SCREEN_NORMAL_FONT_SIZE + SCREEN_FONT_GAP
#define SUB_PAGE_WORD_START_Y3  SUB_PAGE_WORD_START_Y2 + SCREEN_NORMAL_FONT_SIZE + SCREEN_FONT_GAP

//Sub Binding Definiton
#define SUB_PAGE_BINDING_WORD_START_X   0
#define SUB_PAGE_BINDING_WORD_START_Y   (SCREEN_Y -  SCREEN_LARGE_FONT_SIZE)/2

static Arduino_DataBus *bus;
static Arduino_GFX *gfx;

uint8_t BKGRND = BLACK;

void TFTDisplay::init()
{
    if (GPIO_PIN_TFT_BL != UNDEF_PIN)
    {
        pinMode(GPIO_PIN_TFT_BL, OUTPUT);
    }
    bus = new Arduino_ESP32SPI(GPIO_PIN_TFT_DC, GPIO_PIN_TFT_CS, GPIO_PIN_TFT_SCLK, GPIO_PIN_TFT_MOSI, GFX_NOT_DEFINED, HSPI);
    // gfx = new Arduino_ST7735(bus, GPIO_PIN_TFT_RST, 3, false , 80, 160, 26, 1, 26, 1);
    gfx = new Arduino_ST7735(bus, GPIO_PIN_TFT_RST, OPT_OLED_REVERSED ? 3 : 1 /* rotation */, false , 95, 160, 19, 1, 19, 1);

    gfx->begin();
    doScreenBackLight(SCREEN_BACKLIGHT_ON);
}

void TFTDisplay::doScreenBackLight(screen_backlight_t state)
{
    if (GPIO_PIN_TFT_BL != UNDEF_PIN)
    {
        digitalWrite(GPIO_PIN_TFT_BL, state);
    }
}

void TFTDisplay::printScreenshot()
{
    DBGLN("Unimplemented");
}

static void displayFontCenter(uint32_t font_start_x, uint32_t font_end_x, uint32_t font_start_y,
                                            int font_size, const GFXfont& font, String font_string,
                                            uint16_t fgColor, uint16_t bgColor)
{
    gfx->fillRect(font_start_x, font_start_y, font_end_x - font_start_x, font_size, bgColor);
    gfx->setFont(&font);

    int16_t x, y;
    uint16_t w, h;
    gfx->getTextBounds(font_string, font_start_x, font_start_y, &x, &y, &w, &h);
    int start_pos = font_start_x + (font_end_x - font_start_x - w)/2;

    gfx->setCursor(start_pos, font_start_y + h);
    gfx->setTextColor(fgColor, bgColor);
    gfx->print(font_string);
}


void TFTDisplay::displaySplashScreen()
{
    gfx->fillScreen(WHITE);

    size_t sz = INIT_PAGE_LOGO_X * INIT_PAGE_LOGO_Y;
    uint16_t image[sz];
    if (spi_flash_read(logo_image, image, sz * 2) == ESP_OK)
    {
        gfx->draw16bitRGBBitmap(0, 0, image, INIT_PAGE_LOGO_X, INIT_PAGE_LOGO_Y);
    }

    gfx->fillRect(SCREEN_FONT_GAP, INIT_PAGE_FONT_START_Y - INIT_PAGE_FONT_PADDING,
                    SCREEN_X - SCREEN_FONT_GAP*2, SCREEN_NORMAL_FONT_SIZE + INIT_PAGE_FONT_PADDING*2, RED);

    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%s  ELRS-%.6s", HARDWARE_VERSION, version);
    displayFontCenter(INIT_PAGE_FONT_START_X, SCREEN_X - INIT_PAGE_FONT_START_X, INIT_PAGE_FONT_START_Y + 1,
                        SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        String(buffer), CYAN, RED); //YELLOW/BLUE
}

void TFTDisplay::displayIdleScreen(uint8_t changed, uint8_t rate_index, uint8_t power_index, uint8_t ratio_index, uint8_t motion_index, uint8_t fan_index, bool dynamic, uint8_t running_power_index, uint8_t temperature, message_index_t message_index)
{
    if (changed == CHANGED_ALL)
    {
        gpio_set_level(GPIO_NUM_45, 1);
        // Everything has changed! So clear the right side
        gfx->fillRect(SCREEN_X/2, 0, SCREEN_X/2, SCREEN_Y, BKGRND);
    }

    if (changed & CHANGED_TEMP)
    {
        // Left side logo, version, and temp
        gfx->fillRect(0, 0, SCREEN_X/2, SCREEN_Y, elrs_banner_bgColor[message_index]);
        gfx->drawBitmap(IDLE_PAGE_START_X, IDLE_PAGE_START_Y, elrs_banner_bmp, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE,
                        BKGRND);

        // Update the temperature
        char buffer[20];
        // \367 = (char)247 = degree symbol
        // snprintf(buffer, sizeof(buffer), "%d,%02dV %02d\367C", handset_vbat/100, handset_vbat%100, temperature);   //"%.6s %02d\367C"version, temperature
        snprintf(buffer, sizeof(buffer), "%d,%02dV", handset_vbat/100, handset_vbat%100);
        displayFontCenter(0, SCREEN_X/2, SCREEN_LARGE_ICON_SIZE + (SCREEN_Y - SCREEN_LARGE_ICON_SIZE - SCREEN_LARGE_FONT_SIZE)/2,
                         SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT, String(buffer), BKGRND, elrs_banner_bgColor[message_index]);
    }

    // The Radio Params right half of the screen
    uint16_t text_color = (message_index == MSG_ARMED) ? YELLOW : GREEN;    //DARKGREY : BLACK;

    if (connectionState == radioFailed)
    {
        displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
            "BAD", BLACK, BKGRND);
        displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
            "RADIO", BLACK, BKGRND);
    }
    //my@ else if (connectionState == noCrossfire)
    // {
    //     displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
    //         "NO", BLACK, BKGRND);
    //     displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
    //         "HANDSET", BLACK, BKGRND);
    // }
    else
    {
        if (changed & CHANGED_RATE)
        {
            displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, IDLE_PAGE_RATE_START_Y,  SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT,
                                getValue(STATE_PACKET, rate_index), text_color, BKGRND);
        }

        if (changed & CHANGED_TELEMETRY)
        {
            displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, IDLE_PAGE_RATIO_START_Y,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                                getValue(STATE_TELEMETRY_CURR, ratio_index), text_color, BKGRND);
        }

        if (changed & CHANGED_POWER)
        {
            String power = getValue(STATE_POWER, running_power_index);
            if (dynamic || power_index != running_power_index)
            {
                power += " *";
            }
            displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, IDLE_PAGE_POWER_START_Y, SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                                power, text_color, BKGRND);
        }

        //my@:
        char buffer[10];
        char buffer1[10];
        if(config.GetModelMatch())
        {
            snprintf(buffer, sizeof(buffer), "MODEL %d", config.GetModelMatch_VL());
            snprintf(buffer1, sizeof(buffer1), "matched");
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "MODEL");
            snprintf(buffer1, sizeof(buffer1), "NOT SET");
        }
        displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, IDLE_PAGE_STATE_START_Y,  SCREEN_NORMAL_FONT_SIZE,
                SCREEN_NORMAL_FONT, String(buffer), text_color, BKGRND);

        if(connectionHasModelMatch)
        {
            displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, IDLE_PAGE_STATE1_START_Y,  SCREEN_NORMAL_FONT_SIZE,
                SCREEN_NORMAL_FONT, String(buffer1), CYAN, BKGRND); //yellow actually
        }
        else
        {
            snprintf(buffer1, sizeof(buffer1), "mismatch");
            displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, IDLE_PAGE_STATE1_START_Y,  SCREEN_NORMAL_FONT_SIZE,
                SCREEN_NORMAL_FONT, String(buffer1), BLUE, BKGRND);  //red actually
        }
    }
}

//menu order defined in SCREEN/menu.cpp/fsm_state_entry_t const main_menu_fsm
void TFTDisplay::displayMainMenu(menu_item_t menu)
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(MAIN_PAGE_ICON_START_X, MAIN_PAGE_ICON_START_Y, main_menu_icons[menu], SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    displayFontCenter(MAIN_PAGE_WORD_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
        main_menu_strings[menu][0], GREEN, BKGRND);
    displayFontCenter(MAIN_PAGE_WORD_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
        main_menu_strings[menu][1], YELLOW, BKGRND);
}

void TFTDisplay::displayValue(menu_item_t menu, uint8_t value_index)
{
    gfx->fillScreen(BKGRND);

    String val = String(getValue(menu, value_index));
    val.replace("!+", "\xA0");
    val.replace("!-", "\xA1");

    displayFontCenter(SUB_PAGE_VALUE_START_X, SCREEN_X, SUB_PAGE_VALUE_START_Y,  SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT,
                        val.c_str(), GREEN, BKGRND);
    displayFontCenter(SUB_PAGE_TIPS_START_X, SCREEN_X, SUB_PAGE_TIPS_START_Y,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "PRESS TO CONFIRM", YELLOW, BKGRND);
}

void TFTDisplay::displayBLEConfirm()
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(SUB_PAGE_ICON_START_X, SUB_PAGE_ICON_START_Y, elrs_joystick, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "PRESS TO", GREEN, BKGRND);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "START BLE", YELLOW, BKGRND);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "GAMEPAD", YELLOW, BKGRND);
}

void TFTDisplay::displayBLEStatus()
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(SUB_PAGE_ICON_START_X, SUB_PAGE_ICON_START_Y, elrs_joystick, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "BLE", GREEN, BKGRND);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "GAMEPAD", YELLOW, BKGRND);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "RUNNING", YELLOW, BKGRND);
}

void TFTDisplay::displayWiFiConfirm()
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(SUB_PAGE_ICON_START_X, SUB_PAGE_ICON_START_Y, elrs_wifimode, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "PRESS TO", GREEN, BKGRND);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "ENTER WIFI", YELLOW, BKGRND);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "UPDATE MODE", YELLOW, BKGRND);
}

void TFTDisplay::displayWiFiStatus()
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(SUB_PAGE_ICON_START_X, SUB_PAGE_ICON_START_Y, elrs_wifimode, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    if (wifiMode == WIFI_STA) {
        String host_msg = String(wifi_hostname) + ".local";
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            "open http://", GREEN, BKGRND);
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            host_msg, YELLOW, BKGRND);
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            "by browser", YELLOW, BKGRND);
    }
    else
    {
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            wifi_ap_ssid, GREEN, BKGRND);
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            wifi_ap_password, YELLOW, BKGRND);
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            wifi_ap_address, YELLOW, BKGRND);
    }
}

void TFTDisplay::displayBindConfirm()
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(SUB_PAGE_ICON_START_X, SUB_PAGE_ICON_START_Y, elrs_bind, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "PRESS TO", GREEN, BKGRND);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "SEND BIND", YELLOW, BKGRND);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "REQUEST", YELLOW, BKGRND);
}

void TFTDisplay::displayBindStatus()
{
    gfx->fillScreen(BKGRND);

    displayFontCenter(SUB_PAGE_BINDING_WORD_START_X, SCREEN_X, SUB_PAGE_BINDING_WORD_START_Y,  SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT,
                        "BINDING...", RED, BKGRND);
}

void TFTDisplay::displayRunning()
{
    gfx->fillScreen(BKGRND);

    displayFontCenter(SUB_PAGE_BINDING_WORD_START_X, SCREEN_X, SUB_PAGE_BINDING_WORD_START_Y,  SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT,
                        "RUNNING...", YELLOW, BKGRND);
}

void TFTDisplay::displaySending()
{
    gfx->fillScreen(BKGRND);

    displayFontCenter(SUB_PAGE_BINDING_WORD_START_X, SCREEN_X, SUB_PAGE_BINDING_WORD_START_Y,  SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT,
                        "SENDING...", YELLOW, BKGRND);
}

void TFTDisplay::displayLinkstats()
{
    constexpr int16_t LINKSTATS_COL_FIRST   = 5;    //0
    constexpr int16_t LINKSTATS_COL_SECOND  = 45;   //30
    constexpr int16_t LINKSTATS_COL_THIRD   = 110;  //100

    constexpr int16_t LINKSTATS_ROW_FIRST   = 14;
    constexpr int16_t LINKSTATS_ROW_SECOND  = 29;
    constexpr int16_t LINKSTATS_ROW_THIRD   = 44;
    constexpr int16_t LINKSTATS_ROW_FOURTH  = 59;
    constexpr int16_t LINKSTATS_ROW_FIFTH   = 74;
    constexpr int16_t LINKSTATS_ROW_SIXTH   = 89;

    gfx->fillScreen(BKGRND);
    gfx->setFont(&SCREEN_NORMAL_FONT);
    gfx->setTextColor(YELLOW,BKGRND);

    gfx->setCursor(LINKSTATS_COL_FIRST, LINKSTATS_ROW_SECOND);
    gfx->print("LQ");
    gfx->setCursor(LINKSTATS_COL_FIRST, LINKSTATS_ROW_THIRD);
    gfx->print("RSSI");
    gfx->setCursor(LINKSTATS_COL_FIRST, LINKSTATS_ROW_FOURTH);
    gfx->print("SNR");
    gfx->setCursor(LINKSTATS_COL_FIRST, LINKSTATS_ROW_FIFTH);
    gfx->print("Ant");
    gfx->setCursor(LINKSTATS_COL_FIRST, LINKSTATS_ROW_SIXTH);
    gfx->print("Batt");

    gfx->setTextColor(CYAN,BKGRND);
    // Uplink Linkstats
    gfx->setCursor(LINKSTATS_COL_SECOND-4, LINKSTATS_ROW_FIRST);
    gfx->print("Uplink");
    gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_SECOND);
    gfx->print(CRSF::LinkStatistics.uplink_Link_quality);
    gfx->setCursor(LINKSTATS_COL_SECOND-8, LINKSTATS_ROW_THIRD);
    gfx->print((int8_t)CRSF::LinkStatistics.uplink_RSSI_1);
    if (CRSF::LinkStatistics.uplink_RSSI_2 != 0)
    {
        gfx->print('/');
        gfx->print((int8_t)CRSF::LinkStatistics.uplink_RSSI_2);
    }

    gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_FOURTH);
    gfx->print(CRSF::LinkStatistics.uplink_SNR);
    gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_FIFTH);
    gfx->print(CRSF::LinkStatistics.active_antenna);
    gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_SIXTH);

    if(airBat_Val)
    {
        if(config.GetModelMatch())
        {
            if(airBat_Val / config.GetModelMatch_VL() < 35)
            {
                gfx->setTextColor(BLUE,BKGRND);     //red actually
                gfx->print("LOW");
                gpio_set_level(GPIO_NUM_45, 0);
                vTaskDelay(pdMS_TO_TICKS(50));      //50mS pause
                gpio_set_level(GPIO_NUM_45, 1);
            }
            else gfx->print("NORM");
        }
        else
        {
            char buffer[5];
            snprintf(buffer, sizeof(buffer), "%2u.%u", airBat_Val / 10, airBat_Val % 10);
            gfx->print(buffer);
        }
    }
    else gfx->print("NO");

    gfx->setTextColor(GREEN,BKGRND);
    // Downlink Linkstats
    gfx->setCursor(LINKSTATS_COL_THIRD-8, LINKSTATS_ROW_FIRST);
    gfx->print("Downlink");
    gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_SECOND);
    gfx->print(CRSF::LinkStatistics.downlink_Link_quality);
    gfx->setCursor(LINKSTATS_COL_THIRD-8, LINKSTATS_ROW_THIRD);
    gfx->print((int8_t)CRSF::LinkStatistics.downlink_RSSI_1);
    if (isDualRadio())
    {
        gfx->print('/');
        gfx->print((int8_t)CRSF::LinkStatistics.downlink_RSSI_2);
    }

    gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_FOURTH);
    gfx->print(CRSF::LinkStatistics.downlink_SNR);
    gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_FIFTH);
    gfx->print("Model");    //(config.GetModelMatch_ST());
    gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_SIXTH);
    if(connectionHasModelMatch)
    {
        gfx->setCursor(LINKSTATS_COL_THIRD - 6, LINKSTATS_ROW_SIXTH);
        gfx->print("matched");    //(config.GetModelMatch_VL());
    }
    else
    {
        gfx->setTextColor(BLUE,BKGRND);     //res actually
        gfx->print("missed");    //(config.GetModelMatch_VL());
    }
}

void TFTDisplay::displayTelemetry()
{
    constexpr int16_t TELEMETRY_COL_FIRST   = 5;    //5
    // constexpr int16_t TELEMETRY_COL_SECOND  = 40;   //45
    constexpr int16_t TELEMETRY_COL_THIRD   = 75;  //110
    constexpr int16_t TELEMETRY_COL_FOURTH   = 115;

    constexpr int16_t TELEMETRY_ROW_FIRST   = 14;
    constexpr int16_t TELEMETRY_ROW_SECOND  = 29;
    constexpr int16_t TELEMETRY_ROW_THIRD   = 44;
    constexpr int16_t TELEMETRY_ROW_FOURTH  = 59;
    constexpr int16_t TELEMETRY_ROW_FIFTH   = 74;
    constexpr int16_t TELEMETRY_ROW_SIXTH   = 89;

    gfx->fillScreen(BKGRND);
    gfx->setFont(&SCREEN_NORMAL_FONT);
    gfx->setTextColor(YELLOW,BKGRND);

    gfx->setCursor(TELEMETRY_COL_FIRST, TELEMETRY_ROW_SECOND);
    gfx->print("Battery");
    gfx->setCursor(TELEMETRY_COL_FIRST, TELEMETRY_ROW_THIRD);
    gfx->print("FlightMode");
    gfx->setCursor(TELEMETRY_COL_FIRST, TELEMETRY_ROW_FOURTH);
    gfx->print("Pdop");
    gfx->setCursor(TELEMETRY_COL_FIRST, TELEMETRY_ROW_FIFTH);
    gfx->print("Latitude");
    gfx->setCursor(TELEMETRY_COL_FIRST, TELEMETRY_ROW_SIXTH);
    gfx->print("Longitude");

    gfx->setTextColor(GREEN,BKGRND);

    gfx->setCursor(TELEMETRY_COL_THIRD - 2, TELEMETRY_ROW_FIRST);
    gfx->print("TELEMETRY");
    gfx->setTextColor(CYAN,BKGRND);
    // batt
    gfx->setCursor(TELEMETRY_COL_THIRD + 15, TELEMETRY_ROW_SECOND);
    if(airBat_Val)
    {
        if(config.GetModelMatch())
        {
            if(airBat_Val / config.GetModelMatch_VL() < 35)
            {
                gfx->setTextColor(BLUE,BKGRND);     //red actually
                gpio_set_level(GPIO_NUM_45, 0);
                vTaskDelay(pdMS_TO_TICKS(50));      //50mS pause
                gpio_set_level(GPIO_NUM_45, 1);
            }
        }

        char buffer[5];
        snprintf(buffer, sizeof(buffer), "%2u.%u", airBat_Val / 10, airBat_Val % 10);
        gfx->print(buffer);
    }
    else gfx->print("NO");
    // Flight mode
    gfx->setCursor(TELEMETRY_COL_THIRD, TELEMETRY_ROW_THIRD);
    if(flightMode[0])
    {
        gfx->print(flightMode);
    }
    else gfx->print("     NO");
    // Pdop
    gfx->setCursor(TELEMETRY_COL_THIRD, TELEMETRY_ROW_FOURTH);
    gfx->print(adcCounter);
    // Lat
    gfx->setCursor(TELEMETRY_COL_THIRD, TELEMETRY_ROW_FIFTH);
    gfx->print(adcBytes);
    // Lon
    gfx->setCursor(TELEMETRY_COL_THIRD, TELEMETRY_ROW_SIXTH);
    gfx->print(adcRaw1);    

    gfx->setTextColor(WHITE,BKGRND);
    // batt
    gfx->setCursor(TELEMETRY_COL_FOURTH, TELEMETRY_ROW_SECOND);
    gfx->print("Volts");
    // Flight mode
    // gfx->setCursor(TELEMETRY_COL_FOURTH, TELEMETRY_ROW_THIRD);
    // gfx->print("Mode");
    // Pdop
    gfx->setCursor(TELEMETRY_COL_FOURTH, TELEMETRY_ROW_FOURTH);
    gfx->print(adcRaw2);
    // Lat
    gfx->setCursor(TELEMETRY_COL_FOURTH, TELEMETRY_ROW_FIFTH);
    gfx->print(adcRaw3);
    // Lon
    gfx->setCursor(TELEMETRY_COL_FOURTH, TELEMETRY_ROW_SIXTH);
    gfx->print(adcRaw4);
}

void TFTDisplay::displayGimbals()
{
    constexpr int16_t GIMBALS_COL_FIRST   = 5;    //5
    constexpr int16_t GIMBALS_COL_SECOND  = 40;   //45
    constexpr int16_t GIMBALS_COL_THIRD   = 80;  //110
    constexpr int16_t GIMBALS_COL_FOURTH   = 120;

    constexpr int16_t GIMBALS_ROW_FIRST   = 10;
    constexpr int16_t GIMBALS_ROW_SECOND  = 23;
    constexpr int16_t GIMBALS_ROW_THIRD   = 36;
    constexpr int16_t GIMBALS_ROW_FOURTH  = 49;
    constexpr int16_t GIMBALS_ROW_FIFTH   = 62;
    constexpr int16_t GIMBALS_ROW_SIXTH   = 75;
    constexpr int16_t GIMBALS_ROW_SEVENTH = 88;

    gfx->fillScreen(BKGRND);
    gfx->setFont(&SCREEN_SMALL_FONT);
    gfx->setTextColor(YELLOW,BKGRND);

    gfx->setCursor(GIMBALS_COL_FIRST, GIMBALS_ROW_SECOND);
    gfx->print("Index");
    gfx->setCursor(GIMBALS_COL_FIRST, GIMBALS_ROW_THIRD);
    gfx->print("roll");
    gfx->setCursor(GIMBALS_COL_FIRST, GIMBALS_ROW_FOURTH);
    gfx->print("pitch");
    gfx->setCursor(GIMBALS_COL_FIRST, GIMBALS_ROW_FIFTH);
    gfx->print("thrtl");
    gfx->setCursor(GIMBALS_COL_FIRST, GIMBALS_ROW_SIXTH);
    gfx->print("yaw");
    gfx->setCursor(GIMBALS_COL_FIRST, GIMBALS_ROW_SEVENTH);
    gfx->print("ADC");

    gfx->setTextColor(GREEN,BKGRND);
    gfx->setCursor(GIMBALS_COL_SECOND + 3, GIMBALS_ROW_FIRST);
    gfx->print("MIN");

    gfx->setCursor(GIMBALS_COL_THIRD - 2, GIMBALS_ROW_FIRST);
    gfx->print("CENT");

    gfx->setCursor(GIMBALS_COL_FOURTH + 2, GIMBALS_ROW_FIRST);
    gfx->print("MAX");

    gfx->setTextColor(WHITE,BKGRND);
    // memory indexes
    gfx->setCursor(GIMBALS_COL_SECOND, GIMBALS_ROW_SECOND);
    gfx->print(config.GetCalEnd_points());
    gfx->setCursor(GIMBALS_COL_THIRD, GIMBALS_ROW_SECOND);
    gfx->print(config.GetCalCenrt_vals());
    gfx->setCursor(GIMBALS_COL_FOURTH, GIMBALS_ROW_SECOND);
    gfx->print(config.GetCalEnd_points());
    // roll
    gfx->setCursor(GIMBALS_COL_SECOND, GIMBALS_ROW_THIRD);
    gfx->print(config.GetCalibratedValue(ROLL_MIN));
    gfx->setCursor(GIMBALS_COL_THIRD, GIMBALS_ROW_THIRD);
    gfx->print(config.GetCalibratedValue(ROLL_CTR));
    gfx->setCursor(GIMBALS_COL_FOURTH, GIMBALS_ROW_THIRD);
    gfx->print(config.GetCalibratedValue(ROLL_MAX));
    // pitch
    gfx->setCursor(GIMBALS_COL_SECOND, GIMBALS_ROW_FOURTH);
    gfx->print(config.GetCalibratedValue(PITCH_MIN));
    gfx->setCursor(GIMBALS_COL_THIRD, GIMBALS_ROW_FOURTH);
    gfx->print(config.GetCalibratedValue(PITCH_CTR));
    gfx->setCursor(GIMBALS_COL_FOURTH, GIMBALS_ROW_FOURTH);
    gfx->print(config.GetCalibratedValue(PITCH_MAX));
    // throttle
    gfx->setCursor(GIMBALS_COL_SECOND, GIMBALS_ROW_FIFTH);
    gfx->print(config.GetCalibratedValue(THRTL_MIN));
    gfx->setCursor(GIMBALS_COL_THIRD, GIMBALS_ROW_FIFTH);
    gfx->print(config.GetReverse_gmbls());      //(config.GetCalibratedValue(REV_IND));
    gfx->setCursor(GIMBALS_COL_FOURTH, GIMBALS_ROW_FIFTH);
    gfx->print(config.GetCalibratedValue(THRTL_MAX));
    // yaw
    gfx->setCursor(GIMBALS_COL_SECOND, GIMBALS_ROW_SIXTH);
    gfx->print(config.GetCalibratedValue(YAW_MIN));
    gfx->setCursor(GIMBALS_COL_THIRD, GIMBALS_ROW_SIXTH);
    gfx->print(config.GetCalibratedValue(YAW_CTR));
    gfx->setCursor(GIMBALS_COL_FOURTH, GIMBALS_ROW_SIXTH);
    gfx->print(config.GetCalibratedValue(YAW_MAX));
    // ADC
    gfx->setCursor(GIMBALS_COL_SECOND, GIMBALS_ROW_SEVENTH);
    gfx->print(adcCounter);
    gfx->setCursor(GIMBALS_COL_THIRD, GIMBALS_ROW_SEVENTH);
    gfx->print(adcBytes);
    gfx->setCursor(GIMBALS_COL_FOURTH, GIMBALS_ROW_SEVENTH);
    gfx->print((int16_t)handset_vbat);   //handset voltage
}
//my@ #endif
