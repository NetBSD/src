/*-
 * Copyright (c) 1992 Terrence R. Lambert.
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	from: @(#)machdep.c	7.4 (Berkeley) 6/3/91
 *	$Id: machdep.c,v 1.47.2.3 1993/10/06 12:13:48 mycroft Exp $
 */

#include "npx.h"
#include "isa.h"

#include <stddef.h>
#include "param.h"
#include "systm.h"
#include "signalvar.h"
#include "kernel.h"
#include "map.h"
#include "proc.h"
#include "user.h"
#include "exec.h"            /* for PS_STRINGS */
#include "buf.h"
#include "reboot.h"
#include "conf.h"
#include "file.h"
#include "callout.h"
#include "malloc.h"
#include "mbuf.h"
#include "msgbuf.h"
#include "mount.h"
#include "net/netisr.h"

#ifdef SYSVSHM
#include "sys/shm.h"
#endif

#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"

#include "sys/device.h"
#include "sys/exec.h"
#include "sys/vnode.h"

#include "machine/cpu.h"
#include "machine/cpufunc.h"
#include "machine/reg.h"
#include "machine/specialreg.h"
#include "machine/sysarch.h"

#include "i386/isa/isa.h"
#include "i386/isa/isavar.h"
#include "i386/isa/nvram.h"

int _udatasel, _ucodesel;

/*
 * Declare these as initialized data so we can patch them.
 */
int	nswbuf = 0;
#ifdef	NBUF
int	nbuf = NBUF;
#else
int	nbuf = 0;
#endif
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif

int	physmem;
int	boothowto; 
int	cpu_class;

struct	msgbuf *msgbufp;
int	msgbufmapped;

vm_map_t buffer_map;

extern	vm_offset_t avail_start, avail_end;
static	vm_offset_t hole_start, hole_end;
static	vm_offset_t avail_next;
static	vm_size_t avail_remaining;

int	_udatasel, _ucodesel;

void dumpsys __P((void));

void
cpu_startup()
{
	register int unixsize;
	register unsigned i;
	register struct pte *pte;
	int mapaddr, j;
	register caddr_t v;
	int maxbufs, base, residual;
	extern long Usrptsize;
	vm_offset_t minaddr, maxaddr, firstaddr;
	vm_size_t size;

	/*
	 * Initialize error message buffer (at end of core).
	 */

	/* avail_end was pre-decremented in pmap_bootstrap to compensate */
	for (i = 0; i < btoc(sizeof (struct msgbuf)); i++)
		pmap_enter(pmap_kernel(), (caddr_t)msgbufp + i * NBPG,
			   avail_end + i * NBPG, VM_PROT_ALL, TRUE);
	msgbufmapped = 1;

	printf(version);
	identifycpu();
	printf("real mem  = %d\n", ctob(physmem));

	/*
	 * Allocate space for system data structures.
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
/*	valloc(cfree, struct cblock, nclist);  no clists any more!!! - cgd */
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = maxproc * 2);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
	/*
	 * Determine how many buffers to allocate.
	 * Use 10% of memory for the first 2 Meg, 5% of the remaining
	 * memory. Insure a minimum of 16 buffers.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
	    if (physmem < btoc(2 * 1024 * 1024))
		bufpages = physmem / 10 / CLSIZE;
	    else
		bufpages = (btoc(2 * 1024 * 1024) + physmem) / 20 / CLSIZE;

	bufpages = min(NKMEMCLUSTERS*2/5, bufpages);  /* XXX ? - cgd */

	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
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
		firstaddr = (int)kmem_alloc(kernel_map, round_page(size));
		if (firstaddr == 0)
			panic("startup: no room for tables");
		goto again;
	}
	/*
	 * End of second pass, addresses have been assigned
	 */
	if ((vm_size_t)(v - firstaddr) != size)
		panic("startup: table size inconsistency");
#if 0
	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t)&buffers,
				   &maxaddr, size, FALSE);
	minaddr = (vm_offset_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t)0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	if (base >= MAXBSIZE) { /* don't want to alloc more physical mem than needed */
		base = MAXBSIZE;
		residual = 0;
	}
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
#else
	/*
	 * Allocate a submap for buffer space allocations.
	 */
	buffer_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
		bufpages * CLBYTES, TRUE);
#endif

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
#if 0 /* XXX not currently used */
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				16*NCARGS, TRUE);
#endif

	/*
	 * Allocate a submap for physio
	 */
	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, TRUE);

	/*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
	 * we use the more space efficient malloc in place of kmem_alloc.
	 */
	mclrefcnt = (char *)malloc(NMBCLUSTERS+CLBYTES/MCLBYTES,
				   M_MBUF, M_NOWAIT);
	bzero(mclrefcnt, NMBCLUSTERS+CLBYTES/MCLBYTES);
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	printf("avail mem = %d\n", ptoa(vm_page_free_count));
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


struct cpu_nameclass i386_cpus[] = {
	{ "Intel 80286",	CPUCLASS_286 },		/* CPU_286   */
	{ "i386SX",		CPUCLASS_386 },		/* CPU_386SX */
	{ "i386DX",		CPUCLASS_386 },		/* CPU_386   */
	{ "i486SX",		CPUCLASS_486 },		/* CPU_486SX */
	{ "i486DX",		CPUCLASS_486 },		/* CPU_486   */
	{ "i586",		CPUCLASS_586 },		/* CPU_586   */
};

identifycpu()
{
	printf("CPU: ");
	if (cpu >= 0 && cpu < (sizeof i386_cpus/sizeof(struct cpu_nameclass))) {
		printf("%s", i386_cpus[cpu].cpu_name);
		cpu_class = i386_cpus[cpu].cpu_class;
	} else {
#ifdef DIAGNOSTIC
		printf("unknown cpu type %d\n", cpu);
		panic("startup: bad cpu id");
#endif
	}
	printf(" (");
	switch(cpu_class) {
	case CPUCLASS_286:
		printf("286");
		break;
	case CPUCLASS_386:
		printf("386");
		break;
	case CPUCLASS_486:
		printf("486");
		break;
	case CPUCLASS_586:
		printf("586");
		break;
	default:
		printf("unknown");	/* will panic below... */
	}
	printf("-class CPU)");
	printf("\n");	/* cpu speed would be nice, but how? */

	/*
	 * Now that we have told the user what they have,
	 * let them know if that machine type isn't configured.
	 */
	switch (cpu_class) {
	case CPUCLASS_286:	/* a 286 should not make it this far, anyway */
#if !defined(I386_CPU)
	case CPUCLASS_386:
#endif
#if !defined(I486_CPU)
	case CPUCLASS_486:
#endif
#if !defined(I586_CPU)
	case CPUCLASS_586:
#endif
		panic("CPU class not configured");
	default:
		break;
	}
}

