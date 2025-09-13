#ifndef TYPE_DEF_H
#define TYPE_DEF_H

#include <Arduino.h>

// Struct for Sensor Data
typedef struct {
    float temperatura_ambiente;       // Ambient temperature
    float humedad_relativa;           // Relative humidity
    float luz;                        // Light intensity
    float radiacion;                  // Radiation
    float presion_barometrica;        // Barometric pressure
    float co2;                        // CO2 concentration
    float humedad_suelo;              // Soil humidity
    float temperatura_suelo;          // Soil temperature
    float conductividad_suelo;        // Soil conductivity
    float ph_suelo;                   // Soil pH
    float nitrogeno;                  // Nitrogen level
    float fosforo;                    // Phosphorus level
    float potasio;                    // Potassium level
    float viento;                     // Wind speed
    float battery_voltage;            // Battery voltage
} sensordata_type;

// Struct for GPS Data
typedef struct {
             float lat;
             float lon;
             float speed;
             float alt;
             int   vsat;
             int   usat;
             float accuracy;
             int   year;
             int   month;
             int   day;
             int   hour;
             int   min;
             int   sec;
             bool  level;
    unsigned long  epoc; // Epoch time (Unix timestamp)
} gpsdata_type;

#endif // TYPE_DEF_H