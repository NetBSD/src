/* $NetBSD: aarch64_machdep.c,v 1.3 2018/05/03 15:47:22 ryo Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: aarch64_machdep.c,v 1.3 2018/05/03 15:47:22 ryo Exp $");

#include "opt_arm_debug.h"
#include "opt_ddb.h"
#include "opt_kernhist.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/kauth.h>
#include <sys/msgbuf.h>
#include <sys/sysctl.h>

#include <dev/mm.h>

#include <uvm/uvm.h>

#include <machine/bootconfig.h>

#include <aarch64/armreg.h>
#include <aarch64/cpufunc.h>
#ifdef DDB
#include <aarch64/db_machdep.h>
#endif
#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/pmap.h>
#include <aarch64/pte.h>
#include <aarch64/vmparam.h>

char cpu_model[32];
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;

/* sysctl node num */
static int sysctlnode_machdep_cpu_id;
static int sysctlnode_machdep_id_revidr;
static int sysctlnode_machdep_id_mvfr;
static int sysctlnode_machdep_id_mpidr;
static int sysctlnode_machdep_id_aa64isar;
static int sysctlnode_machdep_id_aa64mmfr;
static int sysctlnode_machdep_id_aa64pfr;

const pcu_ops_t * const pcu_ops_md_defs[PCU_UNIT_COUNT] = {
	[PCU_FPU] = &pcu_fpu_ops
};

struct vm_map *phys_map;

/* XXX */
vaddr_t physical_start;
vaddr_t physical_end;
u_long kern_vtopdiff;


/*
 * Upper region: 0xffff_ffff_ffff_ffff  Top of virtual memory
 *
 *               0xffff_ffff_ffe0_0000  End of KVA
 *                                      = VM_MAX_KERNEL_ADDRESS
 *
 *               0xffff_ffc0_0???_????  End of kernel
 *                                      = _end[]
 *               0xffff_ffc0_00??_????  Start of kernel
 *                                      = __kernel_text[]
 *
 *               0xffff_ffc0_0000_0000  Kernel base address & start of KVA
 *                                      = VM_MIN_KERNEL_ADDRESS
 *
 *               0xffff_ffbf_ffff_ffff  End of direct mapped
 *               0xffff_0000_0000_0000  Start of direct mapped
 *                                      = AARCH64_KSEG_START
 *                                      = AARCH64_KMEMORY_BASE
 *
 * Hole:         0xfffe_ffff_ffff_ffff
 *               0x0001_0000_0000_0000
 *
 * Lower region: 0x0000_ffff_ffff_ffff  End of user address space
 *                                      = VM_MAXUSER_ADDRESS
 *
 *               0x0000_0000_0000_0000  Start of user address space
 */