#ifdef PGINPROF
/*
 * Return the difference (in microseconds) between the current time and a
 * previous time as represented  by the arguments.  If there is a pending
 * clock interrupt which has not been serviced due to high ipl, return error
 * code.
 */
/*ARGSUSED*/
vmtime(otime, olbolt, oicr)
	register int otime, olbolt, oicr;
{

	return (((time.tv_sec-otime)*HZ + lbolt-olbolt)*(1000000/HZ));
}
#endif

extern int kstack[];

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	unsigned code;
{
	register struct proc *p = curproc;
	register int *regs;
	register struct sigframe *fp;
	struct sigacts *ps = p->p_sigacts;
	int oonstack;
	extern char sigcode[], esigcode[];

	regs = p->p_regs;
	oonstack = ps->ps_onstack;
	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	if (!ps->ps_onstack && (ps->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(ps->ps_sigsp
				- sizeof(struct sigframe));
		ps->ps_onstack = 1;
	} else
		fp = (struct sigframe *)(regs[tESP]
				- sizeof(struct sigframe));

	if ((unsigned)fp <= USRSTACK - ctob(p->p_vmspace->vm_ssize)) 
		(void)grow(p, (unsigned)fp);

	if (useracc((caddr_t)fp, sizeof (struct sigframe), B_WRITE) == 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		SIGACTION(p, SIGILL) = SIG_DFL;
		sig = sigmask(SIGILL);
		p->p_sigignore &= ~sig;
		p->p_sigcatch &= ~sig;
		p->p_sigmask &= ~sig;
		psignal(p, SIGILL);
		return;
	}

	/* 
	 * Build the argument list for the signal handler.
	 */
	fp->sf_signum = sig;
	fp->sf_code = code;
	fp->sf_scp = &fp->sf_sc;
	fp->sf_handler = catcher;

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	fp->sf_sc.sc_onstack = oonstack;
	fp->sf_sc.sc_mask = mask;
	fp->sf_sc.sc_sp = regs[tESP];
	fp->sf_sc.sc_fp = regs[tEBP];
	fp->sf_sc.sc_pc = regs[tEIP];
	fp->sf_sc.sc_ps = regs[tEFLAGS];
	fp->sf_sc.sc_eax = regs[tEAX];
	fp->sf_sc.sc_ebx = regs[tEBX];
	fp->sf_sc.sc_ecx = regs[tECX];
	fp->sf_sc.sc_edx = regs[tEDX];
	fp->sf_sc.sc_esi = regs[tESI];
	fp->sf_sc.sc_edi = regs[tEDI];
	fp->sf_sc.sc_cs = regs[tCS];
	fp->sf_sc.sc_ds = regs[tDS];
	fp->sf_sc.sc_es = regs[tES];
	fp->sf_sc.sc_ss = regs[tSS];

	/*
	 * Build context to run handler in.
	 */
	regs[tESP] = (int)fp;
	regs[tEIP] = (int)(((char *)PS_STRINGS) - (esigcode - sigcode));
	regs[tCS] = _ucodesel;
	regs[tDS] = _udatasel;
	regs[tES] = _udatasel;
	regs[tSS] = _udatasel;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper priviledges or to cause
 * a machine fault.
 */
struct sigreturn_args {
	struct sigcontext *sigcntxp;
};

sigreturn(p, uap, retval)
	struct proc *p;
	struct sigreturn_args *uap;
	int *retval;
{
	register struct sigcontext *scp;
	register struct sigframe *fp;
	register struct trapframe *tf;
	register int *regs = p->p_regs;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = uap->sigcntxp;
	fp = (struct sigframe *)
	     ((caddr_t)scp - offsetof(struct sigframe, sf_sc));

	if (useracc((caddr_t)fp, sizeof(*fp), 0) == 0)
		return(EINVAL);

	if (useracc((caddr_t)scp, sizeof(*scp), 0) == 0)
		return(EINVAL);

	/* make sure they aren't trying to do anything funny */
	if ((scp->sc_ps & PSL_MBZ) != 0 || (scp->sc_ps & PSL_MBO) != PSL_MBO) {
		return(EINVAL);
	}
	/* compare IOPL; we can't insist that it's always 3 or the X server
	   will fail */
	tf = (struct trapframe *)curproc->p_regs;
	if ((tf->tf_eflags & PSL_IOPL) != (regs[tEFLAGS] & PSL_IOPL))
		return(EINVAL);

	p->p_sigacts->ps_onstack = scp->sc_onstack & 01;
	p->p_sigmask = scp->sc_mask &~
	    (sigmask(SIGKILL)|sigmask(SIGCONT)|sigmask(SIGSTOP));

	/*
	 * Restore signal context.
	 */
	regs[tEBP] = scp->sc_ebp;
	regs[tESP] = scp->sc_esp;
	regs[tISP] = scp->sc_isp;
	regs[tEIP] = scp->sc_eip;
	regs[tEFLAGS] = scp->sc_efl;
	regs[tEAX] = scp->sc_eax;
	regs[tEBX] = scp->sc_ebx;
	regs[tECX] = scp->sc_ecx;
	regs[tEDX] = scp->sc_edx;
	regs[tESI] = scp->sc_esi;
	regs[tEDI] = scp->sc_edi;
	regs[tCS] = scp->sc_cs;
	regs[tDS] = scp->sc_ds;
	regs[tES] = scp->sc_es;
	regs[tSS] = scp->sc_ss;
	return(EJUSTRETURN);
}

int	waittime = -1;

void
boot(howto)
	int howto;
{
	extern int cold;

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0 && rootfs) {
		register struct buf *bp;
		int iter, nbusy;

#if 1
		/* XXX protect against curproc->p_stats.foo refs in sync() */
		extern struct proc proc0;
		if (curproc == NULL)
			curproc = &proc0;
#endif

		waittime = 0;
		(void) splnone();
		printf("syncing disks... ");
		/*
		 * Release inodes held by texts before sync.
		 */
		if (panicstr == 0)
			vnode_pager_umount((struct mount *)NULL);
		sync(&proc0, (void *)NULL, (int *)NULL);

		for (iter = 0; iter < 20; iter++) {
			nbusy = 0;
			for (bp = &buf[nbuf]; --bp >= buf; )
				if ((bp->b_flags & (B_BUSY|B_INVAL)) == B_BUSY)
					nbusy++;
			if (nbusy == 0)
				break;
			printf("%d ", nbusy);
			DELAY(40000 * iter);
		}
		if (nbusy)
			printf("giving up\n");
		else
			printf("done\n");
		resettodr();
	}
	(void) splhigh();			/* XXX */
	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
#ifdef DDB
		Debugger();
#else
		cngetc();
#endif
	} else if (howto & RB_DUMP)
		dumpsys();
	printf("rebooting\n\n");
	/* XXX need to pass RB_SINGLE, RB_KDB, and RB_ASKNAME */
	cpu_reset();
	/*NOTREACHED*/
	for(;;);
}

