/*	$NetBSD: machdep.c,v 1.84.2.2 2010/10/22 07:21:27 uebayasi Exp $	*/

/*
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
/*
 * Copyright (c) 1988 University of Utah.
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.84.2.2 2010/10/22 07:21:27 uebayasi Exp $");

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/tty.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>		/* for MID_* */
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/ksyms.h>
#include <sys/module.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif
#ifdef __ELF__
#include <sys/exec_elf.h>
#endif

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/pte.h>
#include <machine/intr.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <dev/cons.h>

#define MAXMEM	64*1024		/* XXX - from cmap.h */
#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <news68k/news68k/machid.h>
#include <news68k/news68k/isr.h>

#include "le.h"
#include "kb.h"
#include "ms.h"
#include "si.h"
#include "ksyms.h"
/* XXX etc. etc. */

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

struct vm_map *phys_map = NULL;

int	maxmem;			/* max memory per process */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

extern paddr_t avail_start, avail_end;
extern int end, *esym;
extern u_int lowram;
extern u_int ctrl_led_phys;

/* prototypes for local functions */
static void identifycpu(void);
static void initcpu(void);
static int cpu_dumpsize(void);
static int cpu_dump(int (*)(dev_t, daddr_t, void *, size_t), daddr_t *);
static void cpu_init_kcore_hdr(void);

#ifdef news1700
static void news1700_init(void);
static void parityenable(void);
static void parityerror(void);
#endif
#ifdef news1200
static void news1200_init(void);
#endif

/* functions called from locore.s */
void dumpsys(void);
void news68k_init(void);
void straytrap(int, u_short);

/*
 * Machine-dependent crash dump header info.
 */
cpu_kcore_hdr_t cpu_kcore_hdr;

/*
 * Note that the value of delay_divisor is roughly
 * 2048 / cpuspeed (where cpuspeed is in MHz) on 68020
 * and 68030 systems.
 */
int	cpuspeed = 25;		/* relative CPU speed; XXX skewed on 68040 */
int	delay_divisor = 82;	/* delay constant */

/*
 * Early initialization, before main() is called.
 */
void
news68k_init(void)
{
	int i;

	/*
	 * Tell the VM system about available physical memory.  The
	 * news68k only has one segment.
	 */
	uvm_page_physload(atop(avail_start), atop(avail_end),
	    atop(avail_start), atop(avail_end), VM_FREELIST_DEFAULT);

	/* Initialize system variables. */
	switch (systype) {
#ifdef news1700
	case NEWS1700:
		news1700_init();
		break;
#endif
#ifdef news1200
	case NEWS1200:
		news1200_init();
		break;
#endif
	default:
		panic("impossible system type");
	}

	isrinit();

	/*
	 * Initialize error message buffer (at end of core).
	 * avail_end was pre-decremented in pmap_bootstrap to compensate.
	 */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_kenter_pa((vaddr_t)msgbufaddr + i * PAGE_SIZE,
		    avail_end + i * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, 0)
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU, and do autoconfiguration.
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
}

/*
 * Set registers on exec.
 */
void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	struct pcb *pcb = lwp_getpcb(l);

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
	frame->f_regs[A2] = (int)l->l_proc->p_psstr;
	frame->f_regs[A3] = 0;
	frame->f_regs[A4] = 0;
	frame->f_regs[A5] = 0;
	frame->f_regs[A6] = 0;
	frame->f_regs[SP] = stack;

	/* restore a null state frame */
	pcb->pcb_fpregs.fpf_null = 0;
	if (fputype != FPU_NONE)
		m68881_restore(&pcb->pcb_fpregs);
}

/*
 * Info for CTL_HW
 */
char cpu_model[124];

int news_machine_id;

static void
identifycpu(void)
{

	printf("SONY NET WORK STATION, Model %s, ", cpu_model);
	printf("Machine ID #%d\n", news_machine_id);

	delay_divisor = (20480 / cpuspeed + 5) / 10; /* XXX */
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

	/* If rebooting and a dump is requested, do it. */
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
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		DELAY(1000000);
		doboot(RB_POWERDOWN);
		/* NOTREACHED */
	}

	if (howto & RB_HALT) {
		printf("System halted.\n\n");
		doboot(RB_HALT);
		/* NOTREACHED */
	}

	printf("rebooting...\n");
	DELAY(1000000);
	doboot(RB_AUTOBOOT);
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
	 * news68k has one contiguous memory segment.
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
	const struct bdevsw *bdev;
	int chdrsize;	/* size of dump header */
	int nblks;	/* size of dump area */

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL) {
		dumpdev = NODEV;
		return;
	}
	if (bdev->d_psize == NULL)
		return;
	nblks = (*bdev->d_psize)(dumpdev);
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
			printf_nolog("%d ", pg / NPGMB);
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
badaddr(void *addr, int nbytes)
{
	int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return 0;
#endif

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return 1;
	}
	switch (nbytes) {
	case 1:
		i = *(volatile char *)addr;
		break;

	case 2:
		i = *(volatile short *)addr;
		break;

	case 4:
		i = *(volatile int *)addr;
		break;

	default:
		panic("badaddr: bad request");
	}
	nofault = (int *) 0;
	return 0;
}

