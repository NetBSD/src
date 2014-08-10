/*	$NetBSD: machdep.c,v 1.228.2.1 2014/08/10 06:53:57 tls Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.228.2.1 2014/08/10 06:53:57 tls Exp $");

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_fpu_emulate.h"
#include "opt_modular.h"
#include "opt_panicbutton.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>		/* for MID_* */
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/tty.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/cpu.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif /* DDB */
#ifdef __ELF__
#include <sys/exec_elf.h>
#endif

#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/hp300spu.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <dev/cons.h>
#include <dev/mm.h>

#define	MAXMEM	64*1024	/* XXX - from cmap.h */
#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include "opt_useleds.h"

#ifdef USELEDS
#include <hp300/hp300/leds.h>
#endif

#include "ksyms.h"

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

struct vm_map *phys_map = NULL;

extern paddr_t avail_end;

/*
 * bootinfo base (physical and virtual).  The bootinfo is placed, by
 * the boot loader, into the first page of kernel text, which is zero
 * filled (see locore.s) and not mapped at 0.  It is remapped to a
 * different address in pmap_bootstrap().
 */
paddr_t	bootinfo_pa;
vaddr_t	bootinfo_va;

int	maxmem;			/* max memory per process */

extern	u_int lowram;
extern	short exframesize[];

/* prototypes for local functions */
static void	parityenable(void);
static int	parityerror(struct frame *);
static int	parityerrorfind(void);
static void	identifycpu(void);
static void	initcpu(void);

static int	cpu_dumpsize(void);
static int	cpu_dump(int (*)(dev_t, daddr_t, void *, size_t), daddr_t *);
static void	cpu_init_kcore_hdr(void);

/* functions called from locore.s */
void    dumpsys(void);
void	hp300_init(void);
void    straytrap(int, u_short);
void	nmihand(struct frame);

/*
 * Machine-dependent crash dump header info.
 */
static cpu_kcore_hdr_t cpu_kcore_hdr;

/*
 * Note that the value of delay_divisor is roughly
 * 2048 / cpuspeed (where cpuspeed is in MHz) on 68020
 * and 68030 systems.  See clock.c for the delay
 * calibration algorithm.
 */
int	cpuspeed;		/* relative CPU speed; XXX skewed on 68040 */
int	delay_divisor;		/* delay constant */

/*
 * Early initialization, before main() is called.
 */
void
hp300_init(void)
{
	struct btinfo_magic *bt_mag;
	int i;

	extern paddr_t avail_start, avail_end;

#ifdef CACHE_HAVE_VAC
	/*
	 * Determine VA aliasing distance if any
	 */
	switch (machineid) {
	case HP_320:
		pmap_aliasmask = 0x3fff;	/* 16KB */
		break;
	case HP_350:
		pmap_aliasmask = 0x7fff;	/* 32KB */
		break;
	default:
		break;
	}
#endif

	/*
	 * Tell the VM system about available physical memory.  The
	 * hp300 only has one segment.
	 */
	uvm_page_physload(atop(avail_start), atop(avail_end),
	    atop(avail_start), atop(avail_end), VM_FREELIST_DEFAULT);

	/* Calibrate the delay loop. */
	hp300_calibrate_delay();

	/*
	 * Initialize error message buffer (at end of core).
	 * avail_end was pre-decremented in pmap_bootstrap to compensate.
	 */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_kenter_pa((vaddr_t)msgbufaddr + i * PAGE_SIZE,
		    avail_end + i * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));

	/*
	 * Map in the bootinfo page, and make sure the bootinfo
	 * exists by searching for the MAGIC record.  If it's not
	 * there, disable bootinfo.
	 */
	bootinfo_va = virtual_avail;
	virtual_avail += PAGE_SIZE;
	pmap_enter(pmap_kernel(), bootinfo_va, bootinfo_pa,
	    VM_PROT_READ|VM_PROT_WRITE,
	    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	pmap_update(pmap_kernel());
	bt_mag = lookup_bootinfo(BTINFO_MAGIC);
	if (bt_mag == NULL ||
	    bt_mag->magic1 != BOOTINFO_MAGIC1 ||
	    bt_mag->magic2 != BOOTINFO_MAGIC2) {
		pmap_remove(pmap_kernel(), bootinfo_va,
		    bootinfo_va + PAGE_SIZE);
		pmap_update(pmap_kernel());
		virtual_avail -= PAGE_SIZE;
		bootinfo_va = 0;
	}
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{

	/*
	 * Initialize the external I/O extent map.
	 */
	iomap_init();

	/*
	 * Initialize the console before we print anything out.
	 */

	hp300_cninit();

	/*
	 * Issue a warning if the boot loader didn't provide bootinfo.
	 */
	if (bootinfo_va != 0)
		printf("bootinfo found at 0x%08lx\n", bootinfo_pa);
	else
		printf("WARNING: boot loader did not provide bootinfo\n");

#if NKSYMS || defined(DDB) || defined(MODULAR)
	{
		extern int end;
		extern int *esym;

		ksyms_addsyms_elf((int)esym - (int)&end - sizeof(Elf32_Ehdr),
		    (void *)&end, esym);
	}
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	hp300_cninit_deferred();

	if (fputype != FPU_NONE)
		m68k_make_fpu_idle_frame();

	/*
	 * Initialize the kernel crash dump header.
	 */
	cpu_init_kcore_hdr();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	identifycpu();
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, false, NULL);

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/* Safe to use malloc for extio_ex now. */
	extio_ex_malloc_safe = 1;
}

