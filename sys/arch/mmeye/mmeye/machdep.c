/*	$NetBSD: machdep.c,v 1.2 1999/09/14 11:20:54 tsubai Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
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

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_sysv.h"

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
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/syscallargs.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <dev/cons.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/mmeye.h>
#include <sh3/bscreg.h>
#include <sh3/ccrreg.h>
#include <sh3/cpgreg.h>
#include <sh3/intcreg.h>
#include <sh3/pfcreg.h>
#include <sh3/wdtreg.h>

#include <sys/termios.h>

/* the following is used externally (sysctl_hw) */
char machine[] = MACHINE;		/* cpu "architecture" */
char machine_arch[] = MACHINE_ARCH;	/* machine_arch = "sh3" */

#ifdef sh3_debug
int cpu_debug_mode = 1;
#else
int cpu_debug_mode = 0;
#endif

char cpu_model[120];

char bootinfo[BOOTINFO_MAXSIZE];

int physmem;
int dumpmem_low;
int dumpmem_high;
vaddr_t atdevbase;	/* location of start of iomem in virtual */
paddr_t msgbuf_paddr;
struct user *proc0paddr;

vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

extern int boothowto;
extern paddr_t avail_start, avail_end;

#ifdef	SYSCALL_DEBUG
#define	SCDEBUG_ALL 0x0004
extern int	scdebug;
#endif

/*
 * Extent maps to manage I/O and ISA memory hole space.  Allocate
 * storage for 8 regions in each, initially.  Later, ioport_malloc_safe
 * will indicate that it's safe to use malloc() to dynamically allocate
 * region descriptors.
 *
 * N.B. At least two regions are _always_ allocated from the iomem
 * extent map; (0 -> ISA hole) and (end of ISA hole -> end of RAM).
 *
 * The extent maps are not static!  Machine-dependent ISA and EISA
 * routines need access to them for bus address space allocation.
 */
static	long iomem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
struct	extent *ioport_ex;
struct	extent *iomem_ex;
static	int ioport_malloc_safe;

void setup_bootinfo __P((void));
void dumpsys __P((void));
void identifycpu __P((void));
void initSH3 __P((void *));
void InitializeSci  __P((unsigned char));
void Send16550 __P((int c));
void Init16550 __P((void));
void sh3_cache_on __P((void));
void LoadAndReset __P((char *));
void XLoadAndReset __P((char *));
void Sh3Reset __P((void));

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#ifdef COMPAT_NOMID
static int exec_nomid	__P((struct proc *, struct exec_package *));
#endif

/*
 * Machine-dependent startup code
 *
 * This is called from main() in kern/main.c.
 */
void
cpu_startup()
{
	unsigned i;
	caddr_t v;
	int sz;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	struct pcb *pcb;
	char pbuf[9];

	printf(version);

	sprintf(cpu_model, "Hitachi SH3");

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
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
	size = MAXBSIZE * nbuf;
	buffers = 0;
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
		panic("cpu_startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}

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
		curbufsize = CLBYTES * ((i < residual) ? (base+1) : base);

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
				   16*NCARGS, TRUE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, TRUE, FALSE, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_MBUF_SIZE, FALSE, FALSE, NULL);

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * CLBYTES);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/* Safe for i/o port allocation to use malloc now. */
	ioport_malloc_safe = 1;

	curpcb = pcb = &proc0.p_addr->u_pcb;
	pcb->r15 = (int)proc0.p_addr + USPACE - 16;

	proc0.p_md.md_regs = (struct trapframe *)pcb->r15 - 1;

#ifdef SYSCALL_DEBUG
	scdebug |= SCDEBUG_ALL;
#endif

#ifdef FORCE_RB_SINGLE
	boothowto |= RB_SINGLE;
#endif
}

/*
 * Info for CTL_HW
 */
extern	char version[];

