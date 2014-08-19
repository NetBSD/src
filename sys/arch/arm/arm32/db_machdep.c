/*	$NetBSD: db_machdep.c,v 1.14.2.3 2014/08/20 00:02:45 tls Exp $	*/

/*
 * Copyright (c) 1996 Mark Brinicombe
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
 */

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_machdep.c,v 1.14.2.3 2014/08/20 00:02:45 tls Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/systm.h>

#include <arm/arm32/db_machdep.h>
#include <arm/cpufunc.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_command.h>
#include <ddb/db_run.h>

#ifndef _KERNEL
#include <stddef.h>
#endif

#ifdef _KERNEL
static long nil;

int db_access_und_sp(const struct db_variable *, db_expr_t *, int);
int db_access_abt_sp(const struct db_variable *, db_expr_t *, int);
int db_access_irq_sp(const struct db_variable *, db_expr_t *, int);
#endif

static int
ddb_reg_var(const struct db_variable *v, db_expr_t *ep, int op)
{
	register_t * const rp = (register_t *)DDB_REGS;
	if (op == DB_VAR_SET) {
		rp[(uintptr_t)v->valuep] = *ep;
	} else {
		*ep = rp[(uintptr_t)v->valuep];
	}
	return 0;
}


#define XO(f) ((long *)(offsetof(db_regs_t, f) / sizeof(register_t)))
const struct db_variable db_regs[] = {
	{ "spsr", XO(tf_spsr), ddb_reg_var, NULL },
	{ "r0", XO(tf_r0), ddb_reg_var, NULL },
	{ "r1", XO(tf_r1), ddb_reg_var, NULL },
	{ "r2", XO(tf_r2), ddb_reg_var, NULL },
	{ "r3", XO(tf_r3), ddb_reg_var, NULL },
	{ "r4", XO(tf_r4), ddb_reg_var, NULL },
	{ "r5", XO(tf_r5), ddb_reg_var, NULL },
	{ "r6", XO(tf_r6), ddb_reg_var, NULL },
	{ "r7", XO(tf_r7), ddb_reg_var, NULL },
	{ "r8", XO(tf_r8), ddb_reg_var, NULL },
	{ "r9", XO(tf_r9), ddb_reg_var, NULL },
	{ "r10", XO(tf_r10), ddb_reg_var, NULL },
	{ "r11", XO(tf_r11), ddb_reg_var, NULL },
	{ "r12", XO(tf_r12), ddb_reg_var, NULL },
	{ "usr_sp", XO(tf_usr_sp), ddb_reg_var, NULL },
	{ "usr_lr", XO(tf_usr_lr), ddb_reg_var, NULL },
	{ "svc_sp", XO(tf_svc_sp), ddb_reg_var, NULL },
	{ "svc_lr", XO(tf_svc_lr), ddb_reg_var, NULL },
	{ "pc", XO(tf_pc), ddb_reg_var, NULL },
#ifdef _KERNEL
	{ "und_sp", &nil, db_access_und_sp, NULL },
	{ "abt_sp", &nil, db_access_abt_sp, NULL },
	{ "irq_sp", &nil, db_access_irq_sp, NULL },
#endif
};
#undef XO

const struct db_variable * const db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD("frame",	db_show_frame_cmd,	0,
			"Displays the contents of a trapframe",
			"[address]",
			"   address:\taddress of trapfame to display")},
#ifdef _KERNEL
	{ DDB_ADD_CMD("fault",	db_show_fault_cmd,	0,
			"Displays the fault registers",
		     	NULL,NULL) },
#endif
#if defined(_KERNEL) && (defined(CPU_CORTEXA5) || defined(CPU_CORTEXA7))
	{ DDB_ADD_CMD("tlb",	db_show_tlb_cmd,	0,
			"Displays the TLB",
		     	NULL,NULL) },
#endif
#if defined(_KERNEL) && defined(MULTIPROCESSOR)
	{ DDB_ADD_CMD("cpu",	db_switch_cpu_cmd,	0,
			"switch to a different cpu",
		     	NULL,NULL) },
#endif

#ifdef ARM32_DB_COMMANDS
	ARM32_DB_COMMANDS,
#endif
	{ DDB_ADD_CMD(NULL,     NULL,           0,NULL,NULL,NULL) }
};

#ifdef _KERNEL
int
db_access_und_sp(const struct db_variable *vp, db_expr_t *valp, int rw)
{

	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_UND32_MODE);
	return(0);
}