struct hp300_model {
	int id;
	int mmuid;
	const char *name;
	const char *speed;
};

static const struct hp300_model hp300_models[] = {
	{ HP_320,	-1,		"320",		"16.67"	},
	{ HP_330,	-1,		"318/319/330",	"16.67"	},
	{ HP_340,	-1,		"340",		"16.67"	},
	{ HP_345,	-1,		"345",		"50"	},
	{ HP_350,	-1,		"350",		"25"	},
	{ HP_360,	-1,		"360",		"25"	},
	{ HP_362,	-1,		"362",		"25"	},
	{ HP_370,	-1,		"370",		"33.33"	},
	{ HP_375,	-1,		"375",		"50"	},
	{ HP_380,	-1,		"380",		"25"	},
	{ HP_382,	-1,		"382",		"25"	},
	{ HP_385,	-1,		"385",		"33"	},
	{ HP_400,	-1,		"400",		"50"	},
	{ HP_425,	MMUID_425_T,	"425t",		"25"	},
	{ HP_425,	MMUID_425_S,	"425s",		"25"	},
	{ HP_425,	MMUID_425_E,	"425e",		"25"	},
	{ HP_425,	-1,		"425",		"25"	},
	{ HP_433,	MMUID_433_T,	"433t",		"33"	},
	{ HP_433,	MMUID_433_S,	"433s",		"33"	},
	{ HP_433,	-1,		"433",		"33"	},
	{ 0,		-1,		NULL,		NULL	},
};

