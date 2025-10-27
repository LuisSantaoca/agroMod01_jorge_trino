```markdown
# Schematic_modelo6 – Playbook **HW** para envío confiable en **baja cobertura**

> Objetivo: asegurar que el **SIM7080G** entregue datos aun con señal débil, alineando firmware con lo que permite el **esquemático** (pinout, energía, antenas, protección, buses).  
> Fuente: *Schematic_modelo6.pdf* (ESP32-S3 + SIM7080G + RS-485 + RTC)  :contentReference[oaicite:0]{index=0}

---

## 0) Identificación rápida (según esquemático)
- **MCU:** ESP32-S3-WROOM-1-N8 *(U13)*  
- **Módem:** **SIM7080G** *(U1)*, interfaces: UART1 (TXD/RXD/RTS/CTS/DCD/DTR/RI), **PWRKEY**, **NETLIGHT**, **VDD_EXT**, **RF_ANT**, **GNSS_ANT**.  
- **Alimentaciones presentes:** +4.2 V batería, **+3.8 V (GSM)**, **+3.3 V (ESP32/RTC)**, **+5 V (RS-485)**; reguladores: TLV62569 (bucks 3.3 V/3.8 V), LM2596-5.0, MIC2288 (etapas con L/C y diodos SS36L/1N4007).  
- **Antenas:** U.FL **RF_ANT** y **GNSS_ANT** (con red L/C de adaptación, p.ej. L3=47 nH, C11=100 pF).  
- **SIM socket:** KH-SIM1616 con **ESD** (ESDA6V1W5) y series **22 Ω** en **SIM_DATA/CLK/RST**.  
- **RTC:** **DS3231MZ** con respaldo **CR1220**.  
- **RS-485:** Módulo **XY-017-CB** a **A/B** y conector RS-485.  
- **Conectores debug/comunicación** y **protecciones ESD** en líneas de IO.  :contentReference[oaicite:1]{index=1}

---

## 1) Mapa de **señales críticas** para el envío (firmware ⇄ hardware)
- **PWRKEY del SIM7080G:** controlado desde **IO9** del ESP32 vía **Q1 (NPN)** con divisores **R3/R4 = 47 k** → permite *power-cycle* y arranque forzado del módem desde firmware.  
- **UART del módem (datos/AT):** **UART1_TXD/UART1_RXD** expuestos y enrutable al ESP32 (**IO10/IO11** aparecen etiquetados como UART1_RXD/TXD en placa).  
- **RTS/CTS (flow control):** líneas **UART1_RTS / UART1_CTS** están disponibles; habilítalas en firmware para no perder datos cuando la red está lenta.  
- **NETLIGHT:** LED pilotado vía **Q2 (NPN) + R11-R13** → útil para *diagnóstico de cobertura* en campo (patrones de parpadeo).  
- **VDD_EXT (del SIM):** rail de referencia para niveles lógicos del módem (alimentación de buffers/transistores de interfaz).  :contentReference[oaicite:2]{index=2}

---

## 2) Energía **robusta** para TX bursts (lo que la placa ya contempla)
- **Raíl +3.8 V (GSM):** etapa buck dedicada (**TLV62569**) con **L** y **C** cercanos a módem; hay **capacitores bulk** cerca del rail (ej. **C24=220 µF**, **C17=100 µF**, + **100 nF** de desacople). Mantén estos valores y ubicación en el **layout**.  
- **Retorno y tierra RF:** múltiples **GND** alrededor de **RF_ANT** y del módulo para baja impedancia.  
- **Diodos de rueda libre/Schottky** (SS36L) y **L/FB** correctos en reguladores → ayudan a soportar pulsos.  :contentReference[oaicite:3]{index=3}

**Checklist de campo (alimentación):**
1) Batería **≥3.7 V** bajo carga y sin caída abrupta al transmitir.  
2) Verifica ripple <100 mVpp en **+3.8 V** durante POST/attach.  
3) Si hay resets del módem al enviar → agrega **≥220–470 µF** extra *cerámico/electrolítico* cerca de **VBAT/+3.8 V** (pad para C adicional).  

---

## 3) Antenas y RF (**clave en baja cobertura**)
- Usa ambas IPEX con cables cortos y de baja pérdida; respeta el *keep-out* y la **red L/C** incluida (p. ej., L3 47 nH + C11 100 pF en GNSS).  
- **Conector RF_ANT** dedicado al módem; **GNSS_ANT** independiente para GPS.  
- Asegura buen apriete de IPEX y continuidad a la carcasa (masa periférica).  :contentReference[oaicite:4]{index=4}

---

## 4) Secuencia **“entregar sí o sí”** (coordinación HW+FW)
1) **Power-cycle por firmware** (si la celda está degradada): usa **IO9→Q1** para un pulso **PWRKEY** válido y, si no responde, corta/rehabilita el rail **+3.8 V (EN del buck)**.  
2) **AT init “robusto”** (ver playbook AT previo): fija **LTE-only**, **CAT-M/NB**, **bandas** y levanta PDP; usa **RTS/CTS**.  
3) **Socket en trozos**: fragmenta en **512–1024 B**; sube `TIMEOUT` de envío; confirma con **CAACK** (si queda *unacksize*>0, reintenta).  
4) **Telemetría de red**: loguea `CSQ/CPSI/CEREG` junto con **NETLIGHT** (parpadeos) para diagnosticar.  
5) **Si no hay red**: guarda en **RTC/flash**, duerme módem (PWRKEY/CFUN) y re-intenta con *backoff*; cuando recupere, *flush* de cola.  :contentReference[oaicite:5]{index=5}

---

## 5) Protección **SIM** y fiabilidad de bus
- **ESD** en SIM (ESDA6V1W5) y **series 22 Ω** en **SIM_DATA/CLK/RST** → menos rebotes/EMI en arneses largos.  
- Mantén **C9 100 nF** cerca del zócalo y tierra continua bajo el SIM.  :contentReference[oaicite:6]{index=6}

---

## 6) RS-485 (sensores) sin bloquear el módem
- Módulo **XY-017-CB** alimentado a **+5 V** (LM2596-5.0) y líneas **A/B** al conector RS-485; usa **IO15/IO16** del ESP32 como UART RS-485 (según ruteo).  
- **Aislar tiempos**: realiza lectura RS-485 **antes** de levantar TCP, o en ventanas donde el módem esté *idle* (evita transitorios en +5 V afectando +3.8 V).  :contentReference[oaicite:7]{index=7}

---

## 7) RTC y reintentos cronometrados
- **DS3231MZ + CR1220** permite *timestamp* exacto y **reintentos programados** incluso si el MCU reinicia; usa su **INT#/SQW** para despertar al ESP32 en ventanas de envío.  :contentReference[oaicite:8]{index=8}

---

## 8) Lista de verificación rápida (previo a campo)
- [ ] **RTS/CTS conectados y habilitados** en UART del SIM7080.  
- [ ] **IO9→PWRKEY** probado (encendido, apagado y *hard-recovery*).  
- [ ] **Ripple** en +3.8 V medido durante attach y TX.  
- [ ] **Antenas** (RF y GNSS) con SWR aceptable / conectores firmes.  
- [ ] **ESD** presentes en SIM y líneas externas; **bulk caps** cerca del módem.  
- [ ] **RS-485** operativo sin hundir +5 V ni +3.8 V al transmitir.  :contentReference[oaicite:9]{index=9}
```