#define CPUDEBUG

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
	struct btinfo_bootpath *bibp;
	struct trapframe *tf;
	char *osimage;

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

	case CPU_NKPDE:
		return (sysctl_rdint(oldp, oldlenp, newp, nkpde));

	case CPU_BOOTED_KERNEL:
	        bibp = lookup_bootinfo(BTINFO_BOOTPATH);
	        if (!bibp)
			return(ENOENT); /* ??? */
		return (sysctl_rdstring(oldp, oldlenp, newp, bibp->bootpath));

	case CPU_SETPRIVPROC:
		if (newp == NULL)
			return (0);

		/* set current process to priviledged process */
		tf = p->p_md.md_regs;
		tf->tf_ssr |= PSL_MD;
		return (0);

	case CPU_DEBUGMODE:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
				   &cpu_debug_mode));

	case CPU_LOADANDRESET:
		if (newp != NULL) {
			osimage = (char *)(*(u_long *)newp);

			LoadAndReset(osimage);
			/* not reach here */
		}
		return (0);

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
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct proc *p = curproc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int onstack;

	tf = p->p_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (psp->ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (psp->ps_sigact[sig].sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigframe *)((caddr_t)psp->ps_sigstk.ss_sp +
						  psp->ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)tf->tf_r15;
	fp--;

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;
	frame.sf_handler = catcher;

	/* Save register context. */
	frame.sf_sc.sc_ssr = tf->tf_ssr;
	frame.sf_sc.sc_spc = tf->tf_spc;
	frame.sf_sc.sc_pr = tf->tf_pr;
	frame.sf_sc.sc_r15 = tf->tf_r15;
	frame.sf_sc.sc_r14 = tf->tf_r14;
	frame.sf_sc.sc_r13 = tf->tf_r13;
	frame.sf_sc.sc_r12 = tf->tf_r12;
	frame.sf_sc.sc_r11 = tf->tf_r11;
	frame.sf_sc.sc_r10 = tf->tf_r10;
	frame.sf_sc.sc_r9 = tf->tf_r9;
	frame.sf_sc.sc_r8 = tf->tf_r8;
	frame.sf_sc.sc_r7 = tf->tf_r7;
	frame.sf_sc.sc_r6 = tf->tf_r6;
	frame.sf_sc.sc_r5 = tf->tf_r5;
	frame.sf_sc.sc_r4 = tf->tf_r4;
	frame.sf_sc.sc_r3 = tf->tf_r3;
	frame.sf_sc.sc_r2 = tf->tf_r2;
	frame.sf_sc.sc_r1 = tf->tf_r1;
	frame.sf_sc.sc_r0 = tf->tf_r0;
	frame.sf_sc.sc_trapno = tf->tf_trapno;
#ifdef TODO
	frame.sf_sc.sc_err = tf->tf_err;
#endif

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &frame.sf_sc.__sc_mask13);
#endif

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->tf_spc = (int)psp->ps_sigcode;
#ifdef TODO
	tf->tf_ssr &= ~(PSL_T|PSL_VM|PSL_AC);
#endif
	tf->tf_r15 = (int)fp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 */
int
sys___sigreturn14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext *scp, context;
	struct trapframe *tf;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/* Restore signal context. */
	tf = p->p_md.md_regs;
	{
		/*
		 * Check for security violations.  If we're returning to
		 * protected mode, the CPU will validate the segment registers
		 * automatically and generate a trap on violations.  We handle
		 * the trap, rather than doing all of the checking here.
		 */
#ifdef TODO
	  if (((context.sc_ssr ^ tf->tf_ssr) & PSL_USERSTATIC) != 0) {
	    return (EINVAL);
	  }
#endif

	  tf->tf_ssr = context.sc_ssr;
	}
	tf->tf_r0 = context.sc_r0;
	tf->tf_r1 = context.sc_r1;
	tf->tf_r2 = context.sc_r2;
	tf->tf_r3 = context.sc_r3;
	tf->tf_r4 = context.sc_r4;
	tf->tf_r5 = context.sc_r5;
	tf->tf_r6 = context.sc_r6;
	tf->tf_r7 = context.sc_r7;
	tf->tf_r8 = context.sc_r8;
	tf->tf_r9 = context.sc_r9;
	tf->tf_r10 = context.sc_r10;
	tf->tf_r11 = context.sc_r11;
	tf->tf_r12 = context.sc_r12;
	tf->tf_r13 = context.sc_r13;
	tf->tf_r14 = context.sc_r14;
	tf->tf_spc = context.sc_spc;
	tf->tf_r15 = context.sc_r15;
	tf->tf_pr = context.sc_pr;

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &context.sc_mask, 0);

	return (EJUSTRETURN);
}

