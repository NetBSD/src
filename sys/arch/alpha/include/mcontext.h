
#ifndef _ALPHA_MCONTEXT_H_
#define _ALPHA_MCONTEXT_H_

/* In Digital Unix, mcontext_t is also struct sigcontext */

#include <machine/signal.h>

typedef struct sigcontext mcontext_t;

#if 0
typedef struct {
	/* This part matches the DU jmp_buf */
	long	sc_onstack;
	long	sc_mask;
	long	sc_pc;		/* pc at time of signal */
	long	sc_ps;		/* PSL to restore */
	long	sc_regs[32];	/* Integer registers */
	long	sc_ownedfp;	/* Was the FPU in use by this process? */
	long	sc_fpregs[32];	/* FP registers */
	unsigned long	sc_fpcr;	/* floating point control register */
	unsigned long	sc_fp_control;	/* software fpcr */
	/* End of jmp_buf compatibility */

	/* Reserved for kernel use */
	long	sc_reserved1;
	long	sc_kreserved1;
	long	sc_kreserved2;

	size_t	sc_ssize;	/* Stack size */
	caddr_t sc_sbase;	/* Stack start */

	unsigned long	sc_traparg_a0;	/* A0 arg to trap on exception */
	unsigned long	sc_traparg_a1;	/* A1 arg to trap on exception */
	unsigned long	sc_traparg_a2;	/* A2 arg to trap on exception */
	unsigned long	sc_fp_trap_pc;	/* Imprecise FP trap PC */
	unsigned long	sc_fp_trigger_sum;    /* Exception summary at trigger*/
	unsigned long	sc_fp_trigger_inst;   /* Instruction at trigger PC */
} mcontext_t;

#endif

#define _UC_MACHINE_PAD 1 /* XXX check */

#endif	/* !_ALPHA_MCONTEXT_H_ */
