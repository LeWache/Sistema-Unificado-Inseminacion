#include "Tests.h"

#include <SPI.h>
#include <SD.h>

#include "RTClib.h"

#include "DFRobotDFPlayerMini.h"
#include "SoftwareSerial.h"
SoftwareSerial mySoftwareSerial(6, 7); // RX, TX
DFRobotDFPlayerMini myDFPlayer;


void setup() {
  Serial.begin(115200);

  // Primero testeamos la SD:

  Serial.print("Iniciando tarjeta SD...");
  if (!SD.begin(10)) {
    Serial.println("¡Falló el inicio de SD!");
  }
  else {
    Serial.println("Todo ok con la SD.");
    root = SD.open("/");
    printDirectory(root, 0);
    Serial.println("Listo con la SD");
    Serial.println(".");
  }
  //=================================================================================

  Serial.println("Ahora el reloj");
  for (uint8_t i = 0; i < 3; i++) {
    digitalClockDisplay();
    delay(1000);
  }
  Serial.println("Listo el reloj");
  Serial.println(".");
  //=================================================================================

  Serial.println("Vamos con los leds");

  Serial.println("Primero el rojo:");
  Parpadea(5);
  Serial.println("Listo el rojo");

  Serial.println("Ahora el verde:");
  Parpadea(4);
  Serial.println("Listo el verde");
  Serial.println(".");
  //=================================================================================

  Serial.println("Ahora el DF Player:");
  mySoftwareSerial.begin(9600);
  delay(1000);
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Parpadea(5);
    Serial.println("No corrió");
  }
  myDFPlayer.volume(5);
  myDFPlayer.playFolder(15, 1);
}


void loop() {

}
