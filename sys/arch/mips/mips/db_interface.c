/*	$NetBSD: db_interface.c,v 1.38.2.8 2002/06/24 22:05:43 nathanw Exp $	*/

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
#include "opt_kgdb.h"

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <mips/cache.h>
#include <mips/pte.h>
#include <mips/cpu.h>
#include <mips/locore.h>
#include <mips/mips_opcode.h>
#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#ifndef KGDB
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>
#endif

int		db_active = 0;
db_regs_t	ddb_regs;
mips_reg_t	kdbaux[11]; /* XXX struct switchframe: better inside curpcb? XXX */

void db_tlbdump_cmd(db_expr_t, int, db_expr_t, char *);
void db_kvtophys_cmd(db_expr_t, int, db_expr_t, char *);
void db_cp0dump_cmd(db_expr_t, int, db_expr_t, char *);

static void	kdbpoke_4(vaddr_t addr, int newval);
static void	kdbpoke_2(vaddr_t addr, short newval);
static void	kdbpoke_1(vaddr_t addr, char newval);
static short	kdbpeek_2(vaddr_t addr);
static char	kdbpeek_1(vaddr_t addr);
extern vaddr_t	MachEmulateBranch(struct frame *, vaddr_t, unsigned, int);

extern paddr_t kvtophys(vaddr_t);

#ifdef DDB_TRACE
int
kdbpeek(vaddr_t addr)
{

	if (addr == 0 || (addr & 3))
		return 0;
	return *(int *)addr;
}
#endif

static short
kdbpeek_2(vaddr_t addr)
{

	return *(short *)addr;
}

static char
kdbpeek_1(vaddr_t addr)
{

	return *(char *)addr;
}

/*
 * kdbpoke -- write a value to a kernel virtual address.
 *    XXX should handle KSEG2 addresses and check for unmapped pages.
 *    XXX user-space addresess?
 */
static void
kdbpoke_4(vaddr_t addr, int newval)
{

	*(int*) addr = newval;
	wbflush();
}

static void
kdbpoke_2(vaddr_t addr, short newval)
{

	*(short*) addr = newval;
	wbflush();
}

static void
kdbpoke_1(vaddr_t addr, char newval)
{
	*(char*) addr = newval;
	wbflush();
}

#if 0 /* UNUSED */
/*
 * Received keyboard interrupt sequence.
 */
void
kdb_kbd_trap(int *tf)
{

	if (db_active == 0 && (boothowto & RB_KDB)) {
		printf("\n\nkernel: keyboard interrupt\n");
		ddb_trap(-1, tf);
	}
}
#endif

#ifndef KGDB
int
kdb_trap(int type, mips_reg_t /* struct trapframe */ *tfp)
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
	db_set_ddb_regs(type, tfp);

	db_active++;
	cnpollc(1);
	db_trap(type & ~T_USER, 0 /*code*/);
	cnpollc(0);
	db_active--;

	if (type & T_USER)
		*(struct frame *)curlwp->l_md.md_regs = *f;
	else {
		/* Synthetic full scale register context when trap happens */
		tfp[0] = f->f_regs[AST];
		tfp[1] = f->f_regs[V0];
		tfp[2] = f->f_regs[V1];
		tfp[3] = f->f_regs[A0];
		tfp[4] = f->f_regs[A1];
		tfp[5] = f->f_regs[A2];
		tfp[6] = f->f_regs[A3];
		tfp[7] = f->f_regs[T0];
		tfp[8] = f->f_regs[T1];
		tfp[9] = f->f_regs[T2];
		tfp[10] = f->f_regs[T3];
		tfp[11] = f->f_regs[T4];
		tfp[12] = f->f_regs[T5];
		tfp[13] = f->f_regs[T6];
		tfp[14] = f->f_regs[T7];
		tfp[15] = f->f_regs[T8];
		tfp[16] = f->f_regs[T9];
		tfp[17] = f->f_regs[RA];
		tfp[18] = f->f_regs[SR];
		tfp[19] = f->f_regs[MULLO];
		tfp[20] = f->f_regs[MULHI];
		tfp[21] = f->f_regs[PC];
		kdbaux[0] = f->f_regs[S0];
		kdbaux[1] = f->f_regs[S1];
		kdbaux[2] = f->f_regs[S2];
		kdbaux[3] = f->f_regs[S3];
		kdbaux[4] = f->f_regs[S4];
		kdbaux[5] = f->f_regs[S5];
		kdbaux[6] = f->f_regs[S6];
		kdbaux[7] = f->f_regs[S7];
		kdbaux[8] = f->f_regs[SP];
		kdbaux[9] = f->f_regs[S8];
		kdbaux[10] = f->f_regs[GP];
	}

	return (1);
}

