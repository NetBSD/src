/*	$NetBSD: db_interface.c,v 1.10 1999/01/15 01:23:12 castor Exp $	*/

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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include "opt_cputype.h"	/* which mips CPUs do we support? */
#include "opt_ddb.h"

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/reboot.h>

#include <vm/vm.h>
#include <mips/pte.h>
#include <mips/cpu.h>
#include <mips/locore.h>
#include <mips/mips_opcode.h>
#include <dev/cons.h>

#include <mips/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

int	db_active = 0;
mips_reg_t kdbaux[11]; /* XXX struct switchframe: better inside curpcb? XXX */

void db_halt_cmd __P((db_expr_t, int, db_expr_t, char *));
void db_tlbdump_cmd __P((db_expr_t, int, db_expr_t, char *));

extern int	kdbpeek __P((vaddr_t addr));
extern void	kdbpoke __P((vaddr_t addr, int newval));
extern vaddr_t	MachEmulateBranch __P((struct frame *, vaddr_t, unsigned, int));

#ifdef DDB_TRACE
int
kdbpeek(addr)
	vaddr_t addr;
{
	if (addr == 0 || (addr & 3))
		return 0;
	return *(int *)addr;
}
#endif

/*
 * kdbpoke -- write a value to a kernel virtual address.
 *    XXX should handle KSEG2 addresses and check for unmapped pages.
 *    XXX user-space addresess?
 */
void
kdbpoke(vaddr_t addr, int newval)
{
	*(int*) addr = newval;
	wbflush();
#ifdef MIPS1
	if (cpu_arch == 1)
		mips1_FlushICache((vaddr_t) addr, sizeof(int));
#endif
#ifdef MIPS3
	if (cpu_arch == 3) {
		mips3_HitFlushDCache((vaddr_t) addr, sizeof(int));
		mips3_FlushICache((vaddr_t) addr, sizeof(int));
	}
#endif
}

#if 0 /* UNUSED */
/*
 * Received keyboard interrupt sequence.
 */
void
kdb_kbd_trap(tf)
	int *tf;
{
	if (db_active == 0 && (boothowto & RB_KDB)) {
		printf("\n\nkernel: keyboard interrupt\n");
		ddb_trap(-1, tf);
	}
}
#endif

int
kdb_trap(type, tfp)
	int type;
	mips_reg_t *tfp;	/* struct trapframe */
{
	struct frame *f = (struct frame *)&ddb_regs;

#ifdef notyet
	switch (type) {
	case T_BREAK:		/* breakpoint */
	case -1:		/* keyboard interrupt */
		break;
	default:
		printf("kernel: %s trap", trap_type[type & 0xff]);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
		break;
	}
#endif
	/* Should switch to kdb`s own stack here. */

	if (type & T_USER)
		*f = *(struct frame *)curproc->p_md.md_regs;
	else {
		/* Synthetic full scale register context when trap happens */
		f->f_regs[AST] = tfp[0];
		f->f_regs[V0] = tfp[1];
		f->f_regs[V1] = tfp[2];
		f->f_regs[A0] = tfp[3];
		f->f_regs[A1] = tfp[4];
		f->f_regs[A2] = tfp[5];
		f->f_regs[A3] = tfp[6];
		f->f_regs[T0] = tfp[7];
		f->f_regs[T1] = tfp[8];
		f->f_regs[T2] = tfp[9];
		f->f_regs[T3] = tfp[10];
		f->f_regs[T4] = tfp[11];
		f->f_regs[T5] = tfp[12];
		f->f_regs[T6] = tfp[13];
		f->f_regs[T7] = tfp[14];
		f->f_regs[T8] = tfp[15];
		f->f_regs[T9] = tfp[16];
		f->f_regs[RA] = tfp[17];
		f->f_regs[SR] = tfp[18];
		f->f_regs[MULLO] = tfp[19];
		f->f_regs[MULHI] = tfp[20];
		f->f_regs[PC] = tfp[21];
		f->f_regs[S0] = kdbaux[0];
		f->f_regs[S1] = kdbaux[1];
		f->f_regs[S2] = kdbaux[2];
		f->f_regs[S3] = kdbaux[3];
		f->f_regs[S4] = kdbaux[4];
		f->f_regs[S5] = kdbaux[5];
		f->f_regs[S6] = kdbaux[6];
		f->f_regs[S7] = kdbaux[7];
		f->f_regs[SP] = kdbaux[8];
		f->f_regs[S8] = kdbaux[9];
		f->f_regs[GP] = kdbaux[10];
		/* XXX SR, CAUSE and VADDR XXX */
	}

	db_active++;
	cnpollc(1);
	db_trap(type & ~T_USER, 0 /*code*/);
	cnpollc(0);
	db_active--;

	tfp[21] = f->f_regs[PC]; /* resume */

	return (1);
}

void
Debugger()
{
	asm("break");
}


