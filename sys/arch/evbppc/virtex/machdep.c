/*	$NetBSD: machdep.c,v 1.9.4.1 2009/05/13 17:17:41 jym Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Based on Walnut and Explora.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.9.4.1 2009/05/13 17:17:41 jym Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_modular.h"
#include "opt_virtex.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/malloc.h>
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
#include <sys/ksyms.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <dev/cons.h>

#include <machine/bus.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <powerpc/spr.h>

#include <evbppc/virtex/dcr.h>
#include <evbppc/virtex/virtex.h>

#include "ksyms.h"

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#if defined(KGDB)
#include <sys/kgdb.h>
#endif

/*
 * Global variables used here and there
 */
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

/*
 * This should probably be in autoconf!				XXX
 */
char cpu_model[80];
char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

extern struct user *proc0paddr;

char bootpath[256];
paddr_t msgbuf_paddr;
vaddr_t msgbuf_vaddr;

#if NKSYMS || defined(DDB) || defined(MODULAR)
void *startsym, *endsym;
#endif

int lcsplx(int);
void initppc(u_int, u_int);

static void dumpsys(void);
static void install_extint(void (*)(void));
static void trapcpy(int, void *, size_t);


/* These two live in powerpc/powerpc/trap_subr.S */
extern int 		extint, extsize;
extern u_long 		extint_call;

extern int defaulttrap, defaultsize;
extern int sctrap, scsize;
extern int alitrap, alisize;
extern int dsitrap, dsisize;
extern int isitrap, isisize;
extern int mchktrap, mchksize;
extern int tlbimiss4xx, tlbim4size;
extern int tlbdmiss4xx, tlbdm4size;
extern int pitfitwdog, pitfitwdogsize;
extern int debugtrap, debugsize;
extern int errata51handler, errata51size;
#ifdef DDB
extern int ddblow, ddbsize;
#endif
#ifdef IPKDB
extern int ipkdblow, ipkdbsize;
#endif

/* BSS segment start & end. */
extern char 		edata[], end[];

/* One region holds all memory, the other is terminator expected by 405 pmap. */
#define MEMREGIONS 	2
struct mem_region 	physmemr[MEMREGIONS];
struct mem_region 	availmemr[MEMREGIONS];

static struct {
	int 	vector;
	void 	*addr;
	void 	*size;
} traps[] = {
	/* EXC_EXI handled by install_extint(). */
	{ EXC_SC, 	&sctrap, 	&scsize 	},
	{ EXC_ALI, 	&alitrap, 	&alisize 	},
	{ EXC_DSI, 	&dsitrap, 	&dsisize 	},
	{ EXC_ISI, 	&isitrap, 	&isisize 	},
	{ EXC_MCHK, 	&mchktrap, 	&mchksize 	},
	{ EXC_ITMISS, 	&tlbimiss4xx, 	&tlbim4size 	},
	{ EXC_DTMISS, 	&tlbdmiss4xx, 	&tlbdm4size 	},

	/* EXC_{PIT, FIT, WDOG} handlers are spaced by 0x10 bytes only.. */
	{ EXC_PIT, 	&pitfitwdog, 	&pitfitwdogsize },
	{ EXC_DEBUG, 	&debugtrap, 	&debugsize	},

	/* PPC405GP Rev D errata item 51 */	
	{ EXC_DTMISS|EXC_ALI, &errata51handler, &errata51size },

#if defined(DDB)
	{ EXC_PGM, 	&ddblow, 	&ddbsize 	},
#elif defined(IPKDB)
	{ EXC_PGM, 	&ipkdblow, 	&ipkdbsize 	},
#endif
};

/* Maximum TLB page size. */
#define TLB_PG_SIZE 	(16*1024*1024)

