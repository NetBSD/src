/*	machdep.c,v 1.1.2.34 2011/04/29 08:26:18 matt Exp	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "machdep.c,v 1.1.2.34 2011/04/29 08:26:18 matt Exp");

#define __INTR_PRIVATE
#define __MUTEX_PRIVATE
#define _MIPS_BUS_DMA_PRIVATE

#include "opt_multiprocessor.h"
#include "opt_ddb.h"
#include "opt_com.h"
#include "opt_execfmt.h"
#include "opt_memsize.h"
#include "rmixl_pcix.h"
#include "rmixl_pcie.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/termios.h>
#include <sys/ksyms.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(LKM)
#include <mips/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <mips/cpu.h>
#include <mips/psl.h>
#include <mips/cache.h>
#include <mips/mipsNN.h>
#include <mips/mips_opcode.h>
#include <mips/pte.h>

#include "com.h"
#if NCOM == 0
#error no serial console
#endif

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_firmware.h>
#include <mips/rmi/rmixl_comvar.h>
#include <mips/rmi/rmixl_pcievar.h>
#include <mips/rmi/rmixl_pcixvar.h>

//#define MACHDEP_DEBUG 1
#ifdef MACHDEP_DEBUG
int machdep_debug=MACHDEP_DEBUG;
# define DPRINTF(x,...)	do { if (machdep_debug) printf(x, ## __VA_ARGS__); } while(0)
#else
# define DPRINTF(x,...)
#endif

extern	int comcnspeed;

void	mach_init(int, int32_t *, void *, int64_t);

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(int argc, int32_t *argv, void *envp, int64_t infop)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	vaddr_t kernend;
	uint64_t memsize;
	extern char edata[], end[];
	bool uboot_p = false;

	const uint32_t cfg0 = mips3_cp0_config_read();
#if (MIPS64_XLR + MIPS64_XLS) > 0 && (MIPS64_XLP) == 0
	const bool is_xlp_p = false	/* make sure cfg0 is used */
	    && MIPSNN_GET(CFG_AR, cfg0) == MIPSNN_CFG_AR_REV2;
	KASSERT(MIPSNN_GET(CFG_AR, cfg0) == MIPSNN_CFG_AR_REV1);
#elif (MIPS64_XLR + MIPS64_XLS) == 0 && (MIPS64_XLP) > 0
	const bool is_xlp_p = true	/* make sure cfg0 is used */
	    || MIPSNN_GET(CFG_AR, cfg0) == MIPSNN_CFG_AR_REV2;
	KASSERT(MIPSNN_GET(CFG_AR, cfg0) == MIPSNN_CFG_AR_REV2);
#else
	const bool is_xlp_p = (MIPSNN_GET(CFG_AR, cfg0) == MIPSNN_CFG_AR_REV2);
#endif

	rmixl_pcr_init_core(is_xlp_p);

#ifdef MULTIPROCESSOR
	__asm __volatile("dmtc0 %0,$%1,2"
	    ::	"r"(&pmap_tlb0_info.ti_hwlock->mtx_lock),
		"n"(MIPS_COP_0_OSSCRATCH));
#endif

	/*
	 * Clear the BSS segment.
	 */
	kernend = mips_round_page(end);
	memset(edata, 0, (char *)kernend - edata);

	/*
	 * Setup a early console for output by mips_vector_init
	 */
	rmixl_init_early_cons(rcp, is_xlp_p);

	/*
	 * Set up the exception vectors and CPU-specific function
	 * vectors early on.  We need the wbflush() vector set up
	 * before comcnattach() is called (or at least before the
	 * first printf() after that is called).
	 * Also clears the I+D caches.
	 *
	 * specify chip-specific EIRR/EIMR based spl functions
	 */
#ifdef MULTIPROCESSOR
	mips_vector_init(&rmixl_splsw, true);
#else
	mips_vector_init(&rmixl_splsw, false);
#endif

	if (argc < 0) {
		void *bd = (void *)(intptr_t)argc;
		void *imgaddr = argv;
		void *consdev = envp;
		char *bootargs = (void *)(intptr_t)infop;
		printf("%s: u-boot: boardinfo=%p, image-addr=%p, consdev=%p, bootargs=%p <%s>\n",
		    __func__, bd, imgaddr, consdev, bootargs, bootargs);
		uboot_p = true;
		printf("%s: u-boot: console baudrate=%d\n", __func__,
		    *(int *)bd);
		if (*(int *)bd % 1200 == 0)
			comcnspeed = *(int *)bd;
	} else {
		DPRINTF("%s: argc=%d, argv=%p, envp=%p, info=%#"PRIx64"\n",
		    __func__, argc, argv, envp, infop);
	}

	/* mips_vector_init initialized mips_options */
	strcpy(cpu_model, mips_options.mips_cpu->cpu_name);

#if (MIPS64_XLP) > 0
	if (is_xlp_p) {
		rmixl_mach_xlp_init(rcp);
	}
#endif /* MIPS64_XLP */

	/* determine DRAM first */
	memsize = rmixl_physaddr_init();
	DPRINTF("%s: physaddr init done (memsize=%"PRIu64"MB)!\n",
	    __func__, memsize >> 20);

	if (!uboot_p) {
		/* get system info from firmware */
		memsize = rmixlfw_init(infop);
		DPRINTF("%s: firmware init done (memsize=%"PRIu64"MB)!\n",
		    __func__, memsize >> 20);
	} else {
		rcp->rc_psb_info.userapp_cpu_map = 1;
	}

#if 0
	/* set the VM page size */
	uvm_setpagesize();

	physmem = btoc(memsize);
#endif

#if (MIPS64_XLR + MIPS64_XLS) > 0
	if (!is_xlp_p) {
		rmixl_mach_xlsxlr_init(rcp);
	}
#endif /* (MIPS64_XLR + MIPS64_XLS) > 0 */

#if defined(MULTIPROCESSOR) && defined(MACHDEP_DEBUG)
	if (!uboot_p) {
		rmixl_wakeup_info_print(rcp->rc_cpu_wakeup_info);
		rmixl_wakeup_info_print(rcp->rc_cpu_wakeup_info + 1);
		printf("cpu_wakeup_info %p, cpu_wakeup_end %p\n",
			rcp->rc_cpu_wakeup_info,
			rcp->rc_cpu_wakeup_end);
		printf("userapp_cpu_map: %#"PRIx64"\n",
			rcp->rc_psb_info.userapp_cpu_map);
		printf("wakeup: %#"PRIx64"\n", rcp->rc_psb_info.wakeup);
	}
#endif

	/*
	 * Obtain the cpu frequency
	 */
	rmixl_mach_freq_init(rcp, uboot_p, is_xlp_p);

	/*
	 * Look at arguments passed to us and compute boothowto.
	 * - rmixl firmware gives us a 32 bit argv[i], so adapt
	 *   by forcing sign extension in cast to (char *)
	 */
	boothowto = RB_AUTOBOOT;
	// boothowto |= AB_VERBOSE;
	// boothowto |= AB_DEBUG;
	if (!uboot_p) {
		rmixl_mach_init_parse_args(argc, (void *)(intptr_t)argv);
	}
#ifdef DIAGNOSTIC
	printf("boothowto %#x\n", boothowto);
#endif

	/*
	 * Now do the common rmixl dependent initialization.
	 */
	rmixl_mach_init_common(rcp, kernend, memsize, uboot_p, is_xlp_p);
}
