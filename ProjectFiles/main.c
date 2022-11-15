#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <math.h>
#include "seven_seg.h"
#include "aht20.h"
#include "i2c_module.h"
#include "timers.h"
#include "circular_buffer.h"

#define BTN_PRESSED 0
#define BTN_RELEASED 1
#define ON 1
#define OFF 0
#define TEMP_UP 1
#define TEMP_DOWN 0

#define TEMP_THRESHOLD 15 // 1.5 degrees
#define SET_TEMP_TIMEOUT_TIME 2000
#define WAIT_INIT_TIME 6000
#define SENSE_INTERVAL 10000
#define INIT_MESSAGE ((int[4]){11,14,14,15}) //todo: update charmap to allow more letters

const uint RELAY_PIN = 0;
const uint UP_PIN = 14;
const uint DOWN_PIN = 12;
const uint CYCLE_PIN = 9;
const uint LED_PIN = PICO_DEFAULT_LED_PIN;

static volatile int temperature_setting;
static volatile int current_temperature;
static volatile int current_humidity;

enum ui_state {
    display_temp,
    display_humid,
    display_none,
};
static enum ui_state current_state;
static bool user_setting_temp = false;


//timer stuff
static TimerHandle_t screen_timeout_timer = NULL;
static TimerHandle_t wait_init_timer = NULL;


//function declarations
void manage_sensor();




/*****************************************************/
/****************** Callback Functions ***************/
/*****************************************************/

// After user presses a button, and after this timer runs out, reset the 
// display to show the current temp/humidity again
void screen_timeout_callback(TimerHandle_t xTimer){
    user_setting_temp = false;
    if(current_state == display_temp) 
        seven_seg_display_temp(current_temperature);
    else if(current_state == display_humid) 
        seven_seg_display_humidity(current_humidity); 
    else if(current_state == display_none)
        seven_seg_display_off();
}

// start the sensor only after the timer runs out
void start_sensor_callback(){
    xTaskCreate(manage_sensor, "manage_aht20", 256, NULL, 2, NULL);
}



/*****************************************************/
/****************** INIT Functions *******************/
/*****************************************************/

// init buttons and relay pin
void intialize_ios(){
    stdio_init_all();
    //initialize led pin    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    //initialize buttons
    gpio_init(UP_PIN);
    gpio_init(DOWN_PIN);
    gpio_init(CYCLE_PIN);
    gpio_set_dir(UP_PIN, GPIO_IN);
    gpio_set_dir(DOWN_PIN, GPIO_IN);
    gpio_set_dir(CYCLE_PIN, GPIO_IN);
    gpio_set_pulls(UP_PIN, true, false); 
    gpio_set_pulls(DOWN_PIN, true, false); 
    gpio_set_pulls(CYCLE_PIN, true, false); 
    //initialize relay output
    gpio_init(RELAY_PIN);
    gpio_set_dir(RELAY_PIN, GPIO_OUT);
    gpio_set_pulls(RELAY_PIN, false, true); //set pulldown resistor
    gpio_put(RELAY_PIN, OFF); // make sure it's off to begin
}


// initialize all stuffs
void system_initialize(){

    // variable inital values
    current_state = display_temp;
    temperature_setting = 700;
    current_temperature = 999;
    current_humidity = 99;
    user_setting_temp = false;

    buffer_initialize(temperature_setting+20); //+20 so the relay doesn't turn on at first

    //initialize buttons and relay pin
    intialize_ios();

    //initialize peripherals    
    i2c_module_initialize(); 
    seven_seg_begin(); 

    //after short delay
    vTaskDelay(20); 
    // aht20 needs some time to power up
    aht20_initialize();
    // don't take measurements until after some more time has passed
    xTimerStart(wait_init_timer, portMAX_DELAY);

    //display something fun while we wait for temperature reading
    seven_seg_display_test(INIT_MESSAGE);

    // done initializing
    vTaskDelete(NULL);

}



/*****************************************************/
/****************** Helper Functions *****************/
/*****************************************************/

// when the user presses the cycle button, switch between displaying 
// temperature, humidity, and nothing
static void cycle_display_state(){        

    if(current_state==display_temp){
        current_state = display_humid;
        seven_seg_display_humidity(current_humidity); 
    } 
    
    else if(current_state==display_humid){
        current_state = display_none;
        seven_seg_display_off();
    } 
    
    else{
        current_state = display_temp;
        seven_seg_display_temp(current_temperature); 
    } 
}



