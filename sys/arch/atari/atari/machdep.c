/*	$NetBSD: machdep.c,v 1.47 1997/06/08 23:54:22 veego Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
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
 * from: Utah $Hdr: machdep.c 1.63 91/04/24$
 *
 *	@(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/user.h>
#include <sys/exec.h>            /* for PS_STRINGS */
#include <sys/vnode.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#include <net/netisr.h>
#define	MAXMEM	64*1024*CLSIZE	/* XXX - from cmap.h */
#include <vm/vm_param.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/iomap.h>
#include <machine/bus.h>
#include <dev/cons.h>

static void bootsync __P((void));
static void call_sicallbacks __P((void));
static void identifycpu __P((void));
static void netintr __P((void));
void	straymfpint __P((int, u_short));
void	straytrap __P((int, u_short));

extern vm_offset_t avail_end;

/*
 * Declare these as initialized data so we can patch them.
 */
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif
int	msgbufmapped;		/* set when safe to use msgbuf */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;
extern  int   freebufspace;
extern	u_int lowram;

/*
 * For the fpu emulation and the fpu driver
 */
int	fputype = 0;

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

 /*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#if defined (DDB)
        ddb_init();
        if(boothowto & RB_KDB)
                Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	extern	 void		etext __P((void));
	register unsigned	i;
	register caddr_t	v, firstaddr;
		 int		base, residual;

#ifdef DEBUG
	extern	 int		pmapdebug;
		 int		opmapdebug = pmapdebug;
#endif
		 vm_offset_t	minaddr, maxaddr;
		 vm_size_t	size = 0;
		 u_long		memsize;

	/*
	 * Initialize error message buffer (at end of core).
	 */
#ifdef DEBUG
	pmapdebug = 0;
#endif
	/* avail_end was pre-decremented in pmap_bootstrap to compensate */
	for(i = 0; i < btoc(sizeof (struct msgbuf)); i++)
		pmap_enter(pmap_kernel(), (vm_offset_t)msgbufp, 
		    avail_end + i * NBPG, VM_PROT_ALL, TRUE);
	msgbufmapped = 1;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();

	for (i = memsize = 0; i < NMEM_SEGS; i++) {
		if (boot_segs[i].start == boot_segs[i].end)
			break;
		memsize += boot_segs[i].end - boot_segs[i].start;
	}
	printf("real  mem = %ld (%ld pages)\n", memsize, memsize/NBPG);

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * As pages of memory are allocated and cleared,
	 * "firstaddr" is incremented.
	 * An index into the kernel page table corresponding to the
	 * virtual memory address maintained in "v" is kept in "mapaddr".
	 */
	/*
	 * Make two passes.  The first pass calculates how much memory is
	 * needed and allocates it.  The second pass assigns virtual
	 * addresses to the various data structures.
	 */
	firstaddr = 0;
again:
	v = (caddr_t)firstaddr;

#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))
/*	valloc(cfree, struct cblock, nclist); */
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = maxproc * 2);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
#ifdef SYSVSEM
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(sem, struct sem, seminfo.semmns);
	/* This is pretty disgusting! */
	valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif
	/*
	 * Determine how many buffers to allocate. We allocate
	 * the BSD standard of use 10% of memory for the first 2 Meg,
	 * 5% of remaining. Insure a minimum of 16 buffers.
	 * We allocate 3/4 as many swap buffer headers as file i/o buffers.
	 */
  	if (bufpages == 0)
		if (physmem < btoc(2 * 1024 * 1024))
			bufpages = physmem / (10 * CLSIZE);
		else
			bufpages = (btoc(2 * 1024 * 1024) + physmem) /
			    (20 * CLSIZE);

	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}

	if (nswbuf == 0) {
		nswbuf = (nbuf * 3 / 4) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);
	/*
	 * End of first pass, size has been calculated so allocate memory
	 */
	if (firstaddr == 0) {
		size = (vm_size_t)(v - firstaddr);
		firstaddr = (caddr_t) kmem_alloc(kernel_map, round_page(size));
		if (firstaddr == 0)
			panic("startup: no room for tables");
		goto again;
	}
	/*
	 * End of second pass, addresses have been assigned
	 */
	if ((vm_size_t)(v - firstaddr) != size)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t *)&buffers,
				   &maxaddr, size, TRUE);
	minaddr = (vm_offset_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t)0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vm_offset_t)buffers + i * MAXBSIZE;
		curbufsize = CLBYTES * (i < residual ? base+1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = kmem_suballoc(kernel_map,&minaddr,&maxaddr, 16*NCARGS, TRUE);

	/*
	 * Allocate a submap for physio
	 */
	phys_map= kmem_suballoc(kernel_map,&minaddr,&maxaddr,VM_PHYS_SIZE,TRUE);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);

	/*
	 * Tell the VM system that page 0 isn't mapped.
	 *
	 * XXX This is bogus; should just fix KERNBASE and
	 * XXX VM_MIN_KERNEL_ADDRESS, but not right now.
	 */
	if (vm_map_protect(kernel_map, 0, NBPG, VM_PROT_NONE, TRUE)
	    != KERN_SUCCESS)
		panic("can't mark page 0 off-limits");

	/*
	 * Tell the VM system that writing to kernel text isn't allowed.
	 * If we don't, we might end up COW'ing the text segment!
	 *
	 * XXX Should be atari_trunc_page(&kernel_text) instead
	 * XXX of NBPG.
	 */
	if (vm_map_protect(kernel_map, NBPG, atari_round_page(&etext),
	    VM_PROT_READ|VM_PROT_EXECUTE, TRUE) != KERN_SUCCESS)
		panic("can't protect kernel text");

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	printf("avail mem = %ld (%ld pages)\n", ptoa(cnt.v_free_count),
	    ptoa(cnt.v_free_count)/NBPG);
	printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);
	
	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Configure the system.
	 */
	configure();
}

