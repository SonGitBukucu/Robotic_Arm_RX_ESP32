#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <SD.h>


TaskHandle_t task1;
TaskHandle_t task2;

int sayac0 = 0;
int sayac1 = 0;

void task1code(void * parameter);
void task2code(void * parameter);

void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
      task1code, /* Function to implement the task */
      "Task1", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      1,  /* Priority of the task */
      &task1,  /* Task handle. */
      1); /* Core where the task should run */
  delay(500);
  xTaskCreatePinnedToCore(
    task2code, /* Function to implement the task */
    "Task1", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    1,  /* Priority of the task */
    &task2,  /* Task handle. */
    0); /* Core where the task should run */
}

void loop() {

}

void task1code( void * parameter) {
  Serial.println(xPortGetCoreID());
  for(;;) {
    Serial.println(sayac0++);
    delay(1000);
  }
}

void task2code( void * parameter) {
  Serial.println(xPortGetCoreID());
  for(;;) {
    Serial.println(sayac1++);
    delay(1000);
  }
}
