
#include <sys/neutrino.h>
#include <unix.h>
#include <string.h>
#include <tatvk.h>
#include "tadrv.h"
#include "queue.h"

static int32_t mkioStop(void *desc)
{
    int32_t sem_value;
    TA_DESC_T *ch_desc = (TA_DESC_T *)desc;

    switch(ch_desc->state) {
    case DRV_START_STATE:
        ch_desc->state = DRV_STOP_STATE;
        out16(ch_desc->reg + TA_REG_MODE1, RMODE1_RESET_FIFO);
   		if(sem_getvalue(&ch_desc->int_sem, &sem_value) == 0) {
   			if(!sem_value)
   				sem_post(&ch_desc->int_sem);
   		}
    case DRV_CFG_STATE:
        ch_desc->state = DRV_STOP_STATE;
        out16(ch_desc->reg + TA_REG_RESET, 0);
        usleep(10000);
    case DRV_STOP_STATE:
        return EOK;
    }

    return EPROTO;
}

static int32_t mkioStart(void *desc)
{
    TA_DESC_T *ch_desc = (TA_DESC_T *)desc;

    if((ch_desc->state == DRV_STOP_STATE)||
            (ch_desc->state == DRV_CFG_STATE)){
        ch_desc->state = DRV_START_STATE;
        return (sem_post(&ch_desc->stop_sem));
    }

    return EPROTO;
}

static int32_t mkioPause(void *desc)
{
    TA_DESC_T *ch_desc = (TA_DESC_T *)desc;

    if(ch_desc->state == DRV_START_STATE) {
        ch_desc->state = DRV_PAUSE_STATE;
        return EOK;
    }

    return EPROTO;
}

static int32_t mkioContinue(void *desc)
{
    TA_DESC_T *ch_desc = (TA_DESC_T *)desc;

    if(ch_desc->state == DRV_PAUSE_STATE) {
        ch_desc->state = DRV_START_STATE;
        return (sem_post(&ch_desc->pause_sem));
    }

    return EPROTO;
}

static int32_t mkioShutdown(void *desc)
{
    int32_t sem_value;
    TA_DESC_T *ch_desc = (TA_DESC_T *)desc;

    out16(ch_desc->reg + TA_REG_RESET, 0);
    ch_desc->state = DRV_SHUTDOWN_STATE;
	if(sem_getvalue(&ch_desc->int_sem, &sem_value) == 0) {
		if(!sem_value)
			sem_post(&ch_desc->int_sem);
	}
    return (sem_post(&ch_desc->stop_sem));
}

static int32_t mkioGetIdTable(void *desc, uint8_t *in)
{
    TA_DESC_T *ch_desc = (TA_DESC_T *)desc;
    int32_t i, len;
    MIL1553_CW_T *cmd;
    MKIO_ID_DESC *p = (MKIO_ID_DESC *)in;

    memset((void *)ch_desc->id_table, 0, sizeof(MKIO_DRV_ID_DESC)*MKIO_ID_TABLE_SIZE);

    for(i = 0, len = 0; i < MKIO_ID_TABLE_SIZE; i++, p++) {
        len += sizeof(MKIO_ID_DESC);
        if((p->cmd1 == 0xffff)&&(p->cmd2 == 0xffff))
            break;
        (( MKIO_DRV_ID_DESC *)&ch_desc->id_table[i])->cmd1 = p->cmd1;
        (( MKIO_DRV_ID_DESC *)&ch_desc->id_table[i])->cmd2 = p->cmd2;
        (( MKIO_DRV_ID_DESC *)&ch_desc->id_table[i])->offset = p->offset;
        (( MKIO_DRV_ID_DESC *)&ch_desc->id_table[i])->port_offset = p->offset * sizeof(MKIO_BC_RECV_DATA);
        (( MKIO_DRV_ID_DESC *)&ch_desc->id_table[i])->options = p->options;
        cmd = (MIL1553_CW_T *)&p->cmd1;
        if(p->cmd2 == 0) { /* BC -> RT, BC <- RT */
            ch_desc->id_table[i].op = cmd->dir;
            if((cmd->subaddr == MIL1553_CONTROL_MODE_1)||
                    (cmd->subaddr == MIL1553_CONTROL_MODE_2)) {
                /* control mode */
                if(cmd->cmd_nwords > MIL1553_CMD_RESET_REMOTE_TERMINAL)
                    /* команда управления со словом данных */
                    ch_desc->id_table[i].len = 1;
                else
                    ch_desc->id_table[i].len = 0;
            }
            else {
                /* data mode */
                ch_desc->id_table[i].len = cmd->cmd_nwords ? cmd->cmd_nwords : 32;
            }
        } 
        else { /* RT -> RT */
        	if(cmd->dir) {
                wprintf("%s:%d: id %d cmd1 error.\n",__FUNCTION__,__LINE__,i);
                return (-1);
        	}
            cmd = (MIL1553_CW_T *)&p->cmd2;
        	if(!cmd->dir) {
                wprintf("%s:%d: id %d cmd2 error.\n",__FUNCTION__,__LINE__,i);
                return (-1);
        	}
            ch_desc->id_table[i].len = cmd->cmd_nwords ? cmd->cmd_nwords : 32;
            ch_desc->id_table[i].op = MKIO_RT_TO_RT;
        }
    }
    if(i == MKIO_ID_TABLE_SIZE) {
        wprintf("%s:%d: BC id table size exceeds  memory size.\n",__FUNCTION__,__LINE__);
        return (-1);
    }
    ch_desc->num_ids = ++i;

    dprintf("%s:%d len=%d\n",__FUNCTION__,__LINE__,len);
    return len;
}

