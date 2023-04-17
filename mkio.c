
#include <sys/neutrino.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <process.h>
#include <tatvk.h>
#include "tadrv.h"
#include "queue.h"

typedef union {
    struct _pulse   pulse;
} timer_msg_t;

#ifdef TA_DEBUG
void print_port(TA_DESC_T *desc)
{
	  uint16_t value;
	  uint64_t port = (uint64_t)desc->reg;
	  int i;

	  printf("\n-------------- ta%d addr = 0x%08x --------------------", desc->ch, port);
	  for(i = 0; i < 15; i++) {
		  if(i == 1)
			  value = 0;
		  else
			  value = in16(port + i*sizeof(uint16_t));
		  if(i & 7)
			  printf("  0x%04x", value);
		  else
			  printf("\n0x%04x", value);
	  }
	  printf("\n-----------------------------------------------------\n");
}
#endif

static void mkioCreateTable(TA_DESC_T *desc)
{
    uint32_t sa;
    uint16_t block;
    uint16_t ip_rt = 0;
    uint16_t ip_bmm = 0;
    uint16_t bm_offset = MIL1553_MAX_SUB_ADDR * 2;
    uint16_t table = MKIO_ID_TABLE_SIZE - 1;

    out16(desc->reg + TA_REG_MSGA, table); /* Alexandr Prokopenkov fix it */

    if(desc->mode == MKIO_MODE_RT) {
    	ip_rt = RBASE_IP;
    }
    else if (desc->mode == MKIO_MODE_RT_BMM) {
   	    ip_bmm = RBASE_IP;
    	if((desc->options & MKIO_RT_OPT_RT_OFF_MODE) == 0)
        	ip_rt = RBASE_IP;
    }

    for (sa = 1; sa < (MIL1553_MAX_SUB_ADDR - 1); ++sa) {
        block = sa;						/* rx sa */
        out16mem(desc, (table<<6) + block, block | ip_rt);
        out16mem(desc, (block<<6) + RTMES_NEXT_OFFSET, block | ip_rt);
        block = sa | 0x20;				/* tx sa */
        out16mem(desc, (table<<6) + block, block | ip_rt);
        out16mem(desc, (block<<6) + RTMES_NEXT_OFFSET, block | ip_rt);
    }
    block = MIL1553_MAX_SUB_ADDR - 1;	/* All Mode Cmds */
    out16mem(desc, (table<<6) + block, block);
    out16mem(desc, (block<<6) + RTMES_NEXT_OFFSET, block);
    block = MIL1553_MAX_SUB_ADDR;		/* Cmd TX VECTOR */
    out16mem(desc, (table<<6) + block, block);
    out16mem(desc, (block<<6) + RTMES_NEXT_OFFSET, block);
    block = bm_offset - 1;				/* Cmd TX BIT */
    out16mem(desc, (table<<6) + block, block);
    out16mem(desc, (block<<6) + RTMES_NEXT_OFFSET, block);
 	/* Monitor */
    out16mem(desc, table<<6, bm_offset | ip_bmm);
    for(block = bm_offset; block <= (bm_offset + TA_IRQ_FIFO_SIZE - 2); block++)
            out16mem(desc, (block<<6) + RTMES_NEXT_OFFSET, (block + 1) | ip_bmm);
    out16mem(desc, ((block - 1)<<6) + RTMES_NEXT_OFFSET, bm_offset | ip_bmm);

#ifdef TA_DEBUG_RT_BM
    uint16_t buffer[64];

    in16block(desc, buffer, 64, table<<6);
    printf("\nAddress table 0x%04x", table<<6);
    for(i = 0; i < 64; i++) {
    	if(i%16)
    		printf(" %04x",buffer[i]);
    	else
    		printf("\n0x%04x    %04x", i, buffer[i]);
    }
    printf("\nBM blocks last word");
    for(block = 0; block < 1024; block++)  {
    		buffer[0] = in16mem(desc, (block << 6) + RTMES_NEXT_OFFSET);
    		if(block%16)
        		printf(" %04x",buffer[0]);
        	else
        		printf("\n0x%04x    %04x", block, buffer[0]);
    }
    printf("\n");
#endif
}

