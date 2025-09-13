/**
 * @file version_info.h
 * @brief Definiciones de versión y capacidades básicas del firmware.
 */
#ifndef VERSION_INFO_H
#define VERSION_INFO_H

#include <stdint.h> // Para uint16_t

// Versión general del firmware (SemVer)
#define FW_VERSION        "1.0.0"
// Versión del driver de suelo (lectura básica 3 registros con CRC)
#define SOIL_DRV_VERSION  "0.1.0"
// Versión del formato de payload actual
#define PAYLOAD_VERSION   "V1"

// Marca de build (puedes actualizar manualmente si deseas distinguir compilaciones)
#define BUILD_SHORT       "250913-01"  // yyMMdd-idx

// Capacidades (bitmask futura ampliable)
#define CAP_SOIL_BASIC    0x0001
// #define CAP_SOIL_EXTENDED 0x0002
// #define CAP_PAYLOAD_V2    0x0010

inline uint16_t firmwareCapabilities() {
    uint16_t caps = 0;
    caps |= CAP_SOIL_BASIC;
    return caps;
}

#endif // VERSION_INFO_H
