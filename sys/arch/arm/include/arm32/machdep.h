/* $NetBSD: machdep.h,v 1.8.60.1 2009/05/13 17:16:17 jym Exp $ */

#ifndef _ARM32_BOOT_MACHDEP_H_
#define _ARM32_BOOT_MACHDEP_H_

/* misc prototypes used by the many arm machdeps */
void halt(void);
void parse_mi_bootargs(char *);
void data_abort_handler(trapframe_t *);
void prefetch_abort_handler(trapframe_t *);
void undefinedinstruction_bounce(trapframe_t *);
void dumpsys(void);

/* 
 * note that we use void *as all the platforms have different ideas on what
 * the structure is
 */
u_int initarm(void *);

/* from arm/arm32/intr.c */
void dosoftints(void);
void set_spl_masks(void);
#ifdef DIAGNOSTIC
void dump_spl_masks(void);
#endif
#endif