static int32_t mkioInit(TA_DESC_T *desc)
{
    int32_t f_index, m_index;
    uint16_t buf[TA_BLOCK_SIZE], block, mode1, mode2;
    MKIO_DRV_ID_DESC *id;
    MKIO_DRV_FRAME *frame;
    struct itimerspec itime;
    struct timespec tm;
    BC_MESSAGE_T *p = (BC_MESSAGE_T *)buf;

    memset((void *)buf, 0, TA_BLOCK_MEM_SIZE);
    out16(desc->reg + TA_REG_RESET, 0);
    usleep(100);
    out16(desc->reg + TA_REG_TIMCR, 0);
    for(m_index = 0; m_index < TA_MAX_BLOCKS; m_index++)
    	out16block(desc, buf, TA_BLOCK_SIZE, m_index << 6);

    switch(desc->mode) {

    case MKIO_MODE_BC:
        if(!desc->frame_table.num || !desc->num_ids)
            return MKIO_MODE_FAIL;

        mode1 = RMODE1_EN_IRQ|RMODE1_TX_BUS_A|RMODE1_TX_BUS_B|RMODE1_RX_BUS_A|RMODE1_RX_BUS_B;
        out16(desc->reg + TA_REG_MODE1, mode1 | RMODE1_RESET_FIFO);
        out16(desc->reg + TA_REG_MODE1, mode1);
        mode2 = (desc->options & MKIO_BC_OPT_MASK)|RMODE2_BC_DIS_MASK_RES_SW_BITS;
        out16(desc->reg + TA_REG_MODE2, mode2);
        ta_reset_time(desc);
#ifdef BC_TABLE_DEBUG
        printf("\n<< Init channel %d / frames number %d>>", desc->ch, desc->frame_table.num);
#endif
        for(f_index = 0, block = 0; f_index < desc->frame_table.num; f_index++) {
            frame = &desc->frame_table.frame[f_index];
#ifdef BC_TABLE_DEBUG
            printf("\n    Channel %d / Frame %d  / Period %d * %d nsec", desc->ch, f_index, frame->period_cnt, desc->frame_table.period);
#endif
            frame->first_block_offset = block;
            frame->start_time = frame->end_time = 0;
            for(m_index = 0; m_index < frame->len; m_index++, block++) {
                id = &desc->id_table[frame->msg[m_index]];
#ifdef BC_TABLE_DEBUG
                printf("\n        channel %d block %d cmd1=%04X  cmd2=%04X  options=%04x", desc->ch, block, id->cmd1, id->cmd2, id->options);
#endif
                p->cwA = id->cmd1;
                p->cwB = id->cmd2;
                id->control = id->options & MKIO_ID_OPT_MASK;
                if(id->cmd2) id->control |= BCMES_CONTROL_RT_RT;
                if(m_index + 1 == frame->len){
                    id->control |= BCMES_CONTROL_WRITE_VEC;
                    frame->last_block_offset = block;
                    p->next = 0;
                }
                else {
                    id->control |= BCMES_CONTROL_CONTINUE;
                    p->next = block + 1;
                }
                p->control = id->control;
                p->delay = 0;
                id->mem_offset = block << 6;
                out16block(desc, buf, TA_BLOCK_SIZE, id->mem_offset);
            }
        }
#ifdef BC_TABLE_DEBUG
            printf("\n\n");
#endif
        desc->async_frame.first_block_offset = block;
        desc->async_frame.last_block_offset = block;
        desc->async_frame.max_size = MKIO_ID_TABLE_SIZE - block;
        desc->async_frame.len = 0;
        desc->period_cnt = 0;
        if(desc->timer_id < 0) {
            if((desc->timer_chid = ChannelCreate(0)) < 0) {
                wprintf("%s:%d: timer's channel create error.\n", __FUNCTION__,__LINE__);
                desc->tvk.bist |= TA_TIMER_FAIL;
                return MKIO_TIMER_FAIL;
            }
            if((desc->timer_event.sigev_coid = ConnectAttach(0, 0, desc->timer_chid, 0, 0)) < 0) {
                wprintf("%s:%d: timer's channel connect error.\n", __FUNCTION__,__LINE__);
                desc->tvk.bist |= TA_TIMER_FAIL;
                return MKIO_TIMER_FAIL;
            }
            SIGEV_PULSE_INIT(&desc->timer_event, desc->timer_event.sigev_coid, SIGEV_PULSE_PRIO_INHERIT,
					TA_TIMER_PULSE_CODE + desc->ch, 0);
            if(timer_create(CLOCK_MONOTONIC, &desc->timer_event, &desc->timer_id) < 0){
                wprintf("%s:%d: timer create error.\n", __FUNCTION__,__LINE__);
                desc->tvk.bist |= TA_TIMER_FAIL;
                return MKIO_TIMER_FAIL;
            }
        }
        if(desc->frame_table.period > 0)
        	nsec2timespec(&tm, desc->frame_table.period);
        else
        	nsec2timespec(&tm, 100000);
        itime.it_value.tv_sec = tm.tv_sec;
        itime.it_value.tv_nsec = tm.tv_nsec;
        itime.it_interval.tv_sec = tm.tv_sec;
        itime.it_interval.tv_nsec = tm.tv_nsec;
        if(timer_settime(desc->timer_id, 0, &itime, NULL) < 0) {
            wprintf("%s:%d: timer start error.\n", __FUNCTION__,__LINE__);
            desc->tvk.bist |= TA_TIMER_FAIL;
            return MKIO_TIMER_FAIL;
        }
        break;

    case MKIO_MODE_RT:
        mode1 = RMODE1_RT|RMODE1_EN_IRQ|RMODE1_TX_BUS_A|RMODE1_TX_BUS_B|
           RMODE1_RX_BUS_A|RMODE1_RX_BUS_B;
        out16(desc->reg + TA_REG_MODE1, mode1 | RMODE1_RESET_FIFO);
//        out16(desc->reg + TA_REG_MODE1, mode1);
        mode2 = (desc->options & MKIO_RT_OPT_MASK) | (desc->rt_addr<<11);
        out16(desc->reg + TA_REG_MODE2, mode2);
        ta_reset_time(desc);
        mkioCreateTable(desc);
        out16(desc->reg + TA_REG_MODE1, mode1|RMODE1_START_RT);
        break;

    case MKIO_MODE_RT_BMM:
        if(desc->bm_in_port_index < 0) {
            wprintf("%s:%d: bus monitor port not defined.\n", __FUNCTION__,__LINE__);
            return MKIO_MODE_FAIL;
        }
        if(desc->options & MKIO_RT_OPT_RT_OFF_MODE)
            mode1 = RMODE1_BMM|RMODE1_EN_IRQ|RMODE1_RX_BUS_A|RMODE1_RX_BUS_B;
        else
            mode1 = RMODE1_BMM|RMODE1_EN_IRQ|RMODE1_TX_BUS_A|RMODE1_TX_BUS_B|
               RMODE1_RX_BUS_A|RMODE1_RX_BUS_B;
        out16(desc->reg + TA_REG_MODE1, mode1|RMODE1_RESET_FIFO);
//        out16(desc->reg + TA_REG_MODE1, mode1);
        mode2 = desc->options | (desc->rt_addr<<11);
        out16(desc->reg + TA_REG_MODE2, mode2);
        ta_reset_time(desc);
        mkioCreateTable(desc);
        out16(desc->reg + TA_REG_MODE1, mode1|RMODE1_START_RT);
        break;

    default:
        return MKIO_MODE_FAIL;
    }

#ifdef TA_DEBUG
    printf("ch %d init for %d mode\n", desc->ch, desc->mode);
    print_port(desc);
#endif

    return EOK;
}

