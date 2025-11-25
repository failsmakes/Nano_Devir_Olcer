#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Button2.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// IR sensör pin (Nano’da INT0 = D2)
const uint8_t IR_PIN = 2;

// Buton pini (D3)
Button2 btn(3);

// RPM filtre parametreleri
const int FILTER_SIZE = 5;
unsigned long rpmBuffer[FILTER_SIZE];
int filterIndex = 0;
bool bufferFilled = false;

volatile unsigned long pulseCount = 0;
unsigned long lastMillis = 0;
unsigned long maxRPM = 0;

unsigned long rpm_filtered = 0;

// -------------------------
// INTERRUPT CALLBACK
// -------------------------
void onPulse() {
    pulseCount++;
}

// -------------------------
// MOVING AVERAGE FILTER
// -------------------------
unsigned long getFilteredRPM(unsigned long newRPM) {
    rpmBuffer[filterIndex] = newRPM;
    filterIndex++;

    if (filterIndex >= FILTER_SIZE) {
        filterIndex = 0;
        bufferFilled = true;
    }

    unsigned long total = 0;
    int count = bufferFilled ? FILTER_SIZE : filterIndex;

    for (int i = 0; i < count; i++) {
        total += rpmBuffer[i];
    }

    return total / count;
}

// -------------------------
// EKRANI GÜNCELLE
// -------------------------
void updDISPLAY() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("RPM:");
    display.println(rpm_filtered);

    display.setCursor(0, 30);
    display.print("MAX:");
    display.println(maxRPM);

    display.display();
}

// -------------------------
// BUTTON CLICK HANDLER
// -------------------------
void resetMaxRPM(Button2& btn) {
    maxRPM = 0;
    updDISPLAY();
}

void setup() {
    Wire.begin(); // Nano: SDA=A4, SCL=A5

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("RPM Meter");
    display.display();
    delay(1000);

    // IR sensör interrupt
    pinMode(IR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(IR_PIN), onPulse, FALLING);

    // BUTTON2 setup
    btn.setClickHandler(resetMaxRPM);
}

void loop() {
    btn.loop();

    unsigned long currentMillis = millis();

    if (currentMillis - lastMillis >= 1000) {
        lastMillis = currentMillis;

        unsigned long pulses = pulseCount;
        pulseCount = 0;

        unsigned long rpm_raw = pulses * 60;
        rpm_filtered = getFilteredRPM(rpm_raw);

        if (rpm_filtered > maxRPM) maxRPM = rpm_filtered;

        updDISPLAY();
    }
}

/*
| Arduino Nano | SSD1306 |
|-------------|---------|
| A5 (SCL)    | SCL     |
| A4 (SDA)    | SDA     |
| 5V          | VCC     |
| GND         | GND     |

| Arduino Nano | IR Sensör |
|--------------|-----------|
| D2 (INT0)    | OUT       |
| 5V           | VCC       |
| GND          | GND       |

| Arduino Nano | Buton |
|--------------|--------|
| D3           | OUT    |
| GND          | Diğer  |
*/
