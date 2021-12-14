// dirkx@webweaving.org, apache license, for the makerspaceleiden.nl
//
// https://sites.google.com/site/jmaathuis/arduino/lilygo-ttgo-t-display-esp32
// or
// https://www.otronic.nl/a-59613972/esp32/esp32-wroom-4mb-devkit-v1-board-met-wifi-bluetooth-en-dual-core-processor/
// with a tft 1.77 arduino 160x128 RGB display
// https://www.arthurwiz.com/software-development/177-inch-tft-lcd-display-with-st7735s-on-arduino-mega-2560
//
// Tools settings:
//  Board ESP32 Dev module
//  Upload Speed: 921600 (or half that on MacOSX!)
//  CPU Frequency: 240Mhz (WiFi/BT)
//  Flash Frequency: 80Mhz
//  Flash Mode: QIO
//  Flash Size: 4MB (32Mb)
//  Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5 SPIFFS)
//  Core Debug Level: None
//  PSRAM: Disabled
//  Port: [the COM port your board has connected to]
//
// Additional Librariries (via Sketch -> library manager):
///   TFT_eSPI
//    Button2
//    ESP32_AnalogWrite

char terminalName[64];

#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h> // for the HTTP error codes.
#include <ESPmDNS.h>
#include "esp_heap_caps.h"

#include <Button2.h>
#include <analogWrite.h>

#include "TelnetSerialStream.h"
#include "global.h"
#include "display.h"
#include "Drill.h"
#include "RPM.h"

#include "ota.h"

Button2 * btn1, * btn2;

int update = false;

const char * version = VERSION;

// Hardcode for Europe (ESP32/Espresifs defaults for CEST seem wrong)./
const char * cestTimezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";


state_t md = BOOT;
board_t BOARD = BOARD_V4;

// Updates the small clock in the top right corner; and
// will reboot the unit early mornings.
//
static void loop_RebootAtMidnight() {
  static unsigned long lst = millis();
  if (millis() - lst < 60 * 1000)
    return;
  lst = millis();

  static unsigned long debug = 0;
  if (millis() - debug > 60 * 60 * 1000) {
    debug = millis();
    time_t now = time(nullptr);
    char * p = ctime(&now);
    p[5 + 11 + 3] = 0;
  }
  time_t now = time(nullptr);
  if (now < 3600)
    return;

#ifdef AUTO_REBOOT_TIME
  static unsigned long reboot_offset = random(3600);
  char * q = ctime(&now);
  now += reboot_offset;
  char * p = ctime(&now);
  p += 11;
  p[5] = 0;

  if (strncmp(p, AUTO_REBOOT_TIME, strlen(AUTO_REBOOT_TIME)) == 0 && millis() > 3600UL) {
    Log.printf("Nightly reboot of %s has come - also to fetch new pricelist and fix any memory leaks.\n", q);
    yield();
    delay(1000);
    yield();
    ESP.restart();
  }
#endif
}

void settupButtons()
{
  if (BOARD != BOARD_V2) {
    // buttons with pullup; wired to GND
    btn1 = new Button2(BUTTON_1, INPUT_PULLUP);
    btn2 = new Button2(BUTTON_2, INPUT_PULLUP);
  } else {
    // buttons wired to VCC.
    btn1 = new Button2(BUTTON_1, INPUT, false, false /* active high, normal low */);
    btn2 = new Button2(BUTTON_2, INPUT, false, false /* active high, normal low */);
  };
  btn1->setPressedHandler([](Button2 & b) {
    if (md == SCREENSAVER) {
      update = true;
      return;
    };
    Debug.println("left");
  });

  btn2->setPressedHandler([](Button2 & b) {
    if (md == SCREENSAVER) {
      update = true;
      return;
    };
    Debug.println("right");
  });
}

void button_loop()
{
  btn1->loop();
  btn2->loop();
}

static unsigned char normal_led_brightness = NORMAL_LED_BRIGHTNESS;
#ifdef LED_1
void setupLEDS()
{
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  digitalWrite(LED_1, (BOARD == BOARD_V2) ? HIGH : LOW);
  digitalWrite(LED_2, (BOARD == BOARD_V2) ? HIGH : LOW);
  delay(50);
  led_loop(md);
}
#else
void setupLEDS() {}
#endif

void led_loop(state_t md) {
#ifdef LED_1
  analogWrite(LED_1, normal_led_brightness);
  analogWrite(LED_2, normal_led_brightness);
#endif
}

static void setupWiFiConnectionOrReboot() {
  while (millis() < 2000) {
    delay(100);
    if (WiFi.isConnected())
      return;
  };
  // not going well - warn the user and try for a bit,
  // while communicating a timeout with a progress bar.
  //
  updateDisplay_startProgressBar("Waiting for Wifi");
  while ( millis() < WIFI_MAX_WAIT) {
    if (WiFi.isConnected())
      return;

    updateDisplay_progressBar((float)millis() / WIFI_MAX_WAIT);
    delay(HTTP_CODE_OK);
  };
  displayForceShowErrorModal((char *)"reboot", "WiFi problem");
  delay(2500);
  ESP.restart();
}

