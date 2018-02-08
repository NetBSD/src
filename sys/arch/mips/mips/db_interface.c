/*	$NetBSD: db_interface.c,v 1.80 2018/02/08 19:16:24 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.80 2018/02/08 19:16:24 bouyer Exp $");

#include "opt_multiprocessor.h"
#include "opt_cputype.h"	/* which mips CPUs do we support? */
#include "opt_ddb.h"
#include "opt_kgdb.h"

#define __PMAP_PRIVATE

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/atomic.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <mips/asm.h>
#include <mips/regnum.h>
#include <mips/cache.h>
#include <mips/pcb.h>
#include <mips/pte.h>
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
#include <ddb/db_run.h>	/* for db_continue_cmd() proto */
#endif

#define NOCPU   ~0
volatile u_int ddb_cpu = NOCPU;

int		db_active = 0;
db_regs_t	ddb_regs;

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
static void db_watch_cmd(db_expr_t, bool, db_expr_t, const char *);
static void db_unwatch_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif	/* (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0 */

#ifdef MULTIPROCESSOR
static void db_mach_cpu_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif

void db_tlbdump_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_kvtophys_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_cp0dump_cmd(db_expr_t, bool, db_expr_t, const char *);
#ifdef MIPS64_XLS
void db_mfcr_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_mtcr_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif

paddr_t kvtophys(vaddr_t);

CTASSERT(sizeof(ddb_regs) == sizeof(struct reg));

#ifdef DDB_TRACE
bool
kdbpeek(vaddr_t addr, int *valp)
{

	if (addr == 0 || (addr & 3))
		return false;
	*valp = *(int *)addr;
	return true;
}
#endif

#ifndef KGDB
int
kdb_trap(int type, struct reg *regs)
{
	int s;

	switch (type) {
	case T_WATCH:		/* watchpoint */
	case T_BREAK:		/* breakpoint */
		printf("kernel: %s trap\n", trap_names[type & 0x1f]);
		break;
	case -1:		/* keyboard interrupt */
		printf("kernel: kdbint trap\n");
		break;
	default:
		printf("kernel: %s trap\n", trap_names[type & 0x1f]);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
		break;
	}

	s = splhigh();

#if defined(MULTIPROCESSOR)
	bool first_in_ddb = false;
	const u_int cpu_me = cpu_number();
	const u_int old_ddb_cpu = atomic_cas_uint(&ddb_cpu, NOCPU, cpu_me);
	if (old_ddb_cpu == NOCPU) {
		first_in_ddb = true;
		cpu_pause_others();
	} else {
		if (old_ddb_cpu != cpu_me) {
			KASSERT(cpu_is_paused(cpu_me));
			cpu_pause(regs);
			splx(s);
			return 1;
		}
	}
	KASSERT(! cpu_is_paused(cpu_me));
#endif

	ddb_regs = *regs;
	db_active++;
	cnpollc(1);
	db_trap(type & ~T_USER, 0 /*code*/);
	cnpollc(0);
	db_active--;
	*regs = ddb_regs;

#if defined(MULTIPROCESSOR)
	if (atomic_cas_uint(&ddb_cpu, cpu_me, NOCPU) == cpu_me) {
		cpu_resume_others();
	} else {
		cpu_resume(ddb_cpu);
		if (first_in_ddb)
			cpu_pause(regs);
	}
#endif

	splx(s);
	return 1;
}

