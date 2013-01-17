/* Copyright Hsiao-yu Chen  2012
 Arduino based keyboard controller.
 */
#include <Keypad.h>
#include <Usb.h>


/* safety switch to turn it off if the keyboard goes crazy. */
/* pin for mode toggle switch, short to ground */
/* This programming mode stops keyboard from functioning. */
const int MODE_TOGGLE = 2;

/* when toggle switch is closed, LED on, indicating programming mode. */
/* when toggle switch is open, LED off, indicating running mode. */
int statusLED = 13;  /* built in LED */


/* Key matrix definition */
const byte ROWS = 3;
const byte COLS = 2;

//USB HID keyboard scancodes
char keys[ROWS][COLS] = {
  {    4, 5    }, //a, b
  {    6, 7    }, //c, d
  {    225, 226}, //L_shift, L_alt
};
byte rowPins[ROWS] = {
  8, 9, 10};
byte colPins[COLS] = {
  4, 5};
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


/* Debug and disaster relief flags */
const bool DEBUG = 1;    /* Debug via Serial */
const bool PROGRAMMING = 1;  /* Programming mode */



/* USB report buffer, the last 8 bytes. */
uint8_t keyBuffer[8] = {
  0,0,0,0,0,0,0,0};

/* Setup a toggle to switch between 
 programming and run mode. Avoids interference. */
void setupModeToggle() {
  /* pull up */
  pinMode(MODE_TOGGLE, INPUT);
  digitalWrite(MODE_TOGGLE, HIGH);
  /* LED to indicate programming mode */
  pinMode(statusLED, OUTPUT);  
  /*Serial debugging */
  Serial.begin(9600);
}

void debugWriteState() {
  if (isRunState() == PROGRAMMING) {
    Serial.write(" programming ");
  } 
  else {
    Serial.write(" running ");
  }
}

/* Check to see whether the toggle is on run state. */
bool isRunState() {
  return digitalRead(MODE_TOGGLE);
}

//debug method
void printKeyMap() {
  Serial.print("keystate: ");
  Serial.write("{");
  Serial.print(kpd.key[0].kstate);
  Serial.print("-");
  Serial.print(byte(kpd.key[0].kchar));
  Serial.print("-");
  Serial.print(int(kpd.key[0].kchar) >= 224);
  Serial.write(",");
  Serial.print(kpd.key[1].kstate);
  Serial.print("-");
  Serial.print(uint(kpd.key[1].kchar));
  Serial.write(",");
  Serial.print(kpd.key[2].kstate);
  Serial.print("-");
  Serial.print(uint(kpd.key[2].kchar));
  Serial.write(",");
  Serial.print(kpd.key[3].kstate);
  Serial.print("-");
  Serial.print(uint(kpd.key[3].kchar));
  Serial.write(",");
  Serial.print(kpd.key[4].kstate);
  Serial.print("-");
  Serial.print(uint(kpd.key[4].kchar));
  Serial.write(",");
  Serial.print(kpd.key[5].kstate);
  Serial.print("-");
  Serial.print(uint(kpd.key[5].kchar));
  Serial.write(",");
  Serial.println("}"); 
}


void makeKeyBuffer() {
  //TODO: modifier keys
  //The HID boot keyboard scancode starts at 3rd byte, til 8th.
  printKeyMap();
  //zero out keyBuffer before refilling, need optimization
  keyBuffer[0] = 0;
  for (int i=0; i <= 6; i++) {
    keyBuffer[i+3] = 0;
    if (kpd.key[i].kstate == PRESSED || kpd.key[i].kstate == HOLD) {
      //Handle modifier key. modifier byte is the 0th byte.
      //bit 0 is L CTRL, bit 1 is L SHIFT, bit 2 is L ALT, bit 3 is L GUI, bit 4 is R CTRL, bit 5 is R SHIFT, bit 6 is R ALT, and bit 7 is R GUI
      switch (byte(kpd.key[i].kchar)) {
        case 224: //left control
          bitWrite(keyBuffer[0], 0, 1);
          break;
        case 225: //left shift
          bitWrite(keyBuffer[0], 1, 1);
          break;
        case 226: //left alt
          bitWrite(keyBuffer[0], 2, 1);
          break;
        case 227: //left GUI
          bitWrite(keyBuffer[0], 3, 1);
          break;
        case 228: //right control
          bitWrite(keyBuffer[0], 4, 1);
          break;
        case 229: //right shift
          bitWrite(keyBuffer[0], 5, 1);
          break;
        case 230: //right alt
          bitWrite(keyBuffer[0], 6, 1);
          break;
        case 231: //right DUI
          bitWrite(keyBuffer[0], 7, 1);
          break;
        default:  //non-modifier key
          //USB boot specification key range
          if (byte(kpd.key[i].kchar) <= 101) {
            keyBuffer[i+3] = kpd.key[i].kchar;
          }
      }
    }
  }
}

void setup() {
  setupModeToggle();
  kpd.setHoldTime(10);
  kpd.setDebounceTime(15);
}

void loop() {
  /* LED to indicate whether programming mode is on. */
  digitalWrite(statusLED, isRunState());
  /* TODO: optimize to use isRunState once */
  if (DEBUG) {
   //debugWriteState();
  }
  if (isRunState()!=PROGRAMMING && kpd.getKeys()) {
    Serial.println("state changed.");
    for (int i = 0; i < MAPSIZE; i++) {
      makeKeyBuffer();
      HID_SendReport(2, keyBuffer, 8);
    }
  }
}


