/*	$NetBSD: db_interface.c,v 1.57.12.1 2009/05/13 17:18:22 jym Exp $	*/

/*-
 * Copyright (C) 2002 UCHIYAMA Yasushi.  All rights reserved.
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.57.12.1 2009/05/13 17:18:22 jym Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_kstack_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_run.h>
#include <ddb/db_sym.h>

#include <sh3/ubcreg.h>

db_regs_t ddb_regs;		/* register state */


#ifndef KGDB
#include <sh3/cache.h>
#include <sh3/cache_sh3.h>
#include <sh3/cache_sh4.h>
#include <sh3/mmu.h>
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#include <ddb/ddbvar.h>

static void kdb_printtrap(u_int, int);

static void db_tlbdump_cmd(db_expr_t, bool, db_expr_t, const char *);
static char *__db_procname_by_asid(int);
static void __db_tlbdump_pfn(uint32_t);
#ifdef SH4
static void __db_tlbdump_page_size_sh4(uint32_t);
#endif

static void db_cachedump_cmd(db_expr_t, bool, db_expr_t, const char *);
#ifdef SH3
static void __db_cachedump_sh3(vaddr_t);
#endif
#ifdef SH4
static void __db_cachedump_sh4(vaddr_t);
#endif

static void db_frame_cmd(db_expr_t, bool, db_expr_t, const char *);
static void __db_print_symbol(db_expr_t);
static void __db_print_tfstack(struct trapframe *, struct trapframe *);

static void db_reset_cmd(db_expr_t, bool, db_expr_t, const char *);

#ifdef KSTACK_DEBUG
static void db_stackcheck_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif


const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD("cache", db_cachedump_cmd, 0,
		"Dump contents of the cache address array.",
		"[address]",
		"   address: if specified, dump only matching entries" ) },

	{ DDB_ADD_CMD("frame", db_frame_cmd, 0,
		"Dump switch frame and trap frames of curlwp.",
		NULL, NULL) },

	{ DDB_ADD_CMD("reset", db_reset_cmd, 0,
		"Reset machine (by taking a trap with exceptions disabled).",
		NULL, NULL) },
#ifdef KSTACK_DEBUG
	{ DDB_ADD_CMD("stack", db_stackcheck_cmd, 0,
		"Dump kernel stacks of all lwps.",
		NULL, NULL) },
#endif
	{ DDB_ADD_CMD("tlb", db_tlbdump_cmd, 0,
		"Dump TLB contents.",
		NULL, NULL) },

	{ DDB_ADD_CMD(NULL, NULL, 0, NULL, NULL,NULL) }
};

int db_active;


static void
kdb_printtrap(u_int type, int code)
{
	int i;
	i = type >> 5;

	db_printf("%s mode trap: ", type & 1 ? "user" : "kernel");
	if (i >= exp_types)
		db_printf("type 0x%03x", type & ~1);
	else
		db_printf("%s", exp_type[i]);

	db_printf(" code = 0x%x\n", code);
}

int
kdb_trap(int type, int code, db_regs_t *regs)
{
	int s;

	switch (type) {
	case EXPEVT_TRAPA:	/* FALLTHROUGH */
	case EXPEVT_BREAK:
		break;

	default:
		if (!db_onpanic && db_recover == NULL)
			return 0;

		kdb_printtrap(type, code);
		if (db_recover != NULL) {
			db_printf("[pc %x, pr %x]: ",
				  regs->tf_spc, regs->tf_pr);
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* XXX: Should switch to ddb's own stack here. */

	ddb_regs = *regs;

	s = splhigh();
	db_active++;
	cnpollc(true);

	db_trap(type, code);

	cnpollc(false);
	db_active--;
	splx(s);

	*regs = ddb_regs;

	return 1;
}

void
cpu_Debugger(void)
{

	__asm volatile("trapa %0" :: "i"(_SH_TRA_BREAK));
}
#endif /* !KGDB */

#define	M_BSR	0xf000
#define	I_BSR	0xb000
#define	M_BSRF	0xf0ff
#define	I_BSRF	0x0003
#define	M_JSR	0xf0ff
#define	I_JSR	0x400b
#define	M_RTS	0xffff
#define	I_RTS	0x000b
#define	M_RTE	0xffff
#define	I_RTE	0x002b

bool
inst_call(int inst)
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_BSR) == I_BSR || (inst & M_BSRF) == I_BSRF ||
	       (inst & M_JSR) == I_JSR;
}

