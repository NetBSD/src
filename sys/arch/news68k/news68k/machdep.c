/*	$NetBSD: machdep.c,v 1.5 2000/03/10 19:06:43 tsutsui Exp $	*/

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

#include "opt_ddb.h"
#include "opt_compat_hpux.h"
#include "opt_compat_netbsd.h"

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
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/pte.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <dev/cons.h>

#define MAXMEM	64*1024		/* XXX - from cmap.h */
#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <news68k/news68k/machid.h>
#include <news68k/news68k/isr.h>

#include "le.h"
#include "kb.h"
#include "ms.h"
#include "si.h"
/* XXX etc. etc. */

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

vm_map_t exec_map = NULL;  
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

extern paddr_t avail_end;

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
void	identifycpu __P((void));
void	initcpu __P((void));
void	parityenable __P((void));
void	parityerror __P((void));
void	init_intreg __P((void));
int	readidrom __P((u_char *));

int	cpu_dumpsize __P((void));
int	cpu_dump __P((int (*)(dev_t, daddr_t, caddr_t, size_t), daddr_t *));
void	cpu_init_kcore_hdr __P((void));

#ifdef news1700
void	news1700_init __P((void));
#endif
#ifdef news1200
void	news1200_init __P((void));
#endif
/* functions called from locore.s */
void	dumpsys __P((void));
void	news68k_init __P((void));
void	straytrap __P((int, u_short));

/*
 * Machine-dependent crash dump header info.
 */
cpu_kcore_hdr_t cpu_kcore_hdr;

/*
 * Note that the value of delay_divisor is roughly
 * 2048 / cpuspeed (where cpuspeed is in MHz) on 68020
 * and 68030 systems.
 */
int	cpuspeed = 25;		/* relative cpu speed; XXX skewed on 68040 */
int	delay_divisor = 82;	/* delay constant */

/*
 * Early initialization, before main() is called.
 */
