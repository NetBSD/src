/*	$NetBSD: machdep.c,v 1.113.4.2 2002/07/06 14:05:49 lukem Exp $	*/

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
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_multiprocessor.h"
#include "adb.h"
#include "zsc.h"

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

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif
 
#ifdef IPKDB
#include <ipkdb/ipkdb.h>
#endif

#include <machine/autoconf.h>
#include <machine/bat.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/bus.h>
#include <machine/fpu.h>

#include <dev/cons.h>
#include <dev/ofw/openfirm.h>

#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wscons_callbacks.h>

#include <dev/usb/ukbdvar.h>

#include <macppc/dev/adbvar.h>

#if NZSC > 0
#include <machine/z8530var.h>
#endif

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

/*
 * Global variables used here and there
 */
#ifndef MULTIPROCESSOR
struct pcb *curpcb;
struct pmap *curpm;
struct proc *fpuproc;
#endif

extern struct user *proc0paddr;
extern int ofmsr;

struct bat battable[16];
char bootpath[256];
paddr_t msgbuf_paddr;
static int chosen;
struct pmap ofw_pmap;
int ofkbd_ihandle;

#ifdef DDB
void *startsym, *endsym;
#endif

struct ofw_translations {
	vaddr_t va;
	int len;
	paddr_t pa;
	int mode;
};

int ofkbd_cngetc(dev_t);
void cninit_kd(void);
int lcsplx(int);
int save_ofmap(struct ofw_translations *, int);
void restore_ofmap(struct ofw_translations *, int);
static void dumpsys(void);

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
#if defined(DDB) || defined(KGDB)
	extern int ddblow, ddbsize;
#endif
#ifdef IPKDB
	extern int ipkdblow, ipkdbsize;
#endif
	int exc, scratch;
	struct mem_region *allmem, *availmem, *mp;
	struct ofw_translations *ofmap;
	int ofmaplen;
#ifdef MULTIPROCESSOR
	struct cpu_info *ci = &cpu_info[0];
#else
	struct cpu_info *ci = &cpu_info_store;
#endif

	asm volatile ("mtsprg 0,%0" :: "r"(ci));

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
	 * Set up BAT0 to only map the lowest 256 MB area
	 */
	battable[0x0].batl = BATL(0x00000000, BAT_M, BAT_PP_RW);
	battable[0x0].batu = BATU(0x00000000, BAT_BL_256M, BAT_Vs);

	/*
	 * Map PCI memory space.
	 */
	battable[0x8].batl = BATL(0x80000000, BAT_I|BAT_G, BAT_PP_RW);
	battable[0x8].batu = BATU(0x80000000, BAT_BL_256M, BAT_Vs);

	battable[0x9].batl = BATL(0x90000000, BAT_I|BAT_G, BAT_PP_RW);
	battable[0x9].batu = BATU(0x90000000, BAT_BL_256M, BAT_Vs);

	battable[0xa].batl = BATL(0xa0000000, BAT_I|BAT_G, BAT_PP_RW);
	battable[0xa].batu = BATU(0xa0000000, BAT_BL_256M, BAT_Vs);

	battable[0xb].batl = BATL(0xb0000000, BAT_I|BAT_G, BAT_PP_RW);
	battable[0xb].batu = BATU(0xb0000000, BAT_BL_256M, BAT_Vs);

	/*
	 * Map obio devices.
	 */
	battable[0xf].batl = BATL(0xf0000000, BAT_I|BAT_G, BAT_PP_RW);
	battable[0xf].batu = BATU(0xf0000000, BAT_BL_256M, BAT_Vs);

	/*
	 * Now setup fixed bat registers
	 *
	 * Note that we still run in real mode, and the BAT
	 * registers were cleared above.
	 */
	/* BAT0 used for initial 256 MB segment */
	asm volatile ("mtibatl 0,%0; mtibatu 0,%1;"
		      "mtdbatl 0,%0; mtdbatu 0,%1;"
		      :: "r"(battable[0].batl), "r"(battable[0].batu));
	/* BAT1 used for primary I/O 256 MB segment */
	asm volatile ("mtdbatl 1,%0; mtdbatu 1,%1;"
		      :: "r"(battable[8].batl), "r"(battable[8].batu));

	/*
	 * Set up battable to map all RAM regions.
	 * This is here because mem_regions() call needs bat0 set up.
	 */
	mem_regions(&allmem, &availmem);
	for (mp = allmem; mp->size; mp++) {
		paddr_t pa = mp->start & 0xf0000000;
		paddr_t end = mp->start + mp->size;

		do {
			u_int n = pa >> 28;

			battable[n].batl = BATL(pa, BAT_M, BAT_PP_RW);
			battable[n].batu = BATU(pa, BAT_BL_256M, BAT_Vs);
			pa += 0x10000000;
		} while (pa < end);
	}

	chosen = OF_finddevice("/chosen");

	ofmaplen = save_ofmap(NULL, 0);
	ofmap = alloca(ofmaplen);
	save_ofmap(ofmap, ofmaplen);

	proc0.p_cpu = ci;
	proc0.p_addr = proc0paddr;
	memset(proc0.p_addr, 0, sizeof *proc0.p_addr);

	curpcb = &proc0paddr->u_pcb;

	curpm = curpcb->pcb_pmreal = curpcb->pcb_pm = pmap_kernel();

