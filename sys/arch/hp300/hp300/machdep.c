/*	$NetBSD: machdep.c,v 1.158 2002/05/23 03:05:30 gmcgarry Exp $	*/

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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.158 2002/05/23 03:05:30 gmcgarry Exp $");                                                  

#include "opt_ddb.h"
#include "opt_compat_hpux.h"
#include "opt_compat_netbsd.h"
#include "hil.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/buf.h>
#include <sys/clist.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/vnode.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifdef __ELF__
#include <sys/exec_elf.h>
#endif
#endif /* DDB */

#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/hp300spu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <dev/cons.h>

#define	MAXMEM	64*1024	/* XXX - from cmap.h */
#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include "opt_useleds.h"

#include <hp300/dev/hilreg.h>
#include <hp300/dev/hilioctl.h>
#include <hp300/dev/hilvar.h>
#ifdef USELEDS
#include <hp300/hp300/leds.h>
#endif

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

struct vm_map *exec_map = NULL;  
struct vm_map *mb_map = NULL;
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

caddr_t	msgbufaddr;
int	maxmem;			/* max memory per process */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

extern	u_int lowram;
extern	short exframesize[];

#ifdef COMPAT_HPUX
extern struct emul emul_hpux;
#endif

/* prototypes for local functions */
void	parityenable __P((void));
int	parityerror __P((struct frame *));
int	parityerrorfind __P((void));
void    identifycpu __P((void));
void    initcpu __P((void));

int	cpu_dumpsize __P((void));
int	cpu_dump __P((int (*)(dev_t, daddr_t, caddr_t, size_t), daddr_t *));
void	cpu_init_kcore_hdr __P((void));

/* functions called from locore.s */
void    dumpsys __P((void));
void	hp300_init __P((void));
void    straytrap __P((int, u_short));
void	nmihand __P((struct frame));

/*
 * Machine-dependent crash dump header info.
 */
cpu_kcore_hdr_t cpu_kcore_hdr;

/*
 * Note that the value of delay_divisor is roughly
 * 2048 / cpuspeed (where cpuspeed is in MHz) on 68020
 * and 68030 systems.  See clock.c for the delay
 * calibration algorithm.
 */
int	cpuspeed;		/* relative cpu speed; XXX skewed on 68040 */
int	delay_divisor;		/* delay constant */

/*
 * Early initialization, before main() is called.
 */
void
hp300_init()
{
	struct btinfo_magic *bt_mag;
	int i;

	extern paddr_t avail_start, avail_end;

	/*
	 * Tell the VM system about available physical memory.  The
	 * hp300 only has one segment.
	 */
	uvm_page_physload(atop(avail_start), atop(avail_end),
	    atop(avail_start), atop(avail_end), VM_FREELIST_DEFAULT);

	/* Initialize the interrupt handlers. */
	intr_init();

	/* Calibrate the delay loop. */
	hp300_calibrate_delay();

	/*
	 * Initialize error message buffer (at end of core).
	 * avail_end was pre-decremented in pmap_bootstrap to compensate.
	 */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_kenter_pa((vaddr_t)msgbufaddr + i * NBPG,
		    avail_end + i * NBPG, VM_PROT_READ|VM_PROT_WRITE);
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));

	/*
	 * Map in the bootinfo page, and make sure the bootinfo
	 * exists by searching for the MAGIC record.  If it's not
	 * there, disable bootinfo.
	 */
	pmap_enter(pmap_kernel(), bootinfo_va, bootinfo_pa,
	    VM_PROT_READ|VM_PROT_WRITE,
	    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	pmap_update(pmap_kernel());
	bt_mag = lookup_bootinfo(BTINFO_MAGIC);
	if (bt_mag == NULL ||
	    bt_mag->magic1 != BOOTINFO_MAGIC1 ||
	    bt_mag->magic2 != BOOTINFO_MAGIC2) {
		pmap_remove(pmap_kernel(), bootinfo_va, bootinfo_va + NBPG);
		pmap_update(pmap_kernel());
		bootinfo_va = 0;
	}
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
	extern struct map extiomap[];

	/*
	 * Initialize the DIO resource map.
	 */
	rminit(extiomap, (long)EIOMAPSIZE, (long)1, "extio", EIOMAPSIZE/16);

	/*
	 * Initialize the console before we print anything out.
	 */

	hp300_cninit();

	/*
	 * Issue a warning if the boot loader didn't provide bootinfo.
	 */
	if (bootinfo_va == 0)
		printf("WARNING: boot loader did not provide bootinfo\n");

#ifdef DDB
	{
		extern int end;
		extern int *esym;

#ifndef __ELF__
		ddb_init(*(int *)&end, ((int *)&end) + 1, esym);
#else 
		ddb_init((int)esym - (int)&end - sizeof(Elf32_Ehdr),
		    (void *)&end, esym);
#endif
	}
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu
 */
void
cpu_startup()
{
	extern char *etext;
	unsigned i;
	caddr_t v;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * Initialize the kernel crash dump header.
	 */
	cpu_init_kcore_hdr();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Find out how much space we need, allocate it,
	 * and the give everything true virtual addresses.
	 */
	size = (vsize_t)allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(size))) == 0)
		panic("startup: no room for tables");
	if ((allocsys(v, NULL) - v) != size)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
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
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

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

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 nmbclusters * mclbytes, VM_MAP_INTRSAFE,
				 FALSE, NULL);

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Tell the VM system that page 0 isn't mapped.
	 *
	 * XXX This is bogus; should just fix KERNBASE and
	 * XXX VM_MIN_KERNEL_ADDRESS, but not right now.
	 */
	if (uvm_map_protect(kernel_map, 0, NBPG, UVM_PROT_NONE, TRUE) != 0)
		panic("can't mark page 0 off-limits");

	/*
	 * Tell the VM system that writing to kernel text isn't allowed.
	 * If we don't, we might end up COW'ing the text segment!
	 *
	 * XXX Should be m68k_trunc_page(&kernel_text) instead
	 * XXX of NBPG.
	 */
	if (uvm_map_protect(kernel_map, NBPG, m68k_round_page(&etext),
	    UVM_PROT_READ|UVM_PROT_EXEC, TRUE) != 0)
		panic("can't protect kernel text");

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
}