static board_t detectBoard() {
  unsigned char mac[6];
  WiFi.macAddress(mac);

  return BOARD_V4;
};

static const char * board2name(board_t x) {
  switch (x) {
    case BOARD_V4: return "v5: buttons GND, LEDs on HIGH, Screen flipped";
    default: break;
  }
  return "Dunno";
}

void setup()
{
  const char * p = __FILE__;
  if (rindex(p, '/')) p = rindex(p, '/') + 1;

  Serial.begin(115200);
  byte mac[6];
  WiFi.macAddress(mac);
  snprintf(terminalName,  sizeof(terminalName), "%s-%s-%02x%02x%02x", TERMINAL_NAME, VERSION, mac[3], mac[4], mac[5]);

  BOARD = detectBoard();
  Serial.printf( "File:     %s\n", p);
  Serial.println("Version : " VERSION);
  Serial.println("Compiled: " __DATE__ " " __TIME__);
  Serial.printf( "Revision: %s\n", board2name(BOARD));
  Serial.printf( "Heap    :  %d Kb\n",   (512 + heap_caps_get_free_size(MALLOC_CAP_INTERNAL)) / 1024UL);
  Serial.print(  "MacAddr:  ");
  Serial.println(WiFi.macAddress());

#ifdef LED_1
  setupLEDS();
#endif
  setupTFT();
  md = BOOT;
  updateDisplay(BOOT);

  rpm_setup();

  settupButtons();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORK, WIFI_PASSWD);
  WiFi.setHostname(terminalName);

  // setupWiFiConnectionOrReboot();
  setupLog();
  setupOTA();

  // try to get some reliable time; to stop my cert
  // checking code complaining.
  //
  configTzTime(cestTimezone, NTP_POOL);

  Log.printf( "File:     %s\n", p);
  Log.println("Firmware: " TERMINAL_NAME "-" VERSION);
  Log.println("Build:    " __DATE__ " " __TIME__ );
  Log.print(  "Unit:     ");
  Log.println(terminalName);
  Log.printf( "Revision: %s\n", board2name(BOARD));
  Log.print(  "MacAddr:  ");
  Log.println(WiFi.macAddress());
  Log.print(  "IP:       ");
  Log.println(WiFi.localIP());
  Log.printf("Heap:     %d Kb\n",   (512 + heap_caps_get_free_size(MALLOC_CAP_INTERNAL)) / 1024UL);
  Serial.println();

  double rpm = 1000.;
  Log.printf("Material %s: Surface speed: %f m/sec\n", material_db[2].name, material_db[2].recc);
  double diam = rpm2diameter_mm(&(material_db[2]), rpm);
  Log.printf("RPM %f <- DIAM: %f\n", rpm, diam);
  rpm = diameter_mm2rpm(&(material_db[2]), diam),
  Log.printf("RPM %f -> DIAM: %f\n", rpm, diam);

  Log.println("Starting loop");
  md = RUN;
}

void loop()
{
  static unsigned long lastchange = -1;
  static state_t laststate = WAITING_FOR_NTP;
  unsigned int extra_show_delay = 30;

  log_loop();
  loop_RebootAtMidnight();
  ota_loop();
  button_loop();

#ifdef LED_1
  led_loop(md);
#endif

  switch (md) {
    case WAITING_FOR_NTP:
      if (lastchange == -1)
        updateDisplay_progressText("waiting for time");
      if (time(nullptr) > 3600)
        md = RUN;
      break;
    case SCREENSAVER:
      if (update) {
        Log.println("Wakeup");
        setTFTPower(true);
        md = RUN;
        lastchange = millis(); // prevent jump to default straight after wakeup.
      };
      break;
    case RUN:
      if (millis() - lastchange > SCREENSAVER_TIMEOUT) {
        md = SCREENSAVER;
        Log.println("Enabling screensaver (from enter)");
        setTFTPower(false);
        return;
      };
      update = true;
      break;
  };

  if (md != laststate) {
    laststate = md;
    lastchange = millis();
    update = true;
  }

  // generic time/error out if something takes longer than 1- seconds and we are in a state
  // where one does not expect this.
  //
  if ((millis() - lastchange > 10 * 1000 && md > RUN )) {
    if (md == OK_OR_CANCEL) {
      displayForceShowModal("canceling", NULL);
      md = RUN;
    }
    else {
      displayForceShowErrorModal("Timeout", NULL);
    };
    update = true;
  };

  if (update) {
    update = false;
    updateDisplay(md);
  } else {
    updateClock(false);
  };
  delay(extra_show_delay);
}