/*
 * Set registers on exec.
 * XXX Should clear registers except sp, pc,
 * but would break init; should be fixed soon.
 */
void
setregs(p, pack, stack, retval)
	register struct proc *p;
	struct exec_package *pack;
	u_long stack;
	register_t *retval;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;
	
	frame->f_pc = pack->ep_entry & ~1;
	frame->f_regs[SP] = stack;
	frame->f_regs[A2] = (int)PS_STRINGS;

	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;

	if(fputype)
		m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
}

/*
 * Info for CTL_HW
 */
char cpu_model[120];
extern char version[];
 
static void
identifycpu()
{
       char	*mach, *mmu, *fpu, *cpu;

	switch (machineid & ATARI_ANYMACH) {
		case ATARI_TT:
				mach = "Atari TT";
				break;
		case ATARI_FALCON:
				mach = "Atari Falcon";
				break;
		case ATARI_HADES:
				mach = "Atari Hades";
				break;
		default:
				mach = "Atari UNKNOWN";
				break;
	}

	cpu     = "m68k";
	fputype = fpu_probe();
	fpu     = fpu_describe(fputype);

	switch (cputype) {
 
	    case CPU_68060:
		{
			u_int32_t	pcr;
			char		cputxt[30];

			asm(".word 0x4e7a,0x0808;"
			    "movl d0,%0" : "=d"(pcr) : : "d0");
			sprintf(cputxt, "68%s060 rev.%d",
				pcr & 0x10000 ? "LC/EC" : "", (pcr>>8)&0xff);
			cpu = cputxt;
			mmu = "/MMU";
		}
		break;
	case CPU_68040:
		cpu = "m68040";
		mmu = "/MMU";
		break;
	case CPU_68030:
		cpu = "m68030";
		mmu = "/MMU";
		break;
	default: /* XXX */
		cpu = "m68020";
		mmu = " m68851 MMU";
	}
	sprintf(cpu_model, "%s (%s CPU%s%sFPU)", mach, cpu, mmu, fpu);
	printf("%s\n", cpu_model);
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
		return(ENOTDIR);               /* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return(sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
					sizeof(consdev)));
	default:
		return(EOPNOTSUPP);
	}
	/* NOTREACHED */
}

static int waittime = -1;

static void
bootsync(void)
{
	if (waittime < 0) {
		waittime = 0;

		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}
}

void
cpu_reboot(howto, bootstr)
	int	howto;
	char	*bootstr;
{
	/* take a snap shot before clobbering any registers */
	if (curproc && curproc->p_addr)
		savectx(&curproc->p_addr->u_pcb);

	boothowto = howto;
	if((howto & RB_NOSYNC) == 0)
		bootsync();

	/*
	 * Call shutdown hooks. Do this _before_ anything might be
	 * asked to the user in case nobody is there....
	 */
	doshutdownhooks();

	splhigh();			/* extreme priority */
	if(howto & RB_HALT) {
		printf("halted\n\n");
		asm("	stop	#0x2700");
	}
	else {
		if(howto & RB_DUMP)
			dumpsys();

		doboot();
		/*NOTREACHED*/
	}
	panic("Boot() should never come here");
	/*NOTREACHED*/
}

