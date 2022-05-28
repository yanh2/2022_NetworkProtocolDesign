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
            debug_if(DBGMSG_L3,"키보드 입력 값 ::: %s\n", originalWord);
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
                // 1. a) SDU in, c1 = false input 타이머가 돌고 있지 않을 때
                if (L3_timer_input_getTimerStatus() == 0) {

                    // 여기 originalWord[1] == null 로 할건지 말건지 ....
                    if (originalWord[0] == 'y' && wordLen == 2) {
                    //sayReq PDU 보내기(헤더 타입 변경), state 이동시킴, sayReq_timer 시작

                    pc.printf("**************\n 이벤트 A) sayRequest 보내기\n ***************\n");

                        strcpy((char*)sdu, (char*)originalWord);
                        L3_msg_encodeReq(sdu);
                        L3_LLI_dataReqFunc(sdu, wordLen);
                            debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                        L3_timer_sayReq_startTimer();

                        wordLen = 0;
                        pc.printf("Give a word to send : ");
                        L3_event_clearEventFlag(L3_event_dataToSend);     
                        pc.printf("NOW YOUR STATE IS  L3STATE_WAIT_SAY ! ! : \n");  
                        main_state = L3STATE_WAIT_SAY;         
                    }
                } else {
                    // 이거 나중에...................
                    // IDLE일 때는 input 타이머 돌면 안된다.
                    wordLen = 0;
                    pc.printf("여기로 오면 안됨. 타이머 확인하기.");
                    L3_timer_input_stopTimer();
                    L3_event_clearEventFlag(L3_event_dataToSend);
                }
/* 
                {                    
                    //msg header setting
                    strcpy((char*)sdu, (char*)originalWord);
                    L3_LLI_dataReqFunc(sdu, wordLen);

                    debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                }
*/
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
            
            if(L3_event_checkEventFlag(L3_event_msgRcvd))
            {
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                pc.printf("현재 스테이트 : WAIT_SAY\n");
                pc.printf("메세지 잘 받아진다. \n\n");
                if(L3_msg_checkIfAcpt(L3_MSG_TYPE_ACPT))
                {
                    L3_timer_input_startTimer();
                    L3_timer_sayReq_stopTimer();
                    pc.printf("메세지 타입 : SAY_ACCEPT\n");
                    pc.printf("발언권을 받았습니다. 원하는 메세지를 적어주세요. \n");
                    pc.printf("SAY_ON 스테이트로 이동합니다. \n");
                    main_state = L3STATE_SAY_ON;
                }
                else if(L3_msg_checkIfRejt(L3_MSG_TYPE_REJT))
                {
                    L3_timer_sayReq_stopTimer();
                    pc.printf("메세지 타입 : REJECT\n");
                    pc.printf("발언권을 얻지 못했습니다. \n");
                    pc.printf("IDLE 스테이트로 이동합니다.\n");
                    main_state = L3STATE_IDLE;
                }
            }


            break;

        case L3STATE_SAY_ON:
             L3service_processInputWord(); //보낼 메세지 입력하기

            

/*          조건 (input timer가 돌고있는지 확인 -> 돌고있으면 보내) 확인하고 

            input_timer 중지 
*/  
            if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
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
            if (L3_timer_input_getTimerStatus() == 1) { //a) SDU in, c1 == true
                strcpy((char*)sdu, (char*)originalWord);
                L3_msg_encodeData(sdu, originalWord, wordLen);
                L3_LLI_dataReqFunc(sdu, wordLen);
                debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                L3_timer_input_stopTimer();
                wordLen = 0;
                pc.printf("Give a word to send : ");
                L3_event_clearEventFlag(L3_event_dataToSend);
                main_state = L3STATE_IDLE;
            }
            
        }
        
        else if(L3_event_checkEventFlag(L3_event_inputTimeout)){
            L3_event_clearEventFlag(L3_event_inputTimeout);
            main_state = L3STATE_IDLE;
        }
        else if(L3_event_checkEventFlag(L3_event_msgRcvd)){ //어떠한 경우에 sayReject PDU 받을 경우
            L3_event_clearEventFlag(L3_event_msgRcvd);
            main_state = L3STATE_IDLE;
        }

            break;

        default :
            break;
    }
}