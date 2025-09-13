````markdown
# AHT10 – Playbook de COMANDOS I²C para **lecturas confiables** (condiciones adversas / ruido / baja “señal” en bus)

> Objetivo: asegurar que **siempre obtengamos dato** aun con líneas largas, ruido EMI o alimentación débil. Secuencias exactas del manual y recomendaciones de robustez.

---

## 0) Ficha rápida (para diagnóstico)
- **I²C 7-bit address:** `0x38`
- **Alimentación:** `1.8–3.6 V` (recomendado `3.3 V`)  
- **Consumo típico:** `≈23 µA` (medición), `0.25 µA` (sleep)
- **Arranque:** hasta **20 ms** tras power-on para quedar listo (SCL alto)
- **Frecuencias I²C:** típica 0–**100 kHz** (modo estándar), hasta **400 kHz** (alta velocidad) → **usar 100 kHz** en entornos ruidosos
- **Condensador desacople:** **100 nF** entre VDD y GND, muy cercano al sensor
- **Pull-ups:** típ. **10 kΩ** en SDA/SCL
- **Intervalo recomendado de lectura para no auto-calentarse:** **≥2 s** (si quieres ∆T propia < 0.1 °C)
- **Nota de bus:** el manual recomienda **un solo AHT10** en el bus I²C (evitar colisiones)

---

## 1) Secuencias de **comando** (robustas)

### 1.1 Reset suave (recuperación rápida)
- **Soft-Reset:** enviar `0xBA` (soft reset).  
  → Esperar **≤20 ms** y el sensor vuelve a IDLE.

### 1.2 Inicialización
- **Init:** enviar comando **`0xE1`** (Initialization command).  
  *Si tu stack lo requiere, escribe los parámetros que use tu librería tras `0xE1`; el manual solo fija el opcode.*

### 1.3 Medición (temperatura + humedad)
1) **Trigger measure:** escribir **`0xAC`** seguido de **`0x33 0x00`**.  
2) **Esperar ≥75 ms**.  
3) **Read burst**: lectura con la dirección de lectura (`0x38 | 1`) para obtener **7 bytes**:  
   - **Byte0 = STATUS** (bit7=1 → ocupado, bit7=0 → listo)  
   - **Bytes1..6 = datos** (20 bits de RH + 20 bits de T)

> **Bucle robusto:** si `STATUS.bit7==1` tras 75–100 ms, reintenta (backoff corto 5–10 ms). Si tras **10** intentos sigue ocupado, ejecutar **soft-reset (`0xBA`)** y reiniciar la secuencia.

---

## 2) Conversión de datos (exacta)

Sea el buffer leído:  
`b0=status, b1, b2, b3, b4, b5`  *(el manual ilustra 6 datos tras el status)*

- **Crudos (20 bits):**
  - `S_RH = ( (uint32_t)b1 << 12 ) | ( (uint32_t)b2 << 4 ) | ( b3 >> 4 )`
  - `S_T  = ( ((uint32_t)(b3 & 0x0F) << 16) | ( (uint32_t)b4 << 8 ) | b5 )`
- **Escalas:**
  - `RH_% = ( S_RH / 2^20 ) * 100.0`
  - `T_°C = ( S_T  / 2^20 ) * 200.0 - 50.0`

---

## 3) Recomendaciones **en condiciones adversas**
- **Bajar SCL a 100 kHz** y aumentar **tiempos de setup/hold** si tu MCU lo permite.
- **Separar** SDA y SCL físicamente o enrutar **VDD/GND entre ellas**; usar **cable apantallado** en tiradas largas.
- **Activación controlada:** alimenta el AHT10 desde un GPIO/FET para **power-cycle** si se queda “ocupado”.
- **Ventilación** del encapsulado (evita cobre macizo bajo el sensor) para que responda más rápido.
- **Post-soldadura:** rehidratar el polímero (≥12 h a >75 %RH) para evitar drift.
- **Frecuencia de lectura:** no dispares mediciones en ráfaga; deja **≥2 s** entre lecturas sostenidas.

---

## 4) Código de referencia (C++/Arduino, tolerante a fallos)

```cpp
#include <Wire.h>
static const uint8_t AHT10_ADDR = 0x38;

bool aht10_soft_reset() {
  Wire.beginTransmission(AHT10_ADDR);
  Wire.write(0xBA);               // Soft reset
  if (Wire.endTransmission() != 0) return false;
  delay(20);                      // ≤20 ms
  return true;
}

bool aht10_init() {
  Wire.beginTransmission(AHT10_ADDR);
  Wire.write(0xE1);               // Initialization command
  // Si tu lib requiere parámetros, escríbelos aquí (p.ej. 0x08,0x00)
  if (Wire.endTransmission() != 0) return false;
  delay(10);
  return true;
}

bool aht10_read(float& rh, float& tc) {
  // 1) Trigger
  Wire.beginTransmission(AHT10_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;

  // 2) Espera mínima y polling de busy
  const uint32_t t0 = millis();
  delay(80); // ≥75 ms
  for (int tries = 0; tries < 10; ++tries) {
    Wire.requestFrom((int)AHT10_ADDR, 7);
    if (Wire.available() == 7) {
      uint8_t b0 = Wire.read(); // status
      uint8_t b1 = Wire.read();
      uint8_t b2 = Wire.read();
      uint8_t b3 = Wire.read();
      uint8_t b4 = Wire.read();
      uint8_t b5 = Wire.read();
      uint8_t _crc_ignored = Wire.read(); // algunos lotes reportan 7º byte; puede no usarse

      if ((b0 & 0x80) == 0) {
        // Listo: convertir
        uint32_t s_rh = ((uint32_t)b1 << 12) | ((uint32_t)b2 << 4) | (b3 >> 4);
        uint32_t s_t  = ((uint32_t)(b3 & 0x0F) << 16) | ((uint32_t)b4 << 8) | b5;
        rh = (s_rh / 1048576.0f) * 100.0f;            // 2^20 = 1,048,576
        tc = (s_t  / 1048576.0f) * 200.0f - 50.0f;
        return true;
      }
    }
    delay(10); // backoff corto
  }

  // 3) Recuperación: soft reset y reintento único
  if (!aht10_soft_reset()) return false;
  if (!aht10_init())       return false;
  delay(25);
  return aht10_read(rh, tc);  // segundo intento (termina aquí si vuelve a fallar)
}
````

**Uso recomendado:**

```cpp
// Setup: I2C a 100 kHz para entornos ruidosos
Wire.begin();        // SDA,SCL por defecto
Wire.setClock(100000);
aht10_soft_reset();
aht10_init();

// Loop:
float rh, tc;
if (aht10_read(rh, tc)) {
  // OK: enviar/registrar valores
} else {
  // Falla persistente: power-cycle del sensor y reintentar
}
```

---

## 5) Especificaciones clave (para verificación de límites)

* **Precisión RH:** típ. ±2 %RH (máx. ver curva), **Resolución RH:** 0.024 %RH
* **Precisión T:** típ. ±0.3 °C, **Resolución T:** 0.01 °C
* **Rango T/RH:** −40…85 °C, 0…100 %RH (normal 0–80 %RH)
* **Tiempo de respuesta (63 %):** RH \~8 s @25 °C/1 m·s⁻¹; T 5–30 s

---

### Fuente: AHT10 Technical Manual (ASAIR).&#x20;

```
```
