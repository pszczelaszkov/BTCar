#ifndef __SCHEDULER__
#define __SCHEDULER__

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define TIMED_QUEUE_SIZE 5+1//50 ticks per 1 empty task
#define LOW_PRIORITY_QUEUE_SIZE 5
///


/*typedef struct
{
    void (*awaiting_call)();
    uint8_t lock;
    uint8_t awaiting_calls;
} BufferedMutex;//32 Bits-4bytes

inline void lockBufferedMutex(BufferedMutex* mutex){mutex->lock = 1;}
void unlockBufferedMutex(BufferedMutex* mutex)
{
    mutex->lock = 0;
    void (*awaiting_call)() = mutex->awaiting_call;
    uint8_t awaiting_calls = mutex->awaiting_calls;
    mutex->awaiting_calls = 0;

    while(awaiting_calls)
    {
        awaiting_call();
        awaiting_calls--;
    }
}

uint8_t checkBufferedMutex(BufferedMutex* mutex)
{
    if(mutex->lock)
    {
        mutex->awaiting_calls++;
        return 0;
    }
    return 1;
}*/
///
typedef int8_t (*Fptr)(void*);
typedef struct
{

    int8_t (*fptr)(void*);
    uint16_t timestamp;//target timestamp
    uint16_t ticks_to_go;//ticks used to set timestamp
    ///pointers are too big 16bit vs 8bit
    uint8_t next_task_id;
    uint8_t previous_task_id;
    ///
    //int8_t repeat;
    void* argument;

} TimedTask; //84bits-11bytes

typedef struct
{
    int8_t (*fptr)(void*);//function pointer
    void* argument;
    int8_t execute;

} LowPriorityTask;//40 Bits-5bytes

//uint8_t system_clock[8];//system clock in ms
uint16_t SCHEDULER_timestamp;
TimedTask SCHEDULER_tasks[TIMED_QUEUE_SIZE];//ID:0 is "dummy" entry point
int8_t SCHEDULER_last_scheduled_task_id;
uint8_t SCHEDULER_unused_timed_ids[TIMED_QUEUE_SIZE];
uint8_t SCHEDULER_unused_timed_ids_last;
//BufferedMutex SCHEDULER_tasks_mutex;
///
int8_t SCHEDULER_pending_low_priority_task;
LowPriorityTask SCHEDULER_low_priority_tasks[LOW_PRIORITY_QUEUE_SIZE];
uint8_t SCHEDULER_unused_lowpriority_ids[LOW_PRIORITY_QUEUE_SIZE];
int8_t SCHEDULER_unused_lowpriority_ids_last = -1;

///
uint8_t time;

TimedTask* SCHEDULER_addScheduledTask(Fptr fptr,uint16_t ticks_to_go,void* argument)
{
    if(SCHEDULER_unused_timed_ids_last == 0)
        return 0;

    TimedTask* task;
    //lockBufferedMutex(&SCHEDULER_tasks_mutex);
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        uint8_t task_id = SCHEDULER_unused_timed_ids[SCHEDULER_unused_timed_ids_last];
        SCHEDULER_unused_timed_ids_last--;

        task = &SCHEDULER_tasks[task_id];
        task->fptr = fptr;
        task->timestamp = SCHEDULER_timestamp + ticks_to_go;
        task->ticks_to_go = ticks_to_go;
        task->argument = argument;
        task->previous_task_id = SCHEDULER_last_scheduled_task_id;
        task->next_task_id = 0;
//        task->repeat = repeat;

        SCHEDULER_tasks[SCHEDULER_last_scheduled_task_id].next_task_id = task_id;

        SCHEDULER_last_scheduled_task_id = task_id;
    }
    //unlockBufferedMutex(&SCHEDULER_tasks_mutex);
    return task;
}

