/*	$NetBSD: arm32_boot.c,v 1.1.2.3 2014/08/20 00:02:45 tls Exp $	*/

/*
 * Copyright (c) 2002, 2003, 2005  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997,1998 Causality Limited.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (c) 2007 Microsoft
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: arm32_boot.c,v 1.1.2.3 2014/08/20 00:02:45 tls Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/atomic.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <arm/locore.h>
#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <machine/db_machdep.h>
#include <ddb/db_extern.h>

#include <machine/bootconfig.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

vaddr_t
initarm_common(vaddr_t kvm_base, vsize_t kvm_size,
	const struct boot_physmem *bp, size_t nbp)
{
	struct bootmem_info * const bmi = &bootmem_info;

#ifdef VERBOSE_INIT_ARM
	printf("nfreeblocks = %u, free_pages = %d (%#x)\n",
	    bmi->bmi_nfreeblocks, bmi->bmi_freepages,
	    bmi->bmi_freepages);
#endif

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init.
	 */
	uvm_lwp_setuarea(&lwp0, kernelstack.pv_va);

#ifdef VERBOSE_INIT_ARM
	printf("bootstrap done.\n");
#endif

#ifdef VERBOSE_INIT_ARM
	printf("vectors");
#endif
	arm32_vector_init(systempage.pv_va, ARM_VEC_ALL);
#ifdef VERBOSE_INIT_ARM
	printf(" %#"PRIxVADDR"\n", vector_page);
#endif

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("init subsystems: stacks ");
#endif
	set_stackptr(PSR_FIQ32_MODE,
	    fiqstack.pv_va + FIQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_IRQ32_MODE,
	    irqstack.pv_va + IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE,
	    abtstack.pv_va + ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE,
	    undstack.pv_va + UND_STACK_SIZE * PAGE_SIZE);

	/*
	 * Well we should set a data abort handler.
	 * Once things get going this will change as we will need a proper
	 * handler.
	 * Until then we will use a handler that just panics but tells us
	 * why.
	 * Initialisation of the vectors will just panic on a data abort.
	 * This just fills in a slightly better one.
	 */
#ifdef VERBOSE_INIT_ARM
	printf("vectors ");
#endif
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

	/* Initialise the undefined instruction handlers */
#ifdef VERBOSE_INIT_ARM
	printf("undefined ");
#endif
	undefined_init();

	/* Load memory into UVM. */
#ifdef VERBOSE_INIT_ARM
	printf("page ");
#endif
	uvm_setpagesize();	/* initialize PAGE_SIZE-dependent variables */

#ifdef VERBOSE_INIT_ARM
	printf("pmap_physload ");
#endif
	KASSERT(bp != NULL || nbp == 0);
	KASSERT(bp == NULL || nbp != 0);

	for (size_t i = 0; i < bmi->bmi_nfreeblocks; i++) {
		pv_addr_t * const pv = &bmi->bmi_freeblocks[i];
		paddr_t start = atop(pv->pv_pa);
		const paddr_t end = start + atop(pv->pv_size);

		while (start < end) {
			int vm_freelist = VM_FREELIST_DEFAULT;
			paddr_t segend = end;
			/*
			 * This assumes the bp list is sorted in ascending
			 * order.
			 */
			for (size_t j = 0; j < nbp; j++) {
				paddr_t bp_start = bp[j].bp_start;
				paddr_t bp_end = bp_start + bp[j].bp_pages;
				if (start < bp_start) {
					if (segend > bp_start) {
						segend = bp_start;
					}
					break;
				}
				if (start < bp_end) {
					if (segend > bp_end) {
						segend = bp_end;
					}
					vm_freelist = bp[j].bp_freelist;
					break;
				}
			}
	
			uvm_page_physload(start, segend, start, segend,
			    vm_freelist);
			start = segend;
		}
	}

	/* Boot strap pmap telling it where the kernel page table is */
#ifdef VERBOSE_INIT_ARM
	printf("pmap ");
#endif
	pmap_bootstrap(kvm_base, kvm_base + kvm_size);
   
#ifdef __HAVE_MEMORY_DISK__
	md_root_setconf(memory_disk, sizeof memory_disk);
#endif

#ifdef BOOTHOWTO
	boothowto |= BOOTHOWTO;