void
cpu_Debugger(void)
{

	__asm("break");
}
#endif	/* !KGDB */

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{
	const char *src = (char *)addr;

	if (size <= 8 && (size & (size-1)) == 0 && (addr & (size-1)) == 0
	    && ((uintptr_t)data & (size-1)) == 0) {
		if (size == sizeof(uint8_t))
			*(uint8_t *)data = *(const uint8_t *)src;
		else if (size == sizeof(uint16_t))
			*(uint16_t *)data = *(const uint16_t *)src;
		else if (size == sizeof(uint32_t))
			*(uint32_t *)data = *(const uint32_t *)src;
		else
			*(uint64_t *)data = *(const uint64_t *)src;
		return;
	}
	while (size--)
		*data++ = *src++;
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{
	char *p = (char *)addr;
	size_t n = size;

	if (size <= 8 && (size & (size-1)) == 0 && (addr & (size-1)) == 0
	    && ((uintptr_t)data & (size-1)) == 0) {
		if (size == sizeof(uint8_t))
			*(uint8_t *)p = *(const uint8_t *)data;
		else if (size == sizeof(uint16_t))
			*(uint16_t *)p = *(const uint16_t *)data;
		else if (size == sizeof(uint32_t))
			*(uint32_t *)p = *(const uint32_t *)data;
		else
			*(uint64_t *)p = *(const uint64_t *)data;
		return;
	}
	while (n--)
		*p++ = *data++;

	wbflush();
	mips_icache_sync_range(addr, size);
}

#ifndef KGDB
void
db_tlbdump_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
	       const char *modif)
{
	struct tlbmask tlb;

#ifdef MIPS1
	if (!MIPS_HAS_R4K_MMU) {
		int i;

		for (i = 0; i < mips_options.mips_num_tlb_entries; i++) {
			tlb_read_entry(i, &tlb);
			db_printf("TLB%c%2d Hi 0x%08x Lo 0x%08x",
				(tlb.tlb_lo1 & MIPS1_PG_V) ? ' ' : '*',
				i, tlb.tlb_hi,
				tlb.tlb_lo1 & MIPS1_PG_FRAME);
			db_printf(" %c%c%c\n",
				(tlb.tlb_lo1 & MIPS1_PG_D) ? 'D' : ' ',
				(tlb.tlb_lo1 & MIPS1_PG_G) ? 'G' : ' ',
				(tlb.tlb_lo1 & MIPS1_PG_N) ? 'N' : ' ');
		}
	}
#endif
#ifdef MIPS3_PLUS
	if (MIPS_HAS_R4K_MMU) {
		int i;

		for (i = 0; i < mips_options.mips_num_tlb_entries; i++) {
			tlb_read_entry(i, &tlb);
			db_printf("TLB%c%2d Hi 0x%08"PRIxVADDR" ",
			(tlb.tlb_lo0 | tlb.tlb_lo1) & MIPS3_PG_V ? ' ' : '*',
				i, tlb.tlb_hi);
			db_printf("Lo0=0x%09" PRIx64 " %c%c attr %x ",
				(uint64_t)mips_tlbpfn_to_paddr(tlb.tlb_lo0),
				(tlb.tlb_lo0 & MIPS3_PG_D) ? 'D' : ' ',
				(tlb.tlb_lo0 & MIPS3_PG_G) ? 'G' : ' ',
				(int)(tlb.tlb_lo0 >> 3) & 7);
			db_printf("Lo1=0x%09" PRIx64 " %c%c attr %x sz=%x\n",
				(uint64_t)mips_tlbpfn_to_paddr(tlb.tlb_lo1),
				(tlb.tlb_lo1 & MIPS3_PG_D) ? 'D' : ' ',
				(tlb.tlb_lo1 & MIPS3_PG_G) ? 'G' : ' ',
				(int)(tlb.tlb_lo1 >> 3) & 7,
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
		db_printf("0x%" DDB_EXPR_FMT "x -> 0x%" PRIx64 "\n", addr,
		    (uint64_t) kvtophys(addr));
	} else
		printf("not a kernel virtual address\n");
}

#define	FLDWIDTH	10

#define SHOW32(reg, name)	SHOW32SEL(reg, 0, name)
#define SHOW64(reg, name)	SHOW64SEL(reg, 0, name)

#define	SHOW32SEL(num, sel, name)					\
do {									\
	uint32_t __val;							\
									\
	__asm volatile(							\
		".set push			\n\t"			\
		".set mips32			\n\t"			\
		"mfc0 %0,$%1,%2			\n\t"			\
		".set pop			\n\t"			\
	    : "=r"(__val) : "n"(num), "n"(sel));			\
	printf("  %s:%*s %#x\n", name, FLDWIDTH - (int) strlen(name),	\
	    "", __val);							\
} while (0)

/* XXX not 64-bit ABI safe! */
#define	SHOW64SEL(num, sel, name)					\
do {									\
	uint64_t __val;							\
									\
	KASSERT (CPUIS64BITS);						\
	__asm volatile(							\
		".set push 			\n\t"			\
		".set mips64			\n\t"			\
		".set noat			\n\t"			\
		"dmfc0 %0,$%1,%2		\n\t"			\
		".set pop"						\
	    : "=r"(__val) : "n"(num), "n"(sel));			\
	printf("  %s:%*s %#"PRIx64"\n", name, FLDWIDTH - (int) strlen(name), \
	    "", __val);							\
} while (0)

#define	SET32(reg, name, val)						\
do {									\
									\
	__asm volatile("mtc0 %0,$" ___STRING(reg) :: "r"(val));		\
	if (name != NULL)						\
		printf("  %s =%*s %#x\n", name,				\
		    FLDWIDTH - (int) strlen(name), "", val);		\
} while (0)

#define SET64(reg, name)	MIPS64_SET64(reg, 0, name)

#define	MIPS64_SET32(num, sel, name, val)				\
do {									\
									\
	__asm volatile(							\
		".set push			\n\t"			\
		".set mips32			\n\t"			\
		"mtc0 %0,$%1,%2			\n\t"			\
		".set pop			\n\t"			\
	    :: "r"(val), "n"(num), "n"(sel));				\
	if (name != NULL)						\
		printf("  %s =%*s %#x\n", name,				\
		    FLDWIDTH - (int) strlen(name), "", val);		\
} while (0)

/* XXX not 64-bit ABI safe! */
#define	MIPS64_SET64(num, sel, name, val)				\
do {									\
									\
	KASSERT (CPUIS64BITS);						\
	__asm volatile(							\
		".set push 			\n\t"			\
		".set mips64			\n\t"			\
		".set noat			\n\t"			\
		"dmtc0 %0,$%1,%2		\n\t"			\
		".set pop"						\
	    :: "r"(val), "n"(num), "n"(sel));				\
	if (name != NULL)						\
		printf("  %s =%*s %#"PRIx64"\n", name,			\
		    FLDWIDTH - (int) strlen(name), "", (uint64_t)val);	\
} while (0)

void
db_cp0dump_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
	       const char *modif)
{
	u_int cp0flags = mips_options.mips_cpu->cpu_cp0flags;

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

	if (mips_options.mips_cpu_arch >= CPU_ARCH_MIPS3) {
		SHOW32(MIPS_COP_0_COUNT, "count");
	}

	if ((cp0flags & MIPS_CP0FL_USE) != 0) {
		if ((cp0flags & MIPS_CP0FL_EIRR) != 0)
			SHOW64SEL(9, 6, "eirr");
		if ((cp0flags & MIPS_CP0FL_EIMR) != 0)
			SHOW64SEL(9, 7, "eimr");
	}

	if (CPUIS64BITS) {
		SHOW64(MIPS_COP_0_TLB_HI, "entryhi");
	} else {
		SHOW32(MIPS_COP_0_TLB_HI, "entryhi");
	}

	if (mips_options.mips_cpu_arch >= CPU_ARCH_MIPS3) {
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
			SHOW32SEL(15, 1, "ebase");
		if ((cp0flags & MIPS_CP0FL_CONFIG) != 0)
			SHOW32(MIPS_COP_0_CONFIG, "config");
		if ((cp0flags & MIPS_CP0FL_CONFIG1) != 0)
			SHOW32SEL(16, 1, "config1");
		if ((cp0flags & MIPS_CP0FL_CONFIG2) != 0)
			SHOW32SEL(16, 2, "config2");
		if ((cp0flags & MIPS_CP0FL_CONFIG3) != 0)
			SHOW32SEL(16, 3, "config3");
		if ((cp0flags & MIPS_CP0FL_CONFIG4) != 0)
			SHOW32SEL(16, 4, "config4");
		if ((cp0flags & MIPS_CP0FL_CONFIG5) != 0)
			SHOW32SEL(16, 5, "config5");
		if ((cp0flags & MIPS_CP0FL_CONFIG6) != 0)
			SHOW32SEL(16, 6, "config6");
		if ((cp0flags & MIPS_CP0FL_CONFIG7) != 0)
			SHOW32SEL(16, 7, "config7");
		if ((cp0flags & MIPS_CP0FL_HWRENA) != 0)
			SHOW32(7, "hwrena");
		if ((cp0flags & MIPS_CP0FL_USERLOCAL) != 0)
			SHOW32SEL(4, 2, "userlocal");
	} else {
		SHOW32(MIPS_COP_0_CONFIG, "config");
#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
		if (CPUISMIPSNN) {
			uint32_t val;

			val = mipsNN_cp0_config1_read();
			printf("  config1:    %#x\n", val);
		}
#endif
	}

	if (MIPS_HAS_LLSC) {
		if (MIPS_HAS_LLADDR) {
			if (CPUIS64BITS)
				SHOW64(MIPS_COP_0_LLADDR, "lladdr");
			else
				SHOW32(MIPS_COP_0_LLADDR, "lladdr");
		}
	}

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
	for (int i=0; i < curcpu()->ci_cpuwatch_count; i++) {
		const intptr_t lo = mipsNN_cp0_watchlo_read(i);
		const uint32_t hi = mipsNN_cp0_watchhi_read(i);
		printf("  %s%d:%*s %#" PRIxPTR "\t",
		    "watchlo", i, FLDWIDTH - 8, "", lo);
		printf("  %s%d:%*s %#" PRIx32 "\n",
		    "watchhi", i, FLDWIDTH - 8, "", hi);
	}
#endif

	if (CPUIS64BITS) {
		SHOW64(MIPS_COP_0_TLB_XCONTEXT, "xcontext");
	}

	if (CPUISMIPSNN) {
		if (CPUIS64BITS) {
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

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
static void
db_watch_cmd(db_expr_t address, bool have_addr, db_expr_t count,
		 const char *modif)
{
	struct cpu_info * const ci = curcpu();
	cpu_watchpoint_t *cwp;
	register_t mask=0;
	uint32_t asid;
	uint32_t mode;
	db_expr_t value;
	char str[6];

	if (!have_addr) {
		db_printf("%-3s %-5s %-16s %4s %4s\n",
			"#", "MODE", "ADDR", "MASK", "ASID");
		for (u_int i=0; i < ci->ci_cpuwatch_count; i++) {
			cwp = &ci->ci_cpuwatch_tab[i];
			mode = cwp->cw_mode;
			if ((mode & CPUWATCH_RWX) == 0)
				continue;	/* empty/disabled/invalid */
			str[0] = (mode & CPUWATCH_READ)  ?  'r' : '-';
			str[1] = (mode & CPUWATCH_WRITE) ?  'w' : '-';
			str[2] = (mode & CPUWATCH_EXEC)  ?  'x' : '-';
			str[3] = (mode & CPUWATCH_MASK)  ?  'm' : '-';
			str[4] = (mode & CPUWATCH_ASID)  ?  'a' : 'g';
			str[5] = '\0';
			db_printf("%2u: %s %16" PRIxREGISTER
			    " %4" PRIxREGISTER " %4x\n",
			    i, str, cwp->cw_addr, cwp->cw_mask, cwp->cw_asid);
		}
		db_flush_lex();
		return;
	}

	cwp = cpuwatch_alloc();
	if (cwp == NULL) {
		db_printf("no watchpoint available\n");
		db_flush_lex();
		return;
	}

	/*
	 * parse modif to define mode
	 */
	KASSERT(modif != NULL);
	mode = 0;
	for (int i=0; modif[i] != '\0'; i++) {
		switch(modif[i]) {
		case 'w':
			mode |= CPUWATCH_WRITE;
			break;
		case 'm':
			mode |= CPUWATCH_MASK;
			break;
		case 'r':
			mode |= CPUWATCH_READ;
			break;
		case 'x':
			mode |= CPUWATCH_EXEC;
			break;
		case 'a':
			mode |= CPUWATCH_ASID;
			break;
		}
	}
	if (mode == 0) {
		db_printf("mode modifier(s) missing\n");
		db_flush_lex();
		return;
	}

	/*
	 * if mask mode is requested get the mask,
	 */
	if (mode & CPUWATCH_MASK) {
		if (! db_expression(&value)) {
			db_printf("mask missing\n");
			db_flush_lex();
			return;
		}
		mask = (register_t)(value & __BITS(11, 3));
	}

	/*
	 * if asid mode is requested, get the asid;
	 * otherwise use global mode (and set asid=0)
	 */
	if (mode & CPUWATCH_ASID) {
		if (! db_expression(&value)) {
			db_printf("asid missing\n");
			db_flush_lex();
			return;
		}
		asid = (uint32_t)(value & __BITS(7,0));
	} else {
		asid = 0;
	}

	if (mode & (CPUWATCH_MASK|CPUWATCH_ASID))
		db_skip_to_eol();
	else
		db_flush_lex();

	/*
	 * store to the (volatile) table entry
	 * other CPUs can see this and load when resuming from pause
	 */
	cwp->cw_addr = (register_t)address;
	cwp->cw_mask = (register_t)mask;
	cwp->cw_asid = asid;
	cwp->cw_mode = mode;

	/*
	 * program the CPU watchpoint regs
	 */
	cpuwatch_set(cwp);
}

static void
db_unwatch_cmd(db_expr_t address, bool have_addr, db_expr_t count,
		 const char *modif)
{
	struct cpu_info * const ci = curcpu();
	const bool unwatch_all = !have_addr;
	int n;

	n = 0;
	for (u_int i=0; i < ci->ci_cpuwatch_count; i++) {
		cpu_watchpoint_t * const cwp = &ci->ci_cpuwatch_tab[i];
		if (unwatch_all || (cwp->cw_addr == (register_t)address)) {
			cpuwatch_free(cwp);
			n++;
		}
	}
	if (n == 0)
		db_printf("no watch found on address %#" PRIxREGISTER "\n",
		    (register_t)address);
}
#endif	/* (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0 */

#ifdef MIPS64_XLS
void
db_mfcr_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		const char *modif)
{
	uint64_t value;

	if ((mips_options.mips_cpu->cpu_flags & CPU_MIPS_HAVE_MxCR) == 0) {
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
		".set arch=xlr			\n\t"			\
		".set noat			\n\t"			\
		"mfcr %0,%1			\n\t"			\
		".set pop 			\n\t"			\
	    : "=r"(value) : "r"(addr));
	
	db_printf("control reg 0x%" DDB_EXPR_FMT "x = 0x%" PRIx64 "\n",
	    addr, value);
}

void
db_mtcr_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		const char *modif)
{
	db_expr_t value;

	if ((mips_options.mips_cpu->cpu_flags & CPU_MIPS_HAVE_MxCR) == 0) {
		db_printf("mtcr not implemented on this CPU\n");
		return;
	}

	if ((!have_addr) || (! db_expression(&value))) {
		db_printf("Address missing\n");
		db_flush_lex();
		return;
        }
	db_skip_to_eol();

	/* CR[addr] = value */
	__asm volatile(							\
		".set push 			\n\t"			\
		".set arch=xlr			\n\t"			\
		".set noat			\n\t"			\
		"mtcr %0,%1			\n\t"			\
		".set pop 			\n\t"			\
	    :: "r"(value), "r"(addr));

	db_printf("control reg 0x%" DDB_EXPR_FMT "x = 0x%" DDB_EXPR_FMT "x\n",
	    addr, value);
}
#endif /* MIPS64_XLS */

#ifdef MIPS64_OCTEON
#include <mips/cavium/dev/octeon_ciureg.h>

#ifdef MULTIPROCESSOR
static void
db_mach_nmi_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		const char *modif)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	
	if (!have_addr) {
		db_printf("CPU not specific\n");
		return;
	}
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (cpu_index(ci) == addr)
			break;
	}
	if (ci == NULL) {
		db_printf("CPU %ld not configured\n", (long)addr);
		return;
	}
	if (ci == curcpu()) {
		db_printf("CPU %ld is current cpu; request ignored\n",
		    (long)addr);
		return;
	}
	mips3_sd(MIPS_PHYS_TO_XKPHYS_UNCACHED(CIU_NMI), __BIT(ci->ci_cpuid));
}
#endif

