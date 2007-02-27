/*	$NetBSD: machdep.c,v 1.134.2.1 2007/02/27 16:53:23 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.134.2.1 2007/02/27 16:53:23 yamt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_compat_netbsd.h"
#include "opt_m680x0.h"
#include "opt_fpu_emulate.h"
#include "opt_m060sp.h"
#include "opt_panicbutton.h"
#include "opt_extmem.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/ksyms.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(LKM)
#include <sys/exec_elf.h>
#endif

#include <net/netisr.h>
#undef PS	/* XXX netccitt/pk.h conflict with machine/reg.h? */

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <m68k/cacheops.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/kcore.h>

#include <dev/cons.h>

#define	MAXMEM	64*1024	/* XXX - from cmap.h */
#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <sys/device.h>

#include <machine/bus.h>
#include <arch/x68k/dev/intiovar.h>

void initcpu(void);
void identifycpu(void);
void doboot(void) __attribute__((__noreturn__));

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

struct vm_map *exec_map = NULL;  
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

extern paddr_t avail_start, avail_end;
extern vaddr_t virtual_avail;
extern u_int lowram;
extern int end, *esym;
extern psize_t mem_size;

caddr_t	msgbufaddr;
int	maxmem;			/* max memory per process */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

/* prototypes for local functions */
void    identifycpu(void);
void    initcpu(void);
int	cpu_dumpsize(void);
int	cpu_dump(int (*)(dev_t, daddr_t, caddr_t, size_t), daddr_t *);
void	cpu_init_kcore_hdr(void);
#ifdef EXTENDED_MEMORY
static int mem_exists(caddr_t, u_long);
static void setmemrange(void);
#endif

/* functions called from locore.s */
void	dumpsys(void);
void    straytrap(int, u_short);
void	nmihand(struct frame);
void	intrhand(int);

/*
 * On the 68020/68030, the value of delay_divisor is roughly
 * 2048 / cpuspeed (where cpuspeed is in MHz).
 *
 * On the 68040, the value of delay_divisor is roughly
 * 759 / cpuspeed (where cpuspeed is in MHz).
 *
 * On the 68060, the value of delay_divisor is reported to be
 * 128 / cpuspeed (where cpuspeed is in MHz).
 */
int	delay_divisor = 140;	/* assume some reasonable value to start */
static int cpuspeed;		/* MPU clock (in MHz) */

/*
 * Machine-dependent crash dump header info.
 */
cpu_kcore_hdr_t cpu_kcore_hdr;

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{
	/*
	 * bring graphics layer up.
	 */
	config_console();

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#ifdef KGDB
	zs_kgdb_init();			/* XXX */
#endif
#if NKSYMS || defined(DDB) || defined(LKM)
	ksyms_init((int)esym - (int)&end - sizeof(Elf32_Ehdr),
		 (void *)&end, esym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * Tell the VM system about available physical memory.
	 */
	uvm_page_physload(atop(avail_start), atop(avail_end),
			atop(avail_start), atop(avail_end),
			VM_FREELIST_DEFAULT);
#ifdef EXTENDED_MEMORY
	setmemrange();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
	u_int i;
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif
#if 0
	rtclockinit(); /* XXX */
#endif

	if (fputype != FPU_NONE)
		m68k_make_fpu_idle_frame();

	/*
	 * Initialize error message buffer (at end of core).
	 * avail_end was pre-decremented in pmap_bootstrap to compensate.
	 */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_enter(pmap_kernel(), (vaddr_t)msgbufaddr + i * PAGE_SIZE,
		    avail_end + i * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));

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
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16*NCARGS, VM_MAP_PAGEABLE, false, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 nmbclusters * mclbytes, VM_MAP_INTRSAFE,
				 false, NULL);

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
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct frame *frame = (struct frame *)l->l_md.md_regs;

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
	l->l_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	if (fputype)
		m68881_restore(&l->l_addr->u_pcb.pcb_fpregs);
}

/*
 * Info for CTL_HW
 */
char	cpu_model[96];		/* max 85 chars */
static const char *fpu_descr[] = {
#ifdef	FPU_EMULATE
	", emulator FPU", 	/* 0 */
#else
	", no math support",	/* 0 */
#endif
	", m68881 FPU",		/* 1 */
	", m68882 FPU",		/* 2 */
	"/FPU",			/* 3 */
	"/FPU",			/* 4 */
	};

