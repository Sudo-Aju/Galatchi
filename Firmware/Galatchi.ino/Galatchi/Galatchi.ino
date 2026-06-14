#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PCA95x5.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>

#define I2C_SDA         5
#define I2C_SCL         6
#define OLED_ADDR       0x3C
#define PCA9535_ADDR    0x20
#define BUZZER_PIN      10
#define JOY_DOWN        11
#define JOY_RIGHT       12
#define JOY_PRESS       13
#define JOY_UP          14
#define JOY_LEFT        15
#define BTN_RIGHT_90    10 //key 2
#define BTN_LEFT_90     4  // key 1
#define BTN_UP          0
#define BTN_DOWN        3
#define BTN_LEFT        1
#define BTN_RIGHT       2

PCA9555 ioex;
Adafruit_LSM6DSOX imu;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct Alien {
  int hunger;
  int happiness;
  int energy;
  unsigned long age;
};

Alien alien;
int selectedMenu = 0;
const int MENU_COUNT = 3;

enum Screen {
  SCREEN_MAIN,
  SCREEN_FEED,
  SCREEN_PLAY,
  SCREEN_SLEEP
};

Screen currentScreen = SCREEN_MAIN;

bool pcaPressed(uint8_t pin) {
    uint16_t state = ioex.read();
    return !(state & (1 << pin));
}

const unsigned char PROGMEM alienSleep[] = {
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00011000, 0b00011000,
  0b00011111, 0b11111000,
  0b00000111, 0b11100000,
  0b00001111, 0b11110000,
  0b00001101, 0b10110000,
  0b00111111, 0b11111100,
  0b00111111, 0b11111100,
  0b00111111, 0b11111100,
  0b00111111, 0b11111100,
  0b00001111, 0b11110000,
  0b00001111, 0b11110000,
  0b00001100, 0b00110000,
  0b00001100, 0b00110000
};

const unsigned char PROGMEM alienSad[] = {
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00011000, 0b00011000,
  0b00011000, 0b00011000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b11001101, 0b10110011,
  0b11001101, 0b10110011,
  0b00111111, 0b11111100,
  0b00111110, 0b01111100,
  0b00001101, 0b10110000,
  0b00001111, 0b11110000,
  0b00001111, 0b11110000,
  0b00001111, 0b11110000,
  0b00001100, 0b00110000,
  0b00001100, 0b00110000
};

const unsigned char PROGMEM alienHappy[] = {
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00011000, 0b00011000,
  0b00011000, 0b00011000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b11001101, 0b10110011,
  0b11001101, 0b10110011,
  0b00111111, 0b11111100,
  0b00111101, 0b10111100,
  0b00001110, 0b01110000,
  0b00001111, 0b11110000,
  0b00001111, 0b11110000,
  0b00001111, 0b11110000,
  0b00001100, 0b00110000,
  0b00001100, 0b00110000
};

const unsigned char PROGMEM alienNeutral[] = {
  0b00000000, 0b00000000,
  0b00011111, 0b11111000,
  0b00100000, 0b00000100,
  0b01000000, 0b00000010,
  0b01001111, 0b01110010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b01000011, 0b11000010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b00100000, 0b00000100,
  0b00011111, 0b11111000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000
};

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  
  pinMode(BUZZER_PIN, OUTPUT);

  
  ioex.attach(Wire);
  ioex.direction(PCA95x5::Direction::IN_ALL);
  ioex.polarity(PCA95x5::Polarity::ORIGINAL_ALL);

  
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED Init Failed!");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Galagotchi Init..");
    display.display();
  }

  
  if (!imu.begin_I2C()) {
    Serial.println("Failed to find LSM6DSOX chip");
  } else {
    Serial.println("LSM6DSOX Found!");
    
    imu.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
    imu.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);
    imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
    imu.setGyroDataRate(LSM6DS_RATE_104_HZ);
  }

  delay(1000);
  
  alien.hunger = 80;
  alien.happiness = 80;
  alien.energy = 80;
  alien.age = 0;
}

unsigned long lastUpdate = 0;
unsigned long lastButtonPress = 0;
unsigned long lastShakeTime = 0;

void updateAlien() {
  if (millis() - lastUpdate > 5000) {
    alien.hunger--;
    alien.happiness--;
    alien.energy--;

    if (alien.hunger < 0) alien.hunger = 0;
    if (alien.happiness < 0) alien.happiness = 0;
    if (alien.energy < 0) alien.energy = 0;

    alien.age += 5;
    lastUpdate = millis();
  }
}

