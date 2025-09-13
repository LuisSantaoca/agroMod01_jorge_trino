# Formato de Payload Actual (GSM/LTE)

Este documento describe el formato de la trama de datos enviada por el firmware vía TCP (función `envioData`). Está basado en la implementación actual en `agroMod01_jorge_trino.ino`.

## 1. Descripción General
- El payload es una cadena ASCII delimitada por un prefijo `$,'` al inicio y un sufijo `,#` al final.
- Los campos intermedios están separados por comas `,`.
- No existe actualmente ningún mecanismo de escape para comas dentro de campos (todos los campos son numéricos o identificadores simples).
- Ejemplo genérico:
  ```
  $,<uuid>,<epoch>,<t_amb>,<h_rel>,<luz>,<radiacion>,<presion_bar>,<co2>,<hum_suelo>,<temp_suelo>,<cond_suelo>,<ph_suelo>,<N>,<P>,<K>,<viento>,<lat>,<lon>,<batt>,<rssi>,#
  ```

## 2. Orden y Significado de Campos
| Pos | Campo                | Fuente / Variable                | Tipo / Formato | Descripción                                                                 |
|-----|----------------------|----------------------------------|----------------|-----------------------------------------------------------------------------|
|  1  | (Start)              | Fijo `$`                         | marcador       | Delimitador de inicio de trama                                            |
|  2  | uuid                 | `uuid` (string)                  | texto          | Identificador único del dispositivo (SIM ICCID/otro)                      |
|  3  | epoch                | `epoc_string`                   | entero string  | Tiempo Unix (segundos)                                                     |
|  4  | temperatura_ambiente | `data->temperatura_ambiente`     | float          | °C ambiente (sensor AHT10)                                                 |
|  5  | humedad_relativa     | `data->humedad_relativa`         | float          | % HR ambiente (AHT10)                                                      |
|  6  | luz                  | `data->luz`                      | float          | Reservado (actualmente 0)                                                  |
|  7  | radiacion            | `data->radiacion`                | float          | Reservado (actualmente 0)                                                  |
|  8  | presion_barometrica  | `data->presion_barometrica`      | float          | Reservado (actualmente 0)                                                  |
|  9  | co2                  | `data->co2`                      | float          | Reservado (actualmente 0)                                                  |
| 10  | humedad_suelo        | `data->humedad_suelo`            | float          | % VWC (crudo/100 según sensor)                                             |
| 11  | temperatura_suelo    | `data->temperatura_suelo`        | float          | °C suelo (INT16 /100)                                                      |
| 12  | conductividad_suelo  | `data->conductividad_suelo`      | float          | Conductividad. Actual: valor crudo/1000 → mS/cm (sensor entrega µS/cm)      |
| 13  | ph_suelo             | `data->ph_suelo`                 | float          | Reservado (no medido)                                                      |
| 14  | nitrogeno            | `data->nitrogeno`                | float          | Reservado (no medido)                                                      |
| 15  | fosforo              | `data->fosforo`                  | float          | Valor fijo inicializado (10.10)                                           |
| 16  | potasio              | `data->potasio`                  | float          | Reservado (no medido)                                                      |
| 17  | viento               | `data->viento`                   | float          | Reservado (no medido)                                                      |
| 18  | latitud              | `latitud` (String)               | decimal 8      | Coordenada decimal (manual/estática por ahora)                             |
| 19  | longitud             | `longitud` (String)              | decimal 8      | Coordenada decimal                                                         |
| 20  | battery_voltage      | `data->battery_voltage`          | float          | Voltaje de batería estimado                                                |
| 21  | rssi                 | `rssi`                           | texto/num      | Intensidad de señal (ej: -70)                                              |
| 22  | (End)                | Fijo `#`                         | marcador       | Delimitador final                                                          |

## 3. Reglas de Formato Actuales
- No hay checksum ni firma de integridad en la trama construida.
- Acepta valores vacíos implícitos si algún campo no se setea (pero el código siempre concatena un número o 0.0).
- Los valores float se convierten vía `String(float)` (precisión por defecto de Arduino: típicamente 2 a 6 dígitos según core). No se controla la cantidad de decimales.
- `rssi` se añade tal cual regresa del módem (p.ej. "20" o "-70").

## 4. Ejemplo Real (según flujo actual)
```
$,8951102030012345678,1712685300,26.54,44.10,0.00,0.00,0.00,0.00,37.31,21.92,0.590,0.00,0.00,10.10,0.00,0.00,20.76632210,-103.38103328,3.98,-70,#
```
*(Los valores son ilustrativos; `conductividad_suelo` aquí quedó en mS/cm / la lectura cruda era 590 µS/cm)*

## 5. Ambigüedades / Riesgos
| Tema | Detalle | Impacto | Mitigación propuesta |
|------|---------|---------|----------------------|
| Unidad EC | Campo no especifica si es µS/cm o mS/cm | Confusión en backend | Añadir sufijo en spec o normalizar a µS/cm |
| Campos vacíos futuros | Posición rígida | Parse frágil | Versionar payload (ver sección 7) |
| Falta de integridad | No hay checksum | Datos corruptos no detectados | Añadir CRC16 o hash corto al final |
| Precisión variable | Conversión float por defecto | Inconsistencia en almacenamiento | Formatear con `dtostrf` y decimales fijos |
| Campos reservados en 0 | Noise semántico | Aumenta ancho sin valor | Omitir campos no usados en versión 2 |

## 6. Extensión Planeada (Soil Extendido)
Si se agregan registros nuevos del sensor de suelo (salinidad, TDS, constante dieléctrica), dos estrategias:
1. Mantener versión 1 igual y añadir un segundo payload (ej: `$E,...,#`).
2. Versionar: primer campo tras `$` pasa a ser `V1` o `V2`.

Ejemplo V2 compacto (sólo campos con dato real):
```
$V2,<uuid>,<epoch>,Tair,Hair,Ts,VWC,ECuS,Sal,TDS,Eps,Batt,RSSI,#
```

## 7. Recomendación de Versionado
- Primer token después de `$`: `V1` actual (retrocompatibilidad implícita si no está → asumir V1).
- Añadir campo final antes de `#`: `CRC16` (hex) de todo entre `$` y la coma previa.

## 8. Propuesta de Cambios Mínimos No Disruptivos
1. Documentar en este archivo que `conductividad_suelo` se envía en mS/cm (o cambiar a µS/cm sin dividir /1000).
2. Estabilizar número de decimales (ej. 2 para temp/VWC, 3 para EC si mS/cm).
3. Añadir `V1` como primer campo opcional (parser de backend: si no está → legacy).
4. (Futuro) Añadir CRC16: payload termina `,<CRC_HEX>,#`.

## 9. Parser Sugerido (Backend) – Pseudocódigo
```pseudo
split(',') -> tokens
assert tokens[0] == '$'
use version = (tokens[1] startsWith 'V') ? tokens[1] : 'V0'
index shift if V0 vs V1
validate last token == '#'
(optional) recompute CRC
map fields
```

## 10. Estado Actual vs. Futuro
| Aspecto | Actual | Futuro Sugerido |
|---------|--------|-----------------|
| Delimitadores | `$` y `#` | Igual |
| Integridad | Ninguna | CRC16 opcional |
| Versionado | No | Prefijo Vn |
| Unidad EC | mS/cm (derivado) | µS/cm o aclaración |
| Campos nulos | Muchos ceros | Eliminar u optimizar en V2 |

---
**Última actualización:** Generado automáticamente a partir de la revisión del código el (fecha del sistema).  
