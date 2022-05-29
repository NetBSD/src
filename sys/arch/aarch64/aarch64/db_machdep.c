/* $NetBSD: db_machdep.c,v 1.44 2022/05/29 16:45:00 ryo Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_machdep.c,v 1.44 2022/05/29 16:45:00 ryo Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd32.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/lwp.h>
#include <sys/intr.h>

#include <uvm/uvm.h>

#include <arm/cpufunc.h>

#include <aarch64/db_machdep.h>
#include <aarch64/armreg.h>
#include <aarch64/locore.h>
#include <aarch64/pmap.h>

#include <arm/cpufunc.h>

#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_proc.h>
#include <ddb/db_variables.h>
#include <ddb/db_run.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>
#include <ddb/db_user.h>

#include <dev/cons.h>

void db_md_cpuinfo_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_md_frame_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_md_lwp_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_md_pte_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_md_reset_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_md_tlbi_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_md_ttbr_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_md_sysreg_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_md_break_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_md_watch_cmd(db_expr_t, bool, db_expr_t, const char *);
#if defined(_KERNEL) && defined(MULTIPROCESSOR)
void db_md_switch_cpu_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif
#if defined(_KERNEL)
static void db_md_meminfo_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif

#ifdef _KERNEL
#define MAX_BREAKPOINT	15
#define MAX_WATCHPOINT	15
/* The number varies depending on the CPU core (e.g. big.LITTLE) */
static int max_breakpoint = MAX_BREAKPOINT;
static int max_watchpoint = MAX_WATCHPOINT;

struct breakpoint_info {
	db_addr_t addr;
};
static struct breakpoint_info breakpoint_buf[MAX_BREAKPOINT + 1];

struct watchpoint_info {
	db_addr_t addr;
	int size;
	int accesstype;
};
static struct watchpoint_info watchpoint_buf[MAX_WATCHPOINT + 1];
#endif

const struct db_command db_machine_command_table[] = {
#if defined(_KERNEL) && defined(MULTIPROCESSOR)
	{
		DDB_ADD_CMD(
		    "cpu", db_md_switch_cpu_cmd, 0,
		    "switch to a different cpu",
		    NULL, NULL)
	},
#endif
#if defined(_KERNEL)
	{
		DDB_ADD_CMD(
		    "break", db_md_break_cmd, 0,
		    "set or clear breakpoint",
		    "[address|#]",
		    "\taddress: breakpoint address to set\n"
		    "\t#: breakpoint number to remove\n")
	},
	{
		DDB_ADD_CMD(
		    "cpuinfo", db_md_cpuinfo_cmd, 0,
		    "Displays the current cpuinfo",
		    NULL, NULL)
	},
	{
		DDB_ADD_CMD(
		    "frame", db_md_frame_cmd, 0,
		    "Displays the contents of a trapframe",
		    "address",
		    "\taddress:\taddress of trapframe to display")
	},
	{
		DDB_ADD_CMD(
		    "lwp", db_md_lwp_cmd, 0,
		    "Displays the lwp",
		    "address",
		    "\taddress:\taddress of lwp to display")
	},
	{
		DDB_ADD_CMD(
		    "pte", db_md_pte_cmd, 0,
		    "Display information of pte",
		    "address",
		    "\taddress:\tvirtual address of page")
	},
	{
		DDB_ADD_CMD(
		    "reset", db_md_reset_cmd, 0,
		    "Reset the system",
		    NULL, NULL)
	},
	{
		DDB_ADD_CMD(
		    "sysreg", db_md_sysreg_cmd, 0,
		    "Displays system registers",
		    NULL, NULL)
	},
	{
		DDB_ADD_CMD(
		    "tlbi", db_md_tlbi_cmd, 0,
		    "flush tlb",
		    NULL, NULL)
	},
	{
		DDB_ADD_CMD(
		    "ttbr", db_md_ttbr_cmd, 0,
		    "Dump or count TTBR table",
		    "[/apc] address | pid",
		    "\taddress:\taddress of pmap to display\n"
		    "\tpid:\t\tpid of pmap to display")
	},
	{
		DDB_ADD_CMD(
		    "watch", db_md_watch_cmd, 0,
		    "set or clear watchpoint",
		    "[/rwbhlq] [address|#]",
		    "\taddress: watchpoint address to set\n"
		    "\t#: watchpoint number to remove\n"
		    "\t/rw: read or write access\n"
		    "\t/bhlq: size of access\n")
	},
	{
		DDB_ADD_CMD(
		    "meminfo", db_md_meminfo_cmd, 0,
		    "Dump info about memory ranges",
		    NULL, NULL)
	},
#endif
	{
		DDB_END_CMD
	},
};