// increase or decrease the temperature setting by temp_delta
void change_temperature_setting(int temp_delta){
    // I want first btn press to just show the setting, and subsequent
    // presses to actually change the setting. So check if user_setting_temp
    if(user_setting_temp){
        //increase or decrease
        temperature_setting += temp_delta;                
    } else {
        user_setting_temp = true;    
    }

    //show setting for a few seconds then return to actual temp                
    xTimerStart(screen_timeout_timer, portMAX_DELAY);
    seven_seg_display_temp(temperature_setting);
    printf("temperature set to %d\n", temperature_setting);     
}




/*****************************************************/
/* "TASKS" *******************************************/
/*****************************************************/

// blink an LED.
void led_task()
{   
    while (true) {
        gpio_put(LED_PIN, ON);
        vTaskDelay(500);
        gpio_put(LED_PIN, OFF);
        vTaskDelay(500);        
        printf("blonk\n");
    }
}



// repeatedly reads data and displays it 
void manage_sensor(){

    while(true){
        // trigger measurement
        aht20_start_measurement();
        vTaskDelay(80);
        // read measurement from i2c and calculate
        aht20_read_measurement();

        // retrieve measurements from module
        int temp_reading = aht20_get_temp();
        current_humidity = aht20_get_humidity();

        buffer_append(temp_reading);
        current_temperature = buffer_get_avg();

        // update display
        if(!user_setting_temp){
            if(current_state == display_temp) 
                seven_seg_display_temp(current_temperature);
            if(current_state == display_humid) 
                seven_seg_display_humidity(current_humidity); 
        }
        printf("temp: \t\t%0.2fF\n", (float)current_temperature/10);
        printf("humidity: \t%d%%\n\n", current_humidity);
        
        // delay until next time
        vTaskDelay(SENSE_INTERVAL-80);
    }
}




// checks if the temperature setting is above or below the actual temperature
// and sets the relay accordingly
void manage_relay(){
        
    static int relay_state = OFF;

    while(true){
        if (relay_state == OFF){
            if(current_temperature < (temperature_setting)){
                relay_state = ON;       
                gpio_put(RELAY_PIN, relay_state);                 
                printf("relay on\n");
            } 
        }
        else {
            if (current_temperature > (temperature_setting + TEMP_THRESHOLD)){
                relay_state = OFF;
                gpio_put(RELAY_PIN, relay_state);    
                printf("relay off\n");
            }
        }
        
        vTaskDelay(1000);
    }
}



// Checks for button presses and performs the actions
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
            change_temperature_setting(10);
        }

        if(down_btn_state == BTN_RELEASED && gpio_get(DOWN_PIN) == BTN_PRESSED){
            down_btn_state = BTN_PRESSED;            
            change_temperature_setting(-10);
        }
        
        if(cycle_btn_state == BTN_RELEASED && gpio_get(CYCLE_PIN) == BTN_PRESSED){
            cycle_btn_state = BTN_PRESSED;
            //do cycle button stuff
            cycle_display_state();            
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
    /* Create Timers */

    //timer for set temp. When the user presses up or down button, the
    //display will show the temp setting. After timeout, it goes back
    //to displaying whatever was there before they pressed the button.
    screen_timeout_timer = xTimerCreate(
        "screen_timeout",
        SET_TEMP_TIMEOUT_TIME,
        false,
        (void *)0,
        screen_timeout_callback
    );
        
    // Wait for a while before accessing the AHT20, because it spits out
    // garbage for a few seconds after powerup.
    wait_init_timer = xTimerCreate(
        "wait_init",
        WAIT_INIT_TIME,
        false,
        (void *)0,
        start_sensor_callback
    ); 

    /* Create Tasks */

    xTaskCreate(
                    led_task,    // Function that implements the task. 
                    "LED_Task",  // Text name for the task. 
                    256,         // Stack size in words, not bytes.
                    NULL,        // Parameter passed into the task. 
                    1,           // Priority at which the task is created. 
                    NULL );      // Used to pass out the created task's handle. 

    xTaskCreate(system_initialize, "system_initialize", 256, NULL, 5, NULL);
    xTaskCreate(get_inputs, "get_inputs", 256, NULL, 2, NULL);
    xTaskCreate(manage_relay, "manage_relay", 256, NULL, 1, NULL);
    
    
    vTaskStartScheduler();


    while(1){};

}




// interrupt callbacks for buttons
// flesh out and clean up seven seg functions - need more testing