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
#define servo1Pin   0
#define servo2Pin   2
#define servo3Pin   16
#define servo4Pin   17
#define servo5Pin   25
#define servo6Pin   26
#define servo7Pin   27
#define servo8Pin   33
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

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
Servo servo5;
Servo servo6;
Servo servo7;
Servo servo8;

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
      servo1.attach(servo1Pin);
      servo2.attach(servo2Pin);
      servo3.attach(servo3Pin);
      servo4.attach(servo4Pin);               
      servo5.attach(servo5Pin);
      servo6.attach(servo6Pin);
      servo7.attach(servo7Pin);
      servo8.attach(servo8Pin);
      arm = true;
    }
    digitalWrite(servoEnable, HIGH);
    basarili = millis();
    radio.read(&kanal,sizeof(kanal));
    servo1.writeMicroseconds      (kanal[0]);
    servo2.writeMicroseconds      (kanal[1]);
    servo3.writeMicroseconds      (kanal[2]);
    servo4.writeMicroseconds      (kanal[3]);
    servo5.writeMicroseconds      (kanal[4]);
    servo6.writeMicroseconds      (kanal[5]);
    servo7.writeMicroseconds      (kanal[6]);
    servo8.writeMicroseconds      (kanal[7]);
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
