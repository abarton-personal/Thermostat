#ifndef I2C_MODULE_H
#define I2C_MODULE_H

#include "stdint.h"


void i2c_module_initialize();

void i2c_module_send(int addr, uint8_t* txdata, int length);

void i2c_module_read(int addr, uint8_t* rxdata, int length);


#endif