void
identifycpu(void)
{
        /* there's alot of XXX in here... */
	const char *cpu_type, *mach, *mmu, *fpu;
	char clock[16];

	/*
	 * check machine type constant
	 */
	switch (intio_get_sysport_mpustat()) {
	case 0xdc:
		/*
		 * CPU Type == 68030, Clock == 25MHz
		 */
		mach = "030";
		break;
	case 0xfe:
		/*
		 * CPU Type == 68000, Clock == 16MHz
		 */
		mach = "000XVI";
		break;
	case 0xff:
		/*
		 * CPU Type == 68000, Clock == 10MHz
		 */
		mach = "000/ACE/PRO/EXPERT/SUPER";
		break;
	default:
		/*
		 * unknown type
		 */
		mach = "000?(unknown model)";
		break;
	}

	cpuspeed = 2048 / delay_divisor;
	sprintf(clock, "%dMHz", cpuspeed);
	switch (cputype) {
	case CPU_68060:
		cpu_type = "m68060";
		mmu = "/MMU";
		cpuspeed = 128 / delay_divisor;
		sprintf(clock, "%d/%dMHz", cpuspeed*2, cpuspeed);
		break;
	case CPU_68040:
		cpu_type = "m68040";
		mmu = "/MMU";
		cpuspeed = 759 / delay_divisor;
		sprintf(clock, "%d/%dMHz", cpuspeed*2, cpuspeed);
		break;
	case CPU_68030:
		cpu_type = "m68030";
		mmu = "/MMU";
		break;
	case CPU_68020:
		cpu_type = "m68020";
		mmu = ", m68851 MMU";
		break;
	default:
		cpu_type = "unknown";
		mmu = ", unknown MMU";
		break;
	}
	if (fputype >= 0 && fputype < sizeof(fpu_descr)/sizeof(fpu_descr[0]))
		fpu = fpu_descr[fputype];
	else
		fpu = ", unknown FPU";
	sprintf(cpu_model, "X68%s (%s CPU%s%s, %s clock)",
		mach, cpu_type, mmu, fpu, clock);
	printf("%s\n", cpu_model);
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
int	power_switch_is_off = 0;

void
cpu_reboot(int howto, char *bootstr)
{
	/* take a snap shot before clobbering any registers */
	if (curlwp && curlwp->l_addr)
		savectx(&curlwp->l_addr->u_pcb);

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		/*resettodr();*/
	}

	/* Disable interrputs. */
	splhigh();

	if (howto & RB_DUMP)
		dumpsys();

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
	/* a) RB_POWERDOWN
	 *  a1: the power switch is still on
	 *	Power cannot be removed; simply halt the system (b)
	 *	Power switch state is checked in shutdown hook
	 *  a2: the power switch is off
	 *	Remove the power; the simplest way is go back to ROM eg. reboot
	 * b) RB_HALT
	 *      call cngetc
         * c) otherwise
	 *	Reboot
	*/
	if (((howto & RB_POWERDOWN) == RB_POWERDOWN) && power_switch_is_off)
		doboot();
	else if (/*((howto & RB_POWERDOWN) == RB_POWERDOWN) ||*/
		 ((howto & RB_HALT) == RB_HALT)) {
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
cpu_init_kcore_hdr(void)
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	int i;

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
	 * X68k has multiple RAM segments on some models.
	 */
	m->ram_segs[0].start = lowram;
	m->ram_segs[0].size = mem_size - lowram;
	for (i = 1; i < vm_nphysseg; i++) {
		m->ram_segs[i].start = ctob(vm_physmem[i].start);
		m->ram_segs[i].size  = ctob(vm_physmem[i].end
					    - vm_physmem[i].start);
	}
}

/*
 * Compute the size of the machine-dependent crash dump header.
 * Returns size in disk blocks.
 */

#define CHDRSIZE (ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)))
#define MDHDRSIZE roundup(CHDRSIZE, dbtob(1))

int
cpu_dumpsize(void)
{

	return btodb(MDHDRSIZE);
}

/*
 * Called by dumpsys() to dump the machine-dependent header.
 */
int
cpu_dump(int (*dump)(dev_t, daddr_t, caddr_t, size_t), daddr_t *blknop)
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
 * Dumps always skip the first PAGE_SIZE of disk space in
 * case there might be a disk label stored there.  If there
 * is extra space, put dump at the end to reduce the chance
 * that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	const struct bdevsw *bdev;
	int chdrsize;	/* size of dump header */
	int nblks;	/* size of dump area */
	int i;

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

	dumpsize = 0;
	for (i = 0; m->ram_segs[i].size && i < M68K_NPHYS_RAM_SEGS; i++)
		dumpsize += btoc(m->ram_segs[i].size);
	/*
	 * Check to see if we will fit.  Note we always skip the
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

void
dumpsys(void)
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	const struct bdevsw *bdev;
	daddr_t blkno;		/* current block to write */
				/* dump routine */
	int (*dump)(dev_t, daddr_t, caddr_t, size_t);
	int pg;			/* page being dumped */
	paddr_t maddr;		/* PA being dumped */
	int seg;		/* RAM segment being dumped */
	int error;		/* error code from (*dump)() */

	/* XXX initialized here because of gcc lossage */
	seg = 0;
	maddr = m->ram_segs[seg].start;
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
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	dump = bdev->d_dump;
	blkno = dumplo;

	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

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
		if (maddr == 0) {
			/* Skip first page */
			maddr += PAGE_SIZE;
			blkno += btodb(PAGE_SIZE);
			continue;
		}
		while (maddr >=
		    (m->ram_segs[seg].start + m->ram_segs[seg].size)) {
			if (++seg >= M68K_NPHYS_RAM_SEGS ||
			    m->ram_segs[seg].size == 0) {
				error = EINVAL;		/* XXX ?? */
				goto bad;
			}
			maddr = m->ram_segs[seg].start;
		}
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

void
initcpu(void)
{
	/* XXX should init '40 vecs here, too */
#if defined(M68060)
	extern caddr_t vectab[256];
#if defined(M060SP)
	extern u_int8_t I_CALL_TOP[];
	extern u_int8_t FP_CALL_TOP[];
#else
	extern u_int8_t illinst;
#endif
	extern u_int8_t fpfault;
#endif

#ifdef MAPPEDCOPY

	/*
	 * Initialize lower bound for doing copyin/copyout using
	 * page mapping (if not already set).  We don't do this on
	 * VAC machines as it loses big time.
	 */
	if ((int) mappedcopysize == -1) {
		mappedcopysize = PAGE_SIZE;
	}
#endif

#if defined(M68060)
	if (cputype == CPU_68060) {
#if defined(M060SP)
		/* integer support */
		vectab[61] = &I_CALL_TOP[128 + 0x00];

		/* floating point support */
		vectab[11] = &FP_CALL_TOP[128 + 0x30];
		vectab[55] = &FP_CALL_TOP[128 + 0x38];
		vectab[60] = &FP_CALL_TOP[128 + 0x40];

		vectab[54] = &FP_CALL_TOP[128 + 0x00];
		vectab[52] = &FP_CALL_TOP[128 + 0x08];
		vectab[53] = &FP_CALL_TOP[128 + 0x10];
		vectab[51] = &FP_CALL_TOP[128 + 0x18];
		vectab[50] = &FP_CALL_TOP[128 + 0x20];
		vectab[49] = &FP_CALL_TOP[128 + 0x28];
#else
		vectab[61] = &illinst;
#endif
		vectab[48] = &fpfault;
	}
	DCIS();
#endif
}

void
straytrap(int pc, u_short evec)
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
#if defined(DDB)
	Debugger();
#endif
}

