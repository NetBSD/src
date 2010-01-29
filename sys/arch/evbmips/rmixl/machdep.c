/*	$NetBSD: machdep.c,v 1.1.2.18 2010/01/29 00:22:27 cliff Exp $	*/

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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
 */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machdep.c   8.3 (Berkeley) 1/12/94
 *	from: Utah Hdr: machdep.c 1.63 91/04/24
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machdep.c   8.3 (Berkeley) 1/12/94
 *	from: Utah Hdr: machdep.c 1.63 91/04/24
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.1.2.18 2010/01/29 00:22:27 cliff Exp $");

#include "opt_ddb.h"
#include "opt_com.h"
#include "opt_execfmt.h"
#include "opt_memsize.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/termios.h>
#include <sys/ksyms.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(LKM)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <machine/cpu.h>
#include <machine/psl.h>

#include "com.h"
#if NCOM == 0
#error no serial console
#endif

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <mips/rmi/rmixl_comvar.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_firmware.h>
#include <mips/rmi/rmixlreg.h>

#ifdef MACHDEP_DEBUG
int machdep_debug=MACHDEP_DEBUG;
# define DPRINTF(x)	do { if (machdep_debug) printf x ; } while(0)
#else
# define DPRINTF(x)
#endif

#ifndef CONSFREQ
# define CONSFREQ 66000000
#endif
#ifndef CONSPEED
# define CONSPEED 38400
#endif
#ifndef CONMODE
# define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8)
#endif
#ifndef CONSADDR
# define CONSADDR RMIXL_IO_DEV_UART_1
#endif

int		comcnfreq  = CONSFREQ;
int		comcnspeed = CONSPEED;
tcflag_t	comcnmode  = CONMODE;
bus_addr_t	comcnaddr  = (bus_addr_t)CONSADDR;

struct rmixl_config rmixl_configuration;


/*
 * array of tested firmware versions
 * if you find new ones and they work
 * please add them
 */
typedef struct rmiclfw_psb_id {
	uint64_t		psb_version;
	rmixlfw_psb_type_t	psb_type;
} rmiclfw_psb_id_t;
static rmiclfw_psb_id_t rmiclfw_psb_id[] = {
	{	0x4958d4fb00000056ULL, PSB_TYPE_RMI  },
	{	0x49a5a8fa00000056ULL, PSB_TYPE_DELL },
	{	0x4aacdb6a00000056ULL, PSB_TYPE_RMI  },
};
#define RMICLFW_PSB_VERSIONS_LEN \
	(sizeof(rmiclfw_psb_id)/sizeof(rmiclfw_psb_id[0]))

/*
 * storage for fixed extent used to allocate physical address regions
 * because extent(9) start and end values are u_long, they are only
 * 32 bits on a 32 bit kernel, which is insuffucuent since XLS physical
 * address is 40 bits wide.  So the "physaddr" map stores regions
 * in units of megabytes.
 */
static u_long rmixl_physaddr_storage[
	EXTENT_FIXED_STORAGE_SIZE(32)/sizeof(u_long)
];

/* For sysctl_hw. */
extern char cpu_model[];

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

/* Maps for VM objects. */
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int	physmem;		/* Total physical memory */

int	netboot;		/* Are we netbooting? */


phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
u_int mem_cluster_cnt;


void configure(void);
void mach_init(int, int32_t *, void *, int64_t);
static uint64_t rmixlfw_init(int64_t);
static uint64_t mem_clusters_init(rmixlfw_mmap_t *, rmixlfw_mmap_t *);
static void __attribute__((__noreturn__)) rmixl_reset(void);
static void rmixl_physaddr_init(void);
static u_int ram_seg_resv(phys_ram_seg_t *, u_int, u_quad_t, u_quad_t);
void rmixlfw_mmap_print(rmixlfw_mmap_t *);


#ifdef MULTIPROCESSOR
void rmixl_get_wakeup_info(struct rmixl_config *);
#ifdef MACHDEP_DEBUG
static void rmixl_wakeup_info_print(volatile rmixlfw_cpu_wakeup_info_t *);
#endif
#endif