/*
 * Set registers on exec.
 */
void
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;

	frame->f_sr = PSL_USERSET;
	frame->f_pc = pack->ep_entry & ~1;
	frame->f_regs[D0] = 0;
	frame->f_regs[D1] = 0;
	frame->f_regs[D2] = 0;
	frame->f_regs[D3] = 0;
	frame->f_regs[D4] = 0;
	frame->f_regs[D5] = 0;
	frame->f_regs[D6] = 0;
	frame->f_regs[D7] = 0;
	frame->f_regs[A0] = 0;
	frame->f_regs[A1] = 0;
	frame->f_regs[A2] = (int)p->p_psstr;
	frame->f_regs[A3] = 0;
	frame->f_regs[A4] = 0;
	frame->f_regs[A5] = 0;
	frame->f_regs[A6] = 0;
	frame->f_regs[SP] = stack;

	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	if (fputype)
		m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
}

/*
 * Info for CTL_HW
 */
char	cpu_model[120];

struct hp300_model {
	int id;
	int mmuid;
	const char *name;
	const char *speed;
};

const struct hp300_model hp300_models[] = {
	{ HP_320,	-1,		"320",		"16.67"	},
	{ HP_330,	-1,		"318/319/330",	"16.67"	},
	{ HP_340,	-1,		"340",		"16.67"	},
	{ HP_345,	-1,		"345",		"50"	},
	{ HP_350,	-1,		"350",		"25"	},
	{ HP_360,	-1,		"360",		"25"	},
	{ HP_370,	-1,		"370",		"33.33"	},
	{ HP_375,	-1,		"375",		"50"	},
	{ HP_380,	-1,		"380",		"25"	},
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

void
identifycpu()
{
	const char *t, *mc, *s;
	int i, len;

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

	sprintf(cpu_model, "HP 9000/%s (%sMHz MC680%s CPU", t, s, mc);

	/*
	 * ...and the MMU type.
	 */
	switch (mmutype) {
	case MMU_68040:
	case MMU_68030:
		strcat(cpu_model, "+MMU");
		break;
	case MMU_68851:
		strcat(cpu_model, ", MC68851 MMU");
		break;
	case MMU_HP:
		strcat(cpu_model, ", HP MMU");
		break;
	default:
		printf("%s\nunknown MMU type %d\n", cpu_model, mmutype);
		panic("startup");
	}

	len = strlen(cpu_model);

	/*
	 * ...and the FPU type.
	 */
	switch (fputype) {
	case FPU_68040:
		len += sprintf(cpu_model + len, "+FPU");
		break;
	case FPU_68882:
		len += sprintf(cpu_model + len, ", %sMHz MC68882 FPU", s);
		break;
	case FPU_68881:
		len += sprintf(cpu_model + len, ", %sMHz MC68881 FPU",
		    machineid == HP_350 ? "20" : "16.67");
		break;
	default:
		len += sprintf(cpu_model + len, ", unknown FPU");
	}

	/*
	 * ...and finally, the cache type.
	 */
	if (cputype == CPU_68040)
		sprintf(cpu_model + len, ", 4k on-chip physical I/D caches");
	else {
		switch (ectype) {
		case EC_VIRT:
			sprintf(cpu_model + len,
			    ", %dK virtual-address cache",
			    machineid == HP_320 ? 16 : 32);
			break;
		case EC_PHYS:
			sprintf(cpu_model + len,
			    ", %dK physical-address cache",
			    machineid == HP_370 ? 64 : 32);
			break;
		}
	}

	strcat(cpu_model, ")");
	printf("%s\n", cpu_model);
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
#if !defined(HP370)
	case HP_370:
#endif
#if !defined(HP375)
	case HP_375:
#endif
#if !defined(HP380)
	case HP_380:
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
	dev_t consdev;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

int	waittime = -1;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{

#if __GNUC__	/* XXX work around lame compiler problem (gcc 2.7.2) */
	(void)&howto;
#endif
	/* take a snap shot before clobbering any registers */
	if (curproc && curproc->p_addr)
		savectx(&curproc->p_addr->u_pcb);

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
	/*NOTREACHED*/
}

/*
 * Initialize the kernel crash dump header.
 */
void
cpu_init_kcore_hdr()
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	extern int end;

	memset(&cpu_kcore_hdr, 0, sizeof(cpu_kcore_hdr));

	/*
	 * Initialize the `dispatcher' portion of the header.
	 */
	strcpy(h->name, machine);
	h->page_size = NBPG;
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
	m->sysseg_pa = (u_int32_t)(pmap_kernel()->pm_stpa);

	/*
	 * Initialize relocation value such that:
	 *
	 *	pa = (va - KERNBASE) + reloc
	 */
	m->reloc = lowram;

	/*
	 * Define the end of the relocatable range.
	 */
	m->relocend = (u_int32_t)&end;

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
int
cpu_dumpsize()
{
	int size;

	size = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t));
	return (btodb(roundup(size, dbtob(1))));
}

