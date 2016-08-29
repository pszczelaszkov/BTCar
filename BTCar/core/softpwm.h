#ifndef __SOFT_PWM__
#define __SOFT_PWM__
#include "scheduler.h"
#define MAX_PWM_CHANNELS 1

typedef struct
{
    uint16_t pulse_intervals[2];//interval,width
    TimedTask* task;
    volatile uint8_t* port;
    uint8_t pin;
    uint8_t state;//Index of pulse_widths

}SoftPwmChannel;

SoftPwmChannel soft_pwm_channels[MAX_PWM_CHANNELS];
int8_t soft_pwm_channels_last_id = -1;

int8_t channelUpdate(void* ptr)
{
    SoftPwmChannel *channel = ptr;
    TOGGLE(*channel->port,channel->pin);
    channel->state = !channel->state;
    channel->task->ticks_to_go = channel->pulse_intervals[channel->state];
    return 1;
}

SoftPwmChannel* addSoftPWMChannel(uint16_t pulse_interval,uint16_t pulse_width,volatile uint8_t* port,uint8_t pin)
{
    if(soft_pwm_channels_last_id > MAX_PWM_CHANNELS-2)
        return 0x0;

    soft_pwm_channels_last_id++;
    SoftPwmChannel* channel = &soft_pwm_channels[soft_pwm_channels_last_id];
    channel->pulse_intervals[0] = pulse_interval;
    channel->pulse_intervals[1] = pulse_width;
    channel->port = port;
    channel->pin = pin;
    channel->task = SCHEDULER_addScheduledTask(channelUpdate,pulse_interval,channel);
    return channel;
}


#endif
