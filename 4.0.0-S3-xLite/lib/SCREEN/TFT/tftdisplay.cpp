//my@ #if defined(PLATFORM_ESP32) && !defined(PLATFORM_ESP32_C3)

#include <Arduino_GFX_Library.h>
#include "Pragma_Sans36pt7b.h"
#include "Pragma_Sans37pt7b.h"
#include "Pragma_Sans314pt7b.h"

#include "tftdisplay.h"

#include "CRSFRouter.h"
#include "common.h"
#include "logging.h"
#include "logos.h"
#include "options.h"
//my@
#include "devEncoder.h"
#include "devAnalogVbat.h"
#include "devADC.h"
#include "config.h"
//@
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
//my@
    elrs_joystick,
    elrs_fan,
    elrs_fan,
    elrs_fan,
    elrs_motion,
    elrs_motion,
    elrs_motion,
};

#define ST7735_BLUE     RED
#define ST7735_RED      BLUE
#define ST7735_CYAN     YELLOW
#define ST7735_YELLOW   CYAN
#define ST7735_AMBER        0x01FF
#define ST7735_LIGHTBLUE    0xFF00
#define ST7735_LIGHTGREEN    0x5F00
#define ST7735_LIGHTRED      0x0A1F

// Hex color code to 16-bit rgb:
// color = 0x96c76f
// rgb_hex = ((((color&0xFF0000)>>16)&0xf8)<<8) + ((((color&0x00FF00)>>8)&0xfc)<<3) + ((color&0x0000FF)>>3)
constexpr uint16_t elrs_banner_bgColor[] = {
    ST7735_LIGHTBLUE,   //0x4315, // MSG_NONE          => #4361AA (ELRS blue)
    ST7735_LIGHTGREEN,  //0x9E2D, // MSG_CONNECTED     => #9FC76F (ELRS green)
    ST7735_LIGHTRED,    //0xAA08, // MSG_ARMED         => #AA4343 (red)
    ST7735_AMBER,       //0xF501, // MSG_MISMATCH      => #F0A30A (amber)
    ST7735_RED          //0xF800  // MSG_ERROR         => #F0A30A (very red)
};

#define SCREEN_X    160
#define SCREEN_Y    96  //my@    80

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
#define INIT_PAGE_LOGO_Y            55      //53
#define INIT_PAGE_FONT_PADDING      3
#define INIT_PAGE_FONT_START_X      SCREEN_FONT_GAP
#define INIT_PAGE_FONT_START_Y      INIT_PAGE_LOGO_Y + (SCREEN_Y - INIT_PAGE_LOGO_Y - SCREEN_NORMAL_FONT_SIZE)/2

//IDLE PAGE Definition
#define IDLE_PAGE_START_X   SCREEN_CONTENT_GAP
#define IDLE_PAGE_START_Y   4   //2 my@ 0

#define IDLE_PAGE_STAT_START_X  SCREEN_X/2 // is centered, so no GAP
#define IDLE_PAGE_STAT_Y_GAP    0       //my@ (SCREEN_Y -  SCREEN_NORMAL_FONT_SIZE * 3)/4

#define IDLE_PAGE_RATE_START_Y  IDLE_PAGE_START_Y + IDLE_PAGE_STAT_Y_GAP
#define IDLE_PAGE_RATIO_START_Y IDLE_PAGE_RATE_START_Y + SCREEN_LARGE_FONT_SIZE + IDLE_PAGE_STAT_Y_GAP          //my@ large
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

uint8_t BKGRND = BLACK; //my@

void TFTDisplay::init()
{
    if (GPIO_PIN_SCREEN_BL != UNDEF_PIN)
    {
        pinMode(GPIO_PIN_SCREEN_BL, OUTPUT);
    }
    bus = new Arduino_ESP32SPI(GPIO_PIN_SCREEN_DC, GPIO_PIN_SCREEN_CS, GPIO_PIN_SCREEN_SCK, GPIO_PIN_SCREEN_MOSI, GFX_NOT_DEFINED, HSPI);
    // gfx = new Arduino_ST7735(bus, GPIO_PIN_SCREEN_RST, OPT_SCREEN_REVERSED ? 3 : 1 /* rotation */, true, 80, 160, 26, 1, 26, 1);
    gfx = new Arduino_ST7735(bus, GPIO_PIN_SCREEN_RST, OPT_SCREEN_REVERSED ? 3 : 1 /* rotation */, false, 96, 160, 19, 1, 19, 1);
    // gfx = new Arduino_NV3023(bus, GPIO_PIN_SCREEN_RST, OPT_SCREEN_REVERSED ? 1 : 3 /* rotation */, true, 96, 160, 16, 0, 16, 0);

    gfx->begin();
    doScreenBackLight(SCREEN_BACKLIGHT_ON);
}

