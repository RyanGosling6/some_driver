
#ifndef IO_FUNC_H_
#define IO_FUNC_H_

#include <devctl.h>
#include <sys/resmgr.h>
#include <sys/iofunc.h>

typedef struct {
    iofunc_attr_t   attr;
    void            *driver_desc;
    void            **ports_desc_table;
    int32_t         num_ports;
} ext_iofunc_attr_t;

typedef struct {
    int32_t         id;
    int32_t         validity;
    int32_t         nbytes;
    uint64_t        timeout;
    char            msg[0];
} DrvMessageType;

#define CMD_CREATE_SAMPLING_PORT		1
#define CMD_WRITE_SAMPLING_MESSAGE		2
#define CMD_READ_SAMPLING_MESSAGE		3
#define CMD_GET_SAMPLING_PORT_ID        4
#define CMD_GET_SAMPLING_PORT_STATUS	5
#define CMD_CREATE_QUEUING_PORT			6
#define CMD_SEND_QUEUING_MESSAGE	  	7
#define CMD_RECEIVE_QUEUING_MESSAGE		8
#define CMD_GET_QUEUING_PORT_ID         9
#define CMD_GET_QUEUING_PORT_STATUS		10
#define CMD_CLEAR_QUEUING_PORT			11
#define CMD_GET_DRV_BUFFER_SIZE			12

#define IOCTL_CREATE_SAMPLING_PORT __DIOTF(_DCMD_MISC,CMD_CREATE_SAMPLING_PORT,DrvMessageType)
#define IOCTL_WRITE_SAMPLING_MESSAGE __DIOTF(_DCMD_MISC,CMD_WRITE_SAMPLING_MESSAGE,DrvMessageType)
#define IOCTL_READ_SAMPLING_MESSAGE __DIOTF(_DCMD_MISC,CMD_READ_SAMPLING_MESSAGE,DrvMessageType)
#define IOCTL_GET_SAMPLING_PORT_ID __DIOTF(_DCMD_MISC,CMD_GET_SAMPLING_PORT_ID,DrvMessageType)
#define IOCTL_GET_SAMPLING_PORT_STATUS __DIOTF(_DCMD_MISC,CMD_GET_SAMPLING_PORT_STATUS,DrvMessageType)
#define IOCTL_CREATE_QUEUING_PORT    __DIOTF(_DCMD_MISC,CMD_CREATE_QUEUING_PORT,DrvMessageType)
#define IOCTL_SEND_QUEUING_MESSAGE __DIOTF(_DCMD_MISC,CMD_SEND_QUEUING_MESSAGE,DrvMessageType)
#define IOCTL_RECEIVE_QUEUING_MESSAGE __DIOTF(_DCMD_MISC,CMD_RECEIVE_QUEUING_MESSAGE,DrvMessageType)
#define IOCTL_GET_QUEUING_PORT_ID    __DIOTF(_DCMD_MISC,CMD_GET_QUEUING_PORT_ID,DrvMessageType)
#define IOCTL_GET_QUEUING_PORT_STATUS __DIOTF(_DCMD_MISC,CMD_GET_QUEUING_PORT_STATUS,DrvMessageType)
#define IOCTL_CLEAR_QUEUING_PORT __DIOTF(_DCMD_MISC, CMD_CLEAR_QUEUING_PORT,DrvMessageType)
#define IOCTL_GET_DRV_BUFFER_SIZE __DIOF(_DCMD_MISC, CMD_GET_DRV_BUFFER_SIZE,DrvMessageType)

#endif /* IO_FUNC_H_ */
