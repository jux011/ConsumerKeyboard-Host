// waveshare pizero with gamepi hat
static_assert(ARDUINO_ARCH_RP2040);

#define PIN_BUTTON1_IN 21
#define PIN_BUTTON2_IN 20
#define PIN_BUTTON3_IN 16

#define PIN_JOYSTICK_UP_IN 9 // no longer 6
#define PIN_JOYSTICK_DOWN_IN 19
#define PIN_JOYSTICK_LEFT_IN 5
#define PIN_JOYSTICK_RIGHT_IN 26
#define PIN_JOYSTICK_PRESS_IN 13

#define PIN_USB_HOST_DP 6
// #define PIN_USB_HOST_DM 7 // implicit

static void set_pinMode() {
  pinMode(PIN_BUTTON1_IN, INPUT_PULLUP);
  pinMode(PIN_BUTTON2_IN, INPUT_PULLUP);
  pinMode(PIN_BUTTON3_IN, INPUT_PULLUP);

  pinMode(PIN_JOYSTICK_UP_IN, INPUT_PULLUP);
  pinMode(PIN_JOYSTICK_DOWN_IN, INPUT_PULLUP);
  pinMode(PIN_JOYSTICK_LEFT_IN, INPUT_PULLUP);
  pinMode(PIN_JOYSTICK_RIGHT_IN, INPUT_PULLUP);
  pinMode(PIN_JOYSTICK_PRESS_IN, INPUT_PULLUP);
}

static void set_output_poweron() {
  // all 8 buttons are open-pullup closed-ground
}

// set_pinMode();
// set_output_poweron();
