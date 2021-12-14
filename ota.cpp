#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

#include "global.h"
#include "display.h"
#include "ota.h"

void setupOTA() {
  ArduinoOTA.setHostname(terminalName);

#ifdef OTA_HASH
  ArduinoOTA.setPasswordHash(OTA_HASH);
#else
#ifdef OTA_PASSWORD
  ArduinoOTA.setPassword(OTA_PASSWORD);
#endif
#endif

  ArduinoOTA
  .onStart([]() {
    led_loop(BOOT);
    updateDisplay_startProgressBar("firmware update");
  })
  .onEnd([]() {
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    updateDisplay_progressBar(1.0 * progress / total);
  })
  .onError([](ota_error_t error) {
    const char * str;
    if (error == OTA_AUTH_ERROR) str = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) str = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) str = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) str = "Receive Failed";
    else if (error == OTA_END_ERROR) str = "End Failed";
    else str = "Uknown error";
    displayForceShowErrorModal("OTA abort", str);
    delay(2500);
  });
  ArduinoOTA.begin();
};

void ota_loop() {
  ArduinoOTA.handle();
}