int
db_access_abt_sp(const struct db_variable *vp, db_expr_t *valp, int rw)
{

	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_ABT32_MODE);
	return(0);
}

int
db_access_irq_sp(const struct db_variable *vp, db_expr_t *valp, int rw)
{

	if (rw == DB_VAR_GET)
		*valp = get_stackptr(PSR_IRQ32_MODE);
	return(0);
}

void
db_show_fault_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	db_printf("DFAR=%#x DFSR=%#x IFAR=%#x IFSR=%#x\n",
	    armreg_dfar_read(), armreg_dfsr_read(),
	    armreg_ifar_read(), armreg_ifsr_read());
	db_printf("CONTEXTIDR=%#x TTBCR=%#x TTBR=%#x\n",
	    armreg_contextidr_read(), armreg_ttbcr_read(),
	    armreg_ttbr_read());
}

#if defined(CPU_CORTEXA5) || defined(CPU_CORTEXA7)
static void
tlb_print_common_header(const char *str)
{
	db_printf("-W/I-- ----VA---- ----PA---- --SIZE-- D AP XN ASD %s\n", str);
}

static void
tlb_print_addr(size_t way, size_t va_index, vaddr_t vpn, paddr_t pfn)
{
	db_printf("[%1zu:%02zx] 0x%05lx000 0x%05lx000", way, va_index, vpn, pfn);
}

static void
tlb_print_size_domain_prot(const char *sizestr, u_int domain, u_int ap,
    bool xn_p)
{
	db_printf(" %8s %1x %2d %s", sizestr, domain, ap, (xn_p ? "XN" : "--"));
}

static void
tlb_print_asid(bool ng_p, tlb_asid_t asid)
{
	if (ng_p) {
		db_printf(" %3d", asid);
	} else {
		db_printf(" ---");
	}
}

struct db_tlbinfo {
	vaddr_t (*dti_decode_vpn)(size_t, uint32_t, uint32_t);
	void (*dti_print_header)(void);
	void (*dti_print_entry)(size_t, size_t, uint32_t, uint32_t);
	u_int dti_index; 
};

#if defined(CPU_CORTEXA5)
static void
tlb_print_cortex_a5_header(void)
{
	tlb_print_common_header(" S TEX C B");
}

static vaddr_t
tlb_decode_cortex_a5_vpn(size_t va_index, uint32_t d0, uint32_t d1)
{
	const uint64_t d = ((uint64_t)d1 << 32) | d0;

	const u_int size = __SHIFTOUT(d, ARM_A5_TLBDATA_SIZE);
	return __SHIFTOUT(d, ARM_A5_TLBDATA_VA) * (ARM_A5_TLBDATAOP_INDEX + 1)
	    + (va_index << (4*size));
}

static void
tlb_print_cortex_a5_entry(size_t way, size_t va_index, uint32_t d0, uint32_t d1)
{
	static const char size_strings[4][8] = {
	    "  4KB  ", " 64KB  ", "  1MB  ", " 16MB  ",
	};

	const uint64_t d = ((uint64_t)d1 << 32) | d0;

	const paddr_t pfn = __SHIFTOUT(d, ARM_A5_TLBDATA_PA);
	const vaddr_t vpn = tlb_decode_cortex_a5_vpn(va_index, d0, d1);

	tlb_print_addr(way, va_index, vpn, pfn);

	const u_int size = __SHIFTOUT(d, ARM_A5_TLBDATA_SIZE);
	const u_int domain = __SHIFTOUT(d, ARM_A5_TLBDATA_DOM);
	const u_int ap = __SHIFTOUT(d, ARM_A5_TLBDATA_AP);
	const bool xn_p = (d & ARM_A5_TLBDATA_XN) != 0;

	tlb_print_size_domain_prot(size_strings[size], domain, ap, xn_p);

	const bool ng_p = (d & ARM_A5_TLBDATA_nG) != 0;
	const tlb_asid_t asid = __SHIFTOUT(d, ARM_A5_TLBDATA_ASID);

	tlb_print_asid(ng_p, asid);

	const u_int tex = __SHIFTOUT(d, ARM_A5_TLBDATA_TEX);
	const bool c_p = (d & ARM_A5_TLBDATA_C) != 0;
	const bool b_p = (d & ARM_A5_TLBDATA_B) != 0;
	const bool s_p = (d & ARM_A5_TLBDATA_S) != 0;

	db_printf(" %c  %d  %c %c\n", (s_p ? 'S' : '-'), tex,
	    (c_p ? 'C' : '-'), (b_p ? 'B' : '-'));
}

