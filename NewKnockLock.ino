// Modified Arduino Project 12 Knock Lock 
/// by M.Davis Fall 2025

// This is a modified version of the Knock Lock project that replaces a piezo with a tilt switch.

#include <Servo.h>

Servo myServo;

const int buttonPin = 2;   // press to lock
const int yellowLed = 3;   // warning / 2-knock indicator
const int greenLed = 4;    // unlocked indicator
const int redLed   = 5;    // locked indicator
const int switchPin = 6;   // tilt switch to GND (INPUT_PULLUP)

bool locked = false;
int numberOfKnocks = 0;

// --- Robust debounce/detection parameters ---
const unsigned long sampleIntervalMs = 2;   // sampling cadence for debounce
const int lowSamplesNeeded  = 5;            // consecutive LOW samples to confirm press (≈10ms)
const int highSamplesNeeded = 5;            // consecutive HIGH samples to confirm release (≈10ms)
const unsigned long minLowMs   = 15;        // ignore ultra-fast spikes
const unsigned long maxLowMs   = 1200;      // ignore “held” tilt beyond ~1.2s
const unsigned long refractoryMs = 250;     // ignore new knocks right after a valid one
const unsigned long resetTimeout  = 5000;   // sequence timeout
const unsigned long knockDebounceGuard = 0; // extra guard if needed (usually 0 with this approach)

// state for debouncing
int consecutiveLow  = 0;
int consecutiveHigh = 0;
bool lowConfirmed   = false;
unsigned long lowStartMs = 0;
unsigned long inhibitUntil = 0;

unsigned long lastKnockTime = 0;

// --- forward declarations ---
void lockBox();
void unlockBox();
void warnAndReset();

// returns true exactly once per valid knock
bool detectKnock() {
  unsigned long now = millis();
  if (now < inhibitUntil) return false;

  int raw = digitalRead(switchPin); // INPUT_PULLUP: idle HIGH, tilt event LOW

  if (raw == LOW) {
    consecutiveLow++;
    consecutiveHigh = 0;
  } else {
    consecutiveHigh++;
    consecutiveLow = 0;
  }

  // confirm LOW (press)
  if (!lowConfirmed && consecutiveLow >= lowSamplesNeeded) {
    lowConfirmed = true;
    lowStartMs = now;
  }

  // confirm HIGH (release) after a confirmed LOW
  if (lowConfirmed && consecutiveHigh >= highSamplesNeeded) {
    lowConfirmed = false;
    unsigned long lowDur = now - lowStartMs;

    // qualify the LOW duration as a valid "knock"
    if (lowDur >= minLowMs && lowDur <= maxLowMs) {
      inhibitUntil = now + refractoryMs;  // cooldown to ignore bounce clusters
      lastKnockTime = now;
      return true;
    }
  }

  return false;
}

void setup() {
  myServo.attach(9);

  pinMode(yellowLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);

  // If your lock button goes to GND, use INPUT_PULLUP and invert the read below.
  pinMode(buttonPin, INPUT);

  // Tilt switch: one leg to D6, the other to GND
  pinMode(switchPin, INPUT_PULLUP);

  Serial.begin(9600);

  digitalWrite(greenLed, HIGH);
  digitalWrite(redLed, LOW);
  digitalWrite(yellowLed, LOW);
  myServo.write(0);
  Serial.println("the box is unlocked!");
}

void loop() {
  if (!locked) {
    int buttonVal = digitalRead(buttonPin);
    if (buttonVal == HIGH) {   // LOW if using INPUT_PULLUP wiring
      lockBox();
    }
    return;
  }

  // --- LOCKED: look for debounced knocks ---
  if (detectKnock()) {
    numberOfKnocks++;
    Serial.print("Knock #");
    Serial.println(numberOfKnocks);

    if (numberOfKnocks == 2) {
      digitalWrite(yellowLed, HIGH); // steady at 2 knocks
      Serial.println("Yellow LED ON (2 knocks)");
    }

    if (numberOfKnocks == 3) {
      unlockBox();
      return;
    }

    if (numberOfKnocks > 3) {
      warnAndReset();
    }
  }

  // Timeout reset
  if (numberOfKnocks > 0 && (millis() - lastKnockTime > resetTimeout)) {
    Serial.println("Sequence timed out");
    warnAndReset();
  }

  // fixed, small cadence to stabilize sampling
  delay(sampleIntervalMs);
}

// --- helpers ---
void lockBox() {
  locked = true;
  numberOfKnocks = 0;
  lowConfirmed = false;
  consecutiveLow = consecutiveHigh = 0;
  digitalWrite(yellowLed, LOW);
  digitalWrite(greenLed, LOW);
  digitalWrite(redLed, HIGH);
  myServo.write(90);
  Serial.println("the box is locked!");
}

void unlockBox() {
  locked = false;
  numberOfKnocks = 0;
  lowConfirmed = false;
  consecutiveLow = consecutiveHigh = 0;
  digitalWrite(yellowLed, LOW);
  digitalWrite(greenLed, HIGH);
  digitalWrite(redLed, LOW);
  myServo.write(0);
  Serial.println("the box is unlocked!");
}

// flash yellow 3× and reset count
void warnAndReset() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(yellowLed, HIGH);
    delay(200);
    digitalWrite(yellowLed, LOW);
    delay(200);
  }
  numberOfKnocks = 0;
  lowConfirmed = false;
  consecutiveLow = consecutiveHigh = 0;
  lastKnockTime = millis();
  inhibitUntil = lastKnockTime + refractoryMs;
  digitalWrite(yellowLed, LOW);
}