#ifdef	__notyet__		/* Needs some rethinking regarding real/virtual OFW */
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
#if defined(DDB) || defined(IPKDB) || defined(KGDB)
		case EXC_PGM:
		case EXC_TRC:
		case EXC_BPT:
#if defined(DDB) || defined(KGDB)
			memcpy((void *)exc, &ddblow, (size_t)&ddbsize);
#if defined(IPKDB)
#error "cannot enable IPKDB with DDB or KGDB"
#endif
#else
			memcpy((void *)exc, &ipkdblow, (size_t)&ipkdbsize);
#endif
			break;
#endif /* DDB || IPKDB || KGDB */
		}

	cpu_probe_cache();
	/*
	 * external interrupt handler install
	 */
	install_extint(ext_intr);

	__syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);

	/*
	 * Now enable translation (and machine checks/recoverable interrupts).
	 */
	asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0; isync"
		      : "=r"(scratch) : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));

	ofmsr &= ~PSL_IP;

	/*
	 * Parse arg string.
	 */
#ifdef DDB
	memcpy(&startsym, args + strlen(args) + 1, sizeof(startsym));
	memcpy(&endsym, args + strlen(args) + 5, sizeof(endsym));
	if (startsym == NULL || endsym == NULL)
		startsym = endsym = NULL;
#endif

	strcpy(bootpath, args);
	args = bootpath;
	while (*++args && *args != ' ');
	if (*args) {
		*args++ = 0;
		while (*args)
			BOOT_FLAG(*args++, boothowto);
	}

	/*
	 * If the bootpath doesn't start with a / then it isn't
	 * an OFW path and probably is an alias, so look up the alias
	 * and regenerate the full bootpath so device_register will work.
	 */
	if (bootpath[0] != '/' && bootpath[0] != '\0') {
		int aliases = OF_finddevice("/aliases");
		char tmpbuf[100];
		char aliasbuf[256];
		if (aliases != 0) {
			char *cp1, *cp2, *cp;
			char saved_ch = 0;
			int len;
			cp1 = strchr(bootpath, ':');
			cp2 = strchr(bootpath, ',');
			cp = cp1;
			if (cp1 == NULL || (cp2 != NULL && cp2 < cp1))
				cp = cp2;
			tmpbuf[0] = '\0';
			if (cp != NULL) {
				strcpy(tmpbuf, cp);
				saved_ch = *cp;
				*cp = '\0';
			}
			len = OF_getprop(aliases, bootpath, aliasbuf,
			    sizeof(aliasbuf));
			if (len > 0) {
				if (aliasbuf[len-1] == '\0')
					len--;
				memcpy(bootpath, aliasbuf, len);
				strcpy(&bootpath[len], tmpbuf);
			} else {
				*cp = saved_ch;
			}
		}
	}

	/*
	 * i386 port says, that this shouldn't be here,
	 * but I really think the console should be initialized
	 * as early as possible.
	 */
	consinit();

	/*
	 * Set the page size.
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel, NULL);

	restore_ofmap(ofmap, ofmaplen);
}

int
save_ofmap(ofmap, maxlen)
	struct ofw_translations *ofmap;
	int maxlen;
{
	int mmui, mmu, len;

	OF_getprop(chosen, "mmu", &mmui, sizeof mmui);
	mmu = OF_instance_to_package(mmui);

	if (ofmap) {
		memset(ofmap, 0, maxlen);	/* to be safe */
		len = OF_getprop(mmu, "translations", ofmap, maxlen);
	} else
		len = OF_getproplen(mmu, "translations");

	return len;
}

