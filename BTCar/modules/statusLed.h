#ifndef __STATUSLED__
#define __STATUSLED__

#define STATUS_LED_PORT PORTA
#define STATUS_LED_PIN BIT0

#define NO_ERROR 0
#define TEMP_ERROR 4

uint16_t status_led_next_update;
uint16_t status_led_target_delay;
uint8_t status_led_code;
uint8_t status_led_code_index;
int8_t updateStatusLed()
{
    if(status_led_next_update == status_led_target_delay)
    {
        if(status_led_code)
        {
            TOGGLE(STATUS_LED_PORT,STATUS_LED_PIN);
            status_led_target_delay = 2000;
            status_led_code_index++;
            if(status_led_code == status_led_code_index)
            {
                status_led_code_index = 0;
                status_led_target_delay = 10000;
                ENABLE(STATUS_LED_PORT,STATUS_LED_PIN);//set high == disable LED
            }
        }
        else
        {
            TOGGLE(STATUS_LED_PORT,STATUS_LED_PIN);
            status_led_target_delay = 5000;
        }
        status_led_next_update = 0;
    }
    status_led_next_update++;
    return 1;
}

void initializeStatusLed()
{
    status_led_target_delay = 5000;//~500ms
    SCHEDULER_addLowPriorityTask(updateStatusLed,0x0);
}
#endif
