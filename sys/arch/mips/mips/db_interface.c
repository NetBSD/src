/*	$NetBSD: db_interface.c,v 1.9 1999/01/07 00:36:09 nisimura Exp $	*/

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

#include <mips/cpu.h>
#include <mips/cpuregs.h>
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


void db_halt_cmd __P((db_expr_t addr, int have_addr, db_expr_t count,
         char *modif));

void db_trapdump_cmd __P((db_expr_t addr, int have_addr, db_expr_t count,
	 char *modif));

void db_tlbdump_cmd __P((db_expr_t addr, int have_addr, db_expr_t count,
	 char *modif));

extern int	kdbpeek __P((vaddr_t));
extern void	kdbpoke __P((vaddr_t, int));
extern vaddr_t MachEmulateBranch __P((struct frame *, vaddr_t, unsigned, int));
extern void mips1_dump_tlb __P((int, int, void (*printfn)(const char*, ...)));
extern void mips3_dump_tlb __P((int, int, void (*printfn)(const char*, ...)));


/*
 * kdbpoke -- write a value to a kernel virtual address.
 *    XXX should handle KSEG2 addresses and check for unmapped pages.
 *    XXX user-space addresess?
 * Really belongs wherever kdbpeek() is defined.
 */
void
kdbpoke(addr, newval)
	vaddr_t addr;
	int newval;
{
	*(int*) addr = newval;
	wbflush();
	if (CPUISMIPS3) {
#ifdef MIPS3
		mips3_HitFlushDCache(addr, sizeof(int));
		mips3_FlushICache(addr, sizeof(int));
#endif
	} else {
#ifdef MIPS1
		mips1_FlushICache(addr, sizeof(int));
#endif
	}
}

/*
 * Received keyboard interrupt sequence.
 */
void
kdb_kbd_trap(tf)
	int *tf;
{
	if (db_active == 0 && (boothowto & RB_KDB)) {
		printf("\n\nkernel: keyboard interrupt\n");
		kdb_trap(-1, tf);
	}
}

/*
 *  kdb_trap - field a BREAK exception
 */
int
kdb_trap(type, tf)
	int type;
	int *tf;
{

	/* fb_unblank(); */

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

	/*  XXX copy trapframe to ddb_regs */
	ddb_regs = *(db_regs_t *)tf;

	db_active++;
	cnpollc(1);
	db_trap(type, 0 /*code*/);
	cnpollc(0);
	db_active--;

	/*  write ddb_regs back to trapframe */
	*(db_regs_t *)tf = ddb_regs;

	return (1);
}

void
Debugger()
{
	asm("break ");
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
		char *dst = (char*)data;

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
		char *src = (char*)data;

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
	if (CPUISMIPS3) {
#ifdef MIPS3
		mips3_dump_tlb(0, MIPS3_TLB_NUM_TLB_ENTRIES - 1, db_printf);
#endif
	} else {
#ifdef MIPS1
		mips1_dump_tlb(0, MIPS1_TLB_NUM_TLB_ENTRIES - 1, db_printf);
#endif
	}
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
 * Determine whether the instruction involves a delay slot.
 */
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

/*
 * Determine whether the instruction calls a function.
 */
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

/*
 * Determine whether the instruction makes a jump.
 */
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

/*
 * Return the next pc if the given branch is taken.
 * MachEmulateBranch() runs analysis for branch delay slot.
 */
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

/*
 * Return the next pc of arbitrary instructions.
 * MachEmulateBranch() runs analysis for branch delay slots.
 */
db_addr_t
next_instr_address(pc, bd)
	db_addr_t pc;
	boolean_t bd;
{
	vaddr_t ra;
	unsigned fpucsr;

	fpucsr = (curproc) ? curproc->p_addr->u_pcb.pcb_fpregs.r_regs[32] : 0;
	ra = MachEmulateBranch((struct frame *)&ddb_regs, pc, fpucsr, 1);
	return ra;
}
