#include "sh2.h"
#include "sh2_SensorValue.h"
#include "stm32f1xx_hal.h"
#include "sh2_err.h"

extern sh2_Hal_t stm32_hal;

// Callback for sensor events
static void sensorHandler(void *cookie, sh2_SensorEvent_t *pEvent) {
    sh2_SensorValue_t value;

    if (sh2_decodeSensorEvent(&value, pEvent) == SH2_OK) {
        // Process sensor data based on sensor ID
        switch (value.sensorId) {
            case SH2_ROTATION_VECTOR:
                // Handle rotation vector data
                // value.un.rotationVector.i, .j, .k, .real
                break;
            case SH2_ACCELEROMETER:
                // Handle accelerometer data
                // value.un.accelerometer.x, .y, .z
                break;
            // Add other sensors as needed
        }
    }
}

// Callback for async events (like reset)
static void eventHandler(void *cookie, sh2_AsyncEvent_t *pEvent) {
    if (pEvent->eventId == SH2_RESET) {
        // Sensor has reset, reconfigure if needed
    }
}

int BNO085_Init(void) {
    // Open SH2 interface
    int status = sh2_open(&stm32_hal, eventHandler, NULL);
    if (status != SH2_OK) {
        return -1;
    }

    // Register sensor callback
    sh2_setSensorCallback(sensorHandler, NULL);

    // Get product IDs
    sh2_ProductIds_t prodIds;
    status = sh2_getProdIds(&prodIds);
    if (status != SH2_OK) {
        return -2;
    }

    return 0;
}

int BNO085_EnableRotationVector(uint32_t interval_us) {
    sh2_SensorConfig_t config;

    config.changeSensitivityEnabled = false;
    config.wakeupEnabled = false;
    config.changeSensitivityRelative = false;
    config.alwaysOnEnabled = false;
    config.changeSensitivity = 0;
    config.reportInterval_us = interval_us;
    config.batchInterval_us = 0;
    config.sensorSpecific = 0;

    return sh2_setSensorConfig(SH2_ROTATION_VECTOR, &config);
}

void BNO085_Service(void) {
    sh2_service();
}

void BNO085_Update(void)
{
    sh2_service();   // processes incoming SH2/SHTP packets
}
