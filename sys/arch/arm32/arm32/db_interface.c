/*	$NetBSD: db_interface.c,v 1.29.2.2 2000/12/08 09:26:23 bouyer Exp $	*/

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
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>	/* just for boothowto */
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>
#include <machine/katelib.h>
#include <machine/pte.h>
#include <machine/undefined.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <dev/cons.h>

static int nil;

int db_access_und_sp __P((struct db_variable *, db_expr_t *, int));
int db_access_abt_sp __P((struct db_variable *, db_expr_t *, int));
int db_access_irq_sp __P((struct db_variable *, db_expr_t *, int));
u_int db_fetch_reg __P((int, db_regs_t *));

struct db_variable db_regs[] = {
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
	{ "und_sp", (long *)&nil, db_access_und_sp, },
	{ "abt_sp", (long *)&nil, db_access_abt_sp, },
	{ "irq_sp", (long *)&nil, db_access_irq_sp, },
};

struct db_variable *db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

extern label_t	*db_recover;

int	db_active = 0;

int db_access_und_sp(vp, valp, rw)
	struct db_variable *vp;
	db_expr_t *valp;
	int rw;
{
	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_UND32_MODE);
	return(0);
}

int db_access_abt_sp(vp, valp, rw)
	struct db_variable *vp;
	db_expr_t *valp;
	int rw;
{
	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_ABT32_MODE);
	return(0);
}

