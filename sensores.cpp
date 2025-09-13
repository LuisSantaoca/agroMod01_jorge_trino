#include "sensores.h"
#include "timeData.h"
#include "sleepSensor.h"
#include "gsmlte.h"
#include <SoftwareSerial.h> // Library for software-based serial communication
#include <AHT10.h> // Librería para utilizar el sensor AHT10
#ifdef USE_SOIL_DRIVER_V2
#include "modbus_soil.h"
#endif

// PIN definitions
#define RS485_TX      16    // TX pin for RS485 communication
#define RS485_RX      15    // RX pin for RS485 communication
#define RS485_PWREN   3     // Power control pin for RS485 module
//#define PW4851  7         // Additional power control pin for RS485 module
#define ADC_VOLT_BAT  13    // Analog pin for reading battery voltage
//#define DHT_DATA      7     // Digital pin connected to the DHT sensor

// Constants definitions
#define RS485_BAUD          9600      // Baud rate for RS485 communication
#define RS485_CFG           SWSERIAL_8N1  // Configuration byte for RS485 communication
#define ADC_RESOLUTION      4095      // ADC resolution for analog readings
#define VOLTAGE_REF         3.3       // Reference voltage for ADC calculations
#define BATTERY_CALIBRATION 0.2       // Calibration offset for battery voltage
#define TEMP_DIVISOR        100.0     // Divisor for scaling temperature readings
#define HUMIDITY_DIVISOR    100.0     // Divisor for scaling humidity readings
#define EC_DIVISOR          1000.0    // Divisor for scaling electrical conductivity readings
#define MAX_RETRIES         10        // Maximum retries for sensor initialization
#define DHT_TYPE            DHT22     // DHT 22  (AM2302), AM2321

// Global variables
extern String uuid;                   // UUID of the device
extern String epoc_string;            // Epoch time as a string
extern String latitud;                // Latitude as a string
extern String longitud;               // Longitude as a string
extern String rssi;                   // Signal strength (e.g., "-70dBm")
uint8_t sensor_1[] = { 0x12, 0x04, 0x00, 0x00, 0x00, 0x03, 0xB2, 0xA8 }; // Commands for RS485 soil sensor
uint8_t resp_sensor_1[11];            // Response buffer for RS485 soil sensor
EspSoftwareSerial::UART puertosw;     // Software serial object for RS485 communication
//DHT dht(DHT_DATA, DHT_TYPE);          // Instance of DHT sensor
AHT10 myAHT10(0x38);
// Function to initialize sensors
void setupSensores() {
  // Initialize RS485 communication
  puertosw.begin(RS485_BAUD, RS485_CFG, RS485_RX, RS485_TX);
  // Initialize DHT sensor.
  //dht.begin();
  myAHT10.begin();
}

// Function to read data from sensors
void readSensorData(sensordata_type* data) {
  uint8_t index = 0; // Index for RS485 response buffer

  bool soilOk = false;
#ifdef USE_SOIL_DRIVER_V2
  {
    float t, vwc, ecVal;
    if (soilReadBasic(puertosw, 0x12, t, vwc, ecVal)) {
      data->temperatura_suelo = t;          // °C
      data->humedad_suelo = vwc;            // %
      data->conductividad_suelo = ecVal;    // mS/cm (configurable en modbus_soil)
      soilOk = true;
    }
  }
#endif

  if (!soilOk) {
    // Método legacy (fallback) si no está activado el driver v2 o falló la lectura robusta
    for (uint8_t i = 0; i < (sizeof(sensor_1) / sizeof(sensor_1[0])); i++) {
      puertosw.write(sensor_1[i]);
    }
    delay(100); // Wait for the sensor to respond
    while (puertosw.available()) {
      if (index < sizeof(resp_sensor_1)) {
        resp_sensor_1[index] = puertosw.read();
      }
      index++;
    }
    if (index >= 9) { // Mínimo esperado para parsear (no se valida CRC aquí)
      data->temperatura_suelo = word(resp_sensor_1[3], resp_sensor_1[4]) / TEMP_DIVISOR;
      data->humedad_suelo = word(resp_sensor_1[5], resp_sensor_1[6]) / HUMIDITY_DIVISOR;
      data->conductividad_suelo = word(resp_sensor_1[7], resp_sensor_1[8]) / EC_DIVISOR;
    }
  }

  // Reading temperature or humidity from DHT, takes about 250 milliseconds!
  data->humedad_relativa =  myAHT10.readHumidity();
  // Read temperature as Celsius from DHT (the default)
  data->temperatura_ambiente = myAHT10.readTemperature();
  // Check if any reads failed and exit early (to try again).
  //if (isnan(data->humedad_relativa) || isnan(data->temperatura_ambiente)) {
  //  Serial.println(F("Failed to read from DHT sensor!"));
  // }

  // Read battery voltage
  data->battery_voltage = (((analogRead(ADC_VOLT_BAT) * VOLTAGE_REF) / ADC_RESOLUTION) * 2) + BATTERY_CALIBRATION;

  // Retrieve UUID and signal strength from GSM module
  startLTE();
  uuid = getCcid();
  rssi = getSignal();
}

// Function to print sensor data for debugging
void debugData(sensordata_type* data) {
  Serial.println("-----Debug----");
  Serial.println("uuid: " + uuid);
  Serial.println("epoc: " + epoc_string);
  Serial.println("temperatura_ambiente: " + String(data->temperatura_ambiente));
  Serial.println("humedad_relativa : " + String(data->humedad_relativa));
  Serial.println("luz: " + String(data->luz));
  Serial.println("radiación: " + String(data->radiacion));
  Serial.println("presion_barometrica: " + String(data->presion_barometrica));
  Serial.println("co2: " + String(data->co2));
  Serial.println("humedad_suelo: " + String(data->humedad_suelo));
  Serial.println("temperatura_suelo: " + String(data->temperatura_suelo));
  Serial.println("conductividad_suelo: " + String(data->conductividad_suelo));
  Serial.println("ph_suelo: " + String(data->ph_suelo));
  Serial.println("nitrogeno: " + String(data->nitrogeno));
  Serial.println("fosforo: " + String(data->fosforo));
  Serial.println("potasio: " + String(data->potasio));
  Serial.println("viento: " + String(data->viento));
  Serial.println("latitud: " + latitud);
  Serial.println("logitud: " + longitud);
  Serial.println("battery voltage: " + String(data->battery_voltage));
  Serial.println("rssi: " + rssi);
  Serial.println("---------");
}
