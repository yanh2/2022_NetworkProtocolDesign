#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"
#include <string.h>


//FSM state -------------------------------------------------
#define L3STATE_IDLE                0
#define L3STATE_WAIT_SAY            1
#define L3STATE_SAY_ON              2
#define ENABLE_CHANGEIDCMD          3

//state variables
static uint8_t main_state = L3STATE_IDLE; //protocol state
static uint8_t prev_state = main_state;
static bool was_say_on_state = false;


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

    if(main_state != L3STATE_WAIT_SAY){
        if (!L3_event_checkEventFlag(L3_event_dataToSend)) {
            if (c == '\n' || c == '\r') {
                originalWord[wordLen++] = '\0';
                L3_event_setEventFlag(L3_event_dataToSend);
                debug_if(DBGMSG_L3,"KEYBOARD INPUT::: %s\n", originalWord);
            }
            else {
                originalWord[wordLen++] = c;
                if (wordLen >= L3_MAXDATASIZE-1) {
                    originalWord[wordLen++] = '\0';
                    L3_event_setEventFlag(L3_event_dataToSend);
                    pc.printf("\n max reached! word forced to be ready :::: %s\n", originalWord);
                }
            }
        }
    }
}

void L3_initFSM()
{
    //initialize service layer
    pc.attach(&L3service_processInputWord, Serial::RxIrq);

    //pc.printf("Give a word to send : ");
    pc.printf("Please, Enter 'y' for Request a say. ::: \n");
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
        
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {   
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t* getWordData = L3_msg_getWord(dataPtr);
                uint8_t size = L3_LLI_getSize();
                
                /*
                 *  [EVENT] B-3 > 메세지 화면 출력 
                 */
               
                if(L3_msg_checkIfData(dataPtr)){       
                    if (was_say_on_state == true)
                    {    
                        was_say_on_state = false;
                    } else {
                        /* 방법 1.  */
                        pc.printf("====================================================\n");
                        debug("RCVD MSG : %s (length:%i)\n", getWordData, size);
                        pc.printf("====================================================\n");
                        
                        /* 방법 2.  */
                        // pc.printf("====================================================\n");
                        // debug("\n 2 &dataPtr[] ) RCVD MSG : %s (length:%i)\n",  &dataPtr[L3_MSG_OFFSET_DATA], size);
                        // pc.printf("====================================================\n");

                        //pc.printf("Give a word to send : ");
                        wordLen = 0;
                    }

                }
                do {
                    L3_event_clearEventFlag(L3_event_msgRcvd);
                } while(L3_event_checkEventFlag(L3_event_msgRcvd));
            }
            // 메세지 보내는 경우
            else if (L3_event_checkEventFlag(L3_event_dataToSend))
            {
                /*
                 *  [EVENT] A > 'y' 입력 -> 발언권 요청
                 *  c1 = false (timer 정상 동작 확인)
                 */

                if (L3_timer_input_getTimerStatus() == 0) {
                    if (originalWord[0] == 'y' && wordLen == 2) {
                    //sayReq PDU 보내기(헤더 타입 변경), state 이동시킴, sayReq_timer 시작
                    
                        strcpy((char*)sdu, (char*)originalWord);
                        L3_msg_encodeReq(sdu);
                    
                        L3_LLI_dataReqFunc(sdu, wordLen);
                            //debug_if(DBGMSG_L3, "[L3] sending msg....\n");

                        L3_timer_sayReq_startTimer();
                        wordLen = 0;
                        
                        L3_event_clearEventFlag(L3_event_dataToSend);
                        
                        pc.printf("\n*********************************************************\n");
                        pc.printf("                      [STATE] WAIT_SAY");
                        pc.printf("\n*********************************************************\n");
                        main_state = L3STATE_WAIT_SAY;

                    } else {
                        wordLen = 0;
                        pc.printf("You should request a say first. Enter 'y' ::: \n");
                        L3_event_clearEventFlag(L3_event_dataToSend);  
                        
                    }
                } else {
                    // IDLE일 때는 input 타이머 돌면 안된다.
                    wordLen = 0;
                    L3_timer_input_stopTimer();
                    L3_event_clearEventFlag(L3_event_dataToSend);
                    pc.printf("\n--------------------\n [ERROR] timer error occurred. \n--------------------\n");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("                       [STATE] IDLE");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("Please, Enter 'y' for Request a say. ::: \n");
                    main_state = L3STATE_IDLE;
                }
            }
            break;

        case L3STATE_WAIT_SAY:
            if(L3_event_checkEventFlag(L3_event_msgRcvd)) // SAY_ACCEPT or SAY_REJECT 대기
            {
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t* getWordData = L3_msg_getWord(dataPtr);
                uint8_t size = L3_LLI_getSize();

                /*
                 *  [EVENT] B-1 > sayAccept 받았을 때
                 *  1) input_timer 시작   2) sayReq_timer 중지 3) sayON State로 이동
                 */

                if(L3_msg_checkIfAcpt(dataPtr))
                {
                    L3_timer_input_startTimer();
                    L3_timer_sayReq_stopTimer();
                    pc.printf("\n----------------------------------------------------------\n");
                    pc.printf("[ACCEPT] You have a say. Please enter the message you want.");
                    pc.printf("\n----------------------------------------------------------\n");

                    L3_event_clearEventFlag(L3_event_msgRcvd);

#ifdef ENABLE_CHANGEIDCMD
                {
                    uint8_t myid = '3' - '0'; //숫자로 바꿈
                    //debug("[L3] requesting to change to src id %i\n", myid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_SRCID, myid);
                }
#endif
                    pc.printf("\n*********************************************************\n");
                    pc.printf("                      [STATE] SAY_ON");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("You got a say. \n");
                    pc.printf("Give a word to send ::: \n");  
                    main_state = L3STATE_SAY_ON;
                }

                /*
                 *  [EVENT] B-2 > sayReject 받았을 때
                 *  1) sayReq_timer 중지  2) IDLE State로 이동
                 */

                else if(L3_msg_checkIfRejt(dataPtr))
                {
                    L3_timer_sayReq_stopTimer();
                    pc.printf("\n--------------------\n [REJECT] Your SAY is rejected. \n--------------------\n");
                    L3_event_clearEventFlag(L3_event_msgRcvd);
                    pc.printf("\n*********************************************************\n");
                    pc.printf("                       [STATE] IDLE");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("Please, Enter 'y' for Request a say. ::: \n");
                    main_state = L3STATE_IDLE;
                }

                /*
                 *  [EVENT] B-3 > 메세지 화면 출력 
                 */
               
                else if(L3_msg_checkIfData(dataPtr)){           
                    /* 방법 1.  */
                    pc.printf("====================================================\n");
                    debug("RCVD MSG : %s (length:%i)\n", getWordData, size);
                    pc.printf("====================================================\n");
                    
                    /* 방법 2.  */
                    // pc.printf("====================================================\n");
                    // debug("\n 2 &dataPtr[] ) RCVD MSG : %s (length:%i)\n",  &dataPtr[L3_MSG_OFFSET_DATA], size);
                    // pc.printf("====================================================\n");
                }

                L3_event_clearEventFlag(L3_event_msgRcvd);
            }

            /*
             *  [EVENT] C-1 > sayReq_timer 타임아웃 : 발언권 승인이 정해진 시간 내에 도착하지 않았을 경우 
             */
            else if(L3_event_checkEventFlag(L3_event_sayReqTimeout)){
                //wordLen =0;
                L3_event_clearEventFlag(L3_event_sayReqTimeout);
                pc.printf("\n--------------------\n [TIMEOUT] Your say request time is over. \n--------------------\n");
                pc.printf("\n*********************************************************\n");
                pc.printf("                       [STATE] IDLE");
                pc.printf("\n*********************************************************\n");
                pc.printf("Please, Enter 'y' for Request a say. ::: \n");
                main_state = L3STATE_IDLE;
            }
            break;

        case L3STATE_SAY_ON:
            if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
            {

                /*
                *  [EVENT] A > If 키보드 입력, Arbitrator에게 f메세지 전송 
                *  c1 == true ( SAY_ON State에서 타이머 정상동작하는지 확인. )
                */

                if (L3_timer_input_getTimerStatus() == 1) { 
                    // null 문자 추가 삽입 for 문자 잘림 방짐
                    originalWord[wordLen++] = '\0';
                    strcpy((char*)sdu, (char*)originalWord);
                    L3_msg_encodeData(sdu, originalWord, wordLen); //타입지정 위해서 인코드 필요
                    L3_LLI_dataReqFunc(sdu, wordLen);
                    L3_timer_input_stopTimer();
                    
{        
#ifdef ENABLE_CHANGEIDCMD
                {
                    uint8_t myid = '1' - '0'; //숫자로 바꿈
                    //debug("[L3] requesting to change to src id %i\n", myid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_SRCID, myid);
                }
#endif
}
                    L3_event_clearEventFlag(L3_event_dataToSend);

                    // memset(&originalWord[L3_MSG_OFFSET_DATA], '\0', wordLen*sizeof(uint8_t));
                    wordLen = 0;
                    pc.printf("Sending msg is completed. \n");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("                       [STATE] IDLE");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("Please, Enter 'y' for Request a say. ::: \n");
                    
                    was_say_on_state = true;
                    main_state = L3STATE_IDLE;
                } else {
                    // SAY_ON 일 때는 input 타이머 돌아야 함
                    wordLen = 0;
                    L3_timer_input_stopTimer();
                    L3_event_clearEventFlag(L3_event_dataToSend);
                    pc.printf("\n--------------------\n [ERROR] timer error occurred. \n--------------------\n");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("                       [STATE] IDLE");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("Please, Enter 'y' for Request a say. ::: \n");
                    main_state = L3STATE_IDLE;
                }
            } 
            
            /*
            *  [EVENT] C-2 > Input_timer 타임아웃 
            */
            else if(L3_event_checkEventFlag(L3_event_inputTimeout)){
{        
#ifdef ENABLE_CHANGEIDCMD
                {
                    uint8_t myid = '1' - '0'; //숫자로 바꿈
                    //debug("[L3] requesting to change to src id %i\n", myid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_SRCID, myid);
                }
#endif
}
                L3_event_clearEventFlag(L3_event_inputTimeout);
                pc.printf("\n--------------------\n [TIMEOUT] Your input time is over. \n--------------------\n");
                pc.printf("\n*********************************************************\n");
                pc.printf("                       [STATE] IDLE");
                pc.printf("\n*********************************************************\n");
                pc.printf("Please, Enter 'y' for Request a say. ::: \n");
                main_state = L3STATE_IDLE;
            }

               

            else if(L3_event_checkEventFlag(L3_event_msgRcvd)){
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t* getWordData = L3_msg_getWord(dataPtr);
                uint8_t size = L3_LLI_getSize();

                /*
                *  [EVENT] B-2 > 예외) sayReject PDU 받을 경우
                */
                if(L3_msg_checkIfRejt(dataPtr)){
{        
#ifdef ENABLE_CHANGEIDCMD
                    {
                        uint8_t myid = '1' - '0'; //숫자로 바꿈
                        //debug("[L3] requesting to change to src id %i\n", myid);
                        L3_LLI_configReqFunc(L2L3_CFGTYPE_SRCID, myid);
                    }
#endif
}
                    L3_timer_input_stopTimer();
                    L3_event_clearEventFlag(L3_event_msgRcvd);
                    pc.printf("\n--------------------\n [ERROR] An unknown error occurred. \n--------------------\n");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("                       [STATE] IDLE");
                    pc.printf("\n*********************************************************\n");
                    pc.printf("Please, Enter 'y' for Request a say. ::: \n");
                    main_state = L3STATE_IDLE;
                }
                
                /*
                 *  [EVENT] B-3 > 메세지 화면 출력 
                 */
                else if(L3_msg_checkIfData(dataPtr)){           
                    /* 방법 1.  */
                    pc.printf("====================================================\n");
                    debug("RCVD MSG : %s (length:%i)\n", getWordData, size);
                    pc.printf("====================================================\n");
                    
                    /* 방법 2.  */
                    // pc.printf("====================================================\n");
                    // debug("\n 2 &dataPtr[] ) RCVD MSG : %s (length:%i)\n",  &dataPtr[L3_MSG_OFFSET_DATA], size);
                    // pc.printf("====================================================\n");
                }

                L3_event_clearEventFlag(L3_event_msgRcvd);
            }


            break;

        default :
            break;
    }
}