#include "global.h"
#include "display.h"
#include "log.h"
#include "Drill.h"
#include "RPM.h"

#include "NotoSansMedium8.h"
#define AA_FONT_TINY  NotoSansMedium8

#include "NotoSansMedium12.h"
#define AA_FONT_SMALL NotoSansMedium12

#include "NotoSansBold15.h"
#define AA_FONT_MEDIUM NotoSansBold15

#include "NotoSansMedium20.h"
#define AA_FONT_LARGE NotoSanMedium20

#include "NotoSansBold36.h"
#define AA_FONT_HUGE  NotoSansBold36

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#include "bmp.c"

#define HIGHT_TOP_BAR (32)
#define SPRITE_PAD_X (32)

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);
TFT_eSprite spr = TFT_eSprite(&tft);

void setupTFT() {
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, (!TFT_BACKLIGHT_ON));
#endif
  tft.init();
  tft.setRotation((BOARD == BOARD_V4) ? TFT_ROTATION - 2 : TFT_ROTATION);
#ifndef _H_BLUEA160x128
  tft.setSwapBytes(true);
#endif
  spr.createSprite(SPRITE_PAD_X +  tft.width() + SPRITE_PAD_X, tft.height() - HIGHT_TOP_BAR);
  setTFTPower(true);
}

void wifiIcon(int32_t x, int32_t  y) {
  float ss = WiFi.RSSI();
  if (!WiFi.isConnected() || ss == 0) {
    tft.drawTriangle(x, y + 6, x + 10, y, x + 10, y + 6, TFT_RED);
    return;
  };

  // Range is from -80 to -10 or so. Above 60 is ok.
  ss = 5 * (75. + ss) / 30;

  tft.fillRect(x, y, 10, 6, TFT_BLACK);
  for (int s = 0; s < 5; s++) {
    int32_t h = (s + 1 <  ss) ? (s + 2) : 1;
    tft.fillRect(x + s * 2, y + 6 - h, 1, h,  (s + 1 <  ss) ? TFT_WHITE : TFT_YELLOW);
  };
};

static void showLogo() {
  tft.pushImage(
    (tft.width() - msl_logo_map_width) / 2, 0, // (tft.height() - msl_logo_map_width) ,
    msl_logo_map_width, msl_logo_map_width,
    (uint16_t *)  msl_logo_map);
}

static double nicestep(double v) {
  int sign = v < 0 ? -1 : 1;
  v = fabs(v);
  int m = log10(v);
  v *= pow(10, -m);

  if (v < 1.25)
    v = 1;
  else if (v < 3.75)
    v = 2;
  else if (v < 7.5)
    v = 5;
  else
    v = 10;

  return v * pow(10, m - 1);
}

static void drawScale(material_entry *entry, double rpm) {
#define RANGE (250)
#define RSTEP_TICK (5)
#define RSTEP_MINOR (25)
#define RSTEP_MAJOR (50)

  const int32_t width =  spr.width(), height =  spr.height();
  const uint32_t mid = width / 2;
  const double SX = 0.8 * width / RANGE;

  char label[6];
  int m = ((int)rpm / 50) * 50;
  uint16_t mx = mid - (rpm - m) * SX;

  spr.fillRect(0, 0, width, height, TFT_BLACK); // fillScreen has a bug on rotated screens?!

  // red line in the middle
  spr.drawFastVLine(mid, 0, 30, TFT_RED);

  // Actual RPMs mid center
  spr.loadFont(AA_FONT_HUGE);
  spr.setTextDatum(TC_DATUM);
  snprintf(label, sizeof(label), "%d", (int)rpm);
  spr.drawString(label, width / 2, height - 1);

  spr.loadFont(AA_FONT_MEDIUM);
  spr.setTextDatum(BC_DATUM);
  spr.drawString(entry->name, width / 2, height - 36);

  spr.loadFont(AA_FONT_SMALL);
  spr.setTextDatum(BC_DATUM);
  double minr = rpm - RANGE / 2;
  float diam_max = (int)(rpm2diameter_mm(entry, minr) + 0.5);

  double maxr = rpm + RANGE / 2;
  float diam_min = (int)(rpm2diameter_mm(entry, maxr) - 0.5);

  double rstep = 0.5; // nicestep((diam_max - diam_min));

  diam_min = ((int)(diam_min / 0.5) + 1) * 0.5;
  diam_max = ((int)(diam_max / 0.5) - 1) * 0.5;

  int32_t lx = 0;
  for (float diam = diam_min; diam < diam_max; diam += rstep) {
    double drpm = diameter_mm2rpm(entry, 1.*diam);
    int32_t x = mx + (drpm - rpm) * SX;

    if (x > width - 10)
      continue;

    if (x < 10)
      continue;

    if (fabs(lx - x) < 35)
      continue;
    lx = x;

    if (diam == (int)diam)
      snprintf(label, sizeof(label), "%.0f", diam);
    else
      snprintf(label, sizeof(label), "%.1f", diam);

    spr.drawString(label, x, 38);
  };

  for (int r = -RANGE; r <= RANGE; r += RSTEP_TICK)
  {
    int32_t R = r + m;
    int32_t x = mx + r * SX;
    if (x < 5)
      continue;
    if (x > width - 5)
      break;

    int32_t h = 4;
    int32_t c = TFT_DARKGREY;

    if (R % RSTEP_MINOR == 0) {
      h = 8;
      c = TFT_LIGHTGREY;
    };
    if (R % RSTEP_MAJOR == 0) {
      h = 12;
      c = TFT_WHITE;
    };
    spr.drawFastVLine(x, 12, h, c);

    if (R % RSTEP_MAJOR == 0) {
      snprintf(label, sizeof(label), "%d", R);
      spr.drawString(label, x, 0);
    };
  }
}

