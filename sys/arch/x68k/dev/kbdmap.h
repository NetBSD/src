/*	$NetBSD: kbdmap.h,v 1.3 2004/05/12 14:25:08 minoura Exp $	*/

#include <machine/kbdmap.h>

#ifdef _KERNEL
/* XXX: ITE interface */
extern struct kbdmap kbdmap, ascii_kbdmap;
extern unsigned char acctable[KBD_NUM_ACC][64];
#endif
