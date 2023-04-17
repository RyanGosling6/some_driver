
 #ifndef TATVK_H_
#define TATVK_H_
#include <drvtvk.h>

/* INIT */
#define PCI_SERVER_FAIL             0x00000001
#define TA_DEVICE_NOT_FOUND         0x00000002
#define TA_DRIVER_INIT_FAIL         0x00000004

/* POST */
#define TA_TEST_MEMORY_FAIL         0x00000100
#define TA_TEST_CONFIG_FAIL         0x00000200
#define TA_GET_VERSION_FAIL         0x00000400

/* BIST SW*/
#define TA_MODE_FAIL                0x00010000
#define TA_INTERRUPT_FAIL           0x00020000
#define TA_TIMER_FAIL               0x00040000

/* BIST HW */
#define TA_CONFIG_FAIL              0x01000000
#define TA_CHANNEL_GEN              0x02000000

#endif /* TATVK_H_ */
