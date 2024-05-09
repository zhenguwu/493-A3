#include <Mouse.h>
#include <Keyboard.h>
#include <Wire.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

#define CLICKPIN 5
#define TOGGLEPIN 4 
#define CLICKTHRESHHOLD 150
#define DEBOUNCE_DELAY 50
#define DOUBLE_PRESS_MAX 300  // Max time between presses for a double press

// I2C
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

const int g = 9.8;
const float mouseThreshold = 0.2;  // Threshold for tilt to start moving mouse
const float keyThreshold = 6; // Threshold for tilt to start pressing keys

unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long lastPressTime = 0;    // the last time the button was pressed
bool buttonState = HIGH;            // the current reading from the input pin
bool lastButtonState = HIGH;        // the previous reading from the input pin
bool waitingForDoublePress = false;

void setup(void) {
  pinMode(CLICKPIN, INPUT_PULLUP);
  pinMode(TOGGLEPIN, INPUT_PULLUP);

  // Serial.begin(9600);
  Mouse.begin();
  Keyboard.begin();
  // while (!Serial) delay(10);

  if (! lis.begin(0x18)) {
    Serial.println("Couldnt start");
    while (1) yield();
  }

  lis.setRange(LIS3DH_RANGE_2_G);
  delay(100);
}

bool mode = true;
bool brush = true;
void loop() {

  // Read the toggle pin to see if we need to change modes
  bool reading = digitalRead(TOGGLEPIN);

  // Check if the button state has changed
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // if the button state has changed after the debounce delay
    if (reading != buttonState) {
      buttonState = reading;

      // Button press detected
      if (buttonState == LOW) {
        if (!waitingForDoublePress) {
          waitingForDoublePress = true;
          lastPressTime = millis();
        } else if ((millis() - lastPressTime) <= DOUBLE_PRESS_MAX) {
          // Double press detected, toggle between brush and eraser
          if (brush) {
            Keyboard.press('e');
            Keyboard.release('e');
            brush = false;
          } else {
            Keyboard.press('b');
            Keyboard.release('b');
            brush = true;
          }
          waitingForDoublePress = false;
        }
      }
    }
  }

  // Handle single press if no second press has occurred within DOUBLE_PRESS_MAX time
  if (waitingForDoublePress && (millis() - lastPressTime) > DOUBLE_PRESS_MAX) {
    mode = !mode;
    waitingForDoublePress = false;
  }

  // Save the reading for next time
  lastButtonState = reading;

  lis.read();
  sensors_event_t event;
  lis.getEvent(&event);
  float accX = event.acceleration.x;
  float accY = event.acceleration.y;
  float accZ = event.acceleration.z;

  /* Display the results */
  // Serial.print("\t\tX: "); Serial.print(accX);
  // Serial.print(" \tY: "); Serial.print(accY);
  // Serial.print(" \tZ: "); Serial.print(accZ);

  // Serial.println();

  if (mode) {
    // Move the mouse based on tilt
    int mouseX = 0;
    int mouseY = 0;

    // Scale the movement
    if (abs(accX) > mouseThreshold) {
      mouseX = (accX > 0 ? -1 : 1) * map(abs(accX), 0, g, 0, 10);
    }
    if (abs(accY) > mouseThreshold) {
      mouseY = (accY > 0 ? 1 : -1) * map(abs(accY), 0, g, 0, 10);
    }

    Mouse.move(mouseX, mouseY);

    // Click if button is pressed
    int clickButton = digitalRead(CLICKPIN);
    if (clickButton) {
      Mouse.release(MOUSE_LEFT);
    } else {
      Mouse.press(MOUSE_LEFT);
    }
  } else {
    // Send arrow keys to change brush color and weight
    // Control the UP and DOWN arrow keys based on Y tilt
    if (accY < -keyThreshold) {
        Keyboard.press(KEY_UP_ARROW);
    } else if (accY > keyThreshold) {
        Keyboard.press(KEY_DOWN_ARROW);
    } else if (accY < keyThreshold/2 || accY > -keyThreshold/2) {
        // Release both if within the threshold
        Keyboard.release(KEY_UP_ARROW);
        Keyboard.release(KEY_DOWN_ARROW);
    }

    // Control the LEFT and RIGHT arrow keys based on X tilt
    if (accX < -keyThreshold) {
        Keyboard.press(KEY_LEFT_ARROW);
    } else if (accX > keyThreshold) {
        Keyboard.press(KEY_RIGHT_ARROW);
    } else if (accX < keyThreshold/2 || accX > -keyThreshold/2) {
        Keyboard.release(KEY_LEFT_ARROW);
        Keyboard.release(KEY_RIGHT_ARROW);
    }
  }

}
