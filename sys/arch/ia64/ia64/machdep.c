/*	$NetBSD: machdep.c,v 1.7.4.3 2009/08/19 18:46:21 yamt Exp $	*/

/*-
 * Copyright (c) 2003,2004 Marcel Moolenaar
 * Copyright (c) 2000,2001 Doug Rabson
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Chris G. Demetriou.
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

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
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

#include <sys/cdefs.h>
/*__FBSDID("$FreeBSD: src/sys/ia64/ia64/machdep.c,v 1.203 2005/10/14 12:43:45 davidxu Exp $"); */

#include "opt_modular.h"

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/exec.h>
#include <sys/ksyms.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/msgbuf.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <machine/ia64_cpu.h>
#include <machine/pal.h>
#include <machine/sal.h>
#include <machine/ssc.h>

#include <machine/md_var.h>
#include <machine/fpu.h>
#include <machine/efi.h>
#include <machine/bootinfo.h>
#include <machine/vmparam.h>

#include <machine/atomic.h>
#include <machine/pte.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

#if NKSYMS || defined(DDB) || defined(MODULAR)
/* start and end of kernel symbol table */
void	*ksym_start, *ksym_end;
vaddr_t ia64_unwindtab;
vsize_t ia64_unwindtablen;
#endif

struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

void *msgbufaddr;
int	physmem;

char	cpu_model[64];
char	cpu_family[64];

vaddr_t kernstart, kernend;

/* XXX: Move this stuff to cpu_info */

uint64_t processor_frequency;
uint64_t bus_frequency;
uint64_t itc_frequency;
uint64_t ia64_pal_base;
uint64_t ia64_port_base;


extern uint64_t ia64_gateway_page[];

uint64_t pa_bootinfo;
struct bootinfo bootinfo;


extern vaddr_t kernel_text, end;

struct fpswa_iface *fpswa_iface;

struct	user *proc0paddr; /* XXX: See: kern/kern_proc.c:proc0_init() */

#define Mhz     1000000L
#define Ghz     (1000L*Mhz)

static void
identifycpu(void)
{
	uint64_t vendor[3];
	const char *family_name, *model_name;
	uint64_t features, tmp;
	int number, revision, model, family, archrev;

	/*
	 * Assumes little-endian.
	 */
	vendor[0] = ia64_get_cpuid(0);
	vendor[1] = ia64_get_cpuid(1);
	vendor[2] = '\0';

	tmp = ia64_get_cpuid(3);
	number = (tmp >> 0) & 0xff;
	revision = (tmp >> 8) & 0xff;
	model = (tmp >> 16) & 0xff;
	family = (tmp >> 24) & 0xff;
	archrev = (tmp >> 32) & 0xff;

	family_name = model_name = "unknown";
	switch (family) {
	case 0x07:
		family_name = "Itanium";
		model_name = "Merced";
		break;
	case 0x1f:
		family_name = "Itanium 2";
		switch (model) {
		case 0x00:
			model_name = "McKinley";
			break;
		case 0x01:
			/*
			 * Deerfield is a low-voltage variant based on the
			 * Madison core. We need circumstantial evidence
			 * (i.e. the clock frequency) to identify those.
			 * Allow for roughly 1% error margin.
			 */
			tmp = processor_frequency >> 7;
			if ((processor_frequency - tmp) < 1*Ghz &&
			    (processor_frequency + tmp) >= 1*Ghz)
				model_name = "Deerfield";
			else
				model_name = "Madison";
			break;
		case 0x02:
			model_name = "Madison II";
			break;
		}
		break;
	}
	snprintf(cpu_family, sizeof(cpu_family), "%s", family_name);
	snprintf(cpu_model, sizeof(cpu_model), "%s", model_name);

	features = ia64_get_cpuid(4);

	printf("CPU: %s (", model_name);
	if (processor_frequency) {
		printf("%ld.%02ld-Mhz ", (processor_frequency + 4999) / Mhz,
		    ((processor_frequency + 4999) / (Mhz/100)) % 100);
	}
	printf("%s)\n", family_name);
	printf("  Origin = \"%s\"  Revision = %d\n", (char *) vendor, revision);
	printf("  Features = 0x%x\n", (uint32_t) features);

}

