#ifndef SEVEN_SEG_H
#define SEVEN_SEG_H

#include <stdio.h>
#include <stdbool.h>








// prints a dumb sentence
void print_my_message();

// WARNING: Call i2c start before calling this. I'll organize it better later
bool seven_seg_begin();

bool seven_seg_reset();

void seven_seg_display_on();

void seven_seg_display_off();

void seven_seg_brightness(uint8_t brightness);

void seven_seg_display_test(uint8_t pos, uint8_t num);

void seven_seg_display_temp(int temperature);


#endif