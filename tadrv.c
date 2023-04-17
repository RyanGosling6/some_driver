
#include <sys/neutrino.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <tatvk.h>
#include "tadrv.h"
#include <io_func.h>

extern int port_devctl(resmgr_context_t *ctp, io_devctl_t *msg, iofunc_ocb_t *ocb);
extern void *mkioThread(void *desc);

#ifdef TA_DEBUG
extern void print_port(TA_DESC_T *desc);
#endif

static void main_free(TA_DESC_T **desc)
{
    int32_t i, j;

    for(i = 0; i < NTA; i++){
        if(desc[i]) {
            if(desc[i]->msg_tid) pthread_kill(desc[i]->msg_tid, SIGKILL);
            if(desc[i]->mkio_tid) pthread_kill(desc[i]->mkio_tid, SIGKILL);
            if(desc[i]->tvk_tid) pthread_kill(desc[i]->tvk_tid, SIGKILL);
            for(j = 0; j < MAX_PORTS_NUMBER; j++)
            	if(desc[i]->port_desc_table[j]) free(desc[i]->port_desc_table[j]);
            	else break;
            free(desc[i]);
        }
    }
}

static int32_t check_drv_instance(void)
{
    int32_t i, hdl;
    char drv_name[10];

    for(i = 0; i < NTA; i++) {
        sprintf(drv_name,"/dev/%s%d", TA_DRV_NAME, i);
        if((hdl = open(drv_name, O_RDONLY))!= -1) {
            close(hdl);
            return ENOTUNIQ;
        }
    }

    return (EOK);
}

static void *msgThread(void *desc)
{
    char drv_name[10];
    dispatch_t *dpp;
    resmgr_attr_t rattr;
    ext_iofunc_attr_t ioattr;
    resmgr_connect_funcs_t connect_funcs;
    resmgr_io_funcs_t io_funcs;
    dispatch_context_t *ctp;
    TA_DESC_T *ch_desc = (TA_DESC_T *)desc;

    ThreadCtl(_NTO_TCTL_IO, 0);
    out16(ch_desc->reg + TA_REG_RESET, 0);

    /* initialize TA resource manager */
    dpp = dispatch_create ();
    if (dpp == NULL) {
        eprintf("%s:%d: resource manager couldn't dispatch create of /dev/ta%d.\n",
               __FUNCTION__,__LINE__, ch_desc->ch);
        ch_desc->status = MKIO_FAILURE;
        return NULL;
    }
    memset(&rattr, 0, sizeof(resmgr_attr_t));
    rattr.nparts_max = 1;
    rattr.msg_max_size = DRV_MAX_MSG_SIZE;

    iofunc_attr_init((iofunc_attr_t*) &ioattr, S_IFNAM | 0666, 0, 0);
    ioattr.driver_desc = (void *)ch_desc;
    ioattr.ports_desc_table = (void **)&ch_desc->port_desc_table[0];
    ioattr.num_ports = MAX_PORTS_NUMBER;

    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS, &io_funcs);
    io_funcs.devctl = port_devctl;

    ch_desc->state = DRV_CFG_STATE;

    sprintf(drv_name,"/dev/%s%d", TA_DRV_NAME, ch_desc->ch);
    if(resmgr_attach(dpp,  &rattr,  drv_name,  _FTYPE_ANY, 0, &connect_funcs,
            &io_funcs, (iofunc_attr_t*)&ioattr) == -1) {
        eprintf("%s:%d: resource manager attach %s fail.\n",__FUNCTION__,__LINE__, drv_name);
        ch_desc->status = MKIO_FAILURE;
        return NULL;
    }

    if((ctp = dispatch_context_alloc(dpp)) == NULL) {
        eprintf("%s:%d: resource manager couldn't create context of %s.\n",__FUNCTION__,__LINE__, drv_name);
        ch_desc->status = MKIO_FAILURE;
        return NULL;
    }

    if(sem_init(&ch_desc->stop_sem, 0, 0)) {
        eprintf("%s:%d: create stop semaphore of %s fail.\n",__FUNCTION__,__LINE__, drv_name);
        ch_desc->status = MKIO_FAILURE;
        return NULL;
    }

    if(sem_init(&ch_desc->pause_sem, 0, 0)) {
        eprintf("%s:%d: create pause semaphore of % fail.\n",__FUNCTION__,__LINE__, drv_name);
        ch_desc->status = MKIO_FAILURE;
        return NULL;
    }

    if(sem_init(&ch_desc->int_sem, 0, 0)) {
        eprintf("%s:%d: create IRQ semaphore of % fail.\n",__FUNCTION__,__LINE__, drv_name);
        ch_desc->status = MKIO_FAILURE;
        return NULL;
    }

    if(pthread_mutex_init(&ch_desc->hw_mutex, 0)) {
        eprintf("%s:%d: create hardware mutex of % fail.\n",__FUNCTION__,__LINE__, drv_name);
        ch_desc->status = MKIO_FAILURE;
        return NULL;
    }

    if(pthread_mutex_init(&ch_desc->async_mutex, 0)) {
        eprintf("%s:%d: create async mutex of % fail.\n",__FUNCTION__,__LINE__, drv_name);
        ch_desc->status = MKIO_FAILURE;
        return NULL;
    }

    if(pthread_mutex_init(&ch_desc->request_mutex, 0)) {
        eprintf("%s:%d: create request mutex of % fail.\n",__FUNCTION__,__LINE__, drv_name);
        ch_desc->status = MKIO_FAILURE;
        return NULL;
    }

    dprintf("%s: start driver\n", drv_name);
    ch_desc->status = MKIO_READY;

    while(ch_desc->state != DRV_SHUTDOWN_STATE) {
       if((ctp = dispatch_block(ctp)) == NULL)
            eprintf("%s:%d: dispatch block of %s error.\n",__FUNCTION__,__LINE__, drv_name);
        dispatch_handler(ctp);
    }

    dprintf("%s:%d: shutdown ch %d\n", __FUNCTION__,__LINE__, ch_desc->ch);

    return NULL;
}