static int32_t mkioGetFrameTable(TA_DESC_T *desc, uint8_t *in)
{
    int32_t m_index, status_port_len;
    int32_t f_index = 0;
    int32_t len = 0;
    uint16_t *p = (uint16_t *)in;

    for(f_index = 0; f_index < MKIO_FRAME_TABLE_SIZE; f_index++)
        memset((void *)&desc->frame_table.frame[f_index], 0, sizeof(MKIO_DRV_FRAME));
    if(*p == 0xffff) {
        len++;
        goto frame_table_exit;
    }
    if(*p < 2)
    	wprintf("%s:%d: Frame table period set to %d. Must be 2 or more.\n", __FUNCTION__,__LINE__, *p);
    desc->frame_table.period = *p++ * 1000000; /* period in nsec */
    len++;
    for(f_index = 0; f_index < MKIO_FRAME_TABLE_SIZE; f_index++) {
    	dprintf("frame%d ", f_index);
        if(*p == 0xffff) {
        	p++;
            len++;
            desc->frame_table.num = f_index;
            break;
        }
        desc->frame_table.frame[f_index].num = f_index;
        desc->frame_table.frame[f_index].period_cnt = *p++;
        len++;
        for(m_index = 0, status_port_len = sizeof(MKIO_BC_FRAME_RESP); 
                m_index < MKIO_FRAME_SIZE; m_index++) {
        	dprintf(" %d id = %d", m_index, *p);
            len++;
            if(*p== 0xffff) {
            	dprintf(".\n");
            	p++;
#if 0
                if(desc->flags & MKIO_CONF_FLAG_SYNC_LAST_ID)
                    /* TODO INSERT LAST FRAME ID */
                    desc->frame_table.frame[f_index].len = m_index + 1;
                else
#endif
                    desc->frame_table.frame[f_index].len = m_index;
                break;
            }
            else if(*p >= desc->num_ids) {
                wprintf("%s:%d: Frame %d id %d exceeds ID table size.\n",
                       __FUNCTION__,__LINE__, f_index+1, *p);
                p++;
                return (-1);
            }
            desc->frame_table.frame[f_index].msg[m_index] = *p++;
            status_port_len += sizeof(MKIO_BC_ID_RESP);
        }
        if(desc->bc_status_port_index >= 0) {
            if(status_port_len > desc->port_desc_table[desc->bc_status_port_index]->q.max_size) {
                wprintf("%s:%d: Frame %d exceeds port %s size.\n",__FUNCTION__,__LINE__,
                        f_index, desc->port_desc_table[desc->bc_status_port_index]->q.name);
                return (-1);
            }
        }
        if(m_index == MKIO_FRAME_SIZE) {
            wprintf("%s:%d: Frame %d size exceeds max frame size.\n",
                   __FUNCTION__,__LINE__, f_index);
            return (-1);
        }
    }
    if(f_index == MKIO_FRAME_TABLE_SIZE) {
        wprintf("%s:%d: Configuration exceeds frame table size.\n",__FUNCTION__,__LINE__);
        return (-1);
    }

frame_table_exit:
    desc->async_frame.num = f_index;
    len *= sizeof(uint16_t);
    dprintf("%s:%d len=%d\n",__FUNCTION__,__LINE__,len);

    return len;
}

