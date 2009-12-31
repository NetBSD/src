/*	$NetBSD: machdep.c,v 1.1.2.11 2009/12/31 00:54:09 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.1.2.11 2009/12/31 00:54:09 matt Exp $");

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
static uint64_t rmiclfw_psb_versions[] = {
	0x4958d4fb00000056ULL,
	0x49a5a8fa00000056ULL,
	0x4aacdb6a00000056ULL,
};
#define RMICLFW_PSB_VERSIONS_LEN \
	(sizeof(rmiclfw_psb_versions)/sizeof(rmiclfw_psb_versions[0]))

/*
 * kernel copies of firmware info
 */
static rmixlfw_info_t rmixlfw_info;
static rmixlfw_mmap_t rmixlfw_phys_mmap;
static rmixlfw_mmap_t rmixlfw_avail_mmap;
#define RMIXLFW_INFOP_LEGAL	0x8c000000


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
static u_long rmixlfw_init(int64_t);
static u_long mem_clusters_init(rmixlfw_mmap_t *, rmixlfw_mmap_t *);
static void __attribute__((__noreturn__)) rmixl_exit(int);
static void rmixl_physaddr_init(void);
static u_int ram_seg_resv(phys_ram_seg_t *, u_int, u_quad_t, u_quad_t);
void rmixlfw_mmap_print(rmixlfw_mmap_t *);


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
	u_long memsize;
	u_int vm_cluster_cnt;
	uint32_t r;
	phys_ram_seg_t vm_clusters[VM_PHYSSEG_MAX];
	extern char edata[], end[];

	rmixl_mtcr(0, 1);		/* disable all threads except #0 */

	r = rmixl_mfcr(0x300);
	r &= ~__BIT(14);		/* disabled Unaligned Access */
	rmixl_mtcr(0x300, r);

	rmixl_mtcr(0x400, 0);		/* enable MMU clock gating */
					/* set single MMU Thread Mode */
					/* TLB is partitioned (1 partition) */

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
	printf("memsize = %#lx\n", memsize);

	rmixl_physaddr_init();

	/*
	 * Obtain the cpu frequency
	 * Compute the number of ticks for hz.
	 * Compute the delay divisor.
	 * Double the Hz if this CPU runs at twice the
         *  external/cp0-count frequency
	 */
	curcpu()->ci_cpu_freq = rmixlfw_info.cpu_frequency;
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;
	curcpu()->ci_divisor_delay =
		((curcpu()->ci_cpu_freq + 500000) / 1000000);
        if (mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
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
		MIPS_RESET_EXC_VEC, MIPS_RESET_EXC_VEC+NBPG);

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

static u_long
rmixlfw_init(int64_t infop)
{
	struct rmixl_config *rcp = &rmixl_configuration;

	strcpy(cpu_model, "RMI XLS616ATX VIIA");	/* XXX */

	infop |= MIPS_KSEG0_START;
	rmixlfw_info = *(rmixlfw_info_t *)(intptr_t)infop;

	for (int i=0; i < RMICLFW_PSB_VERSIONS_LEN; i++) {
		if (rmiclfw_psb_versions[i] == rmixlfw_info.psb_version)
			goto found;
	}

	rcp->rc_io_pbase = RMIXL_IO_DEV_PBASE;
	rmixl_putchar_init(rcp->rc_io_pbase);

#ifdef DIAGNOSTIC
	rmixl_puts("\r\nWARNING: untested psb_version: ");
	rmixl_puthex64(rmixlfw_info.psb_version);
	rmixl_puts("\r\n");
#endif

	/* XXX trust and use MEMSIZE */
	mem_clusters[0].start = 0;
	mem_clusters[0].size = MEMSIZE;
	mem_cluster_cnt = 1;
	return MEMSIZE;

 found:
	rcp->rc_io_pbase = MIPS_KSEG1_TO_PHYS(rmixlfw_info.io_base);
	rmixl_putchar_init(rcp->rc_io_pbase);
#ifdef MACHDEP_DEBUG
	rmixl_puts("\r\ninfop: ");
	rmixl_puthex64((uint64_t)(intptr_t)infop);
#endif
#ifdef DIAGNOSTIC
	rmixl_puts("\r\nrecognized psb_version: ");
	rmixl_puthex64(rmixlfw_info.psb_version);
	rmixl_puts("\r\n");
#endif

	return mem_clusters_init(
		(rmixlfw_mmap_t *)(intptr_t)rmixlfw_info.psb_physaddr_map,
		(rmixlfw_mmap_t *)(intptr_t)rmixlfw_info.avail_mem_map);
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
static u_long
mem_clusters_init(
	rmixlfw_mmap_t *psb_physaddr_map,
	rmixlfw_mmap_t *avail_mem_map)
{
	rmixlfw_mmap_t *map = NULL;
	const char *mapname;
	uint64_t tmp;
	uint64_t sz;
	uint64_t sum;
	u_int cnt;
#ifdef MEMSIZE
	u_long memsize = MEMSIZE;
#endif

#ifdef MACHDEP_DEBUG
	rmixl_puts("psb_physaddr_map: ");
	rmixl_puthex64((uint64_t)(intptr_t)psb_physaddr_map);
	rmixl_puts("\r\n");
#endif
	if (psb_physaddr_map != NULL) {
		rmixlfw_phys_mmap = *psb_physaddr_map;
		map = &rmixlfw_phys_mmap;
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
		rmixlfw_avail_mmap = *avail_mem_map;
		map = &rmixlfw_avail_mmap;
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
			tmp = sum - memsize;
			sz -= tmp;
			sum -= tmp;
			mem_clusters[cnt].size = sz;
			break;
		}
#endif
		cnt++;
	}
	mem_cluster_cnt = cnt;
	return sum;
}

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
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
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

	rmixl_exit(0);
}

/*
 * goodbye world
 */
#define GPIO_CPU_RST 0xa0			/* XXX TMP */
void __attribute__((__noreturn__))
rmixl_exit(int howto)
{
	/* use firmware callbak to reboot */
	void (*reset)(void) = (void *)(intptr_t)rmixlfw_info.warm_reset;
	if (reset != 0) {
		(*reset)();
		printf("warm reset callback failed, spinning...\n");
	} else {
		printf("warm reset callback absent, spinning...\n");
	}
	for (;;);
}
