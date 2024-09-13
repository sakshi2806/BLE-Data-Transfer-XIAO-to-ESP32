# **BLE Data Transfer from Seeed XIAO nRF52840 to ESP32**
This project demonstrates how to send 240 bytes of data from a Seeed XIAO nRF52840 to an ESP32 via BLE at an interval of 10ms. The Seeed XIAO acts as the BLE peripheral (server), and the ESP32 acts as the BLE client (receiver).

## Table of Contents
- [Components Used in Seeed XIAO nRF52840](#components-used-in-seeed-xiao-nrf52840)
    - [BLE Library (`<bluefruit.h>`)](#ble-library-bluefruith)
    - [FreeRTOS Task Notifications()](#freertos-task-notifications)
    - [BLE Service and Characteristics](#ble-service-and-characteristics)
    - [BLE Advertising](#ble-advertising)
    - [BLE Connection Callback](#ble-connection-callback)
    - [Data Sending](#data-sending)
    - [Connection and Disconnection Handling](#connection-and-disconnection-handling)
- [Components Used in ESP32](#components-used-in-esp32)
- [Detailed Explanation of FreeRTOS Notifications in Seeed XIAO nRF52840](#detailed-explanation-of-freertos-notifications-in-seeed-xiao-nrf52840)
    - [Task Creation](#task-creation)
    - [Task Notification Usage](#task-notification-usage)
    - [Data Transmission](#data-transmission)
    - [Reconnection Handling](#reconnection-handling)

- [Summary of Key Features in Seeed XIAO nRF52840 Code](#summary-of-key-features-in-seeed-xiao-nrf52840-code)
- [Usage Instructions](#usage-instructions)
- [Troubleshooting](#troubleshooting)
- [Author](#author)

## Components Used in Seeed XIAO nRF52840

### BLE Library (<bluefruit.h>)
The `Adafruit Bluefruit` library is used for BLE functionality. The library helps manage BLE services, characteristics, and advertisements.

### FreeRTOS Task Notifications
* **Task Notification:** FreeRTOS task notifications are used to synchronize BLE connection establishment and the data transmission task. When a BLE connection is established, a task notification is sent to resume the data-sending task.
* **Task Creation:** The task responsible for sending data is created using `xTaskCreate()` in the `setup()` function.
* **Task Handle (`xTaskToNotify`):** A task handle is maintained, which gets notified once a BLE connection is established.
* **Task Notification Mechanism:**
   * The `vTaskNotifyGiveFromISR()` function is called from within the BLE connection callback (`connect_callback`) to signal the data-sending task to start transmitting data.

### BLE Service and Characteristics
* **Custom BLE Service:** The UUID `12345678-1234-5678-1234-56789abcdef0` is used to define a custom BLE service.
* **Custom BLE Characteristic:** A characteristic with UUID `abcdef01-1234-5678-1234-56789abcdef1` is used to transfer the data. It has read, write, and notify properties with a fixed size of 240 bytes.

### BLE Advertising
The XIAO starts advertising when powered on. Advertising includes:
* BLE flags for discoverability
* Transmission power
* The custom service's UUID to allow the ESP32 client to detect and connect to it.

Advertising starts in the `startAdv()` function, which sets the advertising interval and starts advertising until a connection is made.

### BLE Connection Callback
When the XIAO connects to a BLE client (ESP32 in this case):
* **MTU Size:** The maximum transmission unit is set to 247 bytes.
* **Connection Interval:** The connection parameters are negotiated for a fast interval (7.5 - 10ms).
* The task notification is triggered to start data transmission using `vTaskNotifyGiveFromISR().`

### Data Sending
* **Data Packet:** A 240-byte packet is sent from the XIAO to the ESP32 every 10ms.
* The data is sent via BLE notifications using the `customCharacteristic.notify()` function.
* A FreeRTOS task (`vDataTask`) handles data transmission. It runs in a loop while the device is connected, sending the 240-byte message array. The task waits for connection establishment using `ulTaskNotifyTake()`.

### Connection and Disconnection Handling
* **Connection:** When a connection is established, the task for sending data is notified, and the XIAO begins to send data.
* **Disconnection:** If the connection is lost, the device stops transmitting data, and the task waits for the next connection notification.

## Components Used in ESP32
* The ESP32 acts as a BLE client and uses the built-in BLE library.
* It scans for devices advertising the custom service UUID, connects to the XIAO when found, and subscribes to notifications from the characteristic.
* The received data is printed to the serial monitor.

## Detailed Explanation of FreeRTOS Notifications in Seeed XIAO nRF52840

### Task Creation
A FreeRTOS task (`vDataTask`) is created in the `setup()` function. This task is responsible for sending the 240-byte data once the BLE connection is established.
```cpp
xTaskCreate(vDataTask, "vDataTask", configMINIMAL_STACK_SIZE, NULL, 1, &xTaskToNotify);
```
### Task Notification Usage
When the BLE connection is established in the `connect_callback()` function, the task handle (`xTaskToNotify`) is notified using:
```cpp
vTaskNotifyGiveFromISR(xTaskToNotify, &xHigherPriorityTaskWoken);
```

This notification allows the data-sending task to start running and transmitting data. In the data-sending task (`vDataTask`), the task waits for the notification using:
```cpp
ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
```

### Data Transmission
Once the notification is received, the task begins sending 240 bytes of data via BLE notifications every 10ms, until the device is disconnected. The data transmission uses:

```cpp
customCharacteristic.notify(message, DATA_SIZE);
```
### Reconnection Handling
If the BLE connection is lost, the task loops back, waiting for a new connection and notification to resume data transmission.

## Summary of Key Features in Seeed XIAO nRF52840 Code
* **BLE Service and Characteristics:** Defined using custom UUIDs.
* **BLE Notifications:** Used to send 240 bytes of data to the ESP32.
* **FreeRTOS Task:** Manages data sending and uses task notifications to synchronize with the BLE connection status.
* **Connection and Disconnection Callbacks:** Handle the start and stop of data transmission based on connection status.
* **Data Transmission Rate:** 240 bytes are transmitted every 10ms using BLE notifications.

## Usage Instructions
1. **Setup Seeed XIAO nRF52840:**
     * Load the provided code into the Seeed XIAO nRF52840 using the Arduino IDE.
     * Ensure that the Adafruit Bluefruit library is installed.
     * Upload the code to the XIAO and open the Serial Monitor.
2. **Setup ESP32:**
    * Load the provided ESP32 code into the ESP32 using the Arduino IDE.
    * Make sure the ESP32 BLE library is installed.
    * Upload the code to the ESP32 and open the Serial Monitor.
3. **Power On:**
   * Power on the Seeed XIAO nRF52840 and ESP32.
   * The XIAO will start advertising, and the ESP32 will scan for devices.
4. **Connection:**
    * The ESP32 will automatically connect to the XIAO once it detects the correct service UUID.
    * Upon successful connection, the data sending task on the XIAO will start sending 240-byte packets every 10ms.
5. **Monitor Data:**
   * Monitor the Serial Monitor on the ESP32 to see the received data.

## Troubleshooting
1. **Device Not Connecting:**
   * **Check UUIDs:** Ensure that the UUIDs in both the XIAO and ESP32 code match exactly.
   * **Advertising:** Verify that the XIAO is correctly advertising the custom service. Check the Serial Monitor for advertising status.
   * **Range:** Ensure that the XIAO and ESP32 are within range of each other.
2. **No Data Received:**
   * **Characteristic Registration:** Make sure that the ESP32 correctly registers for notifications from the BLE characteristic.
   * **Data Size:** Confirm that the data size and MTU settings are compatible between the XIAO and ESP32.
3. **Frequent Disconnections:**
     * **Connection Interval:** Verify that the connection interval settings are compatible. Adjust connection parameters if necessary.
     * **Power Supply:** Ensure both devices have a stable power supply to avoid interruptions.
4. **Errors in Serial Monitor:**
   * **Library Versions:** Check if the versions of the BLE libraries used are compatible with your hardware and code.
   * **Code Updates:** Ensure you have the latest versions of the libraries and that the code matches the library requirements.
5. **Debugging Tips:**
   * Use `Serial.println()` statements to debug the connection status and data transfer.
   * Test each part of the setup (advertising, connection, data sending) separately to isolate issues.

By following these steps, you should be able to set up and troubleshoot the BLE data transfer between the Seeed XIAO nRF52840 and ESP32 effectively.

## Author
Sakshi Mishra