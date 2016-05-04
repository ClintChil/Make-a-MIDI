/*
 ************************************************
 ************** MAKEY MAKEY *********************
 ************************************************
 
 /////////////////////////////////////////////////
 /////////////HOW TO EDIT THE KEYS ///////////////
 /////////////////////////////////////////////////
 - Edit keys in the settings.h file
 - that file should be open in a tab above (in Arduino IDE)
 - more instructions are in that file
 
 //////////////////////////////////////////////////
 ///////// MaKey MaKey FIRMWARE v1.4.1 ////////////
 //////////////////////////////////////////////////
 by: Eric Rosenbaum, Jay Silver, and Jim Lindblom
 MIT Media Lab & Sparkfun
 start date: 2/16/2012 
 current release: 7/5/2012
 */

/////////////////////////
// DEBUG DEFINITIONS ////               
/////////////////////////
//#define DEBUG
//#define DEBUG2 
//#define DEBUG3 
//#define DEBUG_TIMING
//#define DEBUG_MOUSE
//#define DEBUG_TIMING2

////////////////////////
// DEFINED CONSTANTS////
////////////////////////

#define BUFFER_LENGTH    3     // 3 bytes gives us 24 samples
#define NUM_INPUTS       8    // 6 on the front + 12 on the back
//#define TARGET_LOOP_TIME 694   // (1/60 seconds) / 24 samples = 694 microseconds per sample 
//#define TARGET_LOOP_TIME 758  // (1/55 seconds) / 24 samples = 758 microseconds per sample 
#define TARGET_LOOP_TIME 744  // (1/56 seconds) / 24 samples = 744 microseconds per sample 

#include "settings.h"

/////////////////////////
// STRUCT ///////////////
/////////////////////////
typedef struct {
  byte pinNumber;
  int keyCode;
  byte measurementBuffer[BUFFER_LENGTH]; 
  boolean oldestMeasurement;
  byte bufferSum;
  boolean pressed;
  boolean prevPressed;
  boolean isKey;
} 
MakeyMakeyInput;

MakeyMakeyInput inputs[NUM_INPUTS];

///////////////////////////////////
// VARIABLES //////////////////////
///////////////////////////////////
int bufferIndex = 0;
byte byteCounter = 0;
byte bitCounter = 0;

int pressThreshold;
int releaseThreshold;
boolean inputChanged;

// Pin Numbers
// input pin numbers for kickstarter production board
int pinNumbers[NUM_INPUTS] = {
  A0, A1, 0, 1, 2, 3, 4, 5
};

//// input status LED pin numbers
//const int inputLED_a = 9;
//const int inputLED_b = 10;
//const int inputLED_c = 11;
//const int outputK = 14;
//const int outputM = 16;
//byte ledCycleCounter = 0;

// timing
int loopTime = 0;
int prevTime = 0;
int loopCounter = 0;


///////////////////////////
// FUNCTIONS //////////////
///////////////////////////
void initializeArduino();
void initializeInputs();
void updateMeasurementBuffers();
void updateBufferSums();
void updateBufferIndex();
void updateInputStates();
void addDelay();
//void cycleLEDs();
//void danceLeds();
//void updateOutLEDs();

//////////////////////
// SETUP /////////////
//////////////////////
void setup() 
{
  initializeArduino();
  initializeInputs();
//  danceLeds();
}

////////////////////
// MAIN LOOP ///////
////////////////////
void loop() 
{
  updateMeasurementBuffers();
  updateBufferSums();
  updateBufferIndex();
  updateInputStates();
//  cycleLEDs();
//  updateOutLEDs();
  addDelay();
}

//////////////////////////
// INITIALIZE ARDUINO
//////////////////////////
void initializeArduino() {
#ifdef DEBUG
  Serial.begin(9600);  // Serial for debugging
#endif

  /* Set up input pins 
   DEactivate the internal pull-ups, since we're using external resistors */
  for (int i=0; i<NUM_INPUTS; i++)
  {
    pinMode(pinNumbers[i], INPUT);
    digitalWrite(pinNumbers[i], LOW);
  }

//  pinMode(inputLED_a, INPUT);
//  pinMode(inputLED_b, INPUT);
//  pinMode(inputLED_c, INPUT);
//  digitalWrite(inputLED_a, LOW);
//  digitalWrite(inputLED_b, LOW);
//  digitalWrite(inputLED_c, LOW);

//  pinMode(outputK, OUTPUT);
//  pinMode(outputM, OUTPUT);
//  digitalWrite(outputK, LOW);
//  digitalWrite(outputM, LOW);


#ifdef DEBUG
  delay(4000); // allow us time to reprogram in case things are freaking out
#endif

//  // enable HID for sending key presses
//  BeanHid.enable();

  BeanMidi.enable();
}

