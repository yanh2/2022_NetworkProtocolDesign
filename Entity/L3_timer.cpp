#include "mbed.h"
#include "L3_FSMevent.h"
#include "protocol_parameters.h"


//ARQ retransmission timer
static Timeout input_timer;    // t_num = 0
static Timeout sayReq_timer;   // t_num = 1        
static uint8_t input_timerStatus = 0;
static uint8_t sayReq_timerStatus = 0;

//timer event : ARQ timeout
void L3_timer_input_timeoutHandler() 
{
    input_timerStatus = 0;
    L3_event_setEventFlag(L3_event_inputTimeout);
}

void L3_timer_sayReq_timeoutHandler() 
{

    sayReq_timerStatus = 0;
    L3_event_setEventFlag(L3_event_sayReqTimeout);
}

//timer related functions ---------------------------
void L3_timer_input_startTimer()
{
    uint8_t waitTime = 30; //초
    input_timer.attach(L3_timer_input_timeoutHandler, waitTime);
    input_timerStatus = 1;
}

void L3_timer_sayReq_startTimer()
{
    uint8_t waitTime = 5;
    sayReq_timer.attach(L3_timer_sayReq_timeoutHandler, waitTime);
    sayReq_timerStatus = 1;
}


void L3_timer_input_stopTimer()
{
    input_timer.detach();
    input_timerStatus = 0;
}

void L3_timer_sayReq_stopTimer()
{
    sayReq_timer.detach();
    sayReq_timerStatus = 0;
}

uint8_t L3_timer_input_getTimerStatus()
{
    return input_timerStatus;
}

uint8_t L3_timer_sayReq_getTimerStatus()
{
    return sayReq_timerStatus;
}

