#include "mbed.h"

#define L3_MSG_TYPE_DATA    0
#define L3_MSG_TYPE_REQ     1
#define L3_MSG_TYPE_ACPT    2
#define L3_MSG_TYPE_REJT    3

#define L3_MSG_OFFSET_TYPE  0
#define L3_MSG_OFFSET_SEQ   1
#define L3_MSG_OFFSET_DATA  2

#define L3_MSG_REQSIZE      3


int L3_msg_checkIfData(uint8_t* msg);
int L3_msg_checkIfReq(uint8_t* msg);
int L3_msg_checkIfAcpt(uint8_t* msg);
int L3_msg_checkIfRejt(uint8_t* msg);
uint8_t L3_msg_encodeAcpt(uint8_t* msg_data); //arbitrator에서 만드는건 승인(Acpt), 거절(Rejt), 데이터 만듦
uint8_t L3_msg_encodeRejt(uint8_t* msg_data);
uint8_t L3_msg_encodeData(uint8_t* msg_data, uint8_t* data, int len);
uint8_t* L3_msg_getWord(uint8_t* msg);
uint8_t* L3_msg_getType(uint8_t* msg);