#define	BYTES_PER_DUMP	NBPG		/* Must be a multiple of NBPG	*/
static vm_offset_t	dumpspace;	/* Virt. space to map dumppages	*/

/*
 * Reserve _virtual_ memory to map in the page to be dumped
 */
vm_offset_t
reserve_dumppages(p)
vm_offset_t	p;
{
	dumpspace = p;
	return(p + BYTES_PER_DUMP);
}

unsigned	dumpmag  = 0x8fca0101;	/* magic number for savecore	*/
int		dumpsize = 0;		/* also for savecore (pages)	*/
long		dumplo   = 0;		/* (disk blocks)		*/

void
cpu_dumpconf()
{
	int	nblks, i;

	for (i = dumpsize = 0; i < NMEM_SEGS; i++) {
		if (boot_segs[i].start == boot_segs[i].end)
			break;
		dumpsize += boot_segs[i].end - boot_segs[i].start;
	}
	dumpsize = btoc(dumpsize);

	if (dumpdev != NODEV && bdevsw[major(dumpdev)].d_psize) {
		nblks = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
		if (dumpsize > btoc(dbtob(nblks - dumplo)))
			dumpsize = btoc(dbtob(nblks - dumplo));
		else if (dumplo == 0)
			dumplo = nblks - btodb(ctob(dumpsize));
	}
	dumplo -= cpu_dumpsize();

	/*
	 * Don't dump on the first CLBYTES (why CLBYTES?)
	 * in case the dump device includes a disk label.
	 */
	if (dumplo < btodb(CLBYTES))
		dumplo = btodb(CLBYTES);
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
void
dumpsys()
{
	daddr_t	blkno;		/* Current block to write	*/
	int	(*dump) __P((dev_t, daddr_t, caddr_t, size_t));
				/* Dumping function		*/
	u_long	maddr;		/* PA being dumped		*/
	int	segbytes;	/* Number of bytes in this seg.	*/
	int	segnum;		/* Segment we are dumping	*/
	int	nbytes;		/* Bytes left to dump		*/
	int	i, n, error;

	error = msgbufmapped = segnum = 0;
	if (dumpdev == NODEV)
		return;
	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo < 0)
		return;
	printf("\ndumping to dev %x, offset %ld\n", dumpdev, dumplo);

#if defined(DDB) || defined(PANICWAIT)
	printf("Do you want to dump memory? [y]");
	cnputc(i = cngetc());
	switch (i) {
		case 'n':
		case 'N':
			return;
		case '\n':
			break;
		default :
			cnputc('\n');
	}
#endif /* defined(DDB) || defined(PANICWAIT) */

	maddr    = 0;
	segbytes = boot_segs[0].end;
	blkno    = dumplo;
	dump     = bdevsw[major(dumpdev)].d_dump;
	nbytes   = dumpsize * NBPG;

	printf("dump ");

	error = cpu_dump(dump, &blkno);
	if (!error) {
	    for (i = 0; i < nbytes; i += n, segbytes -= n) {
		/*
		 * Skip the hole
		 */
		if (segbytes == 0) {
		    segnum++;
		    maddr    = boot_segs[segnum].start;
		    segbytes = boot_segs[segnum].end - boot_segs[segnum].start;
		}
		/*
		 * Print Mb's to go
		 */
		n = nbytes - i;
		if (n && (n % (1024*1024)) == 0)
			printf("%d ", n / (1024 * 1024));

		/*
		 * Limit transfer to BYTES_PER_DUMP
		 */
		if (n > BYTES_PER_DUMP)
			n = BYTES_PER_DUMP;

		/*
		 * Map to a VA and write it
		 */
		if (maddr != 0) { /* XXX kvtop chokes on this	*/
			(void)pmap_map(dumpspace, maddr, maddr+n, VM_PROT_READ);
			error = (*dump)(dumpdev, blkno, (caddr_t)dumpspace, n);
			if (error)
				break;
		}

		maddr += n;
		blkno += btodb(n);
	    }
	}
	switch (error) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	default:
		printf("succeeded\n");
		break;
	}
	printf("\n\n");
	delay(5000000);		/* 5 seconds */
}

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time since the last clock interrupt (clock.c:clkread).
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for fun,
 * we guarantee that the time will be greater than the value obtained by a
 * previous call.
 */
