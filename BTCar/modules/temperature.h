#ifndef __TEMPERATURE__
#define __TEMPERATURE__
#include <stdlib.h>
#include <string.h>
#include "../core/scheduler.h"
#include "../core/oneWire.h"
#include "../core/USART.h"
#include "statusLed.h"

volatile int8_t temperature_buffer;
volatile int8_t temperature;
int8_t temperature_status;
uint8_t temperature_usart_send_index;

int8_t updateTemperature()
{
    if(one_wire_status == ONE_WIRE_READY)
    {
        switch(temperature_status)
        {
            case 0:
                one_wire_status = ONE_WIRE_RESET;
                temperature_status = 1;
            break;
            case 1:
                if(one_wire_buffer == 0)
                {
                    temperature_status = 2;
                    status_led_code = NO_ERROR;
                }
                else
                {
                    temperature_status = 0;
                    status_led_code = TEMP_ERROR;
                }
            break;
            case 2:
                one_wire_buffer = 0xcc;
                one_wire_status = ONE_WIRE_WRITE;
                temperature_status = 3;
            break;
            case 3:
                one_wire_buffer = 0x44;
                one_wire_status = ONE_WIRE_WRITE;
                temperature_status = 4;
            break;
            case 4:
                one_wire_status = ONE_WIRE_READ;
                if(one_wire_buffer)
                {
                    temperature_status = 5;
                }
            break;
            case 5:
                one_wire_status = ONE_WIRE_RESET;
                temperature_status = 6;
            break;
            case 6:
                one_wire_buffer = 0xcc;
                one_wire_status = ONE_WIRE_WRITE;
                temperature_status = 7;
            break;
            case 7:
                one_wire_buffer = 0xbe;
                one_wire_status = ONE_WIRE_WRITE;
                temperature_status = 8;
            break;
            case 8:
                one_wire_status = ONE_WIRE_READ;
                temperature_status = 9;
            break;
            case 9:
                temperature_buffer = one_wire_buffer >> 4;
                one_wire_status = ONE_WIRE_READ;
                temperature_status = 10;
            break;
            case 10:
                temperature_buffer |= one_wire_buffer << 4;
                temperature = temperature_buffer;
                temperature_status = 0;
            break;
        }
    }
    return 1;
}

void temperatureOnDisplay(char* display_buffer)
{
    itoa(temperature,display_buffer,10);
    for(int8_t i = 0;i < 16;i++)
    {
        if(display_buffer[i] == 0)
        {
            display_buffer[i] = 0xDF;//Degree symbol
            display_buffer[i+1] = 'C';
            break;
        }
    }
}

void temperatureOnUSART()
{
    USART_outcome_buffer[0] = '\r';
    USART_outcome_buffer[1] = '\n';
    itoa(temperature,USART_outcome_buffer+2,10);
    USART_outcome_buffer[6] = '\r';
    USART_outcome_buffer[7] = '\n';
    USART_outcome_buffer[8] = EOT;
    USART_outcome_buffer_index = 0;//TX starts when index < tx buffer size
    USART_parsing_function = USARTonUSART;
}
void initializeTemperature()
{
    initializeOneWire();
    SCHEDULER_addLowPriorityTask(updateTemperature,0x0);
}
#endif
