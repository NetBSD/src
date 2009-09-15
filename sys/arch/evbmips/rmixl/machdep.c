/*	$NetBSD: machdep.c,v 1.1.2.3 2009/09/15 02:46:43 cliff Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.1.2.3 2009/09/15 02:46:43 cliff Exp $");

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

#ifndef CONSPEED
# define CONSPEED 38400
#endif
#ifndef CONMODE
# define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8)
#endif
#ifndef CONSADDR
# define CONSADDR RMIXL_IO_DEV_UART_1
#endif

int		comcnfreq = -1;
uint		comcnspeed = CONSPEED;
uint		comcnmode  = CONMODE;
bus_addr_t	comcnaddr  = (bus_addr_t)CONSADDR;

struct rmixl_config rmixl_configuration;


/*
 * array of tested firmware versions
 * if you fiund new ones and they work
 * please add them
 */
static uint64_t rmiclfw_psb_versions[] = {
	0x4958d4fb00000056,
};
#define RMICLFW_PSB_VERSIONS_LEN \
	(sizeof(rmiclfw_psb_versions)/sizeof(rmiclfw_psb_versions[0]))

/*
 * kernel copies of firmware info
 */
static rmixlfw_info_t rmixlfw_info;
static rmixlfw_mmap_t rmixlfw_phys_mmap;
static rmixlfw_mmap_t rmixlfw_avail_mmap;


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
int mem_cluster_cnt;

void configure(void);
void mach_init(int, int32_t *, void *, void *);
static u_long rmixlfw_init(void *);
static void __attribute__((__noreturn__)) rmixl_exit(int);


/*
 * safepri is a safe priority for sleep to set for a spin-wait during
 * autoconfiguration or after a panic.  Used as an argument to splx().
 */
int	safepri = MIPS1_PSL_LOWIPL;

extern struct user *proc0paddr;

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(int argc, int32_t *argv, void *envp, void *infop)
{
	struct rmixl_config *rcp;
	void *kernend, *v;
        size_t first, last;
	u_long memsize;
	extern char edata[], end[];

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

	rcp = &rmixl_configuration;
	rcp->rc_io_pbase = MIPS_KSEG1_TO_PHYS(rmixlfw_info.io_base);
	rmixl_eb_bus_mem_init(&rcp->rc_eb_memt, rcp); /* need for console */
	rmixl_el_bus_mem_init(&rcp->rc_el_memt, rcp); /* XXX defer ? */

#if NCOM > 0
	rmixl_com_cnattach(comcnaddr, comcnspeed, comcnfreq,
		COM_TYPE_NORMAL, comcnmode);
#endif

	printf("\nNetBSD/rmixl\n");
	printf("memsize = %#lx\n", memsize);

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
		for (char *cp = (char *)(uint64_t)argv[i]; *cp; cp++) {
			int howto;
			/* Ignore superfluous '-', if there is one */
			if (*cp == '-')
				continue;

			howto = 0;
			BOOT_FLAG(*cp, howto);
			if (! howto)
				printf("bootflag '%c' not recognised\n", *cp);
			else
				boothowto |= howto;
		}
	}
	printf("boothowto %#x\n", boothowto);

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
	uvm_page_physload(atop(first), atop(last), atop(first), atop(last),
		VM_FREELIST_DEFAULT);
	for (int i = 1; i < mem_cluster_cnt; i++) {
		first = round_page(mem_clusters[i].start);
		last = mem_clusters[i].start + mem_clusters[i].size;
		uvm_page_physload(atop(first), atop(last), atop(first), atop(last),
			VM_FREELIST_DEFAULT);
	}

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	pmap_bootstrap();

	/*
	 * Allocate space for proc0's USPACE.
	 */
	v = (void *)uvm_pageboot_alloc(USPACE); 
	lwp0.l_addr = proc0paddr = (struct user *)v;
	lwp0.l_md.md_regs = (struct frame *)((char *)v + USPACE) - 1;
#ifdef _LP64
        lwp0.l_md.md_regs->f_regs[_R_SR] = MIPS_SR_KX;
#endif
        proc0paddr->u_pcb.pcb_context.val[_L_SR] =
#ifdef _LP64
            MIPS_SR_KX |
#endif  
            MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */


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

static u_long
rmixlfw_init(void *infop)
{
	uint64_t tmp;
	uint64_t sz;
	uint64_t sum;
#ifdef MEMSIZE
	u_long memsize = MEMSIZE;
#endif

	strcpy(cpu_model, "RMI XLS616ATX VIIA");	/* XXX */

	tmp = (int64_t)infop;
	tmp |= 0xffffffffULL << 32;
	infop = (void *)tmp;
	rmixlfw_info = *(rmixlfw_info_t *)infop;

	for (int i=0; i < RMICLFW_PSB_VERSIONS_LEN; i++) {
		if (rmiclfw_psb_versions[i] == rmixlfw_info.psb_version)
			goto found;
	}

	rmixl_putchar_init(MIPS_KSEG1_TO_PHYS(rmixlfw_info.io_base));
	rmixl_puts("\r\nWARNING: untested psb_version: ");
	rmixl_puthex64(rmixlfw_info.psb_version);
	rmixl_puts("\r\n");
 found:

	rmixlfw_phys_mmap  = *(rmixlfw_mmap_t *)rmixlfw_info.psb_physaddr_map;
	rmixlfw_avail_mmap = *(rmixlfw_mmap_t *)rmixlfw_info.avail_mem_map;

	sum = 0;
	mem_cluster_cnt = 0;
	for (uint32_t i=0; i < rmixlfw_avail_mmap.nmmaps; i++) {
		if (rmixlfw_avail_mmap.entry[i].type != RMIXLFW_MMAP_TYPE_RAM)
			continue;
		mem_clusters[i].start = rmixlfw_avail_mmap.entry[i].start;
		sz = rmixlfw_avail_mmap.entry[i].size;
		sum += sz;
		mem_clusters[i].size = sz;
		mem_cluster_cnt++;
#ifdef DEBUG
		rmixl_puthex32(i);
		rmixl_puts(": ");
		rmixl_puthex64(mem_clusters[i].start);
		rmixl_puts(": ");
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
			mem_clusters[i].size = sz;
			break;
		}
#endif
	}
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
	 * are allocated via the pool allocator, and we use KSEG to
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
	void (*reset)(void) = (void *)rmixlfw_info.warm_reset;
	(*reset)();
	printf("warm reset callback failed, spinning...\n");
	for (;;);
}
