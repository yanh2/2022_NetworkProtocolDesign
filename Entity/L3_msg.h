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
uint8_t L3_msg_encodeReq(uint8_t* msg_ack, uint8_t seq); //entity에서 만드는건 req만 만듦
uint8_t L3_msg_encodeData(uint8_t* msg_data, uint8_t* data, int seq, int len);