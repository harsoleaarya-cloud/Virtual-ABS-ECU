#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int LEFT_PIN = 34, RIGHT_PIN = 35, BRAKE_PIN = 32, BTN_PIN = 4;
const int LED_OK = 25, LED_ABS = 26, LED_FAULT = 27, LED_PRESS = 18, BUZZER = 33;

const int BRAKE_MIN = 20;    // % brake pressure that counts as braking
const int SLIP_LIMIT = 20;   // % slip that triggers ABS
const int MOVING_MIN = 10;   // % speed below which ABS is ignored

enum State { NORMAL, ABS_ACTIVE, FAULT };
State state = NORMAL;

bool faultLatched = false;
int lastBtn = HIGH;
unsigned long lastPress = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void showLine(int row, String text) {
  while (text.length() < 16) text += ' ';
  lcd.setCursor(0, row);
  lcd.print(text);
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_OK, OUTPUT);
  pinMode(LED_ABS, OUTPUT);
  pinMode(LED_FAULT, OUTPUT);
  pinMode(LED_PRESS, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  lcd.init();
  lcd.backlight();
  showLine(0, "Virtual ABS ECU");
  delay(1000);
  lcd.clear();
}

void loop() {
  int btn = digitalRead(BTN_PIN);
  if (btn == LOW && lastBtn == HIGH && millis() - lastPress > 200) {
    faultLatched = !faultLatched;
    lastPress = millis();
  }
  lastBtn = btn;

  int left  = map(analogRead(LEFT_PIN),  0, 4095, 0, 100);
  int right = map(analogRead(RIGHT_PIN), 0, 4095, 0, 100);
  int brake = map(analogRead(BRAKE_PIN), 0, 4095, 0, 100);

  int refSpeed = max(left, right);
  int slowest  = min(left, right);
  int slip = (refSpeed > 0) ? ((refSpeed - slowest) * 100) / refSpeed : 0;

  if (faultLatched) state = FAULT;
  else if (brake > BRAKE_MIN && refSpeed > MOVING_MIN && slip > SLIP_LIMIT) state = ABS_ACTIVE;
  else state = NORMAL;

  digitalWrite(LED_OK,    state == NORMAL);
  digitalWrite(LED_ABS,   state == ABS_ACTIVE);
  digitalWrite(LED_FAULT, state == FAULT);
  digitalWrite(LED_PRESS, brake > BRAKE_MIN && state != FAULT);

  if (state == FAULT) tone(BUZZER, 2000);
  else if (state == ABS_ACTIVE) tone(BUZZER, 800);
  else noTone(BUZZER);

  if (state == FAULT) {
    showLine(0, "SENSOR FAULT!");
    showLine(1, "ABS DISABLED");
  } else {
    showLine(0, "L:" + String(left) + " R:" + String(right) + " B:" + String(brake));
    showLine(1, (state == ABS_ACTIVE ? "ABS ACTIVE S:" : "NORMAL   S:") + String(slip) + "%");
  }

  Serial.printf("%d,%d,%d,%d,%d\n", left, right, brake, slip, (int)state);
  delay(150);
}
