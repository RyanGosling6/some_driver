
#ifndef TADRV_H_
#define TADRV_H_

#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <hw/pci.h>
#include <hw/inout.h>
#include <drvtvk.h>
#include "mkio.h"
#include "ports.h"
#include "ta.h"

#define DRV_MAJOR_VERSION	1
#define DRV_MINOR_VERSION	1

#define MKIO_THREAD_PRIORITY 63
#define MSG_THREAD_PRIORITY  20
#define TVK_THREAD_PRIORITY  10

#define CFG_SUB_VID         0x2C
#define CFG_SUB_DID         0x2E
#define CFG_ADDR1           0x14
#define CFG_IRQ             0x3C
#define PLX_VID             0x10B5
#define PLX_PEX_BUS_DID     0x8111
#define PLX_LOCAL_BUS_DID   0x9056
#define ELCUS_VID           0xE1C5
#define TA_DID              0x0009

#define TA_TIMER_PULSE_CODE 1

#define MAX_PORTS_NUMBER (134 + 1)
#define PORT_NAME_MAX 32

#define DRV_MAX_MSG_SIZE 4096
#define DRV_MSG_SIZE (DRV_MAX_MSG_SIZE-sizeof(int32_t)*6)

#define TA_CYCLE_WAIT_MAX	40

#ifdef TA_DEBUG
#define dprintf printf
#else
#define dprintf(...)
#endif

#define eprintf printf
#define wprintf printf

typedef struct {
    uint16_t cmd1;
    uint16_t cmd2;
    uint16_t control;
    uint16_t offset;        /* 1 -> MKIO_BC_RECV_DATA */
    uint16_t options;       /* MKIO_ID_OPT_ */
    uint16_t op;            /* 0 - BC -> RT, 1 - BC <- RT, 2 - RT -> RT */
    uint16_t len;           /* 1 -> sizeof(uint16_t) */
    uint16_t mem_offset;    /* 1 -> sizeof(uint16_t) */
    uint16_t port_offset;
    uint16_t zero;
} MKIO_DRV_ID_DESC;

typedef struct {
    uint64_t start_time;			/* start frame time in usec */
    uint64_t end_time;				/* end frame time in usec */
    uint16_t period_cnt;			/* frame minimal period cnt */
    uint16_t first_block_offset;	/* 1 -> TA_BLOCK_SIZE */
    uint16_t last_block_offset;		/* 1 -> TA_BLOCK_SIZE */
    uint16_t max_size;	/* max size of frame (1 -> TA_BLOCK_SIZE) */
    uint16_t options;   /* MKIO_ID_OPT_ASYNC_HI and MKIO_ID_OPT_ASYNC_LO */
    uint16_t num;					/* frame number */
    uint16_t len;					/* number of messages at frame */
    uint16_t msg[MKIO_FRAME_SIZE];  /* array of messages id, 256 max*/
} MKIO_DRV_FRAME;

typedef struct {
    uint32_t num;		/* number of frames */
    uint32_t period;	/* frames minimal period nsec */
    MKIO_DRV_FRAME frame[MKIO_FRAME_TABLE_SIZE]; /* array of frames, 256 max */
} MKIO_DRV_FRAME_TABLE;

typedef struct
{
    /* drv */
    int32_t                 state;  /* must be first */
    int32_t                 status;
    int32_t                 ch;
    int32_t                 msg_tid;
    int32_t                 mkio_tid;
    int32_t                 tvk_tid;
    uint32_t                mkio_count;
    uint32_t                mkio_count_old;
    uint32_t                mkio_count_wait;
    uint64_t                reg;
    sem_t                   stop_sem;
    sem_t                   pause_sem;
    sem_t                   int_sem;
    pthread_mutex_t         hw_mutex;
    pthread_mutex_t         async_mutex;
    pthread_mutex_t         request_mutex;
    pthread_cond_t          async_cond;
    
    /* arinc ports */
    PortDescType            *port_desc_table[MAX_PORTS_NUMBER];
    int32_t                 cfg_request_port_index;
    int32_t                 cfg_response_port_index;
    int32_t                 bc_status_port_index;
    int32_t                 bc_in_port_index[BC_IN_PORT_MAX_NUMBER];
    int32_t                 bc_periodic_out_port_index[BC_PERIODIC_OUT_PORT_MAX_NUMBER];
    int32_t                 bc_async_status_port_index[BC_ASYNC_OUT_PORT_MAX_NUMBER];
    int32_t                 bc_async_out_port_index[BC_ASYNC_OUT_PORT_MAX_NUMBER];
    int32_t                 rt_in_port_index;
    int32_t                 rt_periodic_out_port_index[RT_PERIODIC_OUT_PORT_MAX_NUMBER];
    int32_t                 bm_in_port_index;
    int32_t                 tvk_port_index;

    /* mkio */
    uint8_t                 mode;
    uint8_t                 flags;
    int32_t                 num_ids;
    MKIO_DRV_ID_DESC        id_table[MKIO_ID_TABLE_SIZE];
    MKIO_DRV_FRAME_TABLE    frame_table;
    MKIO_DRV_FRAME          async_frame;
    int16_t                 intblock;
    uint16_t                options;    /* MKIO_BC_OPT_ or MKIO_RT_OPT*/
    uint32_t                period_cnt;
    timer_t                 timer_id;
    int32_t                 timer_chid;
    struct sigevent         timer_event;
    uint16_t                rt_addr;


    PCIDRV_TVK_DATA_TYPE    tvk;
    uint32_t                int_count_old;
    uint32_t                int_count_wait;
    
} TA_DESC_T;

