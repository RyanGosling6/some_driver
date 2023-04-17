
#ifndef PORTS_H_
#define PORTS_H_

#include <sampling.h>
#include <queuing.h>

typedef struct
{
    uint8_t magic;
    uint8_t tag;
    uint8_t cmd;
    uint8_t flags;
} COMMAND_HDR;

typedef struct
{
    uint16_t softMajor;
    uint16_t softMinor;
    uint16_t hardMajor;
    uint16_t hardMinor;
} DRV_VERSION;

typedef struct
{
    COMMAND_HDR hdr;
    DRV_VERSION ver;
} DRV_RESPONSE;

typedef enum
{
    DRV_STOP_STATE = 0,
    DRV_CFG_STATE = 1,
    DRV_START_STATE = 2,
    DRV_PAUSE_STATE = 3,
    DRV_SHUTDOWN_STATE = 4
} DRV_STATE_T;

typedef struct
{
    int32_t size;
    int32_t dir;
    uint64_t refresh;
    int32_t validity;
} SamplingPortStatusType;

typedef struct
{
    int32_t nb;
    int32_t max_nb;
    int32_t max_size;
    int32_t dir;
    int32_t zero;
} QueuingPortStatusType;

typedef struct {
    char name[MAX_NAME_LENGTH];
    int32_t size;
    int32_t dir;
    uint64_t refresh;
} SamplingPortConfigType;

typedef struct {
    char name[MAX_NAME_LENGTH];
    int32_t max_size;
    int32_t max_nb;
    int32_t dir;
    int32_t dis;
} QueuingPortConfigType;

typedef struct {
    int32_t type;
    uint32_t channel;
    uint32_t port_number;
    uint32_t refresh_count;
    uint64_t update_time;
    void *queue;
    int32_t (*port_update_func)(void *desc, void *queue, void *msg,
            int32_t *len, uint64_t *time, uint64_t timeout);
    union {
        SamplingPortConfigType s;
        QueuingPortConfigType  q;
    };
} PortDescType;

#endif /* PORTS_H_ */