static void
db_mach_reset_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		const char *modif)
{
	mips3_sd(MIPS_PHYS_TO_XKPHYS_UNCACHED(CIU_SOFT_RST),
	     mips3_ld(MIPS_PHYS_TO_XKPHYS_UNCACHED(CIU_FUSE)));
}

#endif
const struct db_command db_machine_command_table[] = {
#ifdef MULTIPROCESSOR
	{ DDB_ADD_CMD("cpu",	db_mach_cpu_cmd,	0,
	  "switch to another cpu", "cpu#", NULL) },
#endif
	{ DDB_ADD_CMD("cp0",	db_cp0dump_cmd,	0,
		"Dump CP0 registers.",
		NULL, NULL) },
#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
	{ DDB_ADD_CMD("watch",	db_watch_cmd,		CS_MORE,
		"set cp0 watchpoint",
		"address <mask> <asid> </rwxma>", NULL) },
	{ DDB_ADD_CMD("unwatch",db_unwatch_cmd,		0,
		"delete cp0 watchpoint",
		"address", NULL) },
#endif	/* (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0 */
	{ DDB_ADD_CMD("kvtop",	db_kvtophys_cmd,	0,
		"Print the physical address for a given kernel virtual address",
		"address", 
		"   address:\tvirtual address to look up") },
	{ DDB_ADD_CMD("tlb",	db_tlbdump_cmd,		0,
		"Print out TLB entries. (only works with options DEBUG)",
		NULL, NULL) },
#ifdef MIPS64_XLS
	{ DDB_ADD_CMD("mfcr", 	db_mfcr_cmd,		CS_NOREPEAT,
		"Dump processor control register",
		NULL, NULL) },
	{ DDB_ADD_CMD("mtcr", 	db_mtcr_cmd,		CS_NOREPEAT|CS_MORE,
		"Set processor control register",
		NULL, NULL) },
#endif
#ifdef MIPS64_OCTEON
#ifdef MULTIPROCESSOR
	{ DDB_ADD_CMD("nmi", 	db_mach_nmi_cmd,	CS_NOREPEAT,
		"Send NMI to processor",
		"cpu#", NULL) },
#endif
	{ DDB_ADD_CMD("reset", 	db_mach_reset_cmd,	CS_NOREPEAT,
		"Initiate hardware reset",
		NULL, NULL) },
#endif
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
	case OP_REGIMM:
	case OP_J:
	case OP_JAL:
#if MIPS64_OCTEON
	case OP_CVM_BBIT0:
	case OP_CVM_BBIT032:
	case OP_CVM_BBIT1:
	case OP_CVM_BBIT132:
#endif
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
	InstFmt i = { .word = inst, };

	/*
	 * All loads are opcodes 04x or 06x.
	 */
	if ((i.JType.op & 050) != 040)
		return false;

	/*
	 * Except these this opcode is not a load.
	 */
	return i.JType.op != OP_PREF;
}

