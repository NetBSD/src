/*	$NetBSD: machdep.c,v 1.44 1996/01/31 21:33:58 phil Exp $	*/

/*-
 * Copyright (c) 1996 Matthias Pfaller.
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1993 Philip A. Nelson.
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

static char rcsid[] = "/b/source/CVS/src/sys/arch/pc532/pc532/machdep.c,v 1.2 1993/09/13 07:26:49 phil Exp";

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#include <dev/cons.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/pmap.h>

/*
 * Support for VERY low debugging ... in case we get NO output.
 * e.g. in case pmap does not work and can't do regular mapped
 * output. In this case use umprintf to display debug messages.
 */
#if VERYLOWDEBUG
#include <pc532/umprintf.c> 

/* Inform scncnputc the state of mapping. */
int _mapped = 0;
#endif

/* the following is used externally (sysctl_hw) */
char machine[] = "pc532";
char cpu_model[] = "ns32532";

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

struct	msgbuf *msgbufp;
int	msgbufmapped;

vm_map_t buffer_map;

extern	vm_offset_t avail_start, avail_end;

caddr_t allocsys __P((caddr_t));
void dumpsys __P((void));
void cpu_reset __P((void));

/*
 * Machine-dependent startup code
 */
void
cpu_startup()
{
	unsigned i;
	caddr_t v;
	int sz;
	int base, residual;
	vm_offset_t minaddr, maxaddr;
	vm_size_t size;
	int x;

	/*
	 * Initialize error message buffer (at end of core).
	 */
	/* avail_end was pre-decremented in pmap_bootstrap to compensate */
	for (i = 0; i < btoc(sizeof(struct msgbuf)); i++)
		pmap_enter(pmap_kernel(),
		    (vm_offset_t)((caddr_t)msgbufp + i * NBPG),
		    avail_end + i * NBPG, VM_PROT_ALL, TRUE);
	msgbufmapped = 1;

	printf(version);
	printf("real mem  = %d\n", ctob(physmem));

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	sz = (int)allocsys((caddr_t)0);
	if ((v = (caddr_t)kmem_alloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
	if (allocsys(v) - v != sz)
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
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, TRUE);

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
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
	    VM_MBUF_SIZE, FALSE);

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	printf("avail mem = %d\n", ptoa(cnt.v_free_count));
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
 * Allocate space for system data structures.  We are given
 * a starting virtual address and we return a final virtual
 * address; along the way we set each data structure pointer.
 *
 * We call allocsys() with 0 to find out how much space we want,
 * allocate that much and fill it with zeroes, and then call
 * allocsys() again with the correct base virtual address.
 */
caddr_t
allocsys(v)
	register caddr_t v;
{

#define	valloc(name, type, num) \
	    v = (caddr_t)(((name) = (type *)v) + (num))
#ifdef REAL_CLISTS
	valloc(cfree, struct cblock, nclist);
#endif
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
	 * Determine how many buffers to allocate.  We use 10% of the
	 * first 2MB of memory, and 5% of the rest, with a minimum of 16
	 * buffers.  We allocate 1/2 as many swap buffer headers as file
	 * i/o buffers.
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
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);
	return v;
}

