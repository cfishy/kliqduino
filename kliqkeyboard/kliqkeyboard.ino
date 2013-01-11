/* Copyright Hsiao-yu Chen  2012
 Arduino based keyboard controller.
 */
#include <Keypad.h>
#include <Usb.h>

/* Digital output pin for the single key switch, closed to ground */
const int KEY_SWITCH_PIN = 7;   
/* USB HID scan code to send when the key is pressed. */
const int scancode = 4; /* scan code to send */

/* pin for mode toggle switch, short to ground */
/* This programming mode stops keyboard from functioning. */
const int MODE_TOGGLE = 2; 
/* when toggle switch is closed, LED on, indicating programming mode. */
/* when toggle switch is open, LED off, indicating running mode. */
int statusLED = 13;  /* built in LED */


/* Key matrix definition */
const byte ROWS = 3;
const byte COLS = 2;
char keys[ROWS][COLS] = {
  {
    '1', '2'    }
  ,
  {
    '3', '4'    }
};
byte rowPins[ROWS] = {
  8, 9, 10};
byte colPins[COLS] = {
  4, 5};
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


/* Debug and disaster relief */
const bool DEBUG = 1;    /* Debug via Serial */
const bool PROGRAMMING = 1;  /* Programming mode */



/* USB report buffer, the last 8 bytes. */
uint8_t keyBuffer[8] = {
  0,0,0,0,0,0,0,0};
uint8_t previousBuffer[8] = {
  0,0,0,0,0,0,0,0};

/* stateMap is the last state of keypad.bitMap */
uint stateMap[MAPSIZE];

/* Setup a toggle to switch between 
 programming and run mode. Avoids interference. */
void setupModeToggle() {
  /* pull up */
  pinMode(MODE_TOGGLE, INPUT);
  digitalWrite(MODE_TOGGLE, HIGH);
  digitalWrite(KEY_SWITCH_PIN, HIGH);
  /* LED to indicate programming mode */
  pinMode(statusLED, OUTPUT);  
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

int scan() {
  return !digitalRead(KEY_SWITCH_PIN);
}


void setup() {
  setupModeToggle();
  kpd.setHoldTime(10);
  kpd.setDebounceTime(5);
}

bool bufferChanged() {
  //not sure why strncmp isn't working after casting
  //return strncmp((char*)previousBuffer, (char*) keyBuffer, 8);
  for (int i = 0; i < 8; i++) {
    if (previousBuffer[i] != keyBuffer[i]) {
      return true;
    }
  }
  return false;
}

void storeBuffer() {
  //strncpy((char*)previousBuffer, (char*)keyBuffer, 8);
  for (int i = 0; i < 8; i++ ) {
    if (previousBuffer[i] != keyBuffer[i]) {
      previousBuffer[i] = keyBuffer[i];
    }
  }
}

void buildBuffer() {
  int keyPosition = kpd.findInList('1');
  //debug
  Serial.print("1 key is ");
  Serial.println(kpd.key[keyPosition].kstate);
  Serial.println("");

  if (keyBuffer[3]) {
    keyBuffer[3] = 0;
  } 
  else {
    keyBuffer[3] = scancode;
  }
}

bool keyStateChanged() {
  printBitMap();
  printDiffMap();
  
  // When sharing row pins with other hardware
  // they may need to be re-intialized.
  for (byte r=0; r<ROWS; r++) {
    pinMode(rowPins[r],INPUT_PULLUP);
    digitalWrite(rowPins[r],HIGH);
    //debug
    Serial.print("pullup row ");
    Serial.println(r);
  }
  
  for (byte c=0; c < COLS; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], LOW);  // Begin column pulse output.
   
    for (byte r=0; r < ROWS; r++) {
      Serial.print("Reading (");
      Serial.print(r);
      Serial.print(",");
      Serial.print(c);
      Serial.print(")->");
      Serial.print(!digitalRead(rowPins[r]));
      Serial.println("");
      delay(400);
      bitWrite(kpd.bitMap[r], c, !digitalRead(rowPins[r]));  // keypress is active low but invert to high.
    }
    // Set pin to high impedance input. Effectively ends column pulse.
    digitalWrite(colPins[c],HIGH);
    pinMode(colPins[c],INPUT);
    
  }
  
  
  for (byte c=0; c < COLS; c++) {
    for (byte r=0; r < ROWS; r++) {
      if (bitRead(kpd.bitMap[r],c) != bitRead(stateMap[r],c)) {
        Serial.println("FOUND");
        delay(1000);
        return true;
      }
    }
  }
  return false;
}




void storeKeyState() {
  Serial.println("");
  Serial.println("storeKeyState");
  Serial.println("");
  for (byte c=0; c < COLS; c++) {
    for (byte r=0; r < ROWS; r++) {
      bitWrite(stateMap[r],c, bitRead(kpd.bitMap[r],c));
    }
  }
}

void printBitMap() {
  Serial.print("keystate: ");
  Serial.write("{");
  Serial.print(bitRead(kpd.bitMap[0], 0));
  Serial.write(",");
  Serial.print(bitRead(kpd.bitMap[0], 1));
  Serial.write(",");
  Serial.print(bitRead(kpd.bitMap[1], 0));
  Serial.write(",");
  Serial.print(bitRead(kpd.bitMap[1], 1));
  Serial.println("}"); 
}


void printDiffMap() {
  Serial.print("keydiff: ");
  Serial.write("{");
  Serial.print(bitRead(kpd.bitMap[0], 0) != bitRead(stateMap[0],0));
  Serial.write(",");
  Serial.print(bitRead(kpd.bitMap[0], 1) != bitRead(stateMap[0],1));
  Serial.write(",");
  Serial.print(bitRead(kpd.bitMap[1], 0) != bitRead(stateMap[1],0));
  Serial.write(",");
  Serial.print(bitRead(kpd.bitMap[1], 1) != bitRead(stateMap[1],1));
  Serial.println("}"); 
}

void loop() {
  /* LED to indicate whether programming mode is on. */
  digitalWrite(statusLED, isRunState());
  /* TODO: optimize to use isRunState once */
  if (DEBUG) {
    //debugWriteState();
  }

  /* scan */
  if (isRunState() != PROGRAMMING) {
    if (keyStateChanged()) {             //change detected
      Serial.print(" keypressed ");
      delay(1000);
      storeKeyState();                  //remember the state
      buildBuffer();
      HID_SendReport(2, keyBuffer, 8);
     // storeBuffer();
    }
  }
  delay(100);
}


