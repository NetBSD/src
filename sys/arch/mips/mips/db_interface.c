/*	$NetBSD: db_interface.c,v 1.64.16.8 2009/11/13 05:24:43 cliff Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.64.16.8 2009/11/13 05:24:43 cliff Exp $");

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

#include <machine/int_fmtio.h>
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#ifndef KGDB
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>
#include <ddb/db_lex.h>
#endif

int		db_active = 0;
db_regs_t	ddb_regs;
label_t		kdbaux; /* XXX struct switchframe: better inside curpcb? XXX */

void db_tlbdump_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_kvtophys_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_cp0dump_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_mfcr_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_mtcr_cmd(db_expr_t, bool, db_expr_t, const char *);

static void	kdbpoke_4(vaddr_t addr, int newval);
static void	kdbpoke_2(vaddr_t addr, short newval);
static void	kdbpoke_1(vaddr_t addr, char newval);
static short	kdbpeek_2(vaddr_t addr);
static char	kdbpeek_1(vaddr_t addr);
vaddr_t		MachEmulateBranch(struct frame *, vaddr_t, unsigned, int);

paddr_t kvtophys(vaddr_t);

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
		*curlwp->l_md.md_regs = *f;
	else {
		/* Synthetic full scale register context when trap happens */
		tfp[TF_AST] = f->f_regs[_R_AST];
		tfp[TF_V0] = f->f_regs[_R_V0];
		tfp[TF_V1] = f->f_regs[_R_V1];
		tfp[TF_A0] = f->f_regs[_R_A0];
		tfp[TF_A1] = f->f_regs[_R_A1];
		tfp[TF_A2] = f->f_regs[_R_A2];
		tfp[TF_A3] = f->f_regs[_R_A3];
		tfp[TF_T0] = f->f_regs[_R_T0];
		tfp[TF_T1] = f->f_regs[_R_T1];
		tfp[TF_T2] = f->f_regs[_R_T2];
		tfp[TF_T3] = f->f_regs[_R_T3];
		tfp[TF_TA0] = f->f_regs[_R_TA0];
		tfp[TF_TA1] = f->f_regs[_R_TA1];
		tfp[TF_TA2] = f->f_regs[_R_TA2];
		tfp[TF_TA3] = f->f_regs[_R_TA3];
		tfp[TF_T8] = f->f_regs[_R_T8];
		tfp[TF_T9] = f->f_regs[_R_T9];
		tfp[TF_RA] = f->f_regs[_R_RA];
		tfp[TF_SR] = f->f_regs[_R_SR];
		tfp[TF_MULLO] = f->f_regs[_R_MULLO];
		tfp[TF_MULHI] = f->f_regs[_R_MULHI];
		tfp[TF_EPC] = f->f_regs[_R_PC];
		kdbaux.val[_L_S0] = f->f_regs[_R_S0];
		kdbaux.val[_L_S1] = f->f_regs[_R_S1];
		kdbaux.val[_L_S2] = f->f_regs[_R_S2];
		kdbaux.val[_L_S3] = f->f_regs[_R_S3];
		kdbaux.val[_L_S4] = f->f_regs[_R_S4];
		kdbaux.val[_L_S5] = f->f_regs[_R_S5];
		kdbaux.val[_L_S6] = f->f_regs[_R_S6];
		kdbaux.val[_L_S7] = f->f_regs[_R_S7];
		kdbaux.val[_L_GP] = f->f_regs[_R_GP];
		kdbaux.val[_L_SP] = f->f_regs[_R_SP];
		kdbaux.val[_L_S8] = f->f_regs[_R_S8];
		kdbaux.val[_L_SR] = f->f_regs[_R_SR];
	}

	return (1);
}

void
cpu_Debugger(void)
{

	__asm("break");
}
#endif	/* !KGDB */

