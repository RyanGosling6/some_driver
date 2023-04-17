
#ifndef TA_H_
#define TA_H_

#define TA_DEV_VERSION      18
#define TA_BLOCK_SIZE       64
#define TA_BLOCK_MEM_SIZE	(TA_BLOCK_SIZE*sizeof(uint16_t))
#define TA_IRQ_FIFO_SIZE    256
#define TA_MAX_BLOCKS		1024

#define TA_REG_BASE         0
#define TA_REG_IRQ          2
#define TA_REG_CODDEC       4
#define TA_REG_RESET        6
#define TA_REG_MODE1        8
#define TA_REG_ADDR         10
#define TA_REG_MODE2        12
#define TA_REG_DATA         14
#define TA_REG_TIMERH       16
#define TA_REG_TIMERL       18
#define TA_REG_RAO          20
#define TA_REG_TIMCR        24
#define TA_REG_RLCW         26
#define TA_REG_MSGA         28

typedef struct
{
    volatile uint16_t base;
    volatile uint16_t irq;
    volatile uint16_t coddec;
    volatile uint16_t reset;
    volatile uint16_t mode1;
    volatile uint16_t addr;
    volatile uint16_t mode2;
    volatile uint16_t data;
    volatile uint16_t timerh;
    volatile uint16_t timerl;
    volatile uint16_t rao;
    volatile uint16_t unused0;
    volatile uint16_t timcr;
    volatile uint16_t rlcw;
    volatile uint16_t msga;
    volatile uint16_t unused1;
} TA_REG_T;

/* reg base */
#define RBASE_BS                0x8000  /* RT busy for given subaddress */
#define RBASE_IP                0x4000  /* write irq vector to FIFO for RT or MT mode for given subaddress */
#define RBASE_AK                0x2000  /* chip work for last instruction */
#define RBASE_BLOCK_MASK        0x03FF  /* mask of current block address for RT or MT mode */

/* reg irq */
#define RIRQ_FIFO_NOT_EMPTY     0x8000  /* interrupt FIFO is vector */
#define RIRQ_TIMER_OVERFLOW     0x4000  /* timer overflow */
#define RIRQ_CHANNEL_GENERATION 0x2000  /* generation in channel */
#define TAIRQ_EVENTS    (RIRQ_FIFO_NOT_EMPTY | RIRQ_TIMER_OVERFLOW | RIRQ_CHANNEL_GENERATION)
#define RIRQ_DEVICE_VERSION     0x1000  /* data bit of device version */
#define RIRQ_FIFO_OVERFLOW      0x0400  /* overflow of interrupt FIFO */
#define RIRQ_BLOCK_MASK         0x03FF  /* mask of interrupt message block address */

/* reg mode1 */
#define RMODE1_MASK             0xC000  /* mode mask, 0 - BC mode */
#define RMODE1_RT               0x4000  /* RT mode */
#define RMODE1_BMM              0x8000  /* MT mode - monitor of messages */
#define RMODE1_BWM              0xC000  /* MT mode - monitor of words */
#define RMODE1_PAUSE_MASK       0x3000  /* pause mask for SW, 0 - 14 mcs */
#define RMODE1_PAUSE_18         0x1000  /* pause for SW 18 mcs */
#define RMODE1_PAUSE_26         0x2000  /* pause for SW 26 mcs */
#define RMODE1_PAUSE_63         0x3000  /* pause for SW 63 mcs */
#define RMODE1_DIS_BMM          0x0800  /* disable BMM in BWM mode */
#define RMODE1_EN_IRQ           0x0400  /* enable IRQ */
#define RMODE1_TX_BUS_A         0x0200  /* bus A transmit */
#define RMODE1_TX_BUS_B         0x0100  /* bus B transmit */
#define RMODE1_RX_BUS_A         0x0080  /* bus A receive */
#define RMODE1_RX_BUS_B         0x0040  /* bus B receive */
#define RMODE1_DIS_BS           0x0020  /* global disable RT busy for mode messages */
#define RMODE1_DIS_DATA_IRQ     0x0010  /* disable RT IRQ for data messages */
#define RMODE1_START_RT         0x0008  /* disable rewrite RT address and start RT or MT */
#define RMODE1_RESET_FIFO       0x0004  /* reset IRQ FIFO */
#define RMODE1_TX_DATA_BS       0x0002  /* transmit data words for mode CWs if RT busy */
#define RMODE1_DIS_DATA_LCW_BS  0x0001  /* disable transmit data for mode CW "Transmit Last CW" if RT busy */

