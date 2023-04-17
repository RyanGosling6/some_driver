
#ifndef MKIO_H_
#define MKIO_H_

#include <mil1553.h>
#include <stdint.h>

#define NTA	                            4

#define TA_DRV_NAME                     "ta"

#define MKIO_REQUEST_PORT_NAME          "request"
#define MKIO_RESPONSE_PORT_NAME         "response"
#define BC_STATUS_PORT_NAME             "bc/status"
#define BC_ASYNC_STATUS_PORT_NAME       "bc/async/status"
#define BC_IN_PORT_NAME                 "bc/a"
#define BC_IN_PORT_MAX_NUMBER           31
#define BC_PERIODIC_OUT_PORT_NAME       "bc/periodic/out"
#define BC_PERIODIC_OUT_PORT_MAX_NUMBER 32
#define BC_ASYNC_OUT_PORT_NAME          "bc/async/out"
#define BC_ASYNC_OUT_PORT_MAX_NUMBER    32
#define RT_IN_PORT_NAME                 "rt/in"
#define RT_PERIODIC_OUT_PORT_NAME       "rt/periodic/out"
#define RT_PERIODIC_OUT_PORT_MAX_NUMBER 32
#define BM_IN_PORT_NAME                 "bm/in"

/* config header magic field */
#define MKIO_MAGIC          0x5a

#define MKIO_MODE_NONE      0
#define MKIO_MODE_BC        1
#define MKIO_MODE_RT        2
#define MKIO_MODE_RT_BMM    3

#define MKIO_INIT           0x0000
#define MKIO_READY          0x0001
#define MKIO_FAILURE        0x0002
#define MKIO_CHAN_GEN       0x0004
#define MKIO_TIMER_FAIL     0x0008
#define MKIO_INTERRUPT_FAIL 0x0010
#define MKIO_MODE_FAIL      0x0020
#define MKIO_ASYNC_REQUEST  0x0040

/* config header cmd field */
#define MKIO_CMD_BC              1 
#define MKIO_CMD_BC_PARAMS       2
#define MKIO_CMD_RT              3
#define MKIO_CMD_RT_BMM          4
#define MKIO_CMD_RT_BMM_PARAMS   5
#define MKIO_CMD_STOP            6
#define MKIO_CMD_SHUTDOWN        7
#define MKIO_CMD_BC_ID_OPTIONS   8

/* config header response flags field */
#define MKIO_FLAG_ERROR      1
#define MKIO_FLAG_MODE_ERROR 2
#define MKIO_FLAG_NOT_SUPPORTED 3

/* BC ID options (ID options and Send data options) */
#define MKIO_ID_OPT_ASYNC_HI    0x0001 /* asynchronous high level priority message */
#define MKIO_ID_OPT_ASYNC_LO    0x0002 /* asynchronous low level priority message */
#define MKIO_ID_OPT_NOP         0x0020 /* no operarion */
#define MKIO_ID_OPT_BUS_B       0x0080 /* 0 - send at line A / 1 - send at line B */
#define MKIO_ID_OPT_RETRY       0x0200 /* retry enable */
#define MKIO_ID_OPT_CHNG_BUS    0x4000 /* enable change bus for retry */
#define MKIO_ID_OPT_VALID       0x8000 /* rewrite id options */
#define MKIO_ID_OPT_MASK (MKIO_ID_OPT_NOP|MKIO_ID_OPT_BUS_B|MKIO_ID_OPT_RETRY|MKIO_ID_OPT_CHNG_BUS)

/* BC options (BC param) */
#define MKIO_BC_OPT_RETRY_ENABLE        0x1000 /* global retry enable */
#define MKIO_BC_OPT_TWO_RETRY           0x0800 /* two retry enable */
#define MKIO_BC_OPT_RETRY_FIRST_B       0x0400 /* 0 - first retry at line A / 1 - first retry at line B */
#define MKIO_BC_OPT_RETRY_SECOND_B      0x0200 /* 0 - second retry at line A / 1 - second retry at line B */
#define MKIO_BC_OPT_DIS_FLAG_GEN        0x0100 /* disable flag generation at line */
#define MKIO_BC_OPT_GEN_GOTO_RESRV_LINK	0x0040 /* goto reserve link if generation flag */
#define MKIO_BC_OPT_DELAY_MODE_2        0x0002 /* 0 - delay before transmition / 1 - delay after transmition */
#define MKIO_BC_OPT_MASK (MKIO_BC_OPT_RETRY_ENABLE|MKIO_BC_OPT_TWO_RETRY|MKIO_BC_OPT_RETRY_FIRST_B| \
                            MKIO_BC_OPT_RETRY_SECOND_B|MKIO_BC_OPT_DIS_FLAG_GEN| \
                            MKIO_BC_OPT_GEN_GOTO_RESRV_LINK|MKIO_BC_OPT_DELAY_MODE_2)

