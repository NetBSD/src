/* $NetBSD: machdep.h,v 1.1 2002/01/05 22:41:48 chris Exp $ */

#ifndef _ARM32_BOOT_MACHDEP_H_
#define _ARM32_BOOT_MACHDEP_H_

/* misc prototypes used by the many arm machdeps */
void halt __P((void));
vsize_t map_chunk __P((vaddr_t,	vaddr_t, vaddr_t, paddr_t, vsize_t, u_int,
	u_int));
void parse_mi_bootargs __P((char *));
void data_abort_handler __P((trapframe_t *));
void prefetch_abort_handler __P((trapframe_t *));
void dumpsys	__P((void));

#endif
