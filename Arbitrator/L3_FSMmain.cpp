#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"
#include <string.h>


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
            debug_if(DBGMSG_L3,"KEYBOARD INPUT ::: %s\n", originalWord);
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
    //pc.attach(&L3service_processInputWord, Serial::RxIrq);
    pc.printf("Waiting a say request. ::: ");

}

void L3_FSMrun(void)
{   
    if (prev_state != main_state)
    {
        //debug_if(DBGMSG_L3, "[L3] State transition from %i to %i\n", prev_state, main_state);
        prev_state = main_state;
    }

    switch (main_state)
    {
        case L3STATE_IDLE: //IDLE state description
            
            /*
            *  [EVENT] A-1 > 
            *  if) type = sayRequest, 1) input_timer 시작 2) sayAccept pdu send
            *  if) type == data, 무시
            */

            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                if(L3_msg_checkIfReq(dataPtr)){
                    pc.printf("\nSayRequest IN\n");

                    L3_timer_startTimer();
                    originalWord[wordLen++] = 's';
                    strcpy((char*)sdu, (char*)originalWord);
                    L3_msg_encodeAcpt(sdu); //발언권 승인 메세지 보냄

                    /*
                        
                    여기 wordLen 추가함
                    */
                    
                    L3_LLI_dataReqFunc(sdu, wordLen);



                    pc.printf("\nSent SayAccept\n");
                    L3_event_clearEventFlag(L3_event_msgRcvd);
                    pc.printf("\n*********************************************************\n");
                    pc.printf("                      [STATE] SAYING");
                    pc.printf("\n*********************************************************\n");  
                    main_state = L3STATE_SAYING;
                } 
                wordLen = 0;
            }
            break;

        case L3STATE_SAYING:
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t* getWordData = L3_msg_getWord(dataPtr);
                uint8_t size = L3_LLI_getSize();

                /*
                 *  [EVENT] A-2 > 발언권 가진 Entity에게 메세지가 온 경우
                 *  1) Input timer 중지 2) 메세지를 다른 Entity들에게 전송 3) 발언권 회수(IDLE로 이동)
                 */

                if (L3_msg_checkIfData(dataPtr)){
                    L3_timer_stopTimer();

                    wordLen = size;
                    // null 문자 추가 삽입 for 문자 잘림 방짐
                    getWordData[wordLen] = '\0';
                    strcpy((char*)sdu, (char*)getWordData);
                    L3_msg_encodeData(sdu, getWordData, wordLen);
                    pc.printf("====================================================\n");
                    debug("RCVD MSG : %s (length:%i)\n", getWordData, size);
                    pc.printf("====================================================\n");

                    L3_LLI_dataReqFunc(sdu, wordLen);
                   
                    pc.printf("\nCompleted sending messages to entities\n");

                    wordLen = 0;
                    L3_event_clearEventFlag(L3_event_msgRcvd);
                    
                    pc.printf("\n*********************************************************\n");
                    pc.printf("                      [STATE] IDLE");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("Waiting a say request. ::: \n"); 
                    
                    main_state = L3STATE_IDLE;
                } 
                
                /*
                 *  [EVENT] A-1 > 이미 한 Entity가 발언권을 가진 상태에서 다른 Entity가 발언권 요청한 상황
                 *  발언권 요청한 Entity에게 sayRejt를 전송
                 */
                else if(L3_msg_checkIfReq(dataPtr)){
                    wordLen = size;
                    strcpy((char*)sdu, (char*)getWordData); 
                    L3_msg_encodeRejt(sdu); 
                    
                    L3_LLI_dataReqFunc(sdu, wordLen);
                    wordLen = 0;
                    L3_event_clearEventFlag(L3_event_msgRcvd);
                    pc.printf("\n**************\n other entity wants a say. send REJECT \n ***************\n");
                    pc.printf("\n**************\n Sent a sayRejt \n ***************\n");
                }
            }
                /*
                 *  [EVENT] B > 발언권 가진 Entity가 정해진 시간 내에 메세지를 보내지 않음
                 */
            else if(L3_event_checkEventFlag(L3_event_Timeout)){
                L3_event_clearEventFlag(L3_event_Timeout);
                pc.printf("\n--------------------\n [TIMEOUT] The entity did not send a message \n--------------------\n");
                pc.printf("\n*********************************************************\n");
                pc.printf("                      [STATE] IDLE");
                pc.printf("\n*********************************************************\n");  
                pc.printf("Waiting a say request. \n");
                main_state = L3STATE_IDLE;
            }
            break;
        default :
            break;
    }
}