/* RT options (RT param) */
#define MKIO_RT_OPT_RT_OFF_MODE         0x0400 /* RT off mode in RT_BMM */
#define MKIO_RT_OPT_HW_BIT_MODE         0x0200 /* hardware bit mode */
#define MKIO_RT_OPT_BIT_SR              0x0100 /* bit SR of SW */
#define MKIO_RT_OPT_CLR_BIT_SR          0x0080 /* reset bit SR for mode CW "Transmit Vector Word" */
#define MKIO_RT_OPT_EN_GROUP            0x0010 /* enable group CW */
#define MKIO_RT_OPT_BIT_BS              0x0008 /* BS bit of SW */
#define MKIO_RT_OPT_BIT_SF              0x0004 /* SF bit of SW */
#define MKIO_RT_OPT_ALLOW_BUS_CTRL      0x0002 /* allow bus control mode */
#define MKIO_RT_OPT_BIT_TF              0x0001 /* TF bit of Sw */
#define MKIO_RT_OPT_MASK (MKIO_RT_OPT_HW_BIT_MODE|MKIO_RT_OPT_BIT_SR|MKIO_RT_OPT_CLR_BIT_SR| \
                            MKIO_RT_OPT_EN_GROUP|MKIO_RT_OPT_BIT_BS|MKIO_RT_OPT_BIT_SF| \
                            MKIO_RT_OPT_ALLOW_BUS_CTRL|MKIO_RT_OPT_BIT_TF)

/* BC operations */
#define MKIO_BC_TO_RT               0
#define MKIO_RT_TO_BC               1
#define MKIO_RT_TO_RT               2

#define MKIO_ID_TABLE_SIZE          1024
#define MKIO_FRAME_TABLE_SIZE       256
#define MKIO_FRAME_SIZE             256

typedef struct {
    uint16_t cmd1;
    uint16_t cmd2;
    uint16_t offset;    /* 1 -> sizeof(MKIO_BC_RECV_DATA) for BC <- RT cmds */
    uint16_t options;   /* MKIO_ID_OPT_ */
} MKIO_ID_DESC;

typedef struct {
    uint16_t num_clocks;            /* frame period in clocks */
    uint16_t msg[MKIO_FRAME_SIZE]; /* array of messages id, 256 max*/
} MKIO_FRAME;

typedef struct {
    uint16_t clock;     /* clock period msec */
    MKIO_FRAME frame[MKIO_FRAME_TABLE_SIZE]; /* array of frames, 256 max */
} MKIO_FRAME_TABLE;

typedef struct {
    uint64_t t;         /* message time ending */
    uint16_t id;        /* msg id */
    uint16_t sw[2];     /* first and second status word */
    uint16_t state;     /* controller state word BCMES_STATE_ */ 
    uint16_t data;      /* data of control message */
} MKIO_BC_ID_RESP;

typedef struct {
    uint32_t frame;             /* frame number */
    uint64_t start_time;        /* frame start time */
    uint64_t end_time;          /* frame end time */
    MKIO_BC_ID_RESP resp[0];    /* response id descriptors */
} MKIO_BC_FRAME_RESP;

typedef struct {
    uint16_t id;
    uint16_t options;           /* MKIO_ID_OPT_ */
    uint16_t data[MIL1553_MAX_MSG_WORDS];
} MKIO_BC_SEND_DATA;

typedef struct {
    uint64_t t;         /* message time ending */
    uint16_t id;        /* msg id */
    uint16_t sw[2];     /* first and second status word */
    uint16_t state;     /* controller state word BCMES_STATE_ */ 
    uint16_t data[MIL1553_MAX_MSG_WORDS];
} MKIO_BC_RECV_DATA;

typedef struct {
    uint16_t sa;
    uint16_t data[MIL1553_MAX_MSG_WORDS];
} MKIO_RT_SEND_DATA;

typedef struct {
    uint64_t t;         /* ta time */
    uint16_t len;       /* 1 -> uint16_t */
    uint16_t data[MIL1553_MAX_MSG_WORDS];
} MKIO_RT_RECV_DATA;

typedef struct {
    uint16_t optOn;
    uint16_t optOff;
} MKIO_OPTIONS;

typedef struct {
    uint16_t id;
    uint16_t options;
} MKIO_ID_OPTIONS;

#endif /* MKIO_H_ */