static void
identifycpu(void)
{
	const char *t, *mc, *s, *mmu;
	int i; 
	char fpu[64], cache[64];

	/*
	 * Find the model number.
	 */
	for (t = s = NULL, i = 0; hp300_models[i].name != NULL; i++) {
		if (hp300_models[i].id == machineid) {
			if (hp300_models[i].mmuid != -1 &&
			    hp300_models[i].mmuid != mmuid)
				continue;
			t = hp300_models[i].name;
			s = hp300_models[i].speed;
			break;
		}
	}
	if (t == NULL) {
		printf("\nunknown machineid %d\n", machineid);
		goto lose;
	}

	/*
	 * ...and the CPU type.
	 */
	switch (cputype) {
	case CPU_68040:
		mc = "40";
		break;
	case CPU_68030:
		mc = "30";
		break;
	case CPU_68020:
		mc = "20";
		break;
	default:
		printf("\nunknown cputype %d\n", cputype);
		goto lose;
	}


	/*
	 * ...and the MMU type.
	 */
	switch (mmutype) {
	case MMU_68040:
	case MMU_68030:
		mmu = "+MMU";
		break;
	case MMU_68851:
		mmu = ", MC68851 MMU";
		break;
	case MMU_HP:
		mmu = ", HP MMU";
		break;
	default:
		printf("MC680%s\nunknown MMU type %d\n", mc, mmutype);
		panic("startup");
	}

	/*
	 * ...and the FPU type.
	 */
	switch (fputype) {
	case FPU_68040:
		strlcpy(fpu, "+FPU", sizeof(fpu));
		break;
	case FPU_68882:
		snprintf(fpu, sizeof(fpu), ", %sMHz MC68882 FPU", s);
		break;
	case FPU_68881:
		snprintf(fpu, sizeof(fpu), ", %sMHz MC68881 FPU",
		    machineid == HP_350 ? "20" : "16.67");
		break;
	case FPU_NONE:
#ifdef FPU_EMULATE
		strlcpy(fpu, ", emulated FPU", sizeof(fpu));
#else
		strlcpy(fpu, ", no FPU", sizeof(fpu));
#endif
		break;
	default:
		strlcpy(fpu, ", unknown FPU", sizeof(fpu));
	}

	/*
	 * ...and finally, the cache type.
	 */
	if (cputype == CPU_68040)
		snprintf(cache, sizeof(cache),
		    ", 4k on-chip physical I/D caches");
	else {
		switch (ectype) {
		case EC_VIRT:
			snprintf(cache, sizeof(cache),
			    ", %dK virtual-address cache",
			    machineid == HP_320 ? 16 : 32);
			break;
		case EC_PHYS:
			snprintf(cache, sizeof(cache),
			    ", %dK physical-address cache",
			    machineid == HP_370 ? 64 : 32);
			break;
		}
	}

	cpu_setmodel("HP 9000/%s (%sMHz MC680%s CPU%s%s%s)", t, s, mc,
	    mmu, fpu, cache);
	printf("%s\n", cpu_getmodel());
#ifdef DIAGNOSTIC
	printf("cpu: delay divisor %d", delay_divisor);
	if (mmuid)
		printf(", mmuid %d", mmuid);
	printf("\n");
#endif

	/*
	 * Now that we have told the user what they have,
	 * let them know if that machine type isn't configured.
	 */
	switch (machineid) {
	case -1:		/* keep compilers happy */
#if !defined(HP320)
	case HP_320:
#endif
#if !defined(HP330)
	case HP_330:
#endif
#if !defined(HP340)
	case HP_340:
#endif
#if !defined(HP345)
	case HP_345:
#endif
#if !defined(HP350)
	case HP_350:
#endif
#if !defined(HP360)
	case HP_360:
#endif
#if !defined(HP362)
	case HP_362:
#endif
#if !defined(HP370)
	case HP_370:
#endif
#if !defined(HP375)
	case HP_375:
#endif
#if !defined(HP380)
	case HP_380:
#endif
#if !defined(HP382)
	case HP_382:
#endif
#if !defined(HP385)
	case HP_385:
#endif
#if !defined(HP400)
	case HP_400:
#endif
#if !defined(HP425)
	case HP_425:
#endif
#if !defined(HP433)
	case HP_433:
#endif
		panic("SPU type not configured");
	default:
		break;
	}

	return;
 lose:
	panic("startup");
}

/*
 * machine dependent system variables.
 */
SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_STRUCT, "console_device", NULL,
	    sysctl_consdev, 0, NULL, sizeof(dev_t),
	    CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
}

int	waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{
	struct pcb *pcb = lwp_getpcb(curlwp);

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL)
		savectx(pcb);

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested do it. */
	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

#if defined(PANICWAIT) && !defined(DDB)
	if ((howto & RB_HALT) == 0 && panicstr) {
		printf("hit any key to reboot...\n");
		(void)cngetc();
		printf("\n");
	}
#endif

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		printf("System halted.  Hit any key to reboot.\n\n");
		(void)cngetc();
	}

	printf("rebooting...\n");
	DELAY(1000000);
	doboot();
	/* NOTREACHED */
}

/*
 * Initialize the kernel crash dump header.
 */
