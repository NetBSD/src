/*	$NetBSD: machdep.c,v 1.65.2.5 2002/06/23 17:39:03 jdolecek Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/boot_flag.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/db_machdep.h>
#include <ddb/db_extern.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>
#include <machine/bat.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <machine/platform.h>

#include <dev/cons.h>

/*
 * Global variables used here and there
 */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

struct pcb *curpcb;
struct pmap *curpm;
struct proc *fpuproc;

extern struct user *proc0paddr;

struct bat battable[16];

char *bootpath;

paddr_t msgbuf_paddr;
vaddr_t msgbuf_vaddr;

int	lcsplx(int);			/* called from locore.S */

static int fake_spl __P((int));
static void fake_splx __P((int));
static void fake_setsoft __P((int));
static void fake_clock_return __P((struct clockframe *, int));
static void *fake_intr_establish __P((int, int, int, int (*)(void *), void *));
static void fake_intr_disestablish __P((void *));

struct machvec machine_interface = {
	fake_spl,
	fake_spl,
	fake_splx,
	fake_setsoft,
	fake_clock_return,
	fake_intr_establish,
	fake_intr_disestablish,
};

void	ofppc_bootstrap_console(void);

void
initppc(startkernel, endkernel, args)
	u_int startkernel, endkernel;
	char *args;
{
	extern int trapcode, trapsize;
	extern int alitrap, alisize;
	extern int dsitrap, dsisize;
	extern int isitrap, isisize;
	extern int decrint, decrsize;
	extern int tlbimiss, tlbimsize;
	extern int tlbdlmiss, tlbdlmsize;
	extern int tlbdsmiss, tlbdsmsize;
#ifdef DDB
	extern int ddblow, ddbsize;
	extern void *startsym, *endsym;
#endif
#ifdef IPKDB
	extern int ipkdblow, ipkdbsize;
#endif
	int exc, scratch;

	/* Initialize the bootstrap console. */
	ofppc_bootstrap_console();

	/*
	 * Initialize BAT registers to unmapped to not generate
	 * overlapping mappings below.
	 */
	asm volatile ("mtibatu 0,%0" :: "r"(0));
	asm volatile ("mtibatu 1,%0" :: "r"(0));
	asm volatile ("mtibatu 2,%0" :: "r"(0));
	asm volatile ("mtibatu 3,%0" :: "r"(0));
	asm volatile ("mtdbatu 0,%0" :: "r"(0));
	asm volatile ("mtdbatu 1,%0" :: "r"(0));
	asm volatile ("mtdbatu 2,%0" :: "r"(0));
	asm volatile ("mtdbatu 3,%0" :: "r"(0));

	/*
	 * Set up initial BAT table to only map the lowest 256 MB area
	 */
	battable[0].batl = BATL(0x00000000, BAT_M, BAT_PP_RW);
	battable[0].batu = BATU(0x00000000, BAT_BL_256M, BAT_Vs);

	/*
	 * Now setup fixed bat registers
	 *
	 * Note that we still run in real mode, and the BAT
	 * registers were cleared above.
	 */
	/* IBAT0 used for initial 256 MB segment */
	asm volatile ("mtibatl 0,%0; mtibatu 0,%1"
		      :: "r"(battable[0].batl), "r"(battable[0].batu));
	/* DBAT0 used similar */
	asm volatile ("mtdbatl 0,%0; mtdbatu 0,%1"
		      :: "r"(battable[0].batl), "r"(battable[0].batu));

	/*
	 * Initialize the platform structure.  This may add entries
	 * to the BAT table.
	 */
	platform_init();

	proc0.p_addr = proc0paddr;
	memset(proc0.p_addr, 0, sizeof *proc0.p_addr);

	curpcb = &proc0paddr->u_pcb;

	curpm = curpcb->pcb_pmreal = curpcb->pcb_pm = pmap_kernel();

#ifdef __notyet__	/* Needs some rethinking regarding real/virtual OFW */
	OF_set_callback(callback);
#endif

	/*
	 * Set up trap vectors
	 */
	for (exc = EXC_RSVD; exc <= EXC_LAST; exc += 0x100)
		switch (exc) {
		default:
			memcpy((void *)exc, &trapcode, (size_t)&trapsize);
			break;
		case EXC_EXI:
			/*
			 * This one is (potentially) installed during autoconf
			 */
			break;
		case EXC_ALI:
			memcpy((void *)EXC_ALI, &alitrap, (size_t)&alisize);
			break;
		case EXC_DSI:
			memcpy((void *)EXC_DSI, &dsitrap, (size_t)&dsisize);
			break;
		case EXC_ISI:
			memcpy((void *)EXC_ISI, &isitrap, (size_t)&isisize);
			break;
		case EXC_DECR:
			memcpy((void *)EXC_DECR, &decrint, (size_t)&decrsize);
			break;
		case EXC_IMISS:
			memcpy((void *)EXC_IMISS, &tlbimiss, (size_t)&tlbimsize);
			break;
		case EXC_DLMISS:
			memcpy((void *)EXC_DLMISS, &tlbdlmiss, (size_t)&tlbdlmsize);
			break;
		case EXC_DSMISS:
			memcpy((void *)EXC_DSMISS, &tlbdsmiss, (size_t)&tlbdsmsize);
			break;
#if defined(DDB) || defined(IPKDB)
		case EXC_PGM:
		case EXC_TRC:
		case EXC_BPT:
#if defined(DDB)
			memcpy((void *)exc, &ddblow, (size_t)&ddbsize);
#else
			memcpy((void *)exc, &ipkdblow, (size_t)&ipkdbsize);
#endif
			break;
#endif /* DDB || IPKDB */
		}

	__syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);

	/*
	 * Now enable translation (and machine checks/recoverable interrupts).
	 */
	asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
		      : "=r"(scratch) : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));

	/*
	 * Now that translation is enabled (and we can access bus space),
	 * initialize the console.
	 */
	(*platform.cons_init)();

	/*
	 * Parse arg string.
	 */
	bootpath = args;
	while (*++args && *args != ' ');
	if (*args) {
		for(*args++ = 0; *args; args++)
			BOOT_FLAG(*args, boothowto);
	}

	/*
	 * Set the page size.
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel, NULL);

#ifdef DDB
	ddb_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef IPKDB
	/*
	 * Now trap to IPKDB
	 */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif
}

