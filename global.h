#ifndef _H_MPN_GLOBAL
#define _H_MPN_GLOBAL

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#define VERSION "1.04"

/* change log:
 * 1.03	2021/10/02 trust/pairing, screensaver, less momory
 * 1.04 2021/11/09 no screensaver on V1 boards, auto detect 
 *                 board, logging (telnet, syslog, mqtt), falish
 *                 reporting much like the ACL nodes. Support V3
 *                 boards.
 */

#ifndef TERMINAL_NAME
#define TERMINAL_NAME "tft"
#endif

#ifndef WIFI_NETWORK
#define WIFI_NETWORK "MyWifiNetwork"
#endif

#ifndef WIFI_PASSWD
#define WIFI_PASSWD "MyWifiPassword"
#endif

#ifndef PAY_URL
#define PAY_URL "https://test.makerspaceleiden.nl:4443/crm/pettycash/api"
#endif

#define HTTP_TIMEOUT (5000)

// Jump back to the default after this many milliseconds, provided
// that there is a default item set in the CRM.
//
#define DEFAULT_TIMEOUT (60*1000)

// Reboot every day (or comment this out).
#define AUTO_REBOOT_TIME "04:00"

// Full on is a bit bright relative to the screen; so we safe
// that for errors/special cases.
//
#define NORMAL_LED_BRIGHTNESS 220


// Wait up to 10 seconds for wifi during boot.
#define WIFI_MAX_WAIT (20*1000)

#ifndef NTP_POOL
#define NTP_POOL "nl.pool.ntp.org"
#endif

typedef enum { BOOT = 0, WAITING_FOR_NTP, FETCH_CA, SCREENSAVER, RUN, OK_OR_CANCEL } state_t;

// Board differences
// v1 buttons pull to ground; with internal PULLUP used. 
//    No blacklight control.
// v2 has its buttons wired to the VCC; with pulldowns.
//    detect this board by checking that BUT1/2 are 
//    pulled low. Backlight on pin 4. -- grijpvoorraad
// v3 buttons to GND, LEDs on on HIGH -- voorruimte
// v4 buttons to GND, LEDs on on HIGH, screen flipped.
typedef enum { BOARD_V2,  BOARD_V3, BOARD_V4 } board_t;
extern board_t BOARD;


extern const char * version;
extern char terminalName[64];
void led_loop(state_t md);

#endif