static void
cpu_init_kcore_hdr(void)
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	extern int end;

	memset(&cpu_kcore_hdr, 0, sizeof(cpu_kcore_hdr));

	/*
	 * Initialize the `dispatcher' portion of the header.
	 */
	strcpy(h->name, machine);
	h->page_size = PAGE_SIZE;
	h->kernbase = KERNBASE;

	/*
	 * Fill in information about our MMU configuration.
	 */
	m->mmutype	= mmutype;
	m->sg_v		= SG_V;
	m->sg_frame	= SG_FRAME;
	m->sg_ishift	= SG_ISHIFT;
	m->sg_pmask	= SG_PMASK;
	m->sg40_shift1	= SG4_SHIFT1;
	m->sg40_mask2	= SG4_MASK2;
	m->sg40_shift2	= SG4_SHIFT2;
	m->sg40_mask3	= SG4_MASK3;
	m->sg40_shift3	= SG4_SHIFT3;
	m->sg40_addr1	= SG4_ADDR1;
	m->sg40_addr2	= SG4_ADDR2;
	m->pg_v		= PG_V;
	m->pg_frame	= PG_FRAME;

	/*
	 * Initialize pointer to kernel segment table.
	 */
	m->sysseg_pa = (uint32_t)(pmap_kernel()->pm_stpa);

	/*
	 * Initialize relocation value such that:
	 *
	 *	pa = (va - KERNBASE) + reloc
	 */
	m->reloc = lowram;

	/*
	 * Define the end of the relocatable range.
	 */
	m->relocend = (uint32_t)&end;

	/*
	 * hp300 has one contiguous memory segment.
	 */
	m->ram_segs[0].start = lowram;
	m->ram_segs[0].size  = ctob(physmem);
}

/*
 * Compute the size of the machine-dependent crash dump header.
 * Returns size in disk blocks.
 */

#define CHDRSIZE (ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)))
#define MDHDRSIZE roundup(CHDRSIZE, dbtob(1))

static int
cpu_dumpsize(void)
{

	return btodb(MDHDRSIZE);
}

/*
 * Called by dumpsys() to dump the machine-dependent header.
 */
static int
cpu_dump(int (*dump)(dev_t, daddr_t, void *, size_t), daddr_t *blknop)
{
	int buf[MDHDRSIZE / sizeof(int)];
	cpu_kcore_hdr_t *chdr;
	kcore_seg_t *kseg;
	int error;

	kseg = (kcore_seg_t *)buf;
	chdr = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(kcore_seg_t)) /
	    sizeof(int)];

	/* Create the segment header. */
	CORE_SETMAGIC(*kseg, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg->c_size = MDHDRSIZE - ALIGN(sizeof(kcore_seg_t));

	memcpy(chdr, &cpu_kcore_hdr, sizeof(cpu_kcore_hdr_t));
	error = (*dump)(dumpdev, *blknop, (void *)buf, sizeof(buf));
	*blknop += btodb(sizeof(buf));
	return error;
}

/*
 * These variables are needed by /sbin/savecore
 */
uint32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first PAGE_SIZE of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	int chdrsize;	/* size of dump header */
	int nblks;	/* size of dump area */

	if (dumpdev == NODEV)
		return;
	nblks = bdev_size(dumpdev);
	chdrsize = cpu_dumpsize();

	dumpsize = btoc(cpu_kcore_hdr.un._m68k.ram_segs[0].size);

	/*
	 * Check do see if we will fit.  Note we always skip the
	 * first PAGE_SIZE in case there is a disk label there.
	 */
	if (nblks < (ctod(dumpsize) + chdrsize + ctod(1))) {
		dumpsize = 0;
		dumplo = -1;
		return;
	}

	/*
	 * Put dump at the end of the partition.
	 */
	dumplo = (nblks - 1) - ctod(dumpsize) - chdrsize;
}

/*
 * Dump physical memory onto the dump device.  Called by cpu_reboot().
 */
void
dumpsys(void)
{
	const struct bdevsw *bdev;
	daddr_t blkno;		/* current block to write */
				/* dump routine */
	int (*dump)(dev_t, daddr_t, void *, size_t);
	int pg;			/* page being dumped */
	paddr_t maddr;		/* PA being dumped */
	int error;		/* error code from (*dump)() */

	/* XXX initialized here because of gcc lossage */
	maddr = lowram;
	pg = 0;

	/* Make sure dump device is valid. */
	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return;
	if (dumpsize == 0) {
		cpu_dumpconf();
		if (dumpsize == 0)
			return;
	}
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n",
		    major(dumpdev), minor(dumpdev));
		return;
	}
	dump = bdev->d_dump;
	blkno = dumplo;

	printf("\ndumping to dev %u,%u offset %ld\n",
	    major(dumpdev), minor(dumpdev), dumplo);

	printf("dump ");

	/* Write the dump header. */
	error = cpu_dump(dump, &blkno);
	if (error)
		goto bad;

	for (pg = 0; pg < dumpsize; pg++) {
#define NPGMB	(1024*1024/PAGE_SIZE)
		/* print out how many MBs we have dumped */
		if (pg && (pg % NPGMB) == 0)
			printf("%d ", pg / NPGMB);
#undef NPGMB
		pmap_enter(pmap_kernel(), (vaddr_t)vmmap, maddr,
		    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);

		pmap_update(pmap_kernel());
		error = (*dump)(dumpdev, blkno, vmmap, PAGE_SIZE);
 bad:
		switch (error) {
		case 0:
			maddr += PAGE_SIZE;
			blkno += btodb(PAGE_SIZE);
			break;

		case ENXIO:
			printf("device bad\n");
			return;

		case EFAULT:
			printf("device not ready\n");
			return;

		case EINVAL:
			printf("area improper\n");
			return;

		case EIO:
			printf("i/o error\n");
			return;

		case EINTR:
			printf("aborted from console\n");
			return;

		default:
			printf("error %d\n", error);
			return;
		}
	}
	printf("succeeded\n");
}

