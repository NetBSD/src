/* $NetBSD: db_interface.c,v 1.1 1996/02/15 22:37:20 mark Exp $ */

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
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
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
 *
 *	$Id: db_interface.c,v 1.1 1996/02/15 22:37:20 mark Exp $
 */

/*
 * Interface to new debugger.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>	/* just for boothowto */
#include <setjmp.h>

#include <vm/vm.h>

#include <machine/db_machdep.h>
#include <machine/katelib.h>
#include <machine/pte.h>
#include <ddb/db_command.h>
#include <ddb/db_variables.h>

static int nil;

int db_access_svc_sp __P((struct db_variable *, db_expr_t *, int));

struct db_variable db_regs[] = {
	{ "spsr", (int *)&DDB_TF->tf_spsr, FCN_NULL, },
	{ "r0", (int *)&DDB_TF->tf_r0, FCN_NULL, },
	{ "r1", (int *)&DDB_TF->tf_r1, FCN_NULL, },
	{ "r2", (int *)&DDB_TF->tf_r2, FCN_NULL, },
	{ "r3", (int *)&DDB_TF->tf_r3, FCN_NULL, },
	{ "r4", (int *)&DDB_TF->tf_r4, FCN_NULL, },
	{ "r5", (int *)&DDB_TF->tf_r5, FCN_NULL, },
	{ "r6", (int *)&DDB_TF->tf_r6, FCN_NULL, },
	{ "r7", (int *)&DDB_TF->tf_r7, FCN_NULL, },
	{ "r8", (int *)&DDB_TF->tf_r8, FCN_NULL, },
	{ "r9", (int *)&DDB_TF->tf_r9, FCN_NULL, },
	{ "r10", (int *)&DDB_TF->tf_r10, FCN_NULL, },
	{ "r11", (int *)&DDB_TF->tf_r11, FCN_NULL, },
	{ "r12", (int *)&DDB_TF->tf_r12, FCN_NULL, },
	{ "usr_sp", (int *)&DDB_TF->tf_usr_sp, FCN_NULL, },
	{ "svc_sp", (int *)&nil, db_access_svc_sp, },
	{ "usr_lr", (int *)&DDB_TF->tf_usr_lr, FCN_NULL, },
	{ "svc_lr", (int *)&DDB_TF->tf_svc_lr, FCN_NULL, },
	{ "pc", (int *)&DDB_TF->tf_pc, FCN_NULL, },
};

struct db_variable *db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

extern jmp_buf	*db_recover;

int	db_active = 0;

int db_access_svc_sp(vp, valp, rw)
	struct db_variable *vp;
	db_expr_t *valp;
	int rw;
{
	if(rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_SVC32_MODE);
	return(0);
}

#if 0
extern char *trap_type[];
#endif

/*
 * Received keyboard interrupt sequence.
 */
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
kdb_trap(type, tf)
	int	type;
	register struct trapframe *tf;
{

#if 0
	fb_unblank();
#endif

	switch (type) {
	case T_BREAKPOINT:	/* breakpoint */
	case -1:		/* keyboard interrupt */
		break;
	default:
		printf("kernel: trap");
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* Should switch to kdb`s own stack here. */

	ddb_regs.ddb_tf = *tf;

	db_active++;
	cnpollc(TRUE);
	db_trap(type, 0/*code*/);
	cnpollc(FALSE);
	db_active--;

	*tf = ddb_regs.ddb_tf;

	return (1);
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(addr, size, data)
	vm_offset_t	addr;
	register int	size;
	register char	*data;
{
	register char	*src;

	src = (char *)addr;
	while (--size >= 0)
		*data++ = *src++;
}

#define	splpmap() splimp()

static void
db_write_text(dst, ch)
	unsigned char *dst;
	int ch;
{        
	pt_entry_t *ptep, pte, pteo;
	int s;
	vm_offset_t va;

	s = splpmap();
	va = (unsigned long)dst & (~PGOFSET);
	ptep = vtopte(va);

	if ((*ptep & L2_MASK) == L2_INVAL) { 
		db_printf(" address 0x%x not a valid page\n", dst);
		splx(s);
		return;
	}

	pteo = ReadWord(ptep);
	pte = pteo | PT_AP(AP_KRW);
	WriteWord(ptep, pte);
	tlbflush();

	*dst = (unsigned char)ch;

	WriteWord(ptep, pteo);
	tlbflush();
	splx(s);
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(addr, size, data)
	vm_offset_t	addr;
	register int	size;
	register char	*data;
{
	extern char	etext[];
	register char	*dst;

	dst = (char *)addr;
	while (--size >= 0) {
		if ((dst >= (char *)VM_MIN_KERNEL_ADDRESS) && (dst < etext))
			db_write_text(dst, *data);
		else
			*dst = *data;
		dst++, data++;
	}

}

int
Debugger()
{
	asm(".word	0xe7ffffff");
}

struct db_command arm32_db_command_table[] = {
	{ (char *)0, }
};

int
db_trapper(addr, inst, frame)
	u_int		addr;
	u_int		inst;
	trapframe_t	*frame;
{
	printf("db_trapper\n");
	kdb_trap(1, frame);
	return(0);
}

void
db_machine_init()
{
	install_coproc_handler(0, db_trapper);
	db_machine_commands_install(arm32_db_command_table);
}
