/*	$NetBSD: machdep.c,v 1.165.4.1 2000/06/30 16:27:39 simonb Exp $ */

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)machdep.c	8.6 (Berkeley) 1/14/94
 */

#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/extent.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/syscallargs.h>
#include <sys/exec.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <uvm/uvm.h>		/* we use uvm.kernel_object */

#include <sys/sysctl.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/oldmon.h>
#include <machine/bsd_openprom.h>

#include <sparc/sparc/asm.h>
#include <sparc/sparc/cache.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>

#include "fb.h"
#include "power.h"
#include "tctrl.h"

#if NPOWER > 0
#include <sparc/dev/power.h>
#endif
#if NTCTRL > 0
#include <machine/tctrl.h>
#include <sparc/dev/tctrlvar.h>
#endif

vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
extern paddr_t avail_end;

int	physmem;

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int   safepri = 0;

/*
 * dvmamap24 is used to manage DVMA memory for devices that have the upper
 * eight address bits wired to all-ones (e.g. `le' and `ie')
 */
struct extent *dvmamap24;

void	dumpsys __P((void));
void	stackdump __P((void));

caddr_t	mdallocsys __P((caddr_t));

/*
 * Machine-dependent startup code
 */
void
cpu_startup()
{
	unsigned i;
	caddr_t v;
	int base, residual;
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;
#endif
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];

#ifdef DEBUG
	pmapdebug = 0;
#endif

	/*
	 * Map the message buffer (physical location 0).
	 */
	pmap_enter(pmap_kernel(), MSGBUF_VA, 0x0,
	    VM_PROT_READ|VM_PROT_WRITE,
	    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);

	/*
	 * XXX - sun4
	 * Some boot programs mess up physical page 0, which
	 * is where we want to put the msgbuf. There's some
	 * room, so shift it over half a page.
	 */
	initmsgbuf((caddr_t)(MSGBUF_VA + (CPU_ISSUN4 ? 4096 : 0)), MSGBUFSIZE);

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	/*identifycpu();*/
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	size = (vsize_t)allocsys(NULL, mdallocsys);

	if ((v = (caddr_t)uvm_km_alloc(kernel_map, round_page(size))) == 0)
		panic("startup: no room for tables");

	if ((vsize_t)(allocsys(v, mdallocsys) - v) != size)
		panic("startup: table size inconsistency");

        /*
         * allocate virtual and physical memory for the buffers.
         */
        size = MAXBSIZE * nbuf;         /* # bytes for buffers */

        /* allocate VM for buffers... area is not managed by VM system */
        if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
                    NULL, UVM_UNKNOWN_OFFSET,
                    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
                                UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
        	panic("cpu_startup: cannot allocate VM for buffers");

        minaddr = (vaddr_t) buffers;
        if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
        	bufpages = btoc(MAXBSIZE) * nbuf; /* do not overallocate RAM */
        }
        base = bufpages / nbuf;
        residual = bufpages % nbuf;

        /* now allocate RAM for buffers */
	for (i = 0 ; i < nbuf ; i++) {
		vaddr_t curbuf;
		vsize_t curbufsize;
		struct vm_page *pg;

		/*
		 * each buffer has MAXBSIZE bytes of VM space allocated.  of
		 * that MAXBSIZE space we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: "
				    "not enough RAM for buffer cache");
			pmap_enter(kernel_map->pmap, curbuf,
			    VM_PAGE_TO_PHYS(pg), VM_PROT_READ|VM_PROT_WRITE,
			    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
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

	if (CPU_ISSUN4OR4C) {
		/*
		 * Allocate dma map for 24-bit devices (le, ie)
		 * [dvma_base - dvma_end] is for VME devices..
		 */
		dvmamap24 = extent_create("dvmamap24",
					  D24_DVMA_BASE, D24_DVMA_END,
					  M_DEVBUF, 0, 0, EX_NOWAIT);
		if (dvmamap24 == NULL)
			panic("unable to allocate DVMA map");
	}

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
        mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    nmbclusters * mclbytes, VM_MAP_INTRSAFE, FALSE, NULL);

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	pmap_redzone();
}

caddr_t
mdallocsys(v)
	caddr_t v;
{

	/* Clip bufpages if necessary. */
	if (CPU_ISSUN4C && bufpages > (128 * (65536/MAXBSIZE)))
		bufpages = (128 * (65536/MAXBSIZE));

	return (v);
}

/*
 * Set up registers on exec.
 *
 * XXX this entire mess must be fixed
 */