void
db_set_ddb_regs(int type, mips_reg_t *tfp)
{
	struct frame *f = (struct frame *)&ddb_regs;
	
	/* Should switch to kdb`s own stack here. */

	if (type & T_USER)
		*f = *curlwp->l_md.md_regs;
	else {
		/* Synthetic full scale register context when trap happens */
		f->f_regs[_R_AST] = tfp[TF_AST];
		f->f_regs[_R_V0] = tfp[TF_V0];
		f->f_regs[_R_V1] = tfp[TF_V1];
		f->f_regs[_R_A0] = tfp[TF_A0];
		f->f_regs[_R_A1] = tfp[TF_A1];
		f->f_regs[_R_A2] = tfp[TF_A2];
		f->f_regs[_R_A3] = tfp[TF_A3];
		f->f_regs[_R_T0] = tfp[TF_T0];
		f->f_regs[_R_T1] = tfp[TF_T1];
		f->f_regs[_R_T2] = tfp[TF_T2];
		f->f_regs[_R_T3] = tfp[TF_T3];
		f->f_regs[_R_TA0] = tfp[TF_TA0];
		f->f_regs[_R_TA1] = tfp[TF_TA1];
		f->f_regs[_R_TA2] = tfp[TF_TA2];
		f->f_regs[_R_TA3] = tfp[TF_TA3];
		f->f_regs[_R_T8] = tfp[TF_T8];
		f->f_regs[_R_T9] = tfp[TF_T9];
		f->f_regs[_R_RA] = tfp[TF_RA];
		f->f_regs[_R_SR] = tfp[TF_SR];
		f->f_regs[_R_MULLO] = tfp[TF_MULLO];
		f->f_regs[_R_MULHI] = tfp[TF_MULHI];
		f->f_regs[_R_PC] = tfp[TF_EPC];
		f->f_regs[_R_S0] = kdbaux.val[_L_S0];
		f->f_regs[_R_S1] = kdbaux.val[_L_S1];
		f->f_regs[_R_S2] = kdbaux.val[_L_S2];
		f->f_regs[_R_S3] = kdbaux.val[_L_S3];
		f->f_regs[_R_S4] = kdbaux.val[_L_S4];
		f->f_regs[_R_S5] = kdbaux.val[_L_S5];
		f->f_regs[_R_S6] = kdbaux.val[_L_S6];
		f->f_regs[_R_S7] = kdbaux.val[_L_S7];
		f->f_regs[_R_GP] = kdbaux.val[_L_GP];
		f->f_regs[_R_SP] = kdbaux.val[_L_SP];
		f->f_regs[_R_S8] = kdbaux.val[_L_S8];
	}
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{
	int *ip;
	short *sp;

	while (size >= 4)
		ip = (int*)data, *ip = kdbpeek(addr), data += 4, addr += 4, size -= 4;
	while (size >= 2)
		sp = (short *)data, *sp = kdbpeek_2(addr), data += 2, addr += 2, size -= 2;
	if (size == 1)
		*data = kdbpeek_1(addr);
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{
	vaddr_t p = addr;
	size_t n = size;

#ifdef DEBUG_DDB
	printf("db_write_bytes(%lx, %d, %p, val %x)\n", addr, size, data,
	       	(addr &3 ) == 0? *(u_int*)addr: -1);
#endif

	while (n >= 4) {
		kdbpoke_4(p, *(const int *)data);
		p += 4;
		data += 4;
		n -= 4;
	}
	if (n >= 2) {
		kdbpoke_2(p, *(const short *)data);
		p += 2;
		data += 2;
		n -= 2;
	}
	if (n == 1) {
		kdbpoke_1(p, *(const char *)data);
	}

	mips_icache_sync_range((vaddr_t) addr, size);
}

#ifndef KGDB
void
db_tlbdump_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
	       const char *modif)
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
			db_printf("Lo0=0x%09" PRIx64 " %c%c attr %x ",
				(uint64_t)mips_tlbpfn_to_paddr(tlb.tlb_lo0),
				(tlb.tlb_lo0 & MIPS3_PG_D) ? 'D' : ' ',
				(tlb.tlb_lo0 & MIPS3_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo0 >> 3) & 7);
			db_printf("Lo1=0x%09" PRIx64 " %c%c attr %x sz=%x\n",
				(uint64_t)mips_tlbpfn_to_paddr(tlb.tlb_lo1),
				(tlb.tlb_lo1 & MIPS3_PG_D) ? 'D' : ' ',
				(tlb.tlb_lo1 & MIPS3_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo1 >> 3) & 7,
				tlb.tlb_mask);
		}
	}
#endif
}

void
db_kvtophys_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		const char *modif)
{

	if (!have_addr)
		return;
	if (VM_MIN_KERNEL_ADDRESS <= addr && addr < VM_MAX_KERNEL_ADDRESS) {
		/*
		 * Cast the physical address -- some platforms, while
		 * being ILP32, may be using 64-bit paddr_t's.
		 */
		db_printf("0x%lx -> 0x%" PRIx64 "\n", addr,
		    (uint64_t) kvtophys(addr));
	} else
		printf("not a kernel virtual address\n");
}

#define	FLDWIDTH	10
#define	SHOW32(reg, name)						\
do {									\
	uint32_t __val;							\
									\
	__asm volatile("mfc0 %0,$" ___STRING(reg) : "=r"(__val));	\
	printf("  %s:%*s %#x\n", name, FLDWIDTH - (int) strlen(name),	\
	    "", __val);							\
} while (0)

