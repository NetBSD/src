/*	$NetBSD: machdep.c,v 1.35.4.5 2002/06/24 22:04:25 nathanw Exp $	*/

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
#include "opt_kgdb.h"
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
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/intr_machdep.h>
#include <mips/locore.h>

#include <machine/nvram.h>
#include <machine/leds.h>

#include <dev/cons.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#define ELFSIZE		DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

/* For sysctl. */
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;
char cpu_model[] = "Cobalt Microserver";

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

/* Maps for VM objects. */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int	physmem;		/* Total physical memory */

char	bootstring[512];	/* Boot command */
int	netboot;		/* Are we netbooting? */

char *	nfsroot_bstr = NULL;
char *	root_bstr = NULL;
int	bootunit = -1;
int	bootpart = -1;


phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	configure(void);
void	mach_init(unsigned int);
void	decode_bootstring(void);
static char *	strtok_light(char *, const char);

/*
 * safepri is a safe priority for sleep to set for a spin-wait during
 * autoconfiguration or after a panic.  Used as an argument to splx().
 */
int	safepri = MIPS1_PSL_LOWIPL;

extern caddr_t esym;
extern struct user *proc0paddr;



/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(memsize)
	unsigned int memsize;
{
	caddr_t kernend, v;
        u_long first, last;
	vsize_t size;
	extern char edata[], end[];

	/*
	 * Clear the BSS segment.
	 */
#ifdef DDB
	if (memcmp(((Elf_Ehdr *)end)->e_ident, ELFMAG, SELFMAG) == 0 &&
	    ((Elf_Ehdr *)end)->e_ident[EI_CLASS] == ELFCLASS) {
		esym = end;
		esym += ((Elf_Ehdr *)end)->e_entry;
		kernend = (caddr_t)mips_round_page(esym);
		memset(edata, 0, end - edata);
	} else
#endif
	{
		kernend = (caddr_t)mips_round_page(end);
		memset(edata, 0, kernend - edata);
	}

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
	memset((char *)(memsize - 512), 0, 512);
	bootstring[511] = '\0';

	decode_bootstring();

#ifdef DDB
	ddb_init(0, NULL, NULL);
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef KGDB
        if (boothowto & RB_KDB)
                kgdb_connect(0);
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
	 * Compute the size of system data structures.  pmap_bootstrap()
	 * needs some of this information.
	 */
	size = (vsize_t)allocsys(NULL, NULL);

	pmap_bootstrap();

	/*
	 * Allocate space for proc0's USPACE.
	 */
	v = (caddr_t)uvm_pageboot_alloc(USPACE); 
	proc0.p_addr = proc0paddr = (struct user *)v;
	proc0.p_md.md_regs = (struct frame *)(v + USPACE) - 1;
	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_context[11] = MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */

	/*
	 * Allocate space for system data structures.  These data structures
	 * are allocated here instead of cpu_startup() because physical
	 * memory is directly addressable.  We don't have to map these into
	 * virtual address space.
	 */
	v = (caddr_t)uvm_pageboot_alloc(size); 
	if ((allocsys(v, NULL) - v) != size)
		panic("mach_init: table size inconsistency");
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
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
		    UVM_ADV_NORMAL, 0)) != 0)
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
	pmap_update(pmap_kernel());

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
	if (curlwp)
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
	u_int32_t counter0;

	*tvp = time;

	counter0 = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x14000850);

	/*
	 * XXX
	 */

	counter0 /= 50;
	counter0 %= 10000;

	if (counter0 > 9999) {
		counter0 = 9999;
	}

	tvp->tv_usec -= tvp->tv_usec % 10000;
	tvp->tv_usec += 10000 - counter0;

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

#define NINTR	6

static struct cobalt_intr intrtab[NINTR];

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

	intrtab[level].cookie_type = COBALT_COOKIE_TYPE_CPU;
	intrtab[level].func = func;
	intrtab[level].arg = arg;

	return &intrtab[level];
}

void
cpu_intr_disestablish(cookie)
	void *cookie;
{
	struct cobalt_intr *p = cookie;

        if (p->cookie_type == COBALT_COOKIE_TYPE_CPU) {
		p->func = NULL;
		p->arg = NULL;
	}
}

void
cpu_intr(status, cause, pc, ipending)
	u_int32_t status;
	u_int32_t cause;
	u_int32_t pc;
	u_int32_t ipending;
{
	struct clockframe cf;
	static u_int32_t cycles;
	int i;

	uvmexp.intrs++;

	if (ipending & MIPS_INT_MASK_0) {
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

	for (i = 0; i < 5; i++) {
		if (ipending & (MIPS_INT_MASK_0 << i))
			if (intrtab[i].func != NULL)
				if ((*intrtab[i].func)(intrtab[i].arg))
					cause &= ~(MIPS_INT_MASK_0 << i);
	}

	if (ipending & MIPS_INT_MASK_5) {
		cycles = mips3_cp0_count_read();
		mips3_cp0_compare_write(cycles + 1250000);	/* XXX */

#if 0
		cf.pc = pc;
		cf.sr = status;

		statclock(&cf);
#endif
		cause &= ~MIPS_INT_MASK_5;
	}

	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	/* 'softnet' interrupt */
	if (ipending & MIPS_SOFT_INT_MASK_1) {
		clearsoftnet();
		uvmexp.softs++;
		netintr();
	}

	/* 'softclock' interrupt */
	if (ipending & MIPS_SOFT_INT_MASK_0) {
		clearsoftclock();
		uvmexp.softs++;
		intrcnt[SOFTCLOCK_INTR]++;
		softclock(NULL);
	}
}


void
decode_bootstring(void)
{
	char * work;
	char * equ;
	int i;

	/* break apart bootstring on ' ' boundries  and itterate*/
	work = strtok_light(bootstring, ' ');
	while (work != '\0') {
		/* if starts with '-', we got options, walk its decode */
		if (work[0] == '-') {
			i = 1;
			while (work[i] != ' ' && work[i] != '\0') {
				BOOT_FLAG(work[i], boothowto);
				i++;
			}
		} else

		/* if it has a '=' its an assignment, switch and set */
		if ((equ = strchr(work,'=')) != '\0') {
			if(0 == memcmp("nfsroot=", work, 8)) {
				nfsroot_bstr = (equ +1);
			} else
			if(0 == memcmp("root=", work, 5)) {
				root_bstr = (equ +1);
			} 
		} else

		/* else it a single value, switch and process */
		if (memcmp("single", work, 5) == 0) {
			boothowto |= RB_SINGLE;
		} else
		if (memcmp("ro", work, 2) == 0) {
			/* this is also inserted by the firmware */
		}

		/* grab next token */
		work = strtok_light(NULL, ' ');
	}

	if (root_bstr != NULL) {
		/* this should be of the form "/dev/hda1" */
		/* [abcd][1234]    drive partition  linux probe order */
		if ((memcmp("/dev/hd",root_bstr,7) == 0) &&
		    (strlen(root_bstr) == 9) ){
			bootunit = root_bstr[7] - 'a';
			bootpart = root_bstr[8] - '1';
		}
	}
}


static char *
strtok_light(str, sep)
	char * str;
	const char sep;
{
	static char * proc;
	char * head;
	char * work;

	if (str != NULL)
		proc = str;
	if (proc == NULL)  /* end of string return NULL */
		return proc;

	head = proc;

	work = strchr (proc, sep);
	if (work == NULL) {  /* we hit the end */
		proc = work;
	} else {
		proc = (work +1 );
		*work = '\0';
	}

	return head;
}