/* ARGSUSED */
void
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct trapframe *tf = p->p_md.md_tf;
	struct fpstate *fs;
	int psr;

	/* Don't allow misaligned code by default */
	p->p_md.md_flags &= ~MDP_FIXALIGN;

	/*
	 * Set the registers to 0 except for:
	 *	%o6: stack pointer, built in exec())
	 *	%psr: (retain CWP and PSR_S bits)
	 *	%g1: address of PS_STRINGS (used by crt0)
	 *	%pc,%npc: entry point of program
	 */
	psr = tf->tf_psr & (PSR_S | PSR_CWP);
	if ((fs = p->p_md.md_fpstate) != NULL) {
		/*
		 * We hold an FPU state.  If we own *the* FPU chip state
		 * we must get rid of it, and the only way to do that is
		 * to save it.  In any case, get rid of our FPU state.
		 */
		if (p == cpuinfo.fpproc) {
			savefpstate(fs);
			cpuinfo.fpproc = NULL;
		} else if (p->p_md.md_fpumid != -1)
			panic("setreg: own FPU on module %d; fix this",
				p->p_md.md_fpumid);
		p->p_md.md_fpumid = -1;
		free((void *)fs, M_SUBPROC);
		p->p_md.md_fpstate = NULL;
	}
	bzero((caddr_t)tf, sizeof *tf);
	tf->tf_psr = psr;
	tf->tf_global[1] = (int)PS_STRINGS;
	tf->tf_pc = pack->ep_entry & ~3;
	tf->tf_npc = tf->tf_pc + 4;
	stack -= sizeof(struct rwindow);
	tf->tf_out[6] = stack;
}

#ifdef DEBUG
int sigdebug = 0;
int sigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

struct sigframe {
	int	sf_signo;		/* signal number */
	int	sf_code;		/* code */
	struct	sigcontext *sf_scp;	/* SunOS user addr of sigcontext */
	int	sf_addr;		/* SunOS compat, always 0 for now */
	struct	sigcontext sf_sc;	/* actual sigcontext */
};

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
	char *cp;

	/* all sysctl names are this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);	/* overloaded */

	switch (name[0]) {
	case CPU_BOOTED_KERNEL:
		cp = prom_getbootfile();
		if (cp == NULL || cp[0] == '\0')
			return (ENOENT);
		return (sysctl_rdstring(oldp, oldlenp, newp, cp));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * Send an interrupt to process.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct proc *p = curproc;
	struct sigacts *psp = p->p_sigacts;
	struct sigframe *fp;
	struct trapframe *tf;
	int addr, onstack, oldsp, newsp;
	struct sigframe sf;

	tf = p->p_md.md_tf;
	oldsp = tf->tf_out[6];

	/*
	 * Compute new user stack addresses, subtract off
	 * one signal frame, and align.
	 */
	onstack =
	    (psp->ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (psp->ps_sigact[sig].sa_flags & SA_ONSTACK) != 0;

	if (onstack)
		fp = (struct sigframe *)((caddr_t)psp->ps_sigstk.ss_sp +
		                                  psp->ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)oldsp;

	fp = (struct sigframe *)((int)(fp - 1) & ~7);

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig: %s[%d] sig %d newusp %p scp %p\n",
		    p->p_comm, p->p_pid, sig, fp, &fp->sf_sc);
#endif
	/*
	 * Now set up the signal frame.  We build it in kernel space
	 * and then copy it out.  We probably ought to just build it
	 * directly in user space....
	 */
	sf.sf_signo = sig;
	sf.sf_code = code;
	sf.sf_scp = 0;
	sf.sf_addr = 0;			/* XXX */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	sf.sf_sc.sc_onstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;
	sf.sf_sc.sc_mask = *mask;
#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &sf.sf_sc.__sc_mask13);
#endif
	sf.sf_sc.sc_sp = oldsp;
	sf.sf_sc.sc_pc = tf->tf_pc;
	sf.sf_sc.sc_npc = tf->tf_npc;
	sf.sf_sc.sc_psr = tf->tf_psr;
	sf.sf_sc.sc_g1 = tf->tf_global[1];
	sf.sf_sc.sc_o0 = tf->tf_out[0];

	/*
	 * Put the stack in a consistent state before we whack away
	 * at it.  Note that write_user_windows may just dump the
	 * registers into the pcb; we need them in the process's memory.
	 * We also need to make sure that when we start the signal handler,
	 * its %i6 (%fp), which is loaded from the newly allocated stack area,
	 * joins seamlessly with the frame it was in when the signal occurred,
	 * so that the debugger and _longjmp code can back up through it.
	 */
	newsp = (int)fp - sizeof(struct rwindow);
	write_user_windows();
	if (rwindow_save(p) || copyout((caddr_t)&sf, (caddr_t)fp, sizeof sf) ||
	    suword(&((struct rwindow *)newsp)->rw_in[6], oldsp)) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sendsig: window save or copyout error\n");
#endif
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sendsig: %s[%d] sig %d scp %p\n",
		       p->p_comm, p->p_pid, sig, &fp->sf_sc);