int waittime = -1;
int cold = 1;
struct pcb dumppcb;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
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
		/* resettodr(); */
	}

	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	cpu_reset();
	for(;;) ;
	/*NOTREACHED*/
}

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int 	dumpsize = 0;		/* pages */
long	dumplo = 0; 		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{
#ifdef	TODO
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

	dumpsize = btoc(IOM_END + ctob(dumpmem_high));

	/* Always skip the first CLBYTES, in case there is a label there. */
	if (dumplo < ctod(1))
		dumplo = ctod(1);

	/* Put dump at end of partition, and make it fit. */
	if (dumpsize > dtoc(nblks - dumplo))
		dumpsize = dtoc(nblks - dumplo);
	if (dumplo < nblks - ctod(dumpsize))
		dumplo = nblks - ctod(dumpsize);
#endif
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
#define BYTES_PER_DUMP  NBPG	/* must be a multiple of pagesize XXX small */
static vaddr_t dumpspace;

vaddr_t
reserve_dumppages(p)
	vaddr_t p;
{

	dumpspace = p;
	return (p + BYTES_PER_DUMP);
}

void
dumpsys()
{
#ifdef	TODO
	unsigned bytes, i, n;
	int maddr, psize;
	daddr_t blkno;
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int error;

	/* Save registers. */
	savectx(&dumppcb);

	msgbufmapped = 0;	/* don't record dump msgs in msgbuf */
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

	bytes = ctob(dumpmem_high) + IOM_END;
	maddr = 0;
	blkno = dumplo;
	dump = bdevsw[major(dumpdev)].d_dump;
	error = 0;
	for (i = 0; i < bytes; i += n) {
		/*
		 * Avoid dumping the ISA memory hole, and areas that
		 * BIOS claims aren't in low memory.
		 */
		if (i >= ctob(dumpmem_low) && i < IOM_END) {
			n = IOM_END - i;
			maddr += n;
			blkno += btodb(n);
			continue;
		}

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
	delay(5000000);		/* 5 seconds */
#endif	/* TODO */
}

/*
 * Clear registers on exec
 */
void
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	register struct pcb *pcb = &p->p_addr->u_pcb;
	register struct trapframe *tf;

	p->p_md.md_flags &= ~MDP_USEDFPU;
	pcb->pcb_flags = 0;

	tf = p->p_md.md_regs;

	tf->tf_r0 = 0;
	tf->tf_r1 = 0;
	tf->tf_r2 = 0;
	tf->tf_r3 = 0;
	tf->tf_r4 = *(int *)stack;	/* argc */
	tf->tf_r5 = stack+4;		/* argv */
	tf->tf_r6 = stack+4*tf->tf_r4 + 8; /* envp */
	tf->tf_r7 = 0;
	tf->tf_r8 = 0;
	tf->tf_r9 = 0;
	tf->tf_r10 = 0;
	tf->tf_r11 = 0;
	tf->tf_r12 = 0;
	tf->tf_r13 = 0;
	tf->tf_r14 = 0;
	tf->tf_spc = pack->ep_entry;
	tf->tf_ssr = PSL_USERSET;
	tf->tf_r15 = stack;
#ifdef TODO
	tf->tf_r9 = (int)PS_STRINGS;
#endif
}

/*
 * Initialize segments and descriptor tables
 */
#define VBRINIT		((char *)0x8c000000)
#define Trap100Vec	(VBRINIT + 0x100)
#define Trap600Vec	(VBRINIT + 0x600)
#define TLBVECTOR	(VBRINIT + 0x400)
#define VADDRSTART	VM_MIN_KERNEL_ADDRESS

extern int nkpde;
extern char MonTrap100[], MonTrap100_end[];
extern char MonTrap600[], MonTrap600_end[];
extern char _start[], etext[], edata[], end[];
extern char tlbmisshandler_stub[], tlbmisshandler_stub_end[];

void
initSH3(pc)
	void *pc;	/* XXX return address */
{
	paddr_t avail;
	pd_entry_t *pagedir;
	pt_entry_t *pagetab, pte;
	u_int sp;
	int x;
	char *p;

	avail = sh3_round_page(end);

	/*
	 * clear .bss, .common area, page dir area,
	 *	process0 stack, page table area
	 */

	p = (char *)avail + (1 + UPAGES) * NBPG + NBPG * 9;
	bzero(edata, p - edata);

	/*
	 * install trap handler
	 */
	bcopy(MonTrap100, Trap100Vec, MonTrap100_end - MonTrap100);
	bcopy(MonTrap600, Trap600Vec, MonTrap600_end - MonTrap600);
	__asm ("ldc %0, vbr" :: "r"(VBRINIT));

/*
 *                          edata  end
 *	+-------------+------+-----+----------+-------------+------------+
 *	| kernel text | data | bss | Page Dir | Proc0 Stack | Page Table |
 *	+-------------+------+-----+----------+-------------+------------+
 *                                     NBPG       USPACE        9*NBPG
 *                                                (= 4*NBPG)
 *	Build initial page tables
 */
	pagedir = (void *)avail;
	pagetab = (void *)(avail + SYSMAP);
	nkpde = 8;	/* XXX nkpde = kernel page dir area (32 Mbyte) */

	/*
	 *	Construct a page table directory
	 *	In SH3 H/W does not support PTD,
	 *	these structures are used by S/W.
	 */
	pte = (pt_entry_t)pagetab;
	pte |= PG_KW | PG_V | PG_4K | PG_M | PG_N;
	pagedir[KERNTEXTOFF >> PDSHIFT] = pte;

	/* make pde for 0xd0000000, 0xd0400000, 0xd0800000,0xd0c00000,
		0xd1000000, 0xd1400000, 0xd1800000, 0xd1c00000 */
	pte += NBPG;
	for (x = 0; x < nkpde; x++) {
		pagedir[(VADDRSTART >> PDSHIFT) + x] = pte;
		pte += NBPG;
	}

	/* Install a PDE recursively mapping page directory as a page table! */
	pte = (u_int)pagedir;
	pte |= PG_V | PG_4K | PG_KW | PG_M | PG_N;
	pagedir[PDSLOT_PTE] = pte;

	/* set PageDirReg */
	SHREG_TTB = (u_int)pagedir;

	/* Set TLB miss handler */
	p = tlbmisshandler_stub;
	x = tlbmisshandler_stub_end - p;
	bcopy(p, TLBVECTOR, x);

	/*
	 * Activate MMU
	 */
#ifndef ROMIMAGE
	MMEYE_LED = 1;
#endif

#define MMUCR_AT	0x0001	/* address traslation enable */
#define MMUCR_IX	0x0002	/* index mode */
#define MMUCR_TF	0x0004	/* TLB flush */
#define MMUCR_SV	0x0100	/* single virtual space mode */

	SHREG_MMUCR = MMUCR_AT | MMUCR_TF | MMUCR_SV;

	/*
	 * Now here is virtual address
	 */
#ifndef ROMIMAGE
	MMEYE_LED = 0;
#endif

	/* Set proc0paddr */
	proc0paddr = (void *)(avail + NBPG);

	/* Set pcb->PageDirReg of proc0 */
	proc0paddr->u_pcb.pageDirReg = (int)pagedir;

	/* avail_start is first available physical memory address */
	avail_start = avail + NBPG + USPACE + NBPG + NBPG * nkpde;

	/* atdevbase is first available logical memory address */
	atdevbase = VADDRSTART;

	/* MMEYE_LED = 0x01; */

	proc0.p_addr = proc0paddr; /* page dir address */

	/* XXX: PMAP_NEW requires valid curpcb.   also init'd in cpu_startup */
	curpcb = &proc0.p_addr->u_pcb;

	/*
	 * Initialize the I/O port and I/O mem extent maps.
	 * Note: we don't have to check the return value since
	 * creation of a fixed extent map will never fail (since
	 * descriptor storage has already been allocated).
	 *
	 * N.B. The iomem extent manages _all_ physical addresses
	 * on the machine.  When the amount of RAM is found, the two
	 * extents of RAM are allocated from the map (0 -> ISA hole
	 * and end of ISA hole -> end of RAM).
	 */
	iomem_ex = extent_create("iomem", 0x0, 0xffffffff, M_DEVBUF,
	    (caddr_t)iomem_ex_storage, sizeof(iomem_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);

	/* MMEYE_LED = 0x04; */

	consinit();	/* XXX SHOULD NOT BE DONE HERE */

	/* MMEYE_LED = 0x00; */

	splraise(-1);
	enable_intr();

	avail_end = sh3_trunc_page(IOM_RAM_END + 1);

	printf("initSH3\r\n");

	/*
	 * Calculate check sum
	 */
    {
	u_short *p, sum;
	int size;

	size = etext - _start;
	p = (u_short *)_start;
	sum = 0;
	size >>= 1;
	while (size--)
		sum += *p++;
	printf("Check Sum = 0x%x\r\n", sum);
    }
	/*
	 * Allocate the physical addresses used by RAM from the iomem
	 * extent map.  This is done before the addresses are
	 * page rounded just to make sure we get them all.
	 */
	if (extent_alloc_region(iomem_ex, IOM_RAM_BEGIN,
				(IOM_RAM_END-IOM_RAM_BEGIN) + 1,
				EX_NOWAIT)) {
		/* XXX What should we do? */
		printf("WARNING: CAN'T ALLOCATE RAM MEMORY FROM IOMEM EXTENT MAP!\n");
	}

	/* number of pages of physmem addr space */
	physmem = btoc(IOM_RAM_END - IOM_RAM_BEGIN +1);
#ifdef	TODO
	dumpmem = physmem;
#endif

	/*
	 * Initialize for pmap_free_pages and pmap_next_page.
	 * These guys should be page-aligned.
	 */
	if (physmem < btoc(2 * 1024 * 1024)) {
		printf("warning: too little memory available; "
		       "have %d bytes, want %d bytes\n"
		       "running in degraded mode\n"
		       "press a key to confirm\n\n",
		       ctob(physmem), 2*1024*1024);
		cngetc();
	}

	/* Call pmap initialization to make new kernel address space */
	pmap_bootstrap(atdevbase);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	initmsgbuf((caddr_t)msgbuf_paddr, round_page(MSGBUFSIZE));

	/*
	 * set boot device information
	 */
	setup_bootinfo();

#if 0
	sh3_cache_on();
#endif

	/* setup proc0 stack */
	sp = avail + NBPG + USPACE - 16 - sizeof(struct trapframe);

	/*
	 * XXX We can't return here, because we change stack pointer.
	 *     So jump to return address directly.
	 */
	__asm __volatile ("jmp @%0; mov %1, r15" :: "r"(pc), "r"(sp));
}

struct queue {
	struct queue *q_next, *q_prev;
};

/*
 * insert an element into a queue
 */
void
_insque(v1, v2)
	void *v1;
	void *v2;
{
	struct queue *elem = v1, *head = v2;
	struct queue *next;

	next = head->q_next;
	elem->q_next = next;
	head->q_next = elem;
	elem->q_prev = head;
	next->q_prev = elem;
}

/*
 * remove an element from a queue
 */
void
_remque(v)
	void *v;
{
	struct queue *elem = v;
	struct queue *next, *prev;

	next = elem->q_next;
	prev = elem->q_prev;
	next->q_prev = prev;
	prev->q_next = next;
	elem->q_prev = 0;
}

#ifdef COMPAT_NOMID
static int
exec_nomid(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error;
	u_long midmag, magic;
	u_short mid;
	struct exec *execp = epp->ep_hdr;

	/* check on validity of epp->ep_hdr performed by exec_out_makecmds */

	midmag = ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0xffff;
	magic = midmag & 0xffff;

	if (magic == 0) {
		magic = (execp->a_midmag & 0xffff);
		mid = MID_ZERO;
	}

	midmag = mid << 16 | magic;

	switch (midmag) {
	case (MID_ZERO << 16) | ZMAGIC:
		/*
		 * 386BSD's ZMAGIC format:
		 */
		error = exec_aout_prep_oldzmagic(p, epp);
		break;

	case (MID_ZERO << 16) | QMAGIC:
		/*
		 * BSDI's QMAGIC format:
		 * same as new ZMAGIC format, but with different magic number
		 */
		error = exec_aout_prep_zmagic(p, epp);
		break;

	case (MID_ZERO << 16) | NMAGIC:
		/*
		 * BSDI's NMAGIC format:
		 * same as NMAGIC format, but with different magic number
		 * and with text starting at 0.
		 */
		error = exec_aout_prep_oldnmagic(p, epp);
		break;

	case (MID_ZERO << 16) | OMAGIC:
		/*
		 * BSDI's OMAGIC format:
		 * same as OMAGIC format, but with different magic number
		 * and with text starting at 0.
		 */
		error = exec_aout_prep_oldomagic(p, epp);
		break;

	default:
		error = ENOEXEC;
	}

	return error;
}
#endif

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
int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error = ENOEXEC;

#ifdef COMPAT_NOMID
	if ((error = exec_nomid(p, epp)) == 0)
		return error;
#endif /* ! COMPAT_NOMID */

	return error;
}

void
setup_bootinfo(void)
{
	struct btinfo_bootdisk *help;

	*(int *)bootinfo = 1;
	help = (struct btinfo_bootdisk *)(bootinfo + sizeof(int));
	help->biosdev = 0;
	help->partition = 0;
	((struct btinfo_common *)help)->len = sizeof(struct btinfo_bootdisk);
	((struct btinfo_common *)help)->type = BTINFO_BOOTDISK;
}

void *
lookup_bootinfo(type)
	int type;
{
	struct btinfo_common *help;
	int n = *(int*)bootinfo;
	help = (struct btinfo_common *)(bootinfo + sizeof(int));
	while (n--) {
		if (help->type == type)
			return (help);
		help = (struct btinfo_common *)((char*)help + help->len);
	}
	return (0);
}


/*
 * consinit:
 * initialize the system console.
 * XXX - shouldn't deal with this initted thing, but then,
 * it shouldn't be called from init386 either.
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
	ddb_init();
#endif
}

void
cpu_reset()
{
	disable_intr();

	Sh3Reset();
	for (;;);
}

int
bus_space_map (t, addr, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t addr;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	*bshp = (bus_space_handle_t)addr;

	return 0;
}

int
sh_memio_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}

int
sh_memio_alloc(t, rstart, rend, size, alignment, boundary, flags,
	       bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	*bshp = *bpap = rstart;

	return (0);
}

void
sh_memio_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

}

void
sh_memio_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

}

/*
 * InitializeBsc
 * : BSC(Bus State Controler)
 */
void InitializeBsc __P((void));

void
InitializeBsc()
{
	/* MMEYE_LED = 0x01; */

	/* #define NOPCMCIA */
#ifdef NOPCMCIA
	/*
	 * Drive RAS,CAS in stand by mode and bus release mode
	 * Area0 = Normal memory, Area5,6=Normal(no burst)
	 * Area2 = Normal memory, Area3 = DRAM, Area5 = Normal memory
	 * Area4 = Normal Memory
	 * Area6 = Normal memory
	 */
	SHREG_BSC.BCR1.WORD  = 0x1010;
#else
	/*
	 * Drive RAS,CAS in stand by mode and bus release mode
	 * Area0 = Normal memory, Area5,6=Normal(no burst)
	 * Area2 = Normal memory, Area3 = DRAM, Area5 = PCMCIA
	 * Area4 = Normal Memory
	 * Area6 = PCMCIA
	 */
	SHREG_BSC.BCR1.WORD  = 0x1013;
#endif

#define PCMCIA_16
#ifdef PCMCIA_16
	/*
	 * Bus Width
	 * Area4: Bus width = 16bit
	 * Area6,5 = 16bit
	 * Area1 = 8bit
	 * Area2,3: Bus width = 32bit
	 */
	SHREG_BSC.BCR2.WORD  = 0x2af4;
#else
	/*
	 * Bus Width
	 * Area4: Bus width = 16bit
	 * Area6,5 = 8bit
	 * Area1 = 8bit
	 * Area2,3: Bus width = 32bit
	 */
	SHREG_BSC.BCR2.WORD  = 0x16f4;
#endif
	/*
	 * Idle cycle number in transition area and read to write
	 * Area6 = 3, Area5 = 3, Area4 = 3, Area3 = 3, Area2 = 3
	 * Area1 = 3, Area0 = 3
	 */
	SHREG_BSC.WCR1.WORD  = 0x3fff;

#if 0
	/*
	 * Wait cycle
	 * Area 6,5 = 2
	 * Area 4 = 10
	 * Area 3 = 2
	 * Area 2,1 = 3
	 * Area 0 = 6
	 */
	SHREG_BSC.WCR2.WORD  = 0x4bdd;
#else
	/*
	 * Wait cycle
	 * Area 6 = 6
	 * Area 5 = 2
	 * Area 4 = 10
	 * Area 3 = 3
	 * Area 2,1 = 3
	 * Area 0 = 6
	 */
	SHREG_BSC.WCR2.WORD  = 0xabfd;
#endif

	/*
	 * RAS pre-charge = 2cycle, RAS-CAS delay = 3 cycle,
	 * write pre-charge = 1cycle
	 * CAS before RAS refresh RAS assert time = 3  cycle
	 * Disable burst, Bus size=32bit, Column Address=10bit,Refresh ON
	 * CAS before RAS refresh ON, EDO DRAM
	 */
	SHREG_BSC.MCR.WORD   = 0x6135;
	/* SHREG_BSC.MCR.WORD   = 0x4135; */

	/* DRAM Control Register */
	SHREG_BSC.DCR.WORD   = 0x0000;

	/*
	 * PCMCIA Control Register
	 * OE/WE assert delay 3.5 cycle
	 * OE/WE negate-address delay 3.5 cycle
	 */
	SHREG_BSC.PCR.WORD   = 0x00ff;

	/*
	 * Refresh Timer Control/Status Register
	 * Disable interrupt by CMF, closk 1/16, Disable OVF interrupt
	 * Count Limit = 1024
	 * In following statement, the reason why high byte = 0xa5(a4 in RFCR)
	 * is the rule of SH3 in writing these register .
	 */
	SHREG_BSC.RTCSR.WORD = 0xa594;

	/*
	 * Refresh Timer Counter
	 * initialize to 0
	 */
	SHREG_BSC.RTCNT      = 0xa500;

	/*
	 * set Refresh Time Constant Register
	 */
	SHREG_BSC.RTCOR      = 0xa50d;

	/*
	 * init Refresh Count Register
	 */
	SHREG_BSC.RFCR       = 0xa400;

	/*
	 * Set Clock mode (make internal clock double speed)
	 */
#ifdef SH7708R
	SHREG_FRQCR = 0xa100; /* 100MHz */
#else
	SHREG_FRQCR = 0x0112; /* 60MHz */
#endif

#ifndef MMEYE_NO_CACHE
	/* Cache ON */
	SHREG_CCR = 0x0001;
#endif

	/* MMEYE_LED = 0x04; */
}

void
sh3_cache_on(void)
{
#ifndef MMEYE_NO_CACHE
	/* Cache ON */
	SHREG_CCR = 0x0001;
	SHREG_CCR = 0x0009; /* cache clear */
	SHREG_CCR = 0x0001; /* cache on */
#endif
}

#include <machine/mmeye.h>
void
LoadAndReset(osimage)
	char *osimage;
{
	void *buf_addr;
	u_long size;
	u_long *src;
	u_long *dest;
	u_long csum = 0;
	u_long csum2 = 0;
	u_long size2;
#define OSIMAGE_BUF_ADDR 0x8c400000 /* !!!!!! This value depends on physical
				       available memory */

	MMTA_IMASK = 0; /* mask all externel interrupt */

	printf("LoadAndReset: copy start\n");
	buf_addr = (void *)OSIMAGE_BUF_ADDR;

	size = *(u_long *)osimage;
	src = (u_long *)osimage;
	dest = buf_addr;

	size = (size + sizeof(u_long) * 2 + 3) >> 2;
	size2 = size;

	while (size--) {
		csum += *src;
		*dest++ = *src++;
	}

	dest = buf_addr;
	while (size2--)
		csum2 += *dest++;

	printf("LoadAndReset: copy end[%lx,%lx]\n", csum, csum2);
	printf("start XLoadAndReset\n");

	XLoadAndReset(buf_addr);
}

#ifdef sh3_tmp

#define	UART_BASE	0xa4000008

#define	IER	1
#define	IER_RBF	0x01
#define	IER_TBE	0x02
#define	IER_MSI	0x08
#define	FCR	2
#define	LCR	3
#define	LCR_DLAB	0x80
#define	DLM	1
#define	DLL	0
#define	MCR	4
#define	MCR_RTS	0x02
#define	MCR_DTR	0x01
#define	MCR_OUT2	0x08
#define	RTS_MODE	(MCR_RTS|MCR_DTR)
#define	LSR	5
#define	LSR_THRE	0x20
#define	LSR_ERROR	0x1e
#define	THR	0
#define	IIR	2
#define	IIR_II	0x06
#define	IIR_LSI	0x06
#define	IIR_MSI	0x00
#define	IIR_TBE	0x02
#define	IIR_PEND	0x01
#define	RBR	0
#define	MSR	6

#define	OUTP(port, val)	*(volatile unsigned char *)(UART_BASE+port) = val
#define	INP(port)	(*(volatile unsigned char *)(UART_BASE+port))

void
Init16550()
{
	int diviser;
	int tmp;

	/* Set speed */
	/* diviser = 12; */	/* 9600 bps */
	diviser = 6;	/* 19200 bps */

	OUTP(IER, 0);
	/* OUTP(FCR, 0x87); */	/* FIFO mode */
	OUTP(FCR, 0x00);	/* no FIFO mode */

	tmp = INP(LSR);
	tmp = INP(MSR);
	tmp = INP(IIR);
	tmp = INP(RBR);

	OUTP(LCR, INP(LCR) | LCR_DLAB);
	OUTP(DLM, 0xff & (diviser>>8));
	OUTP(DLL, 0xff & diviser);
	OUTP(LCR, INP(LCR) & ~LCR_DLAB);
	OUTP(MCR, 0);

	OUTP(LCR, 0x03);	/* 8 bit , no parity, 1 stop */

	/* start comm */
	OUTP(MCR, RTS_MODE | MCR_OUT2);
	/* OUTP(IER, IER_RBF | IER_TBE | IER_MSI); */
}

void
Send16550(int c)
{
	while (1) {
		OUTP(THR, c);

		while ((INP(LSR) & LSR_THRE) == 0)
			;

		if (c == '\n')
			c = '\r';
		else
			return;
	}
}
#endif /* sh3_tmp */