void
cpu_Debugger(void)
{

	asm("break");
}
#endif	/* !KGDB */

void
db_set_ddb_regs(int type, mips_reg_t *tfp)
{
	struct frame *f = (struct frame *)&ddb_regs;
	
	/* Should switch to kdb`s own stack here. */

	if (type & T_USER)
		*f = *(struct frame *)curlwp->l_md.md_regs;
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
	}
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{

	while (size >= 4)
		*((int*)data)++ = kdbpeek(addr), addr += 4, size -= 4;
	while (size >= 2)
		*((short*)data)++ = kdbpeek_2(addr), addr += 2, size -= 2;
	if (size == 1)
		*((char*)data)++ = kdbpeek_1(addr);
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t size, char *data)
{
	vaddr_t p = addr;
	size_t n = size;

#ifdef DEBUG_DDB
	printf("db_write_bytes(%lx, %d, %p, val %x)\n", addr, size, data,
	       	(addr &3 ) == 0? *(u_int*)addr: -1);
#endif

	while (n >= 4) {
		kdbpoke_4(p, *(int*)data);
		p += 4;
		data += 4;
		n -= 4;
	}
	if (n >= 2) {
		kdbpoke_2(p, *(short*)data);
		p += 2;
		data += 2;
		n -= 2;
	}
	if (n == 1) {
		kdbpoke_1(p, *(char*)data);
	}

	mips_icache_sync_range((vaddr_t) addr, size);
}

#ifndef KGDB
void
db_tlbdump_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{

#ifdef MIPS1
	if (!MIPS_HAS_R4K_MMU) {
		struct mips1_tlb {
			u_int32_t tlb_hi;
			u_int32_t tlb_lo;
		} tlb;
		int i;
		void mips1_TLBRead(int, struct mips1_tlb *);

		for (i = 0; i < mips_num_tlb_entries; i++) {
			mips1_TLBRead(i, &tlb);
			db_printf("TLB%c%2d Hi 0x%08x Lo 0x%08x",
				(tlb.tlb_lo & MIPS1_PG_V) ? ' ' : '*',
				i, tlb.tlb_hi,
				tlb.tlb_lo & MIPS1_PG_FRAME);
			db_printf(" %c%c%c\n",
				(tlb.tlb_lo & MIPS1_PG_D) ? 'D' : ' ',
				(tlb.tlb_lo & MIPS1_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo & MIPS1_PG_N) ? 'N' : ' ');
		}
	}
#endif
#ifdef MIPS3_PLUS
	if (MIPS_HAS_R4K_MMU) {
		struct tlb tlb;
		int i;

		for (i = 0; i < mips_num_tlb_entries; i++) {
#if defined(MIPS3)
#if defined(MIPS3_5900)
			mips5900_TLBRead(i, &tlb);
#else
			mips3_TLBRead(i, &tlb);
#endif
#elif defined(MIPS32)
			mips32_TLBRead(i, &tlb);
#elif defined(MIPS64)
			mips64_TLBRead(i, &tlb);
#endif
			db_printf("TLB%c%2d Hi 0x%08x ",
			(tlb.tlb_lo0 | tlb.tlb_lo1) & MIPS3_PG_V ? ' ' : '*',
				i, tlb.tlb_hi);
			db_printf("Lo0=0x%08x %c%c attr %x ",
				(unsigned)mips_tlbpfn_to_paddr(tlb.tlb_lo0),
				(tlb.tlb_lo0 & MIPS3_PG_D) ? 'D' : ' ',
				(tlb.tlb_lo0 & MIPS3_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo0 >> 3) & 7);
			db_printf("Lo1=0x%08x %c%c attr %x sz=%x\n",
				(unsigned)mips_tlbpfn_to_paddr(tlb.tlb_lo1),
				(tlb.tlb_lo1 & MIPS3_PG_D) ? 'D' : ' ',
				(tlb.tlb_lo1 & MIPS3_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo1 >> 3) & 7,
				tlb.tlb_mask);
		}
	}
#endif
}

void
db_kvtophys_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{

	if (!have_addr)
		return;
	if (MIPS_KSEG2_START <= addr) {
		/*
		 * Cast the physical address -- some platforms, while
		 * being ILP32, may be using 64-bit paddr_t's.
		 */
		db_printf("0x%lx -> 0x%qx\n", addr,
		    (unsigned long long) kvtophys(addr));
	} else
		printf("not a kernel virtual address\n");
}