vaddr_t
initarm_common(vaddr_t kvm_base, vsize_t kvm_size,
    const struct boot_physmem *bp, size_t nbp)
{
	extern char __kernel_text[];
	extern char _end[];
	extern char lwp0uspace[];

	struct trapframe *tf;
	psize_t memsize_total;
	vaddr_t kernstart, kernend;
	vaddr_t kernstart_l2, kernend_l2;	/* L2 table 2MB aligned */
	paddr_t kstartp, kendp;			/* physical page of kernel */
	int i;

	aarch64_getcacheinfo();

	cputype = cpu_idnum();	/* for compatible arm */

	kernstart = trunc_page((vaddr_t)__kernel_text);
	kernend = round_page((vaddr_t)_end);
	kernstart_l2 = kernstart & -L2_SIZE;		/* trunk L2_SIZE(2M) */
	kernend_l2 = (kernend + L2_SIZE - 1) & -L2_SIZE;/* round L2_SIZE(2M) */

	paddr_t kernstart_phys = KERN_VTOPHYS(kernstart);
	paddr_t kernend_phys = KERN_VTOPHYS(kernend);

	/* XXX */
	physical_start = bootconfig.dram[0].address;
	physical_end = physical_start + ptoa(bootconfig.dram[0].pages);

#ifdef VERBOSE_INIT_ARM
	printf(
	    "------------------------------------------\n"
	    "kern_vtopdiff         = 0x%016lx\n"
	    "physical_start        = 0x%016lx\n"
	    "kernel_start_phys     = 0x%016lx\n"
	    "kernel_end_phys       = 0x%016lx\n"
	    "physical_end          = 0x%016lx\n"
	    "VM_MIN_KERNEL_ADDRESS = 0x%016lx\n"
	    "kernel_start_l2       = 0x%016lx\n"
	    "kernel_start          = 0x%016lx\n"
	    "kernel_end            = 0x%016lx\n"
	    "kernel_end_l2         = 0x%016lx\n"
	    "(kernel va area)\n"
	    "(devmap va area)\n"
	    "VM_MAX_KERNEL_ADDRESS = 0x%016lx\n"
	    "------------------------------------------\n",
	    kern_vtopdiff,
	    physical_start,
	    kernstart_phys,
	    kernend_phys,
	    physical_end,
	    VM_MIN_KERNEL_ADDRESS,
	    kernstart_l2,
	    kernstart,
	    kernend,
	    kernend_l2,
	    VM_MAX_KERNEL_ADDRESS);
#endif

	/*
	 * msgbuf is always allocated from bottom of 1st memory block.
	 * against corruption by bootloader, or changing kernel layout.
	 */
	physical_end -= round_page(MSGBUFSIZE);
	bootconfig.dram[0].pages -= atop(round_page(MSGBUFSIZE));
	initmsgbuf(AARCH64_PA_TO_KVA(physical_end), MSGBUFSIZE);

#ifdef DDB
	db_machdep_init();
#endif

	uvm_md_init();

	/* register free physical memory blocks */
	memsize_total = 0;
	kstartp = atop(kernstart_phys);
	kendp = atop(kernend_phys);

	KASSERT(bp != NULL || nbp == 0);
	KASSERT(bp == NULL || nbp != 0);

	KDASSERT(bootconfig.dramblocks <= DRAM_BLOCKS);
	for (i = 0; i < bootconfig.dramblocks; i++) {
		paddr_t start, end;

		/* empty is end */
		if (bootconfig.dram[i].address == 0 &&
		    bootconfig.dram[i].pages == 0)
			break;

		start = atop(bootconfig.dram[i].address);
		end = start + bootconfig.dram[i].pages;

		int vm_freelist = VM_FREELIST_DEFAULT;
		/*
		 * This assumes the bp list is sorted in ascending
		 * order.
		 */
		paddr_t segend = end;
		for (size_t j = 0; j < nbp; j++ /*, start = segend, segend = end */) {
			paddr_t bp_start = bp[j].bp_start;
			paddr_t bp_end = bp_start + bp[j].bp_pages;

			KASSERT(bp_start < bp_end);
			if (start > bp_end || segend < bp_start)
				continue;

			if (start < bp_start)
				start = bp_start;

			if (start < bp_end) {
				if (segend > bp_end) {
					segend = bp_end;
				}
				vm_freelist = bp[j].bp_freelist;

				uvm_page_physload(start, segend, start, segend,
				    vm_freelist);
				memsize_total += ptoa(segend - start);
				start = segend;
				segend = end;
			}
		}
	}
	physmem = atop(memsize_total);

	/*
	 * kernel image is mapped on L2 table (2MB*n) by locore.S
	 * virtual space start from 2MB aligned kernend
	 */
	pmap_bootstrap(kernend_l2, VM_MAX_KERNEL_ADDRESS);

	/*
	 * setup lwp0
	 */
	uvm_lwp_setuarea(&lwp0, lwp0uspace);
	memset(&lwp0.l_md, 0, sizeof(lwp0.l_md));
	memset(lwp_getpcb(&lwp0), 0, sizeof(struct pcb));

	tf = (struct trapframe *)(lwp0uspace + USPACE) - 1;
	memset(tf, 0, sizeof(struct trapframe));
	tf->tf_spsr = SPSR_M_EL0T;
	lwp0.l_md.md_utf = lwp0.l_md.md_ktf = tf;

	return tf;
}

/*
 * machine dependent system variables.
 */
