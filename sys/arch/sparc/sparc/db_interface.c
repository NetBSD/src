/*	$NetBSD: db_interface.c,v 1.31 2000/06/29 07:40:10 mrg Exp $ */

/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 *	From: db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */

/*
 * Interface to new debugger.
 */
#include "opt_ddb.h"			/* XXX ddb vs kgdb */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>

#if defined(DDB)
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#endif

#include <machine/instr.h>
#include <machine/bsd_openprom.h>
#include <machine/promlib.h>
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>

#include "fb.h"

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(addr, size, data)
	vaddr_t	addr;
	size_t	size;
	char	*data;
{
	char	*src;

	src = (char *)addr;
	while (size-- > 0)
		*data++ = *src++;
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(addr, size, data)
	vaddr_t	addr;
	size_t	size;
	char	*data;
{
	extern char	etext[];
	char	*dst;

	dst = (char *)addr;
	while (size-- > 0) {
		if ((dst >= (char *)VM_MIN_KERNEL_ADDRESS) && (dst < etext))
			pmap_writetext(dst, *data);
		else
			*dst = *data;
		dst++, data++;
	}

}


#if defined(DDB)

/*
 * Data and functions used by DDB only.
 */
void
cpu_Debugger()
{
	asm("ta 0x81");
}

static int nil;

struct db_variable db_regs[] = {
	{ "psr", (long *)&DDB_TF->tf_psr, FCN_NULL, },
	{ "pc", (long *)&DDB_TF->tf_pc, FCN_NULL, },
	{ "npc", (long *)&DDB_TF->tf_npc, FCN_NULL, },
	{ "y", (long *)&DDB_TF->tf_y, FCN_NULL, },
	{ "wim", (long *)&DDB_TF->tf_global[0], FCN_NULL, }, /* see reg.h */
	{ "g0", (long *)&nil, FCN_NULL, },
	{ "g1", (long *)&DDB_TF->tf_global[1], FCN_NULL, },
	{ "g2", (long *)&DDB_TF->tf_global[2], FCN_NULL, },
	{ "g3", (long *)&DDB_TF->tf_global[3], FCN_NULL, },
	{ "g4", (long *)&DDB_TF->tf_global[4], FCN_NULL, },
	{ "g5", (long *)&DDB_TF->tf_global[5], FCN_NULL, },
	{ "g6", (long *)&DDB_TF->tf_global[6], FCN_NULL, },
	{ "g7", (long *)&DDB_TF->tf_global[7], FCN_NULL, },
	{ "o0", (long *)&DDB_TF->tf_out[0], FCN_NULL, },
	{ "o1", (long *)&DDB_TF->tf_out[1], FCN_NULL, },
	{ "o2", (long *)&DDB_TF->tf_out[2], FCN_NULL, },
	{ "o3", (long *)&DDB_TF->tf_out[3], FCN_NULL, },
	{ "o4", (long *)&DDB_TF->tf_out[4], FCN_NULL, },
	{ "o5", (long *)&DDB_TF->tf_out[5], FCN_NULL, },
	{ "o6", (long *)&DDB_TF->tf_out[6], FCN_NULL, },
	{ "o7", (long *)&DDB_TF->tf_out[7], FCN_NULL, },
	{ "l0", (long *)&DDB_FR->fr_local[0], FCN_NULL, },
	{ "l1", (long *)&DDB_FR->fr_local[1], FCN_NULL, },
	{ "l2", (long *)&DDB_FR->fr_local[2], FCN_NULL, },
	{ "l3", (long *)&DDB_FR->fr_local[3], FCN_NULL, },
	{ "l4", (long *)&DDB_FR->fr_local[4], FCN_NULL, },
	{ "l5", (long *)&DDB_FR->fr_local[5], FCN_NULL, },
	{ "l6", (long *)&DDB_FR->fr_local[6], FCN_NULL, },
	{ "l7", (long *)&DDB_FR->fr_local[7], FCN_NULL, },
	{ "i0", (long *)&DDB_FR->fr_arg[0], FCN_NULL, },
	{ "i1", (long *)&DDB_FR->fr_arg[1], FCN_NULL, },
	{ "i2", (long *)&DDB_FR->fr_arg[2], FCN_NULL, },
	{ "i3", (long *)&DDB_FR->fr_arg[3], FCN_NULL, },
	{ "i4", (long *)&DDB_FR->fr_arg[4], FCN_NULL, },
	{ "i5", (long *)&DDB_FR->fr_arg[5], FCN_NULL, },
	{ "i6", (long *)&DDB_FR->fr_arg[6], FCN_NULL, },
	{ "i7", (long *)&DDB_FR->fr_arg[7], FCN_NULL, },
};
struct db_variable *db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

extern label_t	*db_recover;

int	db_active = 0;

extern char *trap_type[];

void kdb_kbd_trap __P((struct trapframe *));
void db_prom_cmd __P((db_expr_t, int, db_expr_t, char *));

/*
 * Received keyboard interrupt sequence.
 */
void
kdb_kbd_trap(tf)
	struct trapframe *tf;
{
	if (db_active == 0 && (boothowto & RB_KDB)) {
		printf("\n\nkernel: keyboard interrupt\n");
		kdb_trap(-1, tf);
	}
}

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(type, tf)
	int	type;
	struct trapframe *tf;
{
	int s;

#if NFB > 0
	fb_unblank();
#endif

	/* While we're in the debugger, pause all other CPUs */
	mp_pause_cpus();

	switch (type) {
	case T_BREAKPOINT:	/* breakpoint */
	case -1:		/* keyboard interrupt */
		break;
	default:
		printf("kernel: %s trap", trap_type[type & 0xff]);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* Should switch to kdb`s own stack here. */

	ddb_regs.db_tf = *tf;
	ddb_regs.db_fr = *(struct frame *)tf->tf_out[6];

	s = splhigh();
	db_active++;
	cnpollc(TRUE);
	db_trap(type, 0/*code*/);
	cnpollc(FALSE);
	db_active--;
	splx(s);

	*(struct frame *)tf->tf_out[6] = ddb_regs.db_fr;
	*tf = ddb_regs.db_tf;

	/* Other CPUs can continue now */
	mp_resume_cpus();

	return (1);
}

void
db_prom_cmd(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	prom_abort();
}

struct db_command sparc_db_command_table[] = {
	{ "prom",	db_prom_cmd,	0,	0 },
	{ (char *)0, }
};

void
db_machine_init()
{
	db_machine_commands_install(sparc_db_command_table);
}
#endif /* DDB */


/*
 * support for SOFTWARE_SSTEP:
 * return the next pc if the given branch is taken.
 *
 * note: in the case of conditional branches with annul,
 * this actually returns the next pc in the "not taken" path,
 * but in that case next_instr_address() will return the
 * next pc in the "taken" path.  so even tho the breakpoints
 * are backwards, everything will still work, and the logic is
 * much simpler this way.
 */
db_addr_t
db_branch_taken(inst, pc, regs)
	int inst;
	db_addr_t pc;
	db_regs_t *regs;
{
    union instr insn;
    db_addr_t npc = ddb_regs.db_tf.tf_npc;

    insn.i_int = inst;

    /*
     * if this is not an annulled conditional branch, the next pc is "npc".
     */

    if (insn.i_any.i_op != IOP_OP2 || insn.i_branch.i_annul != 1)
	return npc;

    switch (insn.i_op2.i_op2) {
      case IOP2_Bicc:
      case IOP2_FBfcc:
      case IOP2_BPcc:
      case IOP2_FBPfcc:
      case IOP2_CBccc:
	/* branch on some condition-code */
	switch (insn.i_branch.i_cond)
	{
	  case Icc_A: /* always */
	    return pc + ((inst << 10) >> 8);

	  default: /* all other conditions */
	    return npc + 4;
	}

      case IOP2_BPr:
	/* branch on register, always conditional */
	return npc + 4;

      default:
	/* not a branch */
	panic("branch_taken() on non-branch");
    }
}

boolean_t
db_inst_branch(inst)
	int inst;
{
    union instr insn;

    insn.i_int = inst;

    if (insn.i_any.i_op != IOP_OP2)
	return FALSE;

    switch (insn.i_op2.i_op2) {
      case IOP2_BPcc:
      case IOP2_Bicc:
      case IOP2_BPr:
      case IOP2_FBPfcc:
      case IOP2_FBfcc:
      case IOP2_CBccc:
	return TRUE;

      default:
	return FALSE;
    }
}


boolean_t
db_inst_call(inst)
	int inst;
{
    union instr insn;

    insn.i_int = inst;

    switch (insn.i_any.i_op) {
      case IOP_CALL:
	return TRUE;

      case IOP_reg:
	return (insn.i_op3.i_op3 == IOP3_JMPL) && !db_inst_return(inst);

      default:
	return FALSE;
    }
}


boolean_t
db_inst_unconditional_flow_transfer(inst)
	int inst;
{
    union instr insn;

    insn.i_int = inst;

    if (db_inst_call(inst))
	return TRUE;

    if (insn.i_any.i_op != IOP_OP2)
	return FALSE;

    switch (insn.i_op2.i_op2)
    {
      case IOP2_BPcc:
      case IOP2_Bicc:
      case IOP2_FBPfcc:
      case IOP2_FBfcc:
      case IOP2_CBccc:
	return insn.i_branch.i_cond == Icc_A;

      default:
	return FALSE;
    }
}


boolean_t
db_inst_return(inst)
	int inst;
{
    return (inst == I_JMPLri(I_G0, I_O7, 8) ||		/* ret */
	    inst == I_JMPLri(I_G0, I_I7, 8));		/* retl */
}

boolean_t
db_inst_trap_return(inst)
	int inst;
{
    union instr insn;

    insn.i_int = inst;

    return (insn.i_any.i_op == IOP_reg &&
	    insn.i_op3.i_op3 == IOP3_RETT);
}


int
db_inst_load(inst)
	int inst;
{
    union instr insn;

    insn.i_int = inst;

    if (insn.i_any.i_op != IOP_mem)
	return 0;

    switch (insn.i_op3.i_op3) {
      case IOP3_LD:
      case IOP3_LDUB:
      case IOP3_LDUH:
      case IOP3_LDD:
      case IOP3_LDSB:
      case IOP3_LDSH:
      case IOP3_LDSTUB:
      case IOP3_SWAP:
      case IOP3_LDA:
      case IOP3_LDUBA:
      case IOP3_LDUHA:
      case IOP3_LDDA:
      case IOP3_LDSBA:
      case IOP3_LDSHA:
      case IOP3_LDSTUBA:
      case IOP3_SWAPA:
      case IOP3_LDF:
      case IOP3_LDFSR:
      case IOP3_LDDF:
      case IOP3_LFC:
      case IOP3_LDCSR:
      case IOP3_LDDC:
	return 1;

      default:
	return 0;
    }
}

int
db_inst_store(inst)
	int inst;
{
    union instr insn;

    insn.i_int = inst;

    if (insn.i_any.i_op != IOP_mem)
	return 0;

    switch (insn.i_op3.i_op3) {
      case IOP3_ST:
      case IOP3_STB:
      case IOP3_STH:
      case IOP3_STD:
      case IOP3_LDSTUB:
      case IOP3_SWAP:
      case IOP3_STA:
      case IOP3_STBA:
      case IOP3_STHA:
      case IOP3_STDA:
      case IOP3_LDSTUBA:
      case IOP3_SWAPA:
      case IOP3_STF:
      case IOP3_STFSR:
      case IOP3_STDFQ:
      case IOP3_STDF:
      case IOP3_STC:
      case IOP3_STCSR:
      case IOP3_STDCQ:
      case IOP3_STDC:
	return 1;

      default:
	return 0;
    }
}
