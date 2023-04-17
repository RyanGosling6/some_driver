
#include <errno.h>
#include <hw/pci.h>
#include <stdlib.h>
#include <stddef.h>
#include <tatvk.h>
#include "tadrv.h"

static uint16_t get_version(TA_DESC_T *desc)
{
    int32_t i;
    uint16_t ver;
    uint16_t word;

    out16(desc->reg + TA_REG_MODE1, 4);
    for(i = 0, ver = 0; i < 16; i++) {
        word = in16(desc->reg + TA_REG_IRQ);
        if(word & RIRQ_DEVICE_VERSION)
            ver |= 1<<i;
    }

    return ver;
}

static int32_t test_pci_config_space(TA_DESC_T *desc)
{
    uint16_t word16;
    uint32_t word32;

    pci_read_config16(desc->tvk.inf.BusNumber, desc->tvk.inf.DevFunc, offsetof(struct _pci_config_regs, Vendor_ID), 1, &word16);
    if(word16 != PLX_VID)
        return TA_TEST_CONFIG_FAIL;
    pci_read_config16(desc->tvk.inf.BusNumber, desc->tvk.inf.DevFunc, offsetof(struct _pci_config_regs, Device_ID), 1, &word16);
    if(word16 != PLX_LOCAL_BUS_DID)
        return TA_TEST_CONFIG_FAIL;
    pci_read_config16(desc->tvk.inf.BusNumber, desc->tvk.inf.DevFunc, offsetof(struct _pci_config_regs, Command), 1, &word16);
    if((word16 & 3) != 3 )
        return TA_TEST_CONFIG_FAIL;
    pci_read_config16(desc->tvk.inf.BusNumber, desc->tvk.inf.DevFunc, offsetof(struct _pci_config_regs, Status), 1, &word16);
    if(word16 & 0xf900)
       return TA_TEST_CONFIG_FAIL;
    pci_read_config32(desc->tvk.inf.BusNumber, desc->tvk.inf.DevFunc, offsetof(struct _pci_config_regs, Revision_ID), 1, &word32);
    word32 >>= 8;
    if(word32 != desc->tvk.inf.Class)
        return TA_TEST_CONFIG_FAIL;
    pci_read_config16(desc->tvk.inf.BusNumber, desc->tvk.inf.DevFunc, offsetof(struct _pci_config_regs, Header_Type), 1, &word16);
    if(word16 != 0)
        return TA_TEST_CONFIG_FAIL;

    return EOK;
}

#if 0
static int32_t test_memory(TA_DESC_T *desc)
{
	int32_t i, j, k, block;
    uint16_t data[64];
    uint16_t tdata[3] = {0xAAAA, 0x5555, 0};
    
    for(i = 0; i < 3; i++) {
        for(j = 0; j < TA_BLOCK_SIZE; j++) {
            if((i == 2)|| (j & 1))
                data[j] = tdata[i];
            else
                data[j] = ~tdata[i];
        }
        for(j = 0; j < TA_MAX_BLOCKS; j++) {
            block = j<<6;
            out16block(desc, (void *)data, TA_BLOCK_SIZE, block);
        }
        for(j = 0; j < TA_MAX_BLOCKS; j++) {
            block = j<<6;
            in16block(desc, (void *)data, TA_BLOCK_SIZE, block);
            for(k = 0; k < TA_BLOCK_SIZE; k++)
                if(data[k] != tdata[i])
                    return TA_TEST_MEMORY_FAIL;
        }
    }

    return EOK;
}
#else
static int32_t test_memory(TA_DESC_T *desc)
{
	register uint32_t block;
	register uint32_t addr;
	uint16_t buf[64];

	srand(1);
	for (block = 0; block < TA_MAX_BLOCKS; block++){
		for (addr = 0; addr < TA_BLOCK_SIZE; addr++)
			buf[addr] = (uint16_t)((uint32_t)rand() + (uint32_t)rand());
		out16block(desc, (void *)buf, TA_BLOCK_SIZE, block<<6);
	}

	srand(1);
	for (block = 0; block < TA_MAX_BLOCKS; block++){
        in16block(desc, (void *)buf, TA_BLOCK_SIZE, block<<6);
        for (addr = 0; addr < TA_BLOCK_SIZE; addr++)
        	if (buf[addr] != (uint16_t)((uint32_t)rand() + (uint32_t)rand()))
        		return TA_TEST_MEMORY_FAIL;
	}

	return EOK;
}
#endif

void tvk_post(void *desc)
{
	TA_DESC_T *ch_desc;

	if(!desc)
		return;
	ch_desc = (TA_DESC_T *)desc;
	ch_desc->tvk.version = (int32_t)get_version(ch_desc);
    if(ch_desc->tvk.version != 19)
    	ch_desc->tvk.post |= TA_GET_VERSION_FAIL;
    ch_desc->tvk.post |= test_memory(ch_desc);
    ch_desc->tvk.post |= test_pci_config_space(ch_desc);
    ch_desc->tvk.validity++;
}

void tvk_bist(void *desc)
{
	TA_DESC_T *ch_desc;

	if(!desc)
		return;
	ch_desc = (TA_DESC_T *)desc;
	ch_desc->tvk.bist |= test_pci_config_space(ch_desc);

	if(ch_desc->state == DRV_START_STATE) {
		if(ch_desc->tvk.int_count == ch_desc->int_count_old) {
			ch_desc->int_count_wait++;
			if(ch_desc->int_count_wait > TA_CYCLE_WAIT_MAX)
				ch_desc->tvk.bist |= TA_INTERRUPT_FAIL;
		}
		else {
			ch_desc->int_count_old = ch_desc->tvk.int_count;
			ch_desc->int_count_wait = 0;
		}
		if(ch_desc->mode == MKIO_MODE_BC){
			if(ch_desc->mkio_count == ch_desc->mkio_count_old) {
				ch_desc->mkio_count_wait++;
				if(ch_desc->mkio_count_wait > TA_CYCLE_WAIT_MAX) {
					if(!(ch_desc->tvk.bist & TA_INTERRUPT_FAIL))
						ch_desc->tvk.bist |= TA_TIMER_FAIL;
				}
			}
		}
	}
	else {
		ch_desc->int_count_old = ch_desc->tvk.int_count;
		ch_desc->int_count_wait = 0;
		ch_desc->mkio_count_old = ch_desc->mkio_count;
		ch_desc->mkio_count_wait = 0;
	}
	ch_desc->tvk.validity++;
	ch_desc->tvk.pass_count++;
}