///////////////////////////
// INITIALIZE INPUTS
///////////////////////////
void initializeInputs() {

  float thresholdPerc = SWITCH_THRESHOLD_OFFSET_PERC;
  float thresholdCenterBias = SWITCH_THRESHOLD_CENTER_BIAS/50.0;
  float pressThresholdAmount = (BUFFER_LENGTH * 8) * (thresholdPerc / 100.0);
  float thresholdCenter = ( (BUFFER_LENGTH * 8) / 2.0 ) * (thresholdCenterBias);
  pressThreshold = int(thresholdCenter + pressThresholdAmount);
  releaseThreshold = int(thresholdCenter - pressThresholdAmount);

#ifdef DEBUG
  Serial.println(pressThreshold);
  Serial.println(releaseThreshold);
#endif

  for (int i=0; i<NUM_INPUTS; i++) {
    inputs[i].pinNumber = pinNumbers[i];
    inputs[i].keyCode = keyCodes[i];

    for (int j=0; j<BUFFER_LENGTH; j++) {
      inputs[i].measurementBuffer[j] = 0;
    }
    inputs[i].oldestMeasurement = 0;
    inputs[i].bufferSum = 0;

    inputs[i].pressed = false;
    inputs[i].prevPressed = false;

    inputs[i].isKey = false;

    if (inputs[i].keyCode) {
#ifdef DEBUG_MOUSE
      Serial.println("GOT IT");  
#endif
      inputs[i].isKey = true;
    }
// left this here so the above conditonal makes sense.
//    if (inputs[i].keyCode < 0) {
//#ifdef DEBUG_MOUSE
//      Serial.println("GOT IT");  
//#endif
//
////      inputs[i].isMouseMotion = true;
//    } 
//    else if ((inputs[i].keyCode == MOUSE_LEFT) || (inputs[i].keyCode == MOUSE_RIGHT)) {
////      inputs[i].isMouseButton = true;
//    } 
//    else {
//      inputs[i].isKey = true;
//    }
#ifdef DEBUG
    Serial.println(i);
#endif

  }
}


//////////////////////////////
// UPDATE MEASUREMENT BUFFERS
//////////////////////////////
void updateMeasurementBuffers() {

  for (int i=0; i<NUM_INPUTS; i++) {

    // store the oldest measurement, which is the one at the current index,
    // before we update it to the new one 
    // we use oldest measurement in updateBufferSums
    byte currentByte = inputs[i].measurementBuffer[byteCounter];
    inputs[i].oldestMeasurement = (currentByte >> bitCounter) & 0x01; 

    // make the new measurement
    boolean newMeasurement = digitalRead(inputs[i].pinNumber);

    // invert so that true means the switch is closed
    newMeasurement = !newMeasurement; 

    // store it    
    if (newMeasurement) {
      currentByte |= (1<<bitCounter);
    } 
    else {
      currentByte &= ~(1<<bitCounter);
    }
    inputs[i].measurementBuffer[byteCounter] = currentByte;
  }
}

///////////////////////////
// UPDATE BUFFER SUMS
///////////////////////////
void updateBufferSums() {

  // the bufferSum is a running tally of the entire measurementBuffer
  // add the new measurement and subtract the old one

  for (int i=0; i<NUM_INPUTS; i++) {
    byte currentByte = inputs[i].measurementBuffer[byteCounter];
    boolean currentMeasurement = (currentByte >> bitCounter) & 0x01; 
    if (currentMeasurement) {
      inputs[i].bufferSum++;
    }
    if (inputs[i].oldestMeasurement) {
      inputs[i].bufferSum--;
    }
  }  
}

///////////////////////////
// UPDATE BUFFER INDEX
///////////////////////////
void updateBufferIndex() {
  bitCounter++;
  if (bitCounter == 8) {
    bitCounter = 0;
    byteCounter++;
    if (byteCounter == BUFFER_LENGTH) {
      byteCounter = 0;
    }
  }
}

