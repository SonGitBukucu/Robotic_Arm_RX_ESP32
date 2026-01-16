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
#define NRF24CSN  5 //PLACEHOLDER

#define servoEnable 32 //servoların çalışması için bu pinin HIGH olması lazım
#define servoPanPin   0
#define servoTiltPin   2
#define servoBasPin   16
#define servoIsaretPin   17
#define servoOrtaPin   25
#define servoYuzukPin   26
#define servoSercePin   27
//#define servo8Pin   33
bool arm = false;

#define playbackArti
#define playbackEksi
#define kolModu

TaskHandle_t iletisim;
TaskHandle_t sdKart;

int sayac0 = 0;
int sayac1 = 0;

void iletisimCode(void * parameter);
void sdKartCode(void * parameter);

Servo servoPan;
Servo servoTilt;
Servo servoBas;
Servo servoIsaret;
Servo servoOrta;
Servo servoYuzuk;
Servo servoSerce;
//Servo servo8;

unsigned long basarili = 0;
unsigned int failsafeAralik = 700; // fail-safe devreye girmesi için gereken süre.

short kanal[11];

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
    if (radio.available()) {
      if (arm == false) {
        digitalWrite(servoEnable, HIGH);
        servoPan.attach(servoPanPin);
        servoTilt.attach(servoTiltPin);
        servoBas.attach(servoBasPin);
        servoIsaret.attach(servoIsaretPin);               
        servoOrta.attach(servoOrtaPin);
        servoYuzuk.attach(servoYuzukPin);
        servoSerce.attach(servoSercePin);
        //servo8.attach(servo8Pin);
        arm = true;
      }
      
      basarili = millis();
      radio.read(&kanal,sizeof(kanal));
      servoPan.writeMicroseconds(kanal[0]);
      servoTilt.writeMicroseconds(kanal[1]);
      servoBas.writeMicroseconds(kanal[2]);
      servoIsaret.writeMicroseconds(kanal[3]);
      servoOrta.writeMicroseconds(kanal[4]);
      servoYuzuk.writeMicroseconds(kanal[5]);
      servoSerce.writeMicroseconds(kanal[6]);
      //servo8.writeMicroseconds(kanal[7]);
  }
  if (arm == true && millis() - basarili >= failsafeAralik) {
    digitalWrite(servoEnable, LOW);
  }

  }
}

void sdKartCode( void * parameter) {
  Serial.println(xPortGetCoreID());
  for(;;) {
    Serial.println(sayac1++);
    delay(1000);
  }
}

void failSafe() {
  kanal[0] = 2000;
  kanal[1] = 2000;
  kanal[2] = 1000;
  kanal[3] = 2000;
  kanal[4] = 1500;
  kanal[5] = 1500;
  kanal[6] = 1500;
  kanal[7] = 1500;

  servoPan.writeMicroseconds(kanal[0]);
  servoTilt.writeMicroseconds(kanal[1]);
  servoBas.writeMicroseconds(kanal[2]);
  servoIsaret.writeMicroseconds(kanal[3]);
  servoOrta.writeMicroseconds(kanal[4]);
  servoYuzuk.writeMicroseconds(kanal[5]);
  servoSerce.writeMicroseconds(kanal[6]);
  //servo8.writeMicroseconds(kanal[7]);

  while (millis() - basarili >= failsafeAralik) {
    if (radio.available()) {
      return;
    }  
  }
}
  
