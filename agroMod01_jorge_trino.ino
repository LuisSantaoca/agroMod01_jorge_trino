#define USE_SOIL_DRIVER_V2      // Activar lectura robusta Modbus del sensor de suelo (modbus_soil)
//#define SOIL_DEBUG            // Descomentar para ver tramas TX/RX y parsing de suelo

#include "version_info.h"

#include <Arduino.h>         // Arduino core library
#include <Preferences.h>     // Library for persistent storage
#include "type_def.h"        // Type definitions for sensor and GPS data
#include "sleepSensor.h"     // Functions for managing sleep mode
#include "timeData.h"        // Functions for handling time-related data
#include "gsmlte.h"          // GSM/LTE communication functions
#include "sensores.h"        // Sensor-related functions
#include "data_storage.h"    // Functions for saving and loading data to/from flash

// Macros
#define ADD_TO_RESULT(field) result += String(data->field) + ","

// Global Constants
#define TIME_GPS_UPDATE 1      // Time in hours to update GPS data
#define CONSOLE_BAUD    115200  // Baud rate for console serial communication
#define SEND_RETRIES    6       // Maximum retries before entering sleep mode
#define GPS_RETRIES     300     // Maximum retries to adquire GPS data, each retry take 1 sec

// Global Variables
sensordata_type sensordata = {
    .temperatura_ambiente = 0.0,
    .humedad_relativa = 0.0,
    .luz = 0.0,
    .radiacion = 0.0,
    .presion_barometrica = 0.0,
    .co2 = 0.0,
    .humedad_suelo = 0.0,
    .temperatura_suelo = 0.0,
    .conductividad_suelo = 0.0,
    .ph_suelo = 0.0,
    .nitrogeno = 0.0,
    .fosforo = 10.10,
    .potasio = 0.0,
    .viento = 0.0,
    .battery_voltage = 0.0
};
// Global variables for modem gps information
gpsdata_type gpsdata = {
    .lat = 0.0,
    .lon = 0.0,
    .speed = 0.0,
    .alt = 0.0,
    .vsat = 0,
    .usat = 0,
    .accuracy = 0.0,
    .year = 0,
    .month = 0,
    .day = 0,
    .hour = 0,
    .min = 0,
    .sec = 0,
    .level = false,
    .epoc = 0
};

Preferences gpsrecord;          // Preferences object for persistent storage of GPS data
unsigned long epoc_rtcext = 0;  // Variable to store GPS epoch time
String epoc_string = "";        // Epoch time as astring (Unix timestamp)
String uuid = "";               // UUID (36 characters + null terminator)
String latitud = "";            // Latitude as a string
String longitud = "";           // Longitude as a string
String rssi = "";               // Signal strength (e.g., "-70dBm")

// Setup function, runs once at startup
void setup() {
  Serial.begin(CONSOLE_BAUD);   // Initialize coonsole serial communication at 115200 baud
  Serial.print(F("=== FW ")); Serial.print(FW_VERSION);
  Serial.print(F(" | SoilDrv ")); Serial.print(SOIL_DRV_VERSION);
  Serial.print(F(" | Payload ")); Serial.print(PAYLOAD_VERSION);
  Serial.print(F(" | Build ")); Serial.print(BUILD_SHORT);
  Serial.print(F(" | Caps 0x")); Serial.print(firmwareCapabilities(), HEX);
  Serial.println(F(" ==="));
  printWakeupReason();          // Print the reason for waking up from sleep
  setupGPIO();                  // Setup GPIO pins and I2C bus
  setupModem();                 // Initialize the modem
  setupTimeData();
  epoc_rtcext = getEpochTime(); // Get the current epoch time from the external RTC
  epoc_string = String(epoc_rtcext);
  Serial.println("RTC ext:" + epoc_string);
  // Loading GPS data from flash memory
 /* if (loadGpsDataFromFlash(gpsrecord, gpsdata)) {
      Serial.println("GPS data loaded from flash.");
      Serial.println("RTC loaded:" + String(gpsdata.epoc));
      if ((epoc_rtcext - gpsdata.epoc) > (TIME_GPS_UPDATE * 3600)) { // Check if the GPS data is older than 24 hours
          Serial.println("GPS data is older than 24 hrs., updating data.");
          acquireAndStoreGpsData(); // Acquire and store new GPS data
      } else {
          Serial.println("GPS data is no more than 24 hrs. This is the previous data.");
          modemPwrKeyPulse();       // Send PWRKEY pulse to modem to turn it on
      }
      printGpsData(gpsdata);        // Print GPS data for debugging
  } else {
      Serial.println("No valid GPS data in flash, adquiring new.");
      acquireAndStoreGpsData();     // Acquire and store GPS data if not available
  }
*/
  setupSensores();                  // Initialize sensors
}