bool
inst_store(int inst)
{
	InstFmt i = { .word = inst, };

	/*
	 * All stores are opcodes 05x or 07x.
	 */
	if ((i.JType.op & 050) != 050)
		return false;

	/*
	 * Except these two opcodes are not stores.
	 */
	return i.JType.op != OP_RSVD073 && i.JType.op != OP_CACHE;
}

/*
 * Return the next pc if the given branch is taken.
 * mips_emul_branch() runs analysis for branch delay slot.
 */
db_addr_t
branch_taken(int inst, db_addr_t pc, db_regs_t *regs)
{
	struct pcb * const pcb = lwp_getpcb(curlwp);
	const uint32_t fpucsr = PCB_FSR(pcb);
	vaddr_t ra;

	ra = mips_emul_branch((struct trapframe *)regs, pc, fpucsr, false);
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
		ins = ufetch_uint32((void *)pc);
	else
		ins = *(unsigned *)pc;

	if (inst_branch(ins) || inst_call(ins) || inst_return(ins))
		return (pc + 4);

	return (pc);
}

#ifdef MULTIPROCESSOR

bool 
ddb_running_on_this_cpu_p(void)
{               
	return ddb_cpu == cpu_number();
}

bool 
ddb_running_on_any_cpu_p(void)
{               
	return ddb_cpu != NOCPU;
}

void
db_resume_others(void)
{
	u_int cpu_me = cpu_number();

	if (atomic_cas_uint(&ddb_cpu, cpu_me, NOCPU) == cpu_me)
		cpu_resume_others();
}

static void
db_mach_cpu_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	
	if (!have_addr) {
		cpu_debug_dump();
		return;
	}
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (cpu_index(ci) == addr)
			break;
	}
	if (ci == NULL) {
		db_printf("CPU %ld not configured\n", (long)addr);
		return;
	}
	if (ci != curcpu()) {
		if (!cpu_is_paused(cpu_index(ci))) {
			db_printf("CPU %ld not paused\n", (long)addr);
			return;
		}
		(void)atomic_cas_uint(&ddb_cpu, cpu_number(), cpu_index(ci));
		db_continue_cmd(0, false, 0, "");
	}
}
#endif	/* MULTIPROCESSOR */
