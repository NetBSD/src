/*	$NetBSD: db_machdep.h,v 1.18.6.1 2006/04/22 11:37:51 simonb Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1992 Helsinki University of Technology
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON AND HELSINKI UNIVERSITY OF TECHNOLOGY ALLOW FREE USE
 * OF THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * HELSINKI UNIVERSITY OF TECHNOLOGY DISCLAIM ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
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
 * HISTORY
 * 11-May-92  Tero Kivinen (kivinen) at Helsinki University of Technology
 *	Created.
 *
 */
/*
 * 	File: ns532/db_machdep.h
 *	Author: Tero Kivinen, Helsinki University of Technology 1992.
 *
 *	Machine-dependent defines for kernel debugger.
 *
 *   modified by Phil Nelson for inclusion in 532bsd.
 *
 */

#ifndef	_MACHINE_DB_MACHDEP_H_
#define	_MACHINE_DB_MACHDEP_H_

#include <uvm/uvm_prot.h>

#include <uvm/uvm_param.h>

#include <machine/reg.h>		/* For struct reg */
#include <machine/psl.h>
#include <machine/trap.h>

typedef	vaddr_t		db_addr_t;	/* address - unsigned */
typedef	long		db_expr_t;	/* expression - signed */

typedef struct trapframe db_regs_t;
db_regs_t  	ddb_regs;		/* register state */
#define	DDB_REGS	(&ddb_regs)

#define	PC_REGS(regs)	((db_addr_t)(regs)->tf_regs.r_pc)

#define	BKPT_ADDR(addr)	(addr)		/* breakpoint address */
#define	BKPT_INST	0xf2		/* breakpoint instruction */
#define	BKPT_SIZE	(1)		/* size of breakpoint inst */
#define	BKPT_SET(inst, addr)	(BKPT_INST)

#define	db_clear_single_step(regs)	((regs)->tf_regs.r_psr &= ~PSL_T)
#define	db_set_single_step(regs)	((regs)->tf_regs.r_psr |=  PSL_T)

#define	IS_BREAKPOINT_TRAP(type, code)	((type) == T_BPT)
#define	IS_WATCHPOINT_TRAP(type, code)	((type) == T_WATCHPOINT)

#define	I_BSR		0x02
#define	I_JSR		0x7f /* and low 3 bits of next byte are 0x6 */
#define	I_RET		0x12
#define	I_RETT		0x42
#define	I_RETI		0x52

#define	inst_trap_return(ins)	(((ins)&0xff) == I_RETT || \
				 ((ins)&0xff) == I_RETI)
#define	inst_return(ins)	(((ins)&0xff) == I_RET)
#define	inst_call(ins)		(((ins)&0xff) == I_BSR || \
				 (((ins)&0xff) == I_JSR && \
				  ((ins)&0x0700) == 0x0600))

#define	inst_load(ins)	0
#define	inst_store(ins)	0

extern long db_active_ipl;

/*
 * This is needed for kgdb.
 */
typedef long kgdb_reg_t;
#define	KGDB_NUMREGS	25
#define	KGDB_BUFLEN	512

#ifdef _KERNEL

int	kdb_trap __P((int, int, db_regs_t *));
struct insn;
int	db_dasm_ns32k __P((struct insn *insn, db_addr_t loc));

#endif /* _KERNEL */

/*
 * We use a.out symbols in DDB.
 */
#define	DB_AOUT_SYMBOLS

#endif
