// MUSTAFA ALPER KAYA
// NRF24L01+ modüllerini kullanan robot kol projesinin "kol" (alıcı) kodu.
// ÖZEL MODLAR İÇİN HEM ÖZEL MOD İSMİNİ enum İÇERİSİNE HEM DE KISALTMASINI getSpecialName() İÇİNE YAZMAYI UNUTMAYIN

/*#####################################################         KANAL SIRALAMASI         #####################################################
  Keyfinize göre açın, kapatın, değiştirin.
  1 = Tilt
  2 = Pan
  3 = Baş Parmak
  4 = Bilek
  5 = Yüzük Parmak
  6 = Orta Parmak
  7 = İşaret Parmak
  8 = Serçe Parmak
######################         DOĞRU ÇALIŞMASI İÇİN LÜTFEN HEM VERİCİDE HEM DE ALICIDA AYNI SIRALAMAYI YAPIN         #######################*/

/* #########################        YAPILACAKLAR       #########################
  TEST Özel modlar için ayrı playback yapma
  TEST Özel modlar için ayrı bir durum
   #########################        YAPILACAKLAR       ######################### */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans12pt7b.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <SPI.h>
#include <SD.h>
#include <ESP32Servo.h>


//######################################                OLED EKRAN                ######################################
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define ekranAdres 0x3C
//######################################                OLED EKRAN                ######################################

//######################################                NRF24                ######################################
#define NRF24_CE   2
#define NRF24_CSN  5
#define NRF24_SCK  18
#define NRF24_MISO 19
#define NRF24_MOSI 23
SPIClass vspi = SPIClass(VSPI);

bool arm      = false;
bool failsafe = false;
bool iletisimVar = false;
#define DEBUG_LED 14

unsigned long basarili = 0;
unsigned int failsafeAralik = 700; // fail-safe devreye girmesi için gereken süre.

short kanal[8];

const byte nrf24kod[5] = {'r','o','b','o','t'}; 
RF24 radio(NRF24_CE, NRF24_CSN);
//######################################                NRF24                ######################################


//######################################                SD KART                ######################################
// HSPI (SD Kart için özel pinler)
// SCK: 13, MISO: 34, MOSI: 33, CS: 4
#define SD_SCK  13
#define SD_MISO 34
#define SD_MOSI 33
#define SD_CS   4
SPIClass hspi = SPIClass(HSPI);
//######################################                SD KART                ######################################

//######################################                KOLUN KENDİSİ                ######################################
#define KareAralik 20 // İki "kare" arasındaki milisaniye farkı. 1000 / KareAralik Sistemin Hz değerini verir.
volatile int currentFileIndex = 0;
const int maxFiles = toplamOzelHareket;
static File recFile;
static bool recordingActive = false;
static unsigned long lastRecWrite = 0;

bool serbest  = false;
bool kayit    = false;
bool playback = false;
bool sdHazir  = false;

#define playbackArti 35
#define playbackEksi 36
#define kolModu 39

volatile byte currentMode = 0; 
// 0 = SERBEST, 1 = KAYIT, 2 = PLAYBACK
//######################################                KOLUN KENDİSİ                ######################################

//######################################                ÖZEL MODLAR                 ######################################
// ÖZEL MOD EKLERSENİZ KISALTMASINI getSpecialName() İÇİNE EKLEMEYİ UNUTMAYIN
#define OzelBaslangic 1000
enum OzelHareketler {
  TAS_KAGIT_MAKAS = OzelBaslangic,
  EL_SALLA,
  toplamOzelHareket, //BUNUN EN SONDAKİ ELEMAN OLDUĞUNDA DİKKAT EDİN
};
//######################################                ÖZEL MODLAR                ######################################

//######################################                BİLDİRİMLER                ######################################
TaskHandle_t iletisim;    //NRF24'ler ile ilgili olan fonksiyonları çalıştırır. Çekirdek 1'de çalışır.
TaskHandle_t sdKart;      //SD kart ile ilgili fonksiyonları çalıştırır.    Çekirdek 0'da çalışır.
TaskHandle_t dugmeler;    //Düğmeler ile ilgili fonksiyonları çalıştırır.  Çekirdek 1'de çalışır.
QueueHandle_t dugmeSira;  //Düğmelere basıldığında duyurU yapar.

