/**
 * @file modbus_soil.h
 * @brief Lectura básica robusta (3 registros) del sensor de suelo RS485 (Temp / VWC / EC).
 *
 * Características:
 *  - Construcción dinámica del frame Modbus RTU (función 0x04 Input Registers)
 *  - Validación: tamaño mínimo, dirección, función, byte count, CRC16 (polinomio 0xA001)
 *  - Parseo con manejo de signo en temperatura (INT16)
 *  - Escalado según manual: Temp /100, VWC /100, EC crudo en µS/cm (opcional dividir /1000 a mS/cm)
 *  - Timeout no bloqueante configurable
 *  - Sin efectos secundarios: no inicia / detiene el puerto serie (debe venir ya inicializado)
 *
 * Uso típico:
 *  #include "modbus_soil.h"
 *  float t,v,ec;
 *  if (soilReadBasic(puertosw, 0x12, t, v, ec)) { ... }
 *
 * Para activar esta capa en el firmware existente:
 *  - Definir USE_SOIL_DRIVER_V2 antes de incluir "sensores.h" (ej. en el .ino)
 *  - (Opcional) Definir SOIL_DEBUG para trazas hex en Serial
 */

#ifndef MODBUS_SOIL_H
#define MODBUS_SOIL_H

#include <Arduino.h>
#include <SoftwareSerial.h> // EspSoftwareSerial::UART

// ================= Configuración =================

// Timeout de espera de respuesta (ms)
#ifndef SOIL_READ_TIMEOUT_MS
#define SOIL_READ_TIMEOUT_MS 150
#endif

// Dividir EC (µS/cm) entre 1000 para reportar en mS/cm
#ifndef SOIL_EC_DIVIDE_1000
#define SOIL_EC_DIVIDE_1000 1
#endif

// Tamaño esperado de la respuesta para 3 registros: 1(dir)+1(func)+1(bytecount=6)+6(data)+2(CRC)=11
#define SOIL_MIN_RESPONSE_LEN 11

// =============== API Pública =====================
/**
 * @brief Lee 3 registros consecutivos (0x0000..0x0002) del sensor de suelo.
 * @param serial Referencia al puerto RS485 ya inicializado (EspSoftwareSerial::UART)
 * @param addr Dirección Modbus del sensor
 * @param tempC Salida temperatura °C
 * @param vwcPct Salida humedad volumétrica %
 * @param ecOut Salida conductividad (mS/cm si SOIL_EC_DIVIDE_1000=1, si no µS/cm)
 * @return true si la lectura es válida; false en timeout o validación fallida.
 */
bool soilReadBasic(EspSoftwareSerial::UART &serial, uint8_t addr,
                   float &tempC, float &vwcPct, float &ecOut);

#endif // MODBUS_SOIL_H