void
restore_ofmap(ofmap, len)
	struct ofw_translations *ofmap;
	int len;
{
	int n = len / sizeof(struct ofw_translations);
	int i;

	pmap_pinit(&ofw_pmap);

	ofw_pmap.pm_sr[KERNEL_SR] = KERNEL_SEGMENT;

	for (i = 0; i < n; i++) {
		paddr_t pa = ofmap[i].pa;
		vaddr_t va = ofmap[i].va;
		int len = ofmap[i].len;

		if (va < 0xf0000000)			/* XXX */
			continue;

		while (len > 0) {
			pmap_enter(&ofw_pmap, va, pa, VM_PROT_ALL,
			    VM_PROT_ALL|PMAP_WIRED);
			pa += NBPG;
			va += NBPG;
			len -= NBPG;
		}
	}
	pmap_update(&ofw_pmap);
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
	vaddr_t minaddr, maxaddr;
	int base, residual;
	char pbuf[9];

	initmsgbuf((caddr_t)msgbuf_paddr, round_page(MSGBUFSIZE));

	proc0.p_addr = proc0paddr;
	v = (caddr_t)proc0paddr + USPACE;

	printf("%s", version);
	cpu_identify(NULL, 0);

	format_bytes(pbuf, sizeof(pbuf), ctob((u_int)physmem));
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
	minaddr = 0;
	if (uvm_map(kernel_map, (vaddr_t *)&minaddr, round_page(sz),
		NULL, UVM_UNKNOWN_OFFSET, 0,
		UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
			    UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	buffers = (char *)minaddr;
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

		curbuf = (vaddr_t)buffers + i * MAXBSIZE;
		curbufsize = NBPG * (i < residual ? base + 1 : base);

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
				 16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, FALSE, NULL);

#ifndef PMAP_MAP_POOLPAGE
	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    mclbytes*nmbclusters, VM_MAP_INTRSAFE, FALSE, NULL);
#endif

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up the buffers.
	 */
	bufinit();
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit()
{
	static int initted;

	if (initted)
		return;
	initted = 1;
	cninit();

#ifdef DDB
	ddb_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
	if (boothowto & RB_KDB)
		Debugger();
#endif

#ifdef IPKDB
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif

#ifdef KGDB
#if NZSC > 0
	zs_kgdb_init();
#endif
	if (boothowto & RB_KDB)
		kgdb_connect(1);
#endif
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
	extern volatile int netisr;
	int isr;

	isr = netisr;
	netisr = 0;

#define DONETISR(bit, fn) do {		\
	if (isr & (1 << bit))		\
		fn();			\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR

}

#include "zsc.h"
#include "com.h"
/*
 * Soft tty interrupts.
 */
void
softserial()
{
#if NZSC > 0
	zssoft(NULL);
#endif
#if NCOM > 0
	comsoft();
#endif
}

#if 0
/*
 * Stray interrupts.
 */
void
strayintr(irq)
	int irq;
{
	log(LOG_ERR, "stray interrupt %d\n", irq);
}
#endif

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

#ifdef MULTIPROCESSOR
	/* Halt other CPU.  XXX for now... */
	macppc_send_ipi(&cpu_info[1 - cpu_number()], MACPPC_IPI_HALT);
	delay(100000);	/* XXX */
#endif

	splhigh();

	if (!cold && (howto & RB_DUMP))
		dumpsys();

	doshutdownhooks();

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
#if NADB > 0
		delay(1000000);
		adb_poweroff();
		printf("WARNING: powerdown failed!\n");
#endif
	}

	if (howto & RB_HALT) {
		printf("halted\n\n");

		/* flush cache for msgbuf */
		__syncicache((void *)msgbuf_paddr, round_page(MSGBUFSIZE));

		ppc_exit();
	}

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

	/* flush cache for msgbuf */
	__syncicache((void *)msgbuf_paddr, round_page(MSGBUFSIZE));

#if NADB > 0
	adb_restart();	/* not return */
#endif
	ppc_exit();
}

#if 0
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

int
lcsplx(ipl)
	int ipl;
{
	return spllower(ipl); 	/* XXX */
}

/*
 * Convert kernel VA to physical address
 */