static int32_t mkioGetOptions(void *desc, uint8_t *in)
{
    uint16_t optOn, optOff;
    TA_DESC_T *ch_desc = (TA_DESC_T *)desc;
    uint16_t *p = (uint16_t *)in;

    optOn = *p++;
    optOff = *p;

    ch_desc->options &= ~optOff;
    ch_desc->options |= optOn;

    dprintf("%s:%d\n",__FUNCTION__,__LINE__);
    return (sizeof(uint16_t)*2);
}

static int32_t mkioGetIdOptions(void *desc, uint8_t *in)
{
    TA_DESC_T *ch_desc = (TA_DESC_T *)desc;
    int32_t i, len;
    MKIO_ID_OPTIONS *p = (MKIO_ID_OPTIONS *)in;

    for(i = 0, len = 0; i < MKIO_ID_TABLE_SIZE; i++, p++) {
        len += sizeof(MKIO_ID_OPTIONS);
        if((p->id == 0xffff)&&(p->options == 0xffff))
            break;
        if((p->options & MKIO_ID_OPT_VALID) &&
        	(p->id >= 0) && (p->id < MKIO_ID_TABLE_SIZE)) {
            (( MKIO_DRV_ID_DESC *)&ch_desc->id_table[p->id])->control &= ~MKIO_ID_OPT_MASK;
            (( MKIO_DRV_ID_DESC *)&ch_desc->id_table[p->id])->control |= p->options & MKIO_ID_OPT_MASK;
        }
    }
    dprintf("%s:%d num options = %d\n",__FUNCTION__,__LINE__, i);

    return len;
}

#define NEXT_PARAM(in, length, offset, err) \
    if(offset > 0) { \
        in += offset; \
        length -= offset; \
    } else { \
        err++; \
        goto cfg_exit; \
    } \
    if(length < 0) { \
        err++; \
        goto cfg_exit; \
    }

