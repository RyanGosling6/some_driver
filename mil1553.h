
#ifndef MIL1553_H_
#define MIL1553_H_

#include <stdint.h>

#define MIL1553_LOOPBACK_SUBADDR  30
#define MIL1553_MAX_RT_ADDR       32
#define MIL1553_MAX_SUB_ADDR      MIL1553_MAX_RT_ADDR
#define MIL1553_MAX_MSG_WORDS     32
#define MIL1553_MAX_MSG_SIZE      (MIL1553_MAX_MSG_WORDS*sizeof(uint16_t))

#define MIL1553_CONTROL_MODE_1 0b00000
#define MIL1553_CONTROL_MODE_2 0b11111


typedef struct
{
    uint16_t cmd_nwords:5;
    uint16_t subaddr:5;
    uint16_t dir:1;
    uint16_t address:5;
} MIL1553_CW_T;

#define MIL1553_NWORDS_MASK 0x001F
#define MIL1553_CMD_MASK 0x041F
#define MIL1553_SUBADDR_MASK 0x03E0
#define MIL1553_RT_DIR_MASK 0x0400
#define MIL1553_ADDRESS_MASK 0xF800

#define MIL1553_RT_TRANSMIT 0x0400

#define CW(ADDR,DIR,SUBADDR,NWORDS) ((ADDR<<11)|(DIR)|(SUBADDR<<5)|(NWORDS&0x1F))

#define MIL1553_CMD_DYNAMIC_BUS_CONTROL 0x00
#define MIL1553_CMD_SYNCHRONIZE 0x01
#define MIL1553_CMD_TRANSMIT_STATUS_WORD 0x02
#define MIL1553_CMD_INITIATE_SELFTEST 0x03
#define MIL1553_CMD_TRANSMITTER_SHUTDOWN 0x04
#define MIL1553_CMD_OVR_TRANSMITTER_SHUTDOWN 0x05
#define MIL1553_CMD_INHIBIT_TERMINAL_FLAG_BIT 0x06
#define MIL1553_CMD_OVR_INHIBIT_TERMINAL_FLAG_BIT 0x07
#define MIL1553_CMD_RESET_REMOTE_TERMINAL 0x08
#define MIL1553_CMD_TRANSMIT_VECTOR_WORD 0x10
#define MIL1553_CMD_SYNCHRONIZE_WITH_DATA 0x11
#define MIL1553_CMD_TRANSMIT_LAST_COMMAND 0x12
#define MIL1553_CMD_TRANSMIT_BIT_WORD 0x13
#define MIL1553_CMD_SEL_TRANSMITTER_SHUTDOWN 0x14
#define MIL1553_CMD_OVR_SEL_TRANSMITTER_SHUTDOWN 0x15


typedef struct
{
    uint16_t termFlag         : 1;
    uint16_t dynamicBusAccept : 1;
    uint16_t subsystemFlag    : 1;
    uint16_t busy             : 1;
    uint16_t brodcast         : 1;
    uint16_t unused           : 3;
    uint16_t serviceRequest   : 1;
    uint16_t instrumentation  : 1;
    uint16_t msgError         : 1;
    uint16_t termAddr         : 5;
} MIL1553_SW_T;

#endif /* MIL1553_H_ */