#endif
	/*
	 * Arrange to continue execution at the code copied out in exec().
	 * It needs the function to call in %g1, and a new stack pointer.
	 */
	addr = (int)psp->ps_sigcode;
	tf->tf_global[1] = (int)catcher;
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = newsp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig: about to return to catcher\n");
#endif
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above),
 * and return to the given trap frame (if there is one).
 * Check carefully to make sure that the user has not
 * modified the state to gain improper privileges or to cause
 * a machine fault.
 */
/* ARGSUSED */
int
sys___sigreturn14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext sc, *scp;
	struct trapframe *tf;
	int error;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(p))
		sigexit(p, SIGILL);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
#endif
	if ((error = copyin(SCARG(uap, sigcntxp), &sc, sizeof sc)) != 0)
		return (error);
	scp = &sc;

	tf = p->p_md.md_tf;
	/*
	 * Only the icc bits in the psr are used, so it need not be
	 * verified.  pc and npc must be multiples of 4.  This is all
	 * that is required; if it holds, just do it.
	 */
	if (((scp->sc_pc | scp->sc_npc) & 3) != 0)
		return (EINVAL);

	/* take only psr ICC field */
	tf->tf_psr = (tf->tf_psr & ~PSR_ICC) | (scp->sc_psr & PSR_ICC);
	tf->tf_pc = scp->sc_pc;
	tf->tf_npc = scp->sc_npc;
	tf->tf_global[1] = scp->sc_g1;
	tf->tf_out[0] = scp->sc_o0;
	tf->tf_out[6] = scp->sc_sp;

	if (scp->sc_onstack & SS_ONSTACK)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask */
	(void) sigprocmask1(p, SIG_SETMASK, &scp->sc_mask, 0);

	return (EJUSTRETURN);
}


int	waittime = -1;

void
cpu_reboot(howto, user_boot_string)
	int howto;
	char *user_boot_string;
{
	int i;
	static char str[128];

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

#if NFB > 0
	fb_unblank();
#endif
	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		extern struct proc proc0;

		/* XXX protect against curproc->p_stats.foo refs in sync() */
		if (curproc == NULL)
			curproc = &proc0;
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	(void) splhigh();

	/* If rebooting and a dump is requested, do it. */
#if 0
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
#else
	if (howto & RB_DUMP)
#endif
		dumpsys();

 haltsys:

	/* Run any shutdown hooks. */
	doshutdownhooks();

	/* If powerdown was requested, do it. */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
#if NPOWER > 0
		powerdown();
#endif
#if NTCTRL > 0
		tadpole_powerdown();
#endif
#if NPOWER > 0 || NTCTRL > 0
		printf("WARNING: powerdown failed!\n");
#endif
		/*
		 * RB_POWERDOWN implies RB_HALT... fall into it...
		 */
	}

	if (howto & RB_HALT) {
		printf("halted\n\n");
		prom_halt();
	}

	printf("rebooting\n\n");
	if (user_boot_string && *user_boot_string) {
		i = strlen(user_boot_string);
		if (i > sizeof(str))
			prom_boot(user_boot_string);	/* XXX */
		bcopy(user_boot_string, str, i);
	} else {
		i = 1;
		str[0] = '\0';
	}

	if (howto & RB_SINGLE)
		str[i++] = 's';
	if (howto & RB_KDB)
		str[i++] = 'd';
	if (i > 1) {
		if (str[0] == '\0')
			str[0] = '-';
		str[i] = 0;
	} else
		str[0] = 0;
	prom_boot(str);
	/*NOTREACHED*/
}

u_long	dumpmag = 0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
long	dumplo = 0;

void
cpu_dumpconf()
{
	int nblks, dumpblks;

	if (dumpdev == NODEV || bdevsw[major(dumpdev)].d_psize == 0)
		/* No usable dump device */
		return;

	nblks = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);

	dumpblks = ctod(physmem) + pmap_dumpsize();
	if (dumpblks > (nblks - ctod(1)))
		/*
		 * dump size is too big for the partition.
		 * Note, we safeguard a click at the front for a
		 * possible disk label.
		 */
		return;

	/* Put the dump at the end of the partition */
	dumplo = nblks - dumpblks;

	/*
	 * savecore(8) expects dumpsize to be the number of pages
	 * of actual core dumped (i.e. excluding the MMU stuff).
	 */
	dumpsize = physmem;
}

