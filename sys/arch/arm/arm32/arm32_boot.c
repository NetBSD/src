/*	$NetBSD: arm32_boot.c,v 1.45 2022/12/22 06:58:08 ryo Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: arm32_boot.c,v 1.45 2022/12/22 06:58:08 ryo Exp $");

#include "opt_arm_debug.h"
#include "opt_cputypes.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>

#include <sys/asan.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/reboot.h>

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

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

vaddr_t
initarm_common(vaddr_t kvm_base, vsize_t kvm_size,
	const struct boot_physmem *bp, size_t nbp)
{
	struct bootmem_info * const bmi = &bootmem_info;

	VPRINTF("nfreeblocks = %u, free_pages = %d (%#x)\n",
	    bmi->bmi_nfreeblocks, bmi->bmi_freepages,
	    bmi->bmi_freepages);

	/*
	 * Moved from cpu_startup() as data_abort_handler() references
	 * this during uvm init.
	 */
	uvm_lwp_setuarea(&lwp0, kernelstack.pv_va);

	struct lwp * const l = &lwp0;
	struct pcb * const pcb = lwp_getpcb(l);

	/* Zero out the PCB. */
 	memset(pcb, 0, sizeof(*pcb));

	pcb->pcb_ksp = uvm_lwp_getuarea(l) + USPACE_SVC_STACK_TOP;
	pcb->pcb_ksp -= sizeof(struct trapframe);

	struct trapframe * tf = (struct trapframe *)pcb->pcb_ksp;

	/* Zero out the trapframe. */
	memset(tf, 0, sizeof(*tf));
	lwp_settrapframe(l, tf);

 	tf->tf_spsr = PSR_USR32_MODE;
#ifdef _ARM_ARCH_BE8
	tf->tf_spsr |= PSR_E_BIT;
#endif

	VPRINTF("bootstrap done.\n");

	VPRINTF("vectors");
	arm32_vector_init(systempage.pv_va, ARM_VEC_ALL);
	VPRINTF(" %#"PRIxVADDR"\n", vector_page);

	/*
	 * Pages were allocated during the secondary bootstrap for the
	 * stacks for different CPU modes.
	 * We must now set the r13 registers in the different CPU modes to
	 * point to these stacks.
	 * Since the ARM stacks use STMFD etc. we must set r13 to the top end
	 * of the stack memory.
	 */
	VPRINTF("init subsystems: stacks ");
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
	VPRINTF("vectors ");
	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address = (u_int)undefinedinstruction_bounce;

	/* Initialise the undefined instruction handlers */
	VPRINTF("undefined ");
	undefined_init();

#ifdef FPU_VFP
	/* vfp_detect uses an undefined handler */
	VPRINTF("vfp ");
	vfp_detect(curcpu());
#endif

	/* Load memory into UVM. */
	VPRINTF("page ");
	uvm_md_init();

	VPRINTF("pmap_physload\n");
	KASSERT(bp != NULL || nbp == 0);
	KASSERT(bp == NULL || nbp != 0);

	for (size_t i = 0; i < bmi->bmi_nfreeblocks; i++) {
		pv_addr_t * const pv = &bmi->bmi_freeblocks[i];
		paddr_t start = atop(pv->pv_pa);
		const paddr_t end = start + atop(pv->pv_size);
		int vm_freelist = VM_FREELIST_DEFAULT;

		VPRINTF("block %2zu start %08lx  end %08lx", i,
		    pv->pv_pa, pv->pv_pa + pv->pv_size);

		if (!bp) {
			VPRINTF("... loading in freelist %d\n", vm_freelist);
			uvm_page_physload(start, end, start, end, VM_FREELIST_DEFAULT);
			continue;
		}
		VPRINTF("\n");

		/*
		 * This assumes the bp list is sorted in ascending
		 * order.
		 */
		paddr_t segend = end;
		for (size_t j = 0; j < nbp && start < end; j++) {
			paddr_t bp_start = bp[j].bp_start;
			paddr_t bp_end = bp_start + bp[j].bp_pages;

			VPRINTF("   bp %2zu start %08lx  end %08lx\n",
			    j, ptoa(bp_start), ptoa(bp_end));

			KASSERT(bp_start < bp_end);
			if (start >= bp_end || segend < bp_start)
				continue;

			if (start < bp_start)
				start = bp_start;

			if (start < bp_end) {
				if (segend > bp_end) {
					segend = bp_end;
				}
				vm_freelist = bp[j].bp_freelist;

				VPRINTF("         start %08lx  end %08lx"
				    "... loading in freelist %d\n", ptoa(start),
				    ptoa(segend), vm_freelist);

				uvm_page_physload(start, segend, start, segend,
				    vm_freelist);

				start = segend;
				segend = end;
			}
		}
	}

	/*
	 * Bootstrap pmap telling it where the managed kernel virtual memory
	 * is.
	 */
	VPRINTF("pmap ");
	pmap_bootstrap(kvm_base, kvm_base + kvm_size);

	kasan_init();

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

#ifdef MULTIPROCESSOR
	/*
	 * Ensure BP cache is flushed to memory so that APs start cache
	 * coherency with correct view.
	 */
	cpu_dcache_wbinv_all();
#endif

	VPRINTF("done.\n");

	/* We return the new stack pointer address */
	return pcb->pcb_ksp;
}

#ifdef MULTIPROCESSOR
/*
 * When we are called, the MMU and caches are on and we are running on the stack
 * of the idlelwp for this cpu.
 */
void
cpu_hatch(struct cpu_info *ci, u_int cpuindex, void (*md_cpu_init)(struct cpu_info *))
{
	KASSERT(cpu_index(ci) == cpuindex);

	/*
	 * Raise our IPL to the max
	 */
	splhigh();

	VPRINTF("%s(%s): ", __func__, cpu_name(ci));
	/* mpidr/midr filled in by cpu_init_secondary_processor */

	/*
	 * Make sure we have the right vector page.
	 */
	VPRINTF(" vectors");
	arm32_vector_init(systempage.pv_va, ARM_VEC_ALL);

	/*
	 * Initialize the stack for each mode (we are already running on the
	 * SVC32 stack of the idlelwp).
	 */
	VPRINTF(" stacks");
	set_stackptr(PSR_FIQ32_MODE,
	    fiqstack.pv_va + (cpu_index(ci) + 1) * FIQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_IRQ32_MODE,
	    irqstack.pv_va + (cpu_index(ci) + 1) * IRQ_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE,
	    abtstack.pv_va + (cpu_index(ci) + 1) * ABT_STACK_SIZE * PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE,
	    undstack.pv_va + (cpu_index(ci) + 1) * UND_STACK_SIZE * PAGE_SIZE);

	ci->ci_lastlwp = NULL;
	ci->ci_pmap_lastuser = NULL;
#ifdef ARM_MMU_EXTENDED
	VPRINTF(" tlb");
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
		armreg_pmintenclr_write(PMINTEN_C | PMINTEN_P);
		armreg_pmcntenset_write(CORTEX_CNTENS_C);
	}
#endif

	VPRINTF(" md(%p)", md_cpu_init);
	if (md_cpu_init != NULL)
		(*md_cpu_init)(ci);

	VPRINTF(" interrupts");
	/*
	 * Let the interrupts do what they need to on this CPU.
	 */
	intr_cpu_init(ci);

	VPRINTF(" done!\n");
	cpu_clr_mbox(cpuindex);
}
#endif /* MULTIPROCESSOR */