int
kvtop(addr)
	caddr_t addr;
{
	vaddr_t va;
	paddr_t pa;
	int off;
	extern char end[];

	if (addr < end)
		return (int)addr;

	va = trunc_page((vaddr_t)addr);
	off = (int)addr - va;

	if (pmap_extract(pmap_kernel(), va, &pa) == FALSE) {
		/*printf("kvtop: zero page frame (va=0x%x)\n", addr);*/
		return (int)addr;
	}

	return((int)pa + off);
}

/*
 * Allocate vm space and mapin the I/O address
 */
void *
mapiodev(pa, len)
	paddr_t pa;
	psize_t len;
{
	paddr_t faddr;
	vaddr_t taddr, va;
	int off;

	faddr = trunc_page(pa);
	off = pa - faddr;
	len = round_page(off + len);
	va = taddr = uvm_km_valloc(kernel_map, len);

	if (va == 0)
		return NULL;

	for (; len > 0; len -= NBPG) {
		pmap_kenter_pa(taddr, faddr, VM_PROT_READ | VM_PROT_WRITE);
		faddr += NBPG;
		taddr += NBPG;
	}
	pmap_update(pmap_kernel());
	return (void *)(va + off);
}

#include "akbd.h"
#include "ukbd.h"
#include "ofb.h"
#include "ite.h"
#include "zstty.h"

void
cninit()
{
	struct consdev *cp;
	int stdout, node;
	char type[16];

	if (OF_getprop(chosen, "stdout", &stdout, sizeof(stdout))
	    != sizeof(stdout))
		goto nocons;

	node = OF_instance_to_package(stdout);
	memset(type, 0, sizeof(type));
	if (OF_getprop(node, "device_type", type, sizeof(type)) == -1)
		goto nocons;

#if NOFB > 0
	if (strcmp(type, "display") == 0) {
		cninit_kd();
		return;
	}
#endif /* NOFB > 0 */

#if NITE > 0
	if (strcmp(type, "display") == 0) {
		extern struct consdev consdev_ite;

		cp = &consdev_ite;
		(*cp->cn_probe)(cp);
		(*cp->cn_init)(cp);
		cn_tab = cp;

		return;
	}
#endif

#if NZSTTY > 0
	if (strcmp(type, "serial") == 0) {
		extern struct consdev consdev_zs;

		cp = &consdev_zs;
		(*cp->cn_probe)(cp);
		(*cp->cn_init)(cp);
		cn_tab = cp;

		return;
	}
#endif

nocons:
	return;
}

#if NOFB > 0
struct usb_kbd_ihandles {
	struct usb_kbd_ihandles *next;
	int ihandle;
};