/*
 * Machine-dependent startup code
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	identifycpu();

	/* XXX: startrtclock(); */
#ifdef PERFMON
	perfmon_init();
#endif
	printf("Detected memory  = %ld (%ld MB)\n", ia64_ptob(physmem),
	    ptoa(physmem) / 1048576);

	/*
	 * Display any holes after the first chunk of extended memory.
	 */
	if (bootverbose) {
		int lcv, sizetmp;

		printf("Physical memory chunk(s):\n");
		for (lcv = 0;
		    lcv < vm_nphysseg || vm_physmem[lcv].avail_end != 0;
		    lcv++) {
			sizetmp = vm_physmem[lcv].avail_end -
			    vm_physmem[lcv].avail_start;

			printf("0x%016lx - 0x%016lx, %ld bytes (%d pages)\n",
			    ptoa(vm_physmem[lcv].avail_start),
				ptoa(vm_physmem[lcv].avail_end) - 1,
				    ptoa(sizetmp), sizetmp);
		}
		printf("Total number of segments: vm_nphysseg = %d \n",
		    vm_nphysseg);
	}

 	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use RR7 to
	 * map those pages.
	 */

	banner();

	if (fpswa_iface == NULL)
		printf("Warning: no FPSWA package supplied\n");
	else
		printf("FPSWA Revision = 0x%lx, Entry = %p\n",
		    (long)fpswa_iface->if_rev, (void *)fpswa_iface->if_fpswa);


	/*
	 * Traverse the MADT to discover IOSAPIC and Local SAPIC
	 * information.
	 */
	ia64_probe_sapics();
	/*XXX: ia64_mca_init();*/
}

void
cpu_reboot(int howto, char *bootstr)
{

	efi_reset_system();

	panic("XXX: Reset didn't work ? \n");
	/*NOTREACHED*/
}

bool
cpu_intr_p(void)
{
	return 0;
}

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first PAGE_SIZE of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	return;
}

void
map_pal_code(void)
{
	pt_entry_t pte;
	uint64_t psr;

	if (ia64_pal_base == 0)
		return;

	pte = PTE_PRESENT | PTE_MA_WB | PTE_ACCESSED | PTE_DIRTY |
	    PTE_PL_KERN | PTE_AR_RWX;
	pte |= ia64_pal_base & PTE_PPN_MASK;

	__asm __volatile("ptr.d %0,%1; ptr.i %0,%1" ::
			 "r"(IA64_PHYS_TO_RR7(ia64_pal_base)),
			 "r"(IA64_ID_PAGE_SHIFT<<2));

	__asm __volatile("mov	%0=psr" : "=r"(psr));
	__asm __volatile("rsm	psr.ic|psr.i");
	ia64_srlz_i();
	ia64_set_ifa(IA64_PHYS_TO_RR7(ia64_pal_base));
	ia64_set_itir(IA64_ID_PAGE_SHIFT << 2);
	ia64_srlz_d();
 	__asm __volatile("itr.d	dtr[%0]=%1" ::
			 "r"(1), "r"(*(pt_entry_t *)&pte));
	ia64_srlz_d();
 	__asm __volatile("itr.i	itr[%0]=%1" ::
			 "r"(1), "r"(*(pt_entry_t *)&pte));
	__asm __volatile("mov	psr.l=%0" :: "r" (psr));
	ia64_srlz_i();
}

