#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>
#include "RTClib.h"
#include "LowPower.h"
#include <avr/sleep.h>
#include <avr/power.h>
//#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
//==============================================================================
//========================= CONFIGURACIONES de SOFTWARE ========================

const uint8_t DuracionArchivo = 2;       //Minutos que dura cada archivo de audio a grabar
const uint16_t DuracionProtocolo = 10;  //Minutos de duración del protocolo
// unsigned int Descanso = 1200;            //Minutos de descanso entre protocolos (1200 min = 8548 ciclos de 8.423 seg)

//const uint8_t SilencioCantos = 15;     //Segundos de silencio entre cantos consecutivos  (LO QUEREMOS RANDOMIZAR, ELEGIDO MÁS ADELANTE)
const uint16_t FreqMuestreo = 22000;     //Frecuencia de muestreo (hasta 25 kHz)
//==============================================================================
//========================= CONFIGURACIONES de HARDWARE ========================
//uint8_t RstPin = 3;
const uint8_t LED_Work = 4;
const uint8_t LED_Error = 5;

#define SD_ChipSelectPin 10
#define MIC A0
TMRpcm audio;
SoftwareSerial mySoftwareSerial(6, 7);
DFRobotDFPlayerMini myDFPlayer;
//==============================================================================
char filename[] = "00000000.wav";

volatile boolean Siguiente = true;       //para ir al próximo fichero
volatile boolean ControladorFinal = false;

unsigned long tiempoFichero;
unsigned long SaltoFichero = DuracionArchivo * 60000;
unsigned long TiempoCincoMin;

volatile boolean Reproducir = false;
volatile boolean CincoMin = false;


//unsigned long Silencio = SilencioCantos * 1000;
long InterRandom;

unsigned long previousMillis = 0;
uint8_t CantidadArchivos = DuracionProtocolo / DuracionArchivo;
//unsigned long Dormir = Descanso * 60000;
// unsigned int kDescansoProtocolo = Descanso * 60 / 8.423 ;
//====================== FUNCIONES PARA FUNCIONAMIENTO =========================
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
  digitalWrite(LED_Work, LOW);
  for (int j = 1; j < 6; j++) {
    digitalWrite(LED_Error, HIGH);
    delay(300);
    digitalWrite(LED_Error, LOW);
    delay(500);
  }
  //pinMode(RstPin,  OUTPUT);
  //digitalWrite(RstPin, LOW);
}
//====================== SET UP DEL MODULO =====================================
void setup() {
  //Serial.begin(9600);

  randomSeed(analogRead(A1));
  
  rtc.begin();
  pinMode(LED_Work, OUTPUT);
  pinMode(LED_Error, OUTPUT);
  pinMode(MIC, INPUT);
//  Serial.println(kDescansoProtocolo);
  audio.CSPin = SD_ChipSelectPin;
  if (!SD.begin(SD_ChipSelectPin)) LedError();

  if (rtc.lostPower()) {
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.adjust(DateTime(2023, 6, 5, 17, 10, 0));
  }

  mySoftwareSerial.begin(9600);
  delay(1000);
  if (!myDFPlayer.begin(mySoftwareSerial)) LedError();
  delay(3000);
  myDFPlayer.volume(30); // de 0 (min) a 30 (max)
  myDFPlayer.playFolder(15, 1);
  delay(3000);
  InterRandom = random(10, 21)*1000 ;
}
//====================== LOOP GRABACION REPRODUCCION ===========================
void loop() {
  DateTime now = rtc.now();

  if (Siguiente) {
//    Serial.println("Siguiente");
    delay(2000);
    if (!SD.begin(SD_ChipSelectPin)) LedError();
    getFileName();
    SdFile::dateTimeCallback(dateTime);
    audio.startRecording(filename, FreqMuestreo, MIC);
    digitalWrite(LED_Work, HIGH);
    Siguiente = false;
    ControladorFinal = true;
    tiempoFichero = millis();
    TiempoCincoMin = millis();
    Reproducir = true;
  }

  if ((millis() - tiempoFichero > SaltoFichero) && ControladorFinal) {
    digitalWrite(LED_Work, LOW);
    audio.stopRecording(filename);
    CantidadArchivos--;
    Siguiente = true;
  }

  if ( ((millis() - TiempoCincoMin) > 2*60000) &&  ((millis() - TiempoCincoMin) < 4*60000) && Reproducir) {
    CincoMin = true;
  }
  if ( ((millis() - TiempoCincoMin) > 4*60000) && Reproducir) {
    CincoMin = false;
    TiempoCincoMin = millis();
  }
  
  if (Reproducir && CincoMin) {
    unsigned long currentMillis = millis();
    //myDFPlayer.volume(30);
    
    if ((currentMillis - previousMillis >= InterRandom)) {       // Silencio)) {
      previousMillis = currentMillis;
      InterRandom = random(10, 21)*1000 ;
//      Serial.println("Reproducir");
      myDFPlayer.playFolder(15, 1);
    }
  }

  if (CantidadArchivos == 0  && ControladorFinal) {
    audio.stopRecording(filename);
    digitalWrite(LED_Work, LOW);
    Siguiente = false;
    ControladorFinal = false;
    Reproducir = false;
    CantidadArchivos = DuracionProtocolo / DuracionArchivo;
//    Serial.println("Voy a dormir");
    //delay(Dormir);
    delay(500);
    for (unsigned int k = 0; k < 70; k++) {       //425--> 1 hora de sueño. 
      LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                    SPI_OFF, USART0_OFF, TWI_OFF);
    }
    delay(500);
//    Serial.println("--------------------------");
    Siguiente = true;
  }
}
