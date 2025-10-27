Aquí tienes el archivo listo para pegar en VS Code como `soil_sensor_registers.md` (formato limpio para que Copilot lo entienda y te sugiera código Modbus a partir de esto):

```markdown
# Plantilla de extracción – Sensor de Humedad / Temperatura / EC de Suelo

---
## 1. Identificación del Sensor
- Modelo exacto: **S-Temp&VWC&EC-02 (SenseCAP)**
- Variante: **IP68, cable 5 m, electrodos anticorrosión**
- Protocolo: **RS485 Modbus RTU**
- Alimentación nominal (V): **3.6–30 V DC**
- Consumo (reposo / medición): **≈ 6 mA @ 24 V (quiescente)**

---
## 2. Parámetros de Comunicación
- Baud rate por defecto: **9600 bps**
- Formato: **8N1 (8 data bits, sin paridad, 1 stop)**
- Dirección esclavo (default): **0x01** (cableado) / **0x12** (18 decimal, conector aviación)
- Función usada para lectura: **0x04 (Input Registers)**  
  *(El sensor también soporta 0x03 para Holding Registers.)*
- Cambio de dirección soportado: **Sí** (escritura en **0x0200**; requiere reinicio de energía)
- Rango de direcciones permitido: **0–255**
- Tiempo típico de respuesta (ms): **No especificado**
- Recomendación de timeout (ms): **No especificado** *(usar margen > tiempo de línea + posible “Response Delay” configurable en 0x0206)*

---
## 3. Mapa de Registros (Orden creciente)
> Escala = factor aplicado al valor crudo para obtener la unidad final.

| Dirección (hex) | Parámetro                      | Tipo    | Bytes | Función | Escala | Unidad | Rango esperado | Notas |
|-----------------|--------------------------------|---------|-------|---------|--------|--------|----------------|-------|
| 0x0000          | Temperatura suelo              | INT16   | 2     | 0x04    | ÷100   | °C     | -40 ~ +80      | Negativos en complemento a dos |
| 0x0001          | Humedad volumétrica (VWC)      | UINT16  | 2     | 0x04    | ÷100   | %      | 0 ~ 100        |      |
| 0x0002          | Conductividad eléctrica (EC)   | UINT16  | 2     | 0x04    | ÷1     | µS/cm  | 0 ~ 20000      | Para mS/cm dividir entre 1000 |
| 0x0003          | Salinidad                      | UINT16  | 2     | 0x04    | ÷1     | mg/L   | 0 ~ 20000      |      |
| 0x0004          | TDS                            | UINT16  | 2     | 0x04    | ÷1     | mg/L   | 0 ~ 20000      |      |
| 0x0005          | Constante dieléctrica (ε)      | UINT16  | 2     | 0x04    | ÷100   | —      | 0.00 ~ 82.00   |      |

---
## 4. Ejemplos de Tramas

### 4.1 Lectura (Request) – Leer 3 registros desde 0x0000 (Temp, VWC, EC)
```

01 04 00 00 00 03 B0 0B

```

### 4.2 Respuesta – Ejemplo correspondiente
```

01 04 06 08 90 0E 93 02 4E D2 57

```
- 08 90 → Temperatura = 0x0890 = 2192 → **21.92 °C**
- 0E 93 → VWC = 0x0E93 = 3731 → **37.31 %**
- 02 4E → EC  = 0x024E = 590  → **590 µS/cm**

### 4.3 Escritura (ejemplo) – Cambiar unidad de temperatura a °F
```

01 06 00 21 00 01 18 00

```
*(El dispositivo responde con eco de la trama de escritura.)*

---
## 5. Escalado y Conversión
- Temperatura (°C) = **crudo ÷ 100**
- Humedad VWC (%)  = **crudo ÷ 100**
- EC (µS/cm)       = **crudo** *(para mS/cm: crudo ÷ 1000)*
- Salinidad (mg/L) = **crudo**
- TDS (mg/L)       = **crudo**
- ε (adimensional) = **crudo ÷ 100**

*Compensación de temperatura para EC disponible mediante coeficiente; ver registros de configuración.*

---
## 6. Rango Operativo y Precisión
| Parámetro      | Precisión (±)                    | Resolución | Notas                         |
|----------------|----------------------------------|------------|-------------------------------|
| Temperatura    | ±0.5 °C                          | 0.1 °C     | Rango -40 ~ +80 °C            |
| Humedad VWC    | ±3 % (0–50%), ±5 % (50–100%)     | 0.03 % / 1 % |                              |
| EC             | ±3 %                              | 10 µS/cm   | Rango 0 ~ 10000 µS/cm (spec) |

---
## 7. Recomendaciones de Uso
- Tiempo mínimo entre lecturas: **No especificado**
- Estabilización tras energizar: **No especificado**
- Longitud máxima de bus/cable RS-485: **No especificado**
- Nota de instalación: **“SET” (cable verde) al VCC+ durante el encendido → modo de configuración con parámetros fijos (Addr=0, 9600 8N1) para recuperar y reconfigurar si se olvidan los ajustes.**  
  *(Luego quitar SET de VCC+ y energizar de nuevo para usar los parámetros guardados.)*

---
## 8. Tabla de CRC (Opcional)
| Trama sin CRC            | CRC esperado (LO HI) |
|--------------------------|----------------------|
| 01 04 00 00 00 03        | B0 0B                |

---
## 9. Notas Especiales del Manual
- **Compensación de temperatura integrada** para EC (0–50 °C).
- **Dirección por defecto**: 1 (cableado) / 18 (conector aviación).
- **Registros de comunicación útiles**:  
  - 0x0200 (Slave Address), 0x0201 (Baudrate), 0x0202 (Protocolo RTU/ASCII), 0x0203 (Parity), 0x0204 (Data bits), 0x0205 (Stop bits), 0x0206 (Response Delay = valor×10 ms), 0x0207 (Active Output Interval = valor×1 s).  
  - Cambios en 0x0200–0x0207 **requieren re-energizar** para aplicar.

---
## 10. Histórico de Cambios (Opcional)
| Fecha     | Cambio               | Fuente |
|----------|-----------------------|--------|
| 2023-04  | Manual V1.1 (revisión)| Seeed  |

---
## 11. Extractos Textuales (Opcional)
```

“Modbus Function Code 0x04: used for reading input register… 0x03 for holding register.”
“VWC 0x0001 / UINT16: 0–10000 corresponds to 0–100%”
“EC  0x0002 / UINT16: 0–20000 corresponds to 0–20000 µS/cm”

```

---
## 12. Pendientes / Dudas
- Definir si el firmware leerá con **0x04** (recomendado por tratarse de registros de entrada) o **0x03**.
- Si se desea reportar EC en **mS/cm**, aplicar división por **1000** en el postproceso.
```

**Fuentes:** especificaciones, mapa de registros y ejemplos de tramas del manual oficial SenseCAP S-Temp\&VWC\&EC-02 (V1.1).&#x20;