#define	BYTES_PER_DUMP	(32 * 1024)	/* must be a multiple of pagesize */
static vaddr_t dumpspace;

caddr_t
reserve_dumppages(p)
	caddr_t p;
{

	dumpspace = (vaddr_t)p;
	return (p + BYTES_PER_DUMP);
}

/*
 * Write a crash dump.
 */
void
dumpsys()
{
	int psize;
	daddr_t blkno;
	int (*dump)	__P((dev_t, daddr_t, caddr_t, size_t));
	int error = 0;
	struct memarr *mp;
	int nmem;
	extern struct memarr pmemarr[];
	extern int npmemarr;

	/* copy registers to memory */
	snapshot(cpuinfo.curpcb);
	stackdump();

	if (dumpdev == NODEV)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	psize = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}
	blkno = dumplo;
	dump = bdevsw[major(dumpdev)].d_dump;

	error = pmap_dumpmmu(dump, blkno);
	blkno += pmap_dumpsize();

	for (mp = pmemarr, nmem = npmemarr; --nmem >= 0 && error == 0; mp++) {
		unsigned i = 0, n;
		int maddr = mp->addr;

		if (maddr == 0) {
			/* Skip first page at physical address 0 */
			maddr += NBPG;
			i += NBPG;
			blkno += btodb(NBPG);
		}

		for (; i < mp->len; i += n) {
			n = mp->len - i;
			if (n > BYTES_PER_DUMP)
				 n = BYTES_PER_DUMP;

			/* print out how many MBs we have dumped */
			if (i && (i % (1024*1024)) == 0)
				printf("%d ", i / (1024*1024));

			(void) pmap_map(dumpspace, maddr, maddr + n,
					VM_PROT_READ);
			error = (*dump)(dumpdev, blkno,
					(caddr_t)dumpspace, (int)n);
			pmap_remove(pmap_kernel(), dumpspace, dumpspace + n);
			if (error)
				break;
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

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("error %d\n", error);
		break;
	}
}

/*
 * get the fp and dump the stack as best we can.  don't leave the
 * current stack page
 */
void
stackdump()
{
	struct frame *fp = getfp(), *sfp;

	sfp = fp;
	printf("Frame pointer is at %p\n", fp);
	printf("Call traceback:\n");
	while (fp && ((u_long)fp >> PGSHIFT) == ((u_long)sfp >> PGSHIFT)) {
		printf("  pc = 0x%x  args = (0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) fp = %p\n",
		    fp->fr_pc, fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2],
		    fp->fr_arg[3], fp->fr_arg[4], fp->fr_arg[5], fp->fr_arg[6],
		    fp->fr_fp);
		fp = fp->fr_fp;
	}
}

int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	return (ENOEXEC);
}

#ifdef SUN4
void
oldmon_w_trace(va)
	u_long va;
{
	u_long stop;
	struct frame *fp;

	if (curproc)
		printf("curproc = %p, pid %d\n", curproc, curproc->p_pid);
	else
		printf("no curproc\n");

	printf("uvm: swtch %d, trap %d, sys %d, intr %d, soft %d, faults %d\n",
	    uvmexp.swtch, uvmexp.traps, uvmexp.syscalls, uvmexp.intrs,
		uvmexp.softs, uvmexp.faults);
	write_user_windows();

#define round_up(x) (( (x) + (NBPG-1) ) & (~(NBPG-1)) )

	printf("\nstack trace with sp = 0x%lx\n", va);
	stop = round_up(va);
	printf("stop at 0x%lx\n", stop);
	fp = (struct frame *) va;
	while (round_up((u_long) fp) == stop) {
		printf("  0x%x(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) fp %p\n", fp->fr_pc,
		    fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3],
		    fp->fr_arg[4], fp->fr_arg[5], fp->fr_arg[6], fp->fr_fp);
		fp = fp->fr_fp;
		if (fp == NULL)
			break;
	}
	printf("end of stack trace\n");
}

void
oldmon_w_cmd(va, ar)
	u_long va;
	char *ar;
{
	switch (*ar) {
	case '\0':
		switch (va) {
		case 0:
			panic("g0 panic");
		case 4:
			printf("w: case 4\n");
			break;
		default:
			printf("w: unknown case %ld\n", va);
			break;
		}
		break;
	case 't':
		oldmon_w_trace(va);
		break;
	default:
		printf("w: arg not allowed\n");
	}
}
#endif /* SUN4 */

