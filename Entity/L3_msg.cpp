#include "mbed.h"
#include "L3_msg.h"

int L3_msg_checkIfData(uint8_t* msg)
{
    return (msg[L3_MSG_OFFSET_TYPE] == L3_MSG_TYPE_DATA);
}

int L3_msg_checkIfReq(uint8_t* msg)
{
    return (msg[L3_MSG_OFFSET_TYPE] == L3_MSG_TYPE_REQ);
}

int L3_msg_checkIfAcpt(uint8_t* msg)
{
    return (msg[L3_MSG_OFFSET_TYPE] == L3_MSG_TYPE_ACPT);
}

int L3_msg_checkIfRejt(uint8_t* msg)
{
    return (msg[L3_MSG_OFFSET_TYPE] == L3_MSG_TYPE_REJT);
}
uint8_t L3_msg_encodeReq(uint8_t* msg_ack, uint8_t seq)
{
    msg_ack[L3_MSG_OFFSET_TYPE] =  L3_MSG_TYPE_REQ;
    msg_ack[L3_MSG_OFFSET_SEQ] = seq;
    msg_ack[L3_MSG_OFFSET_DATA] = 1;

    return L2_MSG_ACKSIZE; //return값 뭘로하지? 
}

uint8_t L3_msg_encodeData(uint8_t* msg_data, uint8_t* data, int seq, int len)
{
    msg_data[L3_MSG_OFFSET_TYPE] = L3_MSG_TYPE_DATA;
    msg_data[L3_MSG_OFFSET_SEQ] = seq;
    memcpy(&msg_data[L3_MSG_OFFSET_DATA], data, len*sizeof(uint8_t));

    return len+L3_MSG_OFFSET_DATA; //?
}