static void mkioBcUpdatePeriodicOutData(TA_DESC_T *desc)
{
    PortDescType *port;
    int32_t i, p_index, len;
    uint16_t control;
    MIL1553_CW_T *command;
    MKIO_BC_SEND_DATA *send;
    MKIO_DRV_ID_DESC *id;

    for(i = 0; i < BC_PERIODIC_OUT_PORT_MAX_NUMBER; i++){
        if((p_index = desc->bc_periodic_out_port_index[i]) >= 0){
            port = desc->port_desc_table[p_index];
            if(port) {
                if(lock_out_queue(port->queue, (void **)&send, &len, NULL, 0) == EOK) {
                    while(len >= sizeof(MKIO_BC_SEND_DATA)){
                        id = &desc->id_table[send->id];
                        command = (MIL1553_CW_T *)&id->cmd1;
                        if((command->dir == 0)&&(id->cmd2 == 0)) {
                            out16block(desc, send->data, id->len, id->mem_offset + 1);
                            if(send->options & MKIO_ID_OPT_VALID) {
                                control = in16mem(desc, id->mem_offset + BCMES_CONTROL_OFFSET);
                                control &= ~ MKIO_ID_OPT_MASK;
                                control |= send->options & MKIO_ID_OPT_MASK;
                                out16mem(desc, id->mem_offset + BCMES_CONTROL_OFFSET, control);
                            }
                        }
                        send++;
                        len -= sizeof(MKIO_BC_SEND_DATA);
                    }
                    unlock_out_queue(port->queue);
                }
            }
            else {
            	wprintf("%s:%d Get Apex port fail.\n",__FUNCTION__,__LINE__);
            }
        }
    }
}

