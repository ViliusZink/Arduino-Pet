# Arduino Pet
This is a virtual pet running on Arduino UNO, that has a 8x8 LED Matrix for display, potentiometer, photoresistor and two button inputs.

# Build steps
## Components
1 Arduino Uno

1 MAX7219 8×8 LED matrix

1 Potentiometer (B50K)

1 Photoresistor

1 10 kΩ resistor

2 pushbuttons

1 Breadboard

1 USB cable for programming and power

Jumper wires

## Build
Build the circuit using the schematic in the repository and the information below.

Open the project's .ino file in the Arduino IDE.

Install the required library - LEDMatrixDriver.

Connect the Arduino Uno to the computer using USB and upload code on the Arduino.

Serial Monitor should output pet's stats every second.

## Wiring
LED matrix V –> 5V

matrix GND –> GND

LED matrix DIN –> D11

LED matrix CS –> D9

LED matrix CLK –> D13

Potentiometer middle pin –> A0

Potentiometer left pin –> 5V

Potentiometer right pin –> GND

Photoresistor leg 1 – A1

Photoresistor leg 2 – 5V

10 kΩ resistor between photoresistor leg 1 and GND (after A1 connection)

Button 1 – D2 and GND

Button 2 – D3 and GND

# Timer config
Timer 1 is configured in CTC mode, used to generate an interrupt every second. Prescaler - 1024, OCR1A - 249, interrupt - TIMER2_COMPA_vect.
Interrupt sets flag for sensor input processing, display and servo updates almost every second. It also updates a 3 state counter for different actions.

# ISR roles
The Timer2 ISR counts to a second to set a sensor input processing, display and servo update flag and updates the phase counter.
Sets the sensor-reading flag.

# EEPROM layout
magic (uint16_t) – to identify valid data belonging to this application. Value 123 is used.

version (byte) – to identify the saved data format version. Value 1 is used.

temperature (int) – to save the last measured temperature.

These EEPROM fields are processed by methods in the Pet class, stats are verified to be in usable intervals.