/*
 * Called by dumpsys() to dump the machine-dependent header.
 */
int
cpu_dump(dump, blknop)
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	daddr_t *blknop;
{
	int buf[dbtob(1) / sizeof(int)];
	cpu_kcore_hdr_t *chdr;
	kcore_seg_t *kseg;
	int error;

	kseg = (kcore_seg_t *)buf;
	chdr = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(kcore_seg_t)) /
	    sizeof(int)];

	/* Create the segment header. */
	CORE_SETMAGIC(*kseg, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg->c_size = dbtob(1) - ALIGN(sizeof(kcore_seg_t));

	memcpy(chdr, &cpu_kcore_hdr, sizeof(cpu_kcore_hdr_t));
	error = (*dump)(dumpdev, *blknop, (caddr_t)buf, sizeof(buf));
	*blknop += btodb(sizeof(buf));
	return (error);
}

/*
 * These variables are needed by /sbin/savecore
 */
u_int32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first NBPG of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{
	int chdrsize;	/* size of dump header */
	int nblks;	/* size of dump area */
	int maj;

	if (dumpdev == NODEV)
		return;
	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdevsw[maj].d_psize == NULL)
		return;
	nblks = (*bdevsw[maj].d_psize)(dumpdev);
	chdrsize = cpu_dumpsize();

	dumpsize = btoc(cpu_kcore_hdr.un._m68k.ram_segs[0].size);

	/*
	 * Check do see if we will fit.  Note we always skip the
	 * first NBPG in case there is a disk label there.
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
dumpsys()
{
	daddr_t blkno;		/* current block to write */
				/* dump routine */
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int pg;			/* page being dumped */
	paddr_t maddr;		/* PA being dumped */
	int error;		/* error code from (*dump)() */

	/* XXX initialized here because of gcc lossage */
	maddr = lowram;
	pg = 0;

	/* Make sure dump device is valid. */
	if (dumpdev == NODEV)
		return;
	if (dumpsize == 0) {
		cpu_dumpconf();
		if (dumpsize == 0)
			return;
	}
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	dump = bdevsw[major(dumpdev)].d_dump;
	blkno = dumplo;

	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	printf("dump ");

	/* Write the dump header. */
	error = cpu_dump(dump, &blkno);
	if (error)
		goto bad;

	for (pg = 0; pg < dumpsize; pg++) {
#define NPGMB	(1024*1024/NBPG)
		/* print out how many MBs we have dumped */
		if (pg && (pg % NPGMB) == 0)
			printf("%d ", pg / NPGMB);
#undef NPGMB
		pmap_enter(pmap_kernel(), (vaddr_t)vmmap, maddr,
		    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);

		pmap_update(pmap_kernel());
		error = (*dump)(dumpdev, blkno, vmmap, NBPG);
 bad:
		switch (error) {
		case 0:
			maddr += NBPG;
			blkno += btodb(NBPG);
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

void
initcpu()
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
		mappedcopysize = NBPG;
#endif
	parityenable();
#ifdef USELEDS
	ledinit();
#endif
}

void
straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
}

