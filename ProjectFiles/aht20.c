#include "aht20.h"
#include "i2c_module.h"
#include "stdbool.h"

#define AHT20_ADDRESS 0x38
#define AHT20_INITIALIZE_BYTE 0xBE
#define AHT20_MEASURE_BYTE 0xAC

#define TEMP_HUM_DEFAULT_VALUE 0

static bool initialized = false;
static int temperature = TEMP_HUM_DEFAULT_VALUE;
static int humidity = TEMP_HUM_DEFAULT_VALUE;

void aht20_initialize(){
    // Note: make sure device has been powered on for 20 milliseconds 
    // before communicating

    uint8_t txdata[3] = {AHT20_INITIALIZE_BYTE, 0x08, 0x00};
    i2c_module_send(AHT20_ADDRESS, txdata, 3);
    initialized = true;
}


void aht20_start_measurement(){
    // don't send data until the device has been initialized
    if (!initialized) return;

    // trigger measurement
    uint8_t txdata[3] = {AHT20_MEASURE_BYTE, 0x33, 0x00};
    i2c_module_send(AHT20_ADDRESS, txdata, 3);

    //takes at least 80ms before measurement is ready to read
}

void aht20_read_measurement(){
    // don't send data until the device has been initialized
    if (!initialized) return;

    //get recent temperature measurement
    uint8_t rxdata[6];
    i2c_module_read(AHT20_ADDRESS, rxdata, 6);

    //calculate it from raw values
    unsigned long __humi = 0;
    unsigned long __temp = 0;

    __humi = rxdata[1];
    __humi <<= 8;
    __humi += rxdata[2];
    __humi <<= 4;
    __humi += rxdata[3] >> 4;

    // multiply by magic constant, as per the datasheet
    float h = (float)__humi/1048576.0;
    int h_int = (int)(h*100);
    humidity = h_int;

    __temp = rxdata[3]&0x0f;
    __temp <<=8;
    __temp += rxdata[4];
    __temp <<=8;
    __temp += rxdata[5];

    float t = (float)__temp/1048576.0*200.0-50.0;        
    float t_f = t * 9 / 5 + 32;

    //temp int is the temperature in 10ths of a degree. (75.2 = 752)
    int temp_int = (int) (t_f * 10);

    //make sure it's a valid number
    //(arbitrary limits, since I'm measuring room temp, 
    //between 30 and 110 degrees would be expected)
    if (temp_int > 300 && temp_int < 1100)
        temperature = temp_int;        

    // print them
    // printf("temp: \t\t%0.2fF\n", t_f);
    // printf("humidity: \t%d%%\n\n", h_int);
}

int aht20_get_temp(){
    return temperature;
}

int aht20_get_humidity(){
    return humidity;
}