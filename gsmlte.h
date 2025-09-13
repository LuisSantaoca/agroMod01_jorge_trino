#ifndef GSMLTE_H
#define GSMLTE_H

#include <stdint.h>
#include "Arduino.h"
#include "type_def.h"

// UART and pin definitions
#define UART_BAUD           115200  // UART baud rate
#define PIN_DTR             25      // DTR pin for modem
#define PIN_TX              10      // TX pin for UART communication
#define PIN_RX              11      // RX pin for UART communication
#define PWRKEY_PIN          9       // Power control pin for modem
#define LED_PIN             12      // LED indicator pin

// TinyGSM configuration
#define TINY_GSM_MODEM_SIM7080      // Define the modem type
#define TINY_GSM_RX_BUFFER 1024     // Buffer size for TinyGSM
#define TINY_GSM_YIELD_MS 10        // Yield time for TinyGSM
#define TINY_GSM_MODEM_HAS_GPS      // Enable GPS functionality for the modem

// Serial definitions
#define SerialAT Serial1            // Serial port for AT commands
#define SerialMon Serial            // Serial port for monitoring
#define GSM_PIN ""                  // GSM PIN (if required)

// Function declarations

/**
 * @brief Powers on the modem by toggling the power pin.
 */
void modemPwrKeyPulse();

/**
 * @brief Restarts the modem by powering it off and on.
 */
void modemRestart();

/**
 * @brief Initializes the modem module and prepares it for communication.
 */
void setupModem();

/**
 * @brief Starts the LTE connection and configures the modem.
 * @return True if the connection is successful, false otherwise.
 */
bool startLTE();

/**
 * @brief Flushes the serial port to clear any pending data.
 */
void flushPortSerial();

/**
 * @brief Sends an AT command to the modem and waits for a specific response.
 * @param command The AT command to send.
 * @param expectedResponse The expected response from the modem.
 * @param timeout The timeout period in milliseconds.
 * @return True if the expected response is received, false otherwise.
 */
bool sendATCommand(const String& command, const String& expectedResponse, unsigned long timeout);

/**
 * @brief Reads the response from the modem within a specified timeout.
 * @param timeout The timeout period in milliseconds.
 * @return The response string from the modem.
 */
String readResponse(unsigned long timeout);

/**
 * @brief Opens a TCP connection to the server.
 * @return True if the connection is successful, false otherwise.
 */
bool tcpOpen();

/**
 * @brief Sends data over the TCP connection.
 * @param data The data to send.
 * @param retries The number of retries if sending fails.
 * @return True if the data is sent successfully, false otherwise.
 */
bool tcpSendData(const String data, int retries);

/**
 * @brief Reads data from the TCP connection.
 * @return True if data is successfully read, false otherwise.
 */
bool tcpReadData();

/**
 * @brief Closes the TCP connection.
 * @return True if the connection is closed successfully, false otherwise.
 */
bool tcpClose();

/**
 * @brief Retrieves the IMEI of the modem.
 * @return The IMEI as a string.
 */
String getImei();

/**
 * @brief Retrieves the ICCID of the SIM card.
 * @return The ICCID as a string.
 */
String getCcid();

/**
 * @brief Retrieves the signal quality of the modem.
 * @return The signal quality as a string.
 */
String getSignal();

/**
 * @brief Stops the GPS functionality on the SIM7080 module.
 * This function disables the GPS to save power or when GPS is not needed.
 */
void stopGpsModem();

/**
 * @brief Sets up the GPS on the SIM7080 module.
 * @return True if GPS setup is successful, false otherwise.
 */
bool setupGpsSim(void);

/**
 * @brief Gets the current GPS data.
 * @param gpsdata Reference to a gpsdata_type structure to store the GPS data.
 * @return True if GPS data is successfully retrieved, false otherwise.
 */
bool getGpsSim(gpsdata_type &gpsdata, uint16_t GPS_RETRIES);

#endif // GSMLTE_H