void
news68k_init()
{
	int i;

	extern paddr_t avail_start, avail_end;

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
		pmap_enter(pmap_kernel(), (vaddr_t)msgbufaddr + i * NBPG,
		    avail_end + i * NBPG, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	extern char *kernel_text, *etext;
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
	 * Tell the VM system that the area before the text segment
	 * is invalid.
	 *
	 * XXX This is bogus; should just fix KERNBASE and
	 * XXX VM_MIN_KERNEL_ADDRESS, but not right now.
	 */
	if (uvm_map_protect(kernel_map, 0, m68k_round_page(&kernel_text),
	    UVM_PROT_NONE, TRUE) != KERN_SUCCESS)
		panic("can't mark pre-text pages off-limits");

	/*
	 * Tell the VM system that writing to kernel text isn't allowed.
	 * If we don't, we might end up COW'ing the text segment!
	 *
	 */
	if (uvm_map_protect(kernel_map, m68k_trunc_page(&kernel_text),
	    m68k_round_page(&etext), UVM_PROT_READ|UVM_PROT_EXEC, TRUE)
	    != KERN_SUCCESS)
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
	frame->f_regs[A2] = (int)PS_STRINGS;
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
char cpu_model[124];
extern char version[];

int news_machine_id;

void
identifycpu()
{

	printf("SONY NET WORK STATION, Model %s, ", cpu_model);
	printf("Machine ID #%d\n", news_machine_id);

	delay_divisor = (20480 / cpuspeed + 5) / 10; /* XXX */
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

#if __GNUC__    /* XXX work around lame compiler problem (gcc 2.7.2) */
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
void
cpu_init_kcore_hdr()
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	extern int end;

	bzero(&cpu_kcore_hdr, sizeof(cpu_kcore_hdr));

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
	 * news68k has one contiguous memory segment.
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

	bcopy(&cpu_kcore_hdr, chdr, sizeof(cpu_kcore_hdr_t));
	error = (*dump)(dumpdev, *blknop, (caddr_t)buf, sizeof(buf));
	*blknop += btodb(sizeof(buf));
	return (error);
}

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
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

	/* Don't put dump messages in msgbuf. */
	msgbufenabled = 0;

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
badaddr(addr, nbytes)
	caddr_t addr;
	int nbytes;
{
	int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return (0);
#endif

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
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
	return (0);
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

/*
 *  System dependent initilization
 */

static volatile u_char *dip_switch, *int_status;

volatile u_char *idrom_addr, *ctrl_ast, *ctrl_int2;
volatile u_char *lance_mem, *ctrl_timer, *ctrl_led, *sccport0a;

#ifdef news1700
static volatile u_char *ctrl_parity, *ctrl_parity_clr, *parity_vector;

struct news68k_model {
	const int id;
	const char *name;
};

const struct news68k_model news68k_models[] = {
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

void
news1700_init()
{
	struct oidrom idrom;
	const char *t;
	u_char *p, *q;
	int i;

	dip_switch	= (u_char *)IIOV(0xe1c00100);
	int_status	= (u_char *)IIOV(0xe1c00200);

	idrom_addr	= (u_char *)IIOV(0xe1c00000);
	ctrl_ast	= (u_char *)IIOV(0xe1280000);
	ctrl_int2	= (u_char *)IIOV(0xe1180000);

	lance_mem	= (u_char *)IIOV(0xe0e00000);
	ctrl_timer	= (u_char *)IIOV(0xe1000000);
	ctrl_led	= (u_char *)IIOV(0xe0dc0000);
	sccport0a	= (u_char *)IIOV(0xe0d40002);

	p = (u_char *)idrom_addr;
	q = (u_char *)&idrom;

	for (i = 0; i < sizeof(idrom); i++, p += 2)
		*q++ = ((*p & 0x0f) << 4) | (*(p + 1) & 0x0f);

	for (i = 0; news68k_models[i].name != NULL; i++) {
		if (news68k_models[i].id == idrom.id_model) {
			t = news68k_models[i].name;
		}
	}
	if (t == NULL)
		panic("unexpected system model.\n");

	strcat(cpu_model, t);
	news_machine_id = (idrom.id_serial[0] << 8) + idrom.id_serial[1];

	ctrl_parity	= (u_char *)IIOV(0xe1080000);
	ctrl_parity_clr	= (u_char *)IIOV(0xe1a00000);
	parity_vector	= (u_char *)IIOV(0xe1c00200);

	parityenable();

	cpuspeed = 25;
}

/*
 * parity error handling (vectored NMI?)
 */

void
parityenable()
{

#define PARITY_VECT 0xc0
#define PARITY_PRI 7

	*parity_vector = PARITY_VECT;

	isrlink_vectored((int (*) __P((void *)))parityerror, NULL,
			 PARITY_PRI, PARITY_VECT);

	*ctrl_parity_clr = 1;
	*ctrl_parity = 1;

#ifdef DEBUG
	printf("enable parity check\n");
#endif
}

static int innmihand;	/* simple mutex */

void
parityerror()
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
void
news1200_init()
{
	struct idrom idrom;
	u_char *p, *q;
	int i;

	dip_switch	= (u_char *)IIOV(0xe1680000);
	int_status	= (u_char *)IIOV(0xe1200000);

	idrom_addr	= (u_char *)IIOV(0xe1400000);
	ctrl_ast	= (u_char *)IIOV(0xe1100000);
	ctrl_int2	= (u_char *)IIOV(0xe10c0000);

	lance_mem	= (u_char *)IIOV(0xe1a00000);
	ctrl_timer	= (u_char *)IIOV(0xe1140000);
	ctrl_led	= (u_char *)IIOV(0xe1500001);
	sccport0a	= (u_char *)IIOV(0xe1780002);

	p = (u_char *)idrom_addr;
	q = (u_char *)&idrom;
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

void intrhand_lev2 __P((void));
void intrhand_lev3 __P((void));
void intrhand_lev4 __P((void));
void intrhand_lev5 __P((void));

void (*sir_routines[NSIR]) __P((void *));
void *sir_args[NSIR];
u_char ssir;
int next_sir;

void
intrhand_lev2()
{
	int bit, s;
	u_char sintr;

	/* disable level 2 interrupt */
	*ctrl_int2 = 0;

	s = splhigh();
	sintr = ssir;
	ssir = 0;
	splx(s);

	intrcnt[2]++;
	uvmexp.intrs++;

	for (bit = 0; bit < next_sir; bit++) {
		if (sintr & (1 << bit)) {
			uvmexp.softs++;
			if (sir_routines[bit])
				sir_routines[bit](sir_args[bit]);
		}
	}
}
/*
 * Allocation routines for software interrupts.
 */
u_char
allocate_sir(proc, arg)
	void (*proc) __P((void *));
	void *arg;
{
	int bit;

	if (next_sir >= NSIR)
		panic("allocate_sir: none left");
	bit = next_sir++;
	sir_routines[bit] = proc;
	sir_args[bit] = arg;
	return (1 << bit);
}

void
init_sir()
{
	extern void netintr __P((void));

	sir_routines[SIR_NET]   = (void (*) __P((void *)))netintr;
	sir_routines[SIR_CLOCK] = (void (*) __P((void *)))softclock;
	next_sir = NEXT_SIR;
}

void
intrhand_lev3()
{
	int stat;

	stat = *int_status;
	intrcnt[3]++;
	uvmexp.intrs++;
#if 1
	printf("level 3 interrupt: INT_STATUS = 0x%02x\n", stat);
#endif
}

void
intrhand_lev4()
{
	int stat;
#if NLE > 0
	extern int leintr __P((int));
#endif
#if NSI > 0
	extern int si_intr __P((int));
#endif

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

void
intrhand_lev5()
{

	intrcnt[5]++;
	uvmexp.intrs++;
#if 1
	printf("level 5 interrupt.\n");
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

#if NFB > 0
void bmcons_putc(int);

extern void fbbm_probe(), vt100_open(), setup_fnt(), setup_fnt24();
extern int vt100_write();

#include "fb.h"
#endif /* NFB > 0 */

void
consinit()
{

	int dipsw = *dip_switch;

#if NFB > 0
	fbbm_probe(dipsw);
	vt100_open();
	setup_fnt();
	setup_fnt24();
#else
	dipsw &= ~SW_CONSOLE;
#endif /* NFB > 0 */

	switch (dipsw & SW_CONSOLE) {
	    default:
#if NFB > 0
		cn_tab = &consdev_bm;
		(*cn_tab->cn_init)(cn_tab);
		break;
#endif /* NFB > 0 */
	    case 0:
		tty00_is_console = 1;
		cn_tab = &consdev_zs;
		(*cn_tab->cn_init)(cn_tab);
		break;
	}
#ifdef DDB
	{
		extern int end;
		extern int *esym;

		ddb_init(*(int *)&end, ((int *)&end) + 1, esym);
	}
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

#if NFB > 0
void
bmcons_putc(c)
	int c;
{
	char cnbuf[1];

	cnbuf[0] = (char)c;
	vt100_write(0, cnbuf, 1);
}
#endif /* NFB > 0 */
