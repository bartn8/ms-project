#ifndef __MS_GPIO_H__
#define __MS_GPIO_H__

#include "ms_commons.h"

void readSensors(float *sensors);
uint32_t readTicks();
void updateSensors();
void resetSensors();

#endif