/*	$NetBSD: db_interface.c,v 1.20 2014/03/08 16:55:38 skrll Exp $	*/

/* 
 * Copyright (c) 1996 Scott K. Stevens
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
 *
 *	From: db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */

/*
 * Interface to new debugger.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.20 2014/03/08 16:55:38 skrll Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>	/* just for boothowto */
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>
#include <arm/undefined.h>
#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_interface.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <dev/cons.h>

int db_access_und_sp(const struct db_variable *, db_expr_t *, int);
int db_access_abt_sp(const struct db_variable *, db_expr_t *, int);
int db_access_irq_sp(const struct db_variable *, db_expr_t *, int);
u_int db_fetch_reg(int, db_regs_t *);
int db_trapper(u_int addr, u_int inst, struct trapframe *frame,
    int fault_code);

static void db_write_text(unsigned char *, int ch);

const struct db_variable db_regs[] = {
	{ "spsr", (long *)&DDB_REGS->tf_spsr, FCN_NULL, },
	{ "r0", (long *)&DDB_REGS->tf_r0, FCN_NULL, },
	{ "r1", (long *)&DDB_REGS->tf_r1, FCN_NULL, },
	{ "r2", (long *)&DDB_REGS->tf_r2, FCN_NULL, },
	{ "r3", (long *)&DDB_REGS->tf_r3, FCN_NULL, },
	{ "r4", (long *)&DDB_REGS->tf_r4, FCN_NULL, },
	{ "r5", (long *)&DDB_REGS->tf_r5, FCN_NULL, },
	{ "r6", (long *)&DDB_REGS->tf_r6, FCN_NULL, },
	{ "r7", (long *)&DDB_REGS->tf_r7, FCN_NULL, },
	{ "r8", (long *)&DDB_REGS->tf_r8, FCN_NULL, },
	{ "r9", (long *)&DDB_REGS->tf_r9, FCN_NULL, },
	{ "r10", (long *)&DDB_REGS->tf_r10, FCN_NULL, },
	{ "r11", (long *)&DDB_REGS->tf_r11, FCN_NULL, },
	{ "r12", (long *)&DDB_REGS->tf_r12, FCN_NULL, },
	{ "usr_sp", (long *)&DDB_REGS->tf_usr_sp, FCN_NULL, },
	{ "usr_lr", (long *)&DDB_REGS->tf_usr_lr, FCN_NULL, },
	{ "svc_sp", (long *)&DDB_REGS->tf_svc_sp, FCN_NULL, },
	{ "svc_lr", (long *)&DDB_REGS->tf_svc_lr, FCN_NULL, },
	{ "pc", (long *)&DDB_REGS->tf_pc, FCN_NULL, },
#ifdef __PROG32
	{ "und_sp", (long *)&nil, db_access_und_sp, },
	{ "abt_sp", (long *)&nil, db_access_abt_sp, },
	{ "irq_sp", (long *)&nil, db_access_irq_sp, },
#endif
};

const struct db_variable * const db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

int	db_active = 0;
db_regs_t ddb_regs;	/* register state */

#ifdef __PROG32
int
db_access_und_sp(const struct db_variable *vp, db_expr_t *valp, int rw)
{

	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_UND32_MODE);
	return 0;
}

int
db_access_abt_sp(const struct db_variable *vp, db_expr_t valp, int rw)
{

	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_ABT32_MODE);
	return 0;
}