const struct db_variable db_regs[] = {
	{ "x0",    (long *) &ddb_regs.tf_reg[0],  FCN_NULL, NULL },
	{ "x1",    (long *) &ddb_regs.tf_reg[1],  FCN_NULL, NULL },
	{ "x2",    (long *) &ddb_regs.tf_reg[2],  FCN_NULL, NULL },
	{ "x3",    (long *) &ddb_regs.tf_reg[3],  FCN_NULL, NULL },
	{ "x4",    (long *) &ddb_regs.tf_reg[4],  FCN_NULL, NULL },
	{ "x5",    (long *) &ddb_regs.tf_reg[5],  FCN_NULL, NULL },
	{ "x6",    (long *) &ddb_regs.tf_reg[6],  FCN_NULL, NULL },
	{ "x7",    (long *) &ddb_regs.tf_reg[7],  FCN_NULL, NULL },
	{ "x8",    (long *) &ddb_regs.tf_reg[8],  FCN_NULL, NULL },
	{ "x9",    (long *) &ddb_regs.tf_reg[9],  FCN_NULL, NULL },
	{ "x10",   (long *) &ddb_regs.tf_reg[10], FCN_NULL, NULL },
	{ "x11",   (long *) &ddb_regs.tf_reg[11], FCN_NULL, NULL },
	{ "x12",   (long *) &ddb_regs.tf_reg[12], FCN_NULL, NULL },
	{ "x13",   (long *) &ddb_regs.tf_reg[13], FCN_NULL, NULL },
	{ "x14",   (long *) &ddb_regs.tf_reg[14], FCN_NULL, NULL },
	{ "x15",   (long *) &ddb_regs.tf_reg[15], FCN_NULL, NULL },
	{ "x16",   (long *) &ddb_regs.tf_reg[16], FCN_NULL, NULL },
	{ "x17",   (long *) &ddb_regs.tf_reg[17], FCN_NULL, NULL },
	{ "x18",   (long *) &ddb_regs.tf_reg[18], FCN_NULL, NULL },
	{ "x19",   (long *) &ddb_regs.tf_reg[19], FCN_NULL, NULL },
	{ "x20",   (long *) &ddb_regs.tf_reg[20], FCN_NULL, NULL },
	{ "x21",   (long *) &ddb_regs.tf_reg[21], FCN_NULL, NULL },
	{ "x22",   (long *) &ddb_regs.tf_reg[22], FCN_NULL, NULL },
	{ "x23",   (long *) &ddb_regs.tf_reg[23], FCN_NULL, NULL },
	{ "x24",   (long *) &ddb_regs.tf_reg[24], FCN_NULL, NULL },
	{ "x25",   (long *) &ddb_regs.tf_reg[25], FCN_NULL, NULL },
	{ "x26",   (long *) &ddb_regs.tf_reg[26], FCN_NULL, NULL },
	{ "x27",   (long *) &ddb_regs.tf_reg[27], FCN_NULL, NULL },
	{ "x28",   (long *) &ddb_regs.tf_reg[28], FCN_NULL, NULL },
	{ "x29",   (long *) &ddb_regs.tf_reg[29], FCN_NULL, NULL },
	{ "x30",   (long *) &ddb_regs.tf_reg[30], FCN_NULL, NULL },
	{ "sp",    (long *) &ddb_regs.tf_sp,      FCN_NULL, NULL },
	{ "pc",    (long *) &ddb_regs.tf_pc,      FCN_NULL, NULL },
	{ "spsr",  (long *) &ddb_regs.tf_spsr,    FCN_NULL, NULL }
};

const struct db_variable * const db_eregs = db_regs + __arraycount(db_regs);
int db_active;


void
dump_trapframe(struct trapframe *tf, void (*pr)(const char *, ...))
{
	struct trapframe tf_buf;

	db_read_bytes((db_addr_t)tf, sizeof(tf_buf), (char *)&tf_buf);
	tf = &tf_buf;

#ifdef COMPAT_NETBSD32
	if (tf->tf_spsr & SPSR_A32) {
		(*pr)("    pc=%016"PRIxREGISTER",   spsr=%016"PRIxREGISTER
		    " (AArch32)\n", tf->tf_pc, tf->tf_spsr);
		(*pr)("   esr=%016"PRIxREGISTER",    far=%016"PRIxREGISTER"\n",
		    tf->tf_esr, tf->tf_far);
		(*pr)("    r0=%016"PRIxREGISTER",     r1=%016"PRIxREGISTER"\n",
		    tf->tf_reg[0], tf->tf_reg[1]);
		(*pr)("    r2=%016"PRIxREGISTER",     r3=%016"PRIxREGISTER"\n",
		    tf->tf_reg[2], tf->tf_reg[3]);
		(*pr)("    r4=%016"PRIxREGISTER",     r5=%016"PRIxREGISTER"\n",
		    tf->tf_reg[4], tf->tf_reg[5]);
		(*pr)("    r6=%016"PRIxREGISTER",     r7=%016"PRIxREGISTER"\n",
		    tf->tf_reg[6], tf->tf_reg[7]);
		(*pr)("    r8=%016"PRIxREGISTER",     r9=%016"PRIxREGISTER"\n",
		    tf->tf_reg[8], tf->tf_reg[9]);
		(*pr)("   r10=%016"PRIxREGISTER",    r11=%016"PRIxREGISTER"\n",
		    tf->tf_reg[10], tf->tf_reg[11]);
		(*pr)("   r12=%016"PRIxREGISTER", sp=r13=%016"PRIxREGISTER"\n",
		    tf->tf_reg[12], tf->tf_reg[13]);
		(*pr)("lr=r14=%016"PRIxREGISTER", pc=r15=%016"PRIxREGISTER"\n",
		    tf->tf_reg[14], tf->tf_pc);
		return;
	}
#endif
	(*pr)("    pc=%016"PRIxREGISTER",   spsr=%016"PRIxREGISTER"\n",
	    tf->tf_pc, tf->tf_spsr);
	(*pr)("   esr=%016"PRIxREGISTER",    far=%016"PRIxREGISTER"\n",
	    tf->tf_esr, tf->tf_far);
	(*pr)("    x0=%016"PRIxREGISTER",     x1=%016"PRIxREGISTER"\n",
	    tf->tf_reg[0], tf->tf_reg[1]);
	(*pr)("    x2=%016"PRIxREGISTER",     x3=%016"PRIxREGISTER"\n",
	    tf->tf_reg[2], tf->tf_reg[3]);
	(*pr)("    x4=%016"PRIxREGISTER",     x5=%016"PRIxREGISTER"\n",
	    tf->tf_reg[4], tf->tf_reg[5]);
	(*pr)("    x6=%016"PRIxREGISTER",     x7=%016"PRIxREGISTER"\n",
	    tf->tf_reg[6], tf->tf_reg[7]);
	(*pr)("    x8=%016"PRIxREGISTER",     x9=%016"PRIxREGISTER"\n",
	    tf->tf_reg[8], tf->tf_reg[9]);
	(*pr)("   x10=%016"PRIxREGISTER",    x11=%016"PRIxREGISTER"\n",
	    tf->tf_reg[10], tf->tf_reg[11]);
	(*pr)("   x12=%016"PRIxREGISTER",    x13=%016"PRIxREGISTER"\n",
	    tf->tf_reg[12], tf->tf_reg[13]);
	(*pr)("   x14=%016"PRIxREGISTER",    x15=%016"PRIxREGISTER"\n",
	    tf->tf_reg[14], tf->tf_reg[15]);
	(*pr)("   x16=%016"PRIxREGISTER",    x17=%016"PRIxREGISTER"\n",
	    tf->tf_reg[16], tf->tf_reg[17]);
	(*pr)("   x18=%016"PRIxREGISTER",    x19=%016"PRIxREGISTER"\n",
	    tf->tf_reg[18], tf->tf_reg[19]);
	(*pr)("   x20=%016"PRIxREGISTER",    x21=%016"PRIxREGISTER"\n",
	    tf->tf_reg[20], tf->tf_reg[21]);
	(*pr)("   x22=%016"PRIxREGISTER",    x23=%016"PRIxREGISTER"\n",
	    tf->tf_reg[22], tf->tf_reg[23]);
	(*pr)("   x24=%016"PRIxREGISTER",    x25=%016"PRIxREGISTER"\n",
	    tf->tf_reg[24], tf->tf_reg[25]);
	(*pr)("   x26=%016"PRIxREGISTER",    x27=%016"PRIxREGISTER"\n",
	    tf->tf_reg[26], tf->tf_reg[27]);
	(*pr)("   x28=%016"PRIxREGISTER", fp=x29=%016"PRIxREGISTER"\n",
	    tf->tf_reg[28], tf->tf_reg[29]);
	(*pr)("lr=x30=%016"PRIxREGISTER",     sp=%016"PRIxREGISTER"\n",
	    tf->tf_reg[30],  tf->tf_sp);
}

