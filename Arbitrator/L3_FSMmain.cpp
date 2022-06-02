#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"


//FSM state -------------------------------------------------
#define L3STATE_IDLE                0
#define L3STATE_SAYING              1

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
    if (!L3_event_checkEventFlag(L3_event_dataToSend))
    {
        if (c == '\n' || c == '\r')
        {
            originalWord[wordLen++] = '\0';
            L3_event_setEventFlag(L3_event_dataToSend);
            debug_if(DBGMSG_L3,"키보드 입력 값: ::: %s\n", originalWord);
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

}

void L3_FSMrun(void)
{   
    if (prev_state != main_state)
    {
        //debug_if(DBGMSG_L3, "[L3] State transition from %i to %i\n", prev_state, main_state);
        prev_state = main_state;
    }

    //FSM should be implemented here! ---->>>>
    switch (main_state)
    {
        case L3STATE_IDLE: //IDLE state description
            
            // 메세지를 받는 경우 
            // 1. sayReq : input_timer 시작 & sayAccept pdu send
            // 2. data : 무시 
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                pc.printf((char*)L3_msg_getType);
                if(L3_msg_checkIfReq(dataPtr)){
                    pc.printf("*************************\n 발언권 요청 받음여 \n ************************");
                    L3_timer_startTimer();
                    originalWord[wordLen++] = 's'; //테스트??
                    strcpy((char*)sdu, (char*)originalWord);
                    L3_msg_encodeAcpt(sdu); //발언권 승인 메세지 보냄
                    L3_LLI_dataReqFunc(sdu, wordLen);
                    
                    L3_event_clearEventFlag(L3_event_msgRcvd);
                    main_state = L3STATE_SAYING;
                }  
            }

            break;

        case L3STATE_SAYING:
            //누군가 발언권을 가지고 있는 상태
            // Entity의 키보드 입력을 기다린다. 
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                if (L3_msg_checkIfData(dataPtr)){
                    wordLen = size;
                    strcpy((char*)sdu, (char*)dataPtr);
                    L3_msg_encodeData(sdu, dataPtr, wordLen); //이거 넣는건지 아닌지 확인하기
                    L3_LLI_dataReqFunc(sdu, wordLen); //여기가 전송

                    L3_timer_stopTimer();

                    wordLen = 0;

                    L3_event_clearEventFlag(L3_event_msgRcvd);
                    pc.printf("다보냄 ! ! : \n");  
                    // 발언권 회수해야함
                    main_state = L3STATE_IDLE;
                } else if(L3_msg_checkIfReq(dataPtr)){
                    //거절
                    strcpy((char*)sdu, (char*)dataPtr);
                    L3_msg_encodeRejt(sdu); 
                    L3_LLI_dataReqFunc(sdu, wordLen);
                }
                
            }


            break;
        default :
            break;
    }
}