int
ldcontrolb(addr)
caddr_t addr;
{
	struct pcb *xpcb;
	extern struct user *proc0paddr;
	u_long saveonfault;
	int res;
	int s;

	if (CPU_ISSUN4M) {
		printf("warning: ldcontrolb called in sun4m\n");
		return 0;
	}

	s = splhigh();
	if (curproc == NULL)
		xpcb = (struct pcb *)proc0paddr;
	else
		xpcb = &curproc->p_addr->u_pcb;

	saveonfault = (u_long)xpcb->pcb_onfault;
        res = xldcontrolb(addr, xpcb);
	xpcb->pcb_onfault = (caddr_t)saveonfault;

	splx(s);
	return (res);
}

void
wzero(vb, l)
	void *vb;
	u_int l;
{
	u_char *b = vb;
	u_char *be = b + l;
	u_short *sp;

	if (l == 0)
		return;

	/* front, */
	if ((u_long)b & 1)
		*b++ = 0;

	/* back, */
	if (b != be && ((u_long)be & 1) != 0) {
		be--;
		*be = 0;
	}

	/* and middle. */
	sp = (u_short *)b;
	while (sp != (u_short *)be)
		*sp++ = 0;
}

void
wcopy(vb1, vb2, l)
	const void *vb1;
	void *vb2;
	u_int l;
{
	const u_char *b1e, *b1 = vb1;
	u_char *b2 = vb2;
	u_short *sp;
	int bstore = 0;

	if (l == 0)
		return;

	/* front, */
	if ((u_long)b1 & 1) {
		*b2++ = *b1++;
		l--;
	}

	/* middle, */
	sp = (u_short *)b1;
	b1e = b1 + l;
	if (l & 1)
		b1e--;
	bstore = (u_long)b2 & 1;

	while (sp < (u_short *)b1e) {
		if (bstore) {
			b2[1] = *sp & 0xff;
			b2[0] = *sp >> 8;
		} else
			*((short *)b2) = *sp;
		sp++;
		b2 += 2;
	}

	/* and back. */
	if (l & 1)
		*b2 = *b1e;
}