void iletisimCode(void * parameter);  //NRF24'lerin arasında iletişim kurmasını sağlayan fonksiyon.
void failSafe();  //NRF24 failsafeAralik (şu an 700ms) kadar boyunca iletişim kuramazsa robot kolu FAIL-SAFE moduna geçiren fonksiyon.
void sdKartCode(void * parameter);  //Robot kolun hangi modda olduğunu belirleyen ve kolun o moda göre davranmasını sağlayan fonksiyon.
void sdKayit(); //KAYIT modunda kayıt yapılmasını sağlayan fonksiyon.
void sdPlayback();  //PLAYBACK modunda playback yapılmasını sağlayan fonksiyon.
void dugmelerCode(void * parameter);  //Düğmelere basılıp basılmadığını kontrol eden fonksiyon.
const char* getSpecialName(int index);  //Özel hareketler için belirlenmiş kısaltmalardan gerekeni seçen fonksiyon.
void showModeAndFile(const char*);  //OLED ekranda robot kolun durumunu yukarda, dosya ismini aşağıda gösteren fonksiyon.
void showText(const char *);  //OLED ekranda bir yazıyı otomatik olarak ortalayıp yazdıran fonksiyon.
void stopRecordingIfNeeded(); //Bir sebepten ötürü kayıtta olunmaması gerekirken hala kayıttaysa kaydı durduran fonksiyon.
//######################################                BİLDİRİMLER                ######################################

//######################################                SERVO                ######################################
#define servoPanPin       16
#define servoTiltPin      17
#define servoBilekPin     12
#define servoBasPin       15
#define servoIsaretPin    25
#define servoOrtaPin      26
#define servoYuzukPin     27
#define servoSercePin     32

Servo servoPan;
Servo servoTilt;
Servo servoBilek;
Servo servoBas;
Servo servoIsaret;
Servo servoOrta;
Servo servoYuzuk;
Servo servoSerce;
//######################################                SERVO                ######################################

//######################################                FONKSİYON TANIMLARI                ######################################

void setup() {
  hspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS); // SCK, MISO, MOSI, CS
  vspi.begin(NRF24_SCK, NRF24_MISO, NRF24_MOSI, NRF24_CSN); // SCK, MISO, MOSI, CS

  //Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    //Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.display();
  display.setTextColor(SSD1306_WHITE);

  if (!SD.begin(SD_CS, hspi, 4000000)) {
    //Serial.println("SD init failed!");
    showText("SD HATA");
    while (1) {};
  }
  sdHazir = true;

  delay(150);

  pinMode(DEBUG_LED, OUTPUT);
  pinMode(playbackArti, INPUT);
  pinMode(playbackEksi, INPUT);
  pinMode(kolModu, INPUT);

  dugmeSira = xQueueCreate(10, sizeof(int));
  xTaskCreatePinnedToCore(
    iletisimCode, // Çalışacak fonksiyon
    "iletisim",   // Görevin ismi
    10000,        // Boyut
    NULL,         // Görev girdi parametresi
    1,            // Görev öncelik sırası
    &iletisim,    // Görev referans ismi
    1);           // Görevin çalışacağı çekirdek
  
  xTaskCreatePinnedToCore(
    sdKartCode, // Çalışacak fonksiyon
    "sdKart",   // Görevin ismi
    10000,      // Boyut
    NULL,       // Görev girdi parametresi
    1,          // Görev öncelik sırası
    &sdKart,    // Görev referans ismi
    0);         // Görevin çalışacağı çekirdek

  xTaskCreatePinnedToCore(
    dugmelerCode, // Çalışacak fonksiyon
    "dugmeler",   // Görevin ismi
    10000,        // Boyut
    NULL,         // Görev girdi parametresi
    1,            // Görev öncelik sırası
    &dugmeler,    // Görev referans ismi
    1);           // Görevin çalışacağı çekirdek

  radio.begin(&vspi);
  radio.openReadingPipe(1,nrf24kod);
  radio.setChannel(76);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX); // güç çıkışı yüksekten düşüğe: MAX | HIGH | LOW | MIN
  radio.startListening();
}

