/* configuration information for base-line code */

#include "../include/mon.h"

#ifndef GENASSYM		/* XXX */
#define printf mon_printf
#endif


#define machdep_exec_override(x,y) 1
#define machdep_exec(x,y) sunos_exec(x,y)
#define machdep_netbsd_exec(x,y) 1

#define COMMON_ETHERADDR

#define FIXED_LOAD_ADDR 0x4000
#define BOOT_LOAD_ADDR 0x240000

caddr_t dvma_malloc __P((unsigned int));
