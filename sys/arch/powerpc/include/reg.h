/*	$NetBSD: reg.h,v 1.9 2003/02/08 20:03:22 matt Exp $	*/

#ifndef _POWERPC_REG_H_
#define _POWERPC_REG_H_

/*
 *  Register Usage according the SVR4 ABI for PPC.
 *
 *  Register	Usage
 *  r0		Volatile register which may be modified during function linkage
 *  r1		Stack fram pointer, always valid
 *  r2		System-reserved register
 *  r3-r4	Volatile registers used for parameter passing and return values
 *  r5-r10	Volatile registers used for parameter passing
 *  r11-r12	Volatile register which may be modified during function linkage
 *  r13		Small data area pointer register
 *  f0		Volatile register
 *  f1		Volatile registers used for parameter passing and return values
 *  f2-f8	Volatile registers used for parameter passing
 *  f9-f13	Volatile registers
 *
 *  [Start of callee-saved registers]
 *  r14-r30	Registers used for local variables
 *  r31		Used for local variable or "environent pointers"
 *  f14-f31	Registers used for local variables
 *
 *
 *  Register Usage according the ELF64 ABI (PowerOpen/AIX) for PPC.
 *
 *  Register	Usage
 *  r0		Volatile register which may be modified during function linkage
 *  r1		Stack fram pointer, always valid
 *  r2		TOC pointer
 *  r3		Volatile register used for parameter passing and return value
 *  r4-r10	Volatile registers used for parameter passing
 *  r11		Volatile register used in calls by pointer and as an
 *		environment pointr for languages which require one
 *  r12		Volatile register used for exception handling and glink code
 *  r13		Reserved for use as system thread ID
 *
 *  f0		Volatile register
 *  f1-f4	Volatile registers used for parameter passing and return values
 *  f5-f13	Volatile registers used for parameter passing

 *  [Start of callee-saved registers]
 *  r14-r31	Registers used for local variables
 *  f14-f31	Registers used for local variables
 *
 */

struct reg {				/* base registers */
	register_t fixreg[32];
	register_t lr;			/* Link Register */
	int cr;				/* Condition Register */
	int xer;			/* SPR 1 */
	register_t ctr;			/* Count Register */
	register_t pc;			/* Program Counter */
};

struct fpreg {				/* Floating Point registers */
	double fpreg[32];
	double fpscr;			/* Status and Control Register */
};

struct vreg {				/* Vector registers */
	u_int32_t vreg[32][4];
	register_t vrsave;		/* SPR 256 */
	register_t spare[2];		/* filler */
	register_t vscr;		/* Vector Status And Control Register */
};

#endif /* _POWERPC_REG_H_ */
