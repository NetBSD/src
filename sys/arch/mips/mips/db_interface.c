/*	$NetBSD: db_interface.c,v 1.5.4.4 1998/11/16 10:41:33 nisimura Exp $	*/

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

#include <mips/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <dev/cons.h>


/*
 * forward declarations
 */
int	db_active = 0;


int  ddb_trap __P((int, int *));

void db_halt_cmd __P((db_expr_t addr, int have_addr, db_expr_t count,
         char *modif));

void db_trapdump_cmd __P((db_expr_t addr, int have_addr, db_expr_t count,
	 char *modif));

void db_tlbdump_cmd __P((db_expr_t addr, int have_addr, db_expr_t count,
	 char *modif));

extern int	kdbpeek __P((vaddr_t addr));
extern void	kdbpoke __P((vaddr_t addr, int newval));
extern mips_reg_t MachEmulateBranch __P((mips_reg_t *regsPtr,
     unsigned instPC, unsigned fpcCSR, int allowNonBranch));

/*
 * kdbpoke -- write a value to a kernel virtual address.
 *    XXX should handle KSEG2 addresses and check for unmapped pages.
 *    XXX user-space addresess?
 * Really belongs wherever kdbpeek() is defined.
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
	db_regs_t *tf;
{
	if (db_active == 0 && (boothowto & RB_KDB)) {
		printf("\n\nkernel: keyboard interrupt\n");
		ddb_trap(-1, tf);
	}
}
#endif

/*
 *  ddb_trap - field a BREAK trap
 */
int
ddb_trap(type, tf)
	int type;
	int *tf;
{
	int *regs;

	switch (type) {
	case T_BREAK:		/* breakpoint */
	case -1:		/* keyboard interrupt */
		break;
	default:
#ifdef notyet
		printf("kernel: %s trap", trap_type[type & 0xff]);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
#endif
		break;
	}

	/* Should switch to kdb`s own stack here. */

	memset(&ddb_regs, 0, sizeof ddb_regs);
	regs = (int *)&ddb_regs;
	regs[AST] = tf[0];
	regs[V0] = tf[1];
	regs[V1] = tf[2];
	regs[A0] = tf[3];
	regs[A1] = tf[4];
	regs[A2] = tf[5];
	regs[A3] = tf[6];
	regs[T0] = tf[7];
	regs[T1] = tf[8];
	regs[T2] = tf[9];
	regs[T3] = tf[10];
	regs[T4] = tf[11];
	regs[T5] = tf[12];
	regs[T6] = tf[13];
	regs[T7] = tf[14];
	regs[T8] = tf[15];
	regs[T9] = tf[16];
	regs[RA] = tf[17];
	regs[SR] = tf[18];
	regs[MULLO] = tf[19];
	regs[MULHI] = tf[20];
	regs[PC] = tf[21];

	db_active++;
	cnpollc(1);
	db_trap(type, 0 /*code*/);
	cnpollc(0);
	db_active--;

	/*  write ddb_regs back to trapframe */
	tf[0] = regs[AST];
	tf[1] = regs[V0];
	tf[2] = regs[V1];
	tf[3] = regs[A0];
	tf[4] = regs[A1];
	tf[5] = regs[A2];
	tf[6] = regs[A3];
	tf[7] = regs[T0];
	tf[8] = regs[T1];
	tf[9] = regs[T2];
	tf[10] = regs[T3];
	tf[11] = regs[T4];
	tf[12] = regs[T5];
	tf[13] = regs[T6];
	tf[14] = regs[T7];
	tf[15] = regs[T8];
	tf[16] = regs[T9];
	tf[17] = regs[RA];
	tf[18] = regs[SR];
	tf[19] = regs[MULLO];
	tf[20] = regs[MULHI];
	tf[21] = regs[PC];

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
	register vaddr_t addr;
	register size_t	   size;
	register char *data;
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
	register vaddr_t addr;
	register size_t	   size;
	register char *data;
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
			db_printf("TLB%c%2d vad 0x%08x 0x%08x",
				(tlb.tlb_lo & MIPS1_PG_V) ? ' ' : '*',
				i, tlb.tlb_hi,
				tlb.tlb_lo & MIPS1_PG_FRAME);
			db_printf("%c%c%c\n",
				(tlb.tlb_lo & MIPS1_PG_M) ? 'M' : ' ',
				(tlb.tlb_lo & MIPS1_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo & MIPS1_PG_N) ? 'N' : ' ');
		}
	}