static void
initcpu(void)
{

#ifdef MAPPEDCOPY
	/*
	 * Initialize lower bound for doing copyin/copyout using
	 * page mapping (if not already set).  We don't do this on
	 * VAC machines as it loses big time.
	 */
	if (ectype == EC_VIRT)
		mappedcopysize = -1;	/* in case it was patched */
	else
		mappedcopysize = PAGE_SIZE;
#endif
	parityenable();
#ifdef USELEDS
	ledinit();
#endif
}

void
straytrap(int pc, u_short evec)
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
}

/* XXX should change the interface, and make one badaddr() function */

int	*nofault;

int
badaddr(void *addr)
{
	int i;
	label_t	faultbuf;

	nofault = (int *)&faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *)0;
		return 1;
	}
	i = *(volatile short *)addr;
	__USE(i);
	nofault = (int *)0;
	return 0;
}

int
badbaddr(void *addr)
{
	int i;
	label_t	faultbuf;

	nofault = (int *)&faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *)0;
		return 1;
	}
	i = *(volatile char *)addr;
	__USE(i);
	nofault = (int *) 0;
	return 0;
}

/*
 * lookup_bootinfo:
 *
 *	Look up information in bootinfo from boot loader.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	char *help = (char *)bootinfo_va;

	/* Check for a bootinfo record first. */
	if (help == NULL)
		return NULL;

	do {
		bt = (struct btinfo_common *)help;
		if (bt->type == type)
			return help;
		help += bt->next;
	} while (bt->next != 0 &&
		 (size_t)help < (size_t)bootinfo_va + BOOTINFO_SIZE);

	return NULL;
}

#if defined(PANICBUTTON) && !defined(DDB)
/*
 * Declare these so they can be patched.
 */
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int candbdiv = 2;	/* give em half a second (hz / candbdiv) */

static void	candbtimer(void *);

int crashandburn;

callout_t candbtimer_ch;

void
candbtimer(void *arg)
{

	crashandburn = 0;
}
#endif /* PANICBUTTON & !DDB */

static int innmihand;	/* simple mutex */

/*
 * Level 7 interrupts can be caused by HIL keyboards (in cooked mode only,
 * but we run them in raw mode) or parity errors.
 */
void
nmihand(struct frame frame)
{

	/* Prevent unwanted recursion. */
	if (innmihand)
		return;
	innmihand = 1;

	if (parityerror(&frame))
		return;
	/* panic?? */
	printf("unexpected level 7 interrupt ignored\n");

	innmihand = 0;
}

/*
 * Parity error section.  Contains magic.
 */
#define PARREG		((volatile short *)IIOV(0x5B0000))
static int gotparmem = 0;
#ifdef DEBUG
int ignorekperr = 0;	/* ignore kernel parity errors */
#endif

/*
 * Enable parity detection
 */
static void
parityenable(void)
{
	label_t	faultbuf;

	nofault = (int *)&faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *)0;
		printf("Parity detection disabled\n");
		return;
	}
	*PARREG = 1;
	nofault = (int *)0;
	gotparmem = 1;
}

/*
 * Determine if level 7 interrupt was caused by a parity error
 * and deal with it if it was.  Returns 1 if it was a parity error.
 */