int	*nofault;

int
badaddr(volatile void* addr)
{
	int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = NULL;
		return(1);
	}
	i = *(volatile short *)addr;
	nofault = NULL;
	return(0);
}

int
badbaddr(volatile void *addr)
{
	int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = NULL;
		return(1);
	}
	i = *(volatile char *)addr;
	nofault = NULL;
	return(0);
}

void netintr(void);

void
netintr(void)
{

#define DONETISR(bit, fn) do {		\
	if (netisr & (1 << bit)) {	\
		netisr &= ~(1 << bit);	\
		fn();			\
	}				\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}

void
intrhand(int sr)
{
	printf("intrhand: unexpected sr 0x%x\n", sr);
}

#if (defined(DDB) || defined(DEBUG)) && !defined(PANICBUTTON)
#define PANICBUTTON
#endif

#ifdef PANICBUTTON
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int crashandburn = 0;
int candbdelay = 50;	/* give em half a second */
void candbtimer(void *);

#ifndef DDB
static struct callout candbtimer_ch = CALLOUT_INITIALIZER;
#endif

void
candbtimer(void *arg)
{

	crashandburn = 0;
}
#endif

/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
void
nmihand(struct frame frame)
{
	intio_set_sysport_keyctrl(intio_get_sysport_keyctrl() | 0x04);

	if (1) {
#ifdef PANICBUTTON
		static int innmihand = 0;

		/*
		 * Attempt to reduce the window of vulnerability for recursive
		 * NMIs (e.g. someone holding down the keyboard reset button).
		 */
		if (innmihand == 0) {
			innmihand = 1;
			printf("Got a keyboard NMI\n");
			innmihand = 0;
		}
#ifdef DDB
		Debugger();
#else
		if (panicbutton) {
			if (crashandburn) {
				crashandburn = 0;
				panic(panicstr ?
				      "forced crash, nosync" : "forced crash");
			}
			crashandburn++;
			callout_reset(&candbtimer_ch, candbdelay,
			    candbtimer, NULL);
		}
#endif /* DDB */
#endif /* PANICBUTTON */
		return;
	}
	/* panic?? */
	printf("unexpected level 7 interrupt ignored\n");
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
		break;
#endif
#ifdef COMPAT_44
	case (MID_HP300 << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(l->l_proc, epp);
		break;
#endif
	default:
		error = ENOEXEC;
	}

	return error;
#else /* !(defined(COMPAT_NOMID) || defined(COMPAT_44)) */
	return ENOEXEC;
#endif
}

