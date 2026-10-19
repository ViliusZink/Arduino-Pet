#include <LEDMatrixDriver.hpp>
#include <EEPROM.h>

struct PetSaveData {
  uint16_t magic;
  byte version;
  int health;
  int energy;
  int hunger;
};

const uint16_t EEPROM_MAGIC = 666;
const byte EEPROM_VERSION = 1;
const int EEPROM_ADDRESS = 0;

volatile bool doSensorRead = false;
volatile bool updatePetStats = false;
volatile bool dead = false;
volatile int secondCounter = 0;

ISR(TIMER1_COMPA_vect){
  doSensorRead = true;
  ++secondCounter;

  if(secondCounter >= 30){
    secondCounter = 0;
    updatePetStats = true;
  }
}

class MatrixController{
  private:
  LEDMatrixDriver matrix;
  byte framebuffer[8] = {0};

  public:
  MatrixController() : matrix(1, 9){
    matrix.setEnabled(true);
    matrix.setIntensity(8);
  }

  void refresh() {
    for (int y = 0; y < 8; ++y) {
      matrix.setColumn(y, framebuffer[y]);
    }
    matrix.display();
  }

  void setPixel(int x, int y){
    if (x >= 0 && x < 8 && y >= 0 && y < 8){
      framebuffer[y] |= (1 << (7 - x));
      refresh();
    }
  }

  void unsetPixel(int x, int y){
    if (x >= 0 && x < 8 && y >= 0 && y < 8){
      framebuffer[y] &= ~(1 << (7 - x));
      refresh();
    }
  }

  void setAll(){
    for (int y = 0; y < 8; ++y){
      framebuffer[y] = 255;
    }
    refresh();
  }

  void clearAll(){
    for (int y = 0; y < 8; ++y){
      framebuffer[y] = 0;
    }
    refresh();
  }

  void drawBitmap(byte bitmap[8]){
    for (int y = 0; y < 8; ++y){
      framebuffer[y] = bitmap[y];
    }
    refresh();
  }

  void scrollDown(){
    byte bottom = framebuffer[7];
    for (int y = 7; y > 0; --y){
      framebuffer[y] = framebuffer[y - 1];
    }
    framebuffer[0] = bottom;
    refresh();
  }
};

class Pet{
  private:
  MatrixController &mc;
  bool isCold = false;
  byte animationCounter = 0;
  byte stateCounter = 0;
  int health = 500;
  int energy = 500;
  int hunger = 500;
  byte happyBM[2][8] = {
    {B00000000,
     B00000000,
     B01100110,
     B01100110,
     B00000000,
     B01000010,
     B00111100,
     B00000000},
    {B00000000,
     B00000000,
     B00000000,
     B01100110,
     B00000000,
     B01000010,
     B00111100,
     B00000000}
  };
  byte sleepBM[2][8] = {
    {B00000111,
     B00000010,
     B00000111,
     B00000000,
     B11110000,
     B00100000,
     B01000000,
     B11110000},
    {B00000000,
     B00000111,
     B00000010,
     B11110111,
     B00100000,
     B01000000,
     B11110000,
     B00000000}
  };
  byte coldBM[8] = {
    B01000010,
    B00000111,
    B00010010,
    B10000000,
    B00001010,
    B00000100,
    B01001010,
    B11100000
  };
  byte hotBM[2][8] = {
    {B00000000,
     B00010000,
     B00011000,
     B00111000,
     B00111100,
     B00111100,
     B00011000,
     B00000000},
    {B00000000,
     B00001000,
     B00011000,
     B00011100,
     B00111100,
     B00111100,
     B00011000,
     B00000000}
  };
  byte deadBM[8] = {
    B00000000,
    B10100101,
    B01000010,
    B10100101,
    B00000000,
    B00111100,
    B01000010,
    B00000000
  };
  byte tiredBM[8] = {
    B00000111,
    B00000010,
    B00000111,
    B00000000,
    B01100110,
    B00000000,
    B00111100,
    B00000000
  };
  byte hungryBM[8] = {
    B00000000,
    B01010010,
    B01010110,
    B01110110,
    B00100110,
    B00100010,
    B00100010,
    B00000000
  };
  byte eatingBM[2][8] = {
    {B00110000,
     B01111000,
     B10111000,
     B10011100,
     B01100100,
     B00011111,
     B00000111,
     B00000110},
    {B00000000,
     B00110000,
     B01110000,
     B01111000,
     B00011100,
     B00001111,
     B00000111,
     B00000110}
  };
  byte pettingBM[8] = {
    B00000000,
    B01100110,
    B11111111,
    B11111111,
    B01111110,
    B00111100,
    B00011000,
    B00000000
  };

  public:
  Pet(MatrixController &mc) : mc(mc){}

  void reset(){
    health = 500;
    energy = 500;
    hunger = 500;
  }