void
dump_switchframe(struct trapframe *tf, void (*pr)(const char *, ...))
{
	struct trapframe tf_buf;

	db_read_bytes((db_addr_t)tf, sizeof(tf_buf), (char *)&tf_buf);
	tf = &tf_buf;

	(*pr)("   x19=%016"PRIxREGISTER",    x20=%016"PRIxREGISTER"\n",
	    tf->tf_reg[19], tf->tf_reg[20]);
	(*pr)("   x21=%016"PRIxREGISTER",    x22=%016"PRIxREGISTER"\n",
	    tf->tf_reg[21], tf->tf_reg[22]);
	(*pr)("   x23=%016"PRIxREGISTER",    x24=%016"PRIxREGISTER"\n",
	    tf->tf_reg[23], tf->tf_reg[24]);
	(*pr)("   x25=%016"PRIxREGISTER",    x26=%016"PRIxREGISTER"\n",
	    tf->tf_reg[25], tf->tf_reg[26]);
	(*pr)("   x27=%016"PRIxREGISTER",    x28=%016"PRIxREGISTER"\n",
	    tf->tf_reg[27], tf->tf_reg[28]);
	(*pr)("fp=x29=%016"PRIxREGISTER", lr=x30=%016"PRIxREGISTER"\n",
	    tf->tf_reg[29], tf->tf_reg[30]);
}


#if defined(_KERNEL)
static void
show_cpuinfo(struct cpu_info *ci)
{
	struct cpu_info cpuinfobuf;
	u_int cpuidx;
	int i;

	db_read_bytes((db_addr_t)ci, sizeof(cpuinfobuf), (char *)&cpuinfobuf);

	cpuidx = cpu_index(&cpuinfobuf);
	db_printf("cpu_info=%p, cpu_name=%s\n", ci, cpuinfobuf.ci_cpuname);
	db_printf("%p cpu[%u].ci_cpuid         = 0x%lx\n",
	    &ci->ci_cpuid, cpuidx, cpuinfobuf.ci_cpuid);
	db_printf("%p cpu[%u].ci_curlwp        = %p\n",
	    &ci->ci_curlwp, cpuidx, cpuinfobuf.ci_curlwp);
	db_printf("%p cpu[%u].ci_onproc        = %p\n",
	    &ci->ci_onproc, cpuidx, cpuinfobuf.ci_onproc);
	for (i = 0; i < SOFTINT_COUNT; i++) {
		db_printf("%p cpu[%u].ci_softlwps[%d]   = %p\n",
		    &ci->ci_softlwps[i], cpuidx, i, cpuinfobuf.ci_softlwps[i]);
	}
	db_printf("%p cpu[%u].ci_lastintr      = %" PRIu64 "\n",
	    &ci->ci_lastintr, cpuidx, cpuinfobuf.ci_lastintr);
	db_printf("%p cpu[%u].ci_want_resched  = %d\n",
	    &ci->ci_want_resched, cpuidx, cpuinfobuf.ci_want_resched);
	db_printf("%p cpu[%u].ci_cpl           = %d\n",
	    &ci->ci_cpl, cpuidx, cpuinfobuf.ci_cpl);
	db_printf("%p cpu[%u].ci_softints      = 0x%08x\n",
	    &ci->ci_softints, cpuidx, cpuinfobuf.ci_softints);
	db_printf("%p cpu[%u].ci_intr_depth    = %u\n",
	    &ci->ci_intr_depth, cpuidx, cpuinfobuf.ci_intr_depth);
	db_printf("%p cpu[%u].ci_biglock_count = %u\n",
	    &ci->ci_biglock_count, cpuidx, cpuinfobuf.ci_biglock_count);
}

void
db_md_cpuinfo_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
#ifdef MULTIPROCESSOR
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	bool showall = false;

	if (modif != NULL) {
		for (; *modif != '\0'; modif++) {
			switch (*modif) {
			case 'a':
				showall = true;
				break;
			}
		}
	}

	if (showall) {
		for (CPU_INFO_FOREACH(cii, ci)) {
			show_cpuinfo(ci);
		}
	} else
#endif /* MULTIPROCESSOR */
		show_cpuinfo(curcpu());
}

void
db_md_frame_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	struct trapframe *tf;

	if (!have_addr) {
		db_printf("frame address must be specified\n");
		return;
	}

	tf = (struct trapframe *)addr;
	dump_trapframe(tf, db_printf);
}

void
db_md_lwp_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	lwp_t *l, lwp_buf;
	struct pcb *pcb, pcb_buf;

	if (!have_addr) {
		db_printf("lwp address must be specified\n");
		return;
	}

	db_read_bytes(addr, sizeof(lwp_buf), (char *)&lwp_buf);
	l = &lwp_buf;