#ifdef EXTENDED_MEMORY
#ifdef EM_DEBUG
static int em_debug = 0;
#define DPRINTF(str) do{ if (em_debug) printf str; } while (0);
#else
#define DPRINTF(str)
#endif

static struct memlist {
	caddr_t base;
	psize_t min;
	psize_t max;
} memlist[] = {
	/* TS-6BE16 16MB memory */
	{(caddr_t)0x01000000, 0x01000000, 0x01000000},
	/* 060turbo SIMM slot (4--128MB) */
	{(caddr_t)0x10000000, 0x00400000, 0x08000000},
};
static vaddr_t mem_v, base_v;

/*
 * check memory existency
 */
static int
mem_exists(caddr_t mem, u_long basemax)
{
	/* most variables must be register! */
	volatile unsigned char *m, *b;
	unsigned char save_m, save_b=0;	/* XXX: shutup gcc */
	int baseismem;
	int exists = 0;
	caddr_t base;
	caddr_t begin_check, end_check;
	label_t	faultbuf;

	DPRINTF (("Enter mem_exists(%p, %x)\n", mem, basemax));
	DPRINTF ((" pmap_enter(%p, %p) for target... ", mem_v, mem));
	pmap_enter(pmap_kernel(), mem_v, (paddr_t)mem,
		   VM_PROT_READ|VM_PROT_WRITE, VM_PROT_READ|PMAP_WIRED);
	pmap_update(pmap_kernel());
	DPRINTF ((" done.\n"));

	/* only 24bits are significant on normal X680x0 systems */
	base = (caddr_t)((u_long)mem & 0x00FFFFFF);
	DPRINTF ((" pmap_enter(%p, %p) for shadow... ", base_v, base));
	pmap_enter(pmap_kernel(), base_v, (paddr_t)base,
		   VM_PROT_READ|VM_PROT_WRITE, VM_PROT_READ|PMAP_WIRED);
	pmap_update(pmap_kernel());
	DPRINTF ((" done.\n"));

	m = (void*)mem_v;
	b = (void*)base_v;

	/* This is somewhat paranoid -- avoid overwriting myself */
	__asm("lea %%pc@(begin_check_mem),%0" : "=a"(begin_check));
	__asm("lea %%pc@(end_check_mem),%0" : "=a"(end_check));
	if (base >= begin_check && base < end_check) {
		size_t off = end_check - begin_check;

		DPRINTF ((" Adjusting the testing area.\n"));
		m -= off;
		b -= off;
	}

	nofault = (int *) &faultbuf;
	if (setjmp ((label_t *)nofault)) {
		nofault = (int *) 0;
		pmap_remove(pmap_kernel(), mem_v, mem_v+PAGE_SIZE);
		pmap_remove(pmap_kernel(), base_v, base_v+PAGE_SIZE);
		pmap_update(pmap_kernel());
		DPRINTF (("Fault!!! Returning 0.\n"));
		return 0;
	}

	DPRINTF ((" Let's begin. mem=%p, base=%p, m=%p, b=%p\n",
		  mem, base, m, b));

	(void) *m; 
	/*
	 * Can't check by writing if the corresponding
	 * base address isn't memory.
	 *
	 * I hope this would be no harm....
	 */
	baseismem = base < (caddr_t)basemax;

__asm("begin_check_mem:");
	/* save original value (base must be saved first) */
	if (baseismem)
		save_b = *b;
	save_m = *m;

	/*
	 * stack and other data segment variables are unusable
	 * til end_check_mem, because they may be clobbered.
	 */

	/*
	 * check memory by writing/reading
	 */
	if (baseismem)
		*b = 0x55;
	*m = 0xAA;
	if ((baseismem && *b != 0x55) || *m != 0xAA)
		goto out;

	*m = 0x55;
	if (baseismem)
		*b = 0xAA;
	if (*m != 0x55 || (baseismem && *b != 0xAA))
		goto out;

	exists = 1;
out:
	*m = save_m;
	if (baseismem)
		*b = save_b;

__asm("end_check_mem:");

	nofault = (int *)0;
	pmap_remove(pmap_kernel(), mem_v, mem_v+PAGE_SIZE);
	pmap_remove(pmap_kernel(), base_v, base_v+PAGE_SIZE);
	pmap_update(pmap_kernel());

	DPRINTF ((" End.\n"));

	DPRINTF (("Returning from mem_exists. result = %d\n", exists));

	return exists;
}

