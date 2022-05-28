typedef enum L3_event
{
    L3_event_msgRcvd = 0,
    L3_event_dataToSend = 1,
    L3_event_sayReqToSend = 2,
    L3_event_inputTimeout = 3,
    L3_event_sayReqTimeout = 4
} L3_event_e;


void L3_event_setEventFlag(L3_event_e event);
void L3_event_clearEventFlag(L3_event_e event);
void L3_event_clearAllEventFlag(void);
int L3_event_checkEventFlag(L3_event_e event);