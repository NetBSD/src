/*	$NetBSD: db_machdep.h,v 1.15 1997/01/27 23:02:55 gwr Exp $	*/

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
#ifndef	_M68K_DB_MACHDEP_H_
#define	_M68K_DB_MACHDEP_H_

#include <vm/vm_prot.h>
#include <vm/vm_param.h>
#include <vm/vm_inherit.h>
#include <vm/lock.h>

#include <machine/frame.h>
#include <machine/psl.h>
#include <machine/trap.h>

typedef	vm_offset_t	db_addr_t;	/* address - unsigned */
typedef	int		db_expr_t;	/* expression - signed */
typedef struct trapframe db_regs_t;

db_regs_t	ddb_regs;		/* register state */
#define DDB_REGS	(&ddb_regs)

#define	PC_REGS(regs)	((db_addr_t)(regs)->tf_pc)

#define	BKPT_INST	0x4e4f		/* breakpoint instruction */
#define	BKPT_SIZE	(2)		/* size of breakpoint inst */
#define	BKPT_SET(inst)	(BKPT_INST)

#define	FIXUP_PC_AFTER_BREAK	ddb_regs.tf_pc -= 2;

#define SR_T1 0x8000
#define	db_clear_single_step(regs)	((regs)->tf_sr &= ~SR_T1)
#define	db_set_single_step(regs)	((regs)->tf_sr |=  SR_T1)

#define	IS_BREAKPOINT_TRAP(type, code)	((type) == T_BREAKPOINT)
#ifdef T_WATCHPOINT
#define	IS_WATCHPOINT_TRAP(type, code)	((type) == T_WATCHPOINT)
#else
#define	IS_WATCHPOINT_TRAP(type, code)	0
#endif

#define	M_RTS		0xffff0000
#define I_RTS		0x4e750000
#define M_JSR		0xffc00000
#define I_JSR		0x4e800000
#define M_BSR		0xff000000
#define I_BSR		0x61000000
#define	M_RTE		0xffff0000
#define	I_RTE		0x4e730000

#define	inst_trap_return(ins)	(((ins)&M_RTE) == I_RTE)
#define	inst_return(ins)	(((ins)&M_RTS) == I_RTS)
#define	inst_call(ins)		(((ins)&M_JSR) == I_JSR || \
				 ((ins)&M_BSR) == I_BSR)
#define inst_load(ins)		0
#define inst_store(ins)		0

#ifdef _KERNEL

void	kdb_kintr __P((db_regs_t *));
int	kdb_trap __P((int, db_regs_t *));

#endif /* _KERNEL */

#endif	/* _M68K_DB_MACHDEP_H_ */
