#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <math.h>
#include "seven_seg.h"
#include "aht20.h"
#include "i2c_module.h"



#define BTN_PRESSED 0
#define BTN_RELEASED 1
#define ON 1
#define OFF 0

#define TEMP_THRESHOLD 10 //1 degree

const uint RELAY_PIN = 15;
const uint UP_PIN = 18;
const uint DOWN_PIN = 19;
const uint CYCLE_PIN = 20;

static volatile int temperature_setting;
static volatile bool i2c_in_use = false;
static volatile bool button_is_pressed = false;
static volatile int current_temp = 999;
static volatile int current_humi = 999;

enum ui_state {
    display_temp,
    display_humid,
    display_none,
    set_temp,
};
enum ui_state current_state;











// just an easy verification that stuff was compiled and uploaded correctly
void led_task()
{   
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        gpio_put(LED_PIN, 1);
        vTaskDelay(500);
        gpio_put(LED_PIN, 0);
        vTaskDelay(500);
        gpio_put(LED_PIN, 1);
        vTaskDelay(500);
        gpio_put(LED_PIN, 0);
        vTaskDelay(500);
        gpio_put(LED_PIN, 1);
        vTaskDelay(500);
        gpio_put(LED_PIN, 0);
        vTaskDelay(1500);
        // if(button_is_pressed){
        //     gpio_put(LED_PIN, 1);
        // } else {
        //     gpio_put(LED_PIN, 0);
        // }
        // vTaskDelay(10);
    }
}




// initializes and then repeatedly reads data
void manage_aht20(){

    // wait for it to power up before sending data
    vTaskDelay(20);
    aht20_initialize();

    while(true){
        // trigger measurement
        aht20_start_measurement();
        vTaskDelay(80);
        // read measurement from i2c and calculate
        aht20_read_measurement();

        // retrieve measurements from module
        current_temp = aht20_get_temp();
        current_humi = aht20_get_humidity();
                

        // update display
        seven_seg_display_temp(current_temp);
        printf("temp: \t\t%0.2fF\n", (float)current_temp/10);
        printf("humidity: \t%d%%\n\n", current_humi);
        
        // do that every 3 seconds.
        vTaskDelay(2920);
    }
}





void manage_relay(){
        
    static int relay_state = OFF;
    gpio_put(RELAY_PIN, relay_state);
    
    //while setup is not complete, wait.
    //todo: use a flag instead of guessing time
    vTaskDelay(5000);

    while(true){
        if (relay_state == OFF){
            if(current_temp < (temperature_setting - TEMP_THRESHOLD)){
                relay_state = ON;       
                gpio_put(RELAY_PIN, relay_state);                 
                printf("relay on\n");
            } 
        }
        else {
            if (current_temp > (temperature_setting + TEMP_THRESHOLD)){
                relay_state = OFF;
                gpio_put(RELAY_PIN, relay_state);    
                printf("relay off\n");
            }
        }
        
        vTaskDelay(1000);
    }
}


void get_inputs(){
    // maybe try using hardware interrupts instead of this
    // https://raspberrypi.github.io/pico-sdk-doxygen/group__hardware__gpio.html#ga6347e27da3ab34f1ea65b5ae16ab724f


    static bool up_btn_state = BTN_RELEASED;
    static bool down_btn_state = BTN_RELEASED;
    static bool cycle_btn_state = BTN_RELEASED;
    while(true){
        //check if buttons have been pressed
        if(up_btn_state == BTN_RELEASED && gpio_get(UP_PIN) == BTN_PRESSED){
            up_btn_state = BTN_PRESSED;
            //do up button stuff
            temperature_setting += 10;
            printf("temperature set to %d\n", temperature_setting);
        }

        if(down_btn_state == BTN_RELEASED && gpio_get(DOWN_PIN) == BTN_PRESSED){
            down_btn_state = BTN_PRESSED;
            //do down button stuff
            temperature_setting -= 10;
            printf("temperature set to %d\n", temperature_setting);
        }
        
        if(cycle_btn_state == BTN_RELEASED && gpio_get(CYCLE_PIN) == BTN_PRESSED){
            cycle_btn_state = BTN_PRESSED;
            //do cycle button stuff
            if(current_state==display_temp) current_state = display_humid;
            else if(current_state==display_humid) current_state = display_none;
            else current_state = display_temp;
            printf("current state set to %d\n", current_state);
        }

        //check if buttons have been released
        if(up_btn_state == BTN_PRESSED && gpio_get(UP_PIN) == BTN_RELEASED){
            up_btn_state = BTN_RELEASED;            
        }

        if(down_btn_state == BTN_PRESSED && gpio_get(DOWN_PIN) == BTN_RELEASED){
            down_btn_state = BTN_RELEASED;            
        }
        
        if(cycle_btn_state == BTN_PRESSED && gpio_get(CYCLE_PIN) == BTN_RELEASED){
            cycle_btn_state = BTN_RELEASED;            
        }


        vTaskDelay(5);
    }
    
}


int main()
{
    //initialize stuff
    stdio_init_all();
    i2c_module_initialize();
    seven_seg_begin();
    
    current_state = display_temp;
    temperature_setting = 700;
    
    //initialize buttons
    gpio_init(UP_PIN);
    gpio_init(DOWN_PIN);
    gpio_init(CYCLE_PIN);
    gpio_set_dir(UP_PIN, GPIO_IN);
    gpio_set_dir(DOWN_PIN, GPIO_IN);
    gpio_set_dir(CYCLE_PIN, GPIO_IN);
    //initialize relay output
    gpio_init(RELAY_PIN);
    gpio_set_dir(RELAY_PIN, GPIO_OUT);
    gpio_set_pulls(RELAY_PIN, false, true); //set pulldown resistor

    

    TaskHandle_t led_task_handle = NULL;

    xTaskCreate(
                    led_task,               // Function that implements the task. 
                    "LED_Task",             // Text name for the task. 
                    256,                    // Stack size in words, not bytes.
                    NULL,                   // Parameter passed into the task. 
                    1,                      // Priority at which the task is created. 
                    &led_task_handle );     // Used to pass out the created task's handle. 

    xTaskCreate(manage_aht20, "manage_aht20", 256, NULL, 2, NULL);
    xTaskCreate(get_inputs, "get_inputs", 256, NULL, 2, NULL);
    xTaskCreate(manage_relay, "manage_relay", 256, NULL, 1, NULL);
    
    
    vTaskStartScheduler();


    // aht20_get_measurement();

    while(1){};

}




// todo: study FreeRTOS best practices instead of building on top of hello world
// how to use FreeRTOS timers - move to set temp screen for set time
// flesh out and clean up seven seg functions - need more testing
// clean main the fuck up - 
// move aht20 to its own file
// user interface for setting temp