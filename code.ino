#include <LEDMatrixDriver.hpp>

volatile bool doSensorRead = false;
volatile bool updatePetStats = false;
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
     B00101000,
     B00100100,
     B00100100,
     B00011000,
     B00000000},
    {B00000000,
     B00001000,
     B00011000,
     B00010100,
     B00100100,
     B00100100,
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

  public:
  Pet(MatrixController &mc) : mc(mc){}

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
    }
  }

  void checkLight(int lightValue){
    if(lightValue < 512){
      stateCounter = 1;
    }
    else{
      stateCounter = 0;
    }
  }

  void checkTemp(int heaterValue){
    if(heaterValue <= 340){
      stateCounter = 2;
      health -= 34 - heaterValue / 10;
    }
    else if(heaterValue >= 683){
      stateCounter = 3;
      isCold = false;
      health -= 34 - (1023 - heaterValue) / 10;
    }
    else{
      isCold = false;
    }
    drawCurrentState();
    checkHealth();
  }

  void checkHealth(){
    if(health <= 0){
      cli();
      mc.drawBitmap(deadBM);
      while(1){
      }
      sei();
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
    checkHealth();
    Serial.print("Health: ");
    Serial.print(health);
    Serial.print(" | Energy: ");
    Serial.print(energy);
    Serial.print(" | Hunger: ");
    Serial.println(hunger);
  }
};

MatrixController mc;
Pet pet(mc);

void setup(){
  Serial.begin(9600);

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
  interrupts();
}

void loop(){
  if(doSensorRead){
    doSensorRead = false;

    int heaterValue = analogRead(A0);
    int lightValue = analogRead(A1);

    pet.checkLight(lightValue);
    pet.checkTemp(heaterValue);
  }
  if(updatePetStats){
    updatePetStats = false;
    pet.updateStats();
  }
}
