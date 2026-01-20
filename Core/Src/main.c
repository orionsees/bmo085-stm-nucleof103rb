#include "main.h"
#include "bno085_stm32.h"

I2C_HandleTypeDef hi2c1;

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();

    // Initialize BNO085
    if (BNO085_Init() != 0) {
        Error_Handler();
    }

    // Enable rotation vector at 100Hz
    BNO085_EnableRotationVector(10000); // 10ms = 100Hz

    while (1) {
        // Service the sensor (call frequently)
        BNO085_Service();
        HAL_Delay(1);
    }
}
