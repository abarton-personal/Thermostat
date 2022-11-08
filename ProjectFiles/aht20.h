#ifndef AHT20_H
#define AHT20_H

void aht20_initialize();

void aht20_start_measurement();

void aht20_read_measurement();

int aht20_get_temp();

int aht20_get_humidity();


#endif