/* XXX not 64-bit ABI safe! */
#define	SHOW64(reg, name)						\
do {									\
	uint64_t __val;							\
									\
	__asm volatile(						\
		".set push 			\n\t"			\
		".set mips3			\n\t"			\
		".set noat			\n\t"			\
		"dmfc0 $1,$" ___STRING(reg) "	\n\t"			\
		"dsll %L0,$1,32			\n\t"			\
		"dsrl %L0,%L0,32		\n\t"			\
		"dsrl %M0,$1,32			\n\t"			\
		".set pop"						\
	    : "=r"(__val));						\
	printf("  %s:%*s %#"PRIx64"\n", name, FLDWIDTH - (int) strlen(name), \
	    "", __val);							\
} while (0)

#define	MIPS64_SHOW32(num, sel, name)						\
do {									\
	uint32_t __val;							\
									\
	__asm volatile(							\
		".set push			\n\t"			\
		".set mips64			\n\t"			\
		"mfc0 %0,$" ___STRING(num) "," ___STRING(sel) "\n\t"	\
		".set pop			\n\t"			\
	    : "=r"(__val));						\
	printf("  %s:%*s %#x\n", name, FLDWIDTH - (int) strlen(name),	\
	    "", __val);							\
} while (0)

/* XXX not 64-bit ABI safe! */
#define	MIPS64_SHOW64(num, sel, name)						\
do {									\
	uint64_t __val;							\
									\
	__asm volatile(							\
		".set push 			\n\t"			\
		".set mips64			\n\t"			\
		".set noat			\n\t"			\
		"dmfc0 $1,$" ___STRING(num) "," ___STRING(sel) "\n\t"	\
		"dsll %L0,$1,32			\n\t"			\
		"dsrl %L0,%L0,32		\n\t"			\
		"dsrl %M0,$1,32			\n\t"			\
		".set pop"						\
	    : "=r"(__val));						\
	printf("  %s:%*s %#"PRIx64"\n", name, FLDWIDTH - (int) strlen(name), \
	    "", __val);							\
} while (0)

