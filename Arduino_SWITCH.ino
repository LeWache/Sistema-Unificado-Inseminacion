#include <SD.h>
//#include <SPI.h>
#include <TMRpcm.h>
#include "RTClib.h"
#include "LowPower.h"
#include <avr/sleep.h>
#include <avr/power.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

//==================== VARIABLES CONFIGURABLES ==================================================
const char* Inicio_grabacion[] = {6, 1};              // Hora de descanso hasta el día siguiente
const char* Final_grabacion[] = {9, 1};               // Hora de descanso hasta el día siguiente

const uint16_t FreqMuestreo = 22000;                    // frecuencia de muestreo (hasta 25 kHz)
const uint8_t DuracionArchivo = 2;                      // minutos que dura cada archivo de audio a grabar

const uint8_t MenorTiempoDeSilencio = 50;              // segundos de silencio entre cantos
const uint8_t MayorTiempoDeSilencio = 70;              // segundos de silencio entre cantos
//==================== CONEXION PINES ==========================================================
const uint8_t RstPin = 3;
const uint8_t LED_Funciona = 5;     //ORIGINAL 4, PROTOBOARD 5
const uint8_t LED_Error = 4;        //ORIGINAL 5, PROTOBOARD 4
const uint8_t Acceso_SD = 8;        //PIN QUE DA ENABLE AL SWITCH
const uint8_t Despierto_ESP = 9;    //PIN QUE AVISA AL ESP QUE LE TOCA TRABAJAR

#define SD_ChipSelectPin 10
#define MIC A0

TMRpcm audio;
SoftwareSerial mySoftwareSerial(6, 7);
DFRobotDFPlayerMini myDFPlayer;
//==================== VARIABLES NO CONFIGURABLES ===============================================
char filename[] = "00000000.wav";
const uint8_t HoraFinal = Final_grabacion[0];
const uint8_t MinFinal = Final_grabacion[1];
const uint8_t HoraInicial = Inicio_grabacion[0];
const uint8_t MinInicial = Inicio_grabacion[1];

volatile boolean Dormir = false;                        //arranca despierto
volatile boolean Siguiente = true;                      //para ir al próximo fichero

unsigned long tiempoFichero;
unsigned long TiempoDeSilencio;
unsigned long TiempoUltimoCanto = 0;
const unsigned long SaltoFichero = DuracionArchivo * 60000; //PRUEBA