void loop() {

}

void iletisimCode(void * parameter) {
  //Serial.println(xPortGetCoreID());
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
        arm = true;
      }

      if (iletisimVar == false) {
        iletisimVar = true;
        digitalWrite(DEBUG_LED, HIGH);
      }

      basarili = millis();
      radio.read(&kanal,sizeof(kanal));
      for (int i = 0; i < 8; i++) { //ne olur ne olmaz koruması
        kanal[i] = constrain(kanal[i], 544, 2400);
      }
  }
  if (arm == true && millis() - basarili >= failsafeAralik) {
    failSafe();
  }
  vTaskDelay(1); //şimdilik dursun, daha sonra kaldırınca ne olur ona bakacağım
  }
}

void sdKartCode(void * parameter) {
  //Serial.println(xPortGetCoreID());
  for(;;) {
    int evt;
    while (xQueueReceive(dugmeSira, &evt, 0) == pdTRUE) {
      if (currentMode == 1) {
      continue; // kayıtta değişme
      }
      
      currentFileIndex += evt;
    
      if (currentFileIndex >= maxFiles) {
        currentFileIndex = 0;
      }
      if (currentFileIndex < 0) {
        currentFileIndex = maxFiles - 1;
      }
      showModeAndFile(
        currentMode == 2 ? "PLAYBACK" :
        currentMode == 1 ? "KAYIT" :
                           "SERBEST"
      );
    }

    int rawMod = analogRead(kolModu);
    if (rawMod < 1000) { // SERBEST MOD
      stopRecordingIfNeeded();

      if (serbest == false) {
        currentMode = 0;
        showModeAndFile("SERBEST");

        serbest = true;
        playback = false;
        kayit = false;
      }

    }
    
    else if (rawMod > 3096) { // PLAYBACK MOD DOSYA İSMİ
      stopRecordingIfNeeded();

      if (playback == false) {
        currentMode = 2;
        showModeAndFile("PLAYBACK");

        serbest = false;
        playback = true;
        kayit = false;
      }
    }
    
    else { // KAYIT MOD
      if (!kayit) {
        stopRecordingIfNeeded(); // güvenlik
        currentMode = 1;
        showModeAndFile("KAYIT");
      
        serbest = false;
        playback = false;
        kayit = true;
      }
    
    }
    
    if (currentMode == 0) {
            
      servoPan.writeMicroseconds    (kanal[0]);
      servoTilt.writeMicroseconds   (kanal[1]);
      servoBilek.writeMicroseconds  (kanal[2]);
      servoBas.writeMicroseconds    (kanal[3]);
      servoIsaret.writeMicroseconds (kanal[4]);
      servoOrta.writeMicroseconds   (kanal[5]);
      servoYuzuk.writeMicroseconds  (kanal[6]);
      servoSerce.writeMicroseconds  (kanal[7]);
    }

    if (currentMode == 1) {
        sdKayit();
    }

    else if (currentMode == 2) {
        sdPlayback();
    }

    vTaskDelay(1);
  }
}

void dugmelerCode(void * parameter) {
  bool lastUp = HIGH;
  bool lastDown = HIGH;

  unsigned long upPressedAt = 0;
  unsigned long downPressedAt = 0;

  for (;;) {
    bool up = digitalRead(playbackArti);
    bool down = digitalRead(playbackEksi);

    unsigned long now = millis();

    // YUKARI
    if (up == LOW && lastUp == HIGH) {
      upPressedAt = now;                 // yeni basıldı
      int evt = +1;
      xQueueSend(dugmeSira, &evt, 0);
    }

    if (up == LOW && lastUp == LOW) {
      unsigned long held = now - upPressedAt;

      int step = 0;
      if (held > 1250) step = +10;
      else if (held > 750) step = +5;
      else if (held > 250) step = +1;

      if (step != 0) {
        xQueueSend(dugmeSira, &step, 0);
        vTaskDelay(pdMS_TO_TICKS(120));  // tekrarlama süresi
      }
    }

    // AŞAĞI
    if (down == LOW && lastDown == HIGH) {
      downPressedAt = now;
      int evt = -1;
      xQueueSend(dugmeSira, &evt, 0);
    }

    if (down == LOW && lastDown == LOW) {
      unsigned long held = now - downPressedAt;

      int step = 0;
      if (held > 1250) step = -10;
      else if (held > 750) step = -5;
      else if (held > 250) step = -1;

      if (step != 0) {
        xQueueSend(dugmeSira, &step, 0);
        vTaskDelay(pdMS_TO_TICKS(120));
      }
    }

    lastUp = up;
    lastDown = down;

    vTaskDelay(pdMS_TO_TICKS(20)); // aralık
  }
}

