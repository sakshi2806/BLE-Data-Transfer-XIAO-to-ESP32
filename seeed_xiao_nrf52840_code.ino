#include <bluefruit.h>

// Define UUIDs for the custom service and characteristic
#define SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdef01-1234-5678-1234-56789abcdef1"

// Task handle for the task that will be notified
static TaskHandle_t xTaskToNotify = NULL;

// Global flag to indicate connection status
bool isConnected = false;

// BLE configuration
#define DATA_SIZE 240
#define SEND_INTERVAL 10  // 100 ms interval

// Variables for sending data
uint8_t count = 1;
unsigned long lastSendTime = 0;

// BLE Service and Characteristic
BLEService customService(SERVICE_UUID);                       // Define the custom service
BLECharacteristic customCharacteristic(CHARACTERISTIC_UUID);  // Define the custom characteristic

void startAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(customService);
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);  // Advertising interval: 20 ms to 152.5 ms
  Bluefruit.Advertising.setFastTimeout(30);    // Fast advertising for 30 seconds
  Bluefruit.Advertising.start(0);              // 0 = Don't stop advertising
}

void connect_callback(uint16_t conn_handle) {
  Serial.println("Connected");
  isConnected = true;
  Bluefruit.Connection(conn_handle)->requestMtuExchange(247);
  Bluefruit.Connection(conn_handle)->requestConnectionParameter(6, 8);  // 7.5 - 10 ms connection interval

  // Notify the task that the connection is established
  if(xTaskToNotify){
    Serial.println(1);
  }
  else{
    Serial.print("Null");
  }
  if (xTaskToNotify != NULL && isConnected) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xTaskToNotify, &xHigherPriorityTaskWoken);
    xTaskToNotify = NULL;  // Clear the task handle after notifying

    // Request a context switch if a higher-priority task was woken
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  // Serial.print("Disconnected, reason = 0x");
  // Serial.println(reason, HEX);

  // Set the connected flag to false
  isConnected = false;
}

void setup(void) {
  Serial.begin(115200);


  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  Bluefruit.setTxPower(4);  // Set transmission power to maximum
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Setup the custom service
  customService.begin();

  // Setup the custom characteristic
  customCharacteristic.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_NOTIFY);
  customCharacteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);  // No security required
  customCharacteristic.setFixedLen(DATA_SIZE);                     // Fixed length of 240 bytes
  customCharacteristic.begin();

  // Start advertising
  startAdv();

  // Create the data sending task
  xTaskCreate(
    vDataTask,                 // Task function
    "vDataTask",               // Name of the task
    configMINIMAL_STACK_SIZE,  // Stack size in words, not bytes
    NULL,                      // Task input parameter
    1,                         // Priority of the task
    &xTaskToNotify             // Task handle
  );
}

void loop(void) {
}

void vDataTask(void *pvParameters) {
  const TickType_t xMaxBlockTime = portMAX_DELAY;  // Block indefinitely

  while (1) {
    // Wait for notification
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 10;
    ulTaskNotifyTake(pdTRUE, xMaxBlockTime);


    while (isConnected) {
      xLastWakeTime = xTaskGetTickCount();

      uint8_t message[DATA_SIZE];
      for (int i = 0; i < DATA_SIZE; i++) {
        message[i] = count++;
        if (count > 240) count = 1;
      }


      customCharacteristic.notify(message, DATA_SIZE);

      // // Debugging output to Serial
      // Serial.print("Sent 240 bytes of data: ");
      // for (int i = 0; i < DATA_SIZE; i++) {
      //   Serial.print(message[i]);
      //   Serial.print(" , ");
      // }
      // Serial.println();

      vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }

    // If disconnected, the task will wait for the next notification
    Serial.println("Not connected, waiting for connection.");
  }
}


