
#ifndef QUEUE_H_
#define QUEUE_H_

#include <pthread.h>

#define QUEUE_OPTION_FIFO		0x00000001
#define QUEUE_OPTION_PRIORITY	0x00000002
#define QUEUE_OPTION_SAMPLING	0x10000000
#define QUEUE_OPTION_QUEUING	0x20000000

typedef struct
{
    int32_t     length;
    uint64_t    time;
    void        *next;
    uint8_t     data[0];
} queue_node_type;

typedef struct
{
    queue_node_type     *first;
    queue_node_type     *last;
    int32_t             nb;
    int32_t             max_nb;
    int32_t             max_size;
    int32_t             options;
    pthread_mutex_t     mutex;
    pthread_cond_t      cond;
    uint64_t            refresh;
    queue_node_type     node[0];
} queue_type;

int32_t create_queue(int32_t max_size, int32_t max_nb, int32_t options, void **queue);
int32_t delete_queue(void *queue);
int32_t clear_queue(void *queue);
int32_t num_msgs_queue(void *queue, int32_t *number);
int32_t in_queue(void *desc, void *queue, void *msg, int32_t *size, uint64_t *time, uint64_t timeout);
int32_t out_queue(void *desc, void *queue, void *msg, int32_t *size, uint64_t *time, uint64_t timeout);
int32_t lock_in_queue(void *queue, void **msg, int32_t *size, uint64_t *time, uint64_t timeout);
int32_t unlock_in_queue(void *queue, int32_t size);
int32_t lock_out_queue(void *queue, void **msg, int32_t *size, uint64_t *time, uint64_t timeout);
int32_t unlock_out_queue(void *queue);

#endif /* QUEUE_H_ */
