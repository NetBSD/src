/*	$NetBSD: machdep.c,v 1.7 2000/03/31 14:51:49 soren Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
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

#include "opt_ddb.h"
#include "opt_execfmt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/device.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>

#include <vm/vm_kern.h>
#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/autoconf.h>
#include <machine/intr.h>
#include <mips/locore.h>

#include <machine/nvram.h>
#include <machine/leds.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include <dev/cons.h>

/* For sysctl. */
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;
char cpu_model[] = "Cobalt Microserver";

/* Maps for VM objects. */
vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

int	physmem;		/* Total physical memory */

char	bootstring[512];	/* Boot command */
int	netboot;		/* Are we netbooting? */

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	configure(void);
void	mach_init(unsigned int);

#ifdef DEBUG
/* Stack trace code violates prototypes to get callee's registers. */
extern void	stacktrace(void);
#endif

/*
 * safepri is a safe priority for sleep to set for a spin-wait during
 * autoconfiguration or after a panic.  Used as an argument to splx().
 */
int	safepri = MIPS1_PSL_LOWIPL;

extern struct user *proc0paddr;

static int cobalt_hardware_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(memsize)
	unsigned int memsize;
{
	caddr_t kernend, v, p0;
        u_long first, last;
	vsize_t size;
	extern char edata[], end[];
	int i;

	/*
	 * Clear BSS.
	 */
	kernend = (caddr_t)mips_round_page(end);
	memset(edata, 0, kernend - edata);

	physmem = btoc(memsize - MIPS_KSEG0_START);

	consinit();

	uvm_setpagesize();

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

	/*
	 * The boot command is passed in the top 512 bytes,
	 * so don't clobber that.
	 */
	mem_clusters[0].start = 0;
	mem_clusters[0].size = ctob(physmem) - 512;
	mem_cluster_cnt = 1;

	memcpy(bootstring, (char *)(memsize - 512), 512);
	bootstring[511] = '\0';

	for (i = 0; i < 512; i++) {
		switch (bootstring[i]) {
		case '\0':
			break;
		case ' ':
			continue;
		case '-':
			while (bootstring[i] != ' ') {
				i++;
				switch (bootstring[i]) {
				case 'a':
					boothowto |= RB_ASKNAME;
					break;
				case 'd':
					boothowto |= RB_KDB;
					break;
				case 's':
					boothowto |= RB_SINGLE;
					break;
				}
			}
		}
		if (memcmp("single", bootstring + i, 5) == 0)
			boothowto |= RB_SINGLE;
		if (memcmp("nfsroot=", bootstring + i, 8) == 0)
			netboot = 1;
		/*
		 * XXX Select root device from 'dev=/dev/hd[abcd][1234]' too.
		 */
	}

#ifdef DDB
	/*
	 * Initialize machine-dependent DDB commands, in case of early panic.
	 */
	db_machine_init();

	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
	uvm_page_physload(atop(first), atop(last), atop(first), atop(last),
		VM_FREELIST_DEFAULT);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Allocate space for proc0's USPACE
	 */
	p0 = (caddr_t)pmap_steal_memory(USPACE, NULL, NULL); 
	proc0.p_addr = proc0paddr = (struct user *)p0;
	proc0.p_md.md_regs = (struct frame *)(p0 + USPACE) - 1;
	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_context[11] = MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */

	/*
	 * Allocate space for system data structures.  These data structures
	 * are allocated here instead of cpu_startup() because physical
	 * memory is directly addressable.  We don't have to map these into
	 * virtual address space.
	 */
	size = (vsize_t)allocsys(NULL, NULL);
	v = (caddr_t)pmap_steal_memory(size, NULL, NULL); 
	if ((allocsys(v, NULL) - v) != size)
		panic("mach_init: table size inconsistency");

	mips_hardware_intr = cobalt_hardware_intr;

	pmap_bootstrap();
}

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup()
{
	unsigned i;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("%s memory", pbuf);

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
		    UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = NBPG * ((i < residual) ? (base + 1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
					"buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
				       VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				    16 * NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);
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
	printf(", %s free", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf(", %s in %d buffers\n", pbuf, nbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
}

int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return ENOTDIR;

	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}
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
	delay(500000);

	*(volatile char *)MIPS_PHYS_TO_KSEG1(LED_ADDR) = LED_RESET;
	printf("WARNING: reboot failed!\n");

	for (;;);
}

void
microtime(tvp)
	struct timeval *tvp;
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;

	/*
	 * Make sure that the time returned is always greater
	 * than that returned by the previous call.
	 */
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

unsigned long cpuspeed;

__inline void
delay(n)
	unsigned long n;
{
	volatile register long N = cpuspeed * n;

	while (--N > 0);
}

#ifdef EXEC_ECOFF
#include <sys/exec_ecoff.h>

int
cpu_exec_ecoff_hook(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	extern struct emul emul_netbsd;

	epp->ep_emul = &emul_netbsd;

	return 0;
}
#endif /* EXEC_ECOFF */

#define NINTR	6

static struct {
	int (*func)(void *);
	void *arg;
} intrtab[NINTR];

void *
cpu_intr_establish(level, ipl, func, arg)
	int level;
	int ipl;
	int (*func)(void *);
	void *arg;
{
	if (level < 0 || level >= NINTR)
		panic("invalid interrupt level");

	if (intrtab[level].func != NULL)
		panic("cannot share CPU interrupts");

	intrtab[level].func = func;
	intrtab[level].arg = arg;

	return (void *)-1;
}

static int
cobalt_hardware_intr(mask, pc, status, cause)
	u_int32_t mask;
	u_int32_t pc;
	u_int32_t status;
	u_int32_t cause;
{
	struct clockframe cf;
	static u_int32_t cycles;

	if (cause & MIPS_INT_MASK_0) {
		volatile u_int32_t *irq_src =
				(u_int32_t *)MIPS_PHYS_TO_KSEG1(0x14000c18);

		if (*irq_src & 0x00000100) {
			*irq_src = 0;

			cf.pc = pc;
			cf.sr = status;

			hardclock(&cf);
		}
		cause &= ~MIPS_INT_MASK_0;
	}

	if (cause & MIPS_INT_MASK_5) {
		cycles = mips3_cycle_count();
		mips3_write_compare(cycles + 1250000);	/* XXX */

		cf.pc = pc;
		cf.sr = status;
#if 0
		statclock(&cf);
#endif
		cause &= ~MIPS_INT_MASK_5;
	}

	if (cause & MIPS_INT_MASK_1) {
		if (intrtab[1].func != NULL)
			if ((*intrtab[1].func)(intrtab[1].arg))
				cause &= ~MIPS_INT_MASK_1;
	}

	if (cause & MIPS_INT_MASK_2) {
		if (intrtab[2].func != NULL)
			if ((*intrtab[2].func)(intrtab[2].arg))
				cause &= ~MIPS_INT_MASK_2;
	}

	if (cause & MIPS_INT_MASK_3) {
		if (intrtab[3].func != NULL)
			if ((*intrtab[3].func)(intrtab[3].arg))
				cause &= ~MIPS_INT_MASK_3;
	}

	if (cause & MIPS_INT_MASK_4) {
		if (intrtab[4].func != NULL)
			if ((*intrtab[4].func)(intrtab[4].arg))
				cause &= ~MIPS_INT_MASK_4;
	}

	return ((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);
}