bool
inst_return(int inst)
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_RTS) == I_RTS;
}

bool
inst_trap_return(int inst)
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_RTE) == I_RTE;
}

void
db_set_single_step(db_regs_t *regs)
{

	_reg_write_2(SH_(BBRA), 0);		/* disable break */

#ifdef SH3
	if (CPU_IS_SH3) {
		/* A: compare all address bits */
		_reg_write_4(SH_(BAMRA), 0x00000000);

		/* A: break after execution, ignore ASID */
		_reg_write_4(SH_(BRCR), (UBC_CTL_A_AFTER_INSN
					 | SH3_UBC_CTL_A_MASK_ASID));

		/* will be written to BBRA before RTE */
		regs->tf_ubc = UBC_CYCLE_INSN | UBC_CYCLE_READ
			| SH3_UBC_CYCLE_CPU;
	}
#endif	/* SH3 */

#ifdef SH4
	if (CPU_IS_SH4) {
		/* A: compare all address bits, ignore ASID */
		_reg_write_1(SH_(BAMRA), SH4_UBC_MASK_NONE | SH4_UBC_MASK_ASID);

		/* A: break after execution */
		_reg_write_2(SH_(BRCR), UBC_CTL_A_AFTER_INSN);

		/* will be written to BBRA before RTE */
		regs->tf_ubc = UBC_CYCLE_INSN | UBC_CYCLE_READ;
	}
#endif	/* SH4 */
}

void
db_clear_single_step(db_regs_t *regs)
{

	regs->tf_ubc = 0;
}

#ifndef KGDB

#define	ON(x, c)	((x) & (c) ? '|' : '.')

/*
 * MMU
 */
static void
db_tlbdump_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	static const char *pr[] = { "_r", "_w", "rr", "ww" };
	static const char title[] =
	    "   VPN      ASID    PFN  AREA VDCGWtPR  SZ";
	static const char title2[] =
	    "          U/K                       U/K";
	uint32_t r, e;
	int i;
#ifdef SH3
	if (CPU_IS_SH3) {
		/* MMU configuration. */
		r = _reg_read_4(SH3_MMUCR);
		db_printf("%s-mode, %s virtual storage mode\n",
		    r & SH3_MMUCR_IX
		    ? "ASID + VPN" : "VPN only",
		    r & SH3_MMUCR_SV ? "single" : "multiple");
		i = _reg_read_4(SH3_PTEH) & SH3_PTEH_ASID_MASK;
		db_printf("ASID=%d (%s)", i, __db_procname_by_asid(i));

		db_printf("---TLB DUMP---\n%s\n%s\n", title, title2);
		for (i = 0; i < SH3_MMU_WAY; i++) {
			db_printf(" [way %d]\n", i);
			for (e = 0; e < SH3_MMU_ENTRY; e++) {
				uint32_t a;
				/* address/data array common offset. */
				a = (e << SH3_MMU_VPN_SHIFT) |
				    (i << SH3_MMU_WAY_SHIFT);

				r = _reg_read_4(SH3_MMUAA | a);
				if (r == 0) {
					db_printf("---------- - --- ----------"
					    " - ----x --  --\n");
				} else {
					vaddr_t va;
					int asid;
					asid = r & SH3_MMUAA_D_ASID_MASK;
					r &= SH3_MMUAA_D_VPN_MASK_1K;
					va = r | (e << SH3_MMU_VPN_SHIFT);
					db_printf("0x%08lx %c %3d", va,
					    (int)va < 0 ? 'K' : 'U', asid);

					r = _reg_read_4(SH3_MMUDA | a);
					__db_tlbdump_pfn(r);

					db_printf(" %c%c%c%cx %s %2dK\n",
					    ON(r, SH3_MMUDA_D_V),
					    ON(r, SH3_MMUDA_D_D),
					    ON(r, SH3_MMUDA_D_C),
					    ON(r, SH3_MMUDA_D_SH),
					    pr[(r & SH3_MMUDA_D_PR_MASK) >>
						SH3_MMUDA_D_PR_SHIFT],
					    r & SH3_MMUDA_D_SZ ? 4 : 1);
				}
			}
		}
	}