#define SAFESTRPTR(p)	(((p) == NULL) ? "NULL" : (p))

	db_printf("lwp=%p\n", (void *)addr);

	db_printf("\tlwp_getpcb(l)     =%p\n", lwp_getpcb(l));

	db_printf("\tl->l_md.md_onfault=%p\n", l->l_md.md_onfault);
	db_printf("\tl->l_md.md_utf    =%p\n", l->l_md.md_utf);
	dump_trapframe(l->l_md.md_utf, db_printf);

	db_read_bytes((db_addr_t)l->l_addr, sizeof(pcb_buf), (char *)&pcb_buf);
	pcb = &pcb_buf;

	db_printf("\tl->l_addr.pcb_tf    =%p\n", pcb->pcb_tf);
	if (pcb->pcb_tf != l->l_md.md_utf)
		dump_switchframe(pcb->pcb_tf, db_printf);
	db_printf("\tl->l_md.md_cpacr  =%016" PRIx64 "\n", l->l_md.md_cpacr);
	db_printf("\tl->l_md.md_flags  =%08x\n", l->l_md.md_flags);

	db_printf("\tl->l_cpu          =%p\n", l->l_cpu);
	db_printf("\tl->l_proc         =%p\n", l->l_proc);
	db_printf("\tl->l_private      =%p\n", l->l_private);
	db_printf("\tl->l_name         =%s\n", SAFESTRPTR(l->l_name));
	db_printf("\tl->l_wmesg        =%s\n", SAFESTRPTR(l->l_wmesg));
}

static void
db_par_print(uint64_t par, vaddr_t va)
{
	paddr_t pa = (__SHIFTOUT(par, PAR_PA) << PAR_PA_SHIFT) +
	    (va & __BITS(PAR_PA_SHIFT - 1, 0));

	if (__SHIFTOUT(par, PAR_F) == 0) {
		db_printf("%016" PRIx64
		    ": ATTR=0x%02" __PRIxBITS
		    ", NS=%" __PRIuBITS
		    ", SH=%" __PRIuBITS
		    ", PA=%016" PRIxPADDR
		    " (no fault)\n",
		    par,
		    __SHIFTOUT(par, PAR_ATTR),
		    __SHIFTOUT(par, PAR_NS),
		    __SHIFTOUT(par, PAR_SH),
		    pa);
	} else {
		db_printf("%016" PRIx64
		    ", S=%" __PRIuBITS
		    ", PTW=%" __PRIuBITS
		    ", FST=%" __PRIuBITS
		    " (fault)\n",
		    par,
		    __SHIFTOUT(par, PAR_S),
		    __SHIFTOUT(par, PAR_PTW),
		    __SHIFTOUT(par, PAR_FST));
	}
}

void
db_md_pte_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	uint64_t par;

	if (!have_addr) {
		db_printf("pte address must be specified\n");
		return;
	}

	reg_s1e0r_write(addr);
	isb();
	par = reg_par_el1_read();
	db_printf("Stage1 EL0 translation %016llx -> PAR_EL1 = ", addr);
	db_par_print(par, addr);

	reg_s1e1r_write(addr);
	isb();
	par = reg_par_el1_read();
	db_printf("Stage1 EL1 translation %016llx -> PAR_EL1 = ", addr);
	db_par_print(par, addr);

	db_pteinfo(addr, db_printf);
}

void
db_md_reset_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	if (cpu_reset_address == NULL) {
		db_printf("cpu_reset_address is not set\n");
		return;
	}

	cpu_reset_address();
}

void
db_md_tlbi_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	aarch64_tlbi_all();
}

void
db_md_ttbr_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	bool countmode = false, by_pid = true;

	if (!have_addr) {
		db_printf("usage: machine ttbr [/a] [/p] [/c] address|pid\n");
		db_printf("\t/a == argument is an address of any pmap_t\n");
		db_printf("\t/p == argument is a pid [default]\n");
		db_printf("\t/c == count TLB entries\n");
		return;
	}

	if (modif != NULL) {
		for (; *modif != '\0'; modif++) {
			switch (*modif) {
			case 'c':
				countmode = true;
				break;
			case 'a':
				by_pid = false;
				break;
			case 'p':
				by_pid = true;
				break;
			}
		}
	}

	if (by_pid) {
		proc_t *p = db_proc_find((pid_t)addr);
		if (p == NULL) {
			db_printf("bad address\n");
			return;
		}
		addr = (db_addr_t)p->p_vmspace->vm_map.pmap;
	}

	db_ttbrdump(countmode, addr, db_printf);
}

