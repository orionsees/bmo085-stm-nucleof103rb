#ifndef BNO085_STM32_H
#define BNO085_STM32_H

#include <stdint.h>

int BNO085_Init(void);
int BNO085_EnableRotationVector(uint32_t interval_us);
void BNO085_Service(void);
void BNO085_Update(void);
#endif
