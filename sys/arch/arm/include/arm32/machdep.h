/* $NetBSD: machdep.h,v 1.10 2012/07/29 00:07:10 matt Exp $ */

#ifndef _ARM32_BOOT_MACHDEP_H_
#define _ARM32_BOOT_MACHDEP_H_

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

extern char *booted_kernel;

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