static int32_t cfg_request(
    void *desc,
    void *port,
    void *request,
    int32_t *len,
    uint64_t *time,
    uint64_t timeout)
{
    uint8_t mode;
    int32_t length, lrequest;
    int32_t ret = EOK;
    int32_t err = 0;
    int32_t offset = 0;
    uint8_t *out, *response;
    uint8_t *in = (uint8_t *)request;
    uint16_t data16;
    TA_DESC_T *ch_desc;

    if((!desc)||(!request)||(!len)||(*len <= 0))
        return EINVAL;

    ch_desc = (TA_DESC_T *)desc;
    ch_desc->tvk.bist = 0;
    lrequest = *len;
    if(ch_desc->cfg_response_port_index >= 0) {
        if(ret = lock_in_queue(ch_desc->port_desc_table[ch_desc->cfg_response_port_index]->queue, 
            (void **)&response, &length, NULL, 0))
            goto cfg_exit;
        if(length < (sizeof(COMMAND_HDR) + sizeof(DRV_VERSION))) {
            ret = EINVAL;
            goto cfg_close;
        }
        memset((void *)response, 0, length);
    }
    else {
        ret = EINVAL;
        goto cfg_close;
    }
    out = response;
    if(*in != MKIO_MAGIC) {
        dprintf("%s:%d: request magic error, magic = 0x%02x\n",__FUNCTION__,__LINE__,*in);
        err++;
    }
    *out++ = *in++; /* magic */
    *out++ = *in++; /* tag */
    mode = *in; /* cmd */
    *out++ = *in++;
    if(mode > MKIO_CMD_BC_ID_OPTIONS) {
        dprintf("%s:%d: request cmd error, cmd = 0x%02x\n",__FUNCTION__,__LINE__,mode);
        err++;
    }
    ch_desc->flags = *in++;
    *out++ = 0; /* flag */
    lrequest -= sizeof(COMMAND_HDR);
    length = sizeof(COMMAND_HDR);
    *((uint16_t *)out) = DRV_MAJOR_VERSION;
    out += sizeof(uint16_t);
    *((uint16_t *)out) = DRV_MINOR_VERSION;
    out += sizeof(uint16_t);
    *((uint16_t *)out) = 0;
    out += sizeof(uint16_t);
    *((uint16_t *)out) = ch_desc->tvk.version;
    length += sizeof(DRV_VERSION);
    if(lrequest < 0)
        err++;
    if(err)
        goto cfg_close;

    switch(mode) {

    case MKIO_CMD_BC:
        dprintf("request MKIO_CMD_BC\n");
        if(mkioStop(desc)) {
            err++;
            goto cfg_close;
        }
        offset = mkioGetIdTable(desc, in);
        NEXT_PARAM(in, lrequest, offset, err);
        offset = mkioGetFrameTable(desc, in);
        NEXT_PARAM(in, lrequest, offset, err);
        offset = mkioGetOptions(desc, in);
        NEXT_PARAM(in, lrequest, offset, err);
        if((ch_desc->num_ids == 0)||(ch_desc->frame_table.num == 0))
            break;
        ch_desc->mode = MKIO_MODE_BC;
        if(mkioStart(desc)) {
            err++;
            goto cfg_close;
        }
        break;

    case MKIO_CMD_BC_PARAMS:
        dprintf("request MKIO_CMD_BC_PARAMS\n");
        if(ch_desc->mode != MKIO_MODE_BC){
            ((COMMAND_HDR *)response)->flags = MKIO_FLAG_MODE_ERROR;
            err++;
            goto cfg_close;
        }
        if(mkioPause(desc)){
            err++;
            goto cfg_close;
        }
        offset = mkioGetOptions(desc, in);
        NEXT_PARAM(in, lrequest, offset, err);
        pthread_mutex_lock(&ch_desc->hw_mutex);
        data16 = in16(ch_desc->reg + TA_REG_MODE2);
        data16 &= ~MKIO_BC_OPT_MASK;
        data16 |= ch_desc->options & MKIO_BC_OPT_MASK;
        out16(ch_desc->reg + TA_REG_MODE2, data16);
        pthread_mutex_unlock(&ch_desc->hw_mutex);
        if(mkioContinue(desc)) {
            err++;
            goto cfg_close;
        }
        break;

    case MKIO_CMD_BC_ID_OPTIONS:
        dprintf("request MKIO_CMD_BC_ID_OPTIONS\n");
        if(ch_desc->mode != MKIO_MODE_BC){
            ((COMMAND_HDR *)response)->flags = MKIO_FLAG_MODE_ERROR;
            err++;
            goto cfg_close;
        }
        pthread_mutex_lock(&ch_desc->request_mutex);
        offset = mkioGetIdOptions(desc, in);
        pthread_mutex_unlock(&ch_desc->request_mutex);
        dprintf("%s:%d lrequest=%d offset=%d\n",__FUNCTION__,__LINE__,lrequest,offset);
        NEXT_PARAM(in, lrequest, offset, err);
        break;

    case MKIO_CMD_RT:
        dprintf("request MKIO_CMD_RT\n");
        if(mkioStop(desc)) {
            err++;
            wprintf("%s:%d mkio Stop error\n",__FUNCTION__,__LINE__);
            goto cfg_close;
        }
        ch_desc->rt_addr = *(uint16_t *)in & 0x001F;
        offset = sizeof(uint16_t);
        NEXT_PARAM(in, lrequest, offset, err);
        offset = mkioGetOptions(desc, in);
        NEXT_PARAM(in, lrequest, offset, err);
        ch_desc->mode = MKIO_MODE_RT;
        if(mkioStart(desc)) {
            wprintf("%s:%d mkio Start error\n",__FUNCTION__,__LINE__);
            err++;
            goto cfg_close;
        }
        break;

    case MKIO_CMD_RT_BMM:
        dprintf("request MKIO_CMD_BMM\n");
        if(mkioStop(desc)) {
            err++;
            wprintf("%s:%d mkio Stop error\n",__FUNCTION__,__LINE__);
            goto cfg_close;
        }
        ch_desc->rt_addr = *(uint16_t *)in & 0x001F;
        offset = sizeof(uint16_t);
        NEXT_PARAM(in, lrequest, sizeof(uint16_t), err);
        offset = mkioGetOptions(desc, in);
        NEXT_PARAM(in, lrequest, offset, err);
        ch_desc->mode = MKIO_MODE_RT_BMM;
        if(mkioStart(desc)) {
            err++;
            goto cfg_close;
        }
        break;

    case MKIO_CMD_RT_BMM_PARAMS:
        dprintf("request MKIO_CMD_RT_PARAMS\n");
        if((ch_desc->mode != MKIO_MODE_RT)&&(ch_desc->mode != MKIO_MODE_RT_BMM)){
            ((COMMAND_HDR *)response)->flags = MKIO_FLAG_MODE_ERROR;
            err++;
            goto cfg_close;
        }
        offset = mkioGetOptions(desc, in);
        NEXT_PARAM(in, lrequest, offset, err);
        pthread_mutex_lock(&ch_desc->hw_mutex);
        data16 = in16(ch_desc->reg + TA_REG_MODE2);
        data16 &= ~MKIO_RT_OPT_MASK;
        data16 |= ch_desc->options & MKIO_RT_OPT_MASK;
        out16(ch_desc->reg + TA_REG_MODE2, data16);
        pthread_mutex_unlock(&ch_desc->hw_mutex);
        break;

    case MKIO_CMD_STOP:
        dprintf("request MKIO_CMD_STOP\n");
        if(mkioStop(desc)){
            err++;
            goto cfg_close;
        }
        break;

    case MKIO_CMD_SHUTDOWN:
        dprintf("request MKIO_CMD_SHUTDOWN\n");
        if(mkioShutdown(desc)){
            err++;
            goto cfg_close;
        }
        break;

    default:
        dprintf("request MKIO_CMD_UNKNOWN\n");
        ((COMMAND_HDR *)response)->flags = MKIO_FLAG_MODE_ERROR;
        lrequest = 0;
    }

    if(lrequest != 0)
        err++;

cfg_close:
    if(ret == EOK) {
        ret = unlock_in_queue(ch_desc->port_desc_table[ch_desc->cfg_response_port_index]->queue, length);
    }
    else {
        unlock_in_queue(ch_desc->port_desc_table[ch_desc->cfg_response_port_index]->queue, length);
        ch_desc->tvk.bist |= TA_MODE_FAIL;
    }

cfg_exit:
    if(err) {
        ((COMMAND_HDR *)response)->flags |= MKIO_FLAG_ERROR;
        ch_desc->tvk.bist |= TA_MODE_FAIL;
        mkioStop(desc);
    }

    return ret;
}