int
db_access_irq_sp(const struct db_variable *vp, db_expr_t *valp,	int rw)
{

	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_IRQ32_MODE);
	return 0;
}
#endif /* __PROG32 */

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(int type, db_regs_t *regs)
{
	int s;

	switch (type) {
	case T_BREAKPOINT:	/* breakpoint */
	case -1:		/* keyboard interrupt */
		break;
	default:
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* Should switch to kdb`s own stack here. */

	ddb_regs = *regs;

	s = splhigh();
	db_active++;
	cnpollc(true);
	db_trap(type, 0/*code*/);
	cnpollc(false);
	db_active--;
	splx(s);

	*regs = ddb_regs;

	return 1;
}

volatile bool db_validating, db_faulted;

int
db_validate_address(vm_offset_t addr)
{

	db_faulted = false;
	db_validating = true;
	(void) *(volatile uint8_t *)addr;
	db_validating = false;
	return db_faulted;
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vm_offset_t addr,	size_t size, char *data)
{
	char	*src;

	src = (char *)addr;
	for (; size > 0; size--) {
		if (db_validate_address((u_int)src)) {
			db_printf("address %p is invalid\n", src);
			return;
		}
		*data++ = *src++;
	}
}

static void
db_write_text(unsigned char *dst, int ch)
{        

	if (db_validate_address((u_int)dst)) {
		db_printf(" address %p not a valid page\n", dst);
		return;
	}

	*dst = (unsigned char)ch;
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vm_offset_t addr, size_t size, const char *data)
{
#if 0
	extern char	_stext_[], _etext[];
#endif
	char	*dst;
	int	loop;

	dst = (char *)addr;
	loop = size;
	while (--loop >= 0) {
#if 0 /* FIXME */
		if ((dst >= _stext_) && (dst < _etext))
#endif
			db_write_text(dst, *data);
#if 0
		else {
			if (db_validate_address((u_int)dst)) {
				db_printf("address %p is invalid\n", dst);
				return;
			}
			*dst = *data;
		}
#endif
		dst++, data++;
	}
}

const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD("bsw", db_bus_write_cmd,		CS_MORE,
			"Writes a one or two bytes to the IObus",
			"[/bh] [addr]",
			"   addr:\tIO address to write\n"
			"   /b:\twrite a single byte\n"
			"   /h:\twrite two bytes") },
	{ DDB_ADD_CMD("frame", db_show_frame_cmd,	0,
			"Displays the contents of a trapframe",
			"[address]",
			"   address:\taddress of trapfame to display")},
	{ DDB_ADD_CMD("irqstat", db_irqstat_cmd,		0,
			"Displays the IRQ statistics",
		     	NULL,NULL) },
	{ DDB_ADD_CMD( NULL,     NULL,              0, NULL, NULL,NULL) }
};

int
db_trapper(u_int addr, u_int inst, trapframe_t *frame, int fault_code)
{

	if (fault_code == 0) {
		if ((inst & ~INSN_COND_MASK) == (BKPT_INST & ~INSN_COND_MASK))
			kdb_trap(T_BREAKPOINT, frame);
		else
			kdb_trap(-1, frame);
	} else
		return 1;
	return 0;
}

extern u_int esym;
extern u_int end;

static struct undefined_handler db_uh;

void
db_machine_init(void)
{

	/*
	 * We get called before malloc() is available, so supply a static
	 * struct undefined_handler.
	 */
	db_uh.uh_handler = db_trapper;
	install_coproc_handler_static(CORE_UNKNOWN_HANDLER, &db_uh);
}

u_int
db_fetch_reg(int reg, db_regs_t *regs)
{

	switch (reg) {
	case 0:
		return regs->tf_r0;
	case 1:
		return regs->tf_r1;
	case 2:
		return regs->tf_r2;
	case 3:
		return regs->tf_r3;
	case 4:
		return regs->tf_r4;
	case 5:
		return regs->tf_r5;
	case 6:
		return regs->tf_r6;
	case 7:
		return regs->tf_r7;
	case 8:
		return regs->tf_r8;
	case 9:
		return regs->tf_r9;
	case 10:
		return regs->tf_r10;
	case 11:
		return regs->tf_r11;
	case 12:
		return regs->tf_r12;
	case 13:
		return regs->tf_svc_sp;
	case 14:
		return regs->tf_svc_lr;
	case 15:
		return regs->tf_pc;
	default:
		panic("db_fetch_reg: botch");
	}
}

u_int
branch_taken(u_int insn, u_int pc, db_regs_t *regs)
{
	u_int addr, nregs;

	switch ((insn >> 24) & 0xf) {
	case 0xa:	/* b ... */
	case 0xb:	/* bl ... */
		addr = ((insn << 2) & 0x03ffffff);
		if (addr & 0x02000000)
			addr |= 0xfc000000;
		return pc + 8 + addr;
	case 0x7:	/* ldr pc, [pc, reg, lsl #2] */
		addr = db_fetch_reg(insn & 0xf, regs);
		addr = pc + 8 + (addr << 2);
		db_read_bytes(addr, 4, (char *)&addr);
		return addr;
	case 0x1:	/* mov pc, reg */
		addr = db_fetch_reg(insn & 0xf, regs);
		return addr;
	case 0x8:	/* ldmxx reg, {..., pc} */
	case 0x9:
		addr = db_fetch_reg((insn >> 16) & 0xf, regs);
		nregs = (insn  & 0x5555) + ((insn  >> 1) & 0x5555);
		nregs = (nregs & 0x3333) + ((nregs >> 2) & 0x3333);
		nregs = (nregs + (nregs >> 4)) & 0x0f0f;
		nregs = (nregs + (nregs >> 8)) & 0x001f;
		switch ((insn >> 23) & 0x3) {
		case 0x0:	/* ldmda */
			addr = addr - 0;
			break;
		case 0x1:	/* ldmia */
			addr = addr + 0 + ((nregs - 1) << 2);
			break;
		case 0x2:	/* ldmdb */
			addr = addr - 4;
			break;
		case 0x3:	/* ldmib */
			addr = addr + 4 + ((nregs - 1) << 2);
			break;
		}
		db_read_bytes(addr, 4, (char *)&addr);
		return addr;
	default:
		panic("branch_taken: botch");
	}
}
