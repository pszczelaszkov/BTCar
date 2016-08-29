#ifndef __SOFTRTC__
#define __SOFTRTC__
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "USART.h"

#define YEAR 15
#define MONTH 11
#define DAY 8
#define SECOND 6
#define MINUTE 3
#define HOUR 0

#define RTC_SELECT_MODE 0
#define RTC_MODE_GET 1
#define RTC_MODE_GET_TIME 2
#define RTC_MODE_GET_DATE 3
#define RTC_MODE_SET 4
#define RTC_MODE_SET_SECOND 5
#define RTC_MODE_SET_MINUTE 6
#define RTC_MODE_SET_HOUR 7
#define RTC_MODE_SET_DAY 8
#define RTC_MODE_SET_MONTH 9
#define RTC_MODE_SET_YEAR 10

typedef struct
{
    uint8_t second:6;
    uint8_t minute:6;
    uint8_t hour:5;
    uint8_t day:5;
    uint8_t month:4;
    uint8_t year:6;//32 bits
}Date;

Date rtc_date;
char rtc_formated_date[18];//hh:mm:ssdd/mm/yyyy
uint8_t rtc_usart_command;

void rtcFormatValue(uint8_t value,uint8_t position)
{
    char buffer[] = "000";
    uint8_t left_margin = 0;
    if(value < 10)
        left_margin++;
    if(position == YEAR && value < 100)
        left_margin++;

    itoa(value,buffer + left_margin,10);
    if(position != YEAR)
        strncpy(rtc_formated_date+position,buffer,2);
    else
        strncpy(rtc_formated_date+position,buffer,3);
}

int8_t rtcUpdate()
{
    if(rtc_date.second == 59)
    {
        rtc_date.second = 0;
        if(rtc_date.minute == 59)
        {
            rtc_date.minute = 0;
            if(rtc_date.hour == 23)
            {
                rtc_date.hour = 0;
                uint8_t day_in_month = 30;
                if(rtc_date.month < 8)
                {
                    if(rtc_date.month & 1)//odd
                        day_in_month = 31;
                }
                else
                {
                    if(!(rtc_date.month & 1))//even
                        day_in_month = 31;
                }
                if(rtc_date.month == 2)//February
                {
                    if((rtc_date.year & 3))//dividable by 4 (leap year)
                        day_in_month = 28;
                    else
                        day_in_month = 29;
                }
                if(rtc_date.day == day_in_month)
                {
                    rtc_date.day = 1;
                    if(rtc_date.month == 12)//32 nie miesci sie
                    {
                        rtc_date.month = 1;
                        rtc_date.year++;
                        rtcFormatValue(rtc_date.year,YEAR);
                    }
                    else
                        rtc_date.month++;

                    rtcFormatValue(rtc_date.month,MONTH);
                }
                else
                    rtc_date.day++;

                rtcFormatValue(rtc_date.day,DAY);
            }
            else
                rtc_date.hour++;

            rtcFormatValue(rtc_date.hour,HOUR);
        }
        else
            rtc_date.minute++;

        rtcFormatValue(rtc_date.minute,MINUTE);
    }
    else
        rtc_date.second++;

    rtcFormatValue(rtc_date.second,SECOND);
    return 0;
}

int8_t rtcKickstarter()
{
    SCHEDULER_addLowPriorityTask(rtcUpdate,0x0);
    return 1;
}

void dateOnDisplay(char* display_buffer)
{
    strncpy(display_buffer,rtc_formated_date+DAY,10);
}

void timeOnDisplay(char* display_buffer)
{
    strncpy(display_buffer,rtc_formated_date,8);
}