int db_access_irq_sp(vp, valp, rw)
	struct db_variable *vp;
	db_expr_t *valp;
	int rw;
{
	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_IRQ32_MODE);
	return(0);
}

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(type, regs)
	int type;
	db_regs_t *regs;
{
	int s;

	switch (type) {
	case T_BREAKPOINT:	/* breakpoint */
	case -1:		/* keyboard interrupt */
		break;
	default:
		db_printf("kernel: trap");
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* Should switch to kdb`s own stack here. */

	ddb_regs = *regs;

	s = splhigh();
	db_active++;
	cnpollc(TRUE);
	db_trap(type, 0/*code*/);
	cnpollc(FALSE);
	db_active--;
	splx(s);

	*regs = ddb_regs;

	return (1);
}


/*
 * Received keyboard interrupt sequence.
 */
void
kdb_kbd_trap(regs)
	db_regs_t *regs;
{
	if (db_active == 0 && (boothowto & RB_KDB)) {
		printf("\n\nkernel: keyboard interrupt\n");
		kdb_trap(-1, regs);
	}
}


static int
db_validate_address(addr)
	vm_offset_t addr;
{
	pt_entry_t *ptep;
	pd_entry_t *pdep;
	struct proc *p = curproc;

	/*
	 * If we have a valid pmap for curproc, use it's page directory
	 * otherwise use the kernel pmap's page directory.
	 */
	if (!p || !p->p_vmspace || !p->p_vmspace->vm_map.pmap)
		pdep = kernel_pmap->pm_pdir;
	else
		pdep = p->p_vmspace->vm_map.pmap->pm_pdir;

	/* Make sure the address we are reading is valid */
	switch ((pdep[(addr >> 20) + 0] & L1_MASK)) {
	case L1_SECTION:
		break;
	case L1_PAGE:
		/* Check the L2 page table for validity */
		ptep = vtopte(addr);
		if ((*ptep & L2_MASK) != L2_INVAL)
			break;
		/* FALLTHROUGH */
	default:
		return 1;
	}

	return 0;
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(addr, size, data)
	vm_offset_t	addr;
	int	size;
	char	*data;
{
	char	*src;

	src = (char *)addr;
	while (--size >= 0) {
		if (db_validate_address((u_int)src)) {
			db_printf("address %p is invalid\n", src);
			return;
		}
		*data++ = *src++;
	}
}

static void
db_write_text(dst, ch)
	unsigned char *dst;
	int ch;
{        
	pt_entry_t *ptep, pteo;
	vm_offset_t va;

	va = (unsigned long)dst & (~PGOFSET);
	ptep = vtopte(va);

	if (db_validate_address((u_int)dst)) {
		db_printf(" address %p not a valid page\n", dst);
		return;
	}

	pteo = *ptep;
	*ptep = pteo | PT_AP(AP_KRW);
	cpu_tlb_flushD_SE(va);

	*dst = (unsigned char)ch;

	/* make sure the caches and memory are in sync */
	cpu_cache_syncI_rng((u_int)dst, 4);

	*ptep = pteo;
	cpu_tlb_flushD_SE(va);
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(addr, size, data)
	vm_offset_t	addr;
	int	size;
	char	*data;
{
	extern char	etext[];
	char	*dst;
	int	loop;

	dst = (char *)addr;
	loop = size;
	while (--loop >= 0) {
		if ((dst >= (char *)KERNEL_TEXT_BASE) && (dst < etext))
			db_write_text(dst, *data);
		else {
			if (db_validate_address((u_int)dst)) {
				db_printf("address %p is invalid\n", dst);
				return;
			}
			*dst = *data;
		}
		dst++, data++;
	}
	/* make sure the caches and memory are in sync */
	cpu_cache_syncI_rng(addr, size);

	/* In case the current page tables have been modified ... */
	cpu_tlb_flushID();
}

void
cpu_Debugger()
{
	asm(".word	0xe7ffffff");
}

void db_show_vmstat_cmd __P((db_expr_t addr, int have_addr, db_expr_t count, char *modif));
void db_show_intrchain_cmd	__P((db_expr_t addr, int have_addr, db_expr_t count, char *modif));
void db_show_panic_cmd	__P((db_expr_t addr, int have_addr, db_expr_t count, char *modif));
void db_show_frame_cmd	__P((db_expr_t addr, int have_addr, db_expr_t count, char *modif));
#ifdef	OFW
void db_of_boot_cmd	__P((db_expr_t addr, int have_addr, db_expr_t count, char *modif));
void db_of_enter_cmd	__P((db_expr_t addr, int have_addr, db_expr_t count, char *modif));
void db_of_exit_cmd	__P((db_expr_t addr, int have_addr, db_expr_t count, char *modif));
#endif

struct db_command arm32_db_command_table[] = {
	{ "frame",	db_show_frame_cmd,	0, NULL },
	{ "intrchain",	db_show_intrchain_cmd,	0, NULL },
#ifdef	OFW
	{ "ofboot",	db_of_boot_cmd,		0, NULL },
	{ "ofenter",	db_of_enter_cmd,	0, NULL },
	{ "ofexit",	db_of_exit_cmd,		0, NULL },
#endif
	{ "panic",	db_show_panic_cmd,	0, NULL },
	{ "vmstat",	db_show_vmstat_cmd,	0, NULL },
	{ NULL, 	NULL, 			0, NULL }
};

int
db_trapper(addr, inst, frame, fault_code)
	u_int		addr;
	u_int		inst;
	trapframe_t	*frame;
	int		fault_code;
{
	if (fault_code == 0) {
		frame->tf_pc -= INSN_SIZE;
		if ((inst & ~INSN_COND_MASK) == (BKPT_INST & ~INSN_COND_MASK))
			kdb_trap(T_BREAKPOINT, frame);
		else
			kdb_trap(-1, frame);
	} else
		return (1);
	return (0);
}

extern u_int esym;
extern u_int end;

void
db_machine_init()
{
	struct exec *kernexec = (struct exec *)KERNEL_TEXT_BASE;
	int len;

	/*
	 * The boot loader currently loads the kernel with the a.out
	 * header still attached.
	 */

	if (kernexec->a_syms == 0) {
		printf("[No symbol table]\n");
	} else {
#if !defined(SHARK) && !defined(OFWGENCFG)
		esym = (int)&end + kernexec->a_syms + sizeof(int);
#else
		/* cover the symbols themselves */
		esym = (int)&end + kernexec->a_syms;
#endif
		/*
		 * and the string table.  (int containing size of string
		 * table is included in string table size).
		 */
		len = *((u_int *)esym);
		esym += (len + (sizeof(u_int) - 1)) & ~(sizeof(u_int) - 1);
	}

	install_coproc_handler(0, db_trapper);
	db_machine_commands_install(arm32_db_command_table);
}

u_int
db_fetch_reg(reg, db_regs)
	int reg;
	db_regs_t *db_regs;
{

	switch (reg) {
	case 0:
		return (db_regs->tf_r0);
	case 1:
		return (db_regs->tf_r1);
	case 2:
		return (db_regs->tf_r2);
	case 3:
		return (db_regs->tf_r3);
	case 4:
		return (db_regs->tf_r4);
	case 5:
		return (db_regs->tf_r5);
	case 6:
		return (db_regs->tf_r6);
	case 7:
		return (db_regs->tf_r7);
	case 8:
		return (db_regs->tf_r8);
	case 9:
		return (db_regs->tf_r9);
	case 10:
		return (db_regs->tf_r10);
	case 11:
		return (db_regs->tf_r11);
	case 12:
		return (db_regs->tf_r12);
	case 13:
		return (db_regs->tf_svc_sp);
	case 14:
		return (db_regs->tf_svc_lr);
	case 15:
		return (db_regs->tf_pc);
	default:
		panic("db_fetch_reg: botch");
	}
}

u_int
branch_taken(insn, pc, db_regs)
	u_int insn;
	u_int pc;
	db_regs_t *db_regs;
{
	u_int addr, nregs;

	switch ((insn >> 24) & 0xf) {
	case 0xa:	/* b ... */
	case 0xb:	/* bl ... */
		addr = ((insn << 2) & 0x03ffffff);
		if (addr & 0x02000000)
			addr |= 0xfc000000;
		return (pc + 8 + addr);
	case 0x7:	/* ldr pc, [pc, reg, lsl #2] */
		addr = db_fetch_reg(insn & 0xf, db_regs);
		addr = pc + 8 + (addr << 2);
		db_read_bytes(addr, 4, (char *)&addr);
		return (addr);
	case 0x1:	/* mov pc, reg */
		addr = db_fetch_reg(insn & 0xf, db_regs);
		return (addr);
	case 0x8:	/* ldmxx reg, {..., pc} */
	case 0x9:
		addr = db_fetch_reg((insn >> 16) & 0xf, db_regs);
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
		return (addr);
	default:
		panic("branch_taken: botch");
	}
}
