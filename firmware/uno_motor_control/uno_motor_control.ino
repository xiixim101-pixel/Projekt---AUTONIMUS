/*
 * Projekt Robot VR-Telemetria, RC - Marcin Słowik 2026
 * Projekt_AUTONIMUS - Arduino UNO - sterowanie silnikami z RC (PWM)
 */

// --- Piny mostka H ---
const int pwmForward = 9;   // napęd przód
const int pwmBackward = 6;  // napęd tył
const int steerLeft  = 11;  // skręt w lewo
const int steerRight = 10;  // skręt w prawo

// --- Kanały RC ---
const int rcPins[2] = {2, 3}; // D2, D3 = INT0, INT1
volatile uint16_t rcValue[2] = {1500, 1500};
volatile uint32_t lastRise[2] = {0, 0};

// --- Zmienne do ramek z ESP ---
enum {WAIT_HEADER, RECV_DATA, WAIT_FOOTER};
uint8_t state = WAIT_HEADER, bufIndex = 0;
int8_t buffer[4];
bool espFrame = false;

// --- Parametry RC ---
const int neutral = 1500;
const int deadband = 50;

// ISR pomiaru impulsów RC
void rcISR0() {
  if (digitalRead(rcPins[0]) == HIGH) lastRise[0] = micros();
  else rcValue[0] = micros() - lastRise[0];
}
void rcISR1() {
  if (digitalRead(rcPins[1]) == HIGH) lastRise[1] = micros();
  else rcValue[1] = micros() - lastRise[1];
}

void setup() {
  Serial.begin(9600);

  pinMode(pwmForward, OUTPUT);
  pinMode(pwmBackward, OUTPUT);
  pinMode(steerLeft, OUTPUT);
  pinMode(steerRight, OUTPUT);

  // RC input
  for (int i=0;i<2;i++) pinMode(rcPins[i], INPUT);
  attachInterrupt(digitalPinToInterrupt(rcPins[0]), rcISR0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rcPins[1]), rcISR1, CHANGE);

  Serial.println("UNO: PWM napęd + PWM skręt");
}

void loop() {
  // --- Odbiór ramki z ESP ---
  while (Serial.available()) {
    int8_t c = (int8_t)Serial.read();
    switch (state) {
      case WAIT_HEADER: if (c == (int8_t)0xFF) { state = RECV_DATA; bufIndex = 0; } break;
      case RECV_DATA: buffer[bufIndex++] = c; if (bufIndex == 4) state = WAIT_FOOTER; break;
      case WAIT_FOOTER: if (c == (int8_t)0xFE) espFrame = true; state = WAIT_HEADER; break;
    }
  }

  int motorPWM = 0;   // przód/tył
  int steerPWM = 0;   // lewo/prawo (signed)

  bool rcActive = (rcValue[0] > 900 && rcValue[0] < 2100);

  if (rcActive) {
    //  Gaz przód/tył
    if (rcValue[0] > neutral + deadband)
      motorPWM = map(rcValue[0], neutral + deadband, 2000, 0, 255);
    else if (rcValue[0] < neutral - deadband)
      motorPWM = -map(rcValue[0], neutral - deadband, 1000, 0, 255);

    //  Skręt lewo/prawo (PWM)
    if (rcValue[1] > neutral + deadband)
      steerPWM = map(rcValue[1], neutral + deadband, 2000, 0, 255);   // prawo
    else if (rcValue[1] < neutral - deadband)
      steerPWM = -map(rcValue[1], neutral - deadband, 1000, 0, 255); // lewo

  } else if (espFrame) {
    //  Dane z ESP32
    motorPWM = buffer[0]; // napęd
    steerPWM = buffer[1]; // skręt
    espFrame = false;
  }

  // --- Ograniczenia ---
  if (motorPWM > 255) motorPWM = 255;
  if (motorPWM < -255) motorPWM = -255;
  if (steerPWM > 255) steerPWM = 255;
  if (steerPWM < -255) steerPWM = -255;

  // --- Silnik napędowy (mostek H) ---
  if (motorPWM > 0) {
    analogWrite(pwmForward, motorPWM);
    analogWrite(pwmBackward, 0);
  } else if (motorPWM < 0) {
    analogWrite(pwmForward, 0);
    analogWrite(pwmBackward, -motorPWM);
  } else {
    analogWrite(pwmForward, 0);
    analogWrite(pwmBackward, 0);
  }

  // --- Silnik skrętu (mostek H) ---
  if (steerPWM > 0) {
    analogWrite(steerRight, steerPWM);
    analogWrite(steerLeft, 0);
  } else if (steerPWM < 0) {
    analogWrite(steerRight, 0);
    analogWrite(steerLeft, -steerPWM);
  } else {
    analogWrite(steerRight, 0);
    analogWrite(steerLeft, 0);
  }
}