/* reg mode2 - BC mode */
#define RMODE2_BC_START         0x8000  /* start chain messages */
#define RMODE2_BC_ERR_IRQ       0x4000  /* enable IRQ for errors or bits in SW */
#define RMODE2_BC_STOP          0x2000  /* stop and IRQ at the end of current message */
#define RMODE2_BC_RETRY_ALLOW   0x1000  /* retry allow global */
#define RMODE2_BC_RETRY_ALLOW_2 0x0800  /* two retry allow  */
#define RMODE2_BC_RETRY_1_ALT   0x0400  /* first retry at active line (L) / first retry at inactive line (H) */
#define RMODE2_BC_RETRY_2_ALT   0x0200  /* second retry at active line (L) / second retry at inactive line (H) */
#define RMODE2_BC_DIS_FLAG_GEN  0x0100  /* disable flag generation in line */
#define RMODE2_BC_GOTO_RESERVE  0x0040  /* goto reseve line if generation */
#define RMODE2_BC_MASK_SW_BITS  0x0020  /* enable mask ME, SF, SR, BC, TF, BS bits at SW */
#define RMODE2_BC_DIS_MASK_RES_SW_BITS 0x0010 /* disable mask reserve SW bits DN, T/R */
#define RRG2_BC_STOP_SW         0x0008  /* stop chain messages and IRQ if non mask bit SW */
#define RRG2_BC_STOP_ERROR      0x0004  /* stop chain messages and IRQ if message error */
#define RRG2_BC_BIT_BC_MODE     0x0001  /* compare (H) / prohibition (L) */

/* reg mode2 - RT, MCO, MCL modes */
#define RMODE2_RT_OFF           0xF800  /* RT address */
#define RMODE2_RT_DIS_IRQ_ERR   0x0400  /* disble error IRQ */
#define RMODE2_RT_HW_BIT_MODE   0x0200  /* hardware bit mode */
#define RMODE2_RT_BIT_SR        0x0100  /* bit SR of SW */
#define RMODE2_RT_CLR_BIT_SR    0x0080  /* reset bit SR for mode CW "Transmit Vector Word" */
#define RMODE2_MCL_TEST_BUS     0x0040  /* test bus at MCL mode: bus A (L) / bus B (H) */
#define RMODE2_MCL_TEST_SYNC    0x0020  /* type of sync: data (L) / CW (H) */
#define RMODE2_RT_EN_GROUP      0x0010  /* enable group CW */
#define RMODE2_RT_BIT_BS        0x0008  /* BS bit of SW */
#define RMODE2_RT_BIT_SF        0x0004  /* SF bit of SW */
#define RMODE2_RT_ALLOW_BUS_CTRL 0x0002  /* allow bus control mode */
#define RMODE2_RT_BIT_TF        0x0001  /* TF bit of Sw */

/* reg rao */
#define RRAO_D                  0x8000  /* read/write data of the selected ram */
#define RRAO_W                  0x4000  /* read (L) / write (H) */
#define RRAO_E                  0x2000  /* enable all addresses /subaddresses (L) */
#define RRAO_R                  0x1000  /* enable RT RAM(L)/BMM RAM(H) */
#define RRAO_S                  0x0800  /* minimal time for transmit SW 4.5 mcs (L) / 8 mcs (H) */
#define RRAO_RT_CW_MASK         0x03FF  /* mask of RT ram address which include fields rw+subaddr+nwords of CW */
#define RRAO_MCO_ADDR_MASK      0x03E0  /* maks of MCO ram addres which include RT address field */