/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct sparc_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allocate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources,
	 * and they are not to be freed.
	 *
	 * The bus_dmamap_t includes one bus_dma_segment_t, hence
	 * the (nsegments - 1).
	 */
	mapsize = sizeof(struct sparc_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	bzero(mapstore, mapsize);
	map = (struct sparc_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
_bus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{

	free(map, M_DMAMAP);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf(t, map, m, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m;
	int flags;
{

	panic("_bus_dmamap_load: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{

	panic("_bus_dmamap_load_uio: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for DMA map synchronization.  May be called
 * by bus-specific DMA map synchronization functions.
 */
void
_bus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	vaddr_t low, high;
	struct pglist *mlist;
	int error;

	/* Always round the size. */
	size = round_page(size);
	low = vm_first_phys;
	high = vm_first_phys + vm_num_phys - PAGE_SIZE;

	if ((mlist = malloc(sizeof(*mlist), M_DEVBUF,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	/*
	 * Allocate pages from the VM system.
	 */
	TAILQ_INIT(mlist);
	error = uvm_pglistalloc(size, low, high, 0, 0,
				mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return (error);

	/*
	 * Simply keep a pointer around to the linked list, so
	 * bus_dmamap_free() can return it.
	 *
	 * NOBODY SHOULD TOUCH THE pageq FIELDS WHILE THESE PAGES
	 * ARE IN OUR CUSTODY.
	 */
	segs[0]._ds_mlist = mlist;

	/*
	 * We now have physical pages, but no DVMA addresses yet. These
	 * will be allocated in bus_dmamap_load*() routines. Hence we
	 * save any alignment and boundary requirements in this dma
	 * segment.
	 */
	segs[0].ds_addr = 0;
	segs[0].ds_len = 0;
	segs[0]._ds_va = 0;
	*rsegs = 1;
	return (0);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{

	if (nsegs != 1)
		panic("bus_dmamem_free: nsegs = %d", nsegs);

	/*
	 * Return the list of pages back to the VM system.
	 */
	uvm_pglistfree(segs[0]._ds_mlist);
	free(segs[0]._ds_mlist, M_DEVBUF);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(t, kva, size)
	bus_dma_tag_t t;
	caddr_t kva;
	size_t size;
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PAGE_MASK)
		panic("_bus_dmamem_unmap");
#endif

	size = round_page(size);
	uvm_unmap(kernel_map, (vaddr_t)kva, (vaddr_t)kva + size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
_bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	off_t off;
	int prot, flags;
{

	panic("_bus_dmamem_mmap: not implemented");
}

/*
 * Utility to allocate an aligned kernel virtual address range
 */
vaddr_t
_bus_dma_valloc_skewed(size, boundary, align, skew)
	size_t size;
	u_long boundary;
	u_long align;
	u_long skew;
{
	size_t oversize;
	vaddr_t va, sva;

	/*
	 * Find a region of kernel virtual addresses that is aligned
	 * to the given address modulo the requested alignment, i.e.
	 *
	 *	(va - skew) == 0 mod align
	 *
	 * The following conditions apply to the arguments:
	 *
	 *	- `size' must be a multiple of the VM page size
	 *	- `align' must be a power of two
	 *	   and greater than or equal to the VM page size
	 *	- `skew' must be smaller than `align'
	 *	- `size' must be smaller than `boundary'
	 */

#ifdef DIAGNOSTIC
	if ((size & PAGE_MASK) != 0)
		panic("_bus_dma_valloc_skewed: invalid size %lx", size);
	if ((align & PAGE_MASK) != 0)
		panic("_bus_dma_valloc_skewed: invalid alignment %lx", align);
	if (align < skew)
		panic("_bus_dma_valloc_skewed: align %lx < skew %lx",
			align, skew);
#endif

	/* XXX - Implement this! */
	if (boundary)
		panic("_bus_dma_valloc_skewed: not implemented");

	/*
	 * First, find a region large enough to contain any aligned chunk
	 */
	oversize = size + align - PAGE_SIZE;
	sva = uvm_km_valloc(kernel_map, oversize);
	if (sva == 0)
		return (ENOMEM);

	/*
	 * Compute start of aligned region
	 */
	va = sva;
	va += (skew + align - va) & (align - 1);

	/*
	 * Return excess virtual addresses
	 */
	if (va != sva)
		(void)uvm_unmap(kernel_map, sva, va);
	if (va + size != sva + oversize)
		(void)uvm_unmap(kernel_map, va + size, sva + oversize);

	return (va);
}

/* sun4/sun4c dma map functions */
int	sun4_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
				bus_size_t, struct proc *, int));
int	sun4_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
				bus_dma_segment_t *, int, bus_size_t, int));
void	sun4_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
int	sun4_dmamem_map __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
				int nsegs, size_t size, caddr_t *kvap,
				int flags));

/*
 * sun4/sun4c: load DMA map with a linear buffer.
 */
int
sun4_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	bus_size_t sgsize;
	vaddr_t va = (vaddr_t)buf;
	int pagesz = PAGE_SIZE;
	bus_addr_t dva;
	pmap_t pmap;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	cpuinfo.cache_flush(buf, buflen);

	if ((map->_dm_flags & BUS_DMA_24BIT) == 0) {
		/*
		 * XXX Need to implement "don't dma across this boundry".
		 */
		if (map->_dm_boundary != 0)
			panic("bus_dmamap_load: boundaries not implemented");
		map->dm_mapsize = buflen;
		map->dm_nsegs = 1;
		map->dm_segs[0].ds_addr = (bus_addr_t)va;
		map->dm_segs[0].ds_len = buflen;
		map->_dm_flags |= _BUS_DMA_DIRECTMAP;
		return (0);
	}

	sgsize = round_page(buflen + (va & (pagesz - 1)));

	if (extent_alloc(dvmamap24, sgsize, pagesz, map->_dm_boundary,
			 (flags & BUS_DMA_NOWAIT) == 0 ? EX_WAITOK : EX_NOWAIT,
			 (u_long *)&dva) != 0) {
		return (ENOMEM);
	}

	/*
	 * We always use just one segment.
	 */
	map->dm_mapsize = buflen;
	map->dm_segs[0].ds_addr = dva + (va & (pagesz - 1));
	map->dm_segs[0].ds_len = buflen;
	map->dm_segs[0]._ds_sgsize = sgsize;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	for (; buflen > 0; ) {
		paddr_t pa;
		/*
		 * Get the physical address for this page.
		 */
		(void) pmap_extract(pmap, va, &pa);

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = pagesz - (va & (pagesz - 1));
		if (buflen < sgsize)
			sgsize = buflen;

#ifdef notyet
#if defined(SUN4)
		if (have_iocache)
			pa |= PG_IOC;
#endif
#endif
		pmap_enter(pmap_kernel(), dva,
			   (pa & -pagesz) | PMAP_NC,
			   VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);

		dva += pagesz;
		va += sgsize;
		buflen -= sgsize;
	}

	map->dm_nsegs = 1;
	return (0);
}

/*
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
sun4_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{
	vm_page_t m;
	paddr_t pa;
	bus_addr_t dva;
	bus_size_t sgsize;
	struct pglist *mlist;
	int pagesz = PAGE_SIZE;
	int error;

	map->dm_nsegs = 0;
	sgsize = (size + pagesz - 1) & -pagesz;

	/* Allocate DVMA addresses */
	if ((map->_dm_flags & BUS_DMA_24BIT) != 0) {
		error = extent_alloc(dvmamap24, sgsize, pagesz,
					map->_dm_boundary,
					(flags & BUS_DMA_NOWAIT) == 0
						? EX_WAITOK : EX_NOWAIT,
					(u_long *)&dva);
		if (error)
			return (error);
	} else {
		/* Any properly aligned virtual address will do */
		dva = _bus_dma_valloc_skewed(sgsize, map->_dm_boundary,
					     pagesz, 0);
		if (dva == 0)
			return (ENOMEM);
	}

	map->dm_segs[0].ds_addr = dva;
	map->dm_segs[0].ds_len = size;
	map->dm_segs[0]._ds_sgsize = sgsize;

	/* Map physical pages into IOMMU */
	mlist = segs[0]._ds_mlist;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		if (sgsize == 0)
			panic("sun4_dmamap_load_raw: size botch");
		pa = VM_PAGE_TO_PHYS(m);
#ifdef notyet
#if defined(SUN4)
		if (have_iocache)
			pa |= PG_IOC;
#endif
#endif
		pmap_enter(pmap_kernel(), dva,
			   (pa & -pagesz) | PMAP_NC,
			   VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);

		dva += pagesz;
		sgsize -= pagesz;
	}

	map->dm_nsegs = 1;
	map->dm_mapsize = size;

	return (0);
}

/*
 * sun4/sun4c function for unloading a DMA map.
 */
void
sun4_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	bus_dma_segment_t *segs = map->dm_segs;
	int nsegs = map->dm_nsegs;
	int flags = map->_dm_flags;
	bus_addr_t dva;
	bus_size_t len;
	int i, s, error;

	if ((flags & _BUS_DMA_DIRECTMAP) != 0) {
		/* Nothing to release */
		map->dm_mapsize = 0;
		map->dm_nsegs = 0;
		map->_dm_flags &= ~_BUS_DMA_DIRECTMAP;
		return;
	}

	for (i = 0; i < nsegs; i++) {
		dva = segs[i].ds_addr & -PAGE_SIZE;
		len = segs[i]._ds_sgsize;

		pmap_remove(pmap_kernel(), dva, dva + len);

		if ((flags & BUS_DMA_24BIT) != 0) {
			s = splhigh();
			error = extent_free(dvmamap24, dva, len, EX_NOWAIT);
			splx(s);
			if (error != 0)
				printf("warning: %ld of DVMA space lost\n", len);
		} else {
			uvm_unmap(kernel_map, dva, dva + len);
		}
	}

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
sun4_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	vm_page_t m;
	vaddr_t va;
	struct pglist *mlist;

	if (nsegs != 1)
		panic("sun4_dmamem_map: nsegs = %d", nsegs);

	size = round_page(size);

	va = uvm_km_valloc(kernel_map, size);
	if (va == 0)
		return (ENOMEM);

	segs[0]._ds_va = va;
	*kvap = (caddr_t)va;

	mlist = segs[0]._ds_mlist;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		paddr_t pa;

		if (size == 0)
			panic("sun4_dmamem_map: size botch");

		pa = VM_PAGE_TO_PHYS(m);
		pmap_enter(pmap_kernel(), va, pa | PMAP_NC,
			   VM_PROT_READ | VM_PROT_WRITE,
			   VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);

		va += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	return (0);
}


struct sparc_bus_dma_tag mainbus_dma_tag = {
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	sun4_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	sun4_dmamap_load_raw,
	sun4_dmamap_unload,
	_bus_dmamap_sync,

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	sun4_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};


/*
 * Base bus space handlers.
 */
static int	sparc_bus_map __P(( bus_space_tag_t, bus_type_t, bus_addr_t,
				    bus_size_t, int, vaddr_t,
				    bus_space_handle_t *));
static int	sparc_bus_unmap __P((bus_space_tag_t, bus_space_handle_t,
				     bus_size_t));
static int	sparc_bus_subregion __P((bus_space_tag_t, bus_space_handle_t,
					 bus_size_t, bus_size_t,
					 bus_space_handle_t *));
static int	sparc_bus_mmap __P((bus_space_tag_t, bus_type_t,
				    bus_addr_t, int, bus_space_handle_t *));
static void	*sparc_mainbus_intr_establish __P((bus_space_tag_t, int, int,
						   int (*) __P((void *)),
						   void *));
static void     sparc_bus_barrier __P(( bus_space_tag_t, bus_space_handle_t,
					bus_size_t, bus_size_t, int));


int
sparc_bus_map(t, iospace, addr, size, flags, vaddr, hp)
	bus_space_tag_t t;
	bus_type_t	iospace;
	bus_addr_t	addr;
	bus_size_t	size;
	vaddr_t		vaddr;
	bus_space_handle_t *hp;
{
	vaddr_t v;
	paddr_t pa;
	unsigned int pmtype;
static	vaddr_t iobase;


	if (iobase == NULL)
		iobase = IODEV_BASE;

	size = round_page(size);
	if (size == 0) {
		printf("sparc_bus_map: zero size\n");
		return (EINVAL);
	}

	if (vaddr)
		v = trunc_page(vaddr);
	else {
		v = iobase;
		iobase += size;
		if (iobase > IODEV_END)	/* unlikely */
			panic("sparc_bus_map: iobase=0x%lx", iobase);
	}

	/* note: preserve page offset */
	*hp = (bus_space_handle_t)(v | ((u_long)addr & PGOFSET));

	pa = trunc_page(addr);
	pmtype = PMAP_IOENC(iospace);

	do {
		pmap_enter(pmap_kernel(), v, pa | pmtype | PMAP_NC,
			   VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);
		v += PAGE_SIZE;
		pa += PAGE_SIZE;
	} while ((size -= PAGE_SIZE) > 0);
	return (0);
}

int
sparc_bus_unmap(t, bh, size)
	bus_space_tag_t t;
	bus_size_t	size;
	bus_space_handle_t bh;
{
	vaddr_t va = trunc_page((vaddr_t)bh);
	vaddr_t endva = va + round_page(size);

	pmap_remove(pmap_kernel(), va, endva);
	return (0);
}

int
sparc_bus_subregion(tag, handle, offset, size, nhandlep)
	bus_space_tag_t		tag;
	bus_space_handle_t	handle;
	bus_size_t		offset;
	bus_size_t		size;
	bus_space_handle_t	*nhandlep;
{
	*nhandlep = handle + offset;
	return (0);
}

int
sparc_bus_mmap(t, iospace, paddr, flags, hp)
	bus_space_tag_t t;
	bus_type_t	iospace;
	bus_addr_t	paddr;
	int		flags;
	bus_space_handle_t *hp;
{
	*hp = (bus_space_handle_t)(paddr | PMAP_IOENC(iospace) | PMAP_NC);
	return (0);
}

/*
 * Establish a temporary bus mapping for device probing.
 */
int
bus_space_probe(tag, btype, paddr, size, offset, flags, callback, arg)
	bus_space_tag_t tag;
	bus_type_t	btype;
	bus_addr_t	paddr;
	bus_size_t	size;
	size_t		offset;
	int		flags;
	int		(*callback) __P((void *, void *));
	void		*arg;
{
	bus_space_handle_t bh;
	caddr_t tmp;
	int result;

	if (bus_space_map2(tag, btype, paddr, size, flags, TMPMAP_VA, &bh) != 0)
		return (0);

	tmp = (caddr_t)bh;
	result = (probeget(tmp + offset, size) != -1);
	if (result && callback != NULL)
		result = (*callback)(tmp, arg);
	bus_space_unmap(tag, bh, size);
	return (result);
}


void *
sparc_mainbus_intr_establish(t, level, flags, handler, arg)
	bus_space_tag_t t;
	int	level;
	int	flags;
	int	(*handler)__P((void *));
	void	*arg;
{
	struct intrhand *ih;

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	if ((flags & BUS_INTR_ESTABLISH_FASTTRAP) != 0)
		intr_fasttrap(level, (void (*)__P((void)))handler);
	else
		intr_establish(level, ih);
	return (ih);
}

void sparc_bus_barrier (t, h, offset, size, flags)
	bus_space_tag_t	t;
	bus_space_handle_t h;
	bus_size_t	offset;
	bus_size_t	size;
	int		flags;
{
	/* No default barrier action defined */
	return;
}

struct sparc_bus_space_tag mainbus_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus tag */
	sparc_bus_map,			/* bus_space_map */
	sparc_bus_unmap,		/* bus_space_unmap */
	sparc_bus_subregion,		/* bus_space_subregion */
	sparc_bus_barrier,		/* bus_space_barrier */
	sparc_bus_mmap,			/* bus_space_mmap */
	sparc_mainbus_intr_establish	/* bus_intr_establish */
};