void checkIMU() {
  
  if (millis() - lastShakeTime < 1000) return;

  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  imu.getEvent(&accel, &gyro, &temp);

  
  float magnitude = sqrt(pow(accel.acceleration.x, 2) + 
                         pow(accel.acceleration.y, 2) + 
                         pow(accel.acceleration.z, 2));

  
  if (magnitude > 25.0) {
    alien.happiness += 15;
    alien.energy -= 5;
    
    if(alien.happiness > 100) alien.happiness = 100;
    if(alien.energy < 0) alien.energy = 0;

    // Shake sound effect
    tone(BUZZER_PIN, 1200, 100);
    delay(100);
    tone(BUZZER_PIN, 1600, 150);
    
    lastShakeTime = millis();
  }
}

void checkButtons() {
  if (millis() - lastButtonPress < 200) return;

  if (pcaPressed(JOY_LEFT)) {
    currentScreen = SCREEN_FEED;
    tone(BUZZER_PIN, 1000, 50);
    lastButtonPress = millis();

  } else if (pcaPressed(JOY_UP)) {
    currentScreen = SCREEN_PLAY;
    tone(BUZZER_PIN, 1200, 50);
    lastButtonPress = millis();

  } else if (pcaPressed(JOY_RIGHT)) {
    currentScreen = SCREEN_SLEEP;
    tone(BUZZER_PIN, 800, 50);
    lastButtonPress = millis();

  } else if (pcaPressed(JOY_PRESS)) {
    currentScreen = SCREEN_MAIN;
    tone(BUZZER_PIN, 1500, 50);
    lastButtonPress = millis();

  } else if (pcaPressed(BTN_LEFT_90)) {
    alien.hunger += 5;
    if(alien.hunger > 100) alien.hunger = 100;
    tone(BUZZER_PIN, 1800, 80);
    lastButtonPress = millis();

  } else if (pcaPressed(BTN_RIGHT_90)) {
    alien.happiness += 5;
    if(alien.happiness > 100) alien.happiness =100;
    tone(BUZZER_PIN, 2200, 40);
    delay(50);
    tone(BUZZER_PIN, 2200, 60);
    lastButtonPress = millis();

  } else if (pcaPressed(BTN_UP)) {
    alien.energy += 5;
    if (alien.energy > 100) alien.energy = 100;
    tone(BUZZER_PIN, 1600, 50);
    lastButtonPress = millis();

  } else if (pcaPressed(BTN_DOWN)) {
    alien.energy -= 5;
    if (alien.energy < 0) alien.energy = 0;
    tone(BUZZER_PIN, 600, 50);
    lastButtonPress = millis();

  } else if (pcaPressed(BTN_LEFT)) {
    selectedMenu--;
    if (selectedMenu < 0) selectedMenu = MENU_COUNT - 1;
    tone(BUZZER_PIN, 1000, 30);
    lastButtonPress = millis();

  } else if (pcaPressed(BTN_RIGHT)) {
    selectedMenu++;
    if (selectedMenu >= MENU_COUNT) selectedMenu = 0;
    tone(BUZZER_PIN, 1000, 30);
    lastButtonPress = millis();
  }
}

void handleScreenLogic() {
  switch(currentScreen) {

    case SCREEN_FEED:
      alien.hunger += 15;
      if (alien.hunger > 100) alien.hunger = 100;
      currentScreen = SCREEN_MAIN;
      break;

    case SCREEN_PLAY:
      alien.happiness += 15;
      alien.energy -= 10;
      if (alien.happiness > 100) alien.happiness = 100;
      if (alien.energy < 0) alien.energy = 0;
      currentScreen = SCREEN_MAIN;
      break;

    case SCREEN_SLEEP:
      alien.energy += 25;
      if (alien.energy > 100) alien.energy = 100;
      currentScreen = SCREEN_MAIN;
      break;

    case SCREEN_MAIN:
      break;
  }
}

void drawBar(int x, int y, int value) {
  int barWidth = 100;
  int barHeight = 6;
  int fillWidth = map(value, 0, 100, 0, barWidth);

  display.drawRect(x, y, barWidth, barHeight, SSD1306_WHITE);
  display.fillRect(x, y, fillWidth, barHeight, SSD1306_WHITE);
}

void render() {
  display.clearDisplay();

  const unsigned char* sprite;
  if (alien.energy < 20) {
    sprite = alienSleep;
  } else if (alien.hunger < 30 || alien.happiness < 30) {
    sprite = alienSad;
  } else if (alien.hunger > 50 && alien.happiness > 50 && alien.energy > 50) {
    sprite = alienHappy;
  } else {
    sprite = alienNeutral;
  }

  display.drawBitmap(56, 2, sprite, 16, 16, SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 24);
  display.print("HUN ");
  drawBar(24, 24, alien.hunger);

  display.setCursor(0, 34);
  display.print("HAP ");
  drawBar(24, 34, alien.happiness);

  display.setCursor(0, 44);
  display.print("ENG ");
  drawBar(24, 44, alien.energy);
  
  display.setCursor(0, 56);
  display.println("[Feed] [Play] [Sleep]");

  display.display();
}

void loop() {
  checkIMU();
  checkButtons();
  updateAlien();
  handleScreenLogic();
  render();
  delay(100);
}