/*reg timcr */
#define RTIMCR_DIS_OVERFLOW_STOP 0x2000  /* disable stop after overflow timer */
#define RTIMCR_LOW_OVERFLOW_IRQ	0x1000  /* enable IRQ after overflow timer2 (low word) register */
#define RTIMCR_TIMER_REGS_BLOCK	0x0800  /* blocking of timer registers timer1 and timer2 rewrite */
#define RTIMCR_TIMER_RESET      0x0400  /* reset timer and blocking increment (L) */
#define RTIMCR_INC_2            0x0080  /* timer increment 2 */
#define RTIMCR_INC_4            0x0100  /* timer increment 4 */
#define RTIMCR_INC_8            0x0180  /* timer increment 8 */
#define RTIMCR_INC_16           0x0200  /* timer increment 16 */
#define RTIMCR_INC_32           0x0280  /* timer increment 32 */
#define RTIMCR_INC_64           0x0300  /* timer increment 64 */
#define RTIMCR_INC_OFF          0x0380  /* timer increment turn off */
#define RTIMCR_SYNC_RESET       0x0040  /* CW "Synchronisation" reset timer */
#define RTIMCR_SYNC_REWRITE     0x0020  /* CW "Synchronisation with data" rewrite timer low word */
#define RTIMCR_SUBADDR_REWRITE_MASK 0x001F /* mask of subaddress wich rewrite value of the timer */

/* BC message format 3, RT->RT, SWB - transmiter(RTB) status word.
 * Receiver status word (RTA) there is in data field, after actual data.
 */
typedef struct
{
    uint16_t cwA;       /* 0 */
    uint16_t cwB;       /* 1 */
    uint16_t swB;       /* 2 */
    uint16_t data[55];  /* 3 - 57 */
    uint16_t state;     /* 58 */
    uint16_t timerh;    /* 59 */
    uint16_t timerl;    /* 60 */
    uint16_t control;   /* 61 */
    uint16_t delay;     /* 62 */
    uint16_t next;      /* 63 */
} BC_MESSAGE_T;

#define BCMES_STATE_OFFSET      58
#define BCMES_TIMEH_OFFSET      59
#define BCMES_TIMEL_OFFSET      60
#define BCMES_CONTROL_OFFSET    61
#define BCMES_DELAY_OFFSET      62
#define BCMES_NEXT_OFFSET       63

/* BC message control word */
typedef struct
{
	uint16_t    tf_mask:1;      /* allow test TF bit of SW */
    uint16_t    bs:1;           /* BS mode */
    uint16_t    sf_mask:1;      /* allow test SF bit of SW */
    uint16_t    bs_mask:1;      /* allow test BS bit of SW */
    uint16_t    bc_mask:1;      /* allow test BC bit of SW */
    uint16_t    nop:1;          /* wait without transmit data to channel */
    uint16_t    rt_rt:1;        /* message format RT-RT */
    uint16_t    bus_b:1;        /* bus A (L) / bus B (H) */
    uint16_t    sr_mask:1;      /* allow test SR bit of SW */
    uint16_t    retry:1;        /* allow iterations */
    uint16_t    me_mask:1;      /* allow test ME bit of SW */
    uint16_t    stop_err:1;     /* allow stop chain if error in message */
    uint16_t    stop_sw:1;      /* allow stop chain if non mask bit in SW */
    uint16_t    auto_continue:1; /* allow to pass to next message */
    uint16_t    change_bus:1;   /* allow change bus */
	uint16_t    write_vec:1;    /* enable write interrupt vector to FIFO */
} BCMES_CONTROL_T;

#define BCMES_CONTROL_WRITE_VEC     0x8000
#define BCMES_CONTROL_CHANGE_BUS    0x4000
#define BCMES_CONTROL_CONTINUE      0x2000
#define BCMES_CONTROL_STOP_SW       0x1000
#define BCMES_CONTROL_STOP_ERR      0x0800
#define BCMES_CONTROL_ME_MASK       0x0400
#define BCMES_CONTROL_RETRY	        0x0200
#define BCMES_CONTROL_SR_MASK       0x0100
#define BCMES_CONTROL_BUS_B         0x0080
#define BCMES_CONTROL_RT_RT         0x0040
#define BCMES_CONTROL_NOP           0x0020
#define BCMES_CONTROL_BC_MASK       0x0010
#define BCMES_CONTROL_BS_MASK       0x0008
#define BCMES_CONTROL_SF_MASK       0x0004
#define BCMES_CONTROL_BS            0x0002
#define BCMES_CONTROL_TF_MASK       0x0001

