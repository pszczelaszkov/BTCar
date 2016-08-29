#ifndef __HD44780__
#define __HD44780__

#include "bitwise.h"
#include "util/atomic.h"
#define MAX_DISPLAYS 1
//FLAGS
#define BM_FULL_BYTE 1
#define BM_HALF_BYTE 0
#define FUNCTION_SET 0x20
#define TWO_LINES 0x08
#define BIG_FONT 0x04
#define DISPLAY_CLR 0x01
#define RETURN_HOME 0x02
#define DISPLAY_CTRL 0x08
#define DISPLAY_ON 0x04
#define CURSOR_ON 0x02
#define BLINKING_ON 0x01
#define DDRAM_ADDR 0x80
#define SECOND_LINE 0x40
//
typedef struct HD44780
{
    volatile uint8_t* data_port;
    volatile uint8_t* selectors_port;
    uint8_t E;
    uint8_t RS;
    uint8_t RW;
    uint8_t bus_mode;
}HD44780;

HD44780 hd44780_displays[MAX_DISPLAYS];
uint8_t last_hd44780_id = 0;

HD44780* addDisplayHD44780(volatile uint8_t* data_port,volatile uint8_t* selectors_port,uint8_t E, uint8_t RS,uint8_t RW,uint8_t bus_mode)
{
    if(last_hd44780_id >= MAX_DISPLAYS)
        return 0x0;

    HD44780* display = &hd44780_displays[last_hd44780_id];
    display->data_port = data_port;
    display->selectors_port = selectors_port;
    display->E = E;
    display->RS = RS;
    display->RW = RW;
    display->bus_mode = bus_mode;

    last_hd44780_id++;
    return display;
}

uint8_t readHD44780(HD44780* display)
{
    if(display == 0x0)
        return 0;

    volatile uint8_t* data_direction_register = display->data_port-1;
    volatile uint8_t* input_register = display->data_port-2;
    uint8_t result = 0;
    uint8_t mask = 0xff;
    uint8_t shifts = 0;

    if(display->bus_mode == BM_HALF_BYTE)
    {
        mask = 0xf0;
        shifts = 1;
    }
    //lockBufferedMutex(&scheduler_mutex);
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        DISABLE(*data_direction_register,mask);
        ENABLE(*display->selectors_port,display->RW);
    }
    //unlockBufferedMutex(&scheduler_mutex);
    uint8_t i;
    for(i = 0;i <= shifts;i++)
    {
        //lockBufferedMutex(&scheduler_mutex);
        ATOMIC_BLOCK(ATOMIC_FORCEON)
        {
            ENABLE(*display->selectors_port,display->E);
        }
        //unlockBufferedMutex(&scheduler_mutex);
        uint8_t temp = 0;
        temp |= READ(*input_register,mask);
        if(0 < i)
            temp = temp >> 4;
        //lockBufferedMutex(&scheduler_mutex);
        ATOMIC_BLOCK(ATOMIC_FORCEON)
        {
            DISABLE(*display->selectors_port,display->E);
        }
        //unlockBufferedMutex(&scheduler_mutex);
        result |= temp;
    }
    //lockBufferedMutex(&scheduler_mutex);
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        DISABLE(*display->selectors_port,display->RW);
        ENABLE(*data_direction_register,mask);
    }
    //unlockBufferedMutex(&scheduler_mutex);
    return result;
}

void sendHD44780(HD44780* display,uint8_t data,uint8_t RS,uint8_t only_high_order/*e.g initialization*/)
{
    if(display == 0x0)
        return;

    while(readHD44780(display) & BIT7){}//check BusyFlag
    uint8_t mask = 0xff;
    uint8_t shifts = 0;

    if(display->bus_mode == BM_HALF_BYTE)
    {
        mask = 0xf0;
        shifts = 1;
    }
    if(only_high_order)
    {
        mask = 0xf0;
        shifts = 0;
    }
    uint8_t i;
    for(i = 0;i <= shifts;i++)
    {
        //lockBufferedMutex(&scheduler_mutex);
        ATOMIC_BLOCK(ATOMIC_FORCEON)
        {
            ENABLE(*display->data_port,(data&mask));
            if(RS)
                ENABLE(*display->selectors_port,display->RS);
            ENABLE(*display->selectors_port,display->E);
            DISABLE(*display->selectors_port,display->E);
            DISABLE(*display->data_port,mask);
        }
        //unlockBufferedMutex(&scheduler_mutex);
        data = data << 4;
    }
    //lockBufferedMutex(&scheduler_mutex);
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        DISABLE(*display->selectors_port,display->RS);
    }
    //unlockBufferedMutex(&scheduler_mutex);
}

void initializeHD44780(HD44780* display)
{
    if(display == 0x0)
        return;

    sendHD44780(display,0x30,0,1);
    _delay_ms(5);
    sendHD44780(display,0x30,0,1);
    _delay_us(110);
    sendHD44780(display,0x30,0,1);
    sendHD44780(display,0x20,0,1);

    sendHD44780(display,FUNCTION_SET | display->bus_mode << 5| TWO_LINES,0,0);
    sendHD44780(display,DISPLAY_CLR,0,0);
}

#endif
