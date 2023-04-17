
#ifndef _APEX_TYPE_H
#define _APEX_TYPE_H

#include <stdint.h>



typedef uint8_t         APEX_BYTE;           /* 8-битный без знака */
typedef int32_t         APEX_INTEGER;        /* 32-битный со знаком */
typedef uint32_t        APEX_UNSIGNED;       /* 32-битный без знака */
typedef int64_t         APEX_LONG_INTEGER;   /* 64-битный со знаком */

typedef 
enum {
	SAMPLING = 0,
	QUEUING  = 1
} PORT_TYPE;

typedef
enum {
   NO_ERROR       = 0,
   NO_ACTION      = 1,
   NOT_AVAILABLE  = 2,
   INVALID_PARAM  = 3,
   INVALID_CONFIG = 4,
   INVALID_MODE   = 5,
   TIMED_OUT      = 6
} RETURN_CODE_TYPE;

#define MAX_NAME_LENGTH       32       
typedef char         NAME_TYPE[MAX_NAME_LENGTH];
typedef void *       SYSTEM_ADDRESS_TYPE;
typedef APEX_BYTE *     MESSAGE_ADDR_TYPE;
typedef APEX_INTEGER    MESSAGE_SIZE_TYPE;
typedef APEX_INTEGER    MESSAGE_RANGE_TYPE;
typedef APEX_UNSIGNED   WAITING_RANGE_TYPE;

typedef enum {
   SOURCE      = 0,
   DESTINATION = 1

} PORT_DIRECTION_TYPE;

typedef enum {
   FIFO     = 0,
   PRIORITY = 1
}QUEUING_DISCIPLINE_TYPE;

typedef APEX_LONG_INTEGER     SYSTEM_TIME_TYPE;

typedef APEX_INTEGER    LOCK_LEVEL_TYPE;

#define INFINITE_TIME_VALUE   -1

#endif /* _APEX_TYPE_H */

