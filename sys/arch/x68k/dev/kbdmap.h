/*	$NetBSD: kbdmap.h,v 1.4 2005/12/11 12:19:37 christos Exp $	*/

#include <machine/kbdmap.h>

#ifdef _KERNEL
/* XXX: ITE interface */
extern struct kbdmap kbdmap, ascii_kbdmap;
extern unsigned char acctable[KBD_NUM_ACC][64];
#endif