void failSafe() {
  if (failsafe == false) {
    failsafe = true;
    showText("FAIL-SAFE");
  }

  if (iletisimVar == true) {
    iletisimVar = false;
    digitalWrite(DEBUG_LED, LOW);
  }
  
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
      digitalWrite(DEBUG_LED, HIGH);
      showModeAndFile(
        currentMode == 2 ? "PLAYBACK" :
        currentMode == 1 ? "KAYIT" :
                           "SERBEST"
      );
      failsafe = false;
      return;
    }  
  }
}

const char* getSpecialName(int index) {
  switch (index) {
    case TAS_KAGIT_MAKAS: return "TKM";
    case EL_SALLA:          return "SLM";
    default:                return "SPC";
  }
}

void showModeAndFile(const char *modeText) {
  int16_t x_1, y_1, x_2, y_2;
  uint16_t w_1, h_1, w_2, h_2;
  
  display.clearDisplay();

  display.setFont(&FreeSans12pt7b);
  display.setTextSize(1);
  
  display.getTextBounds(modeText, 0, 0, &x_1, &y_1, &w_1, &h_1);

  display.setCursor((SCREEN_WIDTH - w_1) / 2, (SCREEN_HEIGHT - h_1) / 2);
  display.print(modeText);

  display.setFont(&FreeSans12pt7b);
  display.setTextSize(2);

if (currentFileIndex >= OzelBaslangic) {
  const char* name = getSpecialName(currentFileIndex);

  display.setTextSize(2);
  display.getTextBounds(name, 0, 0, &x_2, &y_2, &w_2, &h_2);
  display.setCursor((SCREEN_WIDTH - w_2) / 2, 60);
  display.print(name);
  display.display();
  return;
}

  String numara = "H-" + String(currentFileIndex);
  display.getTextBounds(numara, 0, 0, &x_2, &y_2, &w_2, &h_2);
  display.setCursor(0, 60);

  display.print("H-");
  display.print(currentFileIndex);

  display.display();
}

void showText(const char *text) {
  int16_t x_2, y_2;
  uint16_t w_2, h_2;
  
  display.clearDisplay();

  display.setFont(&FreeSans12pt7b);
  display.setTextSize(1);
  display.getTextBounds(text, 0, 0, &x_2, &y_2, &w_2, &h_2);
  display.setCursor((SCREEN_WIDTH - w_2) / 2, 40);
  display.print(text);
  display.display();
}

void sdKayit() {
  // ÖZEL HAREKET
  if (currentFileIndex >= OzelBaslangic) {
    if (recordingActive) {
      recFile.close();
      recordingActive = false;
    }
    showText("OZEL MOD");
    return;
  }

  // KAYIT BAŞLA
  if (!recordingActive) {
    char fileName[32];
    sprintf(fileName, "/hareketler/H-%d.txt", currentFileIndex);

    recFile = SD.open(fileName, FILE_WRITE);
    if (!recFile) {
      showText("KAYIT HATA");
      return;
    }

    recordingActive = true;
    lastRecWrite = 0;
    
  }

  // 50 Hz (1000 / 20 = 50)
  if (millis() - lastRecWrite < 1000 / KareAralik) return;
  lastRecWrite = millis();

  for (int i = 0; i < 8; i++) {
    recFile.print(kanal[i]);
    if (i < 7) recFile.print(";");
  }
  recFile.println();

  servoPan.writeMicroseconds    (kanal[0]);
  servoTilt.writeMicroseconds   (kanal[1]);
  servoBilek.writeMicroseconds  (kanal[2]);
  servoBas.writeMicroseconds    (kanal[3]);
  servoIsaret.writeMicroseconds (kanal[4]);
  servoOrta.writeMicroseconds   (kanal[5]);
  servoYuzuk.writeMicroseconds  (kanal[6]);
  servoSerce.writeMicroseconds  (kanal[7]);
}

