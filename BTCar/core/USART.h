#ifndef __USART__
#define __USART__
#include <avr/interrupt.h>
#include "scheduler.h"
#include "string.h"

#define USART_BAUDRATE 51
#define USART_RX_BUFFER_SIZE 8
#define USART_TX_BUFFER_SIZE 16

#define USART_PARSING_FINISHED 0
#define USART_PARSING_INWORK 1
#define EOT 4
#define USART_TEMPERATURE 3
#define USART_RTC 4
/*
    PARSING STARTS WITH /r
    ENDS WITH RESULTING TX
*/
char USART_income_buffer[USART_RX_BUFFER_SIZE];
char USART_outcome_buffer[USART_TX_BUFFER_SIZE];
uint8_t USART_income_buffer_index;
uint8_t USART_outcome_buffer_index;
int8_t USART_parsing_status;
void (*USART_parsing_function)();

//USARTcallbacks
void SERVO_onUSART();

void USARTtransmit()
{
    //if( !( UCSRA & (1<<UDRE)) )//transmit buffer not empty
    //    return;

    if(!USART_parsing_status)
        return;

    char buffer = USART_outcome_buffer[USART_outcome_buffer_index];
    USART_outcome_buffer[USART_outcome_buffer_index] = 0;

    if(buffer == EOT)
    {
        USART_outcome_buffer_index = USART_TX_BUFFER_SIZE;//set max value so its finished
        USART_parsing_status = USART_PARSING_FINISHED;
        return;
    }
    UDR = buffer;
    USART_outcome_buffer_index++;
}

int8_t USARTparse()
{
  /*  if(USART_outcome_buffer_index == USART_TX_BUFFER_SIZE)
    {
        USART_parsing_function();

        for(int8_t i = 0;i < USART_RX_BUFFER_SIZE;i++)
            USART_income_buffer[i] = 0;

        USART_income_buffer_index = 0;
    }
    if(USART_outcome_buffer_index < USART_TX_BUFFER_SIZE)
        USARTtransmit();
*/
    if(USART_outcome_buffer_index == USART_TX_BUFFER_SIZE)
    {
        USART_parsing_function();
        for(int8_t i = 0;i < USART_RX_BUFFER_SIZE;i++)
            USART_income_buffer[i] = 0;
        USART_income_buffer_index = 0;
    }
    else if(USART_outcome_buffer_index == 0)//send only first byte rest is handled by interrupts
    {
        USARTtransmit();
        return 0;//force finish task;
    }
    return USART_parsing_status;
}

void USARTonUSART()
{
    if(strncmp(USART_income_buffer,"servo",5) == 0)
        USART_parsing_function = SERVO_onUSART;
    else
        USART_parsing_status = USART_PARSING_FINISHED;
}

int8_t USARTupdate()
{
    //if(UCSRA & (1<<RXC))
  //  {
        uint8_t buffer = UDR;
        if(buffer == '\r' && !USART_parsing_status)
        {
            USART_parsing_status = USART_PARSING_INWORK;
            SCHEDULER_addLowPriorityTask(USARTparse,0x0);
            return 1;
        }
        if(USART_income_buffer_index < USART_RX_BUFFER_SIZE || buffer == 0x7f)
        {
            if(buffer == 0x7f)//DEL
            {
                if(USART_income_buffer_index > 0)
                    USART_income_buffer_index--;

                USART_income_buffer[USART_income_buffer_index] = 0;
            }
            else
            {
                USART_income_buffer[USART_income_buffer_index] = buffer;
                USART_income_buffer_index++;
            }
            UDR = buffer;//send back
        }
   //}
    return 1;
}

void initializeUSART()
{
    USART_parsing_function = USARTonUSART;
    USART_outcome_buffer_index = USART_TX_BUFFER_SIZE;

    uint8_t baud = USART_BAUDRATE;
    UBRRH = (uint8_t)(baud>>8);
    UBRRL = (uint8_t)baud;
    UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE)|(1<<TXCIE);
    UCSRC = (1<<URSEL)|(3<<UCSZ0);//frame format: 8data, 1stop bit
   // SCHEDULER_addScheduledTask(USARTupdate,1,0x0);
}

ISR(UART_RX_vect)
{
    USARTupdate();
}
ISR(UART_TX_vect)
{
    USARTtransmit();
}
#endif
