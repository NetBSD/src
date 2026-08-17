/*	$NetBSD: x86.c,v 1.5 2026/08/17 18:58:56 riastradh Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
#ifndef lint
__RCSID("$NetBSD: x86.c,v 1.5 2026/08/17 18:58:56 riastradh Exp $");
#endif /* not lint */

#include <ddb/ddb.h>

#include <kvm.h>
#include <nlist.h>
#include <err.h>
#include <stdlib.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <x86/db_machdep.h>

#include "extern.h"

#ifdef VM_MIN_KERNEL_ADDRESS_DEFAULT
vaddr_t vm_min_kernel_address = VM_MIN_KERNEL_ADDRESS_DEFAULT;
#endif

static struct nlist nl[] = {
	{ .n_name = "_dumppcb" },
	{ .n_name = "cpu_infos" },
	{ .n_name = "cpu_info_primary" },
	{ .n_name = "ncpu" },
#ifdef VM_MIN_KERNEL_ADDRESS_DEFAULT
	{ .n_name = "vm_min_kernel_address" },
#endif
	{ .n_name = NULL },
};

struct pcb	pcb;
struct cpu_info	**cpu_infos;
struct cpu_info	*cpu_info_primary_p;
int		ncpu;

void
db_mach_init(kvm_t *kd)
{

	if (kvm_nlist(kd, nl) == -1) {
		errx(EXIT_FAILURE, "kvm_nlist: %s", kvm_geterr(kd));
	}
	if ((size_t)kvm_read(kd, nl[0].n_value, &pcb, sizeof(pcb)) !=
	    sizeof(pcb)) {
		errx(EXIT_FAILURE, "cannot read dumppcb: %s", kvm_geterr(kd));
	}
	if ((size_t)kvm_read(kd, nl[1].n_value, &cpu_infos, sizeof(cpu_infos))
	    != sizeof(cpu_infos)) {
		errx(EXIT_FAILURE, "cannot read cpu_infos: %s", kvm_geterr(kd));
	}
	cpu_info_primary_p = (void *)(uintptr_t)nl[2].n_value;
	if ((size_t)kvm_read(kd, nl[3].n_value, &ncpu, sizeof(ncpu)) !=
	    sizeof(ncpu)) {
		errx(EXIT_FAILURE, "cannot read ncpu: %s", kvm_geterr(kd));
	}
#ifdef VM_MIN_KERNEL_ADDRESS_DEFAULT
	if ((size_t)kvm_read(kd, nl[4].n_value, &vm_min_kernel_address,
	    sizeof(vm_min_kernel_address)) != sizeof(vm_min_kernel_address)) {
		errx(EXIT_FAILURE, "cannot read vm_min_kernel_address: %s",
		    kvm_geterr(kd));
	}
#endif
        ddb_regs.tf_sp = pcb.pcb_sp;
        ddb_regs.tf_bp = pcb.pcb_bp;
        if (ddb_regs.tf_bp != 0 && ddb_regs.tf_sp != 0) {
        	printf("Backtrace from time of crash is available.\n");
	}
}

void db_mach_cpu(db_expr_t, bool, db_expr_t, const char *);

const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD("cpu",	db_mach_cpu,	0,
	  "switch to another cpu", "cpu-no", NULL) },
	{ DDB_END_CMD },
};

static struct cpu_info *
cpu_lookup(unsigned i)
{
	struct cpu_info *ci;

	if (cpu_infos == NULL) {
		if (i != 0)
			return NULL;
		return cpu_info_primary_p;
	}
	if (ncpu < 0 || i >= (unsigned)ncpu)
		return NULL;
	db_read_bytes((db_addr_t)(uintptr_t)&cpu_infos[i],
	    sizeof(ci), (char *)&ci);
	return ci;
}

/*
 * XXX sync with sys/arch/amd64/amd64/db_interface.c
 * XXX sync with sys/arch/i386/i386/db_interface.c
 */
void
db_mach_cpu(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct cpu_info *ci;
	db_regs_t *regs;

#if 0
	if (!have_addr) {
		cpu_debug_dump();
		return;
	}
#endif

	if (addr < 0) {
		db_printf("%ld: CPU out of range\n", addr);
		return;
	}
	ci = cpu_lookup(addr);
	if (ci == NULL) {
		db_printf("CPU %ld not configured\n", addr);
		return;
	}
#if 0
	if (ci != curcpu()) {
		if (!(ci->ci_flags & CPUF_PAUSE)) {
			db_printf("CPU %ld not paused\n", addr);
			return;
		}
	}
#endif
	db_read_bytes((db_addr_t)(uintptr_t)&ci->ci_ddb_regs,
	    sizeof(regs), (char *)&regs);
	if (regs == NULL) {
		db_printf("CPU %ld has no saved regs\n", addr);
		return;
	}
	db_printf("using CPU %ld", addr);
	db_read_bytes((db_addr_t)(uintptr_t)&regs->tf_sp,
	    sizeof(ddb_regs.tf_sp), (char *)&ddb_regs.tf_sp);
	db_read_bytes((db_addr_t)(uintptr_t)&regs->tf_bp,
	    sizeof(ddb_regs.tf_bp), (char *)&ddb_regs.tf_bp);
}
