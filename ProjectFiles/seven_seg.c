

#include "seven_seg.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "i2c_module.h"

#define MY_MESSAGE "get a dog little loggie\n"

#define HT16K33_ADDRESS 0x70

// Commands
#define HT16K33_ON              0x21  // 0=off 1=on
#define HT16K33_STANDBY         0x20  // bit xxxxxxx0

// bit pattern 1000 0xxy
// y    =  display on / off
// xx   =  00=off     01=2Hz     10=1Hz     11=0.5Hz
#define HT16K33_DISPLAYON       0x81
#define HT16K33_DISPLAYOFF      0x80
#define HT16K33_BLINKON0_5HZ    0x87
#define HT16K33_BLINKON1HZ      0x85
#define HT16K33_BLINKON2HZ      0x83
#define HT16K33_BLINKOFF        0x81

// bit pattern 1110 xxxx
// xxxx    =  0000 .. 1111 (0 - F)
#define HT16K33_BRIGHTNESS      0xE0

static const uint8_t charmap[18] = {  // TODO PROGMEM ?

  0x3F,   // 0
  0x06,   // 1
  0x5B,   // 2
  0x4F,   // 3
  0x66,   // 4
  0x6D,   // 5
  0x7D,   // 6
  0x07,   // 7
  0x7F,   // 8
  0x6F,   // 9
  0x77,   // A
  0x7C,   // B
  0x39,   // C
  0x5E,   // D
  0x79,   // E
  0x71,   // F
  0x00,   // space
  0x40,   // minus
};


static uint8_t displaycache[5] = {0,0,0,0,0};

void print_my_message(){
    printf(MY_MESSAGE);
};


static void refresh()
{
  for (uint8_t pos = 0; pos <= 4; pos++)
  {
    uint8_t buffer[2] = {(pos * 2), displaycache[pos]};
    i2c_module_send(HT16K33_ADDRESS, buffer, 2);
  }
}


bool seven_seg_begin(){    

    int ret;
    uint8_t buffer[1] = {HT16K33_ON};
    i2c_module_send(HT16K33_ADDRESS, buffer, 1);    

    seven_seg_display_on();
    seven_seg_brightness(0xF);
}

bool seven_seg_reset(){
    return true;
}

void seven_seg_display_on(){

    int ret;    
    uint8_t buffer[1] = {HT16K33_DISPLAYON};
    i2c_module_send(HT16K33_ADDRESS, buffer, 1);
}

void seven_seg_display_off(){

}

void seven_seg_brightness(uint8_t brightness){

    if (brightness > 0x0F) brightness = 0x0F;
    int ret;
    uint8_t buffer[1] = {HT16K33_BRIGHTNESS | brightness};  //E0 for off, EF for max brightness
    i2c_module_send(HT16K33_ADDRESS, buffer, 1);
}

void seven_seg_display_test(uint8_t pos, uint8_t num){
    
    int ret;
    uint8_t mask = charmap[num];
    uint8_t buffer[2] = {(pos*2), mask};
    // ret = i2c_write_blocking(i2c_default, HT16K33_ADDRESS, buffer, 2, false);
    displaycache[pos] = mask;
    // refresh writes all 4 positions, so why not use it 
    refresh();
    
    // save most recent to displaycache?
}


// arg temperature = temp * 10. IE 753 = 75.3 degrees
void seven_seg_display_temp(int temperature){ 
    
    uint8_t mask;
    // for (uint8_t pos=4; pos >= 0; pos-- ){
    //     // position 2 is the colon
    //     if(pos==2) {
    //         displaycache[pos] = 0x00;
    //         continue;
    //     }
    //     // else assign it to the right-most digit        
    //     int next = (temperature % 10);
    //     mask = charmap[next];    
        
    //     //decimal should be at the 3rd position ?
    //     if (pos==3) mask |= 0x80;

    //     // truncate to get the next digit of temperature
    //     temperature /= 10; 
    //     if(temperature > 0)
    //         displaycache[pos] = mask;
    //     else
    //         displaycache[pos] = 0x00;
                 
    // }
    if(temperature > 999 || temperature < 30) return;
    
    int next = temperature % 10;
    temperature /= 10;
    displaycache[4] = charmap[next];

    next = temperature % 10;
    temperature /= 10;
    displaycache[3] = charmap[next] | 0x80;
    
    displaycache[2] = 0x00;

    next = temperature % 10;    
    displaycache[1] = charmap[next];

    displaycache[0] = 0x00;
    
    refresh();
    
}