static const struct db_tlbinfo tlb_cortex_a5_info = {
	.dti_decode_vpn = tlb_decode_cortex_a5_vpn,
	.dti_print_header = tlb_print_cortex_a5_header,
	.dti_print_entry = tlb_print_cortex_a5_entry,
	.dti_index = ARM_A5_TLBDATAOP_INDEX,
};
#endif /* CPU_CORTEXA5 */

#if defined(CPU_CORTEXA7)
static const char tlb_cortex_a7_esizes[8][8] = {
    " 4KB(S)", " 4KB(L)", "64KB(S)", "64KB(L)",
    " 1MB(S)", " 2MB(L)", "16MB(S)", " 1GB(L)",
};

static void
tlb_print_cortex_a7_header(void)
{
	tlb_print_common_header("IS --OS- SH");
}

static inline vaddr_t
tlb_decode_cortex_a7_vpn(size_t va_index, uint32_t d0, uint32_t d1)
{
	const u_int size = __SHIFTOUT(d0, ARM_A7_TLBDATA0_SIZE);
	const u_int shift = (size & 1)
	    ? ((0x12090400 >> (8*size)) & 0x1f)
	    : (2 * size);

	return __SHIFTOUT(d0, ARM_A7_TLBDATA0_VA) * (ARM_A7_TLBDATAOP_INDEX + 1)
	    + (va_index << shift);
}

static void
tlb_print_cortex_a7_entry(size_t way, size_t va_index, uint32_t d0, uint32_t d1)
{
	const uint32_t d2 = armreg_tlbdata2_read();
	const uint64_t d01 = ((uint64_t)d1 << 32) | d0;
	const uint64_t d12 = ((uint64_t)d2 << 32) | d1;

	const paddr_t pfn = __SHIFTOUT(d12, ARM_A7_TLBDATA12_PA);
	const vaddr_t vpn = tlb_decode_cortex_a7_vpn(va_index, d0, d1);

	tlb_print_addr(way, va_index, vpn, pfn);

	const u_int size = __SHIFTOUT(d0, ARM_A7_TLBDATA0_SIZE);
	const u_int domain = __SHIFTOUT(d2, ARM_A7_TLBDATA2_DOM);
	const u_int ap = __SHIFTOUT(d1, ARM_A7_TLBDATA1_AP);
	const bool xn_p = (d2 & ARM_A7_TLBDATA2_XN1) != 0;

	tlb_print_size_domain_prot(tlb_cortex_a7_esizes[size], domain, ap, xn_p);

	const bool ng_p = (d1 & ARM_A7_TLBDATA1_nG) != 0;
	const tlb_asid_t asid = __SHIFTOUT(d01, ARM_A7_TLBDATA01_ASID);

	tlb_print_asid(ng_p, asid);

	const u_int is = __SHIFTOUT(d2, ARM_A7_TLBDATA2_IS);
	if (is == ARM_A7_TLBDATA2_IS_DSO) {
		u_int mt = __SHIFTOUT(d2, ARM_A7_TLBDATA2_SDO_MT);
		switch (mt) {
		case ARM_A7_TLBDATA2_SDO_MT_D:
			db_printf(" DV\n");
			return;
		case ARM_A7_TLBDATA2_SDO_MT_SO:
			db_printf(" SO\n");
			return;
		default:
			db_printf(" %02u\n", mt);
			return;
		}
	}
	const u_int os = __SHIFTOUT(d2, ARM_A7_TLBDATA2_OS);
	const u_int sh = __SHIFTOUT(d2, ARM_A7_TLBDATA2_SH);
	static const char is_types[3][3] = { "NC", "WB", "WT" };
	static const char os_types[4][6] = { "NC", "WB+WA", "WT", "WB" };
	static const char sh_types[4][3] = { "NC", "na", "OS", "IS" };
	db_printf(" %2s %5s %2s\n", is_types[is], os_types[os], sh_types[sh]);
}

static const struct db_tlbinfo tlb_cortex_a7_info = {
	.dti_decode_vpn = tlb_decode_cortex_a7_vpn,
	.dti_print_header = tlb_print_cortex_a7_header,
	.dti_print_entry = tlb_print_cortex_a7_entry,
	.dti_index = ARM_A7_TLBDATAOP_INDEX,
};
#endif /* CPU_CORTEXA7 */

