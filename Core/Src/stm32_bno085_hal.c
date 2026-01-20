#include "stm32f1xx_hal.h"
#include "sh2_hal.h"
#include <string.h>

// Configuration
#define BNO085_I2C_ADDRESS 0x4A
#define BNO085_RESET_PIN GPIO_PIN_0
#define BNO085_RESET_PORT GPIOA

// External I2C handle (configure in CubeMX)
extern I2C_HandleTypeDef hi2c1;

// Static HAL implementation for STM32
static int stm32_i2c_open(sh2_Hal_t *self);
static void stm32_i2c_close(sh2_Hal_t *self);
static int stm32_i2c_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us);
static int stm32_i2c_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len);
static uint32_t stm32_getTimeUs(sh2_Hal_t *self);

// Timer for microsecond timing
static uint32_t microsCounter = 0;

void HAL_SYSTICK_Callback(void) {
    microsCounter += 1000; // SysTick typically 1ms
}

static uint32_t stm32_getTimeUs(sh2_Hal_t *self) {
    return microsCounter;
}

static int stm32_i2c_open(sh2_Hal_t *self) {
    // Software reset sequence
    uint8_t softreset_pkt[] = {5, 0, 1, 0, 1};

    for (uint8_t attempts = 0; attempts < 5; attempts++) {
        if (HAL_I2C_Master_Transmit(&hi2c1, BNO085_I2C_ADDRESS << 1,
                                     softreset_pkt, 5, 100) == HAL_OK) {
            HAL_Delay(300);
            return 0;
        }
        HAL_Delay(30);
    }
    return -1;
}

static void stm32_i2c_close(sh2_Hal_t *self) {
    // Nothing specific needed
}

static int stm32_i2c_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us) {
    uint8_t header[4];

    // Read 4-byte header
    if (HAL_I2C_Master_Receive(&hi2c1, BNO085_I2C_ADDRESS << 1,
                                header, 4, 100) != HAL_OK) {
        return 0;
    }

    // Get packet size
    uint16_t packet_size = (uint16_t)header[0] | ((uint16_t)header[1] << 8);
    packet_size &= ~0x8000; // Clear continuation bit

    if (packet_size > len || packet_size == 0) {
        return 0;
    }

    // Read payload
    uint16_t cargo_remaining = packet_size;
    uint16_t cursor = 0;

    // First read
    uint16_t first_read = (cargo_remaining < 128) ? cargo_remaining : 128;
    if (HAL_I2C_Master_Receive(&hi2c1, BNO085_I2C_ADDRESS << 1,
                                pBuffer, first_read, 100) != HAL_OK) {
        return 0;
    }

    cursor += first_read;
    cargo_remaining -= first_read;

    // Continue reading if needed
    while (cargo_remaining > 0) {
        uint8_t temp_buf[132]; // 4 byte header + 128 data
        uint16_t read_size = (cargo_remaining + 4 < 132) ? (cargo_remaining + 4) : 132;

        if (HAL_I2C_Master_Receive(&hi2c1, BNO085_I2C_ADDRESS << 1,
                                    temp_buf, read_size, 100) != HAL_OK) {
            return 0;
        }

        // Copy data (skip 4-byte header)
        uint16_t data_size = read_size - 4;
        memcpy(pBuffer + cursor, temp_buf + 4, data_size);
        cursor += data_size;
        cargo_remaining -= data_size;
    }

    *t_us = stm32_getTimeUs(self);
    return packet_size;
}

static int stm32_i2c_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len) {
    if (HAL_I2C_Master_Transmit(&hi2c1, BNO085_I2C_ADDRESS << 1,
                                 pBuffer, len, 100) == HAL_OK) {
        return len;
    }
    return 0;
}

// Initialize HAL structure
sh2_Hal_t stm32_hal = {
    .open = stm32_i2c_open,
    .close = stm32_i2c_close,
    .read = stm32_i2c_read,
    .write = stm32_i2c_write,
    .getTimeUs = stm32_getTimeUs
};