static uint64_t get_msg_time(TA_DESC_T *desc, uint16_t timeh, uint16_t timel)
{
	uint64_t curtime, msg_time;
	uint32_t ta_time, ta_msg_time, dtime;
	struct timespec abstime;

    ta_msg_time = ((uint32_t)timeh << 16) + (uint32_t)timel;
	ta_time = ta_get_time(desc);
	clock_gettime(CLOCK_MONOTONIC, &abstime);
	curtime = timespec2nsec(&abstime);
	if(ta_time >= ta_msg_time)
	    dtime = ta_time - ta_msg_time;
	else
		dtime = 0xffffffff - ta_msg_time + ta_time + 1;

	msg_time = curtime - ((uint64_t)dtime * 1000);

	return msg_time;
}

static void mkioBcFrameResponse(TA_DESC_T *desc, MKIO_DRV_FRAME *frame)
{
    PortDescType *port = NULL;
    uint16_t timel, timeh;
    MKIO_BC_FRAME_RESP *fresp;
    MKIO_DRV_ID_DESC *id;
    MIL1553_CW_T *command;
    int32_t err, i, size, len;
    
    if(frame->period_cnt) {
        if(desc->bc_status_port_index >= 0)
            port = desc->port_desc_table[desc->bc_status_port_index];
    }
    else {
        if(desc->bc_async_status_port_index[frame->num] >= 0)
            port = desc->port_desc_table[desc->bc_async_status_port_index[frame->num]];
    }
    if(port) {
        if((err = lock_in_queue(port->queue, (void **)&fresp, &size, NULL, 0)) == 0) {
            fresp->frame = frame->num;
            fresp->start_time = frame->start_time;
            fresp->end_time = frame->end_time;
            for(i = 0, len = sizeof(MKIO_BC_FRAME_RESP); i < frame->len; i++) {
                len += sizeof(MKIO_BC_ID_RESP);
                if(len > size) {
                  	wprintf("%s:%d Apex port %s too small: port size %d.\n",
               				__FUNCTION__,__LINE__, port->s.name, size);
                    break;
                }
                fresp->resp[i].id = frame->msg[i];
                id = &desc->id_table[frame->msg[i]];
                timeh = in16mem(desc, id->mem_offset + BCMES_TIMEH_OFFSET);
                timel = in16mem(desc, id->mem_offset + BCMES_TIMEL_OFFSET);
                fresp->resp[i].t = get_msg_time(desc, timeh, timel);
                fresp->resp[i].state = in16mem(desc, id->mem_offset + BCMES_STATE_OFFSET);
#ifdef BC_DEBUG
                if(fresp->resp[i].state & 7) {
                  	printf("BC channel %d message id %d state 0x%04x\n", desc->ch, frame->msg[i], fresp->resp[i].state);
                }
#endif
               	command = (MIL1553_CW_T *)&id->cmd1;
                if(id->cmd2 == 0) {
              	    fresp->resp[i].sw[1] = 0;
               		if(command->dir == 0) {
               		    fresp->resp[i].sw[0] = in16mem(desc, id->mem_offset + 1 + id->len);
                        fresp->resp[i].data = in16mem(desc, id->mem_offset + 1);
               		}
               	    else {
               		    fresp->resp[i].sw[0] = in16mem(desc, id->mem_offset + 1);
                        fresp->resp[i].data = in16mem(desc, id->mem_offset + 2);
               	    }
                }
                else {
               		fresp->resp[i].sw[0] = in16mem(desc, id->mem_offset + 3 + id->len);
              		fresp->resp[i].sw[1] = in16mem(desc, id->mem_offset + 2);
               		fresp->resp[i].data = 0;
                }
            }
            unlock_in_queue(port->queue, len);
        }
    }
}

