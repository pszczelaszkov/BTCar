#ifndef __DISPLAY__
#define __DISPLAY__
#include <avr/io.h>
#include <string.h>
#include "../core/scheduler.h"
#include "../core/hd44780.h"
#include "../core/softRTC.h"
#include "temperature.h"

#define DISPLAY_BUFFER_SIZE 16
#define DISPLAY_MODE_SWITCH_INTERVAL 20

char display_buffer[DISPLAY_BUFFER_SIZE];
int8_t display_status;
int8_t display_status_save;
uint8_t display_mode_counter;
uint8_t display_mode_status;
uint16_t display_next_update;
HD44780* display;

int8_t updateDisplay()
{
        if(display_status >= 0)//Send data,display_status works as index
        {
            if(display_buffer[display_status])
            {
                sendHD44780(display,display_buffer[display_status],1,0);
                display_buffer[display_status] = 0;
                display_status++;
            }
            else
                display_status = DISPLAY_BUFFER_SIZE;

            if(display_status == DISPLAY_BUFFER_SIZE)
                    display_status = display_status_save;
        }
        else
        {
            if(display_next_update == 2000)//~200ms
            {
                int8_t i;
                switch(display_status)
                {
                    case -1:
                        sendHD44780(display,DDRAM_ADDR,0,0);
                        strcpy(display_buffer,"Scheduler: ");
                        display_status_save = -2;
                        display_status = 0;
                    break;
                    case -2:
                        sendHD44780(display,DDRAM_ADDR | 11,0,0);
                        itoa(time,display_buffer,10);
                        for(i = 0;i < DISPLAY_BUFFER_SIZE;i++)
                        {
                            if(display_buffer[i] == 0)
                            {
                                display_buffer[i] = '%';
                                break;
                            }
                        }
                        display_status_save = -3;
                        display_status = 0;
                    break;
                    case -3:
                        switch(display_mode_status)
                        {
                            case 0:
                                sendHD44780(display,DDRAM_ADDR | SECOND_LINE | 6,0,0);
                                temperatureOnDisplay(display_buffer);
                            break;
                            case 1:
                                sendHD44780(display,DDRAM_ADDR | SECOND_LINE | 3,0,0);
                                dateOnDisplay(display_buffer);
                                display_mode_status = 2;
                            break;
                            case 2:
                                sendHD44780(display,DDRAM_ADDR | SECOND_LINE | 3,0,0);
                                dateOnDisplay(display_buffer);
                            break;
                            case 3:
                                sendHD44780(display,DDRAM_ADDR | SECOND_LINE | 4,0,0);
                                timeOnDisplay(display_buffer);
                                display_mode_status = 4;
                            break;
                            case 4:
                                sendHD44780(display,DDRAM_ADDR | SECOND_LINE | 4,0,0);
                                timeOnDisplay(display_buffer);
                            break;
                        }
                        display_status_save = -2;
                        display_status = 0;
                    break;
                }
                display_next_update = 0;
                if(display_mode_counter == 0)
                {
                    //switch up cleaning
                    sendHD44780(display,DDRAM_ADDR | SECOND_LINE,0,0);
                    strcpy(display_buffer,"                ");
                    display_mode_counter = DISPLAY_MODE_SWITCH_INTERVAL;
                    if(display_mode_status == 4)
                        display_mode_status = 0;
                    else
                        display_mode_status++;
                }
                else
                    display_mode_counter--;
            }
        }
        display_next_update++;
   return 1;
}

void initializeDisplay()
{
    display = addDisplayHD44780(&PORTA,&PORTA,BIT3,BIT1,BIT2,BM_HALF_BYTE);
    initializeHD44780(display);
    sendHD44780(display,DISPLAY_CTRL | DISPLAY_ON,0,0);
    display_status = -1;
    display_mode_counter = DISPLAY_MODE_SWITCH_INTERVAL;
    SCHEDULER_addLowPriorityTask(updateDisplay,0x0);
}

#endif