#define	FLDWIDTH	10
#define	SHOW32(reg, name)						\
do {									\
	uint32_t __val;							\
									\
	asm volatile("mfc0 %0,$" ___STRING(reg) : "=r"(__val));		\
	printf("  %s:%*s %#x\n", name, FLDWIDTH - (int) strlen(name),	\
	    "", __val);							\
} while (0)

/* XXX not 64-bit ABI safe! */
#define	SHOW64(reg, name)						\
do {									\
	uint64_t __val;							\
									\
	asm volatile(							\
		".set push 			\n\t"			\
		".set mips3			\n\t"			\
		".set noat			\n\t"			\
		"dmfc0 $1,$" ___STRING(reg) "	\n\t"			\
		"dsll %L0,$1,32			\n\t"			\
		"dsrl %L0,%L0,32		\n\t"			\
		"dsrl %M0,$1,32			\n\t"			\
		".set pop"						\
	    : "=r"(__val));						\
	printf("  %s:%*s %#llx\n", name, FLDWIDTH - (int) strlen(name),	\
	    "", __val);							\
} while (0)

void
db_cp0dump_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{

	SHOW32(MIPS_COP_0_TLB_INDEX, "index");
	SHOW32(MIPS_COP_0_TLB_RANDOM, "random");

	if (!MIPS_HAS_R4K_MMU) {
		SHOW32(MIPS_COP_0_TLB_LOW, "entrylow");
	} else {
		if (CPUIS64BITS) {
			SHOW64(MIPS_COP_0_TLB_LO0, "entrylo0");
			SHOW64(MIPS_COP_0_TLB_LO1, "entrylo1");
		} else {
			SHOW32(MIPS_COP_0_TLB_LO0, "entrylo0");
			SHOW32(MIPS_COP_0_TLB_LO1, "entrylo1");
		}
	}

	if (CPUIS64BITS) {
		SHOW64(MIPS_COP_0_TLB_CONTEXT, "context");
	} else {
		SHOW32(MIPS_COP_0_TLB_CONTEXT, "context");
	}

	if (MIPS_HAS_R4K_MMU) {
		SHOW32(MIPS_COP_0_TLB_PG_MASK, "pagemask");
		SHOW32(MIPS_COP_0_TLB_WIRED, "wired");
	}

	if (CPUIS64BITS) {
		SHOW64(MIPS_COP_0_BAD_VADDR, "badvaddr");
	} else {
		SHOW32(MIPS_COP_0_BAD_VADDR, "badvaddr");
	}

	if (cpu_arch >= CPU_ARCH_MIPS3) {
		SHOW32(MIPS_COP_0_COUNT, "count");
	}

	if (CPUIS64BITS) {
		SHOW64(MIPS_COP_0_TLB_HI, "entryhi");
	} else {
		SHOW32(MIPS_COP_0_TLB_HI, "entryhi");
	}

	if (cpu_arch >= CPU_ARCH_MIPS3) {
		SHOW32(MIPS_COP_0_COMPARE, "compare");
	}

	SHOW32(MIPS_COP_0_STATUS, "status");
	SHOW32(MIPS_COP_0_CAUSE, "cause");

	if (CPUIS64BITS) {
		SHOW64(MIPS_COP_0_EXC_PC, "epc");
	} else {
		SHOW32(MIPS_COP_0_EXC_PC, "epc");
	}

	SHOW32(MIPS_COP_0_PRID, "prid");
	SHOW32(MIPS_COP_0_CONFIG, "config");

#if defined(MIPS32) || defined(MIPS64)
	if (CPUISMIPSNN) {
		uint32_t val;

		val = mipsNN_cp0_config1_read();
		printf("  config1:    %#x\n", val);
	}
#endif

	if (MIPS_HAS_LLSC) {
		if (CPUISMIPS64) {
			SHOW64(MIPS_COP_0_LLADDR, "lladdr");
			SHOW64(MIPS_COP_0_WATCH_LO, "watchlo");
		} else {
			SHOW32(MIPS_COP_0_LLADDR, "lladdr");
			SHOW32(MIPS_COP_0_WATCH_LO, "watchlo");
		}

		SHOW32(MIPS_COP_0_WATCH_HI, "watchhi");

		if (CPUIS64BITS) {
			SHOW64(MIPS_COP_0_TLB_XCONTEXT, "xcontext");
		}

		if (CPUISMIPSNN) {
			if (CPUISMIPS64) {
				SHOW64(MIPS_COP_0_PERFCNT, "perfcnt");
			} else {
				SHOW32(MIPS_COP_0_PERFCNT, "perfcnt");
			}
		}

		SHOW32(MIPS_COP_0_ECC, "ecc");
		SHOW32(MIPS_COP_0_CACHE_ERR, "cacherr");
		SHOW32(MIPS_COP_0_TAG_LO, "cachelo");
		SHOW32(MIPS_COP_0_TAG_HI, "cachehi");

		if (CPUIS64BITS) {
			SHOW64(MIPS_COP_0_ERROR_PC, "errorpc");
		} else {
			SHOW32(MIPS_COP_0_ERROR_PC, "errorpc");
		}
	}
}

