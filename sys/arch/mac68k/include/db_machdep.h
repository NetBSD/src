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
 * HISTORY
 * $Log: db_machdep.h,v $
 * Revision 1.1  1993/12/15 03:30:27  briggs
 * Added for ddb support.  I think it's at least partially wrong.  This and
 * the files in mac68k/db_* started off as the sun3 files...
 *
 * Revision 1.2  1993/08/10  08:42:37  glass
 * fixed problem that caused two consecutive segments to be using the same
 * pmeg unknowingly.  still too many printfs, not sure how many are actualy
 * in the machine dependent code.  reaches cpu_startup() where it stops
 * deliberately. next project: autoconfig(), maybe kgdb
 *
 * Revision 1.1  93/08/08  12:18:27  glass
 * various changes
 * 
 * Revision 2.5  91/07/31  18:12:56  dbg
 * 	Changed register save area name.
 * 	[91/07/12            dbg]
 * 
 * Revision 2.4  91/01/08  15:52:19  rpd
 * 	Added dummy inst_load/inst_store macros.
 * 	[91/01/06            rpd]
 * 
 * Revision 2.3  90/10/25  14:47:23  rwd
 * 	Added watchpoint support.
 * 	[90/10/16            rwd]
 * 
 * Revision 2.2  90/08/27  22:11:31  dbg
 * 	Created.
 * 	[90/07/25            dbg]
 * 
 */

/*
 * Machine-dependent defines for new kernel debugger.
 */
#ifndef	_MAC68K_DB_MACHDEP_H_
#define	_MAC68K_DB_MACHDEP_H_

/*#include <mach/sun3/vm_types.h>*/
/*#include <mach/sun3/vm_param.h>*/
/*#include <sun3/thread.h>*/		/* for thread_status */
#include <vm/queue.h>
#include <vm/vm_prot.h>
#include <vm/vm_param.h>
#include <vm/vm_inherit.h>
#include <vm/lock.h>
#include <machine/psl.h>
#include <machine/trap.h>


typedef	vm_offset_t	db_addr_t;	/* address - unsigned */
typedef	int		db_expr_t;	/* expression - signed */
struct mc68020_saved_state {
        int             d0;             /* data registers */
        int             d1;
        int             d2;
        int             d3;
        int             d4;
        int             d5;
        int             d6;
        int             d7;
        int             a0;             /* address registers */
        int             a1;
        int             a2;
        int             a3;
        int             a4;
        int             a5;
        int             a6;
        int             sp;             /* stack pointer */
        unsigned short  sr;             /* status register */
        unsigned int    pc;             /* program counter - UNALIGNED!!! */
        unsigned int    stkfmt  : 4;    /* rte stack frame format */
        unsigned int    vector  : 12;   /* vector number */
};
typedef struct mc68020_saved_state db_regs_t;
db_regs_t	ddb_regs;		/* register state */
#define DDB_REGS	(&ddb_regs)

#define	PC_REGS(regs)	((db_addr_t)(regs)->pc)

#define	BKPT_INST	0x4e4f		/* breakpoint instruction */
#define	BKPT_SIZE	(2)		/* size of breakpoint inst */
#define	BKPT_SET(inst)	(BKPT_INST)

#define	FIXUP_PC_AFTER_BREAK	ddb_regs.pc -= 2;

#define SR_T1 0x8000
#define	db_clear_single_step(regs)	((regs)->sr &= ~SR_T1)
#define	db_set_single_step(regs)	((regs)->sr |=  SR_T1)

#define	IS_BREAKPOINT_TRAP(type, code)	((type) == T_BRKPT)
#define	IS_WATCHPOINT_TRAP(type, code)	((type) == T_WATCHPOINT)

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

#endif	/* _MAC68K_DDB_MACHDEP_H_ */