void microtime(tvp)
	register struct timeval *tvp;
{
	int s = splhigh();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec += clkread();
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

void
straytrap(pc, evec)
int pc;
u_short evec;
{
	static int	prev_evec;

	printf("unexpected trap (vector offset %x) from %x\n",evec & 0xFFF, pc);

	if(prev_evec == evec) {
		delay(1000000);
		prev_evec = 0;
	}
}

void
straymfpint(pc, evec)
int		pc;
u_short	evec;
{
	printf("unexpected mfp-interrupt (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
}

/*
 * Simulated software interrupt handler
 */
void
softint()
{
	if(ssir & SIR_NET) {
		siroff(SIR_NET);
		cnt.v_soft++;
		netintr();
	}
	if(ssir & SIR_CLOCK) {
		siroff(SIR_CLOCK);
		cnt.v_soft++;
		/* XXXX softclock(&frame.f_stackadj); */
		softclock();
	}
	if (ssir & SIR_CBACK) {
		siroff(SIR_CBACK);
		cnt.v_soft++;
		call_sicallbacks();
	}
}

int	*nofault;

int
badbaddr(addr, size)
	register caddr_t addr;
	int		 size;
{
	register int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return(0);
#endif
	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	switch (size) {
		case 1:
			i = *(volatile char *)addr;
			break;
		case 2:
			i = *(volatile short *)addr;
			break;
		case 4:
			i = *(volatile long *)addr;
			break;
		default:
			panic("badbaddr: unknown size");
	}
	nofault = (int *) 0;
	return(0);
}

/*
 * Network interrupt handling
 */
#include "arp.h"
#include "ppp.h"

#ifdef NPPP
void	pppintr __P((void));
#endif
#ifdef INET
void	ipintr __P((void));
#endif
#ifdef NETATALK
void	atintr __P((void));
#endif
#if NARP > 0
void	arpintr __P((void));
#endif
#ifdef NS
void	nsintr __P((void));
#endif
#ifdef ISO
void	clnlintr __P((void));
#endif

static void
netintr()
{
#ifdef INET
#if NARP > 0
	if (netisr & (1 << NETISR_ARP)) {
		netisr &= ~(1 << NETISR_ARP);
		arpintr();
	}
#endif
	if (netisr & (1 << NETISR_IP)) {
		netisr &= ~(1 << NETISR_IP);
		ipintr();
	}
#endif
#ifdef NETATALK
	if (netisr & (1 << NETISR_ATALK)) {
		netisr &= ~(1 << NETISR_ATALK);
		atintr();
	}
#endif
#ifdef NS
	if (netisr & (1 << NETISR_NS)) {
		netisr &= ~(1 << NETISR_NS);
		nsintr();
	}
#endif
#ifdef ISO
	if (netisr & (1 << NETISR_ISO)) {
		netisr &= ~(1 << NETISR_ISO);
		clnlintr();
	}
#endif
#ifdef NPPP
	if (netisr & (1 << NETISR_PPP)) {
		netisr &= ~(1 << NETISR_PPP);
		pppintr();
	}
#endif
}


/*
 * this is a handy package to have asynchronously executed
 * function calls executed at very low interrupt priority.
 * Example for use is keyboard repeat, where the repeat 
 * handler running at splclock() triggers such a (hardware
 * aided) software interrupt.
 * Note: the installed functions are currently called in a
 * LIFO fashion, might want to change this to FIFO
 * later.
 */
struct si_callback {
	struct si_callback *next;
	void (*function) __P((void *rock1, void *rock2));
	void *rock1, *rock2;
};
static struct si_callback *si_callbacks;
static struct si_callback *si_free;
#ifdef DIAGNOSTIC
static int ncbd;	/* number of callback blocks dynamically allocated */
#endif

void add_sicallback (function, rock1, rock2)
void	(*function) __P((void *rock1, void *rock2));
void	*rock1, *rock2;
{
	struct si_callback	*si;
	int			s;

	/*
	 * this function may be called from high-priority interrupt handlers.
	 * We may NOT block for  memory-allocation in here!.
	 */
	s  = splhigh();
	if((si = si_free) != NULL)
		si_free = si->next;
	splx(s);

	if(si == NULL) {
		si = (struct si_callback *)malloc(sizeof(*si),M_TEMP,M_NOWAIT);
#ifdef DIAGNOSTIC
		if(si)
			++ncbd;		/* count # dynamically allocated */
#endif
		if(!si)
			return;
	}

	si->function = function;
	si->rock1    = rock1;
	si->rock2    = rock2;

	s = splhigh();
	si->next     = si_callbacks;
	si_callbacks = si;
	splx(s);

	/*
	 * and cause a software interrupt (spl1). This interrupt might
	 * happen immediately, or after returning to a safe enough level.
	 */
	setsoftcback();
}

void rem_sicallback(function)
void (*function) __P((void *rock1, void *rock2));
{
	struct si_callback	*si, *psi, *nsi;
	int			s;

	s = splhigh();
	for(psi = 0, si = si_callbacks; si; ) {
		nsi = si->next;

		if(si->function != function)
			psi = si;
		else {
			si->next = si_free;
			si_free  = si;
			if(psi)
				psi->next = nsi;
			else si_callbacks = nsi;
		}
		si = nsi;
	}
	splx(s);
}

/* purge the list */
static void call_sicallbacks()
{
	struct si_callback	*si;
	int			s;
	void			*rock1, *rock2;
	void			(*function) __P((void *, void *));

	do {
		s = splhigh ();
		if ((si = si_callbacks) != NULL)
			si_callbacks = si->next;
		splx(s);

		if (si) {
			function = si->function;
			rock1    = si->rock1;
			rock2    = si->rock2;
			s = splhigh ();
			if(si_callbacks)
				setsoftcback();
			si->next = si_free;
			si_free  = si;
			splx(s);
			function(rock1, rock2);
		}
	} while (si);
#ifdef DIAGNOSTIC
	if (ncbd) {
#ifdef DEBUG
		printf("call_sicallback: %d more dynamic structures\n", ncbd);
#endif
		ncbd = 0;
	}
#endif
}

#if defined(DEBUG) && !defined(PANICBUTTON)
#define PANICBUTTON
#endif

#ifdef PANICBUTTON
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int crashandburn = 0;
int candbdelay = 50;	/* give em half a second */

candbtimer()
{
	crashandburn = 0;
}
#endif

/*
 * should only get here, if no standard executable. This can currently
 * only mean, we're reading an old ZMAGIC file without MID, but since Atari
 * ZMAGIC always worked the `right' way (;-)) just ignore the missing
 * MID and proceed to new zmagic code ;-)
 */
int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error = ENOEXEC;
#ifdef COMPAT_NOMID
	struct exec *execp = epp->ep_hdr;
#endif

#ifdef COMPAT_NOMID
	if (!((execp->a_midmag >> 16) & 0x0fff)
	    && execp->a_midmag == ZMAGIC)
		return(exec_aout_prep_zmagic(p, epp));
#endif
#ifdef COMPAT_SUNOS
	{
		extern sunos_exec_aout_makecmds
		    __P((struct proc *, struct exec_package *));
		if ((error = sunos_exec_aout_makecmds(p, epp)) == 0)
#endif
	return(error);
}

int
bus_space_map(t, bpa, size, cachable, mhp)
bus_space_tag_t		t;
bus_addr_t		bpa;
bus_size_t		size;
int			cachable;
bus_space_handle_t	*mhp;
{
	vm_offset_t	va;
	u_long		pa, endpa;

	pa    = atari_trunc_page(bpa + t);
	endpa = atari_round_page((bpa + t + size) - 1);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("bus_mem_map: overflow");
#endif

	va = kmem_alloc_pageable(kernel_map, endpa - pa);
	if (va == 0)
		return 1;
	*mhp = (caddr_t)(va + (bpa & PGOFSET));

	for(; pa < endpa; pa += NBPG, va += NBPG) {
		pmap_enter(pmap_kernel(), (vm_offset_t)va, pa,
				VM_PROT_READ|VM_PROT_WRITE, TRUE);
		if (!cachable)
			pmap_changebit(pa, PG_CI, TRUE);
	}
	return (0);
}

void
bus_space_unmap(t, memh, size)
bus_space_tag_t		t;
bus_space_handle_t	memh;
bus_size_t		size;
{
	vm_offset_t	va, endva;

	va = atari_trunc_page(memh);
	endva = atari_round_page((memh + size) - 1);

#ifdef DIAGNOSTIC
	if (endva < va)
		panic("unmap_iospace: overflow");
#endif

	kmem_free(kernel_map, va, endva - va);
}