#endif

#ifdef KGDB
	if (boothowto & RB_KDB) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif

#ifdef DDB
	db_machine_init();
	ddb_init(0, NULL, NULL);

	if (boothowto & RB_KDB)
		Debugger();
#endif

#ifdef VERBOSE_INIT_ARM
	printf("done.\n");
#endif

	/* We return the new stack pointer address */
	return kernelstack.pv_va + USPACE_SVC_STACK_TOP;
}

#ifdef MULTIPROCESSOR
/*
 * When we are called, the MMU and caches are on and we are running on the stack
 * of the idlelwp for this cpu.
 */
void
cpu_hatch(struct cpu_info *ci, cpuid_t cpuid, void (*md_cpu_init)(struct cpu_info *))
{
	KASSERT(cpu_index(ci) == cpuid);

	/*
	 * Raise our IPL to the max
	 */
	splhigh();

#ifdef VERBOSE_INIT_ARM
	printf("%s(%s): ", __func__, ci->ci_data.cpu_name);
#endif
	uint32_t mpidr = armreg_mpidr_read();
	if (mpidr & MPIDR_MT) {
		ci->ci_data.cpu_smt_id = mpidr & MPIDR_AFF0;
		ci->ci_data.cpu_core_id = mpidr & MPIDR_AFF1;
		ci->ci_data.cpu_package_id = mpidr & MPIDR_AFF2;
	} else {
		ci->ci_data.cpu_core_id = mpidr & MPIDR_AFF0;
		ci->ci_data.cpu_package_id = mpidr & MPIDR_AFF1;
	}

	/*
	 * Make sure we have the right vector page.
	 */
#ifdef VERBOSE_INIT_ARM
	printf(" vectors");
#endif
	arm32_vector_init(systempage.pv_va, ARM_VEC_ALL);

	/*
	 * Initialize the stack for each mode (we are already running on the
	 * SVC32 stack of the idlelwp).
	 */
#ifdef VERBOSE_INIT_ARM
	printf(" stacks");
#endif
	set_stackptr(PSR_FIQ32_MODE,
	    fiqstack.pv_va + cpu_index(ci) * FIQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_IRQ32_MODE,
	    irqstack.pv_va + cpu_index(ci) * IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE,
	    abtstack.pv_va + cpu_index(ci) * ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE,
	    undstack.pv_va + cpu_index(ci) * UND_STACK_SIZE * PAGE_SIZE);

	ci->ci_lastlwp = NULL;
	ci->ci_pmap_lastuser = NULL;
#ifdef ARM_MMU_EXTENDED
#ifdef VERBOSE_INIT_ARM
	printf(" tlb");
#endif
	/*
	 * Attach to the tlb.
	 */
	ci->ci_pmap_cur = pmap_kernel();
	ci->ci_pmap_asid_cur = KERNEL_PID;
#endif

#ifdef CPU_CORTEX
	if (CPU_ID_CORTEX_P(ci->ci_arm_cpuid)) {
		/*
		 * Start and reset the PMC Cycle Counter.
		 */   
		armreg_pmcr_write(ARM11_PMCCTL_E|ARM11_PMCCTL_P|ARM11_PMCCTL_C);
		armreg_pmcntenset_write(CORTEX_CNTENS_C);
	}
#endif

	aprint_naive("%s", device_xname(ci->ci_dev));
	aprint_normal("%s", device_xname(ci->ci_dev));
	identify_arm_cpu(ci->ci_dev, ci);
#ifdef VERBOSE_INIT_ARM
	printf(" vfp");
#endif
	vfp_attach(ci);

#ifdef VERBOSE_INIT_ARM
	printf(" interrupts");
#endif
	/*
	 * Let the interrupts do what they need to on this CPU.
	 */
	intr_cpu_init(ci);

#ifdef VERBOSE_INIT_ARM
	printf(" md(%p)", md_cpu_init);
#endif
	if (md_cpu_init != NULL)
		(*md_cpu_init)(ci);

#ifdef VERBOSE_INIT_ARM
	printf(" done!\n");
#endif
	atomic_and_32(&arm_cpu_mbox, ~(1 << cpuid));
	__asm __volatile("sev; sev; sev");
}
#endif /* MULTIPROCESSOR */
