/*	$NetBSD: machdep.c,v 1.23 2020/08/17 07:50:42 simonb Exp $	*/

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

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.23 2020/08/17 07:50:42 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/termios.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(LKM)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <machine/psl.h>
#include <machine/locore.h>

#include <mips/cavium/autoconf.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/include/bootbusvar.h>

#include <mips/cavium/dev/octeon_uartvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_gpioreg.h>

#include <evbmips/cavium/octeon_uboot.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_private.h>

static void	mach_init_vector(void);
static void	mach_init_bus_space(void);
static void	mach_init_console(void);
static void	mach_init_memory(void);
static void	parse_boot_args(void);

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
int	comcnrate = 115200;	/* XXX should be config option */
#endif /* NCOM > 0 */

/* Maps for VM objects. */
struct vm_map *phys_map = NULL;

int	netboot;

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;
extern char kernel_text[];
extern char edata[];
extern char end[];

void	mach_init(uint64_t, uint64_t, uint64_t, uint64_t);

struct octeon_config octeon_configuration;
struct octeon_btdesc octeon_btdesc;
struct octeon_btinfo octeon_btinfo;

char octeon_nmi_stack[PAGE_SIZE] __section(".data1") __aligned(PAGE_SIZE);

/* Currently the OCTEON kernels only support big endian boards */
CTASSERT(_BYTE_ORDER == _BIG_ENDIAN);

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	uint64_t btinfo_paddr;
	void *fdt_data;

	/* clear the BSS segment */
	memset(edata, 0, end - edata);

	cpu_reset_address = octeon_soft_reset;

#if 1 || defined(OCTEON_EARLY_CONSOLE)	/* XXX - remove "1 ||" when MP works */
	/*
	 * Set up very conservative timer params so we can use delay(9)
	 * early.  It doesn't matter if we delay too long at this stage.
	 */
	octeon_cal_timer(2000 * 1000 * 1000);
	octuart_early_cnattach(comcnrate);
#endif /* OCTEON_EARLY_CONSOLE */

	KASSERT(MIPS_XKPHYS_P(arg3));
	btinfo_paddr = mips3_ld(arg3 + OCTEON_BTINFO_PADDR_OFFSET);

	/* XXX KASSERT these addresses? */
	memcpy(&octeon_btdesc, (void *)arg3, sizeof(octeon_btdesc));
	if ((octeon_btdesc.obt_desc_ver == OCTEON_SUPPORTED_DESCRIPTOR_VERSION) &&
	    (octeon_btdesc.obt_desc_size == sizeof(octeon_btdesc))) {
		btinfo_paddr = MIPS_PHYS_TO_XKPHYS(CCA_CACHEABLE,
		    octeon_btdesc.obt_boot_info_addr);
	} else {
		panic("unknown boot descriptor size %u",
		    octeon_btdesc.obt_desc_size);
	}
	memcpy(&octeon_btinfo, (void *)btinfo_paddr, sizeof(octeon_btinfo));
	parse_boot_args();

	octeon_cal_timer(octeon_btinfo.obt_eclock_hz);

	cpu_setmodel("Cavium Octeon %s",
	    octeon_cpu_model(mips_options.mips_cpu_id));

	if (octeon_btinfo.obt_minor_version >= 3 &&
	    octeon_btinfo.obt_fdt_addr != 0) {
		fdt_data = (void *)MIPS_PHYS_TO_XKPHYS(CCA_CACHEABLE,
		    octeon_btinfo.obt_fdt_addr);
		fdtbus_init(fdt_data);
	}

	mach_init_vector();

	uvm_md_init();

	mach_init_bus_space();

	mach_init_console();

#ifdef DEBUG
	/* Show a couple of boot desc/info params for positive feedback */
	printf(">> boot desc eclock    = %d\n", octeon_btdesc.obt_eclock);
	printf(">> boot desc core mask = %#x\n", octeon_btinfo.obt_core_mask);
	printf(">> boot info board     = %d\n", octeon_btinfo.obt_board_type);
#endif /* DEBUG */

	mach_init_memory();

	/*
	 * Allocate uarea page for lwp0 and set it.
	 */
	mips_init_lwp0_uarea();

#if 0
	curcpu()->ci_nmi_stack = octeon_nmi_stack + sizeof(octeon_nmi_stack) - sizeof(struct kernframe);
	*(uint64_t *)MIPS_PHYS_TO_KSEG0(0x800) = (intptr_t)octeon_reset_vector;
	const uint64_t wdog_reg = MIPS_PHYS_TO_XKPHYS_UNCACHED(CIU_WDOG0);
	uint64_t wdog = mips3_ld(wdog_reg);
	wdog &= ~(CIU_WDOGX_MODE|CIU_WDOGX_LEN);
	wdog |= __SHIFTIN(3, CIU_WDOGX_MODE);
	wdog |= CIU_WDOGX_LEN;		// max period
	mips64_sd_a64(wdog_reg, wdog);
	printf("Watchdog enabled!\n");
#endif

#if defined(DDB)
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

void
consinit(void)
{

	/*
	 * Everything related to console initialization is done
	 * in mach_init().
	 */
}