unsigned	dumpmag = 0x8fca0101;	/* magic number for savecore */
vm_size_t	dumpsize = 0;		/* also for savecore */
long		dumplo = 0;

void
dumpconf()
{
	int nblks;

	dumpsize = physmem;
	if (dumpdev != NODEV && bdevsw[major(dumpdev)].d_psize) {
		nblks = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
		/*
		 * Skip first CLBYTES(?) do avoid smashing disk label.
		 */
		if (dumplo < btodb(CLBYTES))
			dumplo = btodb(CLBYTES);
		/*
		 * Make sure dumpsize fits in partition; put padding at
		 * beginning of partition if any.
		 */
		if (dumpsize > btoc(dbtob(nblks - dumplo)))
			dumpsize = btoc(dbtob(nblks - dumplo));
		else if (dumplo + ctod(dumpsize) > nblks)
			dumplo = nblks - ctod(dumpsize);
	}
}

#define BYTES_PER_DUMP (32 * 1024)	/* must be a multiple of pagesize */
static vm_offset_t dumpspace;

/*
 * Used by pmap_bootstrap to reserve space in the pmap for mapping pages
 * during dump.
 */
vm_offset_t
reserve_dumppages(p)
	vm_offset_t p;
{
	dumpspace = p;
	return(p + BYTES_PER_DUMP);
}

caddr_t
dumpvaddr(maddr, n)
	caddr_t maddr;
	int n;
{
	(void) pmap_map(dumpspace, (vm_offset_t)maddr,
			(vm_offset_t)maddr + (vm_size_t)n, VM_PROT_READ);
	return((caddr_t)dumpspace);
}

void
dumpsys()
{
	vm_size_t bytes, i, n;
	vm_offset_t maddr;
	int psize;
	daddr_t blkno;
	int (*dump) __P((dev_t, daddr_t, caddr_t, int));
	int error = 0;

	if (dumpdev == NODEV)
		return;
	if (dumpsize == 0)
		dumpconf();
	if (dumplo < 0)
		return;
	printf("\ndumping to dev %x, offset %d\n", dumpdev, dumplo);

	psize = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}
	/*
	 * We do all the memory mapping *here*.  There is no excuse for each
	 * driver having to know how to do this!
	 */
	bytes = physmem << PGSHIFT;
	maddr = 0;
	blkno = dumplo;
	dump = bdevsw[major(dumpdev)].d_dump;
	for (i = 0; i < bytes; i += n) {
		n = bytes - i;
		if (n > BYTES_PER_DUMP)
			n = BYTES_PER_DUMP;
		if (i && (i % (1024*1024)) == 0)
			printf("%d ", i / (1024*1024));
		error = (*dump)(dumpdev, blkno, (caddr_t)maddr, (int)n);
		if (error)
			break;
		maddr += n;
		blkno += btodb(n);
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

	case EINTR:
		printf("aborted from console\n");
		break;

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("error %d\n", error);
		break;
	}
}

#ifdef HZ
/*
 * If HZ is defined we use this code, otherwise the code in
 * /sys/i386/i386/microtime.s is used.  The othercode only works
 * for HZ=100.
 */
microtime(tvp)
	register struct timeval *tvp;
{
	int s = splhigh();

	*tvp = time;
	tvp->tv_usec += tick;
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	splx(s);
}
#endif /* HZ */

physstrat(bp, strat, prio)
	struct buf *bp;
	int (*strat)(), prio;
{
	register int s;
	caddr_t baddr;

	/*
	 * vmapbuf clobbers b_addr so we must remember it so that it
	 * can be restored after vunmapbuf.  This is truely rude, we
	 * should really be storing this in a field in the buf struct
	 * but none are available and I didn't want to add one at
	 * this time.  Note that b_addr for dirty page pushes is 
	 * restored in vunmapbuf. (ugh!)
	 */
	baddr = bp->b_un.b_addr;
	vmapbuf(bp);
	(*strat)(bp);
	/* pageout daemon doesn't wait for pushed pages */
	if (bp->b_flags & B_DIRTY)
		return;
	s = splbio();
	while ((bp->b_flags & B_DONE) == 0)
		sleep((caddr_t)bp, prio);
	splx(s);
	vunmapbuf(bp);
	bp->b_un.b_addr = baddr;
}

/*
 * Clear registers on exec
 */
void
setregs(p, entry, stack, retval)
	struct proc *p;
	u_long entry;
	u_long stack;
	int retval[2];
{
	p->p_regs[tEBP] = 0;	/* bottom of the fp chain */
	p->p_regs[tEIP] = entry;
	p->p_regs[tESP] = stack;
	p->p_regs[tSS] = _udatasel;
	p->p_regs[tDS] = _udatasel;
	p->p_regs[tES] = _udatasel;
	p->p_regs[tCS] = _ucodesel;
	/* XXX -- do something with retval? */

	p->p_addr->u_pcb.pcb_flags &= 0 /* FM_SYSCTRC */; /* no fp at all */
	load_cr0(rcr0() | CR0_TS);	/* start emulating */
#if	NNPX > 0
	npxinit(__INITIAL_NPXCW__);
#endif
}

