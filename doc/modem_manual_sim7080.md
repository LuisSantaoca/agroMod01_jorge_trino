```markdown
# SIM7080G – Playbook de comandos AT para **baja cobertura** (asegurar envío)

> Objetivo: maximizar tasa de entrega en zonas con señal débil (CAT-M/NB-IoT). Secuencias y parámetros duros del manual oficial.

---

## 0) Lecturas rápidas de estado (diagnóstico)
- **RSSI/BER:** `AT+CSQ` → `<rssi>,<ber>` (0–31,99).  
- **Modo y celda:** `AT+CPSI?` (CAT-M/NB-IoT: banda, EARFCN, RSRP/RSRQ/SINR).  
- **Registro EPS:** `AT+CEREG?` (usar `AT+CEREG=2` o `=4` para info extendida/PSM).  
- **Modo de red (URC):** `AT+CNSMOD=1` (auto-reporta cambios: GSM/LTE-M/NB).  

---

## 1) Selección de RAT y bandas para acelerar adquisición
- **Forzar tecnología** (evita búsquedas largas):  
  - Automático: `AT+CNMP=2`  
  - **Solo LTE (CAT-M/NB):** `AT+CNMP=38`  
- **Preferencia CAT-M/NB:**  
  - CAT-M: `AT+CMNB=1`  
  - NB-IoT: `AT+CMNB=2`  
  - Ambos: `AT+CMNB=3`  
- **Limitar bandas (recomendado):**  
  - CAT-M: `AT+CBANDCFG="CAT-M",<lista>`  
  - NB-IoT: `AT+CBANDCFG="NB-IOT",<lista>`  
  *(Reducir a 1–2 bandas conocidas acelera reacceso.)*  
- **Búsqueda mejorada de RAT (fallback temporizado):**  
  - `AT+CRATSRCH=<rat_timer>,<srch_align>` (p.ej. `60,20` → cada 60 min intentará RAT superior; alinea antes de eDRX).  
- **Bloqueo de celda (cuando hay celda “buena” identificada):**  
  - CAT-M: `AT+MCELLLOCK=1,<earfcn>,<pci>` / desbloquear `=0`  
  - NB-IoT: `AT+NCELLLOCK=1,<earfcn>,<pci>` / desbloquear `=0`  
- **NB-IoT: escaneo por SNR (menos OOS):** `AT+CNBS=<nivel>`  
  - 1: nivel 0; 2: niveles 0–1; **3: 0–1–2 (mejor en mala señal);** 5: solo nivel 2.

---

## 2) Ahorro vs. entregabilidad (PSM/eDRX)
> Si **“debe entregar ahora”**, **desactive PSM/eDRX** temporalmente.
- **PSM ON/OFF:** `AT+CPSMS=[0|1]` (+ timers T3412/T3324 opcionales).  
- **eDRX:** `AT+CEDRXS=0|1, <AcT>, <eDRX>` y lectura dinámica `AT+CEDRXRDP`.  
- **Optimización de PSM cuando no hay servicio (backoff controlado):**  
  - `AT+CPSMCFGEXT=<mask>,<oos_scans>,<psm_oos>,<rand_window>,<max_oos>,<early_wakeup>`  
  *(Elevar `<psm_oos>` en s y `<rand_window>` evita reconexiones simultáneas; `early_wakeup` ayuda a cerrar envíos antes de que venza PSM.)*  

---

## 3) Activación de datos robusta (APP PDP)
- **Configurar PDP/APN:** `AT+CNCFG=<pdp>,<ip_type>,"<APN>","<user>","<pass>",<auth>`  
  - `<ip_type>`: 0=IPv4v6, 1=IPv4, 2=IPv6 | `<auth>`: 0 none, 1 PAP, 2 CHAP, 3 PAP/CHAP  
- **Activar con reintento automático:** `AT+CNACT=<pdp>,2` *(Auto Active reintenta si falla)*  
  - Consultar: `AT+CNACT?` → `<pdp>,<status>,<address>`  
- Alternativa “3GPP clásica” (si tu stack la usa): `AT+CGDCONT`, `AT+CGATT`, `AT+CGACT`.

---

## 4) **TCP/UDP**: envío tolerante a pérdidas
### 4.1 Parámetros de transporte (ajustar en mala señal)
- **Tiempo de espera de envío (ms):** `AT+CACFG="TIMEOUT",<cid>,<timeout>` *(sube p.ej. 300–1000 ms)*  
- **Empaquetado (bytes):** `AT+CACFG="TRANSPKTSIZE",<size>` *(baja a 512–1024 para reducir fragmentación)*  
- **Tiempo de acumulación:** `AT+CACFG="TRANSWAITTM",<ticks>` *(ticks de 100 ms; p.ej. 10 → 1 s)*  
- **Mostrar IP remota con datos:** `AT+CASRIP=1`  

### 4.2 Patrón robusto de socket
1) **ID TCP/UDP:** `AT+CACID=<cid>`  
2) **Abrir socket:**  
   - TCP: `AT+CAOPEN=<cid>,<pdp>,0,"<host>",<port>`  
   - UDP: `AT+CAOPEN=<cid>,<pdp>,1,"<host>",<port>`  
   - Respuesta: `+CAOPEN: <cid>,0` (éxito)  
3) **Enviar en chunks con control de buffer y ACK:**
   - Consultar espacio: `AT+CASEND=<cid>` → `+CASEND: <leftsize>`  
   - Enviar: `AT+CASEND=<cid>,<len>[,<inputms>]` + datos  
   - Confirmar envío/apilado: `AT+CAACK=<cid>` → `<totalsize>,<unacksize>`  
   - **Reintentar** mientras `<unacksize> > 0` (con backoff exponencial y `TIMEOUT` ampliado).  
4) **Recibir (si aplica):** `AT+CARECV=<cid>,<readlen>` *(usar cuando llegue `+CADATAIND: <cid>`)*  
5) **Estado conexión:** `AT+CASTATE?` (0 cerrado / 1 conectado / 2 server).  
6) **Cerrar limpio:** `AT+CACLOSE=<cid>`

> **Estrategia de reintentos:**  
> - Si `+CAOPEN: <cid>,27` (connect failed) o `…20` (DNS) → re-resolver y reintentar con *backoff* (DNS: `AT+CDNSGIP="<host>",<reint>,<timeout>`).  
> - Si `AT+CAACK` estancado con `<unacksize>` alto → bajar `TRANSPKTSIZE`, subir `TIMEOUT`, y reenviar lote más pequeño.

---

## 5) HTTP(S) con reintentos integrados
1) **URL y límites:**  
   - `AT+SHCONF="URL","https://host[:puerto]"`  
   - `AT+SHCONF="BODYLEN",1024`  
   - `AT+SHCONF="HEADERLEN",350`  
   - **Reintentos:** `AT+SHCONF="POLLCNT",<n>` y `AT+SHCONF="POLLINTMS",<ms>`  
2) **TLS (opcional):** `AT+SHSSL=<ctx>,<calist>[,<cert>]` (cargar certs vía `AT+CSSLCFG`)  
3) **Conectar:** `AT+SHCONN`  
4) **Headers/Body:** `AT+SHAHEAD=<k>,<v>` / `AT+SHBOD=<len>,<timeout>` + cuerpo  
5) **Request:** `AT+SHREQ="<url>",<1=GET|3=POST|…>` → `+SHREQ: <tipo>,<StatusCode>,<DataLen>`  
6) **Leer:** `AT+SHREAD=<start>,<len>` (múltiples si `DataLen>2048`)  
7) **Desconectar:** `AT+SHDISC`

---

## 6) Pruebas de conectividad (útiles en campo)
- **PING IPv4/IPv6:** `AT+SNPING4="<host>",<count>,<size>,<timeout>` / `AT+SNPING6=…`  
- **DNS:** `AT+CDNSGIP="<host>",<reint>,<timeout>`  

---

## 7) Buenas prácticas (firmware)
- **Flujo HW**: mantener **RTS/CTS** (`AT+IFC=2,2`) para no perder datos hacia el módem cuando la red responde lento.  
- **Time-outs conservadores**: elevar `TIMEOUT` (CASEND) y `POLL*` (HTTP) en sitios de señal pobre.  
- **Reducir tamaño de envío**: bajar `TRANSPKTSIZE` (512–1024) y fraccionar tu payload.  
- **PSM/eDRX**: desactivar antes de sesiones críticas; reactivar al terminar para ahorrar batería.  
- **Logging de red**: habilitar `AT+CRRCSTATE=1` (URC de RRC), `AT+CNSMOD=1` y leer `AT+CPSI?` para correlacionar fallos con RSRP/RSRQ/SINR.  
- **Fallback operador/celda**: si `CEREG` se queda en 2/3, probar `AT+COPS=4,2,"<MCCMNC>"` o *unlock* de celda y `CRATSRCH`.

---

## 8) Secuencia mínima “entregar sí o sí” (TCP)
```

