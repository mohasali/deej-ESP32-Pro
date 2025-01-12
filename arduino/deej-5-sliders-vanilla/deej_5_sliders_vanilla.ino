#include <BluetoothSerial.h>

const int MIN_CHANGE = 20;
const int NUM_SLIDERS = 4;
const int analogInputs[NUM_SLIDERS] = {26,33, 32, 35};

const int NUM_BUTTONS = 2;
const int buttonInputs[NUM_BUTTONS] = {4, 16};

const int DOUBLE_PRESS_THRESHOLD = 500; // Time threshold in milliseconds

int analogSliderValues[NUM_SLIDERS];
int buttonValues[NUM_BUTTONS];
int sentButtonValues[NUM_BUTTONS * 2];
unsigned long lastPressTime[NUM_BUTTONS]; // Timestamp of the last button press
float buttonPressBuffer[NUM_BUTTONS]; // Timestamp of the last button press

const uint8_t CONNECTION_NONE = 0;
const uint8_t CONNECTION_USB = 1;
const uint8_t CONNECTION_BT = 2;
uint8_t connection = CONNECTION_NONE;

BluetoothSerial SerialBT;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

void setup() {
  SerialBT.begin();
  Serial.begin(9600);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonInputs[i], INPUT_PULLDOWN);
  }
  analogReadResolution(10);
  delay(1000);
}

void loop() {
  checkSerialConnection(Serial, CONNECTION_USB);
  checkSerialConnection(SerialBT, CONNECTION_BT);

  if (connection == CONNECTION_BT && !SerialBT.hasClient()) {
    connection = CONNECTION_NONE;
    return;
  }

  sendValues("Slider", analogSliderValues, updateSliderValues(), NUM_SLIDERS);
  sendValues("Button", sentButtonValues, checkButtonStates(), NUM_BUTTONS * 2);
  delay(50);
}

bool updateSliderValues() {
  bool valueChange = false;
  for (int potIndex = 0; potIndex < NUM_SLIDERS; potIndex++) {
    int newValue = analogRead(analogInputs[potIndex]);
    if (abs(newValue - analogSliderValues[potIndex]) > MIN_CHANGE) {
      analogSliderValues[potIndex] = constrain(newValue, 0, 1023);
      valueChange = true;
    }
  }
  return valueChange;
}

bool checkButtonStates() {
  bool valueChange = false;
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int newVal = digitalRead(buttonInputs[i]);
    unsigned long currentTime = millis();

    // Check for single press
    if (newVal == 1 && buttonValues[i] == 0) {
      // Check for double press
      if (currentTime - lastPressTime[i] <= DOUBLE_PRESS_THRESHOLD) {
        sentButtonValues[NUM_BUTTONS + i] = 1;// Mark as double pressed
        sentButtonValues[i] = 0; 
        buttonPressBuffer[i] = DOUBLE_PRESS_THRESHOLD+1;
        valueChange = true;
      }
      else{
        buttonPressBuffer[i]=DOUBLE_PRESS_THRESHOLD;
      }

      lastPressTime[i] = currentTime; // Update last press time
    }
    else if(buttonPressBuffer[i]<=0){
      buttonPressBuffer[i] = DOUBLE_PRESS_THRESHOLD+1;
      sentButtonValues[i] = 1;
      valueChange = true;
      }
    else if(buttonPressBuffer[i]<=DOUBLE_PRESS_THRESHOLD){
      buttonPressBuffer[i]-=100;
      }
     else {
      sentButtonValues[i] = 0;
      sentButtonValues[NUM_BUTTONS + i] = 0; // Reset double press state
    }

    buttonValues[i] = newVal;
  }
  return valueChange;
}

void sendValues(String type, int* values, bool valueChange, int num) {
  if (valueChange) {
    String builtString = type + "|";

    for (int i = 0; i < num; i++) {
      builtString += String(values[i]);
      if (i < num - 1) {
        builtString += "|";
      }
    }
    switch (connection) {
      case CONNECTION_USB:
        Serial.println(builtString);
        break;
      case CONNECTION_BT:
        SerialBT.println(builtString);
        break;
    }
  }
}

void checkSerialConnection(Stream &serial, uint8_t connectionType) {
  if (serial.available()) {
    String data = serial.readString();
    data.trim();
    if (data == "connect") {
      serial.println("yes");
      connection = connectionType;
      delay(1000);
      updateSliderValues();
      sendValues("Slider", analogSliderValues, true, NUM_SLIDERS);
    }
  }
}