void
map_gateway_page(void)
{
	pt_entry_t pte;
	uint64_t psr;

	pte = PTE_PRESENT | PTE_MA_WB | PTE_ACCESSED | PTE_DIRTY |
	    PTE_PL_KERN | PTE_AR_X_RX;
	pte |= (uint64_t)ia64_gateway_page & PTE_PPN_MASK;

	__asm __volatile("ptr.d %0,%1; ptr.i %0,%1" ::
	    "r"(VM_MAX_ADDRESS), "r"(PAGE_SHIFT << 2));

	__asm __volatile("mov	%0=psr" : "=r"(psr));
	__asm __volatile("rsm	psr.ic|psr.i");
	ia64_srlz_i();
	ia64_set_ifa(VM_MAX_ADDRESS);
	ia64_set_itir(PAGE_SHIFT << 2);
	ia64_srlz_d();
	__asm __volatile("itr.d	dtr[%0]=%1" :: "r"(3), "r"(*(pt_entry_t*)&pte));
	ia64_srlz_d();
	__asm __volatile("itr.i	itr[%0]=%1" :: "r"(3), "r"(*(pt_entry_t*)&pte));
	__asm __volatile("mov	psr.l=%0" :: "r" (psr));
	ia64_srlz_i();

	/* Expose the mapping to userland in ar.k5 */
	ia64_set_k5(VM_MAX_ADDRESS);
}

static void
calculate_frequencies(void)
{
	struct ia64_sal_result sal;
	struct ia64_pal_result pal;

	sal = ia64_sal_entry(SAL_FREQ_BASE, 0, 0, 0, 0, 0, 0, 0);
	pal = ia64_call_pal_static(PAL_FREQ_RATIOS, 0, 0, 0);
	if (sal.sal_status == 0 && pal.pal_status == 0) {
		if (bootverbose) {
			printf("Platform clock frequency %ld Hz\n",
			    sal.sal_result[0]);
			printf("Processor ratio %ld/%ld, Bus ratio %ld/%ld, "
			       "ITC ratio %ld/%ld\n",
			    pal.pal_result[0] >> 32,
			    pal.pal_result[0] & ((1L << 32) - 1),
			    pal.pal_result[1] >> 32,
			    pal.pal_result[1] & ((1L << 32) - 1),
			    pal.pal_result[2] >> 32,
			    pal.pal_result[2] & ((1L << 32) - 1));
		}
		processor_frequency =
			sal.sal_result[0] * (pal.pal_result[0] >> 32)
			/ (pal.pal_result[0] & ((1L << 32) - 1));
		bus_frequency =
			sal.sal_result[0] * (pal.pal_result[1] >> 32)
			/ (pal.pal_result[1] & ((1L << 32) - 1));
		itc_frequency =
			sal.sal_result[0] * (pal.pal_result[2] >> 32)
			/ (pal.pal_result[2] & ((1L << 32) - 1));
	}

}