/* BC message state word */
typedef struct
{
    uint16_t    error:3;        /* error code */
    uint16_t    bus:1;          /* actual bus number */
    uint16_t    bit_sw2:1;      /* unmasked bit in status word 2 */
    uint16_t    bit_sw1:1;      /* unmasked bit in status word 1 */
    uint16_t    was_retry:1;    /* was retry at the time of transmition */
    uint16_t    completed:1;    /* message completed */
    uint16_t    unused:1;
    uint16_t    seq_stop:1;     /* was stop sequence */
    uint16_t    word_cnt:6;     /* words count */
} BCMES_STATE_T;

#define BCMES_STATE_NWORD_MASK      0xFC00
#define BCMES_STATE_SEQ_STOP        0x0200
#define BCMES_STATE_COMPLETED       0x0080
#define BCMES_STATE_WAS_RETRY       0x0040
#define BCMES_STATE_BIT_SW1         0x0020
#define BCMES_STATE_BIT_SW2         0x0010
#define BCMES_STATE_BUS             0x0008
#define BCMES_STATE_ERROR_MASK      0x0007
#define BCMES_STATE_PARETY_ERR      0x0001  /* parity error */
#define BCMES_STATE_PAUSE_ERR       0x0002  /* pause error */
#define BCMES_STATE_INTEGRITY_ERR   0x0003  /* integrity error */
#define BCMES_STATE_NWORDS_ERR      0x0004  /* number of word more */
#define BCMES_STATE_ADDR_ERR        0x0005  /* RT address error */
#define BCMES_STATE_SYNC_ERR        0x0006  /* lockout pulse error */
#define BCMES_STATE_BUSY_ERR        0x0007  /* busy line error */

/* RT message */
typedef struct
{
    uint16_t    data[58];  /* 0 - 57 */
    uint16_t    state;     /* 58 */
    uint16_t    timerh;    /* 59 */
    uint16_t    timerl;    /* 60 */
    uint16_t    pad[2];    /* 61 - 62 */
    uint16_t    next;      /* 63 */
} RT_MESSAGE_T;

#define RTMES_STATE_OFFSET          58
#define RTMES_TIMERH_OFFSET         59
#define RTMES_TIMERL_OFFSET         60
#define RTMES_NEXT_OFFSET           63

/* RT message state word */
typedef struct
{
    uint16_t    nwords:5;       /* field number words of CW */
    uint16_t    subaddr:5;      /* field subaddress of CW */
    uint16_t    rt:1;           /* bit receive/transmit of CW */
    uint16_t    bus:1;          /* bus received cmd */
    uint16_t    broadcast:1;    /* broadcast cmd */
    uint16_t    unused2:1;
    uint16_t    msg_err:1;      /* message error*/
    uint16_t    unused1:1;
} RTMES_STATE_T;

#define RTMES_STATE_MSG_ERR	        0x4000
#define RTMES_STATE_BROADCAST       0x1000
#define RTMES_STATE_BUS             0x0800
#define RTMES_STATE_RT              0x0400
#define RTMES_STATE_SUBADDR_MASK    0x03E0
#define RTMES_STATE_NWORDS_MASK     0x001F

/* Bus messages monitor message */
typedef struct
{
    uint16_t data[58];  /* 0 - 57 */
    uint16_t state;     /* 58 */
    uint16_t timerh;    /* 59 */
    uint16_t timerl;    /* 60 */
    uint16_t pad[2];    /* 61 - 62 */
    uint16_t next;      /* 63 */
} BMM_MESSAGE_T;

/* Bus messages monitor state word */
typedef struct
{
    uint16_t error:3;
    uint16_t bus:1;
    uint16_t bit_sw2:1;
    uint16_t bit_sw1:1;
    uint16_t msg_type:4;
    uint16_t nwords:6;
} BMMMES_STATE_T;