AT+CMEE=2
AT+CNMP=38        // LTE-only
AT+CMNB=1         // CAT-M (o 2 si NB IoT mejor)
AT+CBANDCFG="CAT-M",<bandas>
AT+CEREG=2
AT+CNACT=0,2      // APP PDP auto-active
AT+CACID=0
AT+CACFG="TIMEOUT",0,1000
AT+CACFG="TRANSPKTSIZE",1024
AT+CAOPEN=0,0,0,"<host>",<port>
AT+CASTATE?
AT+CASEND=0       // consulta buffer
AT+CASEND=0,<len>,15000 <datos>
AT+CAACK=0        // comprobar <unacksize>==0
// reintentos: si fallo, CDNSGIP y CAOPEN de nuevo
AT+CACLOSE=0

```

---

## 9) URCs útiles a monitorear
- **`+CADATAIND: <cid>`** (hay datos entrantes en socket).  
- **`+APP PDP: <pdp>,ACTIVE|DEACTIVE`** (estado PDP APP).  
- **`+CEREG: …` / `+CREG: …`** (registro).  
- **`+CRRCSTATE: <state>`** (0 Idle / 1 Connected).  

---

## 10) Reset/recuperación
- **Reinicio limpio:** `AT+CFUN=1,1` o `AT+CREBOOT` si queda en estado inconsistente.  

---

### Referencias de comandos en el manual oficial SIM7080 Series AT (V1.02).  
// Fuente: SIMCom, *SIM7080 Series_AT Command Manual_V1.02*  :contentReference[oaicite:0]{index=0}
```