static int
parityerror(struct frame *fp)
{
	if (!gotparmem)
		return 0;
	*PARREG = 0;
	DELAY(10);
	*PARREG = 1;
	if (panicstr) {
		printf("parity error after panic ignored\n");
		return 1;
	}
	if (!parityerrorfind())
		printf("WARNING: transient parity error ignored\n");
	else if (USERMODE(fp->f_sr)) {
		printf("pid %d: parity error\n", curproc->p_pid);
		uprintf("sorry, pid %d killed due to memory parity error\n",
		    curproc->p_pid);
		psignal(curproc, SIGKILL);
#ifdef DEBUG
	} else if (ignorekperr) {
		printf("WARNING: kernel parity error ignored\n");
#endif
	} else {
		regdump((struct trapframe *)fp, 128);
		panic("kernel parity error");
	}
	return 1;
}

/*
 * Yuk!  There has got to be a better way to do this!
 * Searching all of memory with interrupts blocked can lead to disaster.
 */
static int
parityerrorfind(void)
{
	static label_t parcatch;
	static int looking = 0;
	volatile int pg, o, s;
	volatile int *ip;
	int i;
	int found;

	/*
	 * If looking is true we are searching for a known parity error
	 * and it has just occurred.  All we do is return to the higher
	 * level invocation.
	 */
	if (looking)
		longjmp(&parcatch);
	s = splhigh();
	/*
	 * If setjmp returns true, the parity error we were searching
	 * for has just occurred (longjmp above) at the current pg+o
	 */
	if (setjmp(&parcatch)) {
		printf("Parity error at 0x%x\n", ctob(pg)|o);
		found = 1;
		goto done;
	}
	/*
	 * If we get here, a parity error has occurred for the first time
	 * and we need to find it.  We turn off any external caches and
	 * loop thru memory, testing every longword til a fault occurs and
	 * we regain control at setjmp above.  Note that because of the
	 * setjmp, pg and o need to be volatile or their values will be lost.
	 */
	looking = 1;
	ecacheoff();
	for (pg = btoc(lowram); pg < btoc(lowram)+physmem; pg++) {
		pmap_enter(pmap_kernel(), (vaddr_t)vmmap, ctob(pg),
		    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);
		pmap_update(pmap_kernel());
		ip = (int *)vmmap;
		for (o = 0; o < PAGE_SIZE; o += sizeof(int))
			i = *ip++;
	}
	__USE(i);
	/*
	 * Getting here implies no fault was found.  Should never happen.
	 */
	printf("Couldn't locate parity error\n");
	found = 0;
 done:
	looking = 0;
	pmap_remove(pmap_kernel(), (vaddr_t)vmmap, (vaddr_t)&vmmap[PAGE_SIZE]);
	pmap_update(pmap_kernel());
	ecacheon();
	splx(s);
	return found;
}

/*
 * cpu_exec_aout_makecmds():
 *	CPU-dependent a.out format hook for execve().
 *
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 * XXX what are the special cases for the hp300?
 * XXX why is this COMPAT_NOMID?  was something generating
 *	hp300 binaries with an a_mid of 0?  i thought that was only
 *	done on little-endian machines...  -- cgd
 */
int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
#if defined(COMPAT_NOMID) || defined(COMPAT_44)
	u_long midmag, magic;
	u_short mid;
	int error;
	struct exec *execp = epp->ep_hdr;

	midmag = ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0xffff;
	magic = midmag & 0xffff;

	midmag = mid << 16 | magic;

	switch (midmag) {
#ifdef COMPAT_NOMID
	case (MID_ZERO << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(l, epp);
		return error;
#endif
#ifdef COMPAT_44
	case (MID_HP300 << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(l, epp);
		return error;
#endif
	}
#endif /* !(defined(COMPAT_NOMID) || defined(COMPAT_44)) */

	return ENOEXEC;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	return (pa < lowram || pa >= 0xfffffffc) ? EFAULT : 0;
}

int
mm_md_kernacc(void *ptr, vm_prot_t prot, bool *handled)
{

	/*
	 * Do not allow reading intio or dio device space.  This could lead
	 * to corruption of device registers.
	 */
	*handled = false;
	return (ISIIOVA(ptr) || ((uint8_t *)ptr >= extiobase &&
	    (uint8_t *)ptr < extiobase + (EIOMAPSIZE * PAGE_SIZE)))
	    ? EFAULT : 0;
}

#ifdef MODULAR
/*
 * Push any modules loaded by the bootloader etc.
 */
void
module_init_md(void)
{
}
#endif