//====================== FUNCIONES RECURRENTES =================================
RTC_DS3231 rtc;
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();
  *date = FAT_DATE(now.year(), now.month(),  now.day());
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}
//==============================================================================
void getFileName() {
  DateTime now = rtc.now();
  sprintf(filename, "%02d%02d%02d%02d.wav", now.day(), now.hour(), now.minute(), now.second());
}
//==============================================================================
void LedError() {
  digitalWrite(LED_Error, LOW);
  digitalWrite(LED_Funciona, LOW);
  for (int j = 1; j < 6; j++) {
    digitalWrite(LED_Error, HIGH);
    delay(100);
    digitalWrite(LED_Error, LOW);
    delay(500);
  }
  pinMode(RstPin,  OUTPUT);
  digitalWrite(RstPin, LOW);
}
//==============================================================================
void Trabaja_Arduino() {
  digitalWrite(LED_Error, LOW);
  delay(100);
  digitalWrite(Despierto_ESP, LOW);
  delay(1000);
  digitalWrite(Acceso_SD, HIGH);
  delay(1000);
}
void Trabaja_ESP() {
  digitalWrite(LED_Error, HIGH);
  delay(100);
  digitalWrite(Acceso_SD, LOW);
  delay(1000);
  digitalWrite(Despierto_ESP, HIGH);
  delay(1000);
}
//====================== SET-UP DEL MODULO =====================================
void setup() {
  rtc.begin();
  pinMode(LED_Funciona, OUTPUT);
  pinMode(LED_Error, OUTPUT);
  pinMode(MIC, INPUT);
  pinMode(Acceso_SD, OUTPUT);
  pinMode(Despierto_ESP, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(50);
  digitalWrite(Acceso_SD, HIGH);
  digitalWrite(Despierto_ESP, LOW);
  delay(1000);

  audio.CSPin = SD_ChipSelectPin;
  if (!SD.begin(SD_ChipSelectPin)) {
    LedError();
  }
  else {
    for (int j = 1; j < 6; j++) {
      digitalWrite(LED_Error, LOW);
      digitalWrite(LED_Funciona, HIGH);
      delay(300);
      digitalWrite(LED_Funciona, LOW);
      delay(500);
    }
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(2040, 11, 11, 11, 11, 11));         //CHEQUEAR SI SE PUEDE CAMBIAR ESTO
  }
  mySoftwareSerial.begin(9600);
  delay(100);
  if (!myDFPlayer.begin(mySoftwareSerial)) LedError();
  delay(300);
  myDFPlayer.volume(30);                                   // min: 0; max: 30
  myDFPlayer.playFolder(15, 1);

  TiempoDeSilencio = random( MenorTiempoDeSilencio, MayorTiempoDeSilencio + 1) * 1000;
  DateTime now = rtc.now();
  if (  (now.hour() * 60 + now.minute() <= HoraInicial * 60 + MinInicial) ) {
    Dormir = true;
    Siguiente = false;
    Trabaja_ESP();
  }
  else {
    Trabaja_Arduino();
  }
}
//====================== LOOP GRABACION REPRODUCCION ===========================
void loop() {
  if (Siguiente) {                                                         //CHEQUEA LA HORA, SI CORRESPONDE GRABA Y ENCIENDE LED O SE VA A DORMIR
    Siguiente = false;
    DateTime now = rtc.now();
    if ( ( now.hour() * 60 + now.minute() >= HoraFinal * 60 + MinFinal)) { // || (now.hour() * 60 + now.minute() <= HoraInicial * 60 + MinInicial) ) {
      delay(100);
      Trabaja_ESP();
      delay(2000);
      Dormir = true;
    }
    if (!Dormir) {
      digitalWrite(Acceso_SD, HIGH);
      digitalWrite(Despierto_ESP, LOW);
      if (!SD.begin(SD_ChipSelectPin)) LedError();
      getFileName();
      SdFile::dateTimeCallback(dateTime);
      audio.startRecording(filename, FreqMuestreo, MIC);
      digitalWrite(LED_Funciona, HIGH);
      tiempoFichero = millis();
    }
  }
  if ( millis() - tiempoFichero > SaltoFichero && !Dormir) {               //CORTA GRABACION Y HABILITA SIGUIENTE FICHERO
    digitalWrite(LED_Funciona, LOW);
    audio.stopRecording(filename);
    Siguiente = true;
  }
  if (  (millis() - TiempoUltimoCanto > TiempoDeSilencio) && !Dormir ) {   //CANTA PASADO UN TIEMPO TiempoDeSilencio Y REDEFINE ESTE DE FORMA ALEATORIA ESE TIEMPO
    TiempoDeSilencio = random( MenorTiempoDeSilencio, MayorTiempoDeSilencio + 1 ) * 1000;
    TiempoUltimoCanto = millis();
    myDFPlayer.playFolder(15, 1);
  }
  if (Dormir) {                                                          //SE VA A DORMIR POR INTERVALOS DE 10 MINUTOS (k=75) Y CHEQUEA HORA, SI CORRESPONDE DESPIERTA
    Trabaja_ESP(); //PRUEBA
    for (uint8_t k = 1; k <=75 ; k++) {      //75 = 10 minutos
      LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                    SPI_OFF, USART0_OFF, TWI_OFF);
    }
    DateTime now = rtc.now();
    delay(500);
    if ( (now.hour() * 60 + now.minute() + 1 < HoraFinal * 60 + MinFinal) && (now.hour() * 60 + now.minute() >= HoraInicial * 60 + MinInicial) ) {
      digitalWrite(LED_Funciona, HIGH);
      Trabaja_Arduino();
      delay(2000);
      Dormir = false;
      Siguiente = true;
      digitalWrite(LED_Funciona, LOW);
    }

  }
}