void TFTDisplay::doScreenBackLight(screen_backlight_t state)
{
    if (GPIO_PIN_SCREEN_BL != UNDEF_PIN)
    {
        digitalWrite(GPIO_PIN_SCREEN_BL, state);
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
        gfx->draw16bitRGBBitmap(0, 5, image, INIT_PAGE_LOGO_X, INIT_PAGE_LOGO_Y);
    }

    gfx->fillRect(SCREEN_FONT_GAP, INIT_PAGE_FONT_START_Y - INIT_PAGE_FONT_PADDING,
                    SCREEN_X - SCREEN_FONT_GAP*2, SCREEN_NORMAL_FONT_SIZE + INIT_PAGE_FONT_PADDING*2, ST7735_BLUE); //ST7735_RED my@ BLACK

    char buffer[50];
    snprintf(buffer, sizeof(buffer), "ELRS-%.6s", version);
    displayFontCenter(INIT_PAGE_FONT_START_X, SCREEN_X - INIT_PAGE_FONT_START_X, INIT_PAGE_FONT_START_Y +1, //add +1
                        SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        String(buffer), ST7735_YELLOW, ST7735_BLUE); //ST7735_YELLOW/ST7735_BLUE   //my@ WHITE, BLACK);
}

void TFTDisplay::displayIdleScreen(uint8_t changed, uint8_t rate_index, uint8_t power_index, uint8_t ratio_index, uint8_t motion_index, uint8_t fan_index, bool dynamic, uint8_t running_power_index, uint8_t temperature, message_index_t message_index)
{
    if (changed == CHANGED_ALL)
    {
        gpio_set_level(GPIO_NUM_45, 0);     //BUZZER OFF
        // Everything has changed! So clear the right side
        gfx->fillRect(SCREEN_X/2, 0, SCREEN_X/2, SCREEN_Y, BKGRND);     //my@ WHITE
    }

    if (changed & CHANGED_TEMP)
    {
        // Left side logo, version, and temp
        gfx->fillRect(0, 0, SCREEN_X/2, SCREEN_Y, elrs_banner_bgColor[message_index]);
        gfx->drawBitmap(IDLE_PAGE_START_X, IDLE_PAGE_START_Y, elrs_banner_bmp, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE,
                        BKGRND);     //my@ WHITE

        // Update the temperature
        char buffer[20];
        // \367 = (char)247 = degree symbol
        //my@ snprintf(buffer, sizeof(buffer), "%.6s %02d\367C", version, temperature);
        // displayFontCenter(0, SCREEN_X/2, SCREEN_LARGE_ICON_SIZE + (SCREEN_Y - SCREEN_LARGE_ICON_SIZE - SCREEN_SMALL_FONT_SIZE)/2,
        //                     SCREEN_SMALL_FONT_SIZE, SCREEN_SMALL_FONT,
        //                     String(buffer), WHITE, elrs_banner_bgColor[message_index]);
        snprintf(buffer, sizeof(buffer), "%d,%02dV", handset_vbat/100, handset_vbat%100);
        displayFontCenter(0, SCREEN_X/2, SCREEN_LARGE_ICON_SIZE + (SCREEN_Y - SCREEN_LARGE_ICON_SIZE - SCREEN_LARGE_FONT_SIZE)/2,
                         SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT, String(buffer), BKGRND, elrs_banner_bgColor[message_index]);
    }

    // The Radio Params right half of the screen
    uint16_t text_color = (message_index == MSG_ARMED) ? ST7735_CYAN : GREEN;    //ST7735_YELLOW : GREEN;

    if (connectionState == radioFailed)
    {
        displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
            "BAD", BLACK, WHITE);
        displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
            "RADIO", BLACK, WHITE);
    }
    else if (connectionState == noCrossfire)
    {
        displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
            "NO", BLACK, WHITE);
        displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
            "HANDSET", BLACK, WHITE);
    }
    else
    {
        if (changed & CHANGED_RATE)
        {
            displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, IDLE_PAGE_RATE_START_Y,  SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT, //my@ large
                                getValue(STATE_PACKET, rate_index), text_color, BKGRND);    //my@ WHITE);
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

        if((connectionState == connected) && connectionHasModelMatch)
        {
            displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, IDLE_PAGE_STATE1_START_Y,  SCREEN_NORMAL_FONT_SIZE,
                SCREEN_NORMAL_FONT, String(buffer1), ST7735_YELLOW, BKGRND); //ST7735_CYAN
        }
        else
        {
            snprintf(buffer1, sizeof(buffer1), "mismatch");
            displayFontCenter(IDLE_PAGE_STAT_START_X, SCREEN_X, IDLE_PAGE_STATE1_START_Y,  SCREEN_NORMAL_FONT_SIZE,
                SCREEN_NORMAL_FONT, String(buffer1), ST7735_RED, BKGRND);  //ST7735_BLUE
        }
    }
}

