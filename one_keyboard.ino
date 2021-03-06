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

 


const bool DEBUG = 1;    /* Debug via Serial */
const bool PROGRAMMING = 1;  /* Programming mode */



/* USB report buffer, the last 8 bytes. */
uint8_t keyBuffer[8] = {0,0,0,0,0,0,0,0};
uint8_t previousBuffer[8] = {0,0,0,0,0,0,0,0};

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
  } else {
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

void updateBuffer() {
  //strncpy((char*)previousBuffer, (char*)keyBuffer, 8);
  for (int i = 0; i < 8; i++ ) {
    if (previousBuffer[i] != keyBuffer[i]) {
      previousBuffer[i] = keyBuffer[i];
    }
  }
}

void loop() {
  /* LED to indicate whether programming mode is on. */
  digitalWrite(statusLED, isRunState());
  /* TODO: optimize to use isRunState once */
  if (DEBUG) {
    debugWriteState();
  }
  
  /* scan */
  if (isRunState() != PROGRAMMING) {
    if (scan()) { // key is pressed
      if (DEBUG) {
        Serial.write(" keypress ");
      }
      keyBuffer[3] = scancode;
     
    } else {
      keyBuffer[3] = 0;
    }
    if (bufferChanged()) {  //send report
      if (DEBUG) {
        Serial.write(" SendReport ");
      }
      HID_SendReport(2, keyBuffer, 8);
      updateBuffer();
    }
  }
  delay(50);
}