static unsigned short lastw = -1;
void  updateDisplay_startProgressBar(const char *str) {
  tft.fillScreen(TFT_BLACK);
  showLogo();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.loadFont(AA_FONT_LARGE);
  tft.drawString(str, tft.width() / 2, tft.height() / 2 - 10);
  lastw = -10;
  tft.drawRect(20, tft.height() - 40, tft.width() - 40, 20, TFT_WHITE);
  updateDisplay_progressBar(0.0);
};

void updateDisplay_progressBar(float p)
{ unsigned short l = tft.width() - 48 - 4;
  unsigned short w = l * p;
  if (w == lastw) return;
  tft.fillRect(20 + 2, tft.height() - 40 + 2, w, 20 - 4, TFT_GREEN);
  lastw = w;
};

void updateDisplay(state_t md)
{
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  switch (md) {
    case BOOT:
      tft.fillScreen(TFT_BLACK);
      showLogo();
      tft.loadFont(AA_FONT_LARGE);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString("MSL Drill Speed", tft.width() / 2, tft.height() / 2 - 10);

      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.loadFont(AA_FONT_SMALL);
      tft.drawString(version, tft.width() / 2, tft.height() / 2 + 26);
      tft.drawString(__DATE__, tft.width() / 2, tft.height() / 2 + 42);
      tft.drawString(__TIME__, tft.width() / 2, tft.height() / 2 + 60);
      break;
    case WAITING_FOR_NTP:
      tft.fillScreen(TFT_BLACK);
      showLogo();
      tft.loadFont(AA_FONT_LARGE);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString("get time...", tft.width() / 2, tft.height() / 2 - 10);
      break;
    case RUN:
      static int rpm = 938;
      if (rpm == 938)
        tft.fillScreen(TFT_BLACK);
      drawScale(&(material_db[2]), rpm_loop());
      rpm += ((esp_random() & 0xFF) - 255.0 / 2) * 0.05;
      spr.pushSprite(-SPRITE_PAD_X, HIGHT_TOP_BAR);
      break;
  }
  updateClock(true);
}

void updateClock(bool force) {
  static time_t last_now = 0;
  time_t now = time(nullptr);

  if (!force) {
    // only show the lock post NTP sync.
    if (now < 1000)
      return;

    if (last_now == now || now % 60 != 0 || now - last_now < 60)
      return;
  };
  last_now = now;

  char * p = ctime(&now);
  p += 11;
  p[5] = 0; // or use 8 to also show seconds.

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.loadFont(AA_FONT_TINY);
  int padding = tft.textWidth(p, -1);
  tft.setTextPadding(padding);

  tft.drawString(p, tft.width(), 0);

  wifiIcon(0, 0);
  return;
  tft.setTextDatum(TL_DATUM);
  char str[128];
  snprintf(str, sizeof(str), "%d Kb %d dBm", (512 + heap_caps_get_free_size(MALLOC_CAP_INTERNAL)) / 1024UL, (int)WiFi.RSSI());
  tft.drawString(str, 0, 0);
};

void updateDisplay_progressText(const char * str) {
  Log.println(str);
  // updateDisplay();
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.loadFont(AA_FONT_MEDIUM);
  int padding = tft.textWidth(str, -1);
  tft.setTextPadding(padding);

  tft.drawString(str, tft.width() / 2,  tft.height() - 20);
}

void updateDisplay_warningText(const char * str) {
  Log.println(str);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  showLogo();
  tft.setTextDatum(MC_DATUM);
  tft.loadFont(AA_FONT_MEDIUM);
  tft.drawString(str, tft.width() / 2,  tft.height() - 20);
}

void displayForceShowErrorModal(const char * str, const char * substr) {
  tft.fillScreen(TFT_BLACK);
  updateClock(true);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.loadFont(AA_FONT_LARGE);
  tft.drawString("ERROR", tft.width() / 2, tft.height() / 2 - 22);
  tft.loadFont(AA_FONT_SMALL);
  tft.drawString(str, tft.width() / 2, tft.height() / 2 + 2);
  tft.drawString((substr && strlen(substr)) ? substr : "resetting...", tft.width() / 2, tft.height() / 2 +  32);
  Log.printf("Error: %s -- resetting\n", str);
  delay(1500);
}

void displayForceShow(const char * str, const char * substr) {
  tft.fillScreen(TFT_BLACK);
  showLogo();
  updateClock(true);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  if (str) {
    tft.loadFont(AA_FONT_LARGE);
    tft.drawString(str, tft.width() / 2, tft.height() / 2 + 2);
  };
  if (substr) {
    tft.loadFont(AA_FONT_SMALL);
    tft.drawString(substr, tft.width() / 2, tft.height() / 2 - 20);
  };
}

void displayForceShowModal(const char * str, const char * substr) {
  displayForceShow(str, substr);
  delay(1500);
}

void setTFTPower(bool onoff) {
  Log.println(onoff ? "Powering display on" : "Powering display off");

#ifdef  TFT_BL
  if (!onoff) digitalWrite(TFT_BL, onoff ? TFT_BACKLIGHT_ON : (!TFT_BACKLIGHT_ON));
#endif

#ifdef ST7735_DISPON
  tft.writecommand(onoff ? ST7735_DISPON : ST7735_DISPOFF);
#else
#ifdef ST7789_DISPON
  tft.writecommand(onoff ? ST7789_DISPON : ST7789_DISPOFF);
#else
#error "No onoff driver for this TFT screen"
#endif
#endif

  delay(100);
#ifdef  TFT_BL
  if (onoff) digitalWrite(TFT_BL, onoff ? TFT_BACKLIGHT_ON : (!TFT_BACKLIGHT_ON));
#endif
}