static inline const struct db_tlbinfo *
tlb_lookup_tlbinfo(void)
{
#if defined(CPU_CORTEXA5) && defined(CPU_CORTEXA7)
	const bool cortex_a5_p = CPU_ID_CORTEX_A5_P(curcpu()->ci_arm_cpuid);
	const bool cortex_a7_p = CPU_ID_CORTEX_A7_P(curcpu()->ci_arm_cpuid);
#elif defined(CPU_CORTEXA5)
	const bool cortex_a5_p = true;
#else
	const bool cortex_a7_p = true;
#endif
#ifdef CPU_CORTEXA5
	if (cortex_a5_p) {
		return &tlb_cortex_a5_info;
	}
#endif
#ifdef CPU_CORTEXA7
	if (cortex_a7_p) {
		return &tlb_cortex_a7_info;
	}
#endif
	return NULL;
}

void
db_show_tlb_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	const struct db_tlbinfo * const dti = tlb_lookup_tlbinfo();

	if (have_addr) {
		const vaddr_t vpn = (vaddr_t)addr >> L2_S_SHIFT;
		const u_int va_index = vpn & dti->dti_index;
		for (size_t way = 0; way < 2; way++) {
			armreg_tlbdataop_write(
			    __SHIFTIN(va_index, dti->dti_index)
			    | __SHIFTIN(way, ARM_TLBDATAOP_WAY));
			__asm("isb");
			const uint32_t d0 = armreg_tlbdata0_read();
			const uint32_t d1 = armreg_tlbdata1_read();
			if ((d0 & ARM_TLBDATA_VALID)
			    && vpn == (*dti->dti_decode_vpn)(va_index, d0, d1)) {
				(*dti->dti_print_header)();
				(*dti->dti_print_entry)(way, va_index, d0, d1);
				return;
			}
		}
		db_printf("VA %#"DDB_EXPR_FMT"x not found in TLB\n", addr);
		return;
	}

	bool first = true;
	size_t n = 0;
	for (size_t va_index = 0; va_index <= dti->dti_index; va_index++) {
		for (size_t way = 0; way < 2; way++) {
			armreg_tlbdataop_write(
			    __SHIFTIN(way, ARM_TLBDATAOP_WAY)
			    | __SHIFTIN(va_index, dti->dti_index));
			__asm("isb");
			const uint32_t d0 = armreg_tlbdata0_read();
			const uint32_t d1 = armreg_tlbdata1_read();
			if (d0 & ARM_TLBDATA_VALID) {
				if (first) {
					(*dti->dti_print_header)();
					first = false;
				}
				(*dti->dti_print_entry)(way, va_index, d0, d1);
				n++;
			}
		}
	}
	db_printf("%zu TLB valid entries found\n", n);
}
#endif /* CPU_CORTEXA5 || CPU_CORTEXA7 */
#endif /* _KERNEL */


void
db_show_frame_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct trapframe *frame;

	if (!have_addr) {
		db_printf("frame address must be specified\n");
		return;
	}

	frame = (struct trapframe *)addr;

	db_printf("frame address = %08x  ", (u_int)frame);
	db_printf("spsr=%08x\n", frame->tf_spsr);
	db_printf("r0 =%08x r1 =%08x r2 =%08x r3 =%08x\n",
	    frame->tf_r0, frame->tf_r1, frame->tf_r2, frame->tf_r3);
	db_printf("r4 =%08x r5 =%08x r6 =%08x r7 =%08x\n",
	    frame->tf_r4, frame->tf_r5, frame->tf_r6, frame->tf_r7);
	db_printf("r8 =%08x r9 =%08x r10=%08x r11=%08x\n",
	    frame->tf_r8, frame->tf_r9, frame->tf_r10, frame->tf_r11);
	db_printf("r12=%08x r13=%08x r14=%08x r15=%08x\n",
	    frame->tf_r12, frame->tf_usr_sp, frame->tf_usr_lr, frame->tf_pc);
	db_printf("slr=%08x\n", frame->tf_svc_lr);
}

#if defined(_KERNEL) && defined(MULTIPROCESSOR)
void
db_switch_cpu_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	if (addr >= maxcpus) {
		db_printf("cpu %"DDB_EXPR_FMT"d out of range", addr);
		return;
	}
	struct cpu_info *new_ci = cpu_lookup(addr);
	if (new_ci == NULL) {
		db_printf("cpu %"DDB_EXPR_FMT"d does not exist", addr);
		return;
	}
	if (DDB_REGS->tf_spsr & PSR_T_bit) {
		DDB_REGS->tf_pc -= 2; /* XXX */
	} else {
		DDB_REGS->tf_pc -= 4;
	}
	db_newcpu = new_ci;
	db_continue_cmd(0, false, 0, "");
}
#endif