const struct db_command db_machine_command_table[] = {
	{ "kvtop",	db_kvtophys_cmd,	0,	0 },
	{ "tlb",	db_tlbdump_cmd,		0,	0 },
	{ "cp0",	db_cp0dump_cmd,		0,	0 },
	{ (char *)0, }
};
#endif	/* !KGDB */

/*
 * Determine whether the instruction involves a delay slot.
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

	case OP_SPECIAL:
		if (i.RType.op == OP_JR || i.RType.op == OP_JALR)
			delay = 1;
		break;
	}
	return delay;
}

/*
 * Determine whether the instruction calls a function.
 */
boolean_t
inst_call(int inst)
{
	boolean_t call;
	InstFmt i;

	i.word = inst;
	if (i.JType.op == OP_SPECIAL
	    && ((i.RType.func == OP_JR && i.RType.rs != 31) ||
		i.RType.func == OP_JALR))
		call = 1;
	else if (i.JType.op == OP_JAL)
		call = 1;
	else
		call = 0;
	return call;
}

/*
 * Determine whether the instruction returns from a function (j ra).  The
 * compiler can use this construct for other jumps, but usually will not.
 * This lets the ddb "next" command to work (also need inst_trap_return()).
 */
boolean_t
inst_return(int inst)
{
	InstFmt i;

	i.word = inst;

	return (i.JType.op == OP_SPECIAL && i.RType.func == OP_JR &&
		i.RType.rs == 31);
}

/*
 * Determine whether the instruction makes a jump.
 */
boolean_t
inst_unconditional_flow_transfer(int inst)
{
	InstFmt i;
	boolean_t jump;

	i.word = inst;
	jump = (i.JType.op == OP_J) ||
	       (i.JType.op == OP_SPECIAL && i.RType.func == OP_JR);
	return jump;
}

/*
 * Determine whether the instruction is a load/store as appropriate.
 */
boolean_t
inst_load(int inst)
{
	InstFmt i;

	i.word = inst;

	switch (i.JType.op) {
	case OP_LWC1:
	case OP_LB:
	case OP_LH:
	case OP_LW:
	case OP_LD:
	case OP_LBU:
	case OP_LHU:
	case OP_LWU:
	case OP_LDL:
	case OP_LDR:
	case OP_LWL:
	case OP_LWR:
	case OP_LL:
		return 1;
	default:
		return 0;
	}
}

boolean_t
inst_store(int inst)
{
	InstFmt i;

	i.word = inst;

	switch (i.JType.op) {
	case OP_SWC1:
	case OP_SB:
	case OP_SH:
	case OP_SW:
	case OP_SD:
	case OP_SDL:
	case OP_SDR:
	case OP_SWL:
	case OP_SWR:
	case OP_SCD:
		return 1;
	default:
		return 0;
	}
}

/*
 * Return the next pc if the given branch is taken.
 * MachEmulateBranch() runs analysis for branch delay slot.
 */
db_addr_t
branch_taken(int inst, db_addr_t pc, db_regs_t *regs)
{
	vaddr_t ra;
	unsigned fpucsr;

	fpucsr = curlwp ? PCB_FSR(&curlwp->l_addr->u_pcb) : 0;
	ra = MachEmulateBranch((struct frame *)regs, pc, fpucsr, 0);
	return ra;
}

/*
 * Return the next pc of an arbitrary instruction.
 */
db_addr_t
next_instr_address(db_addr_t pc, boolean_t bd)
{
	unsigned ins;

	if (bd == FALSE)
		return (pc + 4);
	
	if (pc < MIPS_KSEG0_START)
		ins = fuiword((void *)pc);
	else
		ins = *(unsigned *)pc;

	if (inst_branch(ins) || inst_call(ins) || inst_return(ins))
		return (pc + 4);

	return (pc);
}
