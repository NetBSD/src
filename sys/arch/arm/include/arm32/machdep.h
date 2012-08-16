/* $NetBSD: machdep.h,v 1.11 2012/08/16 18:22:40 matt Exp $ */

#ifndef _ARM32_BOOT_MACHDEP_H_
#define _ARM32_BOOT_MACHDEP_H_

/* Define various stack sizes in pages */ 
#ifndef IRQ_STACK_SIZE
#define IRQ_STACK_SIZE	1
#endif
#ifndef ABT_STACK_SIZE
#define ABT_STACK_SIZE	1
#endif
#ifndef UND_STACK_SIZE
#ifdef IPKDB
#define UND_STACK_SIZE	2
#else
#define UND_STACK_SIZE	1
#endif
#endif
#ifndef FIQ_STACK_SIZE
#define FIQ_STACK_SIZE	1
#endif


extern void (*cpu_reset_address)(void);
extern paddr_t cpu_reset_address_paddr;

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
