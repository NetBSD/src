/*	$NetBSD: db_interface.c,v 1.3.4.2 2002/03/16 15:59:41 jdolecek Exp $	*/

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

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_run.h>

#include <sh3/ubcreg.h>

extern label_t *db_recover;
extern char *trap_type[];
extern int trap_types;

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
#include <ddb/db_run.h>
#include <ddb/ddbvar.h>

void kdb_printtrap(u_int, int);
void db_tlbdump_cmd(db_expr_t, int, db_expr_t, char *);
void __db_tlbdump_page_size_sh4(u_int32_t);
void __db_tlbdump_pfn(u_int32_t);
void db_cachedump_cmd(db_expr_t, int, db_expr_t, char *);
void __db_cachedump_sh3(vaddr_t);
void __db_cachedump_sh4(vaddr_t);

const struct db_command db_machine_command_table[] = {
	{ "tlb",	db_tlbdump_cmd,		0,	0 },
	{ "cache",	db_cachedump_cmd,	0,	0 },
	{ 0 }
};

int db_active;

void
kdb_printtrap(u_int type, int code)
{

	db_printf("kernel mode trap: ");
	if (type >= trap_types)
		db_printf("type %d", type);
	else
		db_printf("%s", trap_type[type]);
	db_printf(" (code = 0x%x)\n", code);
}

int
kdb_trap(int type, int code, db_regs_t *regs)
{
	int s;

	switch (type) {
	case T_NMI:		/* NMI interrupt */
	case T_TRAP:		/* trapa instruction */
	case T_USERBREAK:	/* UBC */
	case -1:		/* keyboard interrupt */
		break;
	default:
		if (!db_onpanic && db_recover == 0)
			return 0;

		kdb_printtrap(type, code);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* XXX Should switch to kdb's own stack here. */

	ddb_regs = *regs;

	s = splhigh();
	db_active++;
	cnpollc(TRUE);
	db_trap(type, code);
	cnpollc(FALSE);
	db_active--;
	splx(s);

	*regs = ddb_regs;

	return 1;
}

void
cpu_Debugger()
{

	__asm__ __volatile__("trapa #0xc3");
}
#endif /* !KGDB */

#define M_BSR	0xf000
#define I_BSR	0xb000
#define M_BSRF	0xf0ff
#define I_BSRF	0x0003
#define M_JSR	0xf0ff
#define I_JSR	0x400b
#define M_RTS	0xffff
#define I_RTS	0x000b
#define M_RTE	0xffff
#define I_RTE	0x002b

boolean_t
inst_call(int inst)
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_BSR) == I_BSR || (inst & M_BSRF) == I_BSRF ||
	       (inst & M_JSR) == I_JSR;
}

boolean_t
inst_return(int inst)
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_RTS) == I_RTS;
}

boolean_t
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
	_reg_write_4(SH_(BARA), 0);		/* break address */
	_reg_write_1(SH_(BASRA), 0);		/* break ASID */
	_reg_write_1(SH_(BAMRA), 0x07);		/* break always */
	_reg_write_2(SH_(BRCR),  0x400);	/* break after each execution */

	regs->tf_ubc = 0x0014;	/* will be written to BBRA */
}

void
db_clear_single_step(db_regs_t *regs)
{

	regs->tf_ubc = 0;
}

#ifndef KGDB
/*
 * MMU
 */