/*
 * Initialize 386 and configure to run kernel
 */

/*
 * Initialize segments & interrupt table
 */

#define	GNULL_SEL	0	/* Null Descriptor */
#define	GCODE_SEL	1	/* Kernel Code Descriptor */
#define	GDATA_SEL	2	/* Kernel Data Descriptor */
#define	GLDT_SEL	3	/* LDT - eventually one per process */
#define	GTGATE_SEL	4	/* Process task switch gate */
#define	GPANIC_SEL	5	/* Task state to consider panic from */
#define	GPROC0_SEL	6	/* Task state process slot zero and up */
#define	GUSERLDT_SEL	7	/* User LDT */
#define NGDT 	GUSERLDT_SEL+1

union descriptor gdt[NGDT];

/* interrupt descriptor table */
struct gate_descriptor idt[NIDT];

/* local descriptor table */
#define	LSYS5CALLS_SEL	0	/* forced by intel BCS */
#define	LSYS5SIGR_SEL	1

#define	L43BSDCALLS_SEL	2	/* notyet */
#define	LUCODE_SEL	3
#define	LUDATA_SEL	4
/* seperate stack, es,fs,gs sels ? */
/* #define	LPOSIXCALLS_SEL	5	/* notyet */
#define NLDT		LUDATA_SEL+1

union descriptor ldt[NLDT];

int _default_ldt, currentldt;

struct	i386tss	tss, panic_tss;

extern  struct user *proc0paddr;