///////////////////////////
// UPDATE INPUT STATES
///////////////////////////
void updateInputStates() {
  inputChanged = false;
  for (int i=0; i<NUM_INPUTS; i++) {
    inputs[i].prevPressed = inputs[i].pressed; // store previous pressed state (only used for mouse buttons)
    if (inputs[i].pressed) {
      if (inputs[i].bufferSum < releaseThreshold) {  
        inputChanged = true;
        inputs[i].pressed = false;
        if (inputs[i].isKey) {
//          BeanHid.releaseKey(inputs[i].keyCode);
          // turn off note on MIDI channel 0 with volume of 100 (0-127)
          BeanMidi.noteOff(CHANNEL0, inputs[i].keyCode, 100);
        }
      }
    } 
    else if (!inputs[i].pressed) {
      if (inputs[i].bufferSum > pressThreshold) {  // input becomes pressed
        inputChanged = true;
        inputs[i].pressed = true; 
        if (inputs[i].isKey) {
//          BeanHid.holdKey(inputs[i].keyCode);
          // play note on MIDI channel 0 with volume of 100 (0-127)
          BeanMidi.noteOn(CHANNEL0, inputs[i].keyCode, 100);
        }
      }
    }
  }
#ifdef DEBUG3
  if (inputChanged) {
    Serial.println("change");
  }
#endif
}

///////////////////////////
// ADD DELAY
///////////////////////////
void addDelay() {

  loopTime = micros() - prevTime;
  if (loopTime < TARGET_LOOP_TIME) {
    int wait = TARGET_LOOP_TIME - loopTime;
    delayMicroseconds(wait);
  }

  prevTime = micros();

#ifdef DEBUG_TIMING
  if (loopCounter == 0) {
    int t = micros()-prevTime;
    Serial.println(t);
  }
  loopCounter++;
  loopCounter %= 999;
#endif

}

