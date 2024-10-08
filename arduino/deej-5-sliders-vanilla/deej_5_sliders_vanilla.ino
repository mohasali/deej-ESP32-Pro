#include <BluetoothSerial.h>

const int MIN_CHANGE = 20;
const int NUM_SLIDERS = 4;
const int analogInputs[NUM_SLIDERS] = {26, 33, 32, 35};

const int NUM_BUTTONS = 1;
const int buttonInputs[NUM_BUTTONS] = {15};

int analogSliderValues[NUM_SLIDERS];
int buttonValues[NUM_BUTTONS];
int sentButtonValues[NUM_BUTTONS];

const uint8_t CONNECTION_NONE = 0;
const uint8_t CONNECTION_USB = 1;
const uint8_t CONNECTION_BT = 2;
uint8_t connection = CONNECTION_NONE;

bool isConnected = false;
BluetoothSerial SerialBT;

/* Check if Bluetooth configurations are enabled in the SDK */
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

void setup() { 
  SerialBT.begin();
  Serial.begin(9600);
  analogReadResolution(10);  // 10Bit resolution
  delay(1000);
}

void loop() {
  checkSerialConnection(Serial, CONNECTION_USB);
  checkSerialConnection(SerialBT, CONNECTION_BT);

  if (connection == CONNECTION_BT && !SerialBT.hasClient()) {
    connection = CONNECTION_NONE;
    return;
  }

  sendValues("Slider", analogSliderValues, updateSliderValues());
  sendValues("Button", sentButtonValues, checkButtonStates());
  // printSliderValues(); // For debug
  delay(50);
}
bool updateSliderValues() {
  bool valueChange = false;
  for (int potIndex = 0; potIndex < NUM_SLIDERS; potIndex++) {
    // Take a new reading
    int newValue = analogRead(analogInputs[potIndex]);
    
    // Adjust threshold for detecting a change in the slider value
    if (abs(newValue - analogSliderValues[potIndex]) > MIN_CHANGE) {
      // Clamp the values between 0 and 1023
      if (newValue + MIN_CHANGE > 1023) {
        analogSliderValues[potIndex] = 1023;
      } else if (newValue - MIN_CHANGE < 0) {
        analogSliderValues[potIndex] = 0;
      } else {
        analogSliderValues[potIndex] = newValue;
      }

      valueChange = true;
    }
  }
  return valueChange;
}


int median(int *array, int size) {
  int temp;
  for (int i = 0; i < size - 1; i++) {
    for (int j = i + 1; j < size; j++) {
      if (array[j] < array[i]) {
        temp = array[i];
        array[i] = array[j];
        array[j] = temp;
      }
    }
  }
  return array[size / 2];
}

void printSliderValues() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    String printedString = String("Slider #") + String(i + 1) + String(": ") + String(analogSliderValues[i]) + String(" mV");
    Serial.write(printedString.c_str());

    if (i < NUM_SLIDERS - 1) {
      Serial.write(" | ");
    } else {
      Serial.write("\n");
    }
  }
}

bool checkButtonStates() {
  bool valueChange = false;
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int newVal = digitalRead(buttonInputs[i]);
    int oldVal = buttonValues[i];
    if (newVal == 1 && oldVal == 0) {
      sentButtonValues[i] = 1;
      valueChange = true;
    } else {
      sentButtonValues[i] = 0;
    }
    buttonValues[i] = newVal;
  }
  return valueChange;
}

void sendValues(String type, int* values, bool valueChange) {
  if (valueChange) {
    String builtString = type + "|";

    for (int i = 0; i < NUM_SLIDERS; i++) {
      builtString += String(values[i]);

      if (i < NUM_SLIDERS - 1) {
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
    // Define a string to store the received data
    String data;
    // Read and store the received data
    data = serial.readString();
    // Clean the data for comparison
    data.trim();
    // Check if the received data matches
    if (data == "connect") {
      // Send a response if the data matches
      serial.println("yes");
      connection = connectionType;
      // Set a delay to ensure connection is properly established
      delay(1000);
      updateSliderValues();
      sendValues("Slider", analogSliderValues, true);
    }
  }
}
