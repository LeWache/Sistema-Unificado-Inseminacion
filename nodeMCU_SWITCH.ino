#include <Arduino.h>
#include "Funciones.h"
#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>
#include <extras/SDHelper.h>
#include <SPI.h>
#include <SD.h>

File root;
//==============================================================================

void setup() {
  delay(15000);
  pinMode(ESP_Despierto, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(500);
}
//==============================================================================

void loop() {
  Mando_Mail = digitalRead(ESP_Despierto);
  if (!Mando_Mail && !Dormir) {
    Dormir = true;
  }
  if (Mando_Mail && !Dormir) {
    digitalWrite(LED_BUILTIN, LOW);
#if defined(ESP_MAIL_DEFAULT_SD_FS)
    SD_Card_Mounting();
    delay(5000);
#endif
    if (!SD.begin(SS)) {
      for (uint8_t k = 1; k <= 8; k++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
      }
    }
    Cantidad_Archivos = Conteo_Archivos();
    delay(10000);
    digitalWrite(LED_BUILTIN, HIGH);
    if (Cantidad_Archivos == 0) {
      Dormir = true;
    }
    else {
      uint16_t i = 0;
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED && i < 600) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);
        i++;
        if (i == 600) {
          Dormir = true;
        }
      }
    }
    while (Mando_Mail && !Dormir && Cantidad_Archivos != 0) {
      digitalWrite(LED_BUILTIN, LOW);
      Mando_Mail = digitalRead(ESP_Despierto);
      root = SD.open("/");
      Lectura_Directorio(root, 0);
      Mando_Mail = digitalRead(ESP_Despierto);
      Cantidad_Archivos --;
      if (Cantidad_Archivos == 0) {
        Cantidad_Archivos = Conteo_Archivos();
        delay(5000);
      }
    }
  }
  if (Dormir) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    system_deep_sleep_instant( 10 * 60 * 1000 * 1000);
    Mando_Mail = digitalRead(ESP_Despierto);
    if (Mando_Mail) {
      Cantidad_Archivos = Conteo_Archivos();  // comentable
      delay(10000);                           // comentable
      if (Cantidad_Archivos != 0) {           // comentable
        Dormir = false;
      }
    }
  }
}
