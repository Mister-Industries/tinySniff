/*
 * Project: tinySniff - tinySniff_Monitor
 * Author: Geoff McIntyre (w/ help from Claude)
 * Revision Date: 3/17/26
 * License: GNU General Public License v3.0
 *
 * Description:
 *   Reads all three tinySniff MEMS gas sensors and streams data to the
 *   Serial Monitor. Output is Serial Plotter compatible — open
 *   Tools > Serial Plotter to watch a live graph of all three channels.
 *
 *   Sensors:
 *     CH4  (Methane / Combustible Gas) — A0 — GM-402B
 *     H2S  (Hydrogen Sulfide)          — A1 — GM-602B
 *     H2   (Hydrogen)                  — A2 — GM-2021B
 *
 * Requirements:
 *   - Libraries: none (raw ADC only)
 *   - Hardware: tinyCore ESP32-S3 + tinySniff HAT
 *
 * Controls:
 *   [r]  Print a single reading
 *   [u]  Toggle units — raw ADC (0-4095) vs millivolts
 *   [?]  Show menu
 *   [Button]  Toggle streaming on/off
 */

#include <Arduino.h>

// --- PINS ---
#define PIN_CH4     A0   // GM-402B  — Methane / Combustible Gas
#define PIN_H2S     A1   // GM-602B  — Hydrogen Sulfide
#define PIN_H2      A2   // GM-2021B — Hydrogen
#define PIN_BUTTON  RX

// --- ADC CONFIG ---
#define ADC_BITS  12
#define ADC_MAX   4095.0f
#define VCC       3.3f

// --- STREAMING ---
#define STREAM_INTERVAL_MS  500  // 2 Hz — comfortable for Serial Plotter

// --- GLOBALS ---
bool streamEnabled = true;
bool showMv        = false;   // false = raw ADC, true = millivolts

unsigned long lastStreamTime = 0;

// Debounce
unsigned long lastDebounceTime = 0;
int lastButtonState = HIGH;

// ---------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------

float toMv(int raw) { return (raw / ADC_MAX) * VCC * 1000.0f; }

void printMenu() {
    Serial.println("\n--- tinySniff Monitor Menu ---");
    Serial.println("[r]  Single reading");
    Serial.println("[u]  Toggle units: raw ADC <-> millivolts");
    Serial.println("[?]  Show menu");
    Serial.println("[Button]  Toggle streaming on/off");
    Serial.println("Plotter labels: CH4, H2S, H2");
    Serial.println("------------------------------");
}

void printReading() {
    int ch4 = analogRead(PIN_CH4);
    int h2s = analogRead(PIN_H2S);
    int h2  = analogRead(PIN_H2);

    if (showMv) {
        // Serial Plotter format — labeled, comma-separated
        Serial.printf("CH4_mV:%.1f,H2S_mV:%.1f,H2_mV:%.1f\n",
            toMv(ch4), toMv(h2s), toMv(h2));
    } else {
        Serial.printf("CH4:%d,H2S:%d,H2:%d\n", ch4, h2s, h2);
    }
}

// ---------------------------------------------------------------
// Setup
// ---------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(2000); // Wait for Web Serial

    analogReadResolution(ADC_BITS);
    pinMode(PIN_CH4,    INPUT);
    pinMode(PIN_H2S,    INPUT);
    pinMode(PIN_H2,     INPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    Serial.println("tinySniff Monitor ready.");
    printMenu();
}

// ---------------------------------------------------------------
// Loop
// ---------------------------------------------------------------

void loop() {

    // 1. Timed streaming
    if (streamEnabled && (millis() - lastStreamTime >= STREAM_INTERVAL_MS)) {
        printReading();
        lastStreamTime = millis();
    }

    // 2. Serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 'r': printReading(); break;
            case 'u':
                showMv = !showMv;
                Serial.printf("Units: %s\n", showMv ? "millivolts" : "raw ADC");
                break;
            case '?': printMenu(); break;
        }
    }

    // 3. Button — toggle streaming (debounced)
    int reading = digitalRead(PIN_BUTTON);
    if (reading != lastButtonState) lastDebounceTime = millis();
    if ((millis() - lastDebounceTime) > 50) {
        static int buttonState = HIGH;
        if (reading != buttonState) {
            buttonState = reading;
            if (buttonState == LOW) {
                streamEnabled = !streamEnabled;
                Serial.println(streamEnabled ? "Streaming ON" : "Streaming OFF");
            }
        }
    }
    lastButtonState = reading;
}