//menu order defined in SCREEN/menu.cpp/fsm_state_entry_t const main_menu_fsm
void TFTDisplay::displayMainMenu(menu_item_t menu)
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(MAIN_PAGE_ICON_START_X, MAIN_PAGE_ICON_START_Y, main_menu_icons[menu], SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    displayFontCenter(MAIN_PAGE_WORD_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
        main_menu_strings[menu][0], GREEN, BKGRND);     //my@BLACK, WHITE);
    displayFontCenter(MAIN_PAGE_WORD_START_X, SCREEN_X, MAIN_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
        main_menu_strings[menu][1], ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
}

void TFTDisplay::displayValue(menu_item_t menu, uint8_t value_index)
{
    gfx->fillScreen(BKGRND);

    String val = String(getValue(menu, value_index));
    val.replace("!+", "\xA0");
    val.replace("!-", "\xA1");

    displayFontCenter(SUB_PAGE_VALUE_START_X, SCREEN_X, SUB_PAGE_VALUE_START_Y,  SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT,
                        val.c_str(), GREEN, BKGRND);            //my@BLACK, WHITE);
    displayFontCenter(SUB_PAGE_TIPS_START_X, SCREEN_X, SUB_PAGE_TIPS_START_Y,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "PRESS TO CONFIRM", ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
}

void TFTDisplay::displayBLEConfirm()
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(SUB_PAGE_ICON_START_X, SUB_PAGE_ICON_START_Y, elrs_joystick, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "PRESS TO", GREEN, BKGRND); //my@BLACK, WHITE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "START BLE", ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "GAMEPAD", ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
}

void TFTDisplay::displayBLEStatus()
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(SUB_PAGE_ICON_START_X, SUB_PAGE_ICON_START_Y, elrs_joystick, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "BLE", GREEN, BKGRND); //my@BLACK, WHITE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "GAMEPAD", ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "RUNNING", ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
}

void TFTDisplay::displayWiFiConfirm()
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(SUB_PAGE_ICON_START_X, SUB_PAGE_ICON_START_Y, elrs_wifimode, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "PRESS TO", GREEN, BKGRND);    //my@BLACK, WHITE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "ENTER WIFI", ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "UPDATE MODE", ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
}

void TFTDisplay::displayWiFiStatus()
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(SUB_PAGE_ICON_START_X, SUB_PAGE_ICON_START_Y, elrs_wifimode, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    if (wifiMode == WIFI_STA) {
        String host_msg = String(wifi_hostname) + ".local";
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            "open http://", GREEN, BKGRND);    //my@BLACK, WHITE);
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            host_msg, ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            "by browser", ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
    }
    else
    {
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            wifi_ap_ssid, GREEN, BKGRND);    //my@BLACK, WHITE);
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            wifi_ap_password, ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
        displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                            wifi_ap_address, ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
    }
}