/* XXXX: Don't allocate 'ci' on stack. */
register struct cpu_info *ci __asm__("r13");
void
ia64_init(void)
{
	paddr_t kernstartpfn, kernendpfn, pfn0, pfn1;
	struct efi_md *md;

	/* NO OUTPUT ALLOWED UNTIL FURTHER NOTICE */

	/*
	 * TODO: Disable interrupts, floating point etc.
	 * Maybe flush cache and tlb
	 */

	ia64_set_fpsr(IA64_FPSR_DEFAULT);


	/*
	 * TODO: Get critical system information (if possible, from the
	 * information provided by the boot program).
	 */

	/*
	 * pa_bootinfo is the physical address of the bootinfo block as
	 * passed to us by the loader and set in locore.s.
	 */
	bootinfo = *(struct bootinfo *)(IA64_PHYS_TO_RR7(pa_bootinfo));

	if (bootinfo.bi_magic != BOOTINFO_MAGIC || bootinfo.bi_version != 1) {
		memset(&bootinfo, 0, sizeof(bootinfo));
		bootinfo.bi_kernend = (vaddr_t) round_page((vaddr_t)&end);
	}


	/*
	 * Look for the I/O ports first - we need them for console
	 * probing.
	 */
	for (md = efi_md_first(); md != NULL; md = efi_md_next(md)) {
		switch (md->md_type) {
		case EFI_MD_TYPE_IOPORT:
			ia64_port_base = IA64_PHYS_TO_RR6(md->md_phys);
			break;
		case EFI_MD_TYPE_PALCODE:
			ia64_pal_base = md->md_phys;
			break;
		}
	}


	/* XXX: We need to figure out whether/how much of the FreeBSD
	 *      getenv/setenv stuff we need. The info we get from ski
	 *      is too trivial to go to the hassle of importing the
	 *	FreeBSD environment stuff.
	 */

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = bootinfo.bi_boothowto;

	/* XXX: Debug: Override to verbose */
	boothowto |= AB_VERBOSE;


	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

	/* OUTPUT NOW ALLOWED */

	if (ia64_pal_base != 0) {
		ia64_pal_base &= ~IA64_ID_PAGE_MASK;
		/*
		 * We use a TR to map the first 256M of memory - this might
		 * cover the palcode too.
		 */
		if (ia64_pal_base == 0)
			printf("PAL code mapped by the kernel's TR\n");
	} else
		printf("PAL code not found\n");

	/*
	 * Wire things up so we can call the firmware.
	 */
	map_pal_code();
	efi_boot_minimal(bootinfo.bi_systab);
	ia64_sal_init();
	calculate_frequencies();

	/*
	 * Find the beginning and end of the kernel.
	 */

	kernstart = trunc_page((vaddr_t) &kernel_text);
#ifdef DDB
	ia64_unwindtab = (uint64_t)bootinfo.bi_unwindtab;
	ia64_unwindtablen = (uint64_t)bootinfo.bi_unwindtablen;
	ksym_start = (void *)bootinfo.bi_symtab;
	ksym_end   = (void *)bootinfo.bi_esymtab;
	kernend = (vaddr_t)round_page((vaddr_t)bootinfo.bi_kernend);
#else
	kernend = (vaddr_t)round_page(bootinfo.bi_kernend);
#endif
	kernstartpfn = atop(IA64_RR_MASK(kernstart));
	kernendpfn = atop(IA64_RR_MASK(kernend));

	/*
	 * Find out this system's page size, and initialize
	 * PAGE_SIZE-dependent variables.
	 */

	uvmexp.pagesize = PAGE_SIZE;
	uvm_setpagesize();


	/*
	 * Find out how much memory is available, by looking at
	 * the memory descriptors.
	 */

	physmem = 0;

	for (md = efi_md_first(); md != NULL; md = efi_md_next(md)) {

#ifdef DEBUG
		printf("MD %p: type %d pa 0x%lx cnt 0x%lx\n", md,
		    md->md_type, md->md_phys, md->md_pages);
#endif

		pfn0 = ia64_btop(round_page(md->md_phys));
		pfn1 = ia64_btop(trunc_page(md->md_phys + md->md_pages * 4096));
		if (pfn1 <= pfn0)
			continue;

		if (md->md_type != EFI_MD_TYPE_FREE)
			continue;

		/*
		 * Wimp out for now since we do not DTRT here with
		 * pci bus mastering (no bounce buffering, for example).
		 */
		if (pfn0 >= ia64_btop(0x100000000UL)) {
			printf("Skipping memory chunk start 0x%lx\n",
			    md->md_phys);
			continue;
		}
		if (pfn1 >= ia64_btop(0x100000000UL)) {
			printf("Skipping memory chunk end 0x%lx\n",
			    md->md_phys + md->md_pages * 4096);
			continue;
		}

		/*
		 * We have a memory descriptor that describes conventional
		 * memory that is for general use. We must determine if the
		 * loader has put the kernel in this region.
		 */
		physmem += (pfn1 - pfn0);
		if (pfn0 <= kernendpfn && kernstartpfn <= pfn1) {
			/*
			 * Must compute the location of the kernel
			 * within the segment.
			 */
#ifdef DEBUG
			printf("Descriptor %p contains kernel\n", md);
#endif
			if (pfn0 < kernstartpfn) {
				/*
				 * There is a chunk before the kernel.
				 */
#ifdef DEBUG
				printf("Loading chunk before kernel: "
				       "0x%lx / 0x%lx\n", pfn0, kernstartpfn);
#endif
				uvm_page_physload(pfn0, kernstartpfn,
				    pfn0, kernstartpfn, VM_FREELIST_DEFAULT);

			}
			if (kernendpfn < pfn1) {
				/*
				 * There is a chunk after the kernel.
				 */
#ifdef DEBUG
				printf("Loading chunk after kernel: "
				       "0x%lx / 0x%lx\n", kernendpfn, pfn1);
#endif

				uvm_page_physload(kernendpfn, pfn1,
				    kernendpfn, pfn1, VM_FREELIST_DEFAULT);

			}
		} else {
			/*
			 * Just load this cluster as one chunk.
			 */
#ifdef DEBUG
			printf("Loading descriptor %p: 0x%lx / 0x%lx\n",
			    md, pfn0, pfn1);
#endif

			uvm_page_physload(pfn0, pfn1, pfn0, pfn1,
			    VM_FREELIST_DEFAULT);

		}
	}

	if (physmem == 0)
		panic("can't happen: system seems to have no memory!");

	/*
	 * Initialize the virtual memory system.
	 */

	pmap_bootstrap();

	/*
	 * Initialize error message buffer (at end of core).
	 */
	msgbufaddr = (void *) uvm_pageboot_alloc(MSGBUFSIZE);
	initmsgbuf(msgbufaddr, MSGBUFSIZE);

	/*
	 * Init mapping for u page(s) for proc 0
	 */
	lwp0.l_addr = proc0paddr =
	    (struct user *)uvm_pageboot_alloc(UPAGES * PAGE_SIZE);


	/*
	 * Set the kernel sp, reserving space for an (empty) trapframe,
	 * and make proc0's trapframe pointer point to it for sanity.
	 */

	/*
	 * Process u-area is organised as follows:
	 *
	 *  -----------------------------------------------------------
	 * |  P  |                  |                    | 16Bytes | T |
	 * |  C  | Register Stack   |      Memory Stack  | <-----> | F |
	 * |  B  | ------------->   |       <----------  |         |   |
	 *  -----------------------------------------------------------
	 *        ^                                      ^
	 *        |___ bspstore                          |___ sp
	 *
	 *                 --------------------------->
         *                       Higher Addresses
	 *
	 *	PCB: struct user;    TF: struct trapframe;
	 */


	lwp0.l_md.md_tf = (struct trapframe *)((uint64_t)proc0paddr +
					USPACE - sizeof(struct trapframe));

	proc0paddr->u_pcb.pcb_special.sp =
	    (uint64_t)lwp0.l_md.md_tf - 16;	/* 16 bytes is the
						 * scratch area defined
						 * by the ia64 ABI
						 */

	proc0paddr->u_pcb.pcb_special.bspstore =
	    (uint64_t) proc0paddr + sizeof(struct user);

	mutex_init(&proc0paddr->u_pcb.pcb_fpcpu_slock, MUTEX_DEFAULT, 0);


	/*
	 * Setup global data for the bootstrap cpu.
	 */


	ci = curcpu();

	/* ar.k4 contains the cpu_info pointer to the
	 * current cpu.
	 */
	ia64_set_k4((uint64_t) ci);
	ci->ci_cpuid = cpu_number();


	/*
	 * Initialise process context. XXX: This should really be in cpu_switch
	 */
	ci->ci_curlwp = &lwp0;

	/*
	 * Initialize the primary CPU's idle PCB to proc0's.  In a
	 * MULTIPROCESSOR configuration, each CPU will later get
	 * its own idle PCB when autoconfiguration runs.
	 */
	ci->ci_idle_pcb = &proc0paddr->u_pcb;

	/* Indicate that proc0 has a CPU. */
	lwp0.l_cpu = ci;


	ia64_set_tpr(0);
	ia64_srlz_d();

	/*
	 * Save our current context so that we have a known (maybe even
	 * sane) context as the initial context for new threads that are
	 * forked from us.
	 */
	if (savectx(&lwp0.l_addr->u_pcb)) panic("savectx failed");

	/*
	 * Initialize debuggers, and break into them if appropriate.
	 */
#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)((uint64_t)ksym_end - (uint64_t)ksym_start),
	    ksym_start, ksym_end);
