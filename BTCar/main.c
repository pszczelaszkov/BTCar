#define F_CPU 8000000UL
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "core/bitwise.h"
#include "core/softpwm.h"
#include "core/USART.h"
#include "modules/statusLed.h"
#include "modules/servo.h"

int main(void)
{

    ENABLE(DDRA,0xff);
    ENABLE(DDRB,0xff);
    ENABLE(DDRC,0xff);
    ENABLE(DDRD,0xff);
    ENABLE(DDRE,0xff);

    ENABLE(PORTA,0xff);

    SCHEDULER_init();
    SERVO_init();
    initializeStatusLed();
    initializeUSART();

    while(1)
    {
        sleep_cpu();
    }
    return 0;
}