void
cninit_kd()
{
	int stdin, node;
	char name[16];
#if NAKBD > 0
	int akbd;
#endif
#if NUKBD > 0
	struct usb_kbd_ihandles *ukbds;
	int ukbd;
#endif

	/*
	 * Attach the console output now (so we can see debugging messages,
	 * if any).
	 */
	ofb_cnattach();

	/*
	 * We must determine which keyboard type we have.
	 */
	if (OF_getprop(chosen, "stdin", &stdin, sizeof(stdin))
	    != sizeof(stdin)) {
		printf("WARNING: no `stdin' property in /chosen\n");
		return;
	}

	node = OF_instance_to_package(stdin);
	memset(name, 0, sizeof(name));
	OF_getprop(node, "name", name, sizeof(name));
	if (strcmp(name, "keyboard") != 0) {
		printf("WARNING: stdin is not a keyboard: %s\n", name);
		return;
	}

#if NAKBD > 0
	memset(name, 0, sizeof(name));
	OF_getprop(OF_parent(node), "name", name, sizeof(name));
	if (strcmp(name, "adb") == 0) {
		printf("console keyboard type: ADB\n");
		akbd_cnattach();
		goto kbd_found;
	}
#endif

	/*
	 * We're not an ADB keyboard; must be USB.  Unfortunately,
	 * we have a few problems:
	 *
	 *	(1) The stupid Macintosh firmware uses a
	 *	    `psuedo-hid' (yes, they even spell it
	 *	    incorrectly!) which apparently merges
	 *	    all USB keyboard input into a single
	 *	    input stream.  Because of this, we can't
	 *	    actually determine which USB controller
	 *	    or keyboard is really the console keyboard!
	 *
	 *	(2) Even if we could, USB requires a lot of
	 *	    the kernel to be running in order for it
	 *	    to work.
	 *
	 * So, what we do is this:
	 *
	 *	(1) Tell the ukbd driver that it is the console.
	 *	    At autoconfiguration time, it will attach the
	 *	    first USB keyboard instance as the console
	 *	    keyboard.
	 *
	 *	(2) Until then, so that we have _something_, we
	 *	    use the OpenFirmware I/O facilities to read
	 *	    the keyboard.
	 */

	/*
	 * stdin is /psuedo-hid/keyboard.  Test `adb-kbd-ihandle and
	 * `usb-kbd-ihandles to figure out the real keyboard(s).
	 *
	 * XXX This must be called before pmap_bootstrap().
	 */

#if NUKBD > 0
	if (OF_call_method("`usb-kbd-ihandles", stdin, 0, 1, &ukbds) >= 0 &&
	    ukbds != NULL && ukbds->ihandle != 0 &&
	    OF_instance_to_package(ukbds->ihandle) != -1) {
		printf("console keyboard type: USB\n");
		ukbd_cnattach();
		goto kbd_found;
	}
	/* Try old method name. */
	if (OF_call_method("`usb-kbd-ihandle", stdin, 0, 1, &ukbd) >= 0 &&
	    ukbd != 0 &&
	    OF_instance_to_package(ukbd) != -1) {
		printf("console keyboard type: USB\n");
		stdin = ukbd;
		ukbd_cnattach();
		goto kbd_found;
	}
#endif

#if NAKBD > 0
	if (OF_call_method("`adb-kbd-ihandle", stdin, 0, 1, &akbd) >= 0 &&
	    akbd != 0 &&
	    OF_instance_to_package(akbd) != -1) {
		printf("console keyboard type: ADB\n");
		stdin = akbd;
		akbd_cnattach();
		goto kbd_found;
	}
#endif

#if NUKBD > 0
	/*
	 * XXX Old firmware does not have `usb-kbd-ihandles method.  Assume
	 * XXX USB keyboard anyway.
	 */
	printf("console keyboard type: USB\n");
	ukbd_cnattach();
	goto kbd_found;
#endif

	/*
	 * No keyboard is found.  Just return.
	 */
	printf("no console keyboard\n");
	return;

#if NAKBD + NUKBD > 0
kbd_found:
	/*
	 * XXX This is a little gross, but we don't get to call
	 * XXX wskbd_cnattach() twice.
	 */
	ofkbd_ihandle = stdin;
	wsdisplay_set_cons_kbd(ofkbd_cngetc, NULL, NULL);
#endif
}
#endif

/*
 * Bootstrap console keyboard routines, using OpenFirmware I/O.
 */
int
ofkbd_cngetc(dev)
	dev_t dev;
{
	u_char c = '\0';
	int len;

	do {
		len = OF_read(ofkbd_ihandle, &c, 1);
	} while (len != 1);

	return c;
}

#ifdef MULTIPROCESSOR
void
save_fpu_proc(p)
	struct proc *p;
{
	volatile struct cpu_info *fpcpu;
	int i;
	extern volatile int IPI[];	/* XXX */

	fpcpu = p->p_addr->u_pcb.pcb_fpcpu;
	if (fpcpu == curcpu()) {
		save_fpu(p);
		return;
	}

#if 0
	printf("save_fpu_proc{%d} pid = %d, fpcpu->ci_cpuid = %d\n",
	    cpu_number(), p->p_pid, fpcpu->ci_cpuid);
#endif

	macppc_send_ipi(fpcpu, MACPPC_IPI_FLUSH_FPU);

	/* Wait for flush. */
#if 0
	while (fpcpu->ci_fpuproc);
#else
	for (i = 0; i < 0x3fffffff; i++) {
		if (fpcpu->ci_fpuproc == NULL)
			goto done;
	}
	printf("save_fpu_proc{%d} pid = %d, fpcpu->ci_cpuid = %d\n",
	    cpu_number(), p->p_pid, fpcpu->ci_cpuid);
	printf("IPI[0] = 0x%x, IPI[1] = 0x%x\n", IPI[0], IPI[1]);
	printf("cpl 0x%x 0x%x\n", cpu_info[0].ci_cpl, cpu_info[1].ci_cpl);
	printf("ipending 0x%x 0x%x\n", cpu_info[0].ci_ipending, cpu_info[1].ci_ipending);
	panic("save_fpu_proc");
done:;

#endif
}
#endif /* MULTIPROCESSOR */
