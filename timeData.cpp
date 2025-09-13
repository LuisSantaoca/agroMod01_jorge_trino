#include <ESP32Time.h>
#include "timeData.h"
#include "RTClib.h"
#include "sleepSensor.h"
//#include "gsmlte.h"
//#include "type_def.h"

// RTC objects
ESP32Time rtc;          // Internal RTC
ESP32Time rtc1(0);      // External RTC
RTC_DS3231 rtcExt;      // DS3231 RTC module

// Task handle for time management
TaskHandle_t TaskTimePower;

/**
 * @brief Helper function to add zero-padding to numbers.
 * @param value The number to pad.
 * @return A string with the number padded to two digits.
 */
String zeroPad(int value) {
    if (value < 10) {
        return "0" + String(value);
    }
    return String(value);
}

/**
 * @brief Sets up the time data by initializing the RTC and setting the initial date/time.
 */
void setupTimeData() {
    int countRtc = 0;

    // Attempt to initialize the external RTC
    while (!rtcExt.begin()) {
        i2cBusRecovery();       // Recover the I2C bus if needed
        Serial.println("Couldn't find RTC");
        delay(1000);
        countRtc++;
        if (countRtc > 10) {
            sleepIOT();         // Enter deep sleep
        }
    }

    setFechaRtc();              // Synchronize the internal RTC with the external RTC
    delay(100);
}

/**
 * @brief FreeRTOS task to manage time-related operations.
 * @param pvParameters Parameters for the task (unused).
 */
void timePower(void* pvParameters) {
    Serial.print("Task Time running on core ");
    Serial.println(xPortGetCoreID());
    Wire.begin(I2C_SDA, I2C_SCL, 100000); // Initialize I2C communication
    if (!rtcExt.begin()) {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        sleepIOT();
    }
    setFechaRtc(); // Synchronize the RTC
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(1);
    for (;;) {
        xLastWakeTime = xTaskGetTickCount();
        vTaskDelayUntil(&xLastWakeTime, xFrequency); // Delay until the next task cycle
    }
}

/**
 * @brief Sets the RTC date and time based on a formatted string.
 * @param fecha The date/time string in the format "YY-MM-DD HH:MM:SS".
 * @return True if the date/time was set successfully, false otherwise.
 */
bool setFecha(String fecha) {
    int se = fecha.substring(15, 17).toInt();
    int mi = fecha.substring(12, 14).toInt();
    int ho = fecha.substring(9, 11).toInt();
    int di = fecha.substring(6, 8).toInt();
    int me = fecha.substring(3, 5).toInt();
    int an = fecha.substring(0, 2).toInt();
    int an0 = an + 2000;

    if (an0 >= 2024) {
        rtc.setTime(se, mi, ho, di, me, an0);
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Sets the external RTC (rtcExt) date and time using a pointer to a gpsdata_type variable.
 * @param gps Pointer to a gpsdata_type structure containing the time information.
 * @return true if the RTC was set successfully, false otherwise.
 */
bool setRtcExtFromGps(gpsdata_type& gps) {
    // Check for valid year (e.g., > 2020)
    if (gps.year < 2021) {
        return false;
    }
    // Create a DateTime object from gpsdata
    DateTime dt(gps.year, gps.month, gps.day, gps.hour, gps.min, gps.sec);
    //extern RTC_DS3231 rtcExt;
    rtcExt.adjust(dt);
    return true;
}

/**
 * @brief Retrieves the current date as a formatted string.
 * @return The date in the format "YYYY-MM-DD".
 */
String getFecha() {
    return String(rtc1.getYear()) + "-" +
           zeroPad(rtc1.getMonth() + 1) + "-" +
           zeroPad(rtc1.getDay());
}

/**
 * @brief Retrieves the current time as a formatted string.
 * @return The time in the format "HH:MM:SS".
 */
String getHora() {
    return zeroPad(rtc1.getHour(true)) + ":" +
           zeroPad(rtc1.getMinute()) + ":" +
           zeroPad(rtc1.getSecond());
}

/**
 * @brief Retrieves the current hour as an integer.
 * @return The current hour.
 */
int getHoraInt() {
    return rtc1.getHour(true);
}

/**
 * @brief Retrieves the current minute as an integer.
 * @return The current minute.
 */
int getMinutoInt() {
    return rtc1.getMinute();
}

/**
 * @brief Retrieves the current second as an integer.
 * @return The current second.
 */
int getSegundoInt() {
    return rtc1.getSecond();
}

/**
 * @brief Synchronizes the internal RTC with the external RTC.
 */
void setFechaRtc() {
    DateTime nowExt = rtcExt.now();
    rtc.setTime(nowExt.second(), nowExt.minute(), nowExt.hour(), nowExt.day(), nowExt.month(), nowExt.year());
}

/**
 * @brief Retrieves the current date and time as a hexadecimal string.
 * @return The date and time in hexadecimal format.
 */
String dateStrHex() {
    char hexBuffer[13];
    sprintf(hexBuffer, "%02X%02X%02X%02X%02X%02X",
            rtc1.getYear() - 2000,
            rtc1.getMonth() + 1,
            rtc1.getDay(),
            rtc1.getHour(true),
            rtc1.getMinute(),
            rtc1.getSecond());
    return String(hexBuffer);
}

/**
 * @brief Retrieves the current date as a hexadecimal string.
 * @return The date in hexadecimal format.
 */
String fechaHex() {
    char hexBuffer[7];
    sprintf(hexBuffer, "%02X%02X%02X",
            rtc1.getDay(),
            rtc1.getMonth() + 1,
            rtc1.getYear() - 2000);
    return String(hexBuffer);
}

/**
 * @brief Updates the epoch time and stores it in the global variable.
 * @param stringoutput If true, prints the epoch time to the serial monitor.
 *                     Defaults to true.
 */
unsigned long getEpochTime() {
    unsigned long epochTime = rtc1.getEpoch();
    return epochTime;
}
