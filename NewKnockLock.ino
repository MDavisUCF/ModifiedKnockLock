// Modified Arduino Project 12 Knock Lock 
/// by M.Davis Fall 2025

// This is a modified version of the Knock Lock project that replaces a piezo with a tilt switch.

// Make sure to have the Servo library installed in the IDE
#include <Servo.h>

// Defining the components
Servo myServo;

const int buttonPin = 2;   // Button Switch
const int yellowLed = 3;   // Standard Yellow LED
const int greenLed = 4;    // Standard Green LED
const int redLed   = 5;    // Standard Red LED
const int switchPin = 6;   // Tilt Switch

bool locked = false;
int numberOfKnocks = 0; // Starting number of knocks

// Debouncing to not have a super sensitive tilt sensor
const unsigned long sampleIntervalMs = 2;   // sampling cadence for debounce
const int lowSamplesNeeded  = 5;            // consecutive LOW samples to confirm press (≈10ms)
const int highSamplesNeeded = 5;            // consecutive HIGH samples to confirm release (≈10ms)
const unsigned long minLowMs   = 15;        // ignore ultra-fast spikes
const unsigned long maxLowMs   = 1200;      // ignore “held” tilt beyond ~1.2s
const unsigned long refractoryMs = 250;     // ignore new knocks right after a valid one
const unsigned long resetTimeout  = 5000;   // sequence timeout
const unsigned long knockDebounceGuard = 0; // extra guard if needed

int consecutiveLow  = 0;
int consecutiveHigh = 0;
bool lowConfirmed   = false;
unsigned long lowStartMs = 0;
unsigned long inhibitUntil = 0;

unsigned long lastKnockTime = 0;

// Setting up locking, unlocking and timeout/reset
void lockBox();
void unlockBox();
void warnAndReset();

// Returns true exactly once per valid knock
bool detectKnock() {
  unsigned long now = millis();
  if (now < inhibitUntil) return false;

  int raw = digitalRead(switchPin); // When there is a tilt, the state changes from HIGH to LOW

  if (raw == LOW) {
    consecutiveLow++;
    consecutiveHigh = 0;
  } else {
    consecutiveHigh++;
    consecutiveLow = 0;
  }

  // Confirm LOW (press)
  if (!lowConfirmed && consecutiveLow >= lowSamplesNeeded) {
    lowConfirmed = true;
    lowStartMs = now;
  }

  // Confirm HIGH (release) after a confirmed LOW
  if (lowConfirmed && consecutiveHigh >= highSamplesNeeded) {
    lowConfirmed = false;
    unsigned long lowDur = now - lowStartMs;

    // Qualify the LOW duration as a valid "knock"
    if (lowDur >= minLowMs && lowDur <= maxLowMs) {
      inhibitUntil = now + refractoryMs;  // cooldown to ignore bounce clusters
      lastKnockTime = now;
      return true;
    }
  }

  return false;
}

void setup() {
  myServo.attach(9); // Servo connected to Digital Pin 9

  pinMode(yellowLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);

  pinMode(buttonPin, INPUT);

  pinMode(switchPin, INPUT_PULLUP);

  Serial.begin(9600); // Start serial monitor

  // GREEN LED ON -- BOX UNLOCKED
  digitalWrite(greenLed, HIGH);
  digitalWrite(redLed, LOW);
  digitalWrite(yellowLed, LOW);
  myServo.write(0);
  Serial.println("the box is unlocked!");
}

void loop() {
  // If the box is Locked, 
  if (!locked) {
    int buttonVal = digitalRead(buttonPin);
    if (buttonVal == HIGH) {
      lockBox();
    }
    return;
  }

  // If a knock is detected from tilt sensor, show in serial monitor as Kock #
  if (detectKnock()) {
    numberOfKnocks++;
    Serial.print("Knock #");
    Serial.println(numberOfKnocks);

  
// If there are only 2 knocks, turn on Yellow LED
    if (numberOfKnocks == 2) {
      digitalWrite(yellowLed, HIGH); // steady at 2 knocks
      Serial.println("Yellow LED ON (2 knocks)");
    }
  
// If there are 3 knocks, unlock the box and turn LED to green (run unlockbox)
    if (numberOfKnocks == 3) {
      unlockBox();
      return;
    }
  }

  // Timeout reset ... if no knocks detected, flash yellow LED and reset knock count (run warnAndReset)
  if (numberOfKnocks > 0 && (millis() - lastKnockTime > resetTimeout)) {
    Serial.println("Sequence timed out");
    warnAndReset();
  }

  // Small delay to help with balancing tilt switch false knocks
  delay(sampleIntervalMs);
}

// Custom Functions
// These are our own helper actions.
// Each one groups related steps so we can call it from loop() with a single line.

// lockBox() Locks the servo and updates LEDs to only Red on
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

// unlockBoc() Unlocks the servo and updates LEDs to only Green on
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

// warnAndReset() Flashes the yellow LED and resets knock count
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
