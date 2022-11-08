#include "i2c_module.h"

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"



struct message {
    int addr;
    uint8_t* txbuffer;
    int length;
};

//todo: make a message queue, have this module handle
// all timing, to avoid hardware conflicts


void i2c_module_initialize(){
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
}



void i2c_module_send(int addr, uint8_t* txdata, int length){
        
    i2c_write_blocking(i2c_default, addr, txdata, length, false);
}


void i2c_module_read(int addr, uint8_t* rxdata, int length){

    i2c_read_blocking(i2c_default, addr, rxdata, 6, false);

}