#define ON(x, c)	((x) & (c) ? '|' : '.')
void
db_tlbdump_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	static const char *pr[] = { "_r", "_w", "rr", "ww" };
	static const char title[] = 
	    "   VPN    ASID    PFN  AREA VDCGWtPR  SZ";
	static const char title2[] = "\t\t\t      (user/kernel)";
	u_int32_t r, e, a;
	int i;

	if (CPU_IS_SH3) {
		/* MMU configuration. */
		r = _reg_read_4(SH3_MMUCR);
		printf("%s-mode, %s virtual storage mode\n",
		    r & SH3_MMUCR_IX
		    ? "ASID + VPN" : "VPN only",
		    r & SH3_MMUCR_SV ? "single" : "multiple");
		printf("---TLB DUMP---\n%s\n%s\n", title, title2);
		for (i = 0; i < SH3_MMU_WAY; i++) {
			printf(" [way %d]\n", i);
			for (e = 0; e < SH3_MMU_ENTRY; e++) {
				/* address/data array common offset. */
				a = (e << SH3_MMU_VPN_SHIFT) |
				    (i << SH3_MMU_WAY_SHIFT);

				r = _reg_read_4(SH3_MMUAA | a);
				printf("0x%08x %3d",
				    r & SH3_MMUAA_D_VPN_MASK_1K,
				    r & SH3_MMUAA_D_ASID_MASK);
				r = _reg_read_4(SH3_MMUDA | a);

				__db_tlbdump_pfn(r);
				printf(" %c%c%c%cx %s %2dK\n",
				    ON(r, SH3_MMUDA_D_V),
				    ON(r, SH3_MMUDA_D_D),
				    ON(r, SH3_MMUDA_D_C),
				    ON(r, SH3_MMUDA_D_SH),
				    pr[(r & SH3_MMUDA_D_PR_MASK) >>
					SH3_MMUDA_D_PR_SHIFT],
				    r & SH3_MMUDA_D_SZ ? 4 : 1);
			}
		}
	} else {
		/* MMU configuration */
		r = _reg_read_4(SH4_MMUCR);
		printf("%s virtual storage mode, SQ access: (kernel%s)\n",
		    r & SH3_MMUCR_SV ? "single" : "multiple",
		    r & SH4_MMUCR_SQMD ? "" : "/user");
		printf("random counter limit=%d\n", (r & SH4_MMUCR_URB_MASK) >>
		    SH4_MMUCR_URB_SHIFT);

		/* Dump ITLB */
		printf("---ITLB DUMP ---\n%s TC SA\n%s\n", title, title2);
		for (i = 0; i < 4; i++) {
			e = i << SH4_ITLB_E_SHIFT;
			r = _reg_read_4(SH4_ITLB_AA | e);
			printf("0x%08x %3d",
			    r & SH4_ITLB_AA_VPN_MASK,
			    r & SH4_ITLB_AA_ASID_MASK);
			r = _reg_read_4(SH4_ITLB_DA1 | e);
			__db_tlbdump_pfn(r);
			printf(" %c_%c%c_ %s ",
			    ON(r, SH4_ITLB_DA1_V),
			    ON(r, SH4_ITLB_DA1_C),
			    ON(r, SH4_ITLB_DA1_SH),
			    pr[(r & SH4_ITLB_DA1_PR) >>
				SH4_UTLB_DA1_PR_SHIFT]);
			__db_tlbdump_page_size_sh4(r);
			r = _reg_read_4(SH4_ITLB_DA2 | e);
			printf(" %c  %d\n",
			    ON(r, SH4_ITLB_DA2_TC), 
			    r & SH4_ITLB_DA2_SA_MASK);
		}
		/* Dump UTLB */
		printf("---UTLB DUMP---\n%s TC SA\n%s\n", title, title2);
		for (i = 0; i < 64; i++) {
			e = i << SH4_UTLB_E_SHIFT;
			r = _reg_read_4(SH4_UTLB_AA | e);
			printf("0x%08x %3d",
			    r & SH4_UTLB_AA_VPN_MASK,
			    r & SH4_UTLB_AA_ASID_MASK);
			r = _reg_read_4(SH4_UTLB_DA1 | e);
			__db_tlbdump_pfn(r);
			printf(" %c%c%c%c%c %s ",
			    ON(r, SH4_UTLB_DA1_V),
			    ON(r, SH4_UTLB_DA1_D),
			    ON(r, SH4_UTLB_DA1_C),
			    ON(r, SH4_UTLB_DA1_SH),
			    ON(r, SH4_UTLB_DA1_WT),
			    pr[(r & SH4_UTLB_DA1_PR_MASK) >>
				SH4_UTLB_DA1_PR_SHIFT]
			    );
			__db_tlbdump_page_size_sh4(r);
			r = _reg_read_4(SH4_UTLB_DA2 | e);
			printf(" %c  %d\n",
			    ON(r, SH4_UTLB_DA2_TC),
			    r & SH4_UTLB_DA2_SA_MASK);
		}
	}
}

void
__db_tlbdump_pfn(u_int32_t r)
{
	u_int32_t pa = (r & SH3_MMUDA_D_PPN_MASK);

	printf(" 0x%08x %d", pa, (pa >> 26) & 7);
}

#ifdef SH4
void
__db_tlbdump_page_size_sh4(u_int32_t r)
{
	switch (r & SH4_PTEL_SZ_MASK) {
	case SH4_PTEL_SZ_1K:
		printf(" 1K");
		break;
	case SH4_PTEL_SZ_4K:
		printf(" 4K");
		break;
	case SH4_PTEL_SZ_64K:
		printf("64K");
		break;
	case SH4_PTEL_SZ_1M:
		printf(" 1M");
		break;
	}
}
#endif /* SH4 */

/*
 * CACHE
 */
void
db_cachedump_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	
	if (CPU_IS_SH3)
		__db_cachedump_sh3(have_addr ? addr : 0);
	else
		__db_cachedump_sh4(have_addr ? addr : 0);
}

#ifdef SH3
void
__db_cachedump_sh3(vaddr_t va_start)
{
	u_int32_t r;
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

	printf("%d-way, way-size=%dB, way-shift=%d, entry-mask=%08x, "
	    "line-size=%dB \n", sh_cache_ways, sh_cache_way_size,
	    sh_cache_way_shift, sh_cache_entry_mask, sh_cache_line_size);
	printf("Entry  Way 0  UV   Way 1  UV   Way 2  UV   Way 3  UV\n");
	for (; va < va_end; va += sh_cache_line_size) {
		entry = va & sh_cache_entry_mask;
		cca = SH3_CCA | entry;
		printf(" %3d ", entry >> CCA_ENTRY_SHIFT);
		for (way = 0; way < sh_cache_ways; way++) {
			r = _reg_read_4(cca | (way << sh_cache_way_shift));
			printf("%08x %c%c ", r & CCA_TAGADDR_MASK,
			    ON(r, CCA_U), ON(r, CCA_V));
		}
		printf("\n");
	}

	/* enable cache */
	_reg_write_4(SH3_CCR, _reg_read_4(SH3_CCR) | SH3_CCR_CE);
	sh_icache_sync_all();

	RUN_P1;
}
#endif /* SH3 */

#ifdef SH4
void
__db_cachedump_sh4(vaddr_t va)
{
	u_int32_t r, e;
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
		
	printf("[I-cache]\n");
	printf("  Entry             V           V           V           V\n");
	for (i = istart; i < iend; i++) {
		if ((i & 3) == 0)
			printf("\n[%3d-%3d] ", i, i + 3);
		r = _reg_read_4(SH4_CCIA | (i << CCIA_ENTRY_SHIFT));
		printf("%08x _%c ", r & CCIA_TAGADDR_MASK, ON(r, CCIA_V));
	}

	printf("\n[D-cache]\n");
	printf("  Entry            UV          UV          UV          UV\n");
	for (i = istart; i < iend; i++) {
		if ((i & 3) == 0)
			printf("\n[%3d-%3d] ", i, i + 3);
		e = (i << CCDA_ENTRY_SHIFT);
		r = _reg_read_4(SH4_CCDA | e);
		printf("%08x %c%c ", r & CCDA_TAGADDR_MASK, ON(r, CCDA_U),
		    ON(r, CCDA_V));

	}
	printf("\n");
	
	_reg_write_4(SH4_CCR,
	    _reg_read_4(SH4_CCR) | SH4_CCR_ICE | SH4_CCR_OCE);
	sh_icache_sync_all();

	RUN_P1;
}
#endif /* SH4 */
#undef ON
#endif /* !KGDB */