/* BC receives formats 1,2,3 */
static void mkioBcUpdateInData(TA_DESC_T *desc, MKIO_DRV_FRAME *frame)
{
    PortDescType *port;
    void *resp;
    MKIO_DRV_ID_DESC *id;
    MKIO_BC_RECV_DATA *recv;
    MIL1553_CW_T *command;
    uint16_t timeh, timel;
    uint64_t time;
	struct timespec abstime;
    int32_t i, p_index, size, err;

	clock_gettime(CLOCK_MONOTONIC, &abstime);
	time = timespec2nsec(&abstime);
    for(i = 0; i < frame->len; i++) {
        id = &desc->id_table[frame->msg[i]];
        if(id->cmd2 == 0)
            command = (MIL1553_CW_T *)&id->cmd1;
        else
            command = (MIL1553_CW_T *)&id->cmd2;
        p_index = desc->bc_in_port_index[command->address];
        if(p_index >= 0) {
            if ((command->dir)&&(command->subaddr > 0)&&(command->subaddr < 31)) {
                port = desc->port_desc_table[p_index];
                if(port) {
                    if((err = lock_in_queue(port->queue, (void **)&resp, &size, &time, -1)) == 0) {
                   	    if(size >= (id->port_offset + sizeof(MKIO_BC_RECV_DATA))) {
               	            recv = (MKIO_BC_RECV_DATA *)(resp + id->port_offset);
               	            recv->id = frame->msg[i];
                            timeh = in16mem(desc, id->mem_offset + BCMES_TIMEH_OFFSET);
                            timel = in16mem(desc, id->mem_offset + BCMES_TIMEL_OFFSET);
                            recv->t = get_msg_time(desc, timeh, timel);
                            recv->state = in16mem(desc, id->mem_offset + BCMES_STATE_OFFSET);
                            if(id->cmd2 == 0) {
                                in16block(desc, recv->data, id->len, id->mem_offset + 2);
                                recv->sw[0] = in16mem(desc, id->mem_offset + 1);
                                recv->sw[1] = 0;
                       	    }
                            else {
                                in16block(desc, recv->data, id->len, id->mem_offset + 3);
                                recv->sw[0] = in16mem(desc, id->mem_offset + 2);
                                recv->sw[1] = in16mem(desc, id->mem_offset + 3 + id->len);
                            }
                            if(frame->period_cnt)
                                out16block(desc, (uint16_t *)id, 2, id->mem_offset);
                   	    }
#if 0
                        else {
                      	    wprintf("%s:%d Apex port %s too small: port size=%d, id offset=%d\n",
                       			    __FUNCTION__,__LINE__, port->s.name, size, id->port_offset + sizeof(MKIO_BC_RECV_DATA));
                        }
#endif
                        unlock_in_queue(port->queue, size);
                    }
                }
                else {
                    wprintf("%s:%d Get Apex port fail.\n",__FUNCTION__,__LINE__);
                }
            }
        }
    }
}
#endif

static void mkioRtUpdateOutData(TA_DESC_T *desc, uint16_t block)
{
    PortDescType *port = NULL;
    uint16_t *p;
    int32_t p_index, len;

    if((p_index = desc->rt_periodic_out_port_index[0]) != (-1)){
        port = desc->port_desc_table[p_index];
        if(port) {
            if(lock_out_queue(port->queue, (void **)&p, &len, NULL, 0) == EOK) {
            	p += ((block & 0x1f) - 1) * MIL1553_MAX_MSG_WORDS;
               	out16block(desc, (void *)p, MIL1553_MAX_MSG_WORDS, block << 6);
                unlock_out_queue(port->queue);
   	        }
        }
    }
}

static void mkioRtUpdateInData(TA_DESC_T *desc, uint16_t block)
{
    PortDescType *port;
    MKIO_RT_RECV_DATA *data;
    uint16_t offset, len, timeh, timel;
    int32_t size, err;
    RTMES_STATE_T state;
	struct timespec abstime;
    uint64_t time;

    if(desc->rt_in_port_index >= 0) {
    	clock_gettime(CLOCK_MONOTONIC, &abstime);
    	time = timespec2nsec(&abstime);
        offset = block << 6;
        *((uint16_t *)&state) = in16mem(desc, offset + RTMES_STATE_OFFSET);
        port = desc->port_desc_table[desc->rt_in_port_index];
        if(err = lock_in_queue(port->queue, (void **)&data, &size, &time, 0)){
            wprintf("%s:%d Open Apex port %s fail : err=%d\n",__FUNCTION__,__LINE__, port->s.name, err);
            return;
        }
        if(size < (sizeof(MKIO_RT_RECV_DATA)*state.subaddr)) {
            unlock_in_queue(port->queue, 0);
            return;
        }
        len = state.nwords ? state.nwords : 32;
        in16block(desc, (void *)data[state.subaddr-1].data, len, offset);
        timeh = in16mem(desc, offset + RTMES_TIMERH_OFFSET);
        timel = in16mem(desc, offset + RTMES_TIMERL_OFFSET);
        data[state.subaddr-1].t = get_msg_time(desc, timeh, timel);
        data[state.subaddr-1].len = len;
        unlock_in_queue(port->queue, size);
    }
}

