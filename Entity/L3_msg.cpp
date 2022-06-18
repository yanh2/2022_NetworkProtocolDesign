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
uint8_t L3_msg_encodeReq(uint8_t* msg_data)
{
    msg_data[L3_MSG_OFFSET_TYPE] = L3_MSG_TYPE_REQ;
    msg_data[L3_MSG_OFFSET_DATA] = 1;

    return L3_MSG_REQSIZE; //return값 뭘로하지?
}

uint8_t L3_msg_encodeData(uint8_t* msg_data, uint8_t* data, int len)
{
    msg_data[L3_MSG_OFFSET_TYPE] = L3_MSG_TYPE_DATA;
    memcpy(&msg_data[L3_MSG_OFFSET_DATA], data, len*sizeof(uint8_t)+1);

    return len+L3_MSG_OFFSET_DATA+1; //?
}

uint8_t* L3_msg_getWord(uint8_t* msg)
{
    return &msg[L3_MSG_OFFSET_DATA];
}

uint8_t* L3_msg_getType(uint8_t* msg)
{
    return &msg[L3_MSG_OFFSET_TYPE];
}