/*
 * This should probably be in autoconf!				XXX
 */
char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

void
install_extint(handler)
	void (*handler) __P((void));
{
	extern int extint, extsize;
	extern u_long extint_call;
	u_long offset = (u_long)handler - (u_long)&extint_call;
	int omsr, msr;

#ifdef	DIAGNOSTIC
	if (offset > 0x1ffffff)
		panic("install_extint: too far away");
#endif
	asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		      : "=r"(omsr), "=r"(msr) : "K"((u_short)~PSL_EE));
	extint_call = (extint_call & 0xfc000003) | offset;
	memcpy((void *)EXC_EXI, &extint, (size_t)&extsize);
	__syncicache((void *)&extint_call, sizeof extint_call);
	__syncicache((void *)EXC_EXI, (int)&extsize);
	asm volatile ("mtmsr %0" :: "r"(omsr));
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup()
{
	int sz, i;
	caddr_t v;
	paddr_t minaddr, maxaddr;
	int base, residual;
	char pbuf[9];

	proc0.p_addr = proc0paddr;
	v = (caddr_t)proc0paddr + USPACE;

	/*
	 * Initialize error message buffer (at end of core).
	 */
	if (!(msgbuf_vaddr = uvm_km_alloc(kernel_map, round_page(MSGBUFSIZE))))
		panic("startup: no room for message buffer");
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_enter(pmap_kernel(), msgbuf_vaddr + i * NBPG,
		    msgbuf_paddr + i * NBPG, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	pmap_update(pmap_kernel());
	initmsgbuf((caddr_t)msgbuf_vaddr, round_page(MSGBUFSIZE));

	printf("%s", version);
	cpu_identify(NULL, 0);

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	sz = (int)allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
	if (allocsys(v, NULL) - v != sz)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	sz = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(sz),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	if (base >= MAXBSIZE) {
		/* Don't want to alloc more physical mem than ever needed */
		base = MAXBSIZE;
		residual = 0;
	}
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
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("startup: not enough memory for "
					"buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ | VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	pmap_update(kernel_map->pmap);

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up the buffers.
	 */
	bufinit();

	/*
	 * Now allow hardware interrupts.
	 */
	{
		int msr;

		splhigh();
		asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
			      : "=r"(msr) : "K"((u_short)(PSL_EE|PSL_RI)));
	}
}

void
consinit()
{

	/* Nothing to do; console is already initialized. */
}

int	ofppc_cngetc(dev_t);
void	ofppc_cnputc(dev_t, int);

struct consdev ofppc_bootcons = {
	NULL, NULL, ofppc_cngetc, ofppc_cnputc, nullcnpollc, NULL,
	    makedev(0,0), 1,
};

int	ofppc_stdin_ihandle, ofppc_stdout_ihandle;
int	ofppc_stdin_phandle, ofppc_stdout_phandle;

void
ofppc_bootstrap_console(void)
{
	int chosen;
	char data[4];

	chosen = OF_finddevice("/chosen");

	if (OF_getprop(chosen, "stdin", data, sizeof(data)) != sizeof(int))
		goto nocons;
	ofppc_stdin_ihandle = of_decode_int(data);
	ofppc_stdin_phandle = OF_instance_to_package(ofppc_stdin_ihandle);

	if (OF_getprop(chosen, "stdout", data, sizeof(data)) != sizeof(int))
		goto nocons;
	ofppc_stdout_ihandle = of_decode_int(data);
	ofppc_stdout_phandle = OF_instance_to_package(ofppc_stdout_ihandle);

	cn_tab = &ofppc_bootcons;

 nocons:
	return;
}

int
ofppc_cngetc(dev_t dev)
{
	u_char ch = '\0';
	int l;

	while ((l = OF_read(ofppc_stdin_ihandle, &ch, 1)) != 1)
		if (l != -2 && l != 0)
			return (-1);

	return (ch);
}

void
ofppc_cnputc(dev_t dev, int c)
{
	char ch = c;

	OF_write(ofppc_stdout_ihandle, &ch, 1);
}

/*
 * Crash dump handling.
 */

void
dumpsys()
{
	printf("dumpsys: TBD\n");
}

/*
 * Soft networking interrupts.
 */
void
softnet()
{
	int isr = netisr;

	netisr = 0;

#define DONETISR(bit, fn) do {		\
	if (isr & (1 << bit))		\
		fn();			\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}

/*
 * Stray interrupts.
 */
void
strayintr(irq)
	int irq;
{
	log(LOG_ERR, "stray interrupt %d\n", irq);
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(howto, what)
	int howto;
	char *what;
{
	static int syncing;
	static char str[256];
	char *ap = str, *ap1 = ap;

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}
	splhigh();
	if (howto & RB_HALT) {
		doshutdownhooks();
		printf("halted\n\n");
		ppc_exit();
	}
	if (!cold && (howto & RB_DUMP))
		dumpsys();
	doshutdownhooks();
	printf("rebooting\n\n");
	if (what && *what) {
		if (strlen(what) > sizeof str - 5)
			printf("boot string too large, ignored\n");
		else {
			strcpy(str, what);
			ap1 = ap = str + strlen(str);
			*ap++ = ' ';
		}
	}
	*ap++ = '-';
	if (howto & RB_SINGLE)
		*ap++ = 's';
	if (howto & RB_KDB)
		*ap++ = 'd';
	*ap++ = 0;
	if (ap[-2] == '-')
		*ap1 = 0;
	ppc_boot(str);
}

#ifdef notyet
/*
 * OpenFirmware callback routine
 */
void
callback(p)
	void *p;
{
	panic("callback");	/* for now			XXX */
}
#endif

/*
 * Perform an `splx()' for locore.
 */
int
lcsplx(int ipl)
{

	return (_spllower(ipl));
}

/*
 * Initial Machine Interface.
 */
static int
fake_spl(int new)
{
	int scratch;

	asm volatile ("mfmsr %0; andi. %0,%0,%1; mtmsr %0; isync"
	    : "=r"(scratch) : "K"((u_short)~(PSL_EE|PSL_ME)));
	return (-1);
}

static void
fake_setsoft(int ipl)
{
	/* Do nothing */
}

static void
fake_splx(new)
	int new;
{

	(void) fake_spl(0);
}

static void
fake_clock_return(frame, nticks)
	struct clockframe *frame;
	int nticks;
{
	/* Do nothing */
}

static void *
fake_intr_establish(irq, level, ist, handler, arg)
	int irq, level, ist;
	int (*handler) __P((void *));
	void *arg;
{

	panic("fake_intr_establish");
}

static void
fake_intr_disestablish(cookie)
	void *cookie;
{

	panic("fake_intr_disestablish");
}
