/**
 * @file modbus_soil.cpp
 */

#include "modbus_soil.h"

// ================= CRC16 Modbus =================
static uint16_t modbusCrc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; ++b) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc; // Low byte primero al transmitir
}

// ================= Utilidades Debug =============
#ifdef SOIL_DEBUG
static void printHex(const uint8_t *buf, size_t len, const __FlashStringHelper *prefix) {
    Serial.print(prefix);
    for (size_t i = 0; i < len; ++i) {
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
        Serial.print(i + 1 < len ? ' ' : '\n');
    }
}
#endif

// ================= Lectura Básica ===============
bool soilReadBasic(EspSoftwareSerial::UART &serial, uint8_t addr,
                   float &tempC, float &vwcPct, float &ecOut) {
    // Limpiar buffer previo
    while (serial.available()) { serial.read(); }

    // Construir request: [addr][0x04][StartHi=0][StartLo=0][CountHi=0][CountLo=3][CRCLo][CRCHi]
    uint8_t frame[8];
    frame[0] = addr;
    frame[1] = 0x04;      // Input Registers
    frame[2] = 0x00;      // Start Hi
    frame[3] = 0x00;      // Start Lo
    frame[4] = 0x00;      // Count Hi
    frame[5] = 0x03;      // Count Lo (3 registros)
    uint16_t crc = modbusCrc16(frame, 6);
    frame[6] = (uint8_t)(crc & 0xFF);       // CRC Low
    frame[7] = (uint8_t)((crc >> 8) & 0xFF);// CRC High

#ifdef SOIL_DEBUG
    printHex(frame, sizeof(frame), F("[SOIL] TX: "));
#endif

    // Enviar frame
    for (uint8_t b : frame) serial.write(b);
    serial.flush();

    // Esperar respuesta
    uint8_t resp[32];
    size_t idx = 0;
    unsigned long start = millis();
    while (millis() - start < SOIL_READ_TIMEOUT_MS) {
        while (serial.available()) {
            if (idx < sizeof(resp)) {
                resp[idx++] = serial.read();
            } else {
                // Buffer overflow → abortar
#ifdef SOIL_DEBUG
                Serial.println(F("[SOIL] Buffer overflow"));
#endif
                return false;
            }
        }
        if (idx >= SOIL_MIN_RESPONSE_LEN) {
            // Posible frame completo; comprobar byte count si llegó todo
            if (idx >= 5) {
                uint8_t byteCount = resp[2];
                size_t expectedLen = 3 + byteCount + 2; // cabecera + datos + CRC
                if (idx >= expectedLen) {
                    break; // Tenemos todo
                }
            }
        }
        // Pequeño yield
        delay(1);
    }

    if (idx < SOIL_MIN_RESPONSE_LEN) {
#ifdef SOIL_DEBUG
        Serial.println(F("[SOIL] Timeout / respuesta incompleta"));
#endif
        return false;
    }

#ifdef SOIL_DEBUG
    printHex(resp, idx, F("[SOIL] RX: "));
#endif

    // Validaciones básicas
    if (resp[0] != addr) {
#ifdef SOIL_DEBUG
        Serial.println(F("[SOIL] Dirección no coincide"));
#endif
        return false;
    }
    if (resp[1] == (0x80 | 0x04)) { // Excepción Modbus
#ifdef SOIL_DEBUG
        Serial.print(F("[SOIL] Excepción código: "));
        Serial.println(resp[2], HEX);
#endif
        return false;
    }
    if (resp[1] != 0x04) {
#ifdef SOIL_DEBUG
        Serial.println(F("[SOIL] Función inesperada"));
#endif
        return false;
    }
    uint8_t byteCount = resp[2];
    if (byteCount != 6) {
#ifdef SOIL_DEBUG
        Serial.print(F("[SOIL] ByteCount inesperado: "));
        Serial.println(byteCount);
#endif
        return false;
    }

    size_t totalLen = 3 + byteCount + 2; // 11 esperado
    if (idx < totalLen) {
#ifdef SOIL_DEBUG
        Serial.println(F("[SOIL] Frame truncado"));
#endif
        return false;
    }

    // Verificar CRC
    uint16_t crcCalc = modbusCrc16(resp, totalLen - 2);
    uint16_t crcResp = (uint16_t)resp[totalLen - 1] << 8 | resp[totalLen - 2]; // Hi <<8 | Lo
    if (crcCalc != crcResp) {
#ifdef SOIL_DEBUG
        Serial.print(F("[SOIL] CRC inválido calc=")); Serial.print(crcCalc, HEX);
        Serial.print(F(" resp=")); Serial.println(crcResp, HEX);
#endif
        return false;
    }

    // Parsear registros
    int16_t rawTemp = (int16_t)((resp[3] << 8) | resp[4]);
    uint16_t rawVwc = (uint16_t)((resp[5] << 8) | resp[6]);
    uint16_t rawEc  = (uint16_t)((resp[7] << 8) | resp[8]);

    tempC = rawTemp / 100.0f;
    vwcPct = rawVwc / 100.0f;
#if SOIL_EC_DIVIDE_1000
    ecOut = rawEc / 1000.0f; // mS/cm
#else
    ecOut = (float)rawEc;    // µS/cm
#endif

#ifdef SOIL_DEBUG
    Serial.print(F("[SOIL] Parsed TempC=")); Serial.print(tempC, 2);
    Serial.print(F(" VWC=")); Serial.print(vwcPct, 2);
    Serial.print(F(" EC=")); Serial.println(ecOut, 3);
#endif

    return true;
}
