
#ifndef DRV_TVK_H_
#define DRV_TVK_H_

#include <stdint.h>
#include <hw/pci.h>

#define TVK_PORT_NAME       "tvk"
#define TVK_PORT_REFRESH    500000000
#define TVK_DETAIL_NUMBER   24

typedef struct
{
    uint32_t            state;
    uint32_t            post;
    uint32_t            bist;
    uint32_t            bist_detail[TVK_DETAIL_NUMBER];
    uint32_t            version;
    uint32_t            date;
    uint32_t            crc32;
    uint32_t            pass_count;
    uint32_t            int_count;
    int32_t             validity;
    struct pci_dev_info inf;
} PCIDRV_TVK_DATA_TYPE;

extern void tvk_post(void *desc);
extern void tvk_bist(void *desc);

#endif /* DRV_TVK_H_ */

