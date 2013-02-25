/*	$NetBSD: db_machdep.c,v 1.14.2.2 2013/02/25 00:28:24 tls Exp $	*/

/* 
 * Copyright (c) 1996 Mark Brinicombe
 *
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_machdep.c,v 1.14.2.2 2013/02/25 00:28:24 tls Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/systm.h>

#include <arm/arm32/db_machdep.h>
#include <arm/cpufunc.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_command.h>

#ifdef _KERNEL
static long nil;

int db_access_und_sp(const struct db_variable *, db_expr_t *, int);
int db_access_abt_sp(const struct db_variable *, db_expr_t *, int);
int db_access_irq_sp(const struct db_variable *, db_expr_t *, int);
#endif

const struct db_variable db_regs[] = {
	{ "spsr", (long *)&DDB_REGS->tf_spsr, FCN_NULL, NULL },
	{ "r0", (long *)&DDB_REGS->tf_r0, FCN_NULL, NULL },
	{ "r1", (long *)&DDB_REGS->tf_r1, FCN_NULL, NULL },
	{ "r2", (long *)&DDB_REGS->tf_r2, FCN_NULL, NULL },
	{ "r3", (long *)&DDB_REGS->tf_r3, FCN_NULL, NULL },
	{ "r4", (long *)&DDB_REGS->tf_r4, FCN_NULL, NULL },
	{ "r5", (long *)&DDB_REGS->tf_r5, FCN_NULL, NULL },
	{ "r6", (long *)&DDB_REGS->tf_r6, FCN_NULL, NULL },
	{ "r7", (long *)&DDB_REGS->tf_r7, FCN_NULL, NULL },
	{ "r8", (long *)&DDB_REGS->tf_r8, FCN_NULL, NULL },
	{ "r9", (long *)&DDB_REGS->tf_r9, FCN_NULL, NULL },
	{ "r10", (long *)&DDB_REGS->tf_r10, FCN_NULL, NULL },
	{ "r11", (long *)&DDB_REGS->tf_r11, FCN_NULL, NULL },
	{ "r12", (long *)&DDB_REGS->tf_r12, FCN_NULL, NULL },
	{ "usr_sp", (long *)&DDB_REGS->tf_usr_sp, FCN_NULL, NULL },
	{ "usr_lr", (long *)&DDB_REGS->tf_usr_lr, FCN_NULL, NULL },
	{ "svc_sp", (long *)&DDB_REGS->tf_svc_sp, FCN_NULL, NULL },
	{ "svc_lr", (long *)&DDB_REGS->tf_svc_lr, FCN_NULL, NULL },
	{ "pc", (long *)&DDB_REGS->tf_pc, FCN_NULL, NULL },
#ifdef _KERNEL
	{ "und_sp", &nil, db_access_und_sp, NULL },
	{ "abt_sp", &nil, db_access_abt_sp, NULL },
	{ "irq_sp", &nil, db_access_irq_sp, NULL },
#endif
};

const struct db_variable * const db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD("frame",	db_show_frame_cmd,	0,
			"Displays the contents of a trapframe",
			"[address]",
			"   address:\taddress of trapfame to display")},
#ifdef _KERNEL
	{ DDB_ADD_CMD("fault",	db_show_fault_cmd,	0,
			"Displays the fault registers",
		     	NULL,NULL) },
#endif
#ifdef ARM32_DB_COMMANDS
	ARM32_DB_COMMANDS,
#endif
	{ DDB_ADD_CMD(NULL,     NULL,           0,NULL,NULL,NULL) }
};

#ifdef _KERNEL
int
db_access_und_sp(const struct db_variable *vp, db_expr_t *valp, int rw)
{

	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_UND32_MODE);
	return(0);
}

int
db_access_abt_sp(const struct db_variable *vp, db_expr_t *valp, int rw)
{

	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_ABT32_MODE);
	return(0);
}

int
db_access_irq_sp(const struct db_variable *vp, db_expr_t *valp, int rw)
{

	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_IRQ32_MODE);
	return(0);
}

void
db_show_fault_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	db_printf("DFAR=%#x DFSR=%#x IFAR=%#x IFSR=%#x TTBR=%#x\n",
	    armreg_dfar_read(), armreg_dfsr_read(),
	    armreg_ifar_read(), armreg_ifsr_read(),
	    armreg_ttbr_read());
}
#endif


void
db_show_frame_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct trapframe *frame;

	if (!have_addr) {
		db_printf("frame address must be specified\n");
		return;
	}

	frame = (struct trapframe *)addr;

	db_printf("frame address = %08x  ", (u_int)frame);
	db_printf("spsr=%08x\n", frame->tf_spsr);
	db_printf("r0 =%08x r1 =%08x r2 =%08x r3 =%08x\n",
	    frame->tf_r0, frame->tf_r1, frame->tf_r2, frame->tf_r3);
	db_printf("r4 =%08x r5 =%08x r6 =%08x r7 =%08x\n",
	    frame->tf_r4, frame->tf_r5, frame->tf_r6, frame->tf_r7);
	db_printf("r8 =%08x r9 =%08x r10=%08x r11=%08x\n",
	    frame->tf_r8, frame->tf_r9, frame->tf_r10, frame->tf_r11);
	db_printf("r12=%08x r13=%08x r14=%08x r15=%08x\n",
	    frame->tf_r12, frame->tf_usr_sp, frame->tf_usr_lr, frame->tf_pc);
	db_printf("slr=%08x\n", frame->tf_svc_lr);
}