void
db_md_sysreg_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
#define SHOW_ARMREG(x)	\
	db_printf("%-16s = %016" PRIx64 "\n", #x, reg_ ## x ## _read())

//	SHOW_ARMREG(cbar_el1);	/* Cortex */
	SHOW_ARMREG(ccsidr_el1);
	SHOW_ARMREG(clidr_el1);
	SHOW_ARMREG(cntfrq_el0);
	SHOW_ARMREG(cntkctl_el1);
	SHOW_ARMREG(cntp_ctl_el0);
	SHOW_ARMREG(cntp_cval_el0);
	SHOW_ARMREG(cntp_tval_el0);
	SHOW_ARMREG(cntpct_el0);
//	SHOW_ARMREG(cntps_ctl_el1);	/* need secure state */
//	SHOW_ARMREG(cntps_cval_el1);	/* need secure state */
//	SHOW_ARMREG(cntps_tval_el1);	/* need secure state */
	SHOW_ARMREG(cntv_ctl_el0);
	SHOW_ARMREG(cntv_ctl_el0);
	SHOW_ARMREG(cntv_cval_el0);
	SHOW_ARMREG(cntv_tval_el0);
	SHOW_ARMREG(cntv_tval_el0);
	SHOW_ARMREG(cntvct_el0);
	SHOW_ARMREG(cpacr_el1);
	SHOW_ARMREG(csselr_el1);
	SHOW_ARMREG(ctr_el0);
	SHOW_ARMREG(currentel);
	SHOW_ARMREG(daif);
	SHOW_ARMREG(dczid_el0);
	SHOW_ARMREG(elr_el1);
	SHOW_ARMREG(esr_el1);
	SHOW_ARMREG(far_el1);
//	SHOW_ARMREG(fpcr);	/* FP trap */
//	SHOW_ARMREG(fpsr);	/* FP trap */
	SHOW_ARMREG(id_aa64afr0_el1);
	SHOW_ARMREG(id_aa64afr1_el1);
	SHOW_ARMREG(id_aa64dfr0_el1);
	SHOW_ARMREG(id_aa64dfr1_el1);
	SHOW_ARMREG(id_aa64isar0_el1);
	SHOW_ARMREG(id_aa64isar1_el1);
	SHOW_ARMREG(id_aa64mmfr0_el1);
	SHOW_ARMREG(id_aa64mmfr1_el1);
	SHOW_ARMREG(id_aa64pfr0_el1);
	SHOW_ARMREG(id_aa64pfr1_el1);
	SHOW_ARMREG(isr_el1);
//	SHOW_ARMREG(l2ctlr_el1);	/* Cortex */
	SHOW_ARMREG(mair_el1);
	SHOW_ARMREG(mdscr_el1);
	SHOW_ARMREG(midr_el1);
	SHOW_ARMREG(mpidr_el1);
	SHOW_ARMREG(mvfr0_el1);
	SHOW_ARMREG(mvfr1_el1);
	SHOW_ARMREG(mvfr2_el1);
	SHOW_ARMREG(nzcv);
	SHOW_ARMREG(par_el1);
	SHOW_ARMREG(pmccfiltr_el0);
	SHOW_ARMREG(pmccntr_el0);
	SHOW_ARMREG(revidr_el1);
//	SHOW_ARMREG(rmr_el1);		/* unknown reason trap */
//	SHOW_ARMREG(rvbar_el1);
	SHOW_ARMREG(sctlr_el1);
	SHOW_ARMREG(spsel);
	SHOW_ARMREG(spsr_el1);
	SHOW_ARMREG(tcr_el1);
	SHOW_ARMREG(tpidr_el0);
	SHOW_ARMREG(tpidrro_el0);
	SHOW_ARMREG(tpidr_el1);
	SHOW_ARMREG(ttbr0_el1);
	SHOW_ARMREG(ttbr1_el1);
	SHOW_ARMREG(vbar_el1);
}

/*
 * hardware breakpoint/watchpoint command
 */
static void
aarch64_set_bcr_bvr(int n, uint64_t bcr, uint64_t bvr)
{
#define DBG_BCR_BVR_SET(regno, bcr, bvr)			\
	do {							\
		reg_dbgbcr ## regno ## _el1_write(bcr);		\
		reg_dbgbvr ## regno ## _el1_write(bvr);		\
	} while (0 /* CONSTCOND */)

	switch (n) {
	case 0:		DBG_BCR_BVR_SET(0,  bcr, bvr);	break;
	case 1:		DBG_BCR_BVR_SET(1,  bcr, bvr);	break;
	case 2:		DBG_BCR_BVR_SET(2,  bcr, bvr);	break;
	case 3:		DBG_BCR_BVR_SET(3,  bcr, bvr);	break;
	case 4:		DBG_BCR_BVR_SET(4,  bcr, bvr);	break;
	case 5:		DBG_BCR_BVR_SET(5,  bcr, bvr);	break;
	case 6:		DBG_BCR_BVR_SET(6,  bcr, bvr);	break;
	case 7:		DBG_BCR_BVR_SET(7,  bcr, bvr);	break;
	case 8:		DBG_BCR_BVR_SET(8,  bcr, bvr);	break;
	case 9:		DBG_BCR_BVR_SET(9,  bcr, bvr);	break;
	case 10:	DBG_BCR_BVR_SET(10, bcr, bvr);	break;
	case 11:	DBG_BCR_BVR_SET(11, bcr, bvr);	break;
	case 12:	DBG_BCR_BVR_SET(12, bcr, bvr);	break;
	case 13:	DBG_BCR_BVR_SET(13, bcr, bvr);	break;
	case 14:	DBG_BCR_BVR_SET(14, bcr, bvr);	break;
	case 15:	DBG_BCR_BVR_SET(15, bcr, bvr);	break;
	}
}

static void
aarch64_set_wcr_wvr(int n, uint64_t wcr, uint64_t wvr)
{
#define DBG_WCR_WVR_SET(regno, wcr, wvr)			\
	do {							\
		reg_dbgwcr ## regno ## _el1_write(wcr);		\
		reg_dbgwvr ## regno ## _el1_write(wvr);		\
	} while (0 /* CONSTCOND */)

	switch (n) {
	case 0:		DBG_WCR_WVR_SET(0,  wcr, wvr);	break;
	case 1:		DBG_WCR_WVR_SET(1,  wcr, wvr);	break;
	case 2:		DBG_WCR_WVR_SET(2,  wcr, wvr);	break;
	case 3:		DBG_WCR_WVR_SET(3,  wcr, wvr);	break;
	case 4:		DBG_WCR_WVR_SET(4,  wcr, wvr);	break;
	case 5:		DBG_WCR_WVR_SET(5,  wcr, wvr);	break;
	case 6:		DBG_WCR_WVR_SET(6,  wcr, wvr);	break;
	case 7:		DBG_WCR_WVR_SET(7,  wcr, wvr);	break;
	case 8:		DBG_WCR_WVR_SET(8,  wcr, wvr);	break;
	case 9:		DBG_WCR_WVR_SET(9,  wcr, wvr);	break;
	case 10:	DBG_WCR_WVR_SET(10, wcr, wvr);	break;
	case 11:	DBG_WCR_WVR_SET(11, wcr, wvr);	break;
	case 12:	DBG_WCR_WVR_SET(12, wcr, wvr);	break;
	case 13:	DBG_WCR_WVR_SET(13, wcr, wvr);	break;
	case 14:	DBG_WCR_WVR_SET(14, wcr, wvr);	break;
	case 15:	DBG_WCR_WVR_SET(15, wcr, wvr);	break;
	}
}

