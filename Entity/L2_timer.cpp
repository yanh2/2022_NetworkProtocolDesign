#include "mbed.h"
#include "L2_FSMevent.h"
#include "protocol_parameters.h"



//ARQ retransmission timer
static Timeout input_timer;    // t_num = 0
static Timeout sayReq_timer;   // t_num = 1        
static uint8_t input_timerStatus = 0;
static uint8_t sayReq_timerStatus = 0;


//timer event : ARQ timeout
void L2_timer_timeoutHandler(void) 
{
    timerStatus = 0;
    L2_event_setEventFlag(L2_event_arqTimeout);
}

//timer related functions ---------------------------
void L2_timer_startTimer(int t_num)
{
    if (t_num == 0){
        uint8_t waitTime = L2_ARQ_MINWAITTIME + rand()%(L2_ARQ_MAXWAITTIME-L2_ARQ_MINWAITTIME);
        input_timer.attach(L2_timer_timeoutHandler, waitTime);
        input_timerStatus = 1;
    } else {
        uint8_t waitTime = L2_ARQ_MINWAITTIME + rand()%(L2_ARQ_MAXWAITTIME-L2_ARQ_MINWAITTIME);
        sayReq_timer.attach(L2_timer_timeoutHandler, waitTime);
        sayReq_timerStatus = 1;
    }

}

void L2_timer_stopTimer(int t_num)
{
    if (t_num == 0){
        input_timer.detach();
        input_timerStatus = 0;
    } else {
        sayReq_timer.detach();
        sayReq_timerStatus = 0;
    }
}

uint8_t L2_timer_getTimerStatus(int t_num)
{
    if (t_num == 0) {
        return input_timerStatus;
    } else {
        return sayReq_timerStatus;
    }
}
