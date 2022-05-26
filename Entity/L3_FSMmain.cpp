#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"


//FSM state -------------------------------------------------
#define L3STATE_IDLE                0
#define L3STATE_WAIT_SAY            1
#define L3STATE_SAY_ON              2

//state variables
static uint8_t main_state = L3STATE_IDLE; //protocol state
static uint8_t prev_state = main_state;

//SDU (input)
static uint8_t originalWord[200];
static uint8_t wordLen=0;

static uint8_t sdu[200];

//serial port interface
static Serial pc(USBTX, USBRX);


//application event handler : generating SDU from keyboard input
static void L3service_processInputWord(void)
{
    char c = pc.getc();
    // state가 say on 가 아니야. 
        // char c = 'y' -> 발언권 보내
    /*if (L3_timer_getTimerStatus(0) == 0 && c == 'y') {
        L3_event_setEventFlag(L3_event_sayReqToSend);
    }*/
    
    if (!L3_event_checkEventFlag(L3_event_dataToSend))
    {
        if (c == '\n' || c == '\r')
        {
            originalWord[wordLen++] = '\0';
            L3_event_setEventFlag(L3_event_dataToSend);
            debug_if(DBGMSG_L3,"word is ready! ::: %s\n", originalWord);
        }
        else
        {
            originalWord[wordLen++] = c;
            if (wordLen >= L3_MAXDATASIZE-1)
            {
                originalWord[wordLen++] = '\0';
                L3_event_setEventFlag(L3_event_dataToSend);
                pc.printf("\n max reached! word forced to be ready :::: %s\n", originalWord);
            }
        }
    }
}


void L3_initFSM()
{
    //initialize service layer
    pc.attach(&L3service_processInputWord, Serial::RxIrq);

    //pc.printf("Give a word to send : ");
    pc.printf("발언권을 얻으려면 'y'를 눌러주세요. : ");
}

void L3_FSMrun(void)
{   
    if (prev_state != main_state)
    {
        debug_if(DBGMSG_L3, "[L3] State transition from %i to %i\n", prev_state, main_state);
        prev_state = main_state;
    }

    //FSM should be implemented here! ---->>>>
    switch (main_state)
    {
        case L3STATE_IDLE: //IDLE state description
            
            // 메세지 받는 경우
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                debug("\n -------------------------------------------------\nRCVD MSG : %s (length:%i)\n -------------------------------------------------\n", 
                            dataPtr, size);
                
                pc.printf("Give a word to send : ");
                
                L3_event_clearEventFlag(L3_event_msgRcvd);
            }

            // 메세지 보내는 경우
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
            {
#ifdef ENABLE_CHANGEIDCMD
                if (strncmp((const char*)originalWord, "changeID: ",9) == 0)
                {
                    uint8_t myid = originalWord[9] - '0';
                    debug("[L3] requesting to change to srce id %i\n", myid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_SRCID, myid);
                }
                else
#endif
                // 1. a) SDU in, c1 = false
                if (L3_timer_sayReq_getTimerStatus() == 0) {
                    if (originalWord[0] == 'y' && originalWord[1] == NULL) {
                    //sayReq PDU 보내기(헤더 타입 변경), state 이동시킴, sayReq_timer 시작
                    
                        strcpy((char*)sdu, (char*)originalWord);
                        
                        L3_msg_encodeReq(sdu);
                            pc.printf(" now msg encoding");
                        L3_LLI_dataReqFunc(sdu, wordLen);
                            pc.printf("now exec msg date req func ");
                        debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                        main_state = L3STATE_WAIT_SAY;
                            pc.printf("NOW YOUR STATE IS  WAIT SAY ! ! : ");
                    }
                } 

/* 
                {                    
                    //msg header setting
                    strcpy((char*)sdu, (char*)originalWord);
                    L3_LLI_dataReqFunc(sdu, wordLen);

                    debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                }
*/
                wordLen = 0;

                pc.printf("Give a word to send : ");

                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            break;

        case L3STATE_WAIT_SAY:
            
            // SDU 들어옴
                // if sayAccept  
                    // input_timer 시작하면서 sayReq_timer 중지
                    // sayON State로 움직여요 

                // else if sayReject 
                    // IDLE State로 움직여요 
                    // sayReq_timer 중지해요

            break;

        case L3STATE_SAY_ON:
            // L3service_processInputWord 실행해요.
/*          조건 (input timer가 돌고있는지 확인 -> 돌고있으면 보내) 확인하고 

            
            それが条件だから...
            
                {                    
                    //msg header setting
                    strcpy((char*)sdu, (char*)originalWord);
                    L3_LLI_dataReqFunc(sdu, wordLen);

                    debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                }

            input_timer 중지 
*/
            break;

        default :
            break;
    }
}