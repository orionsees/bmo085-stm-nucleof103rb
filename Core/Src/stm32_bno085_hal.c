#include "stm32f1xx_hal.h"
#include "sh2_hal.h"
#include "gpio.h"
#include <stdbool.h>

/* GPIO mapping */
#define BNO085_INT_PORT GPIOA
#define BNO085_INT_PIN  GPIO_PIN_0

#define BNO085_RST_PORT GPIOA
#define BNO085_RST_PIN  GPIO_PIN_1

/* I2C address (7-bit 0x4A shifted for HAL) */
#define BNO085_I2C_ADDR (0x4A << 1)

extern I2C_HandleTypeDef hi2c1;

/* ================= TIME BASE ================= */

static volatile uint32_t microsCounter = 0;

void HAL_SYSTICK_Callback(void)
{
    microsCounter += 1000;
}

static uint32_t stm32_getTimeUs(sh2_Hal_t *self)
{
    (void)self;
    return microsCounter;
}

/* ================= I2C ================= */

static int stm32_i2c_open(sh2_Hal_t *self)
{
    (void)self;

    /* Hardware reset */
    HAL_GPIO_WritePin(BNO085_RST_PORT, BNO085_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(BNO085_RST_PORT, BNO085_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(100);

    return 0;
}

static void stm32_i2c_close(sh2_Hal_t *self)
{
    (void)self;
}

static int stm32_i2c_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
    (void)self;

    if (HAL_I2C_Master_Transmit(&hi2c1,
                               BNO085_I2C_ADDR,
                               pBuffer,
                               len,
                               HAL_MAX_DELAY) == HAL_OK)
        return len;

    return 0;
}

static int stm32_i2c_read(sh2_Hal_t *self,
                          uint8_t *pBuffer,
                          unsigned len,
                          uint32_t *t_us)
{
    (void)self;

    uint8_t header[4];

    if (HAL_I2C_Master_Receive(&hi2c1,
                              BNO085_I2C_ADDR,
                              header,
                              4,
                              HAL_MAX_DELAY) != HAL_OK)
        return 0;

    uint16_t packet_size = (uint16_t)header[0] |
                           ((uint16_t)header[1] << 8);
    packet_size &= ~0x8000;

    if (packet_size == 0 || packet_size > len)
        return 0;

    if (HAL_I2C_Master_Receive(&hi2c1,
                              BNO085_I2C_ADDR,
                              pBuffer,
                              packet_size,
                              HAL_MAX_DELAY) != HAL_OK)
        return 0;

    *t_us = stm32_getTimeUs(self);
    return packet_size;
}

/* ================= INT HANDLING ================= */

int sh2_hal_waitForInterrupt(void)
{
    while (HAL_GPIO_ReadPin(BNO085_INT_PORT, BNO085_INT_PIN) == GPIO_PIN_SET)
    {
        /* wait for active-low INT */
    }
    return 0;
}

void sh2_hal_interruptEnable(bool enable)
{
    (void)enable;
}

/* ================= HAL STRUCT ================= */

sh2_Hal_t stm32_hal =
{
    .open      = stm32_i2c_open,
    .close     = stm32_i2c_close,
    .read      = stm32_i2c_read,
    .write     = stm32_i2c_write,
    .getTimeUs = stm32_getTimeUs
};