void
db_cp0dump_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
	       const char *modif)
{
	u_int cp0flags = mycpu->cpu_cp0flags;

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

	if ((cp0flags & MIPS_CP0FL_EIRR) != 0)
		MIPS64_SHOW64(9, 6, "eirr");
	if ((cp0flags & MIPS_CP0FL_EIMR) != 0)
		MIPS64_SHOW64(9, 7, "eimr");

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

	if ((cp0flags & MIPS_CP0FL_USE) != 0) {
		if ((cp0flags & MIPS_CP0FL_EBASE) != 0)
			MIPS64_SHOW32(15, 1, "ebase");
		if ((cp0flags & MIPS_CP0FL_CONFIG) != 0)
			SHOW32(MIPS_COP_0_CONFIG, "config");
		if ((cp0flags & MIPS_CP0FL_CONFIGn(0)) != 0)
			MIPS64_SHOW32(16, 0, "config0");
		if ((cp0flags & MIPS_CP0FL_CONFIGn(1)) != 0)
			MIPS64_SHOW32(16, 1, "config1");
		if ((cp0flags & MIPS_CP0FL_CONFIGn(2)) != 0)
			MIPS64_SHOW32(16, 2, "config2");
		if ((cp0flags & MIPS_CP0FL_CONFIGn(3)) != 0)
			MIPS64_SHOW32(16, 3, "config3");
		if ((cp0flags & MIPS_CP0FL_CONFIGn(4)) != 0)
			MIPS64_SHOW32(16, 4, "config4");
		if ((cp0flags & MIPS_CP0FL_CONFIGn(5)) != 0)
			MIPS64_SHOW32(16, 5, "config5");
		if ((cp0flags & MIPS_CP0FL_CONFIGn(6)) != 0)
			MIPS64_SHOW32(16, 6, "config6");
		if ((cp0flags & MIPS_CP0FL_CONFIGn(7)) != 0)
			MIPS64_SHOW32(16, 7, "config7");
	} else {
		SHOW32(MIPS_COP_0_CONFIG, "config");
#if defined(MIPS32) || defined(MIPS64)
		if (CPUISMIPSNN) {
			uint32_t val;

			val = mipsNN_cp0_config1_read();
			printf("  config1:    %#x\n", val);
		}
#endif
	}

	if (MIPS_HAS_LLSC) {
		if (MIPS_HAS_LLADDR) {
			if (CPUIS64BITS) {
				SHOW64(MIPS_COP_0_LLADDR, "lladdr");
				SHOW64(MIPS_COP_0_WATCH_LO, "watchlo");
			} else {
				SHOW32(MIPS_COP_0_LLADDR, "lladdr");
				SHOW32(MIPS_COP_0_WATCH_LO, "watchlo");
			}
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

		if (((cp0flags & MIPS_CP0FL_USE) == 0) ||
		    ((cp0flags & MIPS_CP0FL_ECC) != 0))
			SHOW32(MIPS_COP_0_ECC, "ecc");

		if (((cp0flags & MIPS_CP0FL_USE) == 0) ||
		    ((cp0flags & MIPS_CP0FL_CACHE_ERR) != 0))
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

void
db_mfcr_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		const char *modif)
{
	uint64_t value;

	if ((mycpu->cpu_flags & CPU_MIPS_HAVE_MxCR) == 0) {
		db_printf("mfcr not implemented on this CPU\n");
		return;
	}

	if (!have_addr) {
		db_printf("Address missing\n");
		return;
	}

	/* value = CR[addr] */
	__asm volatile(							\
		".set push 			\n\t"			\
		".set mips64			\n\t"			\
		".set noat			\n\t"			\
		"mfcr $1,$2			\n\t"			\
		".set pop 			\n\t"			\
	    : "=r"(value) : "r"(addr));
	
	db_printf("control reg 0x%lx = 0x%" PRIx64 "\n", addr, value);
}

void
db_mtcr_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		const char *modif)
{
	db_expr_t value;

	if ((mycpu->cpu_flags & CPU_MIPS_HAVE_MxCR) == 0) {
		db_printf("mtcr not implemented on this CPU\n");
		return;
	}

	if ((!have_addr) || (db_expression(&value) == 0)) {
		db_printf("Address missing\n");
		db_flush_lex();
		return;
        }
	db_skip_to_eol();

	/* CR[addr] = value */
	__asm volatile(							\
		".set push 			\n\t"			\
		".set mips64			\n\t"			\
		".set noat			\n\t"			\
		"mfcr $1,$2			\n\t"			\
		".set pop 			\n\t"			\
	    :: "r"(value), "r"(addr));

	db_printf("control reg 0x%lx = 0x%lx\n", addr, value);
}

const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD("cp0",	db_cp0dump_cmd,		0,
		"Dump CP0 registers.",
		NULL, NULL) },
	{ DDB_ADD_CMD("kvtop",	db_kvtophys_cmd,	0,
		"Print the physical address for a given kernel virtual address",
		"address", 
		"   address:\tvirtual address to look up") },
	{ DDB_ADD_CMD("tlb",	db_tlbdump_cmd,		0,
		"Print out TLB entries. (only works with options DEBUG)",
		NULL, NULL) },
	{ DDB_ADD_CMD("mfcr", 	db_mfcr_cmd,		CS_NOREPEAT,
		"Dump processor control register",
		NULL, NULL) },
	{ DDB_ADD_CMD("mtcr", 	db_mtcr_cmd,		CS_NOREPEAT|CS_MORE,
		"Dump processor control register",
		NULL, NULL) },
	{ DDB_ADD_CMD(NULL,     NULL,               0,  NULL,NULL,NULL) }
};
#endif	/* !KGDB */

/*
 * Determine whether the instruction involves a delay slot.
 */
bool
inst_branch(int inst)
{
	InstFmt i;
	int delslt;

	i.word = inst;
	delslt = 0;
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
		delslt = 1;
		break;

	case OP_COP0:
	case OP_COP1:
		switch (i.RType.rs) {
		case OP_BCx:
		case OP_BCy:
			delslt = 1;
		}
		break;

	case OP_SPECIAL:
		if (i.RType.op == OP_JR || i.RType.op == OP_JALR)
			delslt = 1;
		break;
	}
	return delslt;
}

/*
 * Determine whether the instruction calls a function.
 */
bool
inst_call(int inst)
{
	bool call;
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
bool
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
bool
inst_unconditional_flow_transfer(int inst)
{
	InstFmt i;
	bool jump;

	i.word = inst;
	jump = (i.JType.op == OP_J) ||
	       (i.JType.op == OP_SPECIAL && i.RType.func == OP_JR);
	return jump;
}

/*
 * Determine whether the instruction is a load/store as appropriate.
 */
bool
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

bool
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

	fpucsr = PCB_FSR(&curlwp->l_addr->u_pcb);
	ra = MachEmulateBranch((struct frame *)regs, pc, fpucsr, 0);
	return ra;
}

/*
 * Return the next pc of an arbitrary instruction.
 */
db_addr_t
next_instr_address(db_addr_t pc, bool bd)
{
	unsigned ins;

	if (bd == false)
		return (pc + 4);
	
	if (pc < MIPS_KSEG0_START)
		ins = fuiword((void *)pc);
	else
		ins = *(unsigned *)pc;

	if (inst_branch(ins) || inst_call(ins) || inst_return(ins))
		return (pc + 4);

	return (pc);
}