/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(addr, size, data)
	vaddr_t addr;
	size_t size;
	char *data;
{
	while (size >= 4)
		*((int*)data)++ = kdbpeek(addr), addr += 4, size -= 4;

	if (size) {
		unsigned tmp;
		register char *dst = (char*)data;

		tmp = kdbpeek(addr);
		while (size--) {
#if	BYTE_ORDER == BIG_ENDIAN
			int shift = 24;

			/* want highest -> lowest byte */
			*dst++ = (tmp >> shift) & 0xFF;
			shift -= 8;
#else	/* BYTE_ORDER == LITTLE_ENDIAN */
			*dst++ = tmp & 0xFF;
			tmp >>= 8;
#endif	/* BYTE_ORDER == LITTLE_ENDIAN */
		}
	}
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(addr, size, data)
	vaddr_t addr;
	size_t size;
	char *data;
{
#ifdef DEBUG_DDB
	printf("db_write_bytes(%lx, %d, %p, val %x)\n", addr, size, data,
	       	(addr &3 ) == 0? *(u_int*)addr: -1);
#endif

	while (size >= 4)
		kdbpoke(addr++, *(int*)data), addr += 4, size -= 4;
	if (size) {
		unsigned tmp = kdbpeek(addr), tmp1 = 0;
		register char *src = (char*)data;

		tmp >>= (size << 3);
		tmp <<= (size << 3);
		while (size--) {
			tmp1 <<= 8;
			tmp1 |= src[size] & 0xff;
		}
		kdbpoke(addr, tmp|tmp1);
	}
}


void
db_halt_cmd(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	/*
	 * Force a halt.  Don't sync disks in case we panicked
	 * trying to do just that.
	 */
	cpu_reboot(RB_HALT|RB_NOSYNC, 0);
}

void
db_tlbdump_cmd(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
#ifdef MIPS1
	if (cpu_arch == 1) {
		struct mips1_tlb {
			u_int32_t tlb_hi;
			u_int32_t tlb_lo;
		} tlb;
		int i;
		extern void mips1_TLBRead __P((int, struct mips1_tlb *));

		for (i = 0; i < MIPS1_TLB_NUM_TLB_ENTRIES; i++) {
			mips1_TLBRead(i, &tlb);
			db_printf("TLB%c%2d Hi 0x%08x Lo 0x%08x",
				(tlb.tlb_lo & MIPS1_PG_V) ? ' ' : '*',
				i, tlb.tlb_hi,
				tlb.tlb_lo & MIPS1_PG_FRAME);
			db_printf(" %c%c%c\n",
				(tlb.tlb_lo & MIPS1_PG_M) ? 'M' : ' ',
				(tlb.tlb_lo & MIPS1_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo & MIPS1_PG_N) ? 'N' : ' ');
		}
	}
#endif
#ifdef MIPS3
	if (cpu_arch >= 3) {
		struct tlb tlb;
		int i;

		for (i = 0; i < MIPS3_TLB_NUM_TLB_ENTRIES; i++) {
			mips3_TLBRead(i, &tlb);
			db_printf("TLB%c%2d Hi 0%x08x ",
			(tlb.tlb_lo0 | tlb.tlb_lo1) & MIPS3_PG_V ? ' ' : '*',
				i, tlb.tlb_hi);
			db_printf("Lo0=0x%08x %c%c attr %x",
				(unsigned)pfn_to_vad(tlb.tlb_lo0),
				(tlb.tlb_lo0 & MIPS3_PG_M) ? 'M' : ' ',
				(tlb.tlb_lo0 & MIPS3_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo0 >> 3) & 7);
			db_printf("Lo1=0x%08x %c%c atr %x sz=%x\n", 
				(unsigned)pfn_to_vad(tlb.tlb_lo1),
				(tlb.tlb_lo1 & MIPS3_PG_M) ? 'M' : ' ',
				(tlb.tlb_lo1 & MIPS3_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo1 >> 3) & 7,
				tlb.tlb_mask);
		}
	}
#endif
}

struct db_command mips_db_command_table[] = {
	{ "halt",	db_halt_cmd,		0,	0 },
	{ "tlb",	db_tlbdump_cmd,		0,	0 },
	{ (char *)0, }
};

void
db_machine_init()
{
	db_machine_commands_install(mips_db_command_table);
}

boolean_t
inst_branch(inst)
	int inst;
{
	InstFmt i;
	int delay;

	i.word = inst;
	delay = 0;
	switch (i.JType.op) {
	case OP_BCOND:
	case OP_J:
	case OP_JAL:
	case OP_BEQ:
	case OP_BNE:
	case OP_BLEZ:
	case OP_BGTZ:
	case OP_BEQL:
	case OP_BNEL:
	case OP_BLEZL:
	case OP_BGTZL:
		delay = 1;
		break;

	case OP_COP0:
	case OP_COP1:
		switch (i.RType.rs) {
		case OP_BCx:
		case OP_BCy:
			delay = 1;
		}
		break;
	}
	return delay;
}

boolean_t
inst_call(inst)
	int inst;
{
	boolean_t call;
	InstFmt i;

	i.word = inst;
	if (i.JType.op == OP_SPECIAL
	    && (i.RType.func == OP_JR || i.RType.func == OP_JALR))
		call = 1;
	else if (i.JType.op == OP_JAL)
		call = 1;
	else
		call = 0;
	return call;
}

boolean_t
inst_unconditional_flow_transfer(inst)
	int inst;
{
	InstFmt i;
	boolean_t jump;

	i.word = inst;
	jump = (i.JType.op == OP_J);
	return jump;
}

db_addr_t
branch_taken(inst, pc, regs)
	int inst;
	db_addr_t pc;
	db_regs_t *regs;
{
	vaddr_t ra;
	unsigned fpucsr;

	fpucsr = (curproc) ? curproc->p_addr->u_pcb.pcb_fpregs.r_regs[32] : 0;
	ra = MachEmulateBranch((struct frame *)regs, pc, fpucsr, 0);
	return ra;
}

db_addr_t
next_instr_address(db_addr_t pc, boolean_t bd)
{
	vaddr_t ra;
	unsigned fpucsr;

	fpucsr = (curproc) ? curproc->p_addr->u_pcb.pcb_fpregs.r_regs[32] : 0;
	ra = MachEmulateBranch((struct frame *)&ddb_regs, pc, fpucsr, 1);
	return ra;
}