#endif /* SH3 */
#ifdef SH4
	if (CPU_IS_SH4) {
		uint32_t aa, da1, da2;

		/* MMU configuration */
		r = _reg_read_4(SH4_MMUCR);
		db_printf("%s virtual storage mode, SQ access: (kernel%s)\n",
		    r & SH3_MMUCR_SV ? "single" : "multiple",
		    r & SH4_MMUCR_SQMD ? "" : "/user");
		db_printf("random counter limit=%d\n",
		    (r & SH4_MMUCR_URB_MASK) >> SH4_MMUCR_URB_SHIFT);

		i = _reg_read_4(SH4_PTEH) & SH4_PTEH_ASID_MASK;
		db_printf("ASID=%d (%s)", i, __db_procname_by_asid(i));

		/* Dump ITLB */
		db_printf("---ITLB DUMP ---\n%s TC SA\n%s\n", title, title2);
		for (i = 0; i < 4; i++) {
			e = i << SH4_ITLB_E_SHIFT;

			RUN_P2;
			aa = _reg_read_4(SH4_ITLB_AA | e);
			da1 = _reg_read_4(SH4_ITLB_DA1 | e);
			da2 = _reg_read_4(SH4_ITLB_DA2 | e);
			RUN_P1;

			db_printf("0x%08x   %3d",
			    aa & SH4_ITLB_AA_VPN_MASK,
			    aa & SH4_ITLB_AA_ASID_MASK);

			__db_tlbdump_pfn(da1);
			db_printf(" %c_%c%c_ %s ",
			    ON(da1, SH4_ITLB_DA1_V),
			    ON(da1, SH4_ITLB_DA1_C),
			    ON(da1, SH4_ITLB_DA1_SH),
			    pr[(da1 & SH4_ITLB_DA1_PR) >>
				SH4_UTLB_DA1_PR_SHIFT]);
			__db_tlbdump_page_size_sh4(da1);

			db_printf(" %c  %d\n",
			    ON(da2, SH4_ITLB_DA2_TC),
			    da2 & SH4_ITLB_DA2_SA_MASK);
		}

		/* Dump UTLB */
		db_printf("---UTLB DUMP---\n%s TC SA\n%s\n", title, title2);
		for (i = 0; i < 64; i++) {
			e = i << SH4_UTLB_E_SHIFT;

			RUN_P2;
			aa = _reg_read_4(SH4_UTLB_AA | e);
			da1 = _reg_read_4(SH4_UTLB_DA1 | e);
			da2 = _reg_read_4(SH4_UTLB_DA2 | e);
			RUN_P1;

			db_printf("0x%08x   %3d",
			    aa & SH4_UTLB_AA_VPN_MASK,
			    aa & SH4_UTLB_AA_ASID_MASK);

			__db_tlbdump_pfn(da1);
			db_printf(" %c%c%c%c%c %s ",
			    ON(da1, SH4_UTLB_DA1_V),
			    ON(da1, SH4_UTLB_DA1_D),
			    ON(da1, SH4_UTLB_DA1_C),
			    ON(da1, SH4_UTLB_DA1_SH),
			    ON(da1, SH4_UTLB_DA1_WT),
			    pr[(da1 & SH4_UTLB_DA1_PR_MASK) >>
				SH4_UTLB_DA1_PR_SHIFT]
			    );
			__db_tlbdump_page_size_sh4(da1);

			db_printf(" %c  %d\n",
			    ON(da2, SH4_UTLB_DA2_TC),
			    da2 & SH4_UTLB_DA2_SA_MASK);
		}
	}
#endif /* SH4 */
}

static void
__db_tlbdump_pfn(uint32_t r)
{
	uint32_t pa = (r & SH3_MMUDA_D_PPN_MASK);

	db_printf(" 0x%08x %d", pa, (pa >> 26) & 7);
}

static char *
__db_procname_by_asid(int asid)
{
	static char notfound[] = "---";
	struct proc *p;

	LIST_FOREACH(p, &allproc, p_list) {
		if (p->p_vmspace->vm_map.pmap->pm_asid == asid)
			return (p->p_comm);
	}

	return (notfound);
}

#ifdef SH4
static void
__db_tlbdump_page_size_sh4(uint32_t r)
{
	switch (r & SH4_PTEL_SZ_MASK) {
	case SH4_PTEL_SZ_1K:
		db_printf(" 1K");
		break;
	case SH4_PTEL_SZ_4K:
		db_printf(" 4K");
		break;
	case SH4_PTEL_SZ_64K:
		db_printf("64K");
		break;
	case SH4_PTEL_SZ_1M:
		db_printf(" 1M");
		break;
	}
}
#endif /* SH4 */

