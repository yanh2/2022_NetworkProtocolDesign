#include "mbed.h"
#include "L3_FSMevent.h"
#include "protocol_parameters.h"


//ARQ retransmission timer
static Timeout input_timer;    // t_num = 0
static Timeout sayReq_timer;   // t_num = 1        
static uint8_t input_timerStatus = 0;
static uint8_t sayReq_timerStatus = 0;


//timer event : ARQ timeout
void L3_timer_timeoutHandler(int t_num) 
{
    if (t_num == 0){
        input_timerStatus = 0;
    } else{
        sayReq_timerStatus = 0;
    }
   
    //L3_event_setEventFlag(L3_event_arqTimeout);
}

//timer related functions ---------------------------
void L3_timer_startTimer(int t_num)
{
    if (t_num == 0){
        uint8_t waitTime = 1;
        input_timer.attach(L3_timer_timeoutHandler(0), waitTime);
        input_timerStatus = 1;
    } else {
        uint8_t waitTime = 1;
        sayReq_timer.attach(L3_timer_timeoutHandler(1), waitTime);
        sayReq_timerStatus = 1;
    }
}

void L3_timer_stopTimer(int t_num)
{
    if (t_num == 0){
        input_timer.detach();
        input_timerStatus = 0;
    } else {
        sayReq_timer.detach();
        sayReq_timerStatus = 0;
    }
}


uint8_t L3_timer_getTimerStatus(int t_num)
{
    if (t_num == 0) {
        return input_timerStatus;
    } else {
        return sayReq_timerStatus;
    }
}
