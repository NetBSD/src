/*	$NetBSD: kbdmap.h,v 1.2.50.1 2004/08/03 10:42:47 skrll Exp $	*/

#include <machine/kbdmap.h>

#ifdef _KERNEL
/* XXX: ITE interface */
extern struct kbdmap kbdmap, ascii_kbdmap;
extern unsigned char acctable[KBD_NUM_ACC][64];
#endif
