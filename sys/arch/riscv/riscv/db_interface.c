/*	$NetBSD: db_interface.c,v 1.3 2023/06/12 19:04:14 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.3 2023/06/12 19:04:14 skrll Exp $");

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#include "opt_kgdb.h"
#endif

#define __PMAP_PRIVATE

#include <sys/types.h>
#include <sys/param.h>

#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_active.h>
#include <ddb/db_user.h>
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

#ifdef _KERNEL
db_regs_t	ddb_regs;
#endif

#ifdef MULTIPROCESSOR
static void db_mach_cpu_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif

void db_tlbdump_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_kvtophys_cmd(db_expr_t, bool, db_expr_t, const char *);

paddr_t kvtophys(vaddr_t);

#ifdef _KERNEL

#ifndef KGDB
int
kdb_trap(int type, db_regs_t *regs)
{
	int s;

	switch (type) {
	case CAUSE_BREAKPOINT:	/* breakpoint */
		printf("kernel: breakpoint\n");
		break;
	case -1:		/* keyboard interrupt */
		printf("kernel: kdbint trap\n");
		break;
	default:
		printf("kernel: cause %d\n", type);
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
			cpu_pause();
			splx(s);
			return 1;
		}
	}
	KASSERT(!cpu_is_paused(cpu_me));
#endif
	struct cpu_info * const ci = curcpu();

	ddb_regs = *regs;
	ci->ci_ddb_regs = &ddb_regs;
	db_active++;
	cnpollc(1);
	db_trap(type, 0 /*code*/);
	cnpollc(0);
	db_active--;
	ci->ci_ddb_regs = NULL;
	*regs = ddb_regs;

#if defined(MULTIPROCESSOR)
	if (atomic_cas_uint(&ddb_cpu, cpu_me, NOCPU) == cpu_me) {
		cpu_resume_others();
	} else {
		cpu_resume(ddb_cpu);
		if (first_in_ddb)
			cpu_pause();
	}
#endif

	splx(s);
	return 1;
}
#endif	/* !KGDB */


#ifndef KGDB
void
db_tlbdump_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
	       const char *modif)
{
}

void
db_kvtophys_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		const char *modif)
{

	if (!have_addr)
		return;
}


static void
db_mach_reset_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
		const char *modif)
{
}


const struct db_command db_machine_command_table[] = {
#ifdef MULTIPROCESSOR
	{ DDB_ADD_CMD("cpu",	db_mach_cpu_cmd,	0,
	  "switch to another cpu", "cpu#", NULL) },
#endif
	{ DDB_ADD_CMD("kvtop",	db_kvtophys_cmd,	0,
		"Print the physical address for a given kernel virtual address",
		"address",
		"   address:\tvirtual address to look up") },
	{ DDB_ADD_CMD("reset", 	db_mach_reset_cmd,	CS_NOREPEAT,
		"Initiate hardware reset",
		NULL, NULL) },
	{ DDB_ADD_CMD("tlb",	db_tlbdump_cmd,		0,
		"Print out TLB entries. (only works with options DEBUG)",
		NULL, NULL) },
	{ DDB_END_CMD },
};
#endif	/* !KGDB */

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

#endif	/* _KERNEL */
