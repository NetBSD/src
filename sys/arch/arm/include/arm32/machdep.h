/* $NetBSD: machdep.h,v 1.2.2.2 2002/01/10 19:37:55 thorpej Exp $ */

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

/* 
 * note that we use void * as all the platforms have different ideas on what
 * the structure is
 */
u_int initarm __P((void *));

/* from arm/arm32/intr.c */
void dosoftints __P((void));
void set_spl_masks __P((void));
#ifdef DIAGNOSTIC
void dump_spl_masks __P((void));
#endif
#endif
