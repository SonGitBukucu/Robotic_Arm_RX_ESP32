// MUSTAFA ALPER KAYA
// KODDA PİN DEĞERLERİ HER AN DEĞİŞİKLİĞE UĞRAYABİLİR.

#include <Arduino.h>
#include <SPI.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans24pt7b.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#include <RF24.h>
#include <nRF24L01.h>

#include <SD.h>
#include <ESP32Servo.h>

#define NRF24CE   4
#define NRF24CSN  5 //PLACEHOLDER

// HSPI (SD Kart için özel pinler)
// SCK: 13, MISO: 34, MOSI: 33, CS: 25
#define SD_SCK  13
#define SD_MISO 34
#define SD_MOSI 33
#define SD_CS   25
SPIClass hspi(HSPI);

#define servoEnable 15 //servoların çalışması için bu pinin HIGH olması lazım
#define servoPanPin   16
#define servoTiltPin   17
#define servoBilekPin   18
#define servoBasPin   19
#define servoIsaretPin   32
#define servoOrtaPin   25
#define servoYuzukPin   26
#define servoSercePin   27

bool arm = false;
bool serbest = false;
bool kayit = false;
bool playback = false;

#define playbackArti 35
#define playbackEksi 36 //PLACEHOLDER
#define kolModu 34


TaskHandle_t iletisim;
TaskHandle_t sdKart;

int sayac0 = 0;
int sayac1 = 0;

void iletisimCode(void * parameter);
void sdKartCode(void * parameter);

Servo servoPan;
Servo servoTilt;
Servo servoBilek;
Servo servoBas;
Servo servoIsaret;
Servo servoOrta;
Servo servoYuzuk;
Servo servoSerce;

unsigned long basarili = 0;
unsigned int failsafeAralik = 700; // fail-safe devreye girmesi için gereken süre.

short kanal[8];

const byte nrf24kod[5] = {'r','o','b','o','t'}; 
RF24 radio(NRF24CE, NRF24CSN);

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  pinMode(servoEnable, OUTPUT);
  pinMode(playbackArti, INPUT);
  pinMode(playbackEksi, INPUT);
  pinMode(kolModu, INPUT);

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
        servoPan.attach(servoPanPin);
        servoTilt.attach(servoTiltPin);
        servoBilek.attach(servoBilekPin);
        servoBas.attach(servoBasPin);
        servoIsaret.attach(servoIsaretPin);               
        servoOrta.attach(servoOrtaPin);
        servoYuzuk.attach(servoYuzukPin);
        servoSerce.attach(servoSercePin);
        digitalWrite(servoEnable, HIGH);
        arm = true;
      }

      basarili = millis();
      radio.read(&kanal,sizeof(kanal));
      for (int i = 0; i < sizeof(kanal); i++) { //ne olur ne olmaz koruması
        kanal[i] = constrain(kanal[i], 1000, 2000);
      }
  }
  if (arm == true && millis() - basarili >= failsafeAralik) {
    failSafe();
  }

  vTaskDelay(1); //şimdilik dursun, daha sonra kaldırınca ne olur ona bakacağım
  }
}

void sdKartCode( void * parameter) {
  Serial.println(xPortGetCoreID());
  for(;;) {
    int rawMod = analogRead(kolModu);
    if (rawMod < 100) { // SERBEST MOD
      if (serbest == false) {
        display.clearDisplay();

        // Cursor ayarlaması gerekebilir
        display.setFont(&FreeSans24pt7b);
        display.setTextSize(1);
        display.setCursor(0, 49);
        display.print("SERBEST");
        //display.setFont();
        display.display();
        serbest = true;
      }
      
      servoPan.writeMicroseconds    (kanal[0]);
      servoTilt.writeMicroseconds   (kanal[1]);
      servoBilek.writeMicroseconds  (kanal[2]);
      servoBas.writeMicroseconds    (kanal[3]);
      servoIsaret.writeMicroseconds (kanal[4]);
      servoOrta.writeMicroseconds   (kanal[5]);
      servoYuzuk.writeMicroseconds  (kanal[6]);
      servoSerce.writeMicroseconds  (kanal[7]);
    }
    
    else if (rawMod > 3995) { // PLAYBACK MOD DOSYA İSMİ
      if (playback == false) {
        display.clearDisplay();
        // Cursor ayarlaması gerekebilir
        display.setFont(&FreeSans24pt7b);
        display.setTextSize(1);
        display.setCursor(0, 49);
        display.print("PLAYBACK");
        //display.setFont();
        display.display();
        playback = true;
      }
      //PLAYBACK GERİ KALANI
    }
    
    else { // KAYIT MOD
      if (kayit == false) {
        display.clearDisplay();
            
        // Labels at the top
        display.setTextSize(1);
        //display.setCursor(15, 8);
        display.print("KAYIT");
            
        // Values centered at the bottom
        display.setFont(&FreeSans24pt7b);
        display.setTextSize(1);
        display.setCursor(0, 49); // left quarter for duty
            
        display.print("DOSYA");
        display.display();
        kayit = true;
      }
      //KAYIT GERİ KALANI
    }
  }
}

void failSafe() {
  kanal[0] = 1500;
  kanal[1] = 1500;
  kanal[2] = 1500;
  kanal[3] = 1500;
  kanal[4] = 1500;
  kanal[5] = 1500;
  kanal[6] = 1500;
  kanal[7] = 1500;

  servoPan.writeMicroseconds    (kanal[0]);
  servoTilt.writeMicroseconds   (kanal[1]);
  servoBilek.writeMicroseconds  (kanal[2]);
  servoBas.writeMicroseconds    (kanal[3]);
  servoIsaret.writeMicroseconds (kanal[4]);
  servoOrta.writeMicroseconds   (kanal[5]);
  servoYuzuk.writeMicroseconds  (kanal[6]);
  servoSerce.writeMicroseconds  (kanal[7]);

  while (millis() - basarili >= failsafeAralik) {
    if (radio.available()) {
      return;
    }  
  }
}