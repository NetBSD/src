/*	$NetBSD: kbdmap.h,v 1.2.50.3 2004/09/21 13:24:08 skrll Exp $	*/

#include <machine/kbdmap.h>

#ifdef _KERNEL
/* XXX: ITE interface */
extern struct kbdmap kbdmap, ascii_kbdmap;
extern unsigned char acctable[KBD_NUM_ACC][64];
#endif
