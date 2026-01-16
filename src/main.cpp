#include <Arduino.h>
#include <SPI.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <RF24.h>
#include <nRF24L01.h>
#include <SD.h>
#include <ESP32Servo.h>

#define NRF24CE   4
#define NRF24CSN  5

#define servoEnable //servoların çalışması için bu pinin HIGH olması lazım
#define servo1Pin
#define servo2Pin
#define servo3Pin
#define servo4Pin
#define servo5Pin
#define servo6Pin
#define servo7Pin
#define servo8Pin

#define playbackArti
#define playbackEksi
#define kolModu

TaskHandle_t iletisim;
TaskHandle_t sdKart;

int sayac0 = 0;
int sayac1 = 0;

void iletisimCode(void * parameter);
void sdKartCode(void * parameter);

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
Servo servo5;
Servo servo6;
Servo servo7;
Servo servo8;

const byte nrf24kod[5] = {'r','o','b','o','t'}; 
RF24 radio(NRF24CE, NRF24CSN);

void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    iletisimCode, /* Function to implement the task */
    "Task1", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    1,  /* Priority of the task */
    &iletisim,  /* Task handle. */
    1); /* Core where the task should run */
  delay(500);
  xTaskCreatePinnedToCore(
    sdKartCode, /* Function to implement the task */
    "Task1", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    1,  /* Priority of the task */
    &sdKart,  /* Task handle. */
    0); /* Core where the task should run */

  radio.begin();
  radio.openReadingPipe(1,nrf24kod);
  radio.setChannel(76);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX); // güç çıkışı yüksekten düşüğe: MAX | HIGH | LOW | MIN
  radio.startListening();
}

void loop() {

}

void iletisimCode( void * parameter) {
  Serial.println(xPortGetCoreID());
  for(;;) {
    Serial.println(sayac0++);
    delay(1000);
  }
}

void sdKartCode( void * parameter) {
  Serial.println(xPortGetCoreID());
  for(;;) {
    Serial.println(sayac1++);
    delay(1000);
  }
}