int
badbaddr(void *addr)
{
	int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return 1;
	}
	i = *(volatile char *)addr;
	nofault = (int *) 0;
	return 0;
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
		error = exec_aout_prep_oldzmagic(l->l_proc, epp);
		return(error);
#endif
#ifdef COMPAT_44
	case (MID_HP300 << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(p, epp);
		return error;
#endif
	}
#endif /* !(defined(COMPAT_NOMID) || defined(COMPAT_44)) */

	return ENOEXEC;
}

/*
 *  System dependent initilization
 */

static volatile uint8_t *dip_switch, *int_status;

const uint8_t *idrom_addr;
volatile uint8_t *ctrl_ast, *ctrl_int2;
volatile uint8_t *ctrl_led;
uint32_t sccport0a, lance_mem_phys;

#ifdef news1700
static volatile u_char *ctrl_parity, *ctrl_parity_clr, *parity_vector;

struct news68k_model {
	const int id;
	const char *name;
};

static const struct news68k_model news68k_models[] = {
	{ ICK001,	"ICK001"	},	/*  1 */
	{ ICK00X,	"ICK00X"	},	/*  2 */
	{ NWS799,	"NWS-799"	},	/*  3 */
	{ NWS800,	"NWS-800"	},	/*  4 */
	{ NWS801,	"NWS-801"	},	/*  5 */
	{ NWS802,	"NWS-802"	},	/*  6 */
	{ NWS711,	"NWS-711"	},	/*  7 */
	{ NWS721,	"NWS-721"	},	/*  8 */
	{ NWS1850,	"NWS-1850"	},	/*  9 */
	{ NWS810,	"NWS-810"	},	/* 10 */
	{ NWS811,	"NWS-811"	},	/* 11 */
	{ NWS1830,	"NWS-1830"	},	/* 12 */
	{ NWS1750,	"NWS-1750"	},	/* 13 */
	{ NWS1720,	"NWS-1720"	},	/* 14 */
	{ NWS1930,	"NWS-1930"	},	/* 15 */
	{ NWS1960,	"NWS-1960"	},	/* 16 */
	{ NWS712,	"NWS-712"	},	/* 17 */
	{ NWS1860,	"NWS-1860"	},	/* 18 */
	{ PWS1630,	"PWS-1630"	},	/* 19 */
	{ NWS820,	"NWS-820"	},	/* 20 */
	{ NWS821,	"NWS-821"	},	/* 21 */
	{ NWS1760,	"NWS-1760"	},	/* 22 */
	{ NWS1710,	"NWS-1710"	},	/* 23 */
	{ NWS830,	"NWS-830"	},	/* 30 */
	{ NWS831,	"NWS-831"	},	/* 31 */
	{ NWS841,	"NWS-841"	},	/* 41 */
	{ PWS1570,	"PWS-1570"	},	/* 52 */
	{ PWS1590,	"PWS-1590"	},	/* 54 */
	{ NWS1520,	"NWS-1520"	},	/* 56 */
	{ PWS1550,	"PWS-1550"	},	/* 73 */
	{ PWS1520,	"PWS-1520"	},	/* 74 */
	{ PWS1560,	"PWS-1560"	},	/* 75 */
	{ NWS1530,	"NWS-1530"	},	/* 76 */
	{ NWS1580,	"NWS-1580"	},	/* 77 */
	{ NWS1510,	"NWS-1510"	},	/* 78 */
	{ NWS1410,	"NWS-1410"	},	/* 81 */
	{ NWS1450,	"NWS-1450"	},	/* 85 */
	{ NWS1460,	"NWS-1460"	},	/* 86 */
	{ NWS891,	"NWS-891"	},	/* 91 */
	{ NWS911,	"NWS-911"	},	/* 111 */
	{ NWS921,	"NWS-921"	},	/* 121 */
	{ 0,		NULL		}
};

