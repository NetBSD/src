/* Configuration for GCC for ARM running NetBSD as host.  */

#include <arm/xm-arm.h>

#ifndef SYS_SIGLIST_DECLARED
/* XXX old def (w/o 1) broke w/NetBSD's configure-gen'd(?) config.h -- cgd */
#define SYS_SIGLIST_DECLARED 1
#endif
