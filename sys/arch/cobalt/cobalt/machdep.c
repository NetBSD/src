/*	$NetBSD: machdep.c,v 1.1 2000/03/19 23:07:44 soren Exp $	*/

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

int maxmem;		/* Max memory per process */
int physmem;		/* Total physical memory */

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	configure(void);
void	mach_init(void);

#ifdef DEBUG
/* Stack trace code violates prototypes to get callee's registers. */
extern void	stacktrace(void);
#endif

/*
 * safepri is a safe priority for sleep to set for a spin-wait during
 * autoconfiguration or after a panic.  Used as an argument to splx().
 */
int	safepri = MIPS3_PSL_LOWIPL;

extern void mips_vector_init(void);

extern struct user *proc0paddr;

static int cobalt_hardware_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

/* XXX */
extern int iointr(unsigned int, struct clockframe *);
#define ICU_LEN 16
extern struct intrhand *intrhand[ICU_LEN]; 
extern struct com_mainbus_softc *com0;
extern void *tlp0;
int comintr(void *); 
int tlp_intr(void *);

static int
cobalt_hardware_intr(mask, pc, status, cause)
	u_int32_t mask;
	u_int32_t pc;
	u_int32_t status;
	u_int32_t cause;
{
	struct clockframe cf;
	static u_int32_t cycles;
	int s;

	cf.pc = pc;
	cf.sr = status;

	/* XXX Reverse hardlock and statclock? */

#define TICK_CYCLES 1250000			/* XXX 250 MHz XXX */
	if (cause & MIPS_INT_MASK_5) {
		cycles = mips3_cycle_count();
		mips3_write_compare(cycles + TICK_CYCLES);

		s = splstatclock(); /* XXX redo interrupts XXX */
		statclock(&cf);
		splx(s);	/* XXX redo interrupts XXX */

		cause &= ~MIPS_INT_MASK_5;
	}

	if (cause & MIPS_INT_MASK_4) {
		s = splbio(); /* XXX redo interrupts XXX */
		iointr(mask, &cf);
		splx(s);	/* XXX redo interrupts XXX */
		cause &= ~MIPS_INT_MASK_4;
	}

	if (cause & MIPS_INT_MASK_3) {
		s = spltty(); /* XXX redo interrupts XXX */
		comintr(com0);
		splx(s);	/* XXX redo interrupts XXX */
		cause &= ~MIPS_INT_MASK_3;
	}

	if (cause & MIPS_INT_MASK_1) {
		s = splnet();	/* XXX redo interrupts XXX */
		tlp_intr(tlp0);
		splx(s);	/* XXX redo interrupts XXX */
		cause &= ~MIPS_INT_MASK_1;
	}
	if (cause & MIPS_INT_MASK_2) {
		cause &= ~MIPS_INT_MASK_2;
	}

	if (cause & MIPS_INT_MASK_0) {
		volatile u_int32_t *irq_src =
				(u_int32_t *)MIPS_PHYS_TO_KSEG1(0x14000c18);

		if (*irq_src & 0x00000100) {
			*irq_src = 0;
			s = splclock();	/* XXX redo interrupts XXX */
			hardclock(&cf);
			splx(s);	/* XXX redo interrupts XXX */
		}
		cause &= ~MIPS_INT_MASK_0;
	}

	return ((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);
}

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(void)
{
	caddr_t kernend, v, p0;
        u_long first, last;
	vsize_t size;
	extern char edata[], end[];

	/*
	 * Clear BSS.
	 */
	kernend = (caddr_t)mips_round_page(end);
	memset(edata, 0, kernend - edata);

	consinit();

#if 1
	boothowto = RB_SINGLE;
#endif

	uvm_setpagesize();

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

#ifdef DDB
	/*
	 * Initialize machine-dependent DDB commands, in case of early panic.
	 */
	db_machine_init();

	/*
	 * XXX Also get the symbol table. We wants it, yes, my precious.
	 */
#endif
	
	physmem = btoc(16 * 1024 * 1024);	/* XXX */
	maxmem = physmem / 2;

	/* XXX */
	mem_clusters[0].start = 0;
	mem_clusters[0].size  = ctob(physmem);
	mem_cluster_cnt = 1;

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
	uvm_page_physload(atop(first), atop(last), atop(first), atop(last),
		VM_FREELIST_DEFAULT);

	mips_init_msgbuf();

	p0 = (caddr_t)pmap_steal_memory(USPACE, NULL, NULL); 
	proc0.p_addr = proc0paddr = (struct user *)p0;
	proc0.p_md.md_regs = (struct frame *)((caddr_t)p0 + USPACE) - 1;
	curpcb = &proc0.p_addr->u_pcb;
	memset(p0, 0, USPACE);

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
 * initialize CPU, and do autoconfiguration.
 */
void
cpu_startup()
{
	unsigned i;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];

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
	if ((howto & RB_NOSYNC) && (waittime < 0)) {
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
	delay(100000);

	*(volatile char *)MIPS_PHYS_TO_KSEG1(LED_ADDR) = 0x0f; /* Hard reset. */
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