static void mkioBmUpdateInData(TA_DESC_T *desc, uint16_t block)
{
    PortDescType *port;
    int32_t err;
    int32_t size = 0;
    void *buf;

    if(desc->bm_in_port_index >= 0) {
        port = desc->port_desc_table[desc->bm_in_port_index];
        if(err = lock_in_queue(port->queue, (void **)&buf, &size, NULL, 0)){
            wprintf("%s:%d Open Apex port %s fail : err=%d\n",__FUNCTION__,__LINE__, port->s.name, err);
            return;
        }
        if(size < TA_BLOCK_MEM_SIZE) {
            unlock_in_queue(port->queue, 0);
            wprintf("%s:%d Port %s size %d too small. Must be %d\n",__FUNCTION__,__LINE__,
            		port->s.name, size, TA_BLOCK_SIZE);
            return;
        }
        in16block(desc, buf, TA_BLOCK_SIZE, block << 6);
        if((((uint16_t *)buf)[0] == 0)||
        		((((uint16_t *)buf)[58] & BMMMES_STATE_FORMAT_ERROR) == BMMMES_STATE_FORMAT_ERROR))
        	size = 0;
        else
        	size = TA_BLOCK_MEM_SIZE;
        unlock_in_queue(port->queue, size);
    }
}

static int32_t mkioBcUpdateAsyncOutData(TA_DESC_T *desc, int32_t port_index)
{
    PortDescType *port;
    int32_t len;
    MKIO_BC_SEND_DATA *p;
    MKIO_DRV_ID_DESC *id;
    uint16_t block, mode, control;
    MKIO_DRV_FRAME *frame = &desc->async_frame;
    int32_t num_blocks = 0;

	block = frame->first_block_offset;
    frame->options = 0;
    frame->period_cnt = 0;
    frame->num = port_index;
    port = desc->port_desc_table[desc->bc_async_out_port_index[port_index]];
    if(port) {
        if(lock_out_queue(port->queue, (void **)&p, &len, NULL, 0) == EOK) {
            while(len > 0) {
         	    if(num_blocks >= frame->max_size) {
            	    wprintf("Asynchronous frame size exceeds max size %d.\n", frame->max_size);
            		break;
            	}
         	    frame->msg[num_blocks] = p->id;
                id = &desc->id_table[p->id];
                id->mem_offset = block << 6;
                out16mem(desc, id->mem_offset, id->cmd1);
                if(p->options & MKIO_ID_OPT_VALID)
                    control = p->options & MKIO_ID_OPT_MASK;
                else
                    control = id->options & MKIO_ID_OPT_MASK;
                control |= BCMES_CONTROL_CONTINUE;
                if(id->cmd2 == 0) {
                	if(id->cmd1 & MIL1553_RT_DIR_MASK)
                        out16mem(desc, id->mem_offset + 1, 0);
                	else
                        out16block(desc, p->data, id->len, id->mem_offset + 1);
                }
                else {
                    out16mem(desc, id->mem_offset + 1, id->cmd2);
                    control |= BCMES_CONTROL_RT_RT;
                }
                out16mem(desc, id->mem_offset + BCMES_CONTROL_OFFSET, control);
                out16mem(desc, id->mem_offset + BCMES_DELAY_OFFSET, 0);
                out16mem(desc, id->mem_offset + BCMES_NEXT_OFFSET, block + 1);
                block++;
                num_blocks++;
                len -= sizeof(MKIO_BC_SEND_DATA);
                p++;
            }
            unlock_out_queue(port->queue);
        }

        if(num_blocks) {
            frame->len = num_blocks;
        	block--;
        	frame->last_block_offset = block;
            mode = in16mem(desc, (block << 6) + BCMES_CONTROL_OFFSET);
            mode &= ~BCMES_CONTROL_CONTINUE;
            out16mem(desc, (block << 6) + BCMES_CONTROL_OFFSET, mode | BCMES_CONTROL_WRITE_VEC);
            out16mem(desc, (block << 6) + BCMES_NEXT_OFFSET, 0);
            return num_blocks;
        }
    }

    return EOK;
}

static void mkioBcSendAsyncFrame(TA_DESC_T *desc)
{
    uint16_t mode;

    pthread_mutex_lock(&desc->hw_mutex);
    out16(desc->reg + TA_REG_MSGA, desc->async_frame.first_block_offset);
    mode = in16(desc->reg + TA_REG_MODE2);
    out16(desc->reg + TA_REG_MODE2, mode|RMODE2_BC_START);
    pthread_mutex_unlock(&desc->hw_mutex);
}