/*  
 * machine dependent system variables.
 */ 
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
	u_long code;
{
	register struct proc *p = curproc;
	register int *regs;
	register struct sigframe *fp;
	struct sigacts *ps = p->p_sigacts;
	int oonstack;
	extern char sigcode[], esigcode[];

	regs = p->p_md.md_regs;
	oonstack = ps->ps_sigstk.ss_flags & SS_ONSTACK;

	/*
	 * Allocate space for the signal handler context.
	 */
	if ((ps->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (ps->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(ps->ps_sigstk.ss_sp +
		    ps->ps_sigstk.ss_size - sizeof(struct sigframe));
		ps->ps_sigstk.ss_flags |= SS_ONSTACK;
	} else {
		fp = (struct sigframe *)regs[REG_SP] - 1;
	}

	if ((unsigned)fp <= (unsigned)p->p_vmspace->vm_maxsaddr + MAXSSIZ - ctob(p->p_vmspace->vm_ssize)) 
		(void)grow(p, (unsigned)fp);

	if (useracc((caddr_t)fp, sizeof (struct sigframe), B_WRITE) == 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
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
	fp->sf_sc.sc_fp = regs[REG_FP];
	fp->sf_sc.sc_sp = regs[REG_SP];
	fp->sf_sc.sc_pc = regs[REG_PC];
	fp->sf_sc.sc_ps = regs[REG_PSR];
	fp->sf_sc.sc_sb = regs[REG_SB];
	bcopy (regs, fp->sf_sc.sc_reg, 8*sizeof(int));

	/*
	 * Build context to run handler in.
	 */
	regs[REG_SP] = (int)fp;
	regs[REG_PC] = (int)(((char *)PS_STRINGS) - (esigcode - sigcode));
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
int
sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_sigreturn_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	register struct sigcontext *scp;
	register int *regs = p->p_md.md_regs;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (useracc((caddr_t)scp, sizeof (*scp), B_READ) == 0)
		return(EINVAL);

	/*
	 * Check for security violations.
	 */
	if (((scp->sc_ps ^ regs[REG_PSR]) & PSL_USERSTATIC) != 0)
		return (EINVAL);

	if (scp->sc_onstack & 01)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
	p->p_sigmask = scp->sc_mask & ~sigcantmask;

	/*
	 * Restore signal context.
	 */
	regs[REG_FP]  = scp->sc_fp;
	regs[REG_SP]  = scp->sc_sp;
	regs[REG_PC]  = scp->sc_pc;
	regs[REG_PSR] = scp->sc_ps;
	regs[REG_SB]  = scp->sc_sb;
	bcopy (scp->sc_reg, regs, 8*sizeof(int));

	return(EJUSTRETURN);
}

int waittime = -1;
struct pcb dumppcb;
struct on_stack dumppcb_onstack;

void
boot(howto)
	int howto;
{
	int s;
	extern int cold;
	extern const char *panicstr;

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
		 * will be out of synch; adjust it now. (non panic!)
		 */
		if (panicstr == 0)
			resettodr();
	}

	/* Disable interrupts. */
	s = splhigh();

	/* If rebooting and a dump is requested do it. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP) {
#if STACK_DUMP
		/* dump the stack! */
		{
			int *fp;
			u_int limit = ns532_round_page(fp) - 40;
			int i=0;
			sprd(fp, fp);
			while ((u_int)fp < limit) {
				printf ("0x%x (@0x%x), ", fp[1], fp);
				fp = (int *)fp[0];
				if (++i == 3) {
					printf("\n");
					i=0;
				}
			}
		}
#endif
		di();
		sprd(sp, dumppcb.pcb_ksp);
		sprd(fp, dumppcb.pcb_kfp);
		smr(ptb0, dumppcb.pcb_ptb);
		dumppcb.pcb_onstack = &dumppcb_onstack;
		sprw(psr, dumppcb_onstack.pcb_psr);
		sprw(mod, dumppcb_onstack.pcb_mod);
		lprd(sp, &dumppcb_onstack.pcb_mod);
		__asm __volatile("bsr 1f; 1: enter [r0,r1,r2,r3,r4,r5,r6,r7],8");
		lprd(sp, dumppcb.pcb_ksp);
		lprd(fp, dumppcb.pcb_kfp);
		sprd(sb, dumppcb_onstack.pcb_sb);
		sprd(usp, dumppcb_onstack.pcb_usp);
		ei();
		dumpsys();
	}

haltsys:

	/*
	 * Call shutdown hooks. Do this _before_ anything might be
	 * asked to the user in case nobody is there....
	 */
	doshutdownhooks();

	if (howto & RB_HALT) {
		printf("\n");
		printf ("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	cpu_reset();
	/*NOTREACHED*/
}

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int 	dumpsize = 0;		/* pages */
long	dumplo = 0; 		/* blocks */

/*
 * This is called by configure to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
dumpconf()
{
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
	if (nblks <= ctod(1))
		return;

	dumpsize = physmem;

	/* Always skip the first CLBYTES, in case there is a label there. */
	if (dumplo < ctod(1))
		dumplo = ctod(1);

	/* Put dump at end of partition, and make it fit. */
	if (dumpsize > dtoc(nblks - dumplo))
		dumpsize = dtoc(nblks - dumplo);
	if (dumplo < nblks - ctod(dumpsize))
		dumplo = nblks - ctod(dumpsize);
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
#define BYTES_PER_DUMP  NBPG	/* must be a multiple of pagesize XXX small */
static vm_offset_t dumpspace;

vm_offset_t
reserve_dumppages(p)
	vm_offset_t p;
{

	dumpspace = p;
	return (p + BYTES_PER_DUMP);
}

void
dumpsys()
{
	unsigned bytes, i, n;
	int maddr, psize;
	daddr_t blkno;
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int error = 0;
	int c;

	msgbufmapped = 0;	/* don't record dump msgs in msgbuf */
	if (dumpdev == NODEV)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
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

#if 0	/* XXX this doesn't work.  grr. */
        /* toss any characters present prior to dump */
	while (sget() != NULL); /*syscons and pccons differ */
#endif

	bytes = ctob(physmem);
	maddr = 0;
	blkno = dumplo;
	dump = bdevsw[major(dumpdev)].d_dump;
	for (i = 0; i < bytes; i += n) {
		/* Print out how many MBs we to go. */
		n = bytes - i;
		if (n && (n % (1024*1024)) == 0)
			printf("%d ", n / (1024 * 1024));

		/* Limit size for next transfer. */
		if (n > BYTES_PER_DUMP)
			n =  BYTES_PER_DUMP;

		(void) pmap_map(dumpspace, maddr, maddr + n, VM_PROT_READ);
		error = (*dump)(dumpdev, blkno, (caddr_t)dumpspace, n);
		if (error)
			break;
		maddr += n;
		blkno += btodb(n);			/* XXX? */

#if 0	/* XXX this doesn't work.  grr. */
		/* operator aborting dump? */
		if (sget() != NULL) {
			error = EINTR;
			break;
		}
#endif
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
	printf("\n\n");
	DELAY(5000000);		/* 5 seconds */
}

void
microtime(tvp)
	register struct timeval *tvp;
{
	int s = splhigh();

	*tvp = time;
	tvp->tv_usec += tick;
	splx(s);
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
}

/*
 * Clear registers on exec
 */
void
setregs(p, pack, stack, retval)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
	register_t *retval;
{
	struct on_stack *r = (struct on_stack *)p->p_md.md_regs;
	struct pcb *pcbp = &p->p_addr->u_pcb;
	extern struct proc *fpu_proc;

	if (p == fpu_proc)
		fpu_proc = 0;

	r->pcb_usp    = stack;
	r->pcb_fp     = 0;
	r->pcb_pc     = pack->ep_entry;
	r->pcb_psr    = PSL_USERSET;
	bzero(r->pcb_reg, sizeof(r->pcb_reg));
	r->pcb_reg[0] = (int)PS_STRINGS;

	pcbp->pcb_fsr = 0;
	bzero(pcbp->pcb_freg, sizeof(pcbp->pcb_freg));

	retval[1] = 0;
}

/*
 * Allocate memory pages.
 */
static vm_offset_t
alloc_pages(pages)
	int pages;
{
	vm_offset_t p = avail_start;
	avail_start += pages * NBPG;
	bzero((caddr_t) p, pages * NBPG);
	return(p);
}

/*
 * Map physical to virtual addresses in the kernel page table directory.
 * If -1 is passed as physical address, empty second level page tables
 * are allocated for the selected virtual address range.
 */
static void
map(pd, virtual, physical, protection, size)
	pd_entry_t *pd;
	vm_offset_t virtual, physical;
	int protection, size;
{
	u_int ix1 = pdei(virtual);
	u_int ix2 = ptei(virtual);
	pt_entry_t *pt = (pt_entry_t *) (pd[ix1] & PG_FRAME);

	while (size > 0) {
		if (pt == 0) {
			pt = (pt_entry_t *) alloc_pages(1);
			pd[ix1] = (pd_entry_t) pt | PG_V | PG_KW;
		}
		if (physical != (vm_offset_t) -1) {
			pt[ix2] = (pt_entry_t) (physical | protection | PG_V);
			physical += NBPG;
			size -= NBPG;
		} else {
			size -= (NPTEPD - ix2) * NBPG;
			ix2 = NPTEPD - 1;
		}
		if (++ix2 == NPTEPD) {
			ix1++;
			ix2 = 0;
			pt = (pt_entry_t *) (pd[ix1] & PG_FRAME);
		}
	}
}

/*
 * init532 is the first (and last) procedure called by locore.s.
 *
 * Level one and level two page tables are initialized to create
 * the following mapping:
 *	0xfdbfe000-0xfdbfffff:	UAREA of process 0
 *	0xfdc00000-0xfdffefff:	Kernel level two page tables
 *	0xfdfff000-0xfdffffff:	Kernel level one page table
 *	0xfe000000-0xff7fffff:	Kernel code and data
 *	0xffc00000-0xffc00fff:	Kernel temporary stack
 *	0xffc80000-0xffc80fff:	Duarts and Parity control
 *	0xffd00000-0xffdfffff:	SCSI polled
 *	0xffe00000-0xffefefff:	SCSI DMA
 *	0xffeff000-0xffefffff:	SCSI DMA with EOP
 *	0xfff00000-0xfff3ffff:	EPROM
 *
 * 0xfe000000-0xfe400000 is (temporary) mirrored at address 0.
 *
 * The intbase register is initialized to point to the interrupt
 * vector table in locore.s.
 *
 * The cpu config register gets set.
 *
 * avail_start, avail_end, physmem, PTDpaddr and proc0paddr are set
 * to the correct values.
 *
 * The last action is to switch stacks and call main.
 */

extern struct user *proc0paddr;

#define kppa(x)	(ns532_round_page(x) & 0xffffff)
#define kvpa(x) (ns532_round_page(x))

void
init532()
{
	extern void icu_init();
	extern int inttab[];
	extern char etext[], edata[], end[];
	pd_entry_t *pd;


#if VERYLOWDEBUG
	umprintf ("Starting init532\n");
#endif

#ifndef NS381
	{
		/* Check if we have a FPU installed. */
		extern int _have_fpu;
		int cfg;
		sprd(cfg, cfg);
		if (cfg & CFG_F)
			_have_fpu = 1;
	}
#endif

	/*
	 * Setup the cfg register.
	 * We enable instruction cache, data cache
	 * the memory management instruction set and
	 * direct exception mode.
	 */
	lprd(cfg, CFG_ONE | CFG_IC | CFG_DC | CFG_DE | CFG_M);

	/* Setup memory allocation variables. */
	avail_start = kppa(end);
	avail_end   = ram_size(avail_start);
	physmem     = btoc(avail_end);

#if VERYLOWDEBUG
	umprintf ("avail_start = 0x%x\navail_end=0x%x\nphysmem=0x%x\n",
		  avail_start, avail_end, physmem);
#endif

	/*
	 * Load the address of the kernel's
	 * trap/interrupt vector table.
	 */
	lprd(intbase, inttab);

	/* Allocate page table directory */
	pd = (pd_entry_t *) alloc_pages(1);
	PTDpaddr = (int)pd;

	/* Recursively map in the page directory */
	pd[PTDPTDI] = (pd_entry_t)pd | PG_V | PG_KW;

	/* Map interrupt stack. */
	map(pd, 0xffc00000, alloc_pages(1), PG_KW, 0x001000);

	/* Map Duarts and Parity. */
	map(pd, 0xffc80000, 0x28000000, PG_KW | PG_N, 0x001000);

	/* Map SCSI Polled (Reduced space). */
	map(pd, 0xffd00000, 0x30000000, PG_KW | PG_N, 0x100000);

	/* Map SCSI DMA (Reduced space). */
	map(pd, 0xffe00000, 0x38000000, PG_KW | PG_N, 0x0ff000);

	/* Map SCSI DMA (With A22 "EOP"). */
	map(pd, 0xffeff000, 0x38400000, PG_KW | PG_N, 0x001000);

	/* Map EPROM (for realtime clock). */
	map(pd, 0xfff00000, 0x10000000, PG_KW | PG_N, 0x040000);

	/* Map the ICU. */
	map(pd, 0xfffff000, 0xfffff000, PG_KW | PG_N, 0x001000);

	/* Map UAREA for proc0. */
	proc0paddr = (struct user *)alloc_pages(UPAGES);
	proc0paddr->u_pcb.pcb_ptb = (int) pd;
	proc0paddr = (struct user *) ((vm_offset_t)proc0paddr + KERNBASE);
	proc0.p_addr = proc0paddr;

	/* Allocate second level page tables for kernel virtual address space */
	map(pd, VM_MIN_KERNEL_ADDRESS, (vm_offset_t)-1, 0,
			VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS);

	/* Map monitor scratch area R/W. */
	map(pd, KERNBASE,        0x00000000, PG_KW, 0x2000);
	/* Map kernel text R/O. */
	map(pd, KERNBASE+0x2000, 0x00002000, PG_KR, kppa(etext) - 0x2000);
	/* Map kernel data+bss R/W. */
	map(pd, kvpa(etext), kppa(etext), PG_KW, avail_start - kppa(etext));

	/* Alias the mapping at KERNBASE to 0 */
	pd[pdei(0)] = pd[pdei(KERNBASE)];

	/* Load the ptb registers and start mapping. */
	load_ptb(pd);
	lmr(mcr, 3);

#if VERYLOWDEBUG
	umprintf ("Just before jump to high memory.\n");
#endif

	/* Jump to high memory */
	__asm __volatile("jump @1f; 1:");

#if VERYLOWDEBUG
	/* Let scncnputc know which form to use. */
	_mapped = 1;
#endif

	/* Set up the ICU. */
	icu_init();
	intr_init();

#ifdef DDB
	ddb_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef KGDB
	if (boothowto & RB_KDB)
		kgdb_connect(0);
#endif

	/* Initialize the pmap module. */
	pmap_bootstrap(avail_start + KERNBASE);

	/* Construct an empty syscframe for proc0. */
	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_onstack = (struct on_stack *)
			      ((u_int)proc0.p_addr + USPACE) - 1;

	/* Switch to proc0's stack. */
	lprd(sp, curpcb->pcb_onstack);
	lprd(fp, 0);

	main(curpcb->pcb_onstack);
	panic("main returned to init532\n");
}

/*
 * insert an element into a queue 
 */
void
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
void
_remque(element)
	register struct prochd *element;
{
	((struct prochd *)(element->ph_link))->ph_rlink = element->ph_rlink;
	((struct prochd *)(element->ph_rlink))->ph_link = element->ph_link;
	element->ph_rlink = (struct proc *)0;
}

/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 *
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 * On the ns532 there are no binary compatibility options (yet),
 * Any takers for Sinix, Genix, SVR2/32000 or Minix?
 */
int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	return ENOEXEC;
}

/*
 * consinit:
 * initialize the system console.
 * XXX - shouldn't deal with this initted thing.
 */
void
consinit()
{
	static int initted;
	if (initted)
		return;
	initted = 1;
	cninit();
}

void
cpu_reset()
{
	/* Mask all ICU interrupts. */
	splhigh();

	/* Disable CPU interrupts. */
	di();

	/* Alias kernel memory at 0. */
	PTD[0] = PTD[pdei(KERNBASE)];
	pmap_update();

	/* Jump to low memory. */
	__asm __volatile(
		"addr 	1f(pc),r0;"
		"andd	~%0,r0;"
		"jump	0(r0);"
		"1:"
		: : "i" (KERNBASE) : "r0"
	);

	/* Turn off mapping. */
	lmr(mcr, 0);

	/* Use monitor scratch area as stack. */
	lprd(sp, 0x2000);

	/* Copy start of ROM. */
	bcopy((void *)0x10000000, (void *)0, 0x1f00);

	/* Jump into ROM copy. */
	__asm __volatile("jump @0");
}
