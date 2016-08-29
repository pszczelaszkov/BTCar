#ifndef __ONEWIRE__
#define __ONEWIRE__
#include "scheduler.h"

#define ONE_WIRE_READY 8
#define ONE_WIRE_WRITE 9
#define ONE_WIRE_READ 10
#define ONE_WIRE_RESET 11
#define ONE_WIRE_RELEASE_RESET 12
#define ONE_WIRE_READ_PRESENCE 13

#define ONE_WIRE_DDR DDRB
#define ONE_WIRE_PIN PINB
#define ONE_WIRE_PORT PORTB
#define ONE_WIRE_PBIT BIT1

TimedTask* one_wire_task;
volatile int8_t one_wire_status;
volatile uint8_t one_wire_buffer;
int8_t oneWireDispatcher();

int8_t oneWireReset()
{
    switch(one_wire_status)
    {
        case ONE_WIRE_RESET:
            ENABLE(ONE_WIRE_DDR,ONE_WIRE_PBIT);
            one_wire_task->ticks_to_go = 5;//wait 500us
        break;
        case ONE_WIRE_RELEASE_RESET:
            DISABLE(ONE_WIRE_DDR,ONE_WIRE_PBIT);
            one_wire_task->ticks_to_go = 1;//wait 100us
        break;
        case ONE_WIRE_READ_PRESENCE:
            one_wire_buffer = READ(ONE_WIRE_PIN,ONE_WIRE_PBIT);
            one_wire_task->ticks_to_go = 4;//wait 400us
        break;
        default:
            one_wire_task->fptr = oneWireDispatcher;
            one_wire_task->ticks_to_go = 1;
            one_wire_status = ONE_WIRE_READY;
            return 1;
        break;
    }
    one_wire_status++;
    return 1;
}

int8_t oneWireSend()
{
    DISABLE(ONE_WIRE_DDR,ONE_WIRE_PBIT);
    if(one_wire_buffer & 0x01)//1
    {
        ENABLE(ONE_WIRE_DDR,ONE_WIRE_PBIT);
        one_wire_status++;
        DISABLE(ONE_WIRE_DDR,ONE_WIRE_PBIT);
    }
    else//0
    {
        ENABLE(ONE_WIRE_DDR,ONE_WIRE_PBIT);
        one_wire_status++;
    }
    one_wire_buffer >>= 1;
    if(one_wire_status == 8)
        one_wire_task->fptr = oneWireDispatcher;

    return 1;
}

int8_t oneWireRead()
{
    ENABLE(ONE_WIRE_DDR,ONE_WIRE_PBIT);
    one_wire_status++;
    DISABLE(ONE_WIRE_DDR,ONE_WIRE_PBIT);
    one_wire_buffer >>= 1;
    uint8_t value = READ(ONE_WIRE_PIN,ONE_WIRE_PBIT);
    if(value)
        value = 0x80;//last bit

    one_wire_buffer |= value;
    if(one_wire_status == 8)
        one_wire_task->fptr = oneWireDispatcher;

    return 1;
}

int8_t oneWireDispatcher()
{
    one_wire_task->ticks_to_go = 1;//Make sure update is always per 100us
    DISABLE(ONE_WIRE_DDR,ONE_WIRE_PBIT);//and bus is released
    switch(one_wire_status)
    {
        case ONE_WIRE_RESET:
            one_wire_task->fptr = oneWireReset;
        break;
        case ONE_WIRE_WRITE:
            one_wire_status = 0;
            one_wire_task->fptr = oneWireSend;
        break;
        case ONE_WIRE_READ:
            one_wire_status = 0;
            one_wire_buffer = 0;
            one_wire_task->fptr = oneWireRead;
        break;
    }

    return 1;
}

TimedTask* initializeOneWire()
{
        DISABLE(ONE_WIRE_PORT,ONE_WIRE_PBIT);
        one_wire_task = SCHEDULER_addScheduledTask(oneWireDispatcher,1,0x0);
        one_wire_status = ONE_WIRE_READY;
        return one_wire_task;
}
#endif