static void mkioWaitInt(TA_DESC_T *desc, MKIO_DRV_FRAME *frame)
{
    uint16_t data16;
    struct timespec tm;

    sem_wait(&desc->int_sem);

    while ((data16 = in16(desc->reg + TA_REG_IRQ)) & TAIRQ_EVENTS) {
        if(data16 & RIRQ_CHANNEL_GENERATION) {
           	// Save state some registers
           	uint16_t saveMode1Reg = in16(desc->reg + TA_REG_MODE1);
           	uint16_t saveMode2Reg = in16(desc->reg + TA_REG_MODE2);
           	saveMode2Reg &= ~RMODE1_START_RT;
           	uint16_t saveTmrCntrlReg = in16(desc->reg + TA_REG_TIMCR);
           	uint16_t saveAddrReg = in16(desc->reg + TA_REG_MSGA);
#ifdef BC_CHANNEL_GENERATION_DEBUG
        	printf("############################################################## CHANNEL %d GENERATION\n", desc->ch);
    		printf("int_count=%d, TA_REG_MODE1=0x%04X, TA_REG_MODE2=0x%04X, TA_REG_IRQ=0x%04X, TA_REG_MSGA=0x%04X\n",
    			    desc->tvk.int_count, saveMode1Reg, saveMode2Reg, data16, saveAddrReg);
#endif
           	// make software reset TA
    		out16(desc->reg + TA_REG_RESET, 0);
    		usleep(1000);
    		// restore some registers
    		out16(desc->reg + TA_REG_MODE1, saveMode1Reg);
    		out16(desc->reg + TA_REG_MODE2, saveMode2Reg);
    		out16(desc->reg + TA_REG_TIMCR, saveTmrCntrlReg);
    		out16(desc->reg + TA_REG_MSGA, saveAddrReg);
            desc->tvk.bist |= TA_CHANNEL_GEN;
        }
        else {
            if(data16 & RIRQ_TIMER_OVERFLOW) {
                ta_reset_time((void *)desc);
            }
            if(data16 & RIRQ_FIFO_NOT_EMPTY) {
            	switch(desc->mode) {
            	case MKIO_MODE_BC:
            		clock_gettime(CLOCK_MONOTONIC, &tm);
            		frame->end_time = timespec2nsec(&tm);
            		mkioBcFrameResponse(desc, frame);
            		mkioBcUpdateInData(desc, frame);
            		break;
            	case MKIO_MODE_RT:
            	case MKIO_MODE_RT_BMM:
            		data16 &= RIRQ_BLOCK_MASK;
            		if((data16 > 0)&&(data16 < 0x1f))
            			mkioRtUpdateInData(desc, data16);
            		else if((data16 > 0x1f)&&(data16 < 0x40))
            			mkioRtUpdateOutData(desc, data16);
            		else if(data16 >= 0x40)
            			mkioBmUpdateInData(desc, data16);
            		break;
            	default:
            		break;
            	}
            }
        }
    }

    pthread_mutex_lock(&desc->hw_mutex);
    data16 = in16(desc->reg + TA_REG_MODE1);
   	out16(desc->reg + TA_REG_MODE1, data16 | RMODE1_EN_IRQ);
    pthread_mutex_unlock(&desc->hw_mutex);
}

static void mkioBcUpdatePeriodicFrame(TA_DESC_T *desc, MKIO_DRV_FRAME *frame)
{
	uint16_t index, block, state;
    MKIO_DRV_ID_DESC *id;

    pthread_mutex_lock(&desc->request_mutex);
    for(index = 0; index < frame->len; index++) {
        id = &desc->id_table[frame->msg[index]];
        block = id->mem_offset >> 6;
        out16mem(desc, id->mem_offset, id->cmd1);
        if(id->cmd2)
            out16mem(desc, id->mem_offset + 1, id->cmd2);
        out16mem(desc, id->mem_offset + BCMES_CONTROL_OFFSET, id->control);
        out16mem(desc, id->mem_offset + BCMES_DELAY_OFFSET, 0);
        if(index == frame->len - 1)
            out16mem(desc, id->mem_offset + BCMES_NEXT_OFFSET, 0);
        else
            out16mem(desc, id->mem_offset + BCMES_NEXT_OFFSET, block + 1);
        state = in16mem(desc, id->mem_offset + BCMES_STATE_OFFSET);
        if(((state & 7) && ((state & 7) != 2)) && ((desc->ch == 0) || (desc->ch == 2))) /* state & 7 == 2 неверная пауза перед ответным словом */
        	printf("##bc%d id=%d cmd1=%04x cmd2=%04x state=%04x##\n", desc->ch, frame->msg[index], id->cmd1, id->cmd2, state );
    }
    pthread_mutex_unlock(&desc->request_mutex);
}