void TFTDisplay::displayBindConfirm()
{
    gfx->fillScreen(BKGRND);

    gfx->draw16bitRGBBitmap(SUB_PAGE_ICON_START_X, SUB_PAGE_ICON_START_Y, elrs_bind, SCREEN_LARGE_ICON_SIZE, SCREEN_LARGE_ICON_SIZE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y1,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "PRESS TO", GREEN, BKGRND);    //my@BLACK, WHITE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y2,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "SEND BIND", ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
    displayFontCenter(SUB_PAGE_WORD_START_X, SCREEN_X, SUB_PAGE_WORD_START_Y3,  SCREEN_NORMAL_FONT_SIZE, SCREEN_NORMAL_FONT,
                        "REQUEST", ST7735_CYAN, BKGRND);    //ST7735_YELLOW my@BLACK, WHITE);
}

void TFTDisplay::displayBindStatus()
{
    gfx->fillScreen(BKGRND);

    displayFontCenter(SUB_PAGE_BINDING_WORD_START_X, SCREEN_X, SUB_PAGE_BINDING_WORD_START_Y,  SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT,
                        "BINDING...", ST7735_RED, BKGRND);
}

void TFTDisplay::displayRunning()
{
    gfx->fillScreen(BKGRND);

    displayFontCenter(SUB_PAGE_BINDING_WORD_START_X, SCREEN_X, SUB_PAGE_BINDING_WORD_START_Y,  SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT,
                        "RUNNING...", ST7735_CYAN, BKGRND);  //ST7735_YELLOW
}

void TFTDisplay::displaySending()
{
    gfx->fillScreen(BKGRND);

    displayFontCenter(SUB_PAGE_BINDING_WORD_START_X, SCREEN_X, SUB_PAGE_BINDING_WORD_START_Y,  SCREEN_LARGE_FONT_SIZE, SCREEN_LARGE_FONT,
                        "SENDING...", ST7735_CYAN, BKGRND);  //ST7735_YELLOW
}

// void TFTDisplay::displayLinkstats()
// {
//     constexpr int16_t LINKSTATS_COL_FIRST   = 0;
//     constexpr int16_t LINKSTATS_COL_SECOND  = 30;
//     constexpr int16_t LINKSTATS_COL_THIRD   = 100;

//     constexpr int16_t LINKSTATS_ROW_FIRST   = 10;
//     constexpr int16_t LINKSTATS_ROW_SECOND  = 25;
//     constexpr int16_t LINKSTATS_ROW_THIRD   = 40;
//     constexpr int16_t LINKSTATS_ROW_FOURTH  = 55;
//     constexpr int16_t LINKSTATS_ROW_FIFTH   = 70;

//     gfx->fillScreen(WHITE);
//     gfx->setFont(&SCREEN_SMALL_FONT);
//     gfx->setTextColor(BLACK, WHITE);

//     gfx->setCursor(LINKSTATS_COL_FIRST, LINKSTATS_ROW_SECOND);
//     gfx->print("LQ");
//     gfx->setCursor(LINKSTATS_COL_FIRST, LINKSTATS_ROW_THIRD);
//     gfx->print("RSSI");
//     gfx->setCursor(LINKSTATS_COL_FIRST, LINKSTATS_ROW_FOURTH);
//     gfx->print("SNR");
//     gfx->setCursor(LINKSTATS_COL_FIRST, LINKSTATS_ROW_FIFTH);
//     gfx->print("Ant");

//     // Uplink Linkstats
//     gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_FIRST);
//     gfx->print("Uplink");
//     gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_SECOND);
//     gfx->print(linkStats.uplink_Link_quality);
//     gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_THIRD);
//     gfx->print((int8_t)linkStats.uplink_RSSI_1);
//     if (linkStats.uplink_RSSI_2 != 0)
//     {
//         gfx->print('/');
//         gfx->print((int8_t)linkStats.uplink_RSSI_2);
//     }

//     gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_FOURTH);
//     gfx->print(linkStats.uplink_SNR);
//     gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_FIFTH);
//     gfx->print(linkStats.active_antenna);

