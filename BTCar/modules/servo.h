#ifndef SERVO_H
#define SERVO_H
#include <avr/io.h>
#include "../core/USART.h"
#include <stdlib.h>

#define SERVO_SELECT_MODE 0
#define SERVO_MODE_GET 1
#define SERVO_MODE_SET 2
uint16_t SERVO_servos[1];
int8_t SERVO_USART_command;

void SERVO_onUSART()
{
    USART_outcome_buffer[0] = '\r';
    USART_outcome_buffer[1] = '\n';
    USART_outcome_buffer[2] = EOT;
    switch(SERVO_USART_command)
    {
        case SERVO_SELECT_MODE:
            if(strncmp(USART_income_buffer,"get",3) == 0)
                SERVO_USART_command= SERVO_MODE_GET;
            else if(strncmp(USART_income_buffer,"set",3) == 0)
            {
                SERVO_USART_command = SERVO_MODE_SET;
                USART_outcome_buffer_index = 0;
            }
            else
                USART_outcome_buffer_index = 0;
        break;
        case SERVO_MODE_GET:
            USART_outcome_buffer[0] = '\r';
            USART_outcome_buffer[1] = '\n';
            itoa(SERVO_servos[0],USART_outcome_buffer+2,10);
            USART_outcome_buffer[6] = '\r';
            USART_outcome_buffer[7] = '\n';
            USART_outcome_buffer[8] = EOT;
            USART_outcome_buffer_index = 0;//TX starts when index < tx buffer size
            USART_parsing_function = USARTonUSART;
            SERVO_USART_command = SERVO_SELECT_MODE;
        break;
        case SERVO_MODE_SET:
            SERVO_servos[0] = atoi(USART_income_buffer);
            OCR1B = SERVO_servos[0];
            USART_parsing_function = USARTonUSART;
            SERVO_USART_command = SERVO_SELECT_MODE;
            USART_outcome_buffer_index = 0;
        break;
    }
}

void SERVO_init()
{
    ICR1 = 20000;//50Hz
    OCR1B = 1600;
    TCCR1A = (1<<COM1B1) | (1<<WGM11);
    TCCR1B = (1<<WGM13) | (1<<CS11);
}
#endif