#define BMMMES_STATE_NWORDS_MASK    0xFC00
#define BMMMES_STATE_ERROR_MASK     0x0007
#define BMMMES_STATE_BWM_TYPE       0x03C0  /* bus word monitor state word */
#define BMMMES_STATE_FORMAT1        0x0000  /* unicast BC->RT */
#define BMMMES_STATE_FORMAT2        0x0040  /* unicast RT->BC */
#define BMMMES_STATE_FORMAT3        0x0080  /* unicast RT->RT */
#define BMMMES_STATE_FORMAT4        0x00C0  /* unicast Channel Command */
#define BMMMES_STATE_FORMAT6        0x0100  /* unicast Channel Command with word BC->RT */
#define BMMMES_STATE_FORMAT5        0x0140  /* unicast Channel Command with word BC<-RT */
#define BMMMES_STATE_FORMAT7        0x0200  /* multicast BC->RT */
#define BMMMES_STATE_FORMAT8        0x0280  /* multicast RT->RT */
#define BMMMES_STATE_FORMAT9        0x02C0  /* multicast Channel Command */
#define BMMMES_STATE_FORMAT10       0x0300  /* multicast Channel Command with word BC->RT */
#define BMMMES_STATE_FORMAT_ERROR   0x01C0  /* format error */
#define BMMMES_STATE_PARITY_ERR     0x0001  /* parity or manchester error */
#define BMMMES_STATE_PAUSE_ERR      0x0002  /* pause error in front of SW */
#define BMMMES_STATE_CONTINUITY_ERR 0x0003  /* continuity error */
#define BMMMES_STATE_NWORDS_ERR     0x0004  /* number of words is more */
#define BMMMES_STATE_RTADDR_ERR     0x0005  /* RT address error */
#define BMMMES_STATE_SYNC_ERR       0x0006  /* synchro pulse error */
#define BMMMES_STATE_NOEND_ERR      0x0007  /* message without end */
#define BMMMES_STATE_BUS_B          0x0008  /* message from bus B */
#define BMMMES_STATE_BIT_SW2        0x0010  /* bit set in SW2 */
#define BMMMES_STATE_BIT_SW1        0x0020  /* bit set in SW1 */

/* Bus word monitor message */
#define BWMMES_NUM_MSGS             29
typedef struct {
    uint16_t word;
    uint16_t passport;
} BWM_WORD_T;

typedef struct
{
    BWM_WORD_T words[BWMMES_NUM_MSGS];
    uint16_t state;     /* 58 */
    uint16_t timerh;    /* 59 */
    uint16_t timerl;    /* 60 */
    uint16_t pad[2];    /* 61 - 62 */
    uint16_t next;      /* 63 */
} BWM_MESSAGE_T;

/* Bus words monitor passport of word */
typedef struct
{
    uint16_t pad:3;
    uint16_t not_pause:1;
    uint16_t error:1;
    uint16_t sync:1;
    uint16_t ab:1;
    uint16_t bus:1;
    uint16_t pause:8;
} BWMMES_PASSPORT_T;

#define BWMMES_PASSPORT_NOT_PAUSE      0x0008  /* transmit through channel without pause */
#define BWMMES_PASSPORT_WORD_ERROR     0x0010  /* error in word */
#define BWMMES_PASSPOST_SYNC           0x0020  /* control word */
#define BWMMES_PASSPORT_BUS_AB         0x0040  /* receive from both busses */
#define BWMMES_PASSPORT_BUS_B          0x0080  /* bus B */
#define BWMMES_PASSPORT_PAUSE_MASK     0xFF00  /* code of pause, 00000000 - no pause, 11111111 - overflow */

typedef struct
{
    uint16_t timer_overflow:1;
    uint16_t not_used:5;
    uint16_t code_bmw:4;
    uint16_t nwords:6;
}BWMMES_STATE_T;

#define BWMMES_STATE_TIMER_OVERFLOW    0x0001  /* stop timer overflow */
#define BWMMES_STATE_CODE_BWM          0x03C0  /* code of BWM mode */
#define BWMMES_STATE_NWORDS_MASK	   0xFC00  /* number words mask */

/* Bus words monitor state word */

#define BUS_A 0
#define BUS_B 1
#define BUS_1 0
#define BUS_2 1

#endif /* TA_H_ */