//     // Downlink Linkstats
//     gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_FIRST);
//     gfx->print("Downlink");
//     gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_SECOND);
//     gfx->print(linkStats.downlink_Link_quality);
//     gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_THIRD);
//     gfx->print((int8_t)linkStats.downlink_RSSI_1);
//     if (isDualRadio())
//     {
//         gfx->print('/');
//         gfx->print((int8_t)linkStats.downlink_RSSI_2);
//     }

//     gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_FOURTH);
//     gfx->print(linkStats.downlink_SNR);
// }
//my@ #endif
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
    gfx->setTextColor(ST7735_YELLOW,BKGRND);

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

    gfx->setTextColor(ST7735_CYAN,BKGRND);
    // Uplink Linkstats
    gfx->setCursor(LINKSTATS_COL_SECOND-4, LINKSTATS_ROW_FIRST);
    gfx->print("Uplink");
    gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_SECOND);
    gfx->print(linkStats.uplink_Link_quality);                //(CRSF::LinkStatistics.uplink_Link_quality);
    gfx->setCursor(LINKSTATS_COL_SECOND-8, LINKSTATS_ROW_THIRD);
    gfx->print((int8_t)linkStats.uplink_RSSI_1);              //((int8_t)CRSF::LinkStatistics.uplink_RSSI_1);
    if (isDualRadio())                                          //(CRSF::LinkStatistics.uplink_RSSI_2 != 0)
    {
        gfx->print('/');
        gfx->print((int8_t)linkStats.uplink_RSSI_2);          //((int8_t)CRSF::LinkStatistics.uplink_RSSI_2);
    }

    gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_FOURTH);
    gfx->print(linkStats.uplink_SNR);                           //(CRSF::LinkStatistics.uplink_SNR);
    gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_FIFTH);
    gfx->print(linkStats.active_antenna);                       //(CRSF::LinkStatistics.active_antenna);
    gfx->setCursor(LINKSTATS_COL_SECOND, LINKSTATS_ROW_SIXTH);

    if(airBat_Val)
    {
        if(config.GetModelMatch_VL())       //do not divide by 0
        {
            if(airBat_Val / config.GetModelMatch_VL() < 35)
            {
                gfx->setTextColor(ST7735_RED,BKGRND);     //ST7735_BLUE red actually
                gfx->print("LOW");
                gpio_set_level(GPIO_NUM_45, 1);     //BUZZER ON
                vTaskDelay(pdMS_TO_TICKS(50));      //50mS pause
                gpio_set_level(GPIO_NUM_45, 0);     //BUZZER OFF
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
    gfx->print(linkStats.downlink_Link_quality);                //(CRSF::LinkStatistics.downlink_Link_quality);
    gfx->setCursor(LINKSTATS_COL_THIRD-8, LINKSTATS_ROW_THIRD);
    gfx->print((int8_t)linkStats.downlink_RSSI_1);              //((int8_t)CRSF::LinkStatistics.downlink_RSSI_1);
    if (isDualRadio())
    {
        gfx->print('/');
        gfx->print((int8_t)linkStats.downlink_RSSI_2);          //((int8_t)CRSF::LinkStatistics.downlink_RSSI_2);
    }

    gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_FOURTH);
    gfx->print(linkStats.downlink_SNR);                         //(CRSF::LinkStatistics.downlink_SNR);
    gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_FIFTH);
    gfx->print("Model");    //(config.GetModelMatch_ST());
    gfx->setCursor(LINKSTATS_COL_THIRD, LINKSTATS_ROW_SIXTH);
    if((connectionState == connected) && connectionHasModelMatch)
    {
        gfx->setCursor(LINKSTATS_COL_THIRD - 6, LINKSTATS_ROW_SIXTH);
        gfx->print("matched");    //(config.GetModelMatch_VL());
    }
    else
    {
        gfx->setTextColor(ST7735_RED,BKGRND);     //ST7735_BLUE red actually
        gfx->print("missed");    //(config.GetModelMatch_VL());
    }
}