#endif
#ifdef MIPS3
	if (cpu_arch == 3) {
		struct tlb tlb;
		int i;

		for (i = 0; i < MIPS3_TLB_NUM_TLB_ENTRIES; i++) {
			mips3_TLBRead(i, &tlb);
			db_printf("TLB%c%2d vad 0%x08x ",
			(tlb.tlb_lo0 | tlb.tlb_lo1) & MIPS3_PG_V ? ' ' : '*',
				i, tlb.tlb_hi);
			db_printf("0=0x%08x %c%c attr %x",
				(unsigned)pfn_to_vad(tlb.tlb_lo0),
				(tlb.tlb_lo0 & MIPS3_PG_M) ? 'M' : ' ',
				(tlb.tlb_lo0 & MIPS3_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo0 >> 3) & 7);
			db_printf("1=0x%08x %c%c atr %x sz=%x\n", 
				(unsigned)pfn_to_vad(tlb.tlb_lo1),
				(tlb.tlb_lo1 & MIPS3_PG_M) ? 'M' : ' ',
				(tlb.tlb_lo1 & MIPS3_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo1 >> 3) & 7,
				tlb.tlb_mask);
		}
	}
#endif
}

void
db_trapdump_cmd(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
#ifdef DEBUG
	extern void trapDump __P((char*));
	trapDump("CPU exception log:");
#else
	db_printf("trap log only available with options DEBUG.\n");
#endif
}

struct db_command mips_db_command_table[] = {
	{ "halt",	db_halt_cmd,		0,	0 },
	{ "trapdump",	db_trapdump_cmd,	0,	0 },
	{ "tlb",	db_tlbdump_cmd,		0,	0 },
	{ (char *)0, }
};

void
db_machine_init()
{
	db_machine_commands_install(mips_db_command_table);
}


/*
 * MD functions for software single-step.
 * see  db_ for a description.
 */


/*
 *  inst_branch()
 *	returns TRUE iff the instruction might branch.
 */
boolean_t
inst_branch(int inst)
{
	InstFmt i;
	int delay;

	i.word = inst;
	delay = 0;

	switch (i.JType.op) {
	case OP_BCOND:

	case OP_BLEZ:
	case OP_BLEZL:
	case OP_BGTZ:
	case OP_BGTZL:
	case OP_BEQ:
	case OP_BEQL:
	case OP_BNE:
	case OP_BNEL:
		delay = 1;
		break;

	case OP_COP0:
		switch (i.RType.rs) {
		case OP_BCx:
		case OP_BCy:
			delay = 1;
		}
		break;

	case OP_COP1:
		switch (i.RType.rs) {
		case OP_BCx:
		case OP_BCy:
			delay = 1;
			break;
		default:
			break;
		};
		break;

	case OP_J:
	case OP_JAL:
		delay = 1;
		break;

	}
#ifdef DEBUG_DDB
	printf("  inst_branch(0x%x) returns %d\n",
	        inst, delay);
#endif	

	return delay;
}

/*
 *
 */
boolean_t
inst_call(inst)
	int inst;
{
	register int rv = 0;
	InstFmt i;
	i.word = inst;

	if (i.JType.op == OP_SPECIAL) {
		if (i.RType.func == OP_JR || i.RType.func == OP_JAL)
			rv = 1;
	} else if (i.JType.op == OP_JAL)
		rv = 1;

#ifdef DEBUG_DDB
	printf("  inst_call(0x%x) returns 0x%d\n",
	        inst, rv);
#endif	
	return rv;
}


/*
 * inst_unctiondional_flow_transfer()
 *	return TRUE if the instruction is an unconditional
 *	transter of flow (i.e. unconditional branch)
 */
boolean_t
inst_unconditional_flow_transfer(int inst)
{
	InstFmt i;
	int rv = 0;

	if (i.JType.op == OP_J) rv = 1;

#ifdef DEBUG_DDB
	printf("  insn_unconditional_flow_transfer(0x%x) returns %d\n",
	        inst, rv);
#endif	
	return rv;
}

/* 
 * 
 */
db_addr_t
branch_taken(int inst, db_addr_t pc, db_regs_t *regs)
{
	vaddr_t ra;

	ra = (vaddr_t)MachEmulateBranch(regs->f_regs, pc,
           (curproc) ? curproc->p_addr->u_pcb.pcb_fpregs.r_regs[32]: 0, 0);
#ifdef DEBUG_DDB
	printf("  branch_taken(0x%lx) returns 0x%lx\n", pc, ra);
#endif	
	return (ra);
}


/*
 *  Returns the address of the first instruction following the
 *  one at "pc", which is either in the taken path of the branch
 *  (bd == TRUE) or not.  This is for machines (e.g. mips) with
 *  branch delays.
 *
 *  XXX (jrs) The above comment makes no sense. Maybe Jason is thinking
 *    of mips3 squashed branches?
 *  In any case, the kernel support can already find the address of the
 *  next instruction. We could just return that and put a single breakpoint
 *  at that address. All the cruft above can go.
 */
db_addr_t
next_instr_address(db_addr_t pc, boolean_t bd)
{
	
	vaddr_t ra;
	ra = (vaddr_t)MachEmulateBranch(ddb_regs.f_regs, pc,
           (curproc)? curproc->p_addr->u_pcb.pcb_fpregs.r_regs[32]: 0, 1);
#ifdef DEBUG_DDB
	printf("  next_instr_addr(0x%lx, %d) returns 0x%lx\n", pc, bd, ra);
#endif	
	return ((db_addr_t) ra);
}