void
aarch64_breakpoint_set(int n, vaddr_t addr)
{
	uint64_t bcr, bvr;

	if (addr == 0) {
		bvr = 0;
		bcr = 0;
	} else {
		bvr = addr & DBGBVR_MASK;
		bcr =
		    __SHIFTIN(0, DBGBCR_BT) |
		    __SHIFTIN(0, DBGBCR_LBN) |
		    __SHIFTIN(0, DBGBCR_SSC) |
		    __SHIFTIN(0, DBGBCR_HMC) |
		    __SHIFTIN(15, DBGBCR_BAS) |
		    __SHIFTIN(3, DBGBCR_PMC) |
		    __SHIFTIN(1, DBGBCR_E);
	}

	aarch64_set_bcr_bvr(n, bcr, bvr);
}

void
aarch64_watchpoint_set(int n, vaddr_t addr, u_int size, u_int accesstype)
{
	uint64_t wvr, wcr;
	uint32_t matchbytebit;

	KASSERT(size <= 8);
	if (size > 8)
		size = 8;

	/*
	 * It is always watched in 8byte units, and
	 * BAS is a bit field of byte offset in 8byte units.
	 */
	matchbytebit = 0xff >> (8 - size);
	matchbytebit <<= (addr & 7);
	addr &= ~7UL;

	/* load, store, or both */
	accesstype &= WATCHPOINT_ACCESS_MASK;
	if (accesstype == 0)
		accesstype = WATCHPOINT_ACCESS_LOADSTORE;

	if (addr == 0) {
		wvr = 0;
		wcr = 0;
	} else {
		wvr = addr;
		wcr =
		    __SHIFTIN(0, DBGWCR_MASK) |		/* MASK: no mask */
		    __SHIFTIN(0, DBGWCR_WT) |		/* WT: 0 */
		    __SHIFTIN(0, DBGWCR_LBN) |		/* LBN: 0 */
		    __SHIFTIN(0, DBGWCR_SSC) |		/* SSC: 00 */
		    __SHIFTIN(0, DBGWCR_HMC) |		/* HMC: 0 */
		    __SHIFTIN(matchbytebit, DBGWCR_BAS) | /* BAS: 0-8byte */
		    __SHIFTIN(accesstype, DBGWCR_LSC) |	/* LSC: Load/Store */
		    __SHIFTIN(3, DBGWCR_PAC) |		/* PAC: 11 */
		    __SHIFTIN(1, DBGWCR_E);		/* Enable */
	}

	aarch64_set_wcr_wvr(n, wcr, wvr);
}

static int
db_md_breakpoint_set(int n, vaddr_t addr)
{
	if (n >= __arraycount(breakpoint_buf))
		return -1;

	if ((addr & 3) != 0) {
		db_printf("address must be 4bytes aligned\n");
		return -1;
	}

	breakpoint_buf[n].addr = addr;
	return 0;
}

static int
db_md_watchpoint_set(int n, vaddr_t addr, u_int size, u_int accesstype)
{
	if (n >= __arraycount(watchpoint_buf))
		return -1;

	if (size != 0 && ((addr) & ~7UL) != ((addr + size - 1) & ~7UL)) {
		db_printf(
		    "address and size must fit within a block of 8bytes\n");
		return -1;
	}

	watchpoint_buf[n].addr = addr;
	watchpoint_buf[n].size = size;
	watchpoint_buf[n].accesstype = accesstype;
	return 0;
}

static void
db_md_breakwatchpoints_clear(void)
{
	int i;

	for (i = 0; i <= max_breakpoint; i++)
		aarch64_breakpoint_set(i, 0);
	for (i = 0; i <= max_watchpoint; i++)
		aarch64_watchpoint_set(i, 0, 0, 0);
}

static void
db_md_breakwatchpoints_reload(void)
{
	int i;

	for (i = 0; i <= max_breakpoint; i++) {
		aarch64_breakpoint_set(i,
		    breakpoint_buf[i].addr);
	}
	for (i = 0; i <= max_watchpoint; i++) {
		aarch64_watchpoint_set(i,
		    watchpoint_buf[i].addr,
		    watchpoint_buf[i].size,
		    watchpoint_buf[i].accesstype);
	}
}

void
db_machdep_cpu_init(void)
{
	uint64_t dfr, mdscr;
	int i, cpu_max_breakpoint, cpu_max_watchpoint;

	dfr = reg_id_aa64dfr0_el1_read();
	cpu_max_breakpoint = __SHIFTOUT(dfr, ID_AA64DFR0_EL1_BRPS);
	cpu_max_watchpoint = __SHIFTOUT(dfr, ID_AA64DFR0_EL1_WRPS);

	for (i = 0; i <= cpu_max_breakpoint; i++) {
		/* clear all breakpoints */
		aarch64_breakpoint_set(i, 0);
	}
	for (i = 0; i <= cpu_max_watchpoint; i++) {
		/* clear all watchpoints */
		aarch64_watchpoint_set(i, 0, 0, 0);
	}

	/* enable watchpoint and breakpoint */
	mdscr = reg_mdscr_el1_read();
	mdscr |= MDSCR_MDE | MDSCR_KDE;
	reg_mdscr_el1_write(mdscr);
	reg_oslar_el1_write(0);
}

void
db_machdep_init(struct cpu_info * const ci)
{
	struct aarch64_sysctl_cpu_id * const id = &ci->ci_id;
	const uint64_t dfr = id->ac_aa64dfr0;
	const u_int cpu_max_breakpoint = __SHIFTOUT(dfr, ID_AA64DFR0_EL1_BRPS);
	const u_int cpu_max_watchpoint = __SHIFTOUT(dfr, ID_AA64DFR0_EL1_WRPS);

	/*
	 * num of {watch,break}point may be different depending on the
	 * core.
	 */
	if (max_breakpoint > cpu_max_breakpoint)
		max_breakpoint = cpu_max_breakpoint;
	if (max_watchpoint > cpu_max_watchpoint)
		max_watchpoint = cpu_max_watchpoint;
}