/* XXX should change the interface, and make one badaddr() function */

int	*nofault;

int
badaddr(addr)
	caddr_t addr;
{
	int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile short *)addr;
	nofault = (int *) 0;
	return(0);
}

int
badbaddr(addr)
	caddr_t addr;
{
	int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile char *)addr;
	nofault = (int *) 0;
	return(0);
}

/*
 * lookup_bootinfo:
 *
 *	Look up information in bootinfo from boot loader.
 */
void *
lookup_bootinfo(type)
	int type;
{
	struct btinfo_common *bt;
	char *help = (char *)bootinfo_va;

	/* Check for a bootinfo record first. */
	if (help == NULL)
		return (NULL);

	do {
		bt = (struct btinfo_common *)help;
		if (bt->type == type)
			return (help);
		help += bt->next;
	} while (bt->next != 0 &&
		 (size_t)help < (size_t)bootinfo_va + BOOTINFO_SIZE);

	return (NULL);
}

#ifdef PANICBUTTON
/*
 * Declare these so they can be patched.
 */
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int candbdiv = 2;	/* give em half a second (hz / candbdiv) */

void	candbtimer __P((void *));

int crashandburn;

struct callout candbtimer_ch = CALLOUT_INITIALIZER;

void
candbtimer(arg)
	void *arg;
{

	crashandburn = 0;
}
#endif /* PANICBUTTON */

static int innmihand;	/* simple mutex */

/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
void
nmihand(frame)
	struct frame frame;
{

	/* Prevent unwanted recursion. */
	if (innmihand)
		return;
	innmihand = 1;

#if NHIL > 0
	/* Check for keyboard <CRTL>+<SHIFT>+<RESET>. */
	if (kbdnmi()) {
		printf("Got a keyboard NMI");

		/*
		 * We can:
		 *
		 *	- enter DDB
		 *
		 *	- Start the crashandburn sequence
		 *
		 *	- Ignore it.
		 */
#ifdef DDB
		printf(": entering debugger\n");
		Debugger();
#else
#ifdef PANICBUTTON
		if (panicbutton) {
			if (crashandburn) {
				crashandburn = 0;
				printf(": CRASH AND BURN!\n");
				panic("forced crash");
			} else {
				/* Start the crashandburn sequence */
				printf("\n");
				crashandburn = 1;
				callout_reset(&candbtimer_ch, hz / candbdiv,
				    candbtiner, NULL);
			}
		} else
#endif /* PANICBUTTON */
			printf(": ignoring\n");
#endif /* DDB */

		goto nmihand_out;	/* no more work to do */
	}
#endif

	if (parityerror(&frame))
		return;
	/* panic?? */
	printf("unexpected level 7 interrupt ignored\n");

#if NHIL > 0
nmihand_out:
	innmihand = 0;
#endif
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
void
parityenable()
{
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		printf("Parity detection disabled\n");
		return;
	}
	*PARREG = 1;
	nofault = (int *) 0;
	gotparmem = 1;
}

/*
 * Determine if level 7 interrupt was caused by a parity error
 * and deal with it if it was.  Returns 1 if it was a parity error.
 */
int
parityerror(fp)
	struct frame *fp;
{
	if (!gotparmem)
		return(0);
	*PARREG = 0;
	DELAY(10);
	*PARREG = 1;
	if (panicstr) {
		printf("parity error after panic ignored\n");
		return(1);
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
	return(1);
}

/*
 * Yuk!  There has got to be a better way to do this!
 * Searching all of memory with interrupts blocked can lead to disaster.
 */
int
parityerrorfind()
{
	static label_t parcatch;
	static int looking = 0;
	volatile int pg, o, s;
	volatile int *ip;
	int i;
	int found;

#ifdef lint
	i = o = pg = 0; if (i) return(0);
#endif
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
		for (o = 0; o < NBPG; o += sizeof(int))
			i = *ip++;
	}
	/*
	 * Getting here implies no fault was found.  Should never happen.
	 */
	printf("Couldn't locate parity error\n");
	found = 0;
done:
	looking = 0;
	pmap_remove(pmap_kernel(), (vaddr_t)vmmap, (vaddr_t)&vmmap[NBPG]);
	pmap_update(pmap_kernel());
	ecacheon();
	splx(s);
	return(found);
}

/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
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
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
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
		error = exec_aout_prep_oldzmagic(p, epp);
		return(error);
#endif
#ifdef COMPAT_44
	case (MID_HP300 << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(p, epp);
		return(error);
#endif
	}
#endif /* !(defined(COMPAT_NOMID) || defined(COMPAT_44)) */

	return ENOEXEC;
}
