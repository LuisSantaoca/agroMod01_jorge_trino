#include "gsmlte.h"
#include <TinyGsmClient.h>
#include "timeData.h"
#include "sleepSensor.h"

// TCP connection constants
//#define DB_SERVER_IP      "snrs.natdat.mx"    // address for TCP connection
//#define TCP_PORT          "12606"             // Port number for TCP connection
#define DB_SERVER_IP        "143.110.152.110"   // IP address for TCP connection
#define TCP_PORT            "11234"             // Port number for TCP connection
#define NTP_SERVER_IP       "200.23.51.102"     // NTP server IP address

// Modem mode constants
#define MODEM_NETWORK_MODE  38  // Network mode for the modem
#define CAT_M               1   // CAT-M Preferred mode
#define NB_IOT              2   // NB-IoT Preferred mode
#define CAT_M_NB_IOT        3   // CAT-M and NB-IoT Preferred mode

// Timeout and delay constants
#define SHORT_DELAY           100     // Short delay (100 ms) *60 is minimum to avoid failure to read some data
#define LONG_DELAY            1000    // Long delay (1 second)
#define MODEM_PWRKEY_DELAY    1500    // Delay for modem power-on (1.5 s)
#define MODEM_STABILIZE_DELAY 3000    // Delay for modem stabilization (3 s)

// See all AT commands on serial console, if wanted
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// Global variables for modem information
String iccidsim0 = "";      // ICCID of the SIM card
String signalsim0 = "";     // Signal strength
String imeigsm0 = "";       // IMEI of the modem

// Modem power control function
void modemPwrKeyPulse() {
  digitalWrite(PWRKEY_PIN, HIGH);    // Set power pin LOW to modem
  delay(MODEM_PWRKEY_DELAY);         // Wait for the modem to power on
  digitalWrite(PWRKEY_PIN, LOW);     // Set power pin HIGH to modem
  delay(MODEM_STABILIZE_DELAY);      // Wait for the modem to stabilize
}

void modemRestart() {
  modemPwrKeyPulse();                      // Power off the modem
  delay(LONG_DELAY);                  // Wait for a while
  modemPwrKeyPulse();                      // Power on the modem
  delay(LONG_DELAY);                  // Wait for the modem to initialize
}

// Initialize modem module
void setupModem() {
  SerialMon.begin(115200);            // Start serial monitor at 115200 baud
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX); // Start serial communication with the modem
  digitalWrite(LED_PIN, HIGH);        // Turn on LED
}

// Start LTE connection
bool startLTE() {
  Serial.println("======== INIT LTE ========");
  modem.sendAT("+CFUN=1");
  modem.waitResponse();
  int retry = 0;
  while (!modem.testAT(1000)) {
    Serial.print("+");
    if (retry++ > 3) {
      modemPwrKeyPulse();      // Send pulse to power key
      retry = 0;
      Serial.println("Retry start modem.");
    }
  }
//  if (!modem.init()) {                // Initialize the modem SIM
//    Serial.println("Failed to init modem SIM.");
//    sleepIOT();                       // Enter sleep mode
//    return false;                     // Return false to indicate failure
//  }
  delay(LONG_DELAY);                  // Wait for the modem to initialize
  sendATCommand("+CMGF=1", "OK", SHORT_DELAY);                  // Set SMS text mode
  sendATCommand("+CNMI=2,2,0,0,0\r", "OK", SHORT_DELAY);        // Configure new message indications
  //sendATCommand("+SIMCOMATI", "OK", SHORT_DELAY);               // Get modem information
  //sendATCommand("+CNMP?", "OK", SHORT_DELAY);                   // Query network mode
  sendATCommand("+CGDCONT=1,\"IP\",\"em\"", "OK", SHORT_DELAY); // Define PDP context
  sendATCommand("+CNACT=0,1", "OK", SHORT_DELAY);               // Activate PDP context
  //String name = modem.getModemName();                           // Get modem name
  Serial.println("Modem: " + modem.getModemInfo());             // Print modem information
  sendATCommand("+CNMP=" + String(MODEM_NETWORK_MODE), "OK", SHORT_DELAY); // Set network mode
  sendATCommand("+CMNB=" + String(CAT_M), "OK", SHORT_DELAY);   // Set CAT-M preferred mode
  for (int i = 0; i < 60; i++) {                                // Retry loop for network connection
    if (modem.isNetworkConnected()) {                           // Check if network is connected
      Serial.println("Connected to network.");                  // Print success message
      sendATCommand("+CPSI?", "OK", SHORT_DELAY);               // Get network information
      iccidsim0 = modem.getSimCCID();                           // Get SIM card ICCID
      imeigsm0 = modem.getIMEI();                               // Get modem IMEI
      signalsim0 = String(modem.getSignalQuality());            // Get signal quality
      return true;                                              // Return true to indicate success
    }
    Serial.print(".");
    delay(SHORT_DELAY);                           // Wait before retrying
  }
  Serial.println("Failed to connect to network.");
  modemPwrKeyPulse();                                // Power off the modem
  sleepIOT();                                     // Enter sleep mode
  return false;                                   // Return false to indicate failure
}

// Open TCP connection
bool tcpOpen() {
  String command = "+CAOPEN=0,0,\"TCP\",\"" DB_SERVER_IP "\"," TCP_PORT;  // Build AT command
  return sendATCommand(command, "+CAOPEN: 0,0", 1200);                    // Send command and check response
}

// Close TCP connection
bool tcpClose() {
  return sendATCommand("+CACLOSE=0", "OK", SHORT_DELAY);      // Send command to close TCP connection
}