  void drawCurrentState(){
    animationCounter = (animationCounter + 1) % 2;
    switch(stateCounter){
      case 0:
        mc.drawBitmap(happyBM[animationCounter]);
        break;
      case 1:
        mc.drawBitmap(sleepBM[animationCounter]);
        break;
      case 2:
        if(isCold){
          mc.scrollDown();
        }
        else{
          mc.drawBitmap(coldBM);
          isCold = true;
        }
        break;
      case 3:
        mc.drawBitmap(hotBM[animationCounter]);
        isCold = false;
        break;
      case 4:
        mc.drawBitmap(tiredBM);
        break;
      case 5:
        mc.drawBitmap(hungryBM);
        break;
    }
  }

  void updateState(){
    if(energy < 100){
      stateCounter = 4;
    }
    if(hunger < 100){
      stateCounter = 5;
    }
    else if(energy >= 100){
      stateCounter = 0;
    }
  }

  void checkLight(int lightValue){
    if(lightValue < 512){
      stateCounter = 1;
    }
  }

  void checkTemp(int heaterValue){
    if(heaterValue <= 340){
      stateCounter = 2;
      health -= 34 - heaterValue / 10;
      saveStats();
    }
    else if(heaterValue >= 683){
      stateCounter = 3;
      isCold = false;
      health -= 34 - (1023 - heaterValue) / 10;
      saveStats();
    }
    else{
      isCold = false;
    }
  }

  void checkHealth(){
    if(health <= 0){
      dead = true;
      mc.drawBitmap(deadBM);
    }
  }

  void updateStats(){
    if(stateCounter == 1){
      if(health <= 496){
        health += 4;
      }
      if(energy <= 492){
        energy += 8;
      }
    }
    else{
      if(energy >= 4){
        energy -= 4;
      }
    }
    if(energy <= 0 && health >= 4){
      health -= 4;
    }
    if(hunger <= 0 && health >= 8){
      health -= 8;
    }
    if(hunger >= 4){
      hunger -= 4;
      if(hunger >= 300 && health <= 496){
        health += 4;
      }
    }
    saveStats();
    Serial.print("Health: ");
    Serial.print(health);
    Serial.print(" | Energy: ");
    Serial.print(energy);
    Serial.print(" | Hunger: ");
    Serial.println(hunger);
  }

  void feed(){
    if(hunger <= 400){
      hunger += 100;
    }
    else{
      hunger = 500;
    }
    saveStats();
    mc.drawBitmap(eatingBM[0]);
    delay(400);
    mc.drawBitmap(eatingBM[1]);
    delay(400);
    drawCurrentState();
  }

  void pet(){
    if(health <= 496){
      health += 4;
      saveStats();
    }
    mc.drawBitmap(pettingBM);
    delay(400);
    drawCurrentState();
  }

  void loadStats(){
    PetSaveData data;
    EEPROM.get(EEPROM_ADDRESS, data);

    if (data.magic != EEPROM_MAGIC || data.version != EEPROM_VERSION || data.health <= 0 || data.health > 500 || data.energy < 0 || data.energy > 500 || data.hunger < 0 || data.hunger > 500) {
      health = 500;
      energy = 500;
      hunger = 500;
      saveStats();
      return;
    }

    health = data.health;
    energy = data.energy;
    hunger = data.hunger;
  }

  void saveStats(){
    PetSaveData data;

    data.magic = EEPROM_MAGIC;
    data.version = EEPROM_VERSION;
    data.health = health;
    data.energy = energy;
    data.hunger = hunger;

    EEPROM.put(EEPROM_ADDRESS, data);
  }
};

MatrixController mc;
Pet pet(mc);
unsigned long lastKeyPress = 0;
unsigned long keyDelay = 100;
volatile bool foodButtonWasPressed = false;
volatile bool petButtonWasPressed = false;

void foodButtonPressed(){
  foodButtonWasPressed = true;
}

void petButtonPressed(){
  petButtonWasPressed = true;
}

void setup(){
  Serial.begin(9600);

  pet.loadStats();

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);

  noInterrupts();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 15624;
  TCCR1B |= (1 << WGM12);
  TIMSK1 |= (1 << OCIE1A);
  TCCR1B |= (1 << CS12) | (1 << CS10);

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), foodButtonPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), petButtonPressed, FALLING);

  interrupts();
}

void loop(){
  if(updatePetStats && !dead){
    updatePetStats = false;
    pet.updateStats();
  }
  if(doSensorRead && !dead){
    doSensorRead = false;

    int heaterValue = analogRead(A0);
    int lightValue = analogRead(A1);

    pet.updateState();
    pet.checkLight(lightValue);
    pet.checkTemp(heaterValue);
    pet.drawCurrentState();
    pet.checkHealth();
  }
  if(foodButtonWasPressed){
    if(dead){
      dead = false;
      pet.reset();
    }
    else if(millis() - lastKeyPress >= keyDelay){
      lastKeyPress = millis();
      pet.feed();
    }
    foodButtonWasPressed = false;
  }
  if(petButtonWasPressed){
    if(dead){
      dead = false;
      pet.reset();
    }
    else if(millis() - lastKeyPress >= keyDelay){
      lastKeyPress = millis();
      pet.pet();
    }
    petButtonWasPressed = false;
  }
}
