#include "sleepSensor.h"
#include "Wire.h"
#include "esp_system.h"  // Required for ESP system functions

// Constants
#define uS_TO_S_FACTOR 1000000       // Conversion factor for microseconds to seconds
#define TIME_TO_SLEEP 1200            // Time to sleep in seconds

// RTC memory to retain data across deep sleep cycles
RTC_DATA_ATTR int bootCount = 0;

/**
 * @brief Prints the reason for waking up from deep sleep.
 */
void printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wakeup caused by external signal using RTC_IO");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("Wakeup caused by external signal using RTC_CNTL");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup caused by timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            Serial.println("Wakeup caused by touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            Serial.println("Wakeup caused by ULP program");
            break;
        default:
            Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
            break;
    }
}

/**
 * @brief Sets up the GPIO pins and initializes the I2C bus.
 */
void setupGPIO() {
    // Initial boot setup
    //gpio_hold_dis(GPIO_NUM_7);
    gpio_hold_dis(GPIO_NUM_3);
    gpio_hold_dis(GPIO_NUM_9);
    Wire.begin(GPIO_NUM_6, GPIO_NUM_12, 100000);  // Initialize I2C
    //pinMode(GPIO_NUM_7, OUTPUT);
    pinMode(GPIO_NUM_3, OUTPUT);
    pinMode(GPIO_NUM_9, OUTPUT);
    digitalWrite(GPIO_NUM_3, HIGH);
    //digitalWrite(GPIO_NUM_7, LOW);
    digitalWrite(GPIO_NUM_9, LOW);
}

/**
 * @brief Puts the ESP32 into deep sleep mode.
 */
void sleepIOT() {
    // Prepare GPIO pins for sleep
    digitalWrite(GPIO_NUM_3, LOW);
    //digitalWrite(GPIO_NUM_7, HIGH);
    digitalWrite(GPIO_NUM_9, HIGH);
    pinMode(GPIO_NUM_9, OUTPUT);
    digitalWrite(GPIO_NUM_9, HIGH);
    delay(1200);
    digitalWrite(GPIO_NUM_9, LOW);

    // Hold GPIO states during deep sleep
    //gpio_hold_en(GPIO_NUM_7);
    gpio_hold_en(GPIO_NUM_3);
    gpio_hold_en(GPIO_NUM_9);

    // Configure the ESP32 to wake up after a specified time
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " seconds");
    Serial.println("Going to sleep now");

    // Enter deep sleep
    esp_deep_sleep_start();
}

/**
 * @brief Recovers the I2C bus in case of a bus lock.
 */
void i2cBusRecovery() {
    // Configure SDA and SCL pins for recovery
    pinMode(GPIO_NUM_6, INPUT_PULLUP);  // SDA
    pinMode(GPIO_NUM_12, OUTPUT);      // SCL

    // Generate up to 9 clock pulses to free the SDA line
    for (int i = 0; i < 9; i++) {
        digitalWrite(GPIO_NUM_12, HIGH);
        delayMicroseconds(5);
        digitalWrite(GPIO_NUM_12, LOW);
        delayMicroseconds(5);
    }

    // Generate a STOP condition
    pinMode(GPIO_NUM_6, OUTPUT);
    digitalWrite(GPIO_NUM_6, LOW);
    delayMicroseconds(5);
    digitalWrite(GPIO_NUM_12, HIGH);
    delayMicroseconds(5);
    digitalWrite(GPIO_NUM_6, HIGH);
    delayMicroseconds(5);

    // Reinitialize the I2C bus
    Wire.end();
    delay(10);
    Wire.begin(GPIO_NUM_6, GPIO_NUM_12, 100000);  // Reinitialize I2C with the desired speed
}