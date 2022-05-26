/*	$NetBSD: kbdmap.h,v 1.5 2022/05/26 14:28:56 tsutsui Exp $	*/

#include <machine/kbdmap.h>

#ifdef _KERNEL
/* XXX: ITE interface */
extern struct kbdmap kbdmap;
extern const struct kbdmap ascii_kbdmap;
extern unsigned char acctable[KBD_NUM_ACC][64];
#endif