static void
news1700_init(void)
{
	struct oidrom idrom;
	const char *t;
	const uint8_t *p;
	uint8_t *q;
	u_int i;

	dip_switch	= (uint8_t *)IIOV(0xe1c00100);
	int_status	= (uint8_t *)IIOV(0xe1c00200);

	idrom_addr	= (uint8_t *)IIOV(0xe1c00000);
	ctrl_ast	= (uint8_t *)IIOV(0xe1280000);
	ctrl_int2	= (uint8_t *)IIOV(0xe1180000);
	ctrl_led	= (uint8_t *)IIOV(ctrl_led_phys);

	sccport0a	= IIOV(0xe0d40002);
	lance_mem_phys	= 0xe0e00000;

	p = idrom_addr;
	q = (uint8_t *)&idrom;

	for (i = 0; i < sizeof(idrom); i++, p += 2)
		*q++ = ((*p & 0x0f) << 4) | (*(p + 1) & 0x0f);

	t = NULL;
	for (i = 0; news68k_models[i].name != NULL; i++) {
		if (news68k_models[i].id == idrom.id_model) {
			t = news68k_models[i].name;
		}
	}
	if (t == NULL)
		panic("unexpected system model.");

	strcat(cpu_model, t);
	news_machine_id = (idrom.id_serial[0] << 8) + idrom.id_serial[1];

	ctrl_parity	= (uint8_t *)IIOV(0xe1080000);
	ctrl_parity_clr	= (uint8_t *)IIOV(0xe1a00000);
	parity_vector	= (uint8_t *)IIOV(0xe1c00200);

	parityenable();

	cpuspeed = 25;
}

/*
 * parity error handling (vectored NMI?)
 */

static void
parityenable(void)
{

#define PARITY_VECT 0xc0
#define PARITY_PRI 7

	*parity_vector = PARITY_VECT;

	isrlink_vectored((int (*)(void *))parityerror, NULL,
	    PARITY_PRI, PARITY_VECT);

	*ctrl_parity_clr = 1;
	*ctrl_parity = 1;

#ifdef DEBUG
	printf("enable parity check\n");
#endif
}

static int innmihand;	/* simple mutex */

static void
parityerror(void)
{

	/* Prevent unwanted recursion. */
	if (innmihand)
		return;
	innmihand = 1;

#if 0 /* XXX need to implement XXX */
	panic("parity error");
#else
	printf("parity error detected.\n");
	*ctrl_parity_clr = 1;
#endif
	innmihand = 0;
}
#endif /* news1700 */

#ifdef news1200
static void
news1200_init(void)
{
	struct idrom idrom;
	const uint8_t *p;
	uint8_t *q;
	int i;

	dip_switch	= (uint8_t *)IIOV(0xe1680000);
	int_status	= (uint8_t *)IIOV(0xe1200000);

	idrom_addr	= (uint8_t *)IIOV(0xe1400000);
	ctrl_ast	= (uint8_t *)IIOV(0xe1100000);
	ctrl_int2	= (uint8_t *)IIOV(0xe10c0000);
	ctrl_led	= (uint8_t *)IIOV(ctrl_led_phys);

	sccport0a	= IIOV(0xe1780002);
	lance_mem_phys	= 0xe1a00000;

	p = idrom_addr;
	q = (uint8_t *)&idrom;
	for (i = 0; i < sizeof(idrom); i++, p += 2)
		*q++ = ((*p & 0x0f) << 4) | (*(p + 1) & 0x0f);

	strcat(cpu_model, idrom.id_model);
	news_machine_id = idrom.id_serial;

	cpuspeed = 25;
}
#endif /* news1200 */

/*
 * interrupt handlers
 * XXX should do better handling XXX
 */

void intrhand_lev3(void);
void intrhand_lev4(void);

void
intrhand_lev3(void)
{
	int stat;

	stat = *int_status;
	intrcnt[3]++;
	uvmexp.intrs++;
#if 1
	printf("level 3 interrupt: INT_STATUS = 0x%02x\n", stat);
#endif
}

extern int leintr(int);
extern int si_intr(int);

void
intrhand_lev4(void)
{
	int stat;

#define INTST_LANCE	0x04
#define INTST_SCSI	0x80

	stat = *int_status;
	intrcnt[4]++;
	uvmexp.intrs++;

#if NSI > 0
	if (stat & INTST_SCSI) {
		si_intr(0);
	}
#endif
#if NLE > 0
	if (stat & INTST_LANCE) {
		leintr(0);
	}
#endif
#if 0
	printf("level 4 interrupt\n");
#endif
}

/*
 * consinit() routines - from newsmips/cpu_cons.c
 */

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 * XXX need something better here.
 */
#define SCC_CONSOLE	0
#define SW_CONSOLE	0x07
#define SW_NWB512	0x04
#define SW_NWB225	0x01
#define SW_FBPOP	0x02
#define SW_FBPOP1	0x06
#define SW_FBPOP2	0x03
#define SW_AUTOSEL	0x07

struct consdev *cn_tab = NULL;
extern struct consdev consdev_bm, consdev_zs;

int tty00_is_console = 0;

void
consinit(void)
{

	int dipsw = *dip_switch;

	dipsw &= ~SW_CONSOLE;

	switch (dipsw & SW_CONSOLE) {
	    default: /* XXX no fb support yet */
	    case 0:
		tty00_is_console = 1;
		cn_tab = &consdev_zs;
		(*cn_tab->cn_init)(cn_tab);
		break;
	}
#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)esym - (int)&end - sizeof(Elf32_Ehdr),
		    (void *)&end, esym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
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