void
initppc(u_int startkernel, u_int endkernel)
{
	struct cpu_info * const ci = curcpu();
	u_int 			addr, memsize;
	int 			exc, dbcr0, i;

        /* Initialize cache info for memcpy, memset, etc. */
        cpu_probe_cache();

	memset(physmemr, 0, sizeof(physmemr)); 		/* [1].size = 0 */
	memset(availmemr, 0, sizeof(availmemr)); 	/* [1].size = 0 */

	memsize = (PHYSMEM * 1024 * 1024) & ~PGOFSET;

	physmemr[0].start 	= 0;
	physmemr[0].size 	= memsize;

	availmemr[0].start 	= startkernel;
	availmemr[0].size 	= memsize - availmemr[0].start;

	/* Map kernel memory linearly. */
	for (addr = 0; addr < endkernel; addr += TLB_PG_SIZE)
		ppc4xx_tlb_reserve(addr, addr, TLB_PG_SIZE, TLB_EX);

	/* Give design-specific code a hint for reserved mappings. */
	virtex_machdep_init(roundup(memsize, TLB_PG_SIZE), TLB_PG_SIZE,
	    physmemr, availmemr);

	lwp0.l_cpu = ci;
	lwp0.l_addr = proc0paddr;
	memset(lwp0.l_addr, 0, sizeof(*lwp0.l_addr));

	curpcb = &proc0paddr->u_pcb;
	curpcb->pcb_pm = pmap_kernel();

	for (exc = EXC_RSVD; exc <= EXC_LAST; exc += 0x100)
		trapcpy(exc, &defaulttrap, (size_t)&defaultsize);

	for (i = 0; i < __arraycount(traps); i++)
		trapcpy(traps[i].vector, traps[i].addr, (size_t)traps[i].size);

	__syncicache((void *)EXC_RST, EXC_LAST - EXC_RST + 0x100);

	/* set exception vector base */
	mtspr(SPR_EVPR, 0);

	/* handle trap instruction as PGM exception */
	dbcr0 = mfspr(SPR_DBCR0) & ~DBCR0_TDE;
	mtspr(SPR_DBCR0, dbcr0);

	install_extint(ext_intr);

	/* enable translation */
	__asm volatile (
	    "	mfmsr %0 	\n"
	    "	ori %0,%0,%1 	\n"
	    "	mtmsr %0 	\n"
	    "   sync 		\n"
	    "	isync		\n"
	    : : "r"(0), "K"(PSL_IR | PSL_DR | PSL_ME)); 

	/* now that we enabled MMU, we can map console */
	consinit();

	uvm_setpagesize();
	pmap_bootstrap(startkernel, endkernel);

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
#endif
#ifdef DDB
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
#ifdef KGDB
	/*
	 * Now trap to KGDB
	 */
	kgdb_connect(1);
#endif /* KGDB */
}

/*
 * trapcpy:
 *
 * 	Install a trap vector. We cannot use memcpy because the
 * 	destination may be zero. Borrowed from Explora.
 */
static void
trapcpy(int dest, void *src, size_t len)
{
	uint32_t 		*dest_p = (void *)dest;
	uint32_t 		*src_p = src;

	while (len > 0) {
		*dest_p++ = *src_p++;
		len -= sizeof(uint32_t);
	}
}

static void
install_extint(void (*handler)(void))
{
	u_long 			offset = (u_long)handler - (u_long)&extint_call;

#ifdef DIAGNOSTIC
	if (offset > 0x1ffffff)
		panic("install_extint: too far away");
#endif

	/* Patch branch target in EXC_EXI handler. */
	extint_call = (extint_call & 0xfc000003) | offset;

	memcpy((void *)EXC_EXI, &extint, (size_t)&extsize);
	__syncicache((void *)&extint_call, sizeof(extint_call));
	__syncicache((void *)EXC_EXI, (int)&extsize);
}

/*
 * Machine dependent startup code.
 */

char msgbuf[MSGBUFSIZE];

void
cpu_startup(void)
{
	/* For use by propdb. */
	static u_int 	memsize = PHYSMEM * 1024 * 1024;
	static u_int 	cpuspeed = CPUFREQ * 1000 * 1000;
	prop_number_t 	pn;

	vaddr_t 	minaddr, maxaddr;
	char 		pbuf[9];

	curcpu()->ci_khz = cpuspeed / 1000;

	/* Initialize error message buffer. */
	initmsgbuf((void *)msgbuf, round_page(MSGBUFSIZE));

	printf("%s%s", copyright, version);

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Set up the board properties database.
	 */
	board_info_init();

	pn = prop_number_create_integer(memsize);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "mem-size", pn) == false)
		panic("setting mem-size");
	prop_object_release(pn);

	pn = prop_number_create_integer(cpuspeed);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "processor-frequency",
	    pn) == false)
		panic("setting processor-frequency");
	prop_object_release(pn);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();
	fake_mapiodev = 0;
}


static void
dumpsys(void)
{
	printf("dumpsys: TBD\n");
}

/* Hook used by 405 pmap module. */
void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{
	*mem 	= physmemr;
	*avail 	= availmemr;
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
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

	if (!cold && (howto & RB_DUMP))
		dumpsys();

	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		/* Power off here if we know how...*/
	}

	if (howto & RB_HALT) {
		printf("halted\n\n");

		goto reboot;	/* XXX for now... */

#ifdef DDB
		printf("dropping to debugger\n");
		while(1)
			Debugger();
#endif
#ifdef KGDB
		printf("dropping to kgdb\n");
		while(1)
			kgdb_connect(1);
#endif
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

 reboot:
	ppc4xx_reset();

	printf("ppc4xx_reset() failed!\n");
#ifdef DDB
	while(1)
		Debugger();
#endif
#ifdef KGDB
	while(1)
		kgdb_connect(1);
#else
	while (1)
		/* nothing */;
#endif
}

int
lcsplx(int ipl)
{
	return spllower(ipl); 	/* XXX */
}
