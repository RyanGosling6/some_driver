
#ifndef _APEX_SAMPLING_H
#define _APEX_SAMPLING_H

#include <apex/type.h>

typedef NAME_TYPE                SAMPLING_PORT_NAME_TYPE;
typedef APEX_INTEGER             SAMPLING_PORT_ID_TYPE;

typedef enum
{
   INVALID = 0,
   VALID = 1
} VALIDITY_TYPE;

typedef struct
{
   MESSAGE_SIZE_TYPE    MAX_MESSAGE_SIZE;
   PORT_DIRECTION_TYPE  PORT_DIRECTION;
   SYSTEM_TIME_TYPE     REFRESH_PERIOD;
   VALIDITY_TYPE        LAST_MSG_VALIDITY;

} SAMPLING_PORT_STATUS_TYPE;

#ifdef __cplusplus
extern "C" {
#endif

void CREATE_SAMPLING_PORT (
/*in */ SAMPLING_PORT_NAME_TYPE    SAMPLING_PORT_NAME,
/*in */ MESSAGE_SIZE_TYPE          MAX_MESSAGE_SIZE,
/*in */ PORT_DIRECTION_TYPE        PORT_DIRECTION,
/*in */ SYSTEM_TIME_TYPE           REFRESH_PERIOD,
/*out*/ SAMPLING_PORT_ID_TYPE    *SAMPLING_PORT_ID,
/*out*/ RETURN_CODE_TYPE         *RETURN_CODE  );


void WRITE_SAMPLING_MESSAGE (
/*in */ SAMPLING_PORT_ID_TYPE SAMPLING_PORT_ID,
/*in */ MESSAGE_ADDR_TYPE     MESSAGE_ADDR,  /* �� ������ */
/*in */ MESSAGE_SIZE_TYPE     LENGTH,
/*out*/ RETURN_CODE_TYPE      *RETURN_CODE );

void READ_SAMPLING_MESSAGE (
/*in */ SAMPLING_PORT_ID_TYPE SAMPLING_PORT_ID,
/*out*/ MESSAGE_ADDR_TYPE     MESSAGE_ADDR,
/*out*/ MESSAGE_SIZE_TYPE     *LENGTH,
/*out*/ VALIDITY_TYPE         *VALIDITY,
/*out*/ RETURN_CODE_TYPE      *RETURN_CODE );

void GET_SAMPLING_PORT_ID (
/*in */ SAMPLING_PORT_NAME_TYPE  SAMPLING_PORT_NAME,
/*out*/ SAMPLING_PORT_ID_TYPE    *SAMPLING_PORT_ID,
/*out*/ RETURN_CODE_TYPE         *RETURN_CODE );

void GET_SAMPLING_PORT_STATUS (
/*in */ SAMPLING_PORT_ID_TYPE       SAMPLING_PORT_ID,
/*out*/ SAMPLING_PORT_STATUS_TYPE   *SAMPLING_PORT_STATUS,
/*out*/ RETURN_CODE_TYPE            *RETURN_CODE );

#ifdef __cplusplus
}
#endif
#endif /* _APEX_SAMPLING_H */