static int32_t rt_send_periodic_data_request(
    void *desc,
    void *port,
    void *request,
    int32_t *len,
    uint64_t *time,
    uint64_t timeout)
{
    int32_t size, qlen, err, l;
    MKIO_RT_SEND_DATA *pr;
    void *pq;
    PortDescType *port_desc = NULL;

    if((!desc)||(!port)||(!request)||(!len)||(*len <= 0))
        return EINVAL;

    port_desc = (PortDescType *)port;
    if((err = lock_in_queue(port_desc->queue, &pq, &qlen, NULL, 0)) == 0) {
        pr = (MKIO_RT_SEND_DATA *)request;
        size = MIL1553_MAX_MSG_WORDS * sizeof(uint16_t);
        l = *len;
    	if(l > (qlen + (MIL1553_MAX_SUB_ADDR - 2) * sizeof(uint16_t))) {
            unlock_in_queue(port_desc->queue, qlen);
    		return EPROTO;
    	}

        while(l >= sizeof(MKIO_RT_SEND_DATA)) {
        	if(pr->sa == 0)
        		out16mem(desc, MIL1553_MAX_SUB_ADDR << 6, pr->data[0]);  /* TX VECTOR */
        	else if (pr->sa == (MIL1553_MAX_SUB_ADDR - 1))
        		out16mem(desc, (2*MIL1553_MAX_SUB_ADDR - 1) << 6, pr->data[0]);  /* TX BIT */
        	else if((pr->sa > 0)&&(pr->sa < 31))
        	    memcpy(pq + ((pr->sa - 1) * size), pr->data, size);
        	pr++;
        	l -= sizeof(MKIO_RT_SEND_DATA);
        }

        unlock_in_queue(port_desc->queue, qlen);
    }

    return err;
}