static int
aarch64_sysctl_machdep_sysreg_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint64_t databuf[8];
	void *data;

	node = *rnode;
	node.sysctl_data = data = &databuf;

	/*
	 * Don't keep values in advance due to system registers may have
	 * different values on each CPU cores. (e.g. big.LITTLE)
	 */
	if (rnode->sysctl_num == sysctlnode_machdep_cpu_id) {
		((uint32_t *)data)[0] = reg_midr_el1_read();
		node.sysctl_size = sizeof(uint32_t);

	} else if (rnode->sysctl_num == sysctlnode_machdep_id_revidr) {
		((uint32_t *)data)[0] = reg_revidr_el1_read();
		node.sysctl_size = sizeof(uint32_t);

	} else if (rnode->sysctl_num == sysctlnode_machdep_id_mvfr) {
		((uint32_t *)data)[0] = reg_mvfr0_el1_read();
		((uint32_t *)data)[1] = reg_mvfr1_el1_read();
		((uint32_t *)data)[2] = reg_mvfr2_el1_read();
		node.sysctl_size = sizeof(uint32_t) * 3;

	} else if (rnode->sysctl_num == sysctlnode_machdep_id_mpidr) {
		((uint64_t *)data)[0] = reg_mpidr_el1_read();
		node.sysctl_size = sizeof(uint64_t);

	} else if (rnode->sysctl_num == sysctlnode_machdep_id_aa64isar) {
		((uint64_t *)data)[0] = reg_id_aa64isar0_el1_read();
		((uint64_t *)data)[1] = reg_id_aa64isar1_el1_read();
		node.sysctl_size = sizeof(uint64_t) * 2;

	} else if (rnode->sysctl_num == sysctlnode_machdep_id_aa64mmfr) {
		((uint64_t *)data)[0] = reg_id_aa64mmfr0_el1_read();
		((uint64_t *)data)[1] = reg_id_aa64mmfr1_el1_read();
		node.sysctl_size = sizeof(uint64_t) * 2;

	} else if (rnode->sysctl_num == sysctlnode_machdep_id_aa64pfr) {
		((uint64_t *)data)[0] = reg_id_aa64pfr0_el1_read();
		((uint64_t *)data)[1] = reg_id_aa64pfr1_el1_read();
		node.sysctl_size = sizeof(uint64_t) * 2;

	} else {
		return EOPNOTSUPP;
	}

	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{
	const struct sysctlnode *node;

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_INT,
	    "cpu_id",
	    SYSCTL_DESCR("MIDR_EL1, Main ID Register"),
	    aarch64_sysctl_machdep_sysreg_helper, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctlnode_machdep_cpu_id = node->sysctl_num;

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_INT,
	    "id_revidr",
	    SYSCTL_DESCR("REVIDR_EL1, Revision ID Register"),
	    aarch64_sysctl_machdep_sysreg_helper, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctlnode_machdep_id_revidr = node->sysctl_num;

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRUCT,
	    "id_mvfr",
	    SYSCTL_DESCR("MVFRn_EL1, Media and VFP Feature Registers"),
	    aarch64_sysctl_machdep_sysreg_helper, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctlnode_machdep_id_mvfr = node->sysctl_num;

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRUCT,
	    "id_mpidr",
	    SYSCTL_DESCR("MPIDR_EL1, Multiprocessor Affinity Register"),
	    aarch64_sysctl_machdep_sysreg_helper, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctlnode_machdep_id_mpidr = node->sysctl_num;

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRUCT,
	    "id_aa64isar",
	    SYSCTL_DESCR("ID_AA64ISARn_EL1, "
	    "AArch64 Instruction Set Attribute Registers"),
	    aarch64_sysctl_machdep_sysreg_helper, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctlnode_machdep_id_aa64isar = node->sysctl_num;

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRUCT,
	    "id_aa64mmfr",
	    SYSCTL_DESCR("ID_AA64MMFRn_EL1, "
	    "AArch64 Memory Model Feature Registers"),
	    aarch64_sysctl_machdep_sysreg_helper, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctlnode_machdep_id_aa64mmfr = node->sysctl_num;

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRUCT,
	    "id_aa64pfr",
	    SYSCTL_DESCR("ID_AA64PFRn_EL1, "
	    "AArch64 Processor Feature Registers"),
	    aarch64_sysctl_machdep_sysreg_helper, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctlnode_machdep_id_aa64pfr = node->sysctl_num;
}

void
parse_mi_bootargs(char *args)
{
}

void
machdep_init(void)
{
	/* clear cpu reset hook for early boot */
	cpu_reset_address0 = NULL;
}

bool
mm_md_direct_mapped_phys(paddr_t pa, vaddr_t *vap)
{
	/* XXX */
	if (physical_start <= pa && pa < physical_end) {
		*vap = AARCH64_PA_TO_KVA(pa);
		return true;
	}

	return false;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{
	/* XXX */
	if (physical_start <= pa && pa < physical_end)
		return 0;

	return kauth_authorize_machdep(kauth_cred_get(),
	    KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL);
}

void
cpu_startup(void)
{
	vaddr_t maxaddr, minaddr;

	consinit();

	/*
	 * Allocate a submap for physio.
	 */
	minaddr = 0;
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	   VM_PHYS_SIZE, 0, FALSE, NULL);

	/* Hello! */
	banner();
}

void
cpu_dumpconf(void)
{
}