LowPriorityTask* SCHEDULER_addLowPriorityTask(Fptr fptr,void* argument)
{
    if(SCHEDULER_unused_lowpriority_ids_last < 0)
        return 0;

    //lockBufferedMutex(&SCHEDULER_tasks_mutex);
    int8_t unused_slot;
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        unused_slot = SCHEDULER_unused_lowpriority_ids[SCHEDULER_unused_lowpriority_ids_last];
        SCHEDULER_unused_lowpriority_ids_last--;
    }
    //unlockBufferedMutex(&SCHEDULER_tasks_mutex);

    LowPriorityTask* task = &SCHEDULER_low_priority_tasks[unused_slot];
    task->fptr = fptr;
    task->argument = argument;
    task->execute = 1;

    return task;
}

void SCHEDULER_checkScheduledTasks()
{
    //lockBufferedMutex(&SCHEDULER_tasks_mutex);

    SCHEDULER_timestamp++;
    int8_t task_id = SCHEDULER_tasks[0].next_task_id;
    while(task_id != 0)
    {
        TimedTask* task = &SCHEDULER_tasks[task_id];
        if(SCHEDULER_timestamp == task->timestamp)
        {
            //task->fptr(task->argument);
            if(task->fptr(task->argument))//Repeat if 1
            {
                task->timestamp = SCHEDULER_timestamp + task->ticks_to_go;
            }
            else
            {
                (&SCHEDULER_tasks[task->previous_task_id])->next_task_id = task->next_task_id;
                (&SCHEDULER_tasks[task->next_task_id])->previous_task_id = task->previous_task_id;

                SCHEDULER_unused_timed_ids_last++;
                SCHEDULER_unused_timed_ids[SCHEDULER_unused_timed_ids_last] = task_id;
            }
        }
        task_id = task->next_task_id;
    }
    //unlockBufferedMutex(&SCHEDULER_tasks_mutex);
    uint8_t temp = TCNT0;
    //if(temp > time)
        time = temp;
}

void SCHEDULER_checkLowPriorityTasks()
{
    /*SAFE*/
    if(!SCHEDULER_pending_low_priority_task)
    {
        SCHEDULER_pending_low_priority_task = 1;
        int8_t id;
        for(id = 0;id < LOW_PRIORITY_QUEUE_SIZE;id++)
        {
            LowPriorityTask* task = &SCHEDULER_low_priority_tasks[id];
            if(!task->execute)
                continue;
            if(task->fptr(task->argument))//Repeat if 1
                continue;

            task->execute = 0;

           // lockBufferedMutex(&SCHEDULER_tasks_mutex);
            ATOMIC_BLOCK(ATOMIC_FORCEON)
            {
                SCHEDULER_unused_lowpriority_ids_last++;
                SCHEDULER_unused_lowpriority_ids[SCHEDULER_unused_lowpriority_ids_last] = id;
            }
            //unlockBufferedMutex(&SCHEDULER_tasks_mutex);
        }
        SCHEDULER_pending_low_priority_task = 0;
    }
}

void SCHEDULER_init()
{
    int8_t i;
    for(i = 0;i < TIMED_QUEUE_SIZE;i++)
    {
        SCHEDULER_unused_timed_ids[i] = i;
    }
    SCHEDULER_unused_timed_ids_last = i-1;

    for(i = 0;i < LOW_PRIORITY_QUEUE_SIZE;i++)
    {
        SCHEDULER_unused_lowpriority_ids[i] = i;
    }
    SCHEDULER_unused_lowpriority_ids_last = i-1;

//    SCHEDULER_tasks_mutex.awaiting_call = SCHEDULER_checkScheduledTasks;
    OCR0 = 99;//refresh per 100us
    TIMSK = (1<<OCIE0);//enable compare match interrupt(timer0)
    TCCR0 = (1<<CS01) | (1<<WGM01);//start timer (/8 prescaler)/CTC MODE
    sei();//enable interrupts
}

ISR(TIMER0_COMP_vect,/*ISR_NOBLOCK*/)
{
   // if(checkBufferedMutex(&SCHEDULER_tasks_mutex))
        SCHEDULER_checkScheduledTasks();
    //else
      //  return;
    sei();//TEMP!
    SCHEDULER_checkLowPriorityTasks();
}

#endif