static void
show_breakpoints(void)
{
	uint64_t addr;
	unsigned int i, nused;

	for (nused = 0, i = 0; i <= max_breakpoint; i++) {
		addr = breakpoint_buf[i].addr;
		if (addr == 0) {
			db_printf("%d: disabled\n", i);
		} else {
			db_printf("%d: breakpoint %016" PRIx64 " (", i,
			    addr);
			db_printsym(addr, DB_STGY_ANY, db_printf);
			db_printf(")\n");
			nused++;
		}
	}
	db_printf("breakpoint used %d/%d\n", nused, max_breakpoint + 1);
}

static void
show_watchpoints(void)
{
	uint64_t addr;
	unsigned int i, nused;

	for (nused = 0, i = 0; i <= max_watchpoint; i++) {
		addr = watchpoint_buf[i].addr;
		if (addr == 0) {
			db_printf("%d: disabled\n", i);
		} else {
			db_printf("%d: watching %016" PRIx64 " (", i,
			    addr);
			db_printsym(addr, DB_STGY_ANY, db_printf);
			db_printf("), %d bytes", watchpoint_buf[i].size);

			switch (watchpoint_buf[i].accesstype) {
			case WATCHPOINT_ACCESS_LOAD:
				db_printf(", load");
				break;
			case WATCHPOINT_ACCESS_STORE:
				db_printf(", store");
				break;
			case WATCHPOINT_ACCESS_LOADSTORE:
				db_printf(", load/store");
				break;
			}
			db_printf("\n");
			nused++;
		}
	}
	db_printf("watchpoint used %d/%d\n", nused, max_watchpoint + 1);
}

void
db_md_break_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	int i, rc;
	int added, cleared;

	if (!have_addr) {
		show_breakpoints();
		return;
	}

	added = -1;
	cleared = -1;
	if (0 <= addr && addr <= max_breakpoint) {
		i = addr;
		if (breakpoint_buf[i].addr != 0) {
			db_md_breakpoint_set(i, 0);
			cleared = i;
		}
	} else {
		for (i = 0; i <= max_breakpoint; i++) {
			if (breakpoint_buf[i].addr == addr) {
				db_md_breakpoint_set(i, 0);
				cleared = i;
			}
		}
		if (cleared == -1) {
			for (i = 0; i <= max_breakpoint; i++) {
				if (breakpoint_buf[i].addr == 0) {
					rc = db_md_breakpoint_set(i, addr);
					if (rc != 0)
						return;
					added = i;
					break;
				}
			}
			if (i > max_breakpoint) {
				db_printf("no more available breakpoint\n");
			}
		}
	}

	if (added >= 0)
		db_printf("add breakpoint %d as %016"DDB_EXPR_FMT"x\n",
		    added, addr);
	if (cleared >= 0)
		db_printf("clear breakpoint %d\n", cleared);

	show_breakpoints();
}

void
db_md_watch_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	int i, rc;
	int added, cleared;
	u_int accesstype, watchsize;

	if (!have_addr) {
		show_watchpoints();
		return;
	}

	accesstype = watchsize = 0;
	if ((modif != NULL) && (*modif != '\0')) {
		int ch;
		for (; *modif != '\0'; modif++) {
			ch = *modif;

			switch (ch) {
			case 'b':
				watchsize = 1;
				break;
			case 'h':
				watchsize = 2;
				break;
			case 'l':
				watchsize = 4;
				break;
			case 'q':
				watchsize = 8;
				break;
			case 'r':
				accesstype |= WATCHPOINT_ACCESS_LOAD;
				break;
			case 'w':
				accesstype |= WATCHPOINT_ACCESS_STORE;
				break;
			}
		}
	}
	if (watchsize == 0)
		watchsize = 4;	/* default: 4byte */
	if (accesstype == 0)
		accesstype = WATCHPOINT_ACCESS_LOADSTORE; /* default */

	added = -1;
	cleared = -1;
	if (0 <= addr && addr <= max_watchpoint) {
		i = addr;
		if (watchpoint_buf[i].addr != 0) {
			db_md_watchpoint_set(i, 0, 0, 0);
			cleared = i;
		}
	} else {
		for (i = 0; i <= max_watchpoint; i++) {
			if (watchpoint_buf[i].addr == addr) {
				db_md_watchpoint_set(i, 0, 0, 0);
				cleared = i;
			}
		}
		if (cleared == -1) {
			for (i = 0; i <= max_watchpoint; i++) {
				if (watchpoint_buf[i].addr == 0) {
					rc = db_md_watchpoint_set(i, addr,
					    watchsize, accesstype);
					if (rc != 0)
						return;
					added = i;
					break;
				}
			}
			if (i > max_watchpoint) {
				db_printf("no more available watchpoint\n");
			}
		}
	}

	if (added >= 0)
		db_printf("add watchpoint %d as %016"DDB_EXPR_FMT"x\n",
		    added, addr);
	if (cleared >= 0)
		db_printf("clear watchpoint %d\n", cleared);

	show_watchpoints();
}
#endif

#ifdef MULTIPROCESSOR
volatile struct cpu_info *db_trigger;
volatile struct cpu_info *db_onproc;
volatile struct cpu_info *db_newcpu;
volatile struct trapframe *db_readytoswitch[MAXCPUS];

#ifdef _KERNEL
void
db_md_switch_cpu_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	struct cpu_info *new_ci = NULL;
	u_int cpuno = (u_int)addr;
	int i;

	membar_consumer();
	if (!have_addr) {
		for (i = 0; i < ncpu; i++) {
			if (db_readytoswitch[i] != NULL) {
				db_printf("cpu%d: ready. tf=%p, pc=%016lx ", i,
				    db_readytoswitch[i],
				    db_readytoswitch[i]->tf_pc);
				db_printsym(db_readytoswitch[i]->tf_pc,
				    DB_STGY_ANY, db_printf);
				db_printf("\n");
			} else {
				db_printf("cpu%d: not responding\n", i);
			}
		}
		return;
	}

	if (cpuno < ncpu)
		new_ci = cpu_lookup(cpuno);
	if (new_ci == NULL) {
		db_printf("cpu %u does not exist", cpuno);
		return;
	}
	if (db_readytoswitch[new_ci->ci_index] == 0) {
		db_printf("cpu %u is not responding", cpuno);
		return;
	}

	if (new_ci == curcpu())
		return;

	db_newcpu = new_ci;
	db_continue_cmd(0, false, 0, "");
}