/*
 * CACHE
 */
static void
db_cachedump_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
#ifdef SH3
	if (CPU_IS_SH3)
		__db_cachedump_sh3(have_addr ? addr : 0);
#endif
#ifdef SH4
	if (CPU_IS_SH4)
		__db_cachedump_sh4(have_addr ? addr : 0);
#endif
}

#ifdef SH3
static void
__db_cachedump_sh3(vaddr_t va_start)
{
	uint32_t r;
	vaddr_t va, va_end, cca;
	int entry, way;

	RUN_P2;
	/* disable cache */
	_reg_write_4(SH3_CCR,
	    _reg_read_4(SH3_CCR) & ~SH3_CCR_CE);

	if (va_start) {
		va = va_start & ~(sh_cache_line_size - 1);
		va_end = va + sh_cache_line_size;
	} else {
		va = 0;
		va_end = sh_cache_way_size;
	}

	db_printf("%d-way, way-size=%dB, way-shift=%d, entry-mask=%08x, "
	    "line-size=%dB \n", sh_cache_ways, sh_cache_way_size,
	    sh_cache_way_shift, sh_cache_entry_mask, sh_cache_line_size);
	db_printf("Entry  Way 0  UV   Way 1  UV   Way 2  UV   Way 3  UV\n");
	for (; va < va_end; va += sh_cache_line_size) {
		entry = va & sh_cache_entry_mask;
		cca = SH3_CCA | entry;
		db_printf(" %3d ", entry >> CCA_ENTRY_SHIFT);
		for (way = 0; way < sh_cache_ways; way++) {
			r = _reg_read_4(cca | (way << sh_cache_way_shift));
			db_printf("%08x %c%c ", r & CCA_TAGADDR_MASK,
			    ON(r, CCA_U), ON(r, CCA_V));
		}
		db_printf("\n");
	}

	/* enable cache */
	_reg_bset_4(SH3_CCR, SH3_CCR_CE);
	sh_icache_sync_all();

	RUN_P1;
}
#endif /* SH3 */

#ifdef SH4
static void
__db_cachedump_sh4(vaddr_t va)
{
	uint32_t r, e;
	int i, istart, iend;

	RUN_P2; /* must access from P2 */

	/* disable I/D-cache */
	_reg_write_4(SH4_CCR,
	    _reg_read_4(SH4_CCR) & ~(SH4_CCR_ICE | SH4_CCR_OCE));

	if (va) {
		istart = ((va & CCIA_ENTRY_MASK) >> CCIA_ENTRY_SHIFT) & ~3;
		iend = istart + 4;
	} else {
		istart = 0;
		iend = SH4_ICACHE_SIZE / SH4_CACHE_LINESZ;
	}

	db_printf("[I-cache]\n");
	db_printf("  Entry             V           V           V           V");
	for (i = istart; i < iend; i++) {
		if ((i & 3) == 0)
			db_printf("\n[%3d-%3d] ", i, i + 3);
		r = _reg_read_4(SH4_CCIA | (i << CCIA_ENTRY_SHIFT));
		db_printf("%08x _%c ", r & CCIA_TAGADDR_MASK, ON(r, CCIA_V));
	}

	db_printf("\n\n[D-cache]\n");
	db_printf("  Entry            UV          UV          UV          UV");
	for (i = istart; i < iend; i++) {
		if ((i & 3) == 0)
			db_printf("\n[%3d-%3d] ", i, i + 3);
		e = (i << CCDA_ENTRY_SHIFT);
		r = _reg_read_4(SH4_CCDA | e);
		db_printf("%08x %c%c ", r & CCDA_TAGADDR_MASK, ON(r, CCDA_U),
		    ON(r, CCDA_V));

	}
	db_printf("\n");

	_reg_write_4(SH4_CCR,
	    _reg_read_4(SH4_CCR) | SH4_CCR_ICE | SH4_CCR_OCE);
	sh_icache_sync_all();

	RUN_P1;
}
#endif /* SH4 */

#undef ON

static void
db_frame_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct switchframe *sf = &curpcb->pcb_sf;
	struct trapframe *tf, *tfbot;

	/* Print switch frame */
	db_printf("[switch frame]\n");