static __inline__ uint16_t __attribute__((__unused__))
in16mem(TA_DESC_T *desc, uint16_t __addr) {
    uint16_t __data;

    pthread_mutex_lock(&desc->hw_mutex);
    *((volatile uint16_t *)(desc->reg + TA_REG_ADDR)) = __addr;
    __data = *((volatile uint16_t *)(desc->reg + TA_REG_DATA));
    pthread_mutex_unlock(&desc->hw_mutex);

    return __data;
}

static __inline__ void __attribute__((__unused__))
in16block(TA_DESC_T *desc, void *__buff, uint32_t __len, uint16_t __addr) {
    uint16_t *__p = (uint16_t *)__buff;

    pthread_mutex_lock(&desc->hw_mutex);
    *((volatile uint16_t *)(desc->reg + TA_REG_ADDR)) = __addr;
    --__p;
    while(__len > 0) {
        *++__p = *((volatile uint16_t *)(desc->reg + TA_REG_DATA));
        --__len;
    }
    pthread_mutex_unlock(&desc->hw_mutex);
}

static __inline__ void __attribute__((__unused__))
out16mem(TA_DESC_T *desc, uint32_t __addr, uint16_t __data) {

    pthread_mutex_lock(&desc->hw_mutex);
    *((volatile uint16_t *)(desc->reg + TA_REG_ADDR)) = __addr;
    *((volatile uint16_t *)(desc->reg + TA_REG_DATA)) = __data;
    pthread_mutex_unlock(&desc->hw_mutex);
}

static __inline__ void __attribute__((__unused__))
out16block(TA_DESC_T *desc, const void *__buff, uint32_t __len, uint16_t __addr) {
    uint16_t *__p = (uint16_t *)__buff;

    pthread_mutex_lock(&desc->hw_mutex);
    *((volatile uint16_t *)(desc->reg + TA_REG_ADDR)) = __addr;
    --__p;
    while(__len > 0) {
        *((volatile uint16_t *)(desc->reg + TA_REG_DATA)) = *++__p;
        --__len;
    }
    pthread_mutex_unlock(&desc->hw_mutex);
}

static __inline__ void __attribute__((__unused__))
ta_reset_time(TA_DESC_T *desc) {
    pthread_mutex_lock(&desc->hw_mutex);
    *((volatile uint16_t *)(desc->reg + TA_REG_TIMCR)) = 0;
    *((volatile uint16_t *)(desc->reg + TA_REG_TIMCR)) = RTIMCR_TIMER_RESET;
    pthread_mutex_unlock(&desc->hw_mutex);
}

static __inline__ uint32_t __attribute__((__unused__))
ta_get_time(TA_DESC_T *desc){
    uint16_t __data, __datal, __datah;
    uint32_t __data32;

    pthread_mutex_lock(&desc->hw_mutex);
    __data = *((volatile uint16_t *)(desc->reg + TA_REG_TIMCR));
    *((volatile uint16_t *)(desc->reg + TA_REG_TIMCR)) = __data | RTIMCR_TIMER_REGS_BLOCK;
    __datal = *((volatile uint16_t *)(desc->reg + TA_REG_TIMERL));
    __datah = *((volatile uint16_t *)(desc->reg + TA_REG_TIMERH));
    __data32 = (__datah << 16) + __datal;
    *((volatile uint16_t *)(desc->reg + TA_REG_TIMCR)) = __data;
    pthread_mutex_unlock(&desc->hw_mutex);

    return __data32;
}

#endif /* TADRV_H_ */
