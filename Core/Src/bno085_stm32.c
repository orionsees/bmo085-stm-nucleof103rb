#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "stm32f1xx_hal.h"
#include "gpio.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

/* GPIO mapping */
#define BNO085_INT_PORT GPIOA
#define BNO085_INT_PIN  GPIO_PIN_0

#define BNO085_RST_PORT GPIOA
#define BNO085_RST_PIN  GPIO_PIN_1

extern sh2_Hal_t stm32_hal;
extern UART_HandleTypeDef huart2;

/* ================= Euler struct ================= */

typedef struct {
    float yaw;
    float pitch;
    float roll;
} euler_t;

static euler_t ypr;

/* ================= Quaternion → Euler ================= */

static void quaternionToEuler(float qr, float qi, float qj, float qk, euler_t *ypr)
{
    float sqr = qr * qr;
    float sqi = qi * qi;
    float sqj = qj * qj;
    float sqk = qk * qk;

    ypr->yaw   = atan2f(2.0f * (qi * qj + qk * qr),
                        (sqi - sqj - sqk + sqr));
    ypr->pitch = asinf(-2.0f * (qi * qk - qj * qr) /
                       (sqi + sqj + sqk + sqr));
    ypr->roll  = atan2f(2.0f * (qj * qk + qi * qr),
                        (-sqi - sqj + sqk + sqr));

    /* radians → degrees */
    ypr->yaw   *= 57.2957795f;
    ypr->pitch *= 57.2957795f;
    ypr->roll  *= 57.2957795f;
}

/* ================= Sensor callback ================= */

static void sensorHandler(void *cookie, sh2_SensorEvent_t *event)
{
    sh2_SensorValue_t value;
    char string2[128];

    if (sh2_decodeSensorEvent(&value, event) != SH2_OK)
        return;

    if (value.sensorId == SH2_ARVR_STABILIZED_RV)
    {
        quaternionToEuler(
            value.un.arvrStabilizedRV.real,
            value.un.arvrStabilizedRV.i,
            value.un.arvrStabilizedRV.j,
            value.un.arvrStabilizedRV.k,
            &ypr
        );

        sprintf(string2,"\r\nYPR: %.2f\t%.2f\t%.2f",ypr.yaw, ypr.pitch, ypr.roll);

        HAL_UART_Transmit(&huart2,
                          (uint8_t*)string2,
                          strlen(string2),
                          HAL_MAX_DELAY);
    }
}

/* ================= Async event callback ================= */

static void eventHandler(void *cookie, sh2_AsyncEvent_t *event)
{
    char string2[64];

    if (event->eventId == SH2_RESET)
    {
        sprintf(string2, "\r\nSH2 RESET");
        HAL_UART_Transmit(&huart2,
                          (uint8_t*)string2,
                          strlen(string2),
                          HAL_MAX_DELAY);

        /* Re-enable report after reset */
        sh2_SensorConfig_t cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.reportInterval_us = 5000;   // 200 Hz-ish

        sh2_setSensorConfig(SH2_ARVR_STABILIZED_RV, &cfg);
    }
}

/* ================= Init ================= */

int BNO085_Init(void)
{
    char string2[64];

    /* Hardware reset */
    HAL_GPIO_WritePin(BNO085_RST_PORT, BNO085_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(BNO085_RST_PORT, BNO085_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(100);

    sprintf(string2, "\r\nBNO085 Init");
    HAL_UART_Transmit(&huart2,
                      (uint8_t*)string2,
                      strlen(string2),
                      HAL_MAX_DELAY);

    if (sh2_open(&stm32_hal, eventHandler, NULL) != SH2_OK)
        return -1;

    sh2_setSensorCallback(sensorHandler, NULL);

    /* Enable SAME report as Arduino */
    sh2_SensorConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.reportInterval_us = 5000;   // same as Arduino example

    sh2_setSensorConfig(SH2_ARVR_STABILIZED_RV, &cfg);

    sprintf(string2, "\r\nARVR RV enabled");
    HAL_UART_Transmit(&huart2,
                      (uint8_t*)string2,
                      strlen(string2),
                      HAL_MAX_DELAY);

    return 0;
}

/* ================= Service ================= */

void BNO085_Update(void)
{
    sh2_service();
}