void TFTDisplay::displayTelemetry()
{
    char buffer[7];

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
    gfx->setTextColor(ST7735_CYAN,BKGRND);     //ST7735_YELLOW

    gfx->setCursor(TELEMETRY_COL_FIRST, TELEMETRY_ROW_SECOND);
    gfx->print("Battery");
    gfx->setCursor(TELEMETRY_COL_FIRST, TELEMETRY_ROW_THIRD);
    gfx->print("FlightMode");
    gfx->setCursor(TELEMETRY_COL_FIRST, TELEMETRY_ROW_FOURTH);
    gfx->print("Speed");
    gfx->setCursor(TELEMETRY_COL_FIRST, TELEMETRY_ROW_FIFTH);
    gfx->print("Latitude");
    gfx->setCursor(TELEMETRY_COL_FIRST, TELEMETRY_ROW_SIXTH);
    gfx->print("Longitude");

    gfx->setTextColor(GREEN,BKGRND);

    gfx->setCursor(TELEMETRY_COL_THIRD - 2, TELEMETRY_ROW_FIRST);
    gfx->print("TELEMETRY");
    gfx->setTextColor(ST7735_YELLOW,BKGRND);   //ST7735_CYAN
    // batt
    gfx->setCursor(TELEMETRY_COL_THIRD + 15, TELEMETRY_ROW_SECOND);
    if(airBat_Val)
    {
        if(config.GetModelMatch_VL())       //do not divide by 0
        {
            if(airBat_Val / config.GetModelMatch_VL() < 35)
            {
                gfx->setTextColor(ST7735_RED,BKGRND);      //ST7735_BLUE red actually
                gpio_set_level(GPIO_NUM_45, 1);     //BUZZER ON
                vTaskDelay(pdMS_TO_TICKS(50));      //50mS pause
                gpio_set_level(GPIO_NUM_45, 0);     //BUZZER OFF
            }
        }
        snprintf(buffer, sizeof(buffer), "%2u.%u", airBat_Val / 10, airBat_Val % 10);
        gfx->print(buffer);
    }
    else gfx->print("No");
    // Flight mode
    gfx->setCursor(TELEMETRY_COL_THIRD, TELEMETRY_ROW_THIRD);
    if(flightMode[0])
    {
        gfx->print(flightMode);
    }
    else gfx->print("    Unknown"); //("     NO");
    // Pdop
    gfx->setCursor(TELEMETRY_COL_THIRD, TELEMETRY_ROW_FOURTH);
    if(sats)
    {
        // gfx->print(speed);
        snprintf(buffer, sizeof(buffer), "%2u.%u", speed / 10, speed % 10);
        gfx->print(buffer);
    }
    else gfx->print("     No");
    // Lat
    gfx->setCursor(TELEMETRY_COL_THIRD, TELEMETRY_ROW_FIFTH);
    if(latitude)
    {
        gfx->print(latitude);
    }
    else gfx->print("     No");
    // Lon
    gfx->setCursor(TELEMETRY_COL_THIRD, TELEMETRY_ROW_SIXTH);
    if(longitude)
    {
        gfx->print(longitude);
    }
    else gfx->print("     No");    

    gfx->setTextColor(WHITE,BKGRND);
    // batt
    gfx->setCursor(TELEMETRY_COL_FOURTH, TELEMETRY_ROW_SECOND);
    if(airBat_Val)
    {
        gfx->print("Volts");
    }
    else gfx->print("Data");
    // Flight mode
    // gfx->setCursor(TELEMETRY_COL_FOURTH, TELEMETRY_ROW_THIRD);
    // gfx->print("Mode");
    // Pdop
    gfx->setCursor(TELEMETRY_COL_FOURTH, TELEMETRY_ROW_FOURTH);
    if(sats)
    {
        // gfx->print(sats);
        snprintf(buffer, sizeof(buffer), "%usat", sats);
        gfx->print(buffer);
    }
    else gfx->print("Data");
    // Lat
    gfx->setCursor(TELEMETRY_COL_FOURTH, TELEMETRY_ROW_FIFTH);
    if(!latitude) gfx->print("Data");
    // gfx->print(adcCounter);
    // Lon
    gfx->setCursor(TELEMETRY_COL_FOURTH, TELEMETRY_ROW_SIXTH);
    if(!longitude) gfx->print("Data");
    // gfx->print(adcBytes);
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
    gfx->setTextColor(ST7735_CYAN,BKGRND);     //ST7735_YELLOW

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
    gfx->print(handsetEncoder);   //(int16_t)handset_vbat
}