#endif

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	extern void main(void);
	main();

	panic("Wheeee!!! main() returned!!! \n");
}

uint64_t
ia64_get_hcdp(void)
{

	return bootinfo.bi_hcdp;
}

/*
 * Set registers on exec.
 */
void
setregs(register struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct trapframe *tf;
	uint64_t *ksttop, *kst, regstkp;

	tf = l->l_md.md_tf;
	regstkp = (uint64_t) (l->l_addr) + sizeof(struct user);

	ksttop =
	    (uint64_t*)(regstkp + tf->tf_special.ndirty +
					(tf->tf_special.bspstore & 0x1ffUL));

	/* XXX: tf_special.ndirty on a new stack frame ??? */

	/*
	 * We can ignore up to 8KB of dirty registers by masking off the
	 * lower 13 bits in exception_restore() or epc_syscall(). This
	 * should be enough for a couple of years, but if there are more
	 * than 8KB of dirty registers, we lose track of the bottom of
	 * the kernel stack. The solution is to copy the active part of
	 * the kernel stack down 1 page (or 2, but not more than that)
	 * so that we always have less than 8KB of dirty registers.
	 */
	KASSERT((tf->tf_special.ndirty & ~PAGE_MASK) == 0);

	memset(&tf->tf_special, 0, sizeof(tf->tf_special));
	if ((tf->tf_flags & FRAME_SYSCALL) == 0) {	/* break syscalls. */
		memset(&tf->tf_scratch, 0, sizeof(tf->tf_scratch));
		memset(&tf->tf_scratch_fp, 0, sizeof(tf->tf_scratch_fp));
		tf->tf_special.cfm = (1UL<<63) | (3UL<<7) | 3UL;
		tf->tf_special.bspstore = IA64_BACKINGSTORE;
		/*
		 * Copy the arguments onto the kernel register stack so that
		 * they get loaded by the loadrs instruction. Skip over the
		 * NaT collection points.
		 */
		kst = ksttop - 1;
		if (((uintptr_t)kst & 0x1ff) == 0x1f8)
			*kst-- = 0;
		*kst-- = (uint64_t)l->l_proc->p_psstr;	/* in3 = ps_strings */
		if (((uintptr_t)kst & 0x1ff) == 0x1f8)
			*kst-- = 0;
		*kst-- = 0;				/* in2 = *obj */
		if (((uintptr_t)kst & 0x1ff) == 0x1f8)
			*kst-- = 0;
		*kst-- = 0;				/* in1 = *cleanup */
		if (((uintptr_t)kst & 0x1ff) == 0x1f8)
			*kst-- = 0;
		*kst = stack; /* in0 = sp */
		tf->tf_special.ndirty = (ksttop - kst) << 3;
	} else {				/* epc syscalls (default). */
		tf->tf_special.cfm = (3UL<<62) | (3UL<<7) | 3UL;
		tf->tf_special.bspstore = IA64_BACKINGSTORE + 24;
		/*
		 * Write values for out0, out1, out2 and out3 to the user's
		 * backing store and arrange for them to be restored into
		 * the user's initial register frame.
		 * Assumes that (bspstore & 0x1f8) < 0x1e0.
		 */

		/* in0 = sp */
		suword((char *)tf->tf_special.bspstore - 32, stack);

		/* in1 == *cleanup */
		suword((char *)tf->tf_special.bspstore -  24, 0);

		/* in2 == *obj */
		suword((char *)tf->tf_special.bspstore -  16, 0);

		/* in3 = ps_strings */
		suword((char *)tf->tf_special.bspstore - 8,
		    (uint64_t)l->l_proc->p_psstr);

	}

	tf->tf_special.iip = pack->ep_entry;
	tf->tf_special.sp = (stack & ~15) - 16;
	tf->tf_special.rsc = 0xf;
	tf->tf_special.fpsr = IA64_FPSR_DEFAULT;
	tf->tf_special.psr = IA64_PSR_IC | IA64_PSR_I | IA64_PSR_IT |
	    IA64_PSR_DT | IA64_PSR_RT | IA64_PSR_DFH | IA64_PSR_BN |
	    IA64_PSR_CPL_USER;
	return;
}

void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	return;
}

void
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted, void *sas, void *ap, void *sp, sa_upcall_t upcall)
{
	return;
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	return;
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	return EINVAL;
}