static int32_t tvk_request(
    void *desc,
    void *port,
    void *request,
    int32_t *len,
    uint64_t *time,
    uint64_t timeout)
{
    TA_DESC_T *ch_desc;

    if((!desc)||(!request)||(!len))
        return EINVAL;

    if(*len < sizeof(PCIDRV_TVK_DATA_TYPE))
        return EINVAL;

    ch_desc = (TA_DESC_T *)desc;
    ch_desc->tvk.state = ch_desc->state;
    memcpy(request, (void *)&ch_desc->tvk, sizeof(PCIDRV_TVK_DATA_TYPE));

    if(ch_desc->tvk.validity)
        ch_desc->tvk.validity = 0;
    else
    	return ENODATA;

    return EOK;
}

int32_t setup_port(
    void *desc,
    PortDescType *port,
    int32_t port_index
    )
{
	int32_t i;
    uint32_t length, subindex;
    TA_DESC_T *ch_desc;

    if((!port)||(!desc)||(port_index < 0)||(port_index > MAX_PORTS_NUMBER))
        return (-1);

    ch_desc = (TA_DESC_T *)desc;
    switch(port->type) {
    case SAMPLING:
        length = strlen(BC_IN_PORT_NAME);
        if(strncmp(port->s.name, BC_IN_PORT_NAME, length) == 0) {
            subindex = atoi(&port->s.name[length]);
            if((subindex < BC_IN_PORT_MAX_NUMBER)&&(subindex >= 0)) {
            	if(ch_desc->bc_in_port_index[subindex] == (-1)) {
                    ch_desc->bc_in_port_index[subindex] = port_index;
                    port->port_update_func = out_queue;
            	}
            	else if(port->s.dir == SOURCE) {
            		wprintf("%s:%d Port %s more than once created.\n", __FUNCTION__,__LINE__, port->s.name);
            		return (-1);
            	}
                return 0;
            }
            else return (-1);
        }
        length = strlen(RT_IN_PORT_NAME);
        if(strncmp(port->s.name, RT_IN_PORT_NAME, length) == 0) {
        	if(ch_desc->rt_in_port_index == (-1)) {
                ch_desc->rt_in_port_index = port_index;
                port->port_update_func = out_queue;
        	}
        	else if(port->s.dir == SOURCE) {
        		wprintf("%s:%d Port %s more than once created.\n", __FUNCTION__,__LINE__, port->s.name);
        		return (-1);
        	}
            return (0);
        }
        length = strlen(TVK_PORT_NAME);
        if(strncmp(port->s.name, TVK_PORT_NAME, length) == 0) {
        	if(ch_desc->tvk_port_index == (-1)) {
                ch_desc->tvk_port_index = port_index;
                port->port_update_func = tvk_request;
        	}
        	else if(port->s.dir == SOURCE) {
        		wprintf("%s:%d Port %s more than once created.\n", __FUNCTION__,__LINE__, port->s.name);
        		return (-1);
        	}
            return (0);
        }
        break;

    case QUEUING:
        length = strlen(MKIO_REQUEST_PORT_NAME);
        if(strncmp(port->q.name, MKIO_REQUEST_PORT_NAME, length) == 0) {
        	if(ch_desc->cfg_request_port_index != (-1)) {
        		wprintf("%s:%d Port %s more than once created.\n", __FUNCTION__,__LINE__, port->q.name);
        		return (-1);
        	}
            ch_desc->cfg_request_port_index = port_index;
            port->port_update_func = cfg_request;
            return (0);
        }
        length = strlen(MKIO_RESPONSE_PORT_NAME);
        if(strncmp(port->q.name, MKIO_RESPONSE_PORT_NAME, length) == 0) {
        	if(ch_desc->cfg_response_port_index != (-1)) {
        		wprintf("%s:%d Port %s more than once created.\n", __FUNCTION__,__LINE__, port->q.name);
        		return (-1);
        	}
            ch_desc->cfg_response_port_index = port_index;
            port->port_update_func = out_queue;
            return (0);
        }
        length = strlen(BC_STATUS_PORT_NAME);
        if(strncmp(port->q.name, BC_STATUS_PORT_NAME, length) == 0) {
        	if(ch_desc->bc_status_port_index != (-1)) {
        		wprintf("%s:%d Port %s more than once created.\n", __FUNCTION__,__LINE__, port->q.name);
        		return (-1);
        	}
            ch_desc->bc_status_port_index = port_index;
            port->port_update_func = out_queue;
            return (0);
        }
        length = strlen(BC_ASYNC_STATUS_PORT_NAME);
        if(strncmp(port->q.name, BC_ASYNC_STATUS_PORT_NAME, length) == 0) {
            subindex = atoi(&port->q.name[length]);
            if(subindex < BC_ASYNC_OUT_PORT_MAX_NUMBER) {
        	    if(ch_desc->bc_async_status_port_index[subindex] != (-1)) {
        		    wprintf("%s:%d Port %s[%d] more than once created.\n", __FUNCTION__,__LINE__, port->q.name, subindex);
        		    return (-1);
        	    }
                ch_desc->bc_async_status_port_index[subindex] = port_index;
                port->port_update_func = out_queue;
                return subindex;
            }
            else return (-1);
        }
        length = strlen(BC_PERIODIC_OUT_PORT_NAME);
        if(strncmp(port->q.name, BC_PERIODIC_OUT_PORT_NAME, length) == 0) {
            subindex = atoi(&port->q.name[length]);
            if(subindex < BC_PERIODIC_OUT_PORT_MAX_NUMBER) {
            	if(ch_desc->bc_periodic_out_port_index[subindex] != (-1)) {
            		wprintf("%s:%d Port %s[%d] more than once created.\n", __FUNCTION__,__LINE__, port->q.name, subindex);
            		return (-1);
            	}
                ch_desc->bc_periodic_out_port_index[subindex] = port_index;
                port->port_update_func = in_queue;
                return subindex;
            }
            else return (-1);
        }
        length = strlen(BC_ASYNC_OUT_PORT_NAME);
        if(strncmp(port->q.name, BC_ASYNC_OUT_PORT_NAME, length) == 0) {
            subindex = atoi(&port->q.name[length]);
            if(subindex < BC_ASYNC_OUT_PORT_MAX_NUMBER) {
            	if(ch_desc->bc_async_out_port_index[subindex] != (-1)) {
            		wprintf("%s:%d Port %s[%d] more than once created.\n", __FUNCTION__,__LINE__, port->q.name, subindex);
            		return (-1);
            	}
                ch_desc->bc_async_out_port_index[subindex] = port_index;
                port->port_update_func = in_queue;
                return subindex;
            }
            else return (-1);
        }
        length = strlen(RT_PERIODIC_OUT_PORT_NAME);
        if(strncmp(port->q.name, RT_PERIODIC_OUT_PORT_NAME, length) == 0) {
            subindex = atoi(&port->q.name[length]);
            if(subindex < RT_PERIODIC_OUT_PORT_MAX_NUMBER) {
            	if(ch_desc->rt_periodic_out_port_index[subindex] != (-1)) {
            		wprintf("%s:%d Port %s[%d] more than once created.\n", __FUNCTION__,__LINE__, port->q.name, subindex);
            		return (-1);
            	}
            	for(i = 0; i < RT_PERIODIC_OUT_PORT_MAX_NUMBER; i++) {
            		if(ch_desc->rt_periodic_out_port_index[i] != (-1)) {
            			port->queue = ch_desc->port_desc_table[ch_desc->rt_periodic_out_port_index[i]]->queue;
            			break;
            		}
            	}
            	ch_desc->rt_periodic_out_port_index[subindex] = port_index;
                port->q.max_nb = 1;
                port->q.max_size = (MIL1553_MAX_SUB_ADDR - 2) * MIL1553_MAX_MSG_WORDS * sizeof(uint16_t);
                if(port->queue == NULL) {
                	if(create_queue(port->q.max_size, port->q.max_nb, QUEUE_OPTION_SAMPLING, &port->queue))
                		return (-1);
                }
                port->port_update_func = rt_send_periodic_data_request;
                return subindex;
            }
            else return (-1);
        }
        length = strlen(BM_IN_PORT_NAME);
        if(strncmp(port->q.name, BM_IN_PORT_NAME, length) == 0) {
        	if(ch_desc->bm_in_port_index != (-1)) {
        		wprintf("%s:%d Port %s more than once created.\n", __FUNCTION__,__LINE__, port->q.name);
        		return (-1);
        	}
            ch_desc->bm_in_port_index = port_index;
            port->port_update_func = out_queue;
            return (0);
        }
    }

    return (-1);
}
