/*	$OpenBSD: db_machdep.h,v 1.2 1997/03/21 00:48:48 niklas Exp $	*/
/*	$NetBSD: db_machdep.h,v 1.24.12.1 2017/12/03 11:36:37 jdolecek Exp $	*/

/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */

/*
 * Machine-dependent defines for new kernel debugger.
 */
#ifndef	_PPC_DB_MACHDEP_H_
#define	_PPC_DB_MACHDEP_H_

#include <uvm/uvm_prot.h>
#include <uvm/uvm_param.h>
#include <machine/trap.h>

#ifdef _KERNEL
#include "opt_ppcarch.h"
#endif

#define	DB_ELF_SYMBOLS

typedef	vaddr_t		db_addr_t;	/* address - unsigned */
#define	DDB_EXPR_FMT	"l"		/* expression is long */
typedef	long		db_expr_t;	/* expression - signed */
struct powerpc_saved_state {
	u_int32_t	r[32];		/* data registers */
	u_int32_t	iar;
	u_int32_t	msr;
	u_int32_t	lr;
	u_int32_t	ctr;
	u_int32_t	cr;
	u_int32_t	xer;
	u_int32_t	mq;
	u_int32_t	dear;
	u_int32_t	esr;
	u_int32_t	pid;
};
typedef struct powerpc_saved_state db_regs_t;
extern	db_regs_t	ddb_regs;		/* register state */
#define DDB_REGS	(&ddb_regs)

#define	PC_REGS(regs)	(*(db_addr_t *)&(regs)->iar)

#define	BKPT_ADDR(addr)	(addr)		/* breakpoint address */
#define	BKPT_ASM	"trap"		/* should match BKPT_INST */
#define	BKPT_INST	0x7fe00008	/* breakpoint instruction */
#define	BKPT_SIZE	(4)		/* size of breakpoint inst */
#define	BKPT_SET(inst, addr)	(BKPT_INST)

#ifndef PPC_IBM4XX
#define SR_SINGLESTEP	0x400
#define	db_clear_single_step(regs)	((regs)->msr &= ~SR_SINGLESTEP)
#define	db_set_single_step(regs)	((regs)->msr |=  SR_SINGLESTEP)
#else
#define	SOFTWARE_SSTEP
#endif

#define T_BREAKPOINT	0xffff
#define	IS_BREAKPOINT_TRAP(type, code)	((type) == T_BREAKPOINT)

#define T_WATCHPOINT	0xeeee
#ifdef T_WATCHPOINT
#define	IS_WATCHPOINT_TRAP(type, code)	((type) == T_WATCHPOINT)
#else
#define	IS_WATCHPOINT_TRAP(type, code)	0
#endif

#define	M_RTS		0xfc0007ff
#define I_RTS		0x4c000020
#define	I_BLRL		0x4c000021
#define M_BC		0xfc000001
#define I_BC		0x40000000
#define I_BCL		0x40000001
#define M_B		0xfc000001
#define I_B		0x48000000
#define I_BL		0x48000001
#define	M_BCTR		0xfc0007fe
#define	I_BCTR		0x4c000420
#define	I_BCTRL		0x4c000421
#define	M_RFI		0xfc0007fe
#define	I_RFI		0x4c000064

#define	inst_trap_return(ins)	(((ins)&M_RFI) == I_RFI)
#define	inst_return(ins)	(((ins)&M_RTS) == I_RTS)
#define	inst_call(ins)		(((ins)&M_BC  ) == I_BCL   || \
				 ((ins)&M_B   ) == I_BL    || \
				 ((ins)&M_BCTR) == I_BCTRL || \
				 ((ins)&M_RTS ) == I_BLRL )
#define	inst_branch(ins)	(((ins)&M_BC  ) == I_BC || \
				 ((ins)&M_B   ) == I_B  || \
				 ((ins)&M_BCTR) == I_BCTR )
#define	inst_unconditional_flow_transfer(ins)	\
				(((ins)&M_B   ) == I_B    || \
				 ((ins)&M_BCTR) == I_BCTR )
#define inst_load(ins)		0
#define inst_store(ins)		0
#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
#define next_instr_address(v, b) ((db_addr_t) ((b) ? (v) : ((v) + 4)))
extern db_addr_t branch_taken(int, db_addr_t, db_regs_t *);
#endif

/*
 * GDB's register array is:
 *  32 4-byte GPRs
 *  32 8-byte FPRs
 *   7 4-byte UISA special-purpose registers
 *  16 4-byte segment registers
 *  32 4-byte standard OEA special-purpose registers,
 * and up to 64 4-byte non-standard OES special-purpose registers.
 * GDB keeps some extra space, so the total size of the register array
 * they use is 880 bytes (gdb-5.0).
 * KGDB_NUMREGS 220
 */
/*
 * GDB's register array of gdb-6.0 is defined in
 * usr/src/gnu/dist/gdb6/gdb/regformats/reg-ppc.dat
 * GDB's register array is:
 *  32 4-byte GPRs
 *  32 8-byte FPRs
 *   7 4-byte UISA special-purpose registers: pc, ps, cr, lr, ctr, xer, fpscr
 * index of pc in array: 32 + 2*32 = 96
 * size 32 * 4 + 32 * 8 + 7 * 4 = 103 * 4 = 412 bytes
 * KGD_NUMREGS 103
 */
typedef long	kgdb_reg_t;
#define KGDB_PPC_PC_REG		96	/* first UISA SP register */
#define KGDB_PPC_MSR_REG	97
#define KGDB_PPC_CR_REG		98
#define KGDB_PPC_LR_REG		99
#define KGDB_PPC_CTR_REG	100
#define KGDB_PPC_XER_REG	101
#define KGDB_PPC_FPSCR_REG	102
#define KGDB_NUMREGS		103	/* Treat all registers as 4-byte */
#define KGDB_BUFLEN		(2*KGDB_NUMREGS*sizeof(kgdb_reg_t)+1)

#ifdef _KERNEL

void	kdb_kintr(void *);
int	kdb_trap(int, void *);

bool	ddb_running_on_this_cpu_p(void);
bool	ddb_running_on_any_cpu_p(void);
void	db_resume_others(void);

/*
 * We have machine-dependent commands.
 */
#define	DB_MACHINE_COMMANDS

#endif /* _KERNEL */

#endif	/* _PPC_DB_MACHDEP_H_ */