void stopRecordingIfNeeded() {
  if (recordingActive) {
    recFile.close();
    recordingActive = false;
    //showText("KAYIT BITTI");
  }
}

void sdPlayback() {
  static File file;
  static bool fileOpen = false;
  static unsigned long lastStep = 0;
  static int lastFileIndex = -1;
  static bool wasInPlayback = false;

  if (currentMode != 2) {
    wasInPlayback = false;
    return;
  }

  if (!wasInPlayback) {
    // PLAYBACK'E YENİ GİRİLDİ
    if (fileOpen) file.close();
    fileOpen = false;
    lastFileIndex = -1;
    lastStep = 0;
    wasInPlayback = true;
  }

  if (!sdHazir) return;

  // 50 Hz (1000 / 20 = 50)
  if (millis() - lastStep < 1000 / KareAralik) return;
  lastStep = millis();

  // DOSYA DEĞİŞİMİ
  if (currentFileIndex != lastFileIndex) {
    if (fileOpen) {
      file.close();
      fileOpen = false;
    }
    lastFileIndex = currentFileIndex;
  }

  // ÖZEL HAREKET
  if (currentFileIndex >= OzelBaslangic) {
      if (!fileOpen) {
      char fileName[32];
      sprintf(fileName, "/ozel/%s.txt", getSpecialName(currentFileIndex));

      file = SD.open(fileName, FILE_READ);
      if (!file) {
        showModeAndFile("PB YOK");
        return;
      }

      fileOpen = true;
    }

    if (!file.available()) {
      file.close();
      fileOpen = false;
      return;
    }

    String line = file.readStringUntil('\n');

    int values[8];
    int count = sscanf(
      line.c_str(),
      "%d;%d;%d;%d;%d;%d;%d;%d",
      &values[0], &values[1], &values[2], &values[3],
      &values[4], &values[5], &values[6], &values[7]
    );

    if (count != 8) return;

    servoPan.writeMicroseconds(values[0]);
    servoTilt.writeMicroseconds(values[1]);
    servoBilek.writeMicroseconds(values[2]);
    servoBas.writeMicroseconds(values[3]);
    servoIsaret.writeMicroseconds(values[4]);
    servoOrta.writeMicroseconds(values[5]);
    servoYuzuk.writeMicroseconds(values[6]);
    servoSerce.writeMicroseconds(values[7]);
    return;
  }

  if (!fileOpen) {
    char fileName[32];
    sprintf(fileName, "/hareketler/H-%d.txt", currentFileIndex);

    file = SD.open(fileName, FILE_READ);
    if (!file) {
      showModeAndFile("PB YOK");
      return;
    }

    fileOpen = true;
  }

  if (!file.available()) {
    file.close();
    fileOpen = false;
    return;
  }

  String line = file.readStringUntil('\n');

  int values[8];
  int count = sscanf(
    line.c_str(),
    "%d;%d;%d;%d;%d;%d;%d;%d",
    &values[0], &values[1], &values[2], &values[3],
    &values[4], &values[5], &values[6], &values[7]
  );

  if (count != 8) return;

  servoPan.writeMicroseconds(values[0]);
  servoTilt.writeMicroseconds(values[1]);
  servoBilek.writeMicroseconds(values[2]);
  servoBas.writeMicroseconds(values[3]);
  servoIsaret.writeMicroseconds(values[4]);
  servoOrta.writeMicroseconds(values[5]);
  servoYuzuk.writeMicroseconds(values[6]);
  servoSerce.writeMicroseconds(values[7]);
}

//######################################                FONKSİYON TANIMLARI                ######################################