void
mach_init_vector(void)
{

	/* Make sure exception base at 0 (MIPS_COP_0_EBASE) */
	__asm __volatile("mtc0 %0, $15, 1" : : "r"(0x80000000) );

	/*
	 * Set up the exception vectors and CPU-specific function
	 * vectors early on.  We need the wbflush() vector set up
	 * before comcnattach() is called (or at least before the
	 * first printf() after that is called).
	 * Also clears the I+D caches.
	 */
	mips_vector_init(NULL, true);
}

void
mach_init_bus_space(void)
{
	struct octeon_config *mcp = &octeon_configuration;

	octeon_dma_init(mcp);

	iobus_bootstrap(mcp);
	bootbus_bootstrap(mcp);
}

void
mach_init_console(void)
{
#if NCOM > 0
	struct octeon_config *mcp = &octeon_configuration;
	int status;
	extern int octuart_com_cnattach(bus_space_tag_t, int, int);

	/*
	 * Delay to allow firmware putchars to complete.
	 * FIFO depth * character time.
	 * character time = (1000000 / (defaultrate / 10))
	 */
	delay(640000000 / comcnrate);

	status = octuart_com_cnattach(
		&mcp->mc_iobus_bust,
		0,	/* XXX port 0 */
		comcnrate);
	if (status != 0)
		panic("can't initialize console!");	/* XXX print to nowhere! */
#else
	panic("octeon: not configured to use serial console");
#endif /* NCOM > 0 */
}

static void
mach_init_memory(void)
{
	struct octeon_bootmem_desc *memdesc;
	struct octeon_bootmem_block_header *block;
	paddr_t blockaddr;
	int i;

	mem_cluster_cnt = 0;

	if (octeon_btinfo.obt_phy_mem_desc_addr == 0)
		panic("bootmem desc is missing");

	memdesc = (void *)MIPS_PHYS_TO_XKPHYS(CCA_CACHEABLE,
                    octeon_btinfo.obt_phy_mem_desc_addr);
	printf("u-boot bootmem desc @ 0x%x version %d.%d\n",
	    octeon_btinfo.obt_phy_mem_desc_addr,
	    memdesc->bmd_major_version, memdesc->bmd_minor_version);
	if (memdesc->bmd_major_version > 3)
		panic("unhandled bootmem desc version %d.%d",
		    memdesc->bmd_major_version, memdesc->bmd_minor_version);

	blockaddr = memdesc->bmd_head_addr;
	if (blockaddr == 0)
		panic("bootmem list is empty");

	for (i = 0; i < VM_PHYSSEG_MAX && blockaddr != 0;
	    i++, blockaddr = block->bbh_next_block_addr) {
		block = (void *)MIPS_PHYS_TO_XKPHYS(CCA_CACHEABLE, blockaddr);

		mem_clusters[mem_cluster_cnt].start = blockaddr;
		mem_clusters[mem_cluster_cnt].size = block->bbh_size;
		mem_cluster_cnt++;
	}

	physmem = btoc(octeon_btinfo.obt_dram_size * 1024 * 1024);

#ifdef MULTIPROCESSOR
	const uint64_t fuse = octeon_xkphys_read_8(CIU_FUSE);
	const int cores = popcount64(fuse);
	mem_clusters[0].start += cores * PAGE_SIZE;
	mem_clusters[0].size  -= cores * PAGE_SIZE;
#endif

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	mips_page_physload(mips_trunc_page(kernel_text), mips_round_page(end),
	    mem_clusters, mem_cluster_cnt, NULL, 0);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	pmap_bootstrap();
}

void
parse_boot_args(void)
{
	int i;
	char *arg, *p;

	for (i = 0; i < octeon_btdesc.obt_argc; i++) {
		arg = (char *)MIPS_PHYS_TO_KSEG0(octeon_btdesc.obt_argv[i]);
		if (*arg == '-') {
			for (p = arg + 1; *p; p++) {
				switch (*p) {
				case '1':
					boothowto |= RB_MD1;
					break;
				case 's':
					boothowto |= RB_SINGLE;
					break;
				case 'd':
					boothowto |= RB_KDB;
					break;
				case 'a':
					boothowto |= RB_ASKNAME;
					break;
				case 'q':
					boothowto |= AB_QUIET;
					break;
				case 'v':
					boothowto |= AB_VERBOSE;
					break;
				case 'x':
					boothowto |= AB_DEBUG;
					break;
				case 'z':
					boothowto |= AB_SILENT;
					break;
				}
			}
		}
		if (strncmp(arg, "root=", 5) == 0)
			rootspec = strchr(arg, '=') + 1;
	}
}

/*
 * cpu_startup
 * cpu_reboot
 */

int	waittime = -1;

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup(void)
{

	/*
	 * Do the common startup items.
	 */
	cpu_startup_common();

	/*
	 * Virtual memory is bootstrapped -- notify the bus spaces
	 * that memory allocation is now safe.
	 */
	octeon_configuration.mc_mallocsafe = 1;

	fdtbus_intr_init();
}

void
cpu_reboot(int howto, char *bootstr)
{

	/* Take a snapshot before clobbering any registers. */
	savectx(curpcb);

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

	printf("%s\n\n", ((howto & RB_HALT) != 0) ? "halted." : "rebooting...");

	/*
	 * Need a small delay here, otherwise we see the first few characters of
	 * the warning below.
	 */
	delay(80000);

	octeon_soft_reset();

	delay(1000000);

	printf("WARNING: reset failed!\nSpinning...");

	for (;;)
		/* spin forever */ ;	/* XXX */
}