void rtcOnUSART()
{
    USART_outcome_buffer[0] = '\r';
    USART_outcome_buffer[1] = '\n';
    USART_outcome_buffer[2] = EOT;
    switch(rtc_usart_command)
    {
        case RTC_SELECT_MODE:
            if(strncmp(USART_income_buffer,"get",3) == 0)
            {
                rtc_usart_command = RTC_MODE_GET;
                USART_outcome_buffer_index = 0;
            }
            else if(strncmp(USART_income_buffer,"set",3) == 0)
                rtc_usart_command = RTC_MODE_SET;
            else
                USART_outcome_buffer_index = 0;
        break;
        case RTC_MODE_GET:
            if(strncmp(USART_income_buffer,"time",4) == 0)
                rtc_usart_command = RTC_MODE_GET_TIME;
            else if(strncmp(USART_income_buffer,"date",4) == 0)
                rtc_usart_command = RTC_MODE_GET_DATE;

        break;
        case RTC_MODE_GET_TIME:
            USART_outcome_buffer_index = 0;
            strncpy(USART_outcome_buffer+2,rtc_formated_date,8);
            USART_outcome_buffer[10] = '\r';
            USART_outcome_buffer[11] = '\n';
            USART_outcome_buffer[12] = EOT;
            rtc_usart_command = RTC_MODE_GET;
        break;
        case RTC_MODE_GET_DATE:
            USART_outcome_buffer_index = 0;
            strncpy(USART_outcome_buffer+2,rtc_formated_date+8,10);
            USART_outcome_buffer[13] = '\r';
            USART_outcome_buffer[14] = '\n';
            USART_outcome_buffer[15] = EOT;
            rtc_usart_command = RTC_MODE_GET;
        break;
        case RTC_MODE_SET:
            strcpy(USART_outcome_buffer+2,"second?");
            USART_outcome_buffer[10] = '\r';
            USART_outcome_buffer[11] = '\n';
            USART_outcome_buffer[12] = EOT;

            USART_outcome_buffer_index = 0;
            rtc_usart_command++;
        break;
        case RTC_MODE_SET_SECOND:
            strcpy(USART_outcome_buffer+2,"minute?");
            USART_outcome_buffer[10] = '\r';
            USART_outcome_buffer[11] = '\n';
            USART_outcome_buffer[12] = EOT;

            USART_outcome_buffer_index = 0;
            rtc_date.second = atoi(USART_income_buffer);
            rtc_usart_command++;
        break;
        case RTC_MODE_SET_MINUTE:
            strcpy(USART_outcome_buffer+2,"hour?");
            USART_outcome_buffer[8] = '\r';
            USART_outcome_buffer[9] = '\n';
            USART_outcome_buffer[10] = EOT;

            USART_outcome_buffer_index = 0;
            rtc_date.minute = atoi(USART_income_buffer);
            rtcFormatValue(rtc_date.minute,MINUTE);
            rtc_usart_command++;
        break;
        case RTC_MODE_SET_HOUR:
            strcpy(USART_outcome_buffer+2,"day?");
            USART_outcome_buffer[7] = '\r';
            USART_outcome_buffer[8] = '\n';
            USART_outcome_buffer[9] = EOT;

            USART_outcome_buffer_index = 0;
            rtc_date.hour = atoi(USART_income_buffer);
            rtcFormatValue(rtc_date.hour,HOUR);
            rtc_usart_command++;
        break;
        case RTC_MODE_SET_DAY:
            strcpy(USART_outcome_buffer+2,"month?");
            USART_outcome_buffer[9] = '\r';
            USART_outcome_buffer[10] = '\n';
            USART_outcome_buffer[11] = EOT;

            USART_outcome_buffer_index = 0;
            rtc_date.day = atoi(USART_income_buffer);
            rtcFormatValue(rtc_date.day,DAY);
            rtc_usart_command++;
        break;
        case RTC_MODE_SET_MONTH:
            strcpy(USART_outcome_buffer+2,"year?");
            USART_outcome_buffer[7] = '\r';
            USART_outcome_buffer[8] = '\n';
            USART_outcome_buffer[9] = EOT;

            USART_outcome_buffer_index = 0;
            rtc_date.month = atoi(USART_income_buffer);
            rtcFormatValue(rtc_date.month,MONTH);
            rtc_usart_command++;
        break;
        case RTC_MODE_SET_YEAR:
            strcpy(USART_outcome_buffer+2,"Done.");
            USART_outcome_buffer[7] = '\r';
            USART_outcome_buffer[8] = '\n';
            USART_outcome_buffer[9] = EOT;

            USART_outcome_buffer_index = 0;
            rtc_date.year = atoi(USART_income_buffer+1);
            rtcFormatValue(rtc_date.year,YEAR);
            rtc_usart_command = RTC_SELECT_MODE;
            USART_parsing_function = USARTonUSART;
        break;
    }
}

void initializeSoftRTC()
{
    strcpy(rtc_formated_date,"00:00:0001/01/2000");
    rtc_date.month = 1;
    rtc_date.day = 1;
    SCHEDULER_addScheduledTask(rtcKickstarter,10000,0x0);
}
#endif