/* software prototypes -- in more palatable form */
struct soft_segment_descriptor gdt_segs[] = {
	/* Null Descriptor */
{	0x0,			/* segment base address  */
	0x0,			/* length */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Code Descriptor for kernel */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMERA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
	/* Data Descriptor for kernel */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMRWA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
	/* LDT Descriptor */
{	(int) ldt,			/* segment base address  */
	sizeof(ldt)-1,		/* length - all address space */
	SDT_SYSLDT,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Null Descriptor - Placeholder */
{	0x0,			/* segment base address  */
	0x0,			/* length */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Panic Tss Descriptor */
{	(int) &panic_tss,		/* segment base address  */
	sizeof(tss)-1,		/* length - all address space */
	SDT_SYS386TSS,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Proc 0 Tss Descriptor */
{	(int) kstack,			/* segment base address  */
	sizeof(tss)-1,		/* length - all address space */
	SDT_SYS386TSS,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* User LDT Descriptor per process */
{	(int) ldt,			/* segment base address  */
	(512 * sizeof(union descriptor)-1),		/* length */
	SDT_SYSLDT,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
};

struct soft_segment_descriptor ldt_segs[] = {
	/* Null Descriptor - overwritten by call gate */
{	0x0,			/* segment base address  */
	0x0,			/* length */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Null Descriptor - overwritten by call gate */
{	0x0,			/* segment base address  */
	0x0,			/* length */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Null Descriptor - overwritten by call gate */
{	0x0,			/* segment base address  */
	0x0,			/* length */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0, 0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Code Descriptor for user */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMERA,		/* segment type */
	SEL_UPL,		/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
	/* Data Descriptor for user */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMRWA,		/* segment type */
	SEL_UPL,		/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0, 0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ } };

setidt(idx, func, typ, dpl) char *func; {
	struct gate_descriptor *ip = idt + idx;

	ip->gd_looffset = (int)func;
	ip->gd_selector = 8;
	ip->gd_stkcpy = 0;
	ip->gd_xx = 0;
	ip->gd_type = typ;
	ip->gd_dpl = dpl;
	ip->gd_p = 1;
	ip->gd_hioffset = ((int)func)>>16 ;
}

#define	IDTVEC(name)	__CONCAT(X, name)
extern	IDTVEC(div), IDTVEC(dbg), IDTVEC(nmi), IDTVEC(bpt), IDTVEC(ofl),
	IDTVEC(bnd), IDTVEC(ill), IDTVEC(dna), IDTVEC(dble), IDTVEC(fpusegm),
	IDTVEC(tss), IDTVEC(missing), IDTVEC(stk), IDTVEC(prot),
	IDTVEC(page), IDTVEC(rsvd), IDTVEC(fpu), IDTVEC(rsvd0),
	IDTVEC(rsvd1), IDTVEC(rsvd2), IDTVEC(rsvd3), IDTVEC(rsvd4),
	IDTVEC(rsvd5), IDTVEC(rsvd6), IDTVEC(rsvd7), IDTVEC(rsvd8),
	IDTVEC(rsvd9), IDTVEC(rsvd10), IDTVEC(rsvd11), IDTVEC(rsvd12),
	IDTVEC(rsvd13), IDTVEC(rsvd14), IDTVEC(rsvd14), IDTVEC(syscall);

int lcr0(), lcr3(), rcr0(), rcr2();
int _gsel_tss;

init386(first_avail)
	vm_offset_t first_avail;
{
	extern ssdtosd(), lgdt(), lidt(), lldt(), etext; 
	int x, *pi;
	unsigned biosbasemem, biosextmem;
	struct gate_descriptor *gdp;
	extern char sigcode[], esigcode[];
	/* table descriptors - used to load tables by microp */
	struct region_descriptor r_gdt, r_idt;
	int	pagesinbase, pagesinext;

	proc0.p_addr = proc0paddr;

#ifndef LKM		/* don't do this if we're using LKM's */
	/* make gdt memory segments */
	gdt_segs[GCODE_SEL].ssd_limit = btoc((int) &etext + NBPG);
#endif

	for (x=0; x < NGDT; x++) ssdtosd(gdt_segs+x, gdt+x);
	/* make ldt memory segments */
	ldt_segs[LUCODE_SEL].ssd_limit = btoc(VM_MAXUSER_ADDRESS) - 1;
	ldt_segs[LUDATA_SEL].ssd_limit = btoc(VM_MAXUSER_ADDRESS) - 1;
	/* Note. eventually want private ldts per process */
	for (x=0; x < NLDT; x++) ssdtosd(ldt_segs+x, ldt+x);

	/* exceptions */
	setidt(0, &IDTVEC(div),  SDT_SYS386TGT, SEL_KPL);
	setidt(1, &IDTVEC(dbg),  SDT_SYS386TGT, SEL_KPL);
	setidt(2, &IDTVEC(nmi),  SDT_SYS386TGT, SEL_KPL);
 	setidt(3, &IDTVEC(bpt),  SDT_SYS386TGT, SEL_UPL);
	setidt(4, &IDTVEC(ofl),  SDT_SYS386TGT, SEL_KPL);
	setidt(5, &IDTVEC(bnd),  SDT_SYS386TGT, SEL_KPL);
	setidt(6, &IDTVEC(ill),  SDT_SYS386TGT, SEL_KPL);
	setidt(7, &IDTVEC(dna),  SDT_SYS386TGT, SEL_KPL);
	setidt(8, &IDTVEC(dble),  SDT_SYS386TGT, SEL_KPL);
	setidt(9, &IDTVEC(fpusegm),  SDT_SYS386TGT, SEL_KPL);
	setidt(10, &IDTVEC(tss),  SDT_SYS386TGT, SEL_KPL);
	setidt(11, &IDTVEC(missing),  SDT_SYS386TGT, SEL_KPL);
	setidt(12, &IDTVEC(stk),  SDT_SYS386TGT, SEL_KPL);
	setidt(13, &IDTVEC(prot),  SDT_SYS386TGT, SEL_KPL);
	setidt(14, &IDTVEC(page),  SDT_SYS386TGT, SEL_KPL);
	setidt(15, &IDTVEC(rsvd),  SDT_SYS386TGT, SEL_KPL);
	setidt(16, &IDTVEC(fpu),  SDT_SYS386TGT, SEL_KPL);
	setidt(17, &IDTVEC(rsvd0),  SDT_SYS386TGT, SEL_KPL);
	setidt(18, &IDTVEC(rsvd1),  SDT_SYS386TGT, SEL_KPL);
	setidt(19, &IDTVEC(rsvd2),  SDT_SYS386TGT, SEL_KPL);
	setidt(20, &IDTVEC(rsvd3),  SDT_SYS386TGT, SEL_KPL);
	setidt(21, &IDTVEC(rsvd4),  SDT_SYS386TGT, SEL_KPL);
	setidt(22, &IDTVEC(rsvd5),  SDT_SYS386TGT, SEL_KPL);
	setidt(23, &IDTVEC(rsvd6),  SDT_SYS386TGT, SEL_KPL);
	setidt(24, &IDTVEC(rsvd7),  SDT_SYS386TGT, SEL_KPL);
	setidt(25, &IDTVEC(rsvd8),  SDT_SYS386TGT, SEL_KPL);
	setidt(26, &IDTVEC(rsvd9),  SDT_SYS386TGT, SEL_KPL);
	setidt(27, &IDTVEC(rsvd10),  SDT_SYS386TGT, SEL_KPL);
	setidt(28, &IDTVEC(rsvd11),  SDT_SYS386TGT, SEL_KPL);
	setidt(29, &IDTVEC(rsvd12),  SDT_SYS386TGT, SEL_KPL);
	setidt(30, &IDTVEC(rsvd13),  SDT_SYS386TGT, SEL_KPL);
	setidt(31, &IDTVEC(rsvd14),  SDT_SYS386TGT, SEL_KPL);

	disable_intr();
	r_gdt.rd_limit = sizeof(gdt)-1;
	r_gdt.rd_base = (int) gdt;
	lgdt(&r_gdt);
	r_idt.rd_limit = sizeof(idt)-1;
	r_idt.rd_base = (int) idt;
	lidt(&r_idt);
	_default_ldt = GSEL(GLDT_SEL, SEL_KPL);
	lldt(_default_ldt);
	currentldt = _default_ldt;

#ifdef DDB
	ddb_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef KGDB
	if (boothowto & RB_KDB)
	    kgdb_connect(0);
#endif

	/*
	 * Use BIOS values stored in RTC CMOS RAM, since probing
	 * breaks certain 386 AT relics.
	 */
	biosbasemem = (nvram(NVRAM_BASEMEM_LO)<<0) |
		      (nvram(NVRAM_BASEMEM_HI)<<8);
	biosextmem = (nvram(NVRAM_EXTMEM_LO)<<0) |
		     (nvram(NVRAM_EXTMEM_HI)<<8);

#ifndef BIOS_BASEMEM
#define BIOS_BASEMEM 640
#endif

	if (biosbasemem != BIOS_BASEMEM) {
		printf( "Warning: Base memory %dK, assuming %dK\n", biosbasemem, BIOS_BASEMEM);
		biosbasemem = BIOS_BASEMEM;		/* assume base */
	}

	if (biosextmem > 65536) {
		printf("Warning: Extended memory %dK(>64M), assuming 0K\n",
		       biosextmem);
		biosextmem = 0;				/* assume none */
	}

	avail_start = 0x1000;	/* BIOS leaves data in low memory */
				/* and VM system doesn't work with phys 0 */
	avail_end = biosextmem ? 0x100000 + biosextmem * 1024
			       : biosbasemem * 1024;

	/* number of pages of physmem addr space; XXX first page unused */
	physmem = atop(avail_end) - 1;

	/*
	 *	Initialize for pmap_free_pages and pmap_next_page.
	 *	These guys should be page-aligned.
	 */
	hole_start = biosbasemem * 1024;
	hole_end = round_page((vm_offset_t)first_avail);
	avail_next = avail_start;
	avail_remaining = atop((avail_end - avail_start) -
			       (hole_end - hole_start));

	if (avail_remaining < 2048/4) {
		printf("Too little RAM available; running in degraded mode.\n"
		       "Press a key to confirm.\n\n");
		/*
		 * People with less than 2 Meg have to press a key; this way
		 * we see the messages and can tell them why they blow up later.
		 * If they get working well enough to recompile, they can remove
		 * this; otherwise, it's a toy and they have to lump it.
		 */
		cngetc();
	}

	/* call pmap initialization to make new kernel address space */
	pmap_bootstrap((vm_offset_t)atdevbase + IOM_SIZE);

	/* now running on new page tables, configured,and u/iom is accessible */

	/* make a initial tss so microp can get interrupt stack on syscall! */
	proc0.p_addr->u_pcb.pcb_tss.tss_esp0 = (int) kstack + UPAGES*NBPG;
	proc0.p_addr->u_pcb.pcb_tss.tss_ss0 = GSEL(GDATA_SEL, SEL_KPL) ;
	_gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);

	((struct i386tss *)gdt_segs[GPROC0_SEL].ssd_base)->tss_ioopt = 
		(sizeof(tss))<<16;

	ltr(_gsel_tss);

	/* make a call gate to reenter kernel with */
	gdp = &ldt[LSYS5CALLS_SEL].gd;

	x = (int) &IDTVEC(syscall);
	gdp->gd_looffset = x++;
	gdp->gd_selector = GSEL(GCODE_SEL,SEL_KPL);
	gdp->gd_stkcpy = 1;	/* leaves from for eflags like a trap */
	gdp->gd_type = SDT_SYS386CGT;
	gdp->gd_dpl = SEL_UPL;
	gdp->gd_p = 1;
	gdp->gd_hioffset = ((int) &IDTVEC(syscall)) >>16;

	/* transfer to user mode */

	_ucodesel = LSEL(LUCODE_SEL, SEL_UPL);
	_udatasel = LSEL(LUDATA_SEL, SEL_UPL);

	/* setup proc 0's pcb */
	proc0.p_addr->u_pcb.pcb_flags = 0;
	proc0.p_addr->u_pcb.pcb_ptd = IdlePTD;
}

extern struct pte	*CMAP1, *CMAP2;
extern caddr_t		CADDR1, CADDR2;
/*
 * zero out physical memory
 * specified in relocation units (NBPG bytes)
 */
clearseg(n) {

	*(int *)CMAP2 = PG_V | PG_KW | ctob(n);
	load_cr3(rcr3());
	bzero(CADDR2,NBPG);
}

/*
 * copy a page of physical memory
 * specified in relocation units (NBPG bytes)
 */
void
copyseg(frm, n) {

	*(int *)CMAP2 = PG_V | PG_KW | ctob(n);
	load_cr3(rcr3());
	bcopy((void *)frm, (void *)CADDR2, NBPG);
}

/*
 * copy a page of physical memory
 * specified in relocation units (NBPG bytes)
 */
void
physcopyseg(frm, to) {

	*(int *)CMAP1 = PG_V | PG_KW | ctob(frm);
	*(int *)CMAP2 = PG_V | PG_KW | ctob(to);
	load_cr3(rcr3());
	bcopy(CADDR1, CADDR2, NBPG);
}

/*aston() {
	schednetisr(NETISR_AST);
}*/

void
setsoftclock() {
	schednetisr(NETISR_SCLK);
}

/*
 * insert an element into a queue 
 */
#undef insque
_insque(element, head)
	register struct prochd *element, *head;
{
	element->ph_link = head->ph_link;
	head->ph_link = (struct proc *)element;
	element->ph_rlink = (struct proc *)head;
	((struct prochd *)(element->ph_link))->ph_rlink=(struct proc *)element;
}

/*
 * remove an element from a queue
 */
#undef remque
_remque(element)
	register struct prochd *element;
{
	((struct prochd *)(element->ph_link))->ph_rlink = element->ph_rlink;
	((struct prochd *)(element->ph_rlink))->ph_link = element->ph_link;
	element->ph_rlink = (struct proc *)0;
}

vmunaccess() {}

/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 * 
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 * On the i386, old (386bsd) ZMAGIC binaries and BSDI QMAGIC binaries
 * if COMPAT_NOMID is given as a kernel option.
 */

cpu_exec_aout_makecmds(p, epp)
struct proc *p;
struct exec_package *epp;
{
#ifdef COMPAT_NOMID
  int error;
  u_long midmag, magic;
  u_short mid;

  midmag = ntohl(epp->ep_execp->a_midmag);
  mid = (midmag >> 16 ) & 0xffff;
  magic = midmag & 0xffff;

  if(magic==0) {
    magic = (epp->ep_execp->a_midmag & 0xffff);
    mid = MID_ZERO;
  }

  switch (mid << 16 | magic) {
  /*
   * 386BSD's ZMAGIC format:
   */
  case (MID_ZERO << 16) | ZMAGIC:
    error = cpu_exec_aout_prep_oldzmagic(p, epp);
    break;

  /*
   * BSDI's QMAGIC format:
   * the same as our new ZMAGIC format, but with a different magic number
   */
  case (MID_ZERO << 16) | QMAGIC:
    error = exec_aout_prep_zmagic(p, epp);
    break;

  default:
    error = ENOEXEC;
  }

  return error;
#else /* ! COMPAT_NOMID */
  return ENOEXEC;
#endif
}

#ifdef COMPAT_NOMID
/*
 * cpu_exec_aout_prep_oldzmagic():
 *	Prepare the vmcmds to build a vmspace for an old (386BSD) ZMAGIC
 *	binary.
 *
 * Cloned from exec_aout_prep_zmagic() in kern/exec_aout.c; a more verbose
 * description of operation is there.
 */

int
cpu_exec_aout_prep_oldzmagic(p, epp)
     struct proc *p;
     struct exec_package *epp;
{
  struct exec *execp = epp->ep_execp;
  struct exec_vmcmd *ccmdp;

#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up size fields in epp\n");
#endif
  epp->ep_taddr = 0;
  epp->ep_tsize = execp->a_text;
  epp->ep_daddr = epp->ep_taddr + execp->a_text;
  epp->ep_dsize = execp->a_data + execp->a_bss;
  epp->ep_maxsaddr = USRSTACK - MAXSSIZ;
  epp->ep_minsaddr = USRSTACK;
  epp->ep_ssize = p->p_rlimit[RLIMIT_STACK].rlim_cur;
  epp->ep_entry = execp->a_entry;

  /* check if vnode is in open for writing, because we want to demand-page
   * out of it.  if it is, don't do it, for various reasons
   */
  if ((execp->a_text != 0 || execp->a_data != 0) &&
      (epp->ep_vp->v_flag & VTEXT) == 0 && epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
    if (epp->ep_vp->v_flag & VTEXT)
      panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
    epp->ep_vcp = NULL;
#ifdef EXEC_DEBUG
    printf("exec_prep_oldzmagic: returning with ETXTBSY\n");
#endif
    return ETXTBSY;
  }
  epp->ep_vp->v_flag |= VTEXT;

  /* set up command for text segment */
#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up text segment commands\n");
#endif
  epp->ep_vcp = new_vmcmd(vmcmd_map_pagedvn,
			  execp->a_text,
			  epp->ep_taddr,
			  epp->ep_vp,
			  NBPG,                  /* should this be CLBYTES? */
			  VM_PROT_READ|VM_PROT_EXECUTE);
  ccmdp = epp->ep_vcp;

  /* set up command for data segment */
#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up data segment commands\n");
#endif
  ccmdp->ev_next = new_vmcmd(vmcmd_map_pagedvn,
			     execp->a_data,
			     epp->ep_daddr,
			     epp->ep_vp,
			     execp->a_text + NBPG, /* should be CLBYTES? */
			     VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
  ccmdp = ccmdp->ev_next;

  /* set up command for bss segment */
#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up bss segment commands\n");
#endif
  ccmdp->ev_next = new_vmcmd(vmcmd_map_zero,
			     execp->a_bss,
			     epp->ep_daddr + execp->a_data,
			     0,
			     0,
			     VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
  ccmdp = ccmdp->ev_next;

  /* set up commands for stack.  note that this takes *two*, one
   * to map the part of the stack which we can access, and one
   * to map the part which we can't.
   *
   * arguably, it could be made into one, but that would require
   * the addition of another mapping proc, which is unnecessary
   *
   * note that in memory, thigns assumed to be:
   *    0 ....... ep_maxsaddr <stack> ep_minsaddr
   */
#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up unmapped stack segment commands\n");
#endif
  ccmdp->ev_next = new_vmcmd(vmcmd_map_zero,
			     ((epp->ep_minsaddr - epp->ep_ssize) -
			      epp->ep_maxsaddr),
			     epp->ep_maxsaddr,
			     0,
			     0,
			     VM_PROT_NONE);
  ccmdp = ccmdp->ev_next;
#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: setting up mapped stack segment commands\n");
#endif
  ccmdp->ev_next = new_vmcmd(vmcmd_map_zero,
			     epp->ep_ssize,
			     (epp->ep_minsaddr - epp->ep_ssize),
			     0,
			     0,
			     VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

#ifdef EXEC_DEBUG
  printf("exec_prep_oldzmagic: returning with no error\n");
#endif
  return 0;
}
#endif /* COMPAT_NOMID */

/*
 * The registers are in the frame; the frame is in the user area of
 * the process in question; when the process is active, the registers
 * are in "the kernel stack"; when it's not, they're still there, but
 * things get flipped around.  So, since p->p_regs is the whole address
 * of the register set, take its offset from the kernel stack, and
 * index into the user block.  Don't you just *love* virtual memory?
 * (I'm starting to think seymour is right...)
 */

int
ptrace_set_pc (struct proc *p, unsigned int addr) {
	struct pcb *pcb;
	void *regs = (char*)p->p_addr +
		((char*) p->p_regs - (char*) kstack);

	pcb = &p->p_addr->u_pcb;
	((struct trapframe *)regs)->tf_eip = addr;
	return 0;
}

int
ptrace_single_step (struct proc *p) {
	struct pcb *pcb;
	void *regs = (char*)p->p_addr +
		((char*) p->p_regs - (char*) kstack);

	pcb = &p->p_addr->u_pcb;
	((struct trapframe *)regs)->tf_eflags |= PSL_T;
	return 0;
}
/*
 * Copy the registers to user-space.  This is tedious because
 * we essentially duplicate code for trapframe and syscframe. *sigh*
 */

int
ptrace_getregs (struct proc *p, unsigned int *addr) {
	int error;
	struct trapframe *tp;
	struct pcb *pcb;
	struct regs regs = {0};
	void *ptr = (char*)p->p_addr +
		((char*) p->p_regs - (char*) kstack);

	pcb = &p->p_addr->u_pcb;
	tp = ptr;
	regs.r_es = tp->tf_es;
	regs.r_ds = tp->tf_ds;
	regs.r_edi = tp->tf_edi;
	regs.r_esi = tp->tf_esi;
	regs.r_ebp = tp->tf_ebp;
	regs.r_ebx = tp->tf_ebx;
	regs.r_edx = tp->tf_edx;
	regs.r_ecx = tp->tf_ecx;
	regs.r_eax = tp->tf_eax;
	regs.r_eip = tp->tf_eip;
	regs.r_cs = tp->tf_cs;
	regs.r_eflags = tp->tf_eflags;
	regs.r_esp = tp->tf_esp;
	regs.r_ss = tp->tf_ss;
	return copyout (&regs, addr, sizeof (regs));
}

int
ptrace_setregs (struct proc *p, unsigned int *addr) {
	int error;
	struct trapframe *tp;
	struct pcb *pcb;
	struct regs regs = {0};
	void *ptr = (char*)p->p_addr +
		((char*) p->p_regs - (char*) kstack);

	if (error = copyin (addr, &regs, sizeof(regs)))
		return error;

	pcb = &p->p_addr->u_pcb;
	tp = ptr;
	tp->tf_es = regs.r_es;
	tp->tf_ds = regs.r_ds;
	tp->tf_edi = regs.r_edi;
	tp->tf_esi = regs.r_esi;
	tp->tf_ebp = regs.r_ebp;
	tp->tf_ebx = regs.r_ebx;
	tp->tf_edx = regs.r_edx;
	tp->tf_ecx = regs.r_ecx;
	tp->tf_eax = regs.r_eax;
	tp->tf_eip = regs.r_eip;
	tp->tf_cs = regs.r_cs;
	tp->tf_eflags = regs.r_eflags;
	tp->tf_esp = regs.r_esp;
	tp->tf_ss = regs.r_ss;
	return 0;
}

#ifdef USER_LDT
void
set_user_ldt(struct pcb *pcb)
{
	gdt_segs[GUSERLDT_SEL].ssd_base = (unsigned)pcb->pcb_ldt;
	gdt_segs[GUSERLDT_SEL].ssd_limit = (pcb->pcb_ldt_len * sizeof(union descriptor)) - 1;
	ssdtosd(gdt_segs+GUSERLDT_SEL, gdt+GUSERLDT_SEL);
	lldt(GSEL(GUSERLDT_SEL, SEL_KPL));
	currentldt = GSEL(GUSERLDT_SEL, SEL_KPL);
}

struct i386_get_ldt_args {
	int start;
	union descriptor *desc;
	int num;
};

i386_get_ldt(p, args, retval)
	struct proc *p;
	char *args;
	int *retval;
{
	int error = 0;
	struct pcb *pcb = (struct pcb *)p->p_addr;
	int nldt, num;
	union descriptor *lp;
	int s;
	struct i386_get_ldt_args ua, *uap;

	if ((error = copyin(args, &ua, sizeof(struct i386_get_ldt_args))) < 0)
		return(error);

	uap = &ua;
#ifdef	DEBUG
	printf("i386_get_ldt: start=%d num=%d descs=%x\n", uap->start, uap->num, uap->desc);
#endif

	if (uap->start < 0 || uap->num < 0)
		return(EINVAL);

	s = splhigh();

	if (pcb->pcb_ldt) {
		nldt = pcb->pcb_ldt_len;
		num = min(uap->num, nldt);
		lp = &((union descriptor *)(pcb->pcb_ldt))[uap->start];
	} else {
		nldt = sizeof(ldt)/sizeof(ldt[0]);
		num = min(uap->num, nldt);
		lp = &ldt[uap->start];
	}
	if (uap->start > nldt) {
		splx(s);
		return(EINVAL);
	}

	error = copyout(lp, uap->desc, num * sizeof(union descriptor));
	if (!error)
		*retval = num;

	splx(s);
	return(error);
}

struct i386_set_ldt_args {
	int start;
	union descriptor *desc;
	int num;
};

i386_set_ldt(p, args, retval)
	struct proc *p;
	char *args;
	int *retval;
{
	int error = 0, i, n;
	struct pcb *pcb = (struct pcb *)p->p_addr;
	union descriptor *lp;
	int s;
	struct i386_set_ldt_args ua, *uap;

	if ((error = copyin(args, &ua, sizeof(struct i386_set_ldt_args))) < 0)
		return(error);

	uap = &ua;

#ifdef	DEBUG
	printf("i386_set_ldt: start=%d num=%d descs=%x\n", uap->start, uap->num, uap->desc);
#endif

	if (uap->start < 0 || uap->num < 0)
		return(EINVAL);

	/* XXX Should be 8192 ! */
	if (uap->start > 512 ||
	    (uap->start + uap->num) > 512)
		return(EINVAL);

	/* allocate user ldt */
	if (!pcb->pcb_ldt) {
		union descriptor *new_ldt =
			(union descriptor *)kmem_alloc(kernel_map, 512*sizeof(union descriptor));
		bcopy(ldt, new_ldt, sizeof(ldt));
		pcb->pcb_ldt = (caddr_t)new_ldt;
		pcb->pcb_ldt_len = 512;		/* XXX need to grow */
#ifdef DEBUG
		printf("i386_set_ldt(%d): new_ldt=%x\n", p->p_pid, new_ldt);
#endif
	}

	/* Check descriptors for access violations */
	for (i = 0, n = uap->start; i < uap->num; i++, n++) {
		union descriptor desc, *dp;
		dp = &uap->desc[i];
		error = copyin(dp, &desc, sizeof(union descriptor));
		if (error)
			return(error);

		/* Only user (ring-3) descriptors */
		if (desc.sd.sd_dpl != SEL_UPL)
			return(EACCES);

		/* Must be "present" */
		if (desc.sd.sd_p == 0)
			return(EACCES);

		switch (desc.sd.sd_type) {
		case SDT_SYSNULL:
		case SDT_SYS286CGT:
		case SDT_SYS386CGT:
			break;
		case SDT_MEMRO:
		case SDT_MEMROA:
		case SDT_MEMRW:
		case SDT_MEMRWA:
		case SDT_MEMROD:
		case SDT_MEMRODA:
		case SDT_MEME:
		case SDT_MEMEA:
		case SDT_MEMER:
		case SDT_MEMERA:
		case SDT_MEMEC:
		case SDT_MEMEAC:
		case SDT_MEMERC:
		case SDT_MEMERAC: {
#if 0
			unsigned long base = (desc.sd.sd_hibase << 24)&0xFF000000;
			base |= (desc.sd.sd_lobase&0x00FFFFFF);
			if (base >= KERNBASE)
				return(EACCES);
#endif
			break;
		}
		default:
			return(EACCES);
			/*NOTREACHED*/
		}
	}

	s = splhigh();

	/* Fill in range */
	for (i = 0, n = uap->start; i < uap->num && !error; i++, n++) {
		union descriptor desc, *dp;
		dp = &uap->desc[i];
		lp = &((union descriptor *)(pcb->pcb_ldt))[n];
#ifdef DEBUG
		printf("i386_set_ldt(%d): ldtp=%x\n", p->p_pid, lp);
#endif
		error = copyin(dp, lp, sizeof(union descriptor));
	}
	if (!error) {
		*retval = uap->start;
		need_resched();
	}

	splx(s);
	return(error);
}
#endif	/* USER_LDT */

struct sysarch_args {
	int op;
	char *parms;
};

sysarch(p, uap, retval)
	struct proc *p;
	register struct sysarch_args *uap;
	int *retval;
{
	int error = 0;

	switch(uap->op) {
#ifdef	USER_LDT
	case I386_GET_LDT: 
		error = i386_get_ldt(p, uap->parms, retval);
		break;

	case I386_SET_LDT: 
		error = i386_set_ldt(p, uap->parms, retval);
		break;
#endif
	default:
		error = EINVAL;
		break;
	}
	return(error);
}

/* XXX probably should be in pmap.c */

unsigned int pmap_free_pages()
{
	return avail_remaining;
}

pmap_next_page(addrp)
	vm_offset_t	*addrp;
{
	if (avail_next == avail_end)
		return FALSE;

	/* skip the hole */
	if (avail_next == hole_start)
		avail_next = hole_end;

	*addrp = avail_next;
	avail_next += PAGE_SIZE;
	avail_remaining--;
	return TRUE;
}

pmap_page_index(pa)
	vm_offset_t pa;
{
	if (pa >= avail_start && pa < hole_start)
		return i386_btop(pa - avail_start);
	if (pa >= hole_end && pa < avail_end)
		return i386_btop(pa - hole_end + hole_start - avail_start);
	return -1;
}