#endif /* _KERNEL */
#endif /* MULTIPROCESSOR */

#ifdef DDB
int
kdb_trap(int type, struct trapframe *tf)
{
#ifdef MULTIPROCESSOR
	struct cpu_info * const ci = curcpu();
	bool static_brk = false;
#endif
	int s;
	bool restore_hw_watchpoints = true;

	switch (type) {
	case DB_TRAP_WATCHPOINT:
	case DB_TRAP_BREAKPOINT:
		/*
		 * In the case of a hardware watchpoint or breakpoint,
		 * even if cpu return from ddb as is, it will be trapped again,
		 * so clear it all once.
		 *
		 * breakpoint and watchpoint will be restored at the end of
		 * next DB_TRAP_BKPT_INSN (ddb's STEP_INVISIBLE mode).
		 */
		db_md_breakwatchpoints_clear();
		restore_hw_watchpoints = false;
		break;
	case DB_TRAP_BKPT_INSN:
#ifdef MULTIPROCESSOR
		/* brk #0xffff in cpu_Debugger() ? */
		if (__SHIFTOUT(tf->tf_esr, ESR_ISS) == 0xffff)
			static_brk = true;
		/* FALLTHRU */
#endif
	case DB_TRAP_SW_STEP:
	case DB_TRAP_UNKNOWN:
	case -1:	/* from pic_ipi_ddb() */
		break;
	default:
		if (db_recover != 0) {
			db_error("Faulted in DDB: continuing...\n");
			/* NOTREACHED */
		}
		break;
	}

#ifdef MULTIPROCESSOR
	if (ncpu > 1) {
		/*
		 * Try to take ownership of DDB.
		 * If we do, tell all other CPUs to enter DDB too.
		 */
		if (atomic_cas_ptr(&db_onproc, NULL, ci) == NULL) {
			intr_ipi_send(NULL, IPI_DDB);
			db_trigger = ci;
		} else {
			/*
			 * If multiple CPUs catch kdb_trap() that is not IPI_DDB
			 * derived at the same time, only the CPU that was able
			 * to get db_onproc first will execute db_trap.
			 * The CPU that could not get db_onproc will be set to
			 * type = -1 once, and kdb_trap will be called again
			 * with the correct type after kdb_trap returns.
			 */
			type = -1;
			restore_hw_watchpoints = true;
		}
	}
	db_readytoswitch[ci->ci_index] = tf;
#endif

	for (;;) {
#ifdef MULTIPROCESSOR
		if (ncpu > 1) {
			/* waiting my turn, or exit */
			dsb(ishld);
			while (db_onproc != ci) {
				__asm __volatile ("wfe");

				dsb(ishld);
				if (db_onproc == NULL)
					goto kdb_trap_done;
			}
			/* It's my turn! */
		}
#endif /* MULTIPROCESSOR */

		/* Should switch to kdb`s own stack here. */
		ddb_regs = *tf;

		s = splhigh();
		db_active++;
		cnpollc(true);
		db_trap(type, 0/*code*/);
		cnpollc(false);
		db_active--;
		splx(s);

		*tf = ddb_regs;

#ifdef MULTIPROCESSOR
		if (ncpu < 2)
			break;

		if (db_newcpu == NULL && db_onproc != db_trigger) {
			/*
			 * If the "machine cpu" switches CPUs after entering
			 * ddb from a breakpoint or watchpoint, it will return
			 * control to the CPU that triggered the ddb in the
			 * first place in order to correctly reset the
			 * breakpoint or watchpoint.
			 * If db_trap() returns further from here,
			 * watchpoints and breakpoints will be reset.
			 * (db_run_mode = STEP_INVISIBLE)
			 */
			db_newcpu = db_trigger;
		}

		if (db_newcpu != NULL) {
			/* XXX: override sys/ddb/db_run.c:db_run_mode */
			db_continue_cmd(0, false, 0, "");

			/*
			 * When static BRK instruction (cpu_Debugger()),
			 * db_trap() advance the $PC to the next instruction of
			 * BRK. If db_trap() will be called twice by
			 * "machine cpu" command, the $PC will be advanced to
			 * the after the next instruction.
			 * To avoid this, change 'type' so that the second call
			 * to db_trap() will not change the PC.
			 */
			if (static_brk)
				type = -1;

			db_onproc = db_newcpu;
			db_newcpu = NULL;
			dsb(ishst);
			/* waking up the CPU waiting for its turn to db_trap */
			sev();

			continue;	/* go to waiting my turn */
		}
#endif /* MULTIPROCESSOR */

		break;
	}

#ifdef MULTIPROCESSOR
	if (ncpu > 1 && db_onproc == ci) {
		db_onproc = NULL;
		dsb(ishst);
		/* waking up the CPU waiting for its turn to exit */
		sev();

		db_readytoswitch[cpu_index(ci)] = NULL;
		/* wait for all other CPUs are ready to exit */
		for (;;) {
			int i;
			dsb(ishld);
			for (i = 0; i < ncpu; i++) {
				if (db_readytoswitch[i] != NULL)
					break;
			}
			if (i == ncpu)
				break;
		}
		db_trigger = NULL;
		sev();
	} else {
 kdb_trap_done:
		db_readytoswitch[cpu_index(ci)] = NULL;
		dsb(ishst);
		__asm __volatile ("wfe");
	}
#endif
	if (restore_hw_watchpoints)
		db_md_breakwatchpoints_reload();

	return 1;
}
#endif

#if defined(_KERNEL)
static void
db_md_meminfo_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	unsigned blk;

	for (blk = 0; blk < bootconfig.dramblocks; blk++) {
		db_printf("blk[%u]: start %lx end %lx (pages %x)\n",
		    blk, bootconfig.dram[blk].address,
		    bootconfig.dram[blk].address +
		    (uint64_t)bootconfig.dram[blk].pages * PAGE_SIZE,
		    bootconfig.dram[blk].pages);
	}
}
#endif
