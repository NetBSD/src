/*	$NetBSD: kbdmap.h,v 1.2.50.2 2004/09/18 14:42:25 skrll Exp $	*/

#include <machine/kbdmap.h>

#ifdef _KERNEL
/* XXX: ITE interface */
extern struct kbdmap kbdmap, ascii_kbdmap;
extern unsigned char acctable[KBD_NUM_ACC][64];
#endif