void loop() {
  latitud = "20.76632210";
  longitud = "-103.38103328";

  readSensorData(&sensordata);                // Read sensor data into the `sensordata` structure
  debugData(&sensordata);                     // Print sensor data for debugging
  //data_encrypted = encrypt(convertStructToString(&sensordata)); // Encrypt the sensor data
  //Serial.println("Encrypted data:");          // Print a label for the encrypted data
  //Serial.println(data_encrypted);             // Print the encrypted data
  envioData(convertStructToString(&sensordata));                  // Send the encrypted data
  sleepIOT();                             // Enter sleep mode if response is successful
}

// Function to convert sensor data structure to a concatenated string
String convertStructToString(sensordata_type* data) {
  String result = "";                           // Initialize an empty string
  
  result += "$,";                               // Add start delimiter
  result += uuid + ",";                         // Add UUID
  result += epoc_string + ",";                         // Add epoch time
  ADD_TO_RESULT(temperatura_ambiente);          // Add ambient temperature
  ADD_TO_RESULT(humedad_relativa);              // Add relative humidity
  ADD_TO_RESULT(luz);                           // Add light level
  ADD_TO_RESULT(radiacion);                     // Add radiation level
  ADD_TO_RESULT(presion_barometrica);           // Add barometric pressure
  ADD_TO_RESULT(co2);                           // Add CO2 level
  ADD_TO_RESULT(humedad_suelo);                 // Add soil humidity
  ADD_TO_RESULT(temperatura_suelo);             // Add soil temperature
  ADD_TO_RESULT(conductividad_suelo);           // Add soil conductivity
  ADD_TO_RESULT(ph_suelo);                      // Add soil pH
  ADD_TO_RESULT(nitrogeno);                     // Add nitrogen level
  ADD_TO_RESULT(fosforo);                       // Add phosphorus level
  ADD_TO_RESULT(potasio);                       // Add potassium level
  ADD_TO_RESULT(viento);                        // Add wind speed
  result += latitud + ",";                      // Add latitude
  result += longitud + ",";                     // Add longitude
  ADD_TO_RESULT(battery_voltage);               // Add battery voltage
  result += rssi + ",#";                        // Add signal strength and end delimiter
  Serial.println("String constructed to send:"); 
  Serial.println(result);                       // Print the constructed string

  return result;                                // Return the constructed string
}

// Function to send data over GSM/LTE
void envioData(String data) {
  if (tcpOpen()) {                            // Check if TCP connection is successful
    if (tcpSendData(data, SEND_RETRIES)) {    // Send data and check response
      tcpClose();                             // Close TCP connection
    }
  }
}

// Function to print GPS data for debugging
void printGpsData(const gpsdata_type& gpsdata) {
  Serial.print("lat:"); Serial.print(gpsdata.lat, 8); Serial.print("\t");
  Serial.print("lon:"); Serial.print(gpsdata.lon, 8); Serial.println();
  Serial.print("speed:"); Serial.print(gpsdata.speed); Serial.print("\t");
  Serial.print("altitude:"); Serial.print(gpsdata.alt); Serial.println();
  Serial.print("year:"); Serial.print(gpsdata.year);
  Serial.print(" month:"); Serial.print(gpsdata.month);
  Serial.print(" day:"); Serial.print(gpsdata.day);
  Serial.print(" hour:"); Serial.print(gpsdata.hour);
  Serial.print(" minutes:"); Serial.print(gpsdata.min);
  Serial.print(" second:"); Serial.print(gpsdata.sec); Serial.println();
  Serial.print("epoc:"); Serial.print(gpsdata.epoc); Serial.println();
  Serial.println();
}

// Function to acquire and store GPS data
void acquireAndStoreGpsData() {
  if (!setupGpsSim()) {          // If GPS setup fails
    Serial.println("GPS setup failed.");
  } else {
    Serial.println("GPS setup successful.");
    if (getGpsSim(gpsdata, GPS_RETRIES)) {    // Attempt to get GPS data up to n times
      setRtcExtFromGps(gpsdata);              // Set the external RTC with GPS date/time data
      setFechaRtc();                          // Synchronize the internal RTC with the external RTC
      gpsdata.epoc = getEpochTime();          // Update the epoch time from the internal RTC
      saveGpsDataToFlash(gpsrecord, gpsdata); // Save GPS data to flash memory
      epoc_string = String(gpsdata.epoc);     // Convert epoch time to string
      latitud = String(gpsdata.lat, 8);       // Convert latitude to string with 8 decimal places
      longitud = String(gpsdata.lon, 8);      // Convert longitude to string with 8 decimal places
      printGpsData(gpsdata);                  // Print GPS data for debugging
    }
  }
}
