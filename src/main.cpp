// MUSTAFA ALPER KAYA
// KODDA PİN DEĞERLERİ HER AN DEĞİŞİKLİĞE UĞRAYABİLİR.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans24pt7b.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#include <RF24.h>
#include <nRF24L01.h>
#include <SPI.h>
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
SPIClass hspi = SPIClass(HSPI);

#define servoEnable 15 //servoların çalışması için bu pinin HIGH olması lazım
#define servoPanPin   16
#define servoTiltPin   17
#define servoBilekPin   18
#define servoBasPin   19
#define servoIsaretPin   25
#define servoOrtaPin   26
#define servoYuzukPin   27
#define servoSercePin   32

bool arm      = false;

volatile byte currentMode = 0; 
// 0 = SERBEST, 1 = KAYIT, 2 = PLAYBACK

volatile int currentFileIndex = 0;
const int maxFiles = 20;

bool serbest  = false;
bool kayit    = false;
bool playback = false;

#define playbackArti 35
#define playbackEksi 36 //PLACEHOLDER
#define kolModu 34


TaskHandle_t iletisim;
TaskHandle_t sdKart;

void iletisimCode(void * parameter);
void failSafe();
void sdKartCode(void * parameter);
void sdKayit();
void sdPlayback();
void buttonTask(void * parameter);
void showModeAndFile(const char*);

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
  hspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS); // SCK, MISO, MOSI, CS
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
    "iletisim", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    1,  /* Priority of the task */
    &iletisim,  /* Task handle. */
    1); /* Core where the task should run */
  
  delay(500);
  
  xTaskCreatePinnedToCore(
    sdKartCode, /* Function to implement the task */
    "sdKart", /* Name of the task */
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

void iletisimCode(void * parameter) {
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
      for (int i = 0; i < 8; i++) { //ne olur ne olmaz koruması
        kanal[i] = constrain(kanal[i], 1000, 2000);
      }
  }
  if (arm == true && millis() - basarili >= failsafeAralik) {
    failSafe();
  }

  vTaskDelay(1); //şimdilik dursun, daha sonra kaldırınca ne olur ona bakacağım
  }
}

void sdKartCode(void * parameter) {
  Serial.println(xPortGetCoreID());
  for(;;) {
    int rawMod = analogRead(kolModu);
    if (rawMod < 100) { // SERBEST MOD
      if (serbest == false) {
        currentMode = 0;
        showModeAndFile("SERBEST");
        serbest = true;
        playback = false;
        kayit = false;
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
        currentMode = 2;
        showModeAndFile("PLAYBACK");
        serbest = false;
        playback = true;
        kayit = false;
      }
      //PLAYBACK GERİ KALANI
    }
    
    else { // KAYIT MOD
      if (kayit == false) {
        currentMode = 1;
        showModeAndFile("KAYIT");
        serbest = false;
        playback = false;
        kayit = true;
      }
      //KAYIT GERİ KALANI
    }
  }
}

void buttonTask(void * parameter) {
  bool lastUp = true;
  bool lastDown = true;

  for (;;) {
    bool up = digitalRead(playbackArti);
    bool down = digitalRead(playbackEksi);

    if (up == LOW && lastUp == HIGH) {
      currentFileIndex++;
      if (currentFileIndex >= maxFiles) currentFileIndex = 0;
    }

    if (down == LOW && lastDown == HIGH) {
      currentFileIndex--;
      if (currentFileIndex < 0) currentFileIndex = maxFiles - 1;
    }

    lastUp = up;
    lastDown = down;

    vTaskDelay(pdMS_TO_TICKS(150)); // debounce
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

void showModeAndFile(const char *modeText) {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 10);
  display.print(modeText);

  display.setFont(&FreeSans24pt7b);
  display.setCursor(0, 49);

  display.print("HRKT-");
  display.print(currentFileIndex);

  display.display();
}

void handleRecord() {
  static unsigned long lastWrite = 0;

  if (millis() - lastWrite < 20) return; // 50 Hz
  lastWrite = millis();

  char filename[12];
  sprintf(filename, "/F%d.txt", currentFileIndex);

  File file = SD.open(filename, FILE_APPEND);
  if (!file) return;

  for (int i = 0; i < 8; i++) {
    file.print(kanal[i]);
    if (i < 7) file.print(",");
  }
  file.println();

  file.close();
}

void handlePlayback() {
  static File file;
  static bool fileOpen = false;

  if (!fileOpen) {
    char filename[12];
    sprintf(filename, "/HRKT-%d.txt", currentFileIndex);
    file = SD.open(filename);
    if (!file) return;
    fileOpen = true;
  }

  if (!file.available()) {
    file.close();
    fileOpen = false;
    return;
  }

  String line = file.readStringUntil('\n');

  int values[8];
  sscanf(line.c_str(),
         "%d,%d,%d,%d,%d,%d,%d,%d",
         &values[0], &values[1], &values[2], &values[3],
         &values[4], &values[5], &values[6], &values[7]);

  servoPan.writeMicroseconds(values[0]);
  servoTilt.writeMicroseconds(values[1]);
  servoBilek.writeMicroseconds(values[2]);
  servoBas.writeMicroseconds(values[3]);
  servoIsaret.writeMicroseconds(values[4]);
  servoOrta.writeMicroseconds(values[5]);
  servoYuzuk.writeMicroseconds(values[6]);
  servoSerce.writeMicroseconds(values[7]);

  delay(20); // playback speed
}
