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
#define ENABLE_CHANGEIDCMD          1

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

    if(main_state != L3STATE_WAIT_SAY){
        if (!L3_event_checkEventFlag(L3_event_dataToSend)) {
            if (c == '\n' || c == '\r') {
                originalWord[wordLen++] = '\0';
                L3_event_setEventFlag(L3_event_dataToSend);
                debug_if(DBGMSG_L3,"키보드 입력 값 ::: %s\n", originalWord);
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
    pc.printf("발언권을 얻으려면 'y'를 눌러주세요. : ");
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
            // 메세지 받는 경우
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {
                
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                if(L3_msg_checkIfData(dataPtr)){ //승인,거절은 버려지게               

                debug("\n -------------------------------------------------\nRCVD MSG : %s (length:%i)\n -------------------------------------------------\n", 
                            dataPtr, size);
                
                pc.printf("Give a word to send : ");
                }

                L3_event_clearEventFlag(L3_event_msgRcvd);
            }

            // 메세지 보내는 경우
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
            {
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
                        
                        L3_event_clearEventFlag(L3_event_dataToSend);
                        
                        pc.printf("NOW YOUR STATE IS  WAIT SAY ! ! : \n");  
                            
                        main_state = L3STATE_WAIT_SAY;

                    } else {
                        wordLen = 0;
                        pc.printf("y를 통해 발언권을 신청한 후 메세지를 입력해주세요.\n");

                        L3_event_clearEventFlag(L3_event_dataToSend);  
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
            // 메세지 받는 것  // SDU 들어옴   
            if(L3_event_checkEventFlag(L3_event_msgRcvd))
            {
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                pc.printf("현재 스테이트 : WAIT_SAY\n");
                pc.printf("메세지 잘 받아진다. \n\n");

                // if sayAccept  
                    // input_timer 시작하면서 sayReq_timer 중지
                    // sayON State로 움직여요 
                if(L3_msg_checkIfAcpt(dataPtr))
                {
                    L3_timer_input_startTimer();
                    L3_timer_sayReq_stopTimer();
                    pc.printf("메세지 타입 : SAY_ACCEPT\n");
                    pc.printf("발언권을 받았습니다. 원하는 메세지를 적어주세요. \n");
                    pc.printf("SAY_ON 스테이트로 이동합니다. \n");
                    L3_event_clearEventFlag(L3_event_msgRcvd);

#ifdef ENABLE_CHANGEIDCMD
                {
                    uint8_t myid = '3' - '0'; //숫자로 바꿈
                    pc.printf("*****myid: %d\n", myid); //3으로 srcID 변경
                    debug("[L3] requesting to change to srce id %i\n", myid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_SRCID, myid);
                }
#endif
                    main_state = L3STATE_SAY_ON;
                }

                // else if sayReject 
                    // IDLE State로 움직여요 
                    // sayReq_timer 중지해요
                else if(L3_msg_checkIfRejt(dataPtr))
                {
                    L3_timer_sayReq_stopTimer();
                    pc.printf("메세지 타입 : REJECT\n");
                    pc.printf("발언권을 얻지 못했습니다. \n");
                    pc.printf("IDLE 스테이트로 이동합니다.\n");
                    L3_event_clearEventFlag(L3_event_msgRcvd);
                    main_state = L3STATE_IDLE;
                }

            }
            
            break;

        case L3STATE_SAY_ON:
            if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
            {
                pc.printf("**************\n SAY ON 에서 메세지  보내기\n ***************\n");
                if (L3_timer_input_getTimerStatus() == 1) { //a) SDU in, c1 == true
                    strcpy((char*)sdu, (char*)originalWord);
                    L3_msg_encodeData(sdu, originalWord, wordLen);
                    L3_LLI_dataReqFunc(sdu, wordLen);
                    //debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                    L3_timer_input_stopTimer();
                    wordLen = 0;
                    pc.printf("Give a word to send : ");
                    L3_event_clearEventFlag(L3_event_dataToSend);

{        
#ifdef ENABLE_CHANGEIDCMD
                {
                    uint8_t myid = '1' - '0'; //숫자로 바꿈
                    pc.printf("*****myid: %d\n", myid); //1으로 srcID 변경(발언권 없애는 의미_이거 아니면 변수 바뀐거 체크하는 핸들러 필요)
                    debug("[L3] requesting to change to srce id %i\n", myid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_SRCID, myid);
                }
#endif
}
                    main_state = L3STATE_IDLE;
                }
            } 
            
            // // 인풋 타임아웃 -> 아이들로 돌아가세용
            // else if(L3_event_checkEventFlag(L3_event_inputTimeout)){
            //     L3_event_clearEventFlag(L3_event_inputTimeout);
            //     main_state = L3STATE_IDLE;
            // }

            
            // else if(L3_event_checkEventFlag(L3_event_msgRcvd)){ //어떠한 경우에 sayReject PDU 받을 경우
            //     L3_event_clearEventFlag(L3_event_msgRcvd);
            //     main_state = L3STATE_IDLE;
            // }

            break;

        default :
            break;
    }
}