/*	$NetBSD: db_interface.c,v 1.6 2002/02/11 17:50:17 uch Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/cpufunc.h>
#include <machine/db_machdep.h>

#include <sh3/ubcreg.h>

#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#include <ddb/db_run.h>
#include <ddb/ddbvar.h>

void kdb_printtrap(u_int, int);
void db_tlbdump_cmd(db_expr_t, int, db_expr_t, char *);
void __db_tlbdump_page_size_sh4(u_int32_t);
void __db_tlbdump_pfn(u_int32_t);

extern label_t *db_recover;
extern char *trap_type[];
extern int trap_types;

const struct db_command db_machine_command_table[] = {
	{ "tlb",	db_tlbdump_cmd,		0,	0 },
	{ 0 }
};

int db_active;

void
kdb_printtrap(type, code)
	u_int type;
	int code;
{
	db_printf("kernel mode trap: ");
	if (type >= trap_types)
		db_printf("type %d", type);
	else
		db_printf("%s", trap_type[type]);
	db_printf(" (code = 0x%x)\n", code);
}

int
kdb_trap(type, code, regs)
	int type, code;
	db_regs_t *regs;
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
	breakpoint();
}

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
inst_call(inst)
	int inst;
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_BSR) == I_BSR || (inst & M_BSRF) == I_BSRF ||
	       (inst & M_JSR) == I_JSR;
}

boolean_t
inst_return(inst)
	int inst;
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_RTS) == I_RTS;
}

boolean_t
inst_trap_return(inst)
	int inst;
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_RTE) == I_RTE;
}

void
db_set_single_step(regs)
	db_regs_t *regs;
{
	SHREG_BBRA = 0;		/* disable break */
	SHREG_BARA = 0;		/* break address */
	SHREG_BASRA = 0;	/* break ASID */
	SHREG_BAMRA = 0x07;	/* break always */
	SHREG_BRCR = 0x400;	/* break after each execution */

	regs->tf_ubc = 0x0014;	/* will be written to BBRA */
}

void
db_clear_single_step(regs)
	db_regs_t *regs;
{
	regs->tf_ubc = 0;
}

void
db_tlbdump_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
#define ON(x, c)	((x) & (c) ? '|' : '.')
	static const char *pr[] = { "_r", "_w", "rr", "ww" };
	static const char title[] = 
	    "   VPN    ASID    PFN  AREA VDCGWtPR  SZ";
	static const char title2[] = "\t\t\t      (user/kernel)";
	int i, cpu_is_sh4;
	u_int32_t r, e, a;
#ifdef SH4
	cpu_is_sh4 = 1;
#else
	cpu_is_sh4 = 0;
#endif	
	if (!cpu_is_sh4) {
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
				    r & SH3_MMUAA_D_VPN_MASK,
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
		printf("%s virtual storage mode,  SQ access: (kernel%s)\n",
		    r & SH3_MMUCR_SV ? "single" : "multiple",
		    r & SH4_MMUCR_SQMD ? "" : "/user");

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
#undef ON
}

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

void
__db_tlbdump_pfn(u_int32_t r)
{
	u_int32_t pa = (r & SH3_MMUDA_D_PPN_MASK);

	printf(" 0x%08x %d", pa, (pa >> 26) & 7);
}