static void
setmemrange(void)
{
	int i;
	psize_t s, minimum, maximum;
	struct memlist *mlist = memlist;
	u_long h;
	int basemax = ctob(physmem);

	/*
	 * VM system is not started yet.  Use the first and second avalable
	 * pages to map the (possible) target memory and its shadow.
	 */
	mem_v = virtual_avail;		/* target */
	base_v = mem_v + PAGE_SIZE;	/* shadow */

	{	/* Turn off the processor cache. */
		int cacr;
		PCIA();		/* cpusha dc */
		switch (cputype) {
		default:
		case CPU_68030:
			cacr = CACHE_OFF;
			break;
		case CPU_68040:
			cacr = CACHE40_OFF;
			break;
		case CPU_68060:
			cacr = CACHE60_OFF;
			break;
		}
		__asm volatile ("movc %0,%%cacr"::"d"(cacr));
	}

	/* discover extended memory */
	for (i = 0; i < sizeof(memlist) / sizeof(memlist[0]); i++) {
		minimum = mlist[i].min;
		maximum = mlist[i].max;
		/*
		 * Normally, x68k hardware is NOT 32bit-clean.
		 * But some type of extended memory is in 32bit address space.
		 * Check whether.
		 */
		if (!mem_exists(mlist[i].base, basemax))
			continue;
		h = 0;
		/* range check */
		for (s = minimum; s <= maximum; s += 0x00100000) {
			if (!mem_exists(mlist[i].base + s - 4, basemax))
				break;
			h = (u_long)(mlist[i].base + s);
		}
		if ((u_long)mlist[i].base < h) {
			uvm_page_physload(atop(mlist[i].base), atop(h),
					  atop(mlist[i].base), atop(h),
					  VM_FREELIST_DEFAULT);
			mem_size += h - (u_long) mlist[i].base;
		}
			
	}

	{	/* Re-enable the processor cache. */
		int cacr;
		ICIA();
		switch (cputype) {
		default:
		case CPU_68030:
			cacr = CACHE_ON;
			break;
		case CPU_68040:
			cacr = CACHE40_ON;
			break;
		case CPU_68060:
			cacr = CACHE60_ON;
			break;
		}
		__asm volatile ("movc %0,%%cacr"::"d"(cacr));
	}

	physmem = m68k_btop(mem_size);
}
#endif