#define	SF(x)	db_printf("sf_" #x "\t\t0x%08x\t", sf->sf_ ## x);	\
		__db_print_symbol(sf->sf_ ## x)

	SF(sr);
	SF(pr);
	SF(gbr);
	SF(r8);
	SF(r9);
	SF(r10);
	SF(r11);
	SF(r12);
	SF(r13);
	SF(r14);
	SF(r15);
	db_printf("sf_r6_bank\t0x%08x\n", sf->sf_r6_bank);
	db_printf("sf_r7_bank\t0x%08x\n", sf->sf_r7_bank);
#undef	SF


	/* Print trap frame stack */
	tfbot = (struct trapframe *)((vaddr_t)curpcb + PAGE_SIZE);

	__asm("stc r6_bank, %0" : "=r"(tf));

	db_printf("[trap frames]\n");
	__db_print_tfstack(tf, tfbot);
}


static void
__db_print_tfstack(struct trapframe *tf, struct trapframe *tfbot)
{
	db_printf("[[-- dumping frames from 0x%08x to 0x%08x --]]\n",
		  (uint32_t)tf, (uint32_t)tfbot);

	for (; tf != tfbot; tf++) {
		db_printf("-- %p-%p --\n", tf, tf + 1);
		db_printf("tf_expevt\t0x%08x\n", tf->tf_expevt);

#define	TF(x)	db_printf("tf_" #x "\t\t0x%08x\t", tf->tf_ ## x);	\
		__db_print_symbol(tf->tf_ ## x)

		TF(ubc);
		TF(ssr);
		TF(spc);
		TF(pr);
		TF(gbr);
		TF(macl);
		TF(mach);
		TF(r0);
		TF(r1);
		TF(r2);
		TF(r3);
		TF(r4);
		TF(r5);
		TF(r6);
		TF(r7);
		TF(r8);
		TF(r9);
		TF(r10);
		TF(r11);
		TF(r12);
		TF(r13);
		TF(r14);
		TF(r15);
#undef	TF
	}
}


static void
__db_print_symbol(db_expr_t value)
{
	const char *name;
	db_expr_t offset;

	db_find_sym_and_offset((db_addr_t)value, &name, &offset);
	if (name != NULL && offset <= db_maxoff && offset != value)
		db_printsym(value, DB_STGY_ANY, db_printf);

	db_printf("\n");
}

#ifdef KSTACK_DEBUG
/*
 * Stack overflow check
 */
static void
db_stackcheck_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		  const char *modif)
{
	struct lwp *l;
	struct user *u;
	struct pcb *pcb;
	uint32_t *t32;
	uint8_t *t8;
	int i, j;

#define	MAX_STACK	(USPACE - PAGE_SIZE)
#define	MAX_FRAME	(PAGE_SIZE - sizeof(struct user))

	db_printf("stack max: %d byte, frame max %d byte,"
	    " sizeof(struct trapframe) %d byte\n", MAX_STACK, MAX_FRAME,
	    sizeof(struct trapframe));
	db_printf("   PID.LID    "
		  "stack bot    max used    frame bot     max used"
		  "  nest\n");

	LIST_FOREACH(l, &alllwp, l_list) {
		u = l->l_addr;
		pcb = &u->u_pcb;
		/* stack */
		t32 = (uint32_t *)(pcb->pcb_sf.sf_r7_bank - MAX_STACK);
		for (i = 0; *t32++ == 0xa5a5a5a5; i++)
			continue;
		i = MAX_STACK - i * sizeof(int);

		/* frame */
		t8 = (uint8_t *)((vaddr_t)pcb + PAGE_SIZE - MAX_FRAME);
		for (j = 0; *t8++ == 0x5a; j++)
			continue;
		j = MAX_FRAME - j;

		db_printf("%6d.%-6d 0x%08x %6d (%3d%%) 0x%08lx %6d (%3d%%) %d %s\n",
		    l->l_proc->p_pid, l->l_lid,
		    pcb->pcb_sf.sf_r7_bank, i, i * 100 / MAX_STACK,
		    (vaddr_t)pcb + PAGE_SIZE, j, j * 100 / MAX_FRAME,
		    j / sizeof(struct trapframe),
		    l->l_proc->p_comm);
	}
#undef	MAX_STACK
#undef	MAX_FRAME
}
#endif /* KSTACK_DEBUG */


static void
db_reset_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
	     const char *modif)
{
    _cpu_exception_suspend();
    __asm volatile("trapa %0" :: "i"(_SH_TRA_BREAK));

    /* NOTREACHED, but just in case ... */
    printf("Reset failed\n");
}

#endif /* !KGDB */