/*
 * safepri is a safe priority for sleep to set for a spin-wait during
 * autoconfiguration or after a panic.  Used as an argument to splx().
 */
int	safepri = MIPS1_PSL_LOWIPL;

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(int argc, int32_t *argv, void *envp, int64_t infop)
{
	struct rmixl_config *rcp = &rmixl_configuration;
	void *kernend;
	uint64_t memsize;
	u_int vm_cluster_cnt;
	uint32_t r;
	phys_ram_seg_t vm_clusters[VM_PHYSSEG_MAX];
	extern char edata[], end[];

#ifndef MULTIPROCESSOR
	rmixl_mtcr(0, 1);		/* disable all threads except #0 */
	rmixl_mtcr(0x400, 0);		/* enable MMU clock gating */
					/* set single MMU Thread Mode */
					/* TLB is partitioned (1 partition) */
#endif

	r = rmixl_mfcr(0x300);
	r &= ~__BIT(14);		/* disabled Unaligned Access */
	rmixl_mtcr(0x300, r);

	/*
	 * Clear the BSS segment.
	 */
	kernend = (void *)mips_round_page(end);
	memset(edata, 0, (char *)kernend - edata);

	/*
	 * Set up the exception vectors and CPU-specific function
	 * vectors early on.  We need the wbflush() vector set up
	 * before comcnattach() is called (or at least before the
	 * first printf() after that is called).
	 * Also clears the I+D caches.
	 */
	mips_vector_init();

	/* mips_vector_init initialized mips_options */
	strcpy(cpu_model, mips_options.mips_cpu->cpu_name);

	/* get system info from firmware */
	memsize = rmixlfw_init(infop);

	/* set the VM page size */
	uvm_setpagesize();

	physmem = btoc(memsize);

	rmixl_obio_eb_bus_mem_init(&rcp->rc_obio_eb_memt, rcp);

#if NCOM > 0
	rmixl_com_cnattach(comcnaddr, comcnspeed, comcnfreq,
		COM_TYPE_NORMAL, comcnmode);
#endif

	printf("\nNetBSD/rmixl\n");
	printf("memsize = %#"PRIx64"\n", memsize);

#if defined(MULTIPROCESSOR) && defined(MACHDEP_DEBUG)
	rmixl_wakeup_info_print(rcp->rc_cpu_wakeup_info);
	rmixl_wakeup_info_print(rcp->rc_cpu_wakeup_info + 1);
	printf("cpu_wakeup_info %p, cpu_wakeup_end %p\n",
		rcp->rc_cpu_wakeup_info,
		rcp->rc_cpu_wakeup_end);
	printf("userapp_cpu_map: %#"PRIx64"\n",
		rcp->rc_psb_info.userapp_cpu_map);
	printf("wakeup: %#"PRIx64"\n", rcp->rc_psb_info.wakeup);
{
	register_t sp;
	asm volatile ("move	%0, $sp\n" : "=r"(sp));
	printf("sp: %#"PRIx64"\n", sp);
}
#endif

	rmixl_physaddr_init();

	/*
	 * Obtain the cpu frequency
	 * Compute the number of ticks for hz.
	 * Compute the delay divisor.
	 * Double the Hz if this CPU runs at twice the
         *  external/cp0-count frequency
	 */
	curcpu()->ci_cpu_freq = rcp->rc_psb_info.cpu_frequency;
	curcpu()->ci_cctr_freq = curcpu()->ci_cpu_freq;
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;
	curcpu()->ci_divisor_delay =
		((curcpu()->ci_cpu_freq + 500000) / 1000000);
        if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		curcpu()->ci_cpu_freq *= 2;

	/*
	 * Look at arguments passed to us and compute boothowto.
	 * - rmixl firmware gives us a 32 bit argv[i], so adapt
	 *   by forcing sign extension in cast to (char *)
	 */
	boothowto = RB_AUTOBOOT;
	for (int i = 1; i < argc; i++) {
		for (char *cp = (char *)(intptr_t)argv[i]; *cp; cp++) {
			int howto;
			/* Ignore superfluous '-', if there is one */
			if (*cp == '-')
				continue;

			howto = 0;
			BOOT_FLAG(*cp, howto);
			if (howto != 0)
				boothowto |= howto;
#ifdef DIAGNOSTIC
			else
				printf("bootflag '%c' not recognised\n", *cp);
#endif
		}
	}
#ifdef DIAGNOSTIC
	printf("boothowto %#x\n", boothowto);
#endif

	/*
	 * Reserve pages from the VM system.
	 * to maintain mem_clusters[] as a map of raw ram,
	 * copy into temporary table vm_clusters[]
	 * work on that and use it to feed vm_physload()
	 */
	KASSERT(sizeof(mem_clusters) == sizeof(vm_clusters));
	memcpy(&vm_clusters, &mem_clusters, sizeof(vm_clusters));
	vm_cluster_cnt = mem_cluster_cnt;

	/* reserve 0..start..kernend pages */
	vm_cluster_cnt = ram_seg_resv(vm_clusters, vm_cluster_cnt,
		0, round_page(MIPS_KSEG0_TO_PHYS(kernend)));

	/* reserve reset exception vector page */
	/* should never be in our clusters anyway... */
	vm_cluster_cnt = ram_seg_resv(vm_clusters, vm_cluster_cnt,
		0x1FC00000, 0x1FC00000+NBPG);

#ifdef MULTIPROCEESOR
	/* reserve the cpu_wakeup_info area */
	vm_cluster_cnt = ram_seg_resv(vm_clusters, vm_cluster_cnt,
		(u_quad_t)trunc_page(rcp->rc_cpu_wakeup_info),
		(u_quad_t)round_page(rcp->rc_cpu_wakeup_end));
#endif

#ifdef MEMLIMIT
	/* reserve everything > MEMLIMIT */
	vm_cluster_cnt = ram_seg_resv(vm_clusters, vm_cluster_cnt,
		(u_quad_t)MEMLIMIT, (u_quad_t)~0);
#endif

	/*
	 * Load vm_clusters[] into the VM system.
	 */
	mips_page_physload(MIPS_KSEG0_START, (vaddr_t) kernend,
	    vm_clusters, vm_cluster_cnt, NULL, 0);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	pmap_bootstrap();

	/*
	 * Allocate space for proc0's USPACE.
	 */
	mips_init_lwp0_uarea();

	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
#if NKSYMS || defined(DDB) || defined(LKM)
	ksyms_init(0, 0, 0);
#endif

#if defined(DDB)
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * ram_seg_resv - cut reserved regions out of segs, fragmenting as needed
 *
 * we simply build a new table of segs, then copy it back over the given one
 * this is inefficient but simple and called only a few times
 *
 * note: 'last' here means 1st addr past the end of the segment (start+size)
 */
static u_int
ram_seg_resv(phys_ram_seg_t *segs, u_int nsegs,
	u_quad_t resv_first, u_quad_t resv_last)
{
        u_quad_t first, last;
	int new_nsegs=0;
	int resv_flag;
	phys_ram_seg_t new_segs[VM_PHYSSEG_MAX];

	for (u_int i=0; i < nsegs; i++) {
		resv_flag = 0;
		first = trunc_page(segs[i].start);
		last = round_page(segs[i].start + segs[i].size);

		KASSERT(new_nsegs < VM_PHYSSEG_MAX);
		if ((resv_first <= first) && (resv_last >= last)) {
			/* whole segment is resverved */
			continue;
		}
		if ((resv_first > first) && (resv_first < last)) {
			u_quad_t new_last;

			/*
			 * reserved start in segment
			 * salvage the leading fragment
			 */
			resv_flag = 1;
			new_last = last - (last - resv_first);
			KASSERT (new_last > first);
			new_segs[new_nsegs].start = first;
			new_segs[new_nsegs].size = new_last - first;
			new_nsegs++;
		}
		if ((resv_last > first) && (resv_last < last)) {
			u_quad_t new_first;

			/*
			 * reserved end in segment
			 * salvage the trailing fragment
			 */
			resv_flag = 1;
			new_first = first + (resv_last - first);
			KASSERT (last > (new_first + NBPG));
			new_segs[new_nsegs].start = new_first;
			new_segs[new_nsegs].size = last - new_first;
			new_nsegs++;
		}
		if (resv_flag == 0) {
			/*
			 * nothing reserved here, take it all
			 */
			new_segs[new_nsegs].start = first;
			new_segs[new_nsegs].size = last - first;
			new_nsegs++;
		}

	}

	memcpy(segs, new_segs, sizeof(new_segs));

	return new_nsegs;
}

/*
 * create an extent for physical address space
 * these are in units of MB for sake of compression (for sake of 32 bit kernels)
 * allocate the regions where we have known functions (DRAM, IO, etc)
 * what remains can be allocated as needed for other stuff
 * e.g. to configure BARs that are not already initialized and enabled.
 */
static void
rmixl_physaddr_init(void)
{
	struct extent *ext;
	unsigned long start = 0UL;
	unsigned long end = (__BIT(40) / (1024 * 1024)) -1;
	u_long base;
	u_long size;
	uint32_t r;

	ext = extent_create("physaddr", start, end, M_DEVBUF,
		(void *)rmixl_physaddr_storage, sizeof(rmixl_physaddr_storage),
		EX_NOWAIT | EX_NOCOALESCE);

	if (ext == NULL)
		panic("%s: extent_create failed", __func__);

	/*
	 * grab regions per DRAM BARs
	 */
	for (u_int i=0; i < RMIXL_SBC_DRAM_NBARS; i++) { 
		r = RMIXL_IOREG_READ(RMIXL_SBC_DRAM_BAR(i));
		if ((r & RMIXL_DRAM_BAR_STATUS) == 0)
			continue;	/* not enabled */
		base = (u_long)(DRAM_BAR_TO_BASE((uint64_t)r) / (1024 * 1024));
		size = (u_long)(DRAM_BAR_TO_SIZE((uint64_t)r) / (1024 * 1024));

		DPRINTF(("%s: %d: %d: 0x%08x -- 0x%010lx:%lu MB\n",
			__func__, __LINE__, i, r, base * (1024 * 1024), size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}

	/*
	 * grab regions per PCIe CFG, ECFG, IO, MEM BARs
	 */
	r = RMIXL_IOREG_READ(RMIXL_SBC_PCIE_CFG_BAR);
	if ((r & RMIXL_PCIE_CFG_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIE_CFG_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)RMIXL_PCIE_CFG_SIZE / (1024 * 1024);
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "CFG", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}
	r = RMIXL_IOREG_READ(RMIXL_SBC_PCIE_ECFG_BAR);
	if ((r & RMIXL_PCIE_ECFG_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIE_ECFG_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)RMIXL_PCIE_ECFG_SIZE / (1024 * 1024);
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "ECFG", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}
	r = RMIXL_IOREG_READ(RMIXL_SBC_PCIE_MEM_BAR);
	if ((r & RMIXL_PCIE_MEM_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIE_MEM_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)(RMIXL_PCIE_MEM_BAR_TO_SIZE((uint64_t)r)
			/ (1024 * 1024));
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "MEM", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}
	r = RMIXL_IOREG_READ(RMIXL_SBC_PCIE_IO_BAR);
	if ((r & RMIXL_PCIE_IO_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIE_IO_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)(RMIXL_PCIE_IO_BAR_TO_SIZE((uint64_t)r)
			/ (1024 * 1024));
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "IO", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}

	/*
	 *  at this point all regions left in "physaddr" extent
	 *  are unused holes in the physical adress space
	 *  available for use as needed.
	 */
	rmixl_configuration.rc_phys_ex = ext;
#ifdef MACHDEP_DEBUG
	extent_print(ext);
#endif
}

static uint64_t
rmixlfw_init(int64_t infop)
{
	struct rmixl_config *rcp = &rmixl_configuration;

#ifdef MULTIPROCESSOR
	rmixl_get_wakeup_info(rcp);
#endif

	infop |= MIPS_KSEG0_START;
	rcp->rc_psb_info = *(rmixlfw_info_t *)(intptr_t)infop;

	rcp->rc_psb_type = PSB_TYPE_UNKNOWN;
	for (int i=0; i < RMICLFW_PSB_VERSIONS_LEN; i++) {
		if (rmiclfw_psb_id[i].psb_version ==
		    rcp->rc_psb_info.psb_version) {
			rcp->rc_psb_type = rmiclfw_psb_id[i].psb_type;
			goto found;
		}
	}

	rcp->rc_io_pbase = RMIXL_IO_DEV_PBASE;
	rmixl_putchar_init(rcp->rc_io_pbase);

#ifdef DIAGNOSTIC
	rmixl_puts("\r\nWARNING: untested psb_version: ");
	rmixl_puthex64(rcp->rc_psb_info.psb_version);
	rmixl_puts("\r\n");
#endif

#ifdef MEMSIZE
	/* XXX trust and use MEMSIZE */
	mem_clusters[0].start = 0;
	mem_clusters[0].size = MEMSIZE;
	mem_cluster_cnt = 1;
	return MEMSIZE;
#else
	rmixl_puts("\r\nERROR: configure MEMSIZE\r\n");
	cpu_reboot(RB_HALT, NULL);
	/* NOTREACHED */
#endif

 found:
	rcp->rc_io_pbase = MIPS_KSEG1_TO_PHYS(rcp->rc_psb_info.io_base);
	rmixl_putchar_init(rcp->rc_io_pbase);
#ifdef MACHDEP_DEBUG
	rmixl_puts("\r\ninfop: ");
	rmixl_puthex64((uint64_t)(intptr_t)infop);
#endif
#ifdef DIAGNOSTIC
	rmixl_puts("\r\nrecognized psb_version=");
	rmixl_puthex64(rcp->rc_psb_info.psb_version);
	rmixl_puts(", psb_type=");
	rmixl_puts(rmixlfw_psb_type_name(rcp->rc_psb_type));
	rmixl_puts("\r\n");
#endif

	return mem_clusters_init(
		(rmixlfw_mmap_t *)(intptr_t)rcp->rc_psb_info.psb_physaddr_map,
		(rmixlfw_mmap_t *)(intptr_t)rcp->rc_psb_info.avail_mem_map);
}

void
rmixlfw_mmap_print(rmixlfw_mmap_t *map)
{
#ifdef MACHDEP_DEBUG
	for (uint32_t i=0; i < map->nmmaps; i++) {
		rmixl_puthex32(i);
		rmixl_puts(", ");
		rmixl_puthex64(map->entry[i].start);
		rmixl_puts(", ");
		rmixl_puthex64(map->entry[i].size);
		rmixl_puts(", ");
		rmixl_puthex32(map->entry[i].type);
		rmixl_puts("\r\n");
	}
#endif
}

/*
 * mem_clusters_init
 *
 * initialize mem_clusters[] table based on memory address mapping
 * provided by boot firmware.
 *
 * prefer avail_mem_map if we can, otherwise use psb_physaddr_map.
 * these will be limited by MEMSIZE if it is configured.
 * if neither are available, just use MEMSIZE.
 */
static uint64_t
mem_clusters_init(
	rmixlfw_mmap_t *psb_physaddr_map,
	rmixlfw_mmap_t *avail_mem_map)
{
	rmixlfw_mmap_t *map = NULL;
	const char *mapname;
	uint64_t sz;
	uint64_t sum;
	u_int cnt;
#ifdef MEMSIZE
	uint64_t memsize = MEMSIZE;
#endif

#ifdef MACHDEP_DEBUG
	rmixl_puts("psb_physaddr_map: ");
	rmixl_puthex64((uint64_t)(intptr_t)psb_physaddr_map);
	rmixl_puts("\r\n");
#endif
	if (psb_physaddr_map != NULL) {
		map = psb_physaddr_map;
		mapname = "psb_physaddr_map";
		rmixlfw_mmap_print(map);
	}
#ifdef DIAGNOSTIC
	else {
		rmixl_puts("WARNING: no psb_physaddr_map\r\n");
	}
#endif

#ifdef MACHDEP_DEBUG
	rmixl_puts("avail_mem_map: ");
	rmixl_puthex64((uint64_t)(intptr_t)avail_mem_map);
	rmixl_puts("\r\n");
#endif
	if (avail_mem_map != NULL) {
		map = avail_mem_map;
		mapname = "avail_mem_map";
		rmixlfw_mmap_print(map);
	}
#ifdef DIAGNOSTIC
	else {
		rmixl_puts("WARNING: no avail_mem_map\r\n");
	}
#endif

	if (map == NULL) {
#ifndef MEMSIZE
		rmixl_puts("panic: no firmware memory map, "
			"must configure MEMSIZE\r\n");
		for(;;);	/* XXX */
#else
#ifdef DIAGNOSTIC
		rmixl_puts("WARNING: no avail_mem_map, "
			"using MEMSIZE\r\n");
#endif

		mem_clusters[0].start = 0;
		mem_clusters[0].size = MEMSIZE;
		mem_cluster_cnt = 1;
		return MEMSIZE;
#endif	/* MEMSIZE */
	}

#ifdef DIAGNOSTIC
	rmixl_puts("using ");
	rmixl_puts(mapname);
	rmixl_puts("\r\n");
#endif
#ifdef MACHDEP_DEBUG
	rmixl_puts("memory clusters:\r\n");
#endif
	sum = 0;
	cnt = 0;
	for (uint32_t i=0; i < map->nmmaps; i++) {
		if (map->entry[i].type != RMIXLFW_MMAP_TYPE_RAM)
			continue;
		mem_clusters[cnt].start = map->entry[i].start;
		sz = map->entry[i].size;
		sum += sz;
		mem_clusters[cnt].size = sz;
#ifdef MACHDEP_DEBUG
		rmixl_puthex32(i);
		rmixl_puts(": ");
		rmixl_puthex64(mem_clusters[cnt].start);
		rmixl_puts(", ");
		rmixl_puthex64(sz);
		rmixl_puts(": ");
		rmixl_puthex64(sum);
		rmixl_puts("\r\n");
#endif
#ifdef MEMSIZE
		/*
		 * configurably limit memsize
		 */
		if (sum == memsize)
			break;
		if (sum > memsize) {
			uint64_t tmp;

			tmp = sum - memsize;
			sz -= tmp;
			sum -= tmp;
			mem_clusters[cnt].size = sz;
			cnt++;
			break;
		}
#endif
		cnt++;
	}
	mem_cluster_cnt = cnt;
	return sum;
}

#ifdef MULTIPROCESSOR
/*
 * firmware passes wakeup info structure in CP0 OS Scratch reg #7
 * they do not explicitly give us the size of the wakeup area.
 * we "know" that firmware loader sets wip->gp thusly:
 *   gp = stack_start[vcpu] = round_page(wakeup_end) + (vcpu * (PAGE_SIZE * 2))
 * so
 *   round_page(wakeup_end) == gp - (vcpu * (PAGE_SIZE * 2))
 * Only the "master" cpu runs this function, so
 *   vcpu = wip->master_cpu
 */
void
rmixl_get_wakeup_info(struct rmixl_config *rcp)
{
	volatile rmixlfw_cpu_wakeup_info_t *wip;
	int32_t scratch_7;
	intptr_t end;

	__asm__ volatile(
		".set push"				"\n"
		".set noreorder"			"\n"
		".set mips64"				"\n" 
		"dmfc0	%0, $22, 7"			"\n"
		".set pop"				"\n"
			: "=r"(scratch_7));

	wip = (volatile rmixlfw_cpu_wakeup_info_t *)
			(intptr_t)scratch_7;
	end = wip->entry.gp - (wip->master_cpu & (PAGE_SIZE * 2));;

	if (wip->valid == 1) {
		rcp->rc_cpu_wakeup_end = (const void *)end;
		rcp->rc_cpu_wakeup_info = wip;
	}
};

#ifdef MACHDEP_DEBUG
static void
rmixl_wakeup_info_print(volatile rmixlfw_cpu_wakeup_info_t *wip)
{
	int i;

	printf("%s: wip %p, size %lu\n", __func__, wip, sizeof(*wip));

	printf("cpu_status %#x\n",  wip->cpu_status);
	printf("valid: %d\n", wip->valid);
	printf("entry: addr %#x, args %#x, sp %#"PRIx64", gp %#"PRIx64"\n",
		wip->entry.addr,
		wip->entry.args,
		wip->entry.sp,
		wip->entry.gp);
	printf("master_cpu %d\n", wip->master_cpu);
	printf("master_cpu_mask %#x\n", wip->master_cpu_mask);
	printf("buddy_cpu_mask %#x\n", wip->buddy_cpu_mask);
	printf("psb_os_cpu_map %#x\n", wip->psb_os_cpu_map);
	printf("argc %d\n", wip->argc);
	printf("argv:");
	for (i=0; i < wip->argc; i++)
		printf(" %#x", wip->argv[i]);
	printf("\n");
	printf("valid_tlb_entries %d\n", wip->valid_tlb_entries);
	printf("tlb_map:\n");
	for (i=0; i < wip->valid_tlb_entries; i++) {
		volatile const struct lib_cpu_tlb_mapping *m =
			&wip->tlb_map[i];
		printf(" %d", m->page_size);
		printf(", %d", m->asid);
		printf(", %d", m->coherency);
		printf(", %d", m->coherency);
		printf(", %d", m->attr);
		printf(", %#x", m->virt);
		printf(", %#"PRIx64"\n", m->phys);
	}
	printf("elf segs:\n");
	for (i=0; i < MAX_ELF_SEGMENTS; i++) {
		volatile const struct core_segment_info *e =
			&wip->seg_info[i];
		printf(" %#"PRIx64"", e->vaddr);
		printf(", %#"PRIx64"", e->memsz);
		printf(", %#x\n", e->flags);
	}
	printf("envc %d\n", wip->envc);
	for (i=0; i < wip->envc; i++)
		printf(" %#x \"%s\"", wip->envs[i],
			(char *)(intptr_t)(int32_t)(wip->envs[i]));
	printf("\n");
	printf("app_mode %d\n", wip->app_mode);
	printf("printk_lock %#x\n", wip->printk_lock);
	printf("kseg_master %d\n", wip->kseg_master);
	printf("kuseg_reentry_function %#x\n", wip->kuseg_reentry_function);
	printf("kuseg_reentry_args %#x\n", wip->kuseg_reentry_args);
	printf("app_shared_mem_addr %#"PRIx64"\n", wip->app_shared_mem_addr);
	printf("app_shared_mem_size %#"PRIx64"\n", wip->app_shared_mem_size);
	printf("app_shared_mem_orig %#"PRIx64"\n", wip->app_shared_mem_orig);
	printf("loader_lock %#x\n", wip->loader_lock);
	printf("global_wakeup_mask %#x\n", wip->global_wakeup_mask);
	printf("unused_0 %#x\n", wip->unused_0);
}
#endif	/* MACHDEP_DEBUG */
#endif 	/* MULTIPROCESSOR */

void
consinit(void)
{

	/*
	 * Everything related to console initialization is done
	 * in mach_init().
	 */
}

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup()
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	format_bytes(pbuf, sizeof(pbuf), ctob((uint64_t)physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Virtual memory is bootstrapped -- notify the bus spaces
	 * that memory allocation is now safe.
	 */
	rmixl_configuration.rc_mallocsafe = 1;

	minaddr = 0;
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				    VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * (No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use XKSEG to
	 * map those pages.)
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

int	waittime = -1;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{

	/* Take a snapshot before clobbering any registers. */
	if (curproc)
		savectx((struct user *)curpcb);

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* If "always halt" was specified as a boot flag, obey. */
	if (boothowto & RB_HALT)
		howto |= RB_HALT;

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && (waittime < 0)) {
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	splhigh();

	if (howto & RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* For proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n\n");

	rmixl_reset();
}

/*
 * goodbye world
 */
void __attribute__((__noreturn__))
rmixl_reset(void)
{
	uint32_t r;

	r = RMIXL_IOREG_READ(RMIXL_IO_DEV_GPIO + RMIXL_GPIO_RESET);
	r |= RMIXL_GPIO_RESET_RESET;
	RMIXL_IOREG_WRITE(RMIXL_IO_DEV_GPIO + RMIXL_GPIO_RESET, r);

	printf("soft reset failed, spinning...\n");
	for (;;);
}