/////////////////////////////
//// CYCLE LEDS
/////////////////////////////
//void cycleLEDs() {
//  pinMode(inputLED_a, INPUT);
//  pinMode(inputLED_b, INPUT);
//  pinMode(inputLED_c, INPUT);
//  digitalWrite(inputLED_a, LOW);
//  digitalWrite(inputLED_b, LOW);
//  digitalWrite(inputLED_c, LOW);
//
//  ledCycleCounter++;
//  ledCycleCounter %= 6;
//
//  if ((ledCycleCounter == 0) && inputs[0].pressed) {
//    pinMode(inputLED_a, INPUT);
//    digitalWrite(inputLED_a, LOW);
//    pinMode(inputLED_b, OUTPUT);
//    digitalWrite(inputLED_b, HIGH);
//    pinMode(inputLED_c, OUTPUT);
//    digitalWrite(inputLED_c, LOW);
//  }
//  if ((ledCycleCounter == 1) && inputs[1].pressed) {
//    pinMode(inputLED_a, OUTPUT);
//    digitalWrite(inputLED_a, HIGH);
//    pinMode(inputLED_b, OUTPUT);
//    digitalWrite(inputLED_b, LOW);
//    pinMode(inputLED_c, INPUT);
//    digitalWrite(inputLED_c, LOW);
//
//  }
//  if ((ledCycleCounter == 2) && inputs[2].pressed) {
//    pinMode(inputLED_a, OUTPUT);
//    digitalWrite(inputLED_a, LOW);
//    pinMode(inputLED_b, OUTPUT);
//    digitalWrite(inputLED_b, HIGH);
//    pinMode(inputLED_c, INPUT);
//    digitalWrite(inputLED_c, LOW);
//  }
//  if ((ledCycleCounter == 3) && inputs[3].pressed) {
//    pinMode(inputLED_a, INPUT);
//    digitalWrite(inputLED_a, LOW);
//    pinMode(inputLED_b, OUTPUT);
//    digitalWrite(inputLED_b, LOW);
//    pinMode(inputLED_c, OUTPUT);
//    digitalWrite(inputLED_c, HIGH);
//  }
//  if ((ledCycleCounter == 4) && inputs[4].pressed) {
//    pinMode(inputLED_a, OUTPUT);
//    digitalWrite(inputLED_a, LOW);
//    pinMode(inputLED_b, INPUT);
//    digitalWrite(inputLED_b, LOW);
//    pinMode(inputLED_c, OUTPUT);
//    digitalWrite(inputLED_c, HIGH);
//  }
//  if ((ledCycleCounter == 5) && inputs[5].pressed) {
//    pinMode(inputLED_a, OUTPUT);
//    digitalWrite(inputLED_a, HIGH);
//    pinMode(inputLED_b, INPUT);
//    digitalWrite(inputLED_b, LOW);
//    pinMode(inputLED_c, OUTPUT);
//    digitalWrite(inputLED_c, LOW);
//  }
//
//}
//
/////////////////////////////
//// DANCE LEDS
/////////////////////////////
//void danceLeds()
//{
//  int delayTime = 50;
//  int delayTime2 = 100;
//
//  // CIRCLE
//  for(int i=0; i<4; i++)
//  {
//    // UP
//    pinMode(inputLED_a, INPUT);
//    digitalWrite(inputLED_a, LOW);
//    pinMode(inputLED_b, OUTPUT);
//    digitalWrite(inputLED_b, HIGH);
//    pinMode(inputLED_c, OUTPUT);
//    digitalWrite(inputLED_c, LOW);
//    delay(delayTime);
//
//    //RIGHT
//    pinMode(inputLED_a, INPUT);
//    digitalWrite(inputLED_a, LOW);
//    pinMode(inputLED_b, OUTPUT);
//    digitalWrite(inputLED_b, LOW);
//    pinMode(inputLED_c, OUTPUT);
//    digitalWrite(inputLED_c, HIGH);
//    delay(delayTime);
//
//
//    // DOWN
//    pinMode(inputLED_a, OUTPUT);
//    digitalWrite(inputLED_a, HIGH);
//    pinMode(inputLED_b, OUTPUT);
//    digitalWrite(inputLED_b, LOW);
//    pinMode(inputLED_c, INPUT);
//    digitalWrite(inputLED_c, LOW);
//    delay(delayTime);
//
//    // LEFT
//    pinMode(inputLED_a, OUTPUT);
//    digitalWrite(inputLED_a, LOW);
//    pinMode(inputLED_b, OUTPUT);    
//    digitalWrite(inputLED_b, HIGH);
//    pinMode(inputLED_c, INPUT);
//    digitalWrite(inputLED_c, LOW);
//    delay(delayTime);    
//  }    
//
//  // WIGGLE
//  for(int i=0; i<4; i++)
//  {
//    // SPACE
//    pinMode(inputLED_a, OUTPUT);
//    digitalWrite(inputLED_a, HIGH);
//    pinMode(inputLED_b, INPUT);
//    digitalWrite(inputLED_b, LOW);
//    pinMode(inputLED_c, OUTPUT);
//    digitalWrite(inputLED_c, LOW);
//    delay(delayTime2);    
//
//    // CLICK
//    pinMode(inputLED_a, OUTPUT);
//    digitalWrite(inputLED_a, LOW);
//    pinMode(inputLED_b, INPUT);
//    digitalWrite(inputLED_b, LOW);
//    pinMode(inputLED_c, OUTPUT);
//    digitalWrite(inputLED_c, HIGH);
//    delay(delayTime2);    
//  }
//}
//
//void updateOutLEDs()
//{
//  boolean keyPressed = 0;
//  boolean mousePressed = 0;
//
//  for (int i=0; i<NUM_INPUTS; i++)
//  {
//    if (inputs[i].pressed)
//    {
//      if (inputs[i].isKey)
//      {
//        keyPressed = 1;
//#ifdef DEBUG
//        Serial.print("Key ");
//        Serial.print(i);
//        Serial.println(" pressed");
//#endif
//      }
//      else
//      {
//        mousePressed = 1;
//      }
//    }
//  }
//
//  if (keyPressed)
//  {
//    digitalWrite(outputK, HIGH);
//    TXLED1;
//  }
//  else
//  {
//    digitalWrite(outputK, LOW);
//    TXLED0;
//  }
//
//  if (mousePressed)
//  {
//    digitalWrite(outputM, HIGH);
//    RXLED1;
//  }
//  else
//  {
//    digitalWrite(outputM, LOW);
//    RXLED0;
//  }
//}