static void *tvkThread(void *desc)
{
	while(((TA_DESC_T *)desc)->state != DRV_SHUTDOWN_STATE){
        tvk_bist(desc);
        usleep(5000000);
    }

    return NULL;
}


int32_t main(int argc, char *argv[]) {
    int32_t i, j, ret;
    int32_t phdl;
    void *hdl;
    struct pci_dev_info inf;
    uint16_t word16;
    struct sigevent event;
    uint64_t int_timeout = 1000000000;
    TA_DESC_T *desc[NTA];
    uint64_t hHiddenPort;
    uint64_t hPort[2];
    uint32_t post = 0;
    int32_t interrupt_id = 0;
    int32_t sem_value;
    pthread_attr_t attr;
    struct sched_param params;

    if(check_drv_instance()){
        wprintf("%s:%d: driver was installed early.\n",__FUNCTION__,__LINE__);
        return (EXIT_SUCCESS);
    }

    /* Connect to the PCI server */
    phdl = pci_attach( 0 );
    if( phdl == -1 ) {
        eprintf("%s:%d: unable to attach to PCI server\n",__FUNCTION__,__LINE__);
        post |= PCI_SERVER_FAIL;
        goto ta_system_init;
    }

    /* Initialize the pci_dev_info structure */
  	memset(&inf, 0, sizeof(inf));
    inf.VendorId = PLX_VID;
    inf.DeviceId = PLX_LOCAL_BUS_DID;
    hdl = pci_attach_device( NULL, 0, 0, &inf );
    if( hdl == NULL ) {
        pci_detach( phdl );
        eprintf("%s:%d: unable to locate TA adapter\n",__FUNCTION__,__LINE__);
        post |= TA_DEVICE_NOT_FOUND;
        goto ta_system_init;
    }

    if((inf.SubsystemId != TA_DID)||(inf.SubsystemVendorId != ELCUS_VID)) {
        post |= TA_DEVICE_NOT_FOUND;
        goto ta_system_init;
    }

    ThreadCtl(_NTO_TCTL_IO, 0);

    /* map physical device memory to process virtual memory */
    uint64_t hport = PCI_IO_ADDR(inf.CpuBaseAddress[1]);
    hHiddenPort = (uint64_t)mmap_device_io(inf.BaseAddressSize[1], hport);
    if(hHiddenPort == MAP_DEVICE_FAILED) {
        eprintf("map physical device I/O memory to process fail.\n");
        post |= TA_DEVICE_NOT_FOUND;
        goto ta_system_init;
    }
    hport = PCI_IO_ADDR(inf.CpuBaseAddress[2]);
    hPort[0] = (uint64_t)mmap_device_io(inf.BaseAddressSize[2], hport);
    if(hPort[0] == MAP_DEVICE_FAILED) {
        eprintf("map physical device I/O memory to process fail.\n");
        post |= TA_DEVICE_NOT_FOUND;
        goto ta_system_init;
    }
    dprintf("port0 handle at addr 0x%08x\n",hPort[0]);
    hport = PCI_IO_ADDR(inf.CpuBaseAddress[3]);
    hPort[1] = (uint64_t)mmap_device_io(inf.BaseAddressSize[3], hport);
    if(hPort[1] == MAP_DEVICE_FAILED) {
        eprintf("map physical device I/O memory to process fail.\n");
        post |= TA_DEVICE_NOT_FOUND;
        goto ta_system_init;
    }
    dprintf("port1 handle at addr 0x%08x\n",hPort[1]);

    word16 = in16(hHiddenPort + 0xf0);
    if((word16 & 0xff) != 0xc1) {
    	eprintf("Can not find TA1XMC4 device.");
    	post |= TA_DEVICE_NOT_FOUND;
    }
    word16 = in16(hHiddenPort + 0x68);
    out16(hHiddenPort + 0x68, word16 | 0x800); /*Enable INT*/
    word16 = in16(hHiddenPort + 0x6A);
    out16(hHiddenPort + 0x6A, word16 & 0xfffe); /*Disable INTout*/

ta_system_init:

    for(i=0; i < NTA; i++) {
        desc[i] = (TA_DESC_T *)malloc(sizeof(TA_DESC_T));
        if(desc[i] == NULL) {
            eprintf("%s:%d: create device descriptor fail.\n",__FUNCTION__,__LINE__);
            goto ta_fail;
        }
        memset((char *)desc[i], 0, sizeof(TA_DESC_T));
        desc[i]->ch = i;
        if(!post)
          desc[i]->reg = hPort[i>>1] + 32*(i&1);
#ifdef TA_DEBUG
        print_port(desc[i]);
#endif
        desc[i]->cfg_request_port_index = -1;
        desc[i]->cfg_response_port_index = -1;
        desc[i]->bc_status_port_index = -1;
        for(j = 0; j < BC_IN_PORT_MAX_NUMBER; j++)
            desc[i]->bc_in_port_index[j] = -1;
        for(j = 0; j < BC_PERIODIC_OUT_PORT_MAX_NUMBER; j++)
            desc[i]->bc_periodic_out_port_index[j] = -1;
        for(j = 0; j < BC_ASYNC_OUT_PORT_MAX_NUMBER; j++) {
            desc[i]->bc_async_out_port_index[j] = -1;
            desc[i]->bc_async_status_port_index[j] = -1;
        }
        desc[i]->rt_in_port_index = -1;
        for(j = 0; j < RT_PERIODIC_OUT_PORT_MAX_NUMBER; j++)
            desc[i]->rt_periodic_out_port_index[j] = -1;
        desc[i]->bm_in_port_index = -1;
        desc[i]->tvk_port_index = -1;
        desc[i]->timer_id = -1;
        desc[i]->tvk.inf = inf;
        desc[i]->tvk.version = DRV_MINOR_VERSION + (DRV_MAJOR_VERSION << 16);

        pthread_attr_init(&attr);
        sched_getparam(0, &params);
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if(ret == EOK) {
            params.sched_priority = MSG_THREAD_PRIORITY;
            ret = pthread_attr_setschedparam(&attr,&params);
        }
        if(ret) {
    	    eprintf("%s:%d msg thread attribute set fail: %s.\n",
    			__FUNCTION__,__LINE__, strerror(ret));
            post |= TA_DRIVER_INIT_FAIL;
        }
        if(ret = pthread_create(&desc[i]->msg_tid, &attr, msgThread, (void *)desc[i])) {
            eprintf("%s:%d: ta%d message thread create failed: %s\n",__FUNCTION__,__LINE__,i,strerror(ret));
            goto ta_fail;
        }
        while(!desc[i]->status)
            usleep(10);
        if(desc[i]->status == MKIO_FAILURE)
            goto ta_fail;
        desc[i]->tvk.post = post;
        if(!desc[i]->tvk.post) {
            tvk_post((void *)desc[i]);
            params.sched_priority = MKIO_THREAD_PRIORITY;
            if(ret = pthread_attr_setschedparam(&attr,&params)) {
    	        eprintf("%s:%d mkio thread attribute set fail: %s.\n",
    			    __FUNCTION__,__LINE__, strerror(ret));
                post |= TA_DRIVER_INIT_FAIL;
            }
            if(ret = pthread_create(&desc[i]->mkio_tid, &attr, mkioThread, (void *)desc[i])) {
                eprintf("%s:%d: ta%d mkio thread create failed: %s\n",__FUNCTION__,__LINE__,i,strerror(ret));
                desc[i]->tvk.post |= TA_DRIVER_INIT_FAIL;
            }
        }
        params.sched_priority = TVK_THREAD_PRIORITY;
        ret = pthread_attr_setschedparam(&attr,&params);
        if(ret) {
            eprintf("%s:%d tvk thread attribute set fail: %s.\n",
   			__FUNCTION__,__LINE__, strerror(ret));
            post |= TA_DRIVER_INIT_FAIL;
        }
        if(ret = pthread_create(&desc[i]->tvk_tid, &attr, tvkThread, (void *)desc[i])) {
            wprintf("%s:%d ta%d tvk thread create failed: %s.\n",__FUNCTION__,__LINE__,i,strerror(ret));
            desc[i]->tvk.post |= TA_DRIVER_INIT_FAIL;
        }
        else{
            desc[i]->tvk.validity++;
        }
    }

	sched_getparam(0, &params);
    params.sched_priority = MKIO_THREAD_PRIORITY;
	sched_setparam(0, &params);
//    	sched_setparam(1, &params);

	if(!post) {
        SIGEV_INTR_INIT(&event);
        interrupt_id = InterruptAttachEvent(inf.Irq, &event, 0);
        if(interrupt_id == -1){
            eprintf("%s:%d: interrupt attach failed.\n",__FUNCTION__,__LINE__);
            for(i = 0; i < NTA; i++)
                desc[i]->tvk.post |= TA_DRIVER_INIT_FAIL;
            post |= TA_DRIVER_INIT_FAIL;
        }
    }
    while(desc[3]->state != DRV_SHUTDOWN_STATE){
        if(!post) {
        	TimerTimeout(CLOCK_MONOTONIC,_NTO_TIMEOUT_INTR, &event, &int_timeout, NULL);
            if(ret = InterruptWait_r(0, NULL) != EOK) {
            	if(ret == ETIMEDOUT)
            		continue;
                wprintf("%s:%d: interrupt error : %s\n",__FUNCTION__,__LINE__,strerror(ret));
                for(i = 0; i < NTA; i++)
                    desc[i]->tvk.bist |= TA_INTERRUPT_FAIL;
            }
            for(i = 0; i < NTA; i++) {
                if(sem_getvalue(&desc[i]->int_sem, &sem_value) == 0) {
                    if(sem_value) continue;
                    word16 = in16(desc[i]->reg + TA_REG_RESET);
                    if(word16 & TAIRQ_EVENTS) {
                        word16 = in16(desc[i]->reg + TA_REG_MODE1);
                        word16 &= ~RMODE1_EN_IRQ;
                        out16(desc[i]->reg + TA_REG_MODE1, word16);
                        word16 = in16(desc[i]->reg + TA_REG_MODE2);
                        word16 &= ~RMODE2_BC_START;
                   	    out16(desc[i]->reg + TA_REG_MODE2, word16);
                        desc[i]->tvk.int_count++;
                        sem_post(&desc[i]->int_sem);
                    }                    
                }
                else {
                    wprintf("%s:%d: channel %d interrupt semaphore error \n",__FUNCTION__,__LINE__, i);
                }
            }
            if(InterruptUnmask(inf.Irq, interrupt_id) < 0) {
                for(i = 0; i < NTA; i++)
                    desc[i]->tvk.bist |= TA_INTERRUPT_FAIL;
            }
        }
        else {
            usleep(100000);
        }
    }

    InterruptDetach(interrupt_id);
    for(i = 0; i < NTA; i++)
        sem_post(&desc[i]->int_sem);
    usleep(1000000);

ta_fail:
    main_free(desc);
    return (EOK);
}