#undef FRAME_TABLE_DEBUG
static int32_t mkioProcess(TA_DESC_T *desc)
{
    int32_t i, f_index, rcvid;
    uint16_t mode;
    timer_msg_t msg;
    struct timespec tm;
    MKIO_DRV_FRAME *frame;
#ifdef FRAME_TABLE_DEBUG
    static int count = 0;
#endif

    switch(desc->mode) {
    case MKIO_MODE_BC:
        if((rcvid = MsgReceive(desc->timer_chid, &msg, sizeof(msg), NULL)) < 0){
            wprintf("%s:%d: pulse receive error.\n", __FUNCTION__,__LINE__);
            desc->tvk.bist |= TA_TIMER_FAIL;
            return MKIO_TIMER_FAIL;
        }
        else if (rcvid == 0) { /* we got a pulse */
#ifdef TA_TIMER_DEBUG
        	printf("timer type=0x%04x subtype=0x%04x code=0x%02x sival=0x%08x scoid=0x%08x\n",
        			msg.pulse.type, msg.pulse.subtype, msg.pulse.code, msg.pulse.value.sival_int, msg.pulse.scoid);
#endif
            if (msg.pulse.code == (TA_TIMER_PULSE_CODE + desc->ch))
                desc->period_cnt++;
            else
                return EOK;
        }
        else {
            return EOK;
        }
        for(f_index = 0; f_index < desc->frame_table.num; f_index++) {
        	for(i = 0; i < BC_ASYNC_OUT_PORT_MAX_NUMBER; i++) {
                if(desc->bc_async_out_port_index[i] < 0)
                    break;
                if(mkioBcUpdateAsyncOutData(desc, i)) {
            	    clock_gettime(CLOCK_MONOTONIC, &tm);
            	    desc->async_frame.start_time = timespec2nsec(&tm);
                    mkioBcSendAsyncFrame(desc);
                    mkioWaitInt(desc, &desc->async_frame);
                }
        	}
            frame = &desc->frame_table.frame[f_index];
            if((desc->period_cnt % frame->period_cnt) == 0) {
            	mkioBcUpdatePeriodicOutData(desc);
                clock_gettime(CLOCK_MONOTONIC, &tm);
                frame->start_time = timespec2nsec(&tm);
#ifdef FRAME_TABLE_DEBUG
                count++;
                if((count % 1000) == 0)
                    printf("Frame period count %d, time %ld\n", count, frame->start_time);
#endif
                pthread_mutex_lock(&desc->hw_mutex);
                mode = in16(desc->reg + TA_REG_MODE2);
                out16(desc->reg + TA_REG_MSGA, frame->first_block_offset);
           	    out16(desc->reg + TA_REG_MODE2, mode|RMODE2_BC_START);
           	    pthread_mutex_unlock(&desc->hw_mutex);
                mkioWaitInt(desc, frame);
                mkioBcUpdatePeriodicFrame(desc, frame);
            }
        }
        break;
    default:
    	mkioWaitInt(desc, NULL);
    	break;
    }

    return EOK;
}

static int32_t mkioBreak(TA_DESC_T *desc)
{
    if(desc->timer_id >= 0) {
        timer_delete(desc->timer_id);
        desc->timer_id = -1;
    }

    return EOK;
}

void *mkioThread(void *desc)
{
    TA_DESC_T *ch_desc = (TA_DESC_T *)desc;

    ThreadCtl(_NTO_TCTL_IO, 0);

    while(ch_desc->state != DRV_SHUTDOWN_STATE) {
    	dprintf("%s:%d: Channel %d new state wait\n", __FUNCTION__,__LINE__,ch_desc->ch);
        if(sem_wait(&ch_desc->stop_sem)) {
        	wprintf("%s:%d: semaphor fail\n", __FUNCTION__,__LINE__);
            ch_desc->status = MKIO_FAILURE;
            return NULL;
        }
    	dprintf("%s:%d: Channel %d new state %d\n", __FUNCTION__,__LINE__,ch_desc->ch, ch_desc->state);
        if(ch_desc->state != DRV_SHUTDOWN_STATE)
            ch_desc->status = mkioInit(ch_desc);
        while(((ch_desc->state == DRV_START_STATE)||
                (ch_desc->state == DRV_PAUSE_STATE))&&
                (ch_desc->status == EOK)) {
            if(ch_desc->state == DRV_PAUSE_STATE) {
                sem_wait(&ch_desc->pause_sem);
            }
            ch_desc->status = mkioProcess(ch_desc);
            ch_desc->mkio_count++;
        }
        dprintf("%s:%d: Channel %d break from current state\n", __FUNCTION__,__LINE__,ch_desc->ch);
        ch_desc->status = mkioBreak(ch_desc);
    }
    dprintf("%s:%d: shutdown ch %d\n", __FUNCTION__,__LINE__, ch_desc->ch);

    return NULL;
}