// Read data from TCP
bool tcpReadData() {
  return sendATCommand("+CARECV=0,10", "+CARECV: 1", SHORT_DELAY); // Read data from TCP
}

// Send data over TCP
bool tcpSendData(String datos, int retries) {
  for (int attempt = 0; attempt < retries; attempt++) {
    if (sendATCommand("+CASEND=0," + String(datos.length() + 2), ">", 100)) {
      //Serial.println("Sending: " + datos);
      if (sendATCommand(datos, "CADATAIND: 0", LONG_DELAY)) {
        Serial.println("\nData sent successfully on attempt " + String(attempt + 1) + "\n");
        return true; // Return true if data is sent successfully
      }
    }
    Serial.println("Attempt " + String(attempt + 1) + " failed. Retrying...");
    delay(LONG_DELAY); // Wait before retrying
  }
  Serial.println("Failed to send data after " + String(retries) + " attempts.");
  return false; // Return false if all attempts fail
}

// Get modem IMEI
String getImei() {
  return imeigsm0;
}

// Get SIM card ICCID
String getCcid() {
  return iccidsim0;
}

// Get signal quality
String getSignal() {
  return signalsim0;
}

// Helper function to flush the serial port
void flushPortSerial() {
  while (SerialAT.available()) {
    char c = SerialAT.read(); // Read and discard all available data
  }
}

// Helper function to read response from the modem
String readResponse(unsigned long timeout) {
  unsigned long start = millis(); // Record the start time
  String response = "";

  flushPortSerial(); // Clear the serial buffer
  while (millis() - start < timeout) { // Wait until timeout
    while (SerialAT.available()) {
      char c = SerialAT.read(); // Read available data
      response += c;
    }
  }
  return response; // Return the response
}

// Helper function to send AT commands and wait for a specific response
bool sendATCommand(const String& command, const String& expectedResponse, unsigned long timeout) {
  String response = "";
  unsigned long start = millis(); // Record the start time

  flushPortSerial(); // Clear the serial buffer
  //Serial.println("Command:" + command); // Print the command being sent
  modem.sendAT(command); // Send the AT command
  while (millis() - start < timeout) { // Wait until timeout
    while (SerialAT.available()) {
      char c = SerialAT.read(); // Read available data
      response += c;
    }
  }
  if (response.indexOf(expectedResponse) != -1) { // Check if the expected response is received
    //Serial.println(":\r" + response); // Print the response
    return true; // Return true if the response matches
  }
  return false; // Return false if the response does not match
}

// Function to stop GNSS on SIM7080 module
void stopGpsModem() {
  modem.disableGPS(); // Disable GPS function
  delay(3000); // Wait for a while
}

// Function to setup GPS on SIM7080 module
bool setupGpsSim(void) { 
  Serial.println("======== INIT GPS ========");
  Serial.print("Turning on modem... ");
  modemPwrKeyPulse(); // Power on the modem
  int retry = 0;
  while (!modem.testAT(1000)) {
    Serial.print("#");
    if (retry++ > 15) {
      modemPwrKeyPulse();      // Send pulse to power key
      retry = 0;
      Serial.println("Retry start modem.");
    }
  }
  Serial.println();
  Serial.println("Modem started!");
  modem.sendAT("+CFUN=0");
  modem.waitResponse();
  delay(3000);
  // When configuring GNSS, you need to stop GPS first
  modem.disableGPS();
  delay(500);
  // GNSS Work Mode Set GPS+BEIDOU [gps, glo, bd, gal, qzss]
  modem.sendAT("+CGNSMOD=1,0,1,0,0");
  modem.waitResponse();
  // minInterval = 1000,minDistance = 0,accuracy = 0 []
  //modem.sendAT("+SGNSCMD=2,1000,0,0");
  modem.sendAT("+SGNSCMD=1,0");
  modem.waitResponse();
  // Turn off GNSS.
  modem.sendAT("+SGNSCMD=0");
  modem.waitResponse();
  delay(500);
  // GPS function needs to be enabled for the first use
  if (modem.enableGPS() == false) {
    Serial.print("Modem enable gps function failed!!");
    return false; // Return false if enabling GPS fails
  }
  else {
    // Set GNSS Cold Start - This command is used to reset the GNSS module and clear any previous data
    //modem.sendAT("+CGNSCOLD");
    //modem.waitResponse();
    return true; // Return true if GPS is enabled successfully
  }
}

// Function to get GPS position from SIM7080 module
bool getGpsSim(gpsdata_type &gpsdata, uint16_t GPS_RETRIES) {
  // Try to get GPS data up to n times
  for (int i = 0; i < GPS_RETRIES; ++i) {
    if (modem.getGPS(&gpsdata.lat, &gpsdata.lon, &gpsdata.speed, &gpsdata.alt, 
                     &gpsdata.vsat, &gpsdata.usat, &gpsdata.accuracy,
                     &gpsdata.year, &gpsdata.month, &gpsdata.day,
                     &gpsdata.hour, &gpsdata.min, &gpsdata.sec)) {
      Serial.println("Successfully got GPS data!");
      modem.disableGPS(); 
      delay(3000);
      return true;                // Return true if GPS data is successfully retrieved
    } else {
        Serial.print(".");        // Print a dot to indicate an attempt
        delay(1000);              // Wait for 1 second before retrying
        if (i == GPS_RETRIES-1) { // If this is the last attempt
          Serial.println("\nFailed to get GPS data after " + String(GPS_RETRIES) + " attempts.");
          modem.disableGPS();     // Disable GPS to save power
          delay(3000);
          return false;           // Return false if GPS data retrieval fails
      }
    }
  }
}
