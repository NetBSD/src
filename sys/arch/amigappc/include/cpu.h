#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_lockdebug.h"
#endif

#include <machine/frame.h>
#include <machine/psl.h>
#include <machine/intr.h>

#include <sys/sched.h>
struct cpu_info {
	struct schedstate_percpu ci_schedstate; /* scheduler state */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif
};

#ifdef _KERNEL
extern struct cpu_info cpu_info_store;

#define	curcpu()			(&cpu_info_store)

u_long	clkread	__P((void));
void	physaccess	__P((caddr_t, caddr_t, int, int));

#endif /* _KERNEL */

/* ADAM: taken from macppc/cpu.h */
#define CLKF_USERMODE(frame)    (((frame)->srr1 & PSL_PR) != 0)
#define CLKF_BASEPRI(frame)     ((frame)->pri == 0)
#define CLKF_PC(frame)          ((frame)->srr0)
#define CLKF_INTR(frame)        ((frame)->depth > 0)

#define cpu_swapout(p)
#define cpu_wait(p)
#define cpu_number()            0

extern void delay __P((unsigned));
#define DELAY(n)                delay(n)

extern __volatile int want_resched;
extern __volatile int astpending;

#define need_resched()          (want_resched = 1, astpending = 1)
#define need_proftick(p)        ((p)->p_flag |= P_OWEUPC, astpending = 1)
#define signotify(p)            (astpending = 1)

extern char bootpath[];

#if defined(_KERNEL) || defined(_STANDALONE)
#define CACHELINESIZE   32
#endif

/* ADAM: commented out to avoid CTL_MACHDEP_NAMES redefiniton (see below) */
/*#include <powerpc/cpu.h>*/

/* end of ADAM */


/* ADAM: maybe we will need this??? */
/* values for machineid (happen to be AFF_* settings of AttnFlags) */
/*
#define AMIGA_68020	(1L<<1)
#define AMIGA_68030	(1L<<2)
#define AMIGA_68040	(1L<<3)
#define AMIGA_68881	(1L<<4)
#define AMIGA_68882	(1L<<5)
#define	AMIGA_FPU40	(1L<<6)
#define AMIGA_68060	(1L<<7)
*/

#ifdef _KERNEL
int machineid;
#endif

/* ADAM: copied from powerpc/cpu.h */
#ifndef _POWERPC_CPU_H_
#define _POWERPC_CPU_H_

extern void __syncicache __P((void *, int));

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CACHELINE   1
#define CPU_MAXID       2

#endif  /* _POWERPC_CPU_H_ */

/* ADAM: copied from amiga/cpu.h */
#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
}

#ifdef _KERNEL
/*
 * Prototypes from amiga_init.c
 */
void    *alloc_z2mem __P((long));

/*
 * Prototypes from autoconf.c
 */
int     is_a1200 __P((void));
int     is_a3000 __P((void));
int     is_a4000 __P((void));
#endif

#endif /* !_MACHINE_CPU_H_ */
