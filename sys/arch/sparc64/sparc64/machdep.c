/*	$NetBSD: machdep.c,v 1.165 2004/03/24 15:34:51 atatat Exp $ */

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
 *	@(#)machdep.c	8.6 (Berkeley) 1/14/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.165 2004/03/24 15:34:51 atatat Exp $");

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ras.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/syscallargs.h>
#include <sys/exec.h>
#include <sys/ucontext.h>

#include <uvm/uvm.h>

#include <sys/sysctl.h>
#ifndef	ELFSIZE
#ifdef __arch64__
#define	ELFSIZE	64
#else
#define	ELFSIZE	32
#endif
#endif
#include <sys/exec_elf.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/openfirm.h>
#include <machine/sparc64.h>

#include <sparc64/sparc64/cache.h>

/* #include "fb.h" */

int bus_space_debug = 0; /* This may be used by macros elsewhere. */
#ifdef DEBUG
#define DPRINTF(l, s)   do { if (bus_space_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
extern vaddr_t avail_end;

int	physmem;

extern	caddr_t msgbufaddr;

/*
 * Maximum number of DMA segments we'll allow in dmamem_load()
 * routines.  Can be overridden in config files, etc.
 */
#ifndef MAX_DMA_SEGS
#define MAX_DMA_SEGS	20
#endif

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int   safepri = 0;

void	dumpsys __P((void));
void	stackdump __P((void));


/*
 * Machine-dependent startup code
 */
void
cpu_startup()
{
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;
#endif
	vaddr_t minaddr, maxaddr;
	extern struct user *proc0paddr;
	char pbuf[9];

#ifdef DEBUG
	pmapdebug = 0;
#endif

	lwp0.l_addr = proc0paddr;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	/*identifycpu();*/
	format_bytes(pbuf, sizeof(pbuf), ctob((u_int64_t)physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
        exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
                                 16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

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

#if 0
	pmap_redzone();
#endif
}

/*
 * Set up registers on exec.
 */

#ifdef __arch64__
#define STACK_OFFSET	BIAS
#undef CCFSZ
#define CCFSZ	CC64FSZ
#else
#define STACK_OFFSET	0
#endif

/* ARGSUSED */
void
setregs(l, pack, stack)
	struct lwp *l;
	struct exec_package *pack;
	vaddr_t stack;
{
	register struct trapframe64 *tf = l->l_md.md_tf;
	register struct fpstate64 *fs;
	register int64_t tstate;
	int pstate = PSTATE_USER;
#ifdef __arch64__
	Elf_Ehdr *eh = pack->ep_hdr;
#endif

	/* Clear the P_32 flag. */
	l->l_proc->p_flag &= ~P_32;

	/* Don't allow misaligned code by default */
	l->l_md.md_flags &= ~MDP_FIXALIGN;

	/*
	 * Set the registers to 0 except for:
	 *	%o6: stack pointer, built in exec())
	 *	%tstate: (retain icc and xcc and cwp bits)
	 *	%g1: address of p->p_psstr (used by crt0)
	 *	%tpc,%tnpc: entry point of program
	 */
#ifdef __arch64__
	/* Check what memory model is requested */
	switch ((eh->e_flags & EF_SPARCV9_MM)) {
	default:
		printf("Unknown memory model %d\n", 
		       (eh->e_flags & EF_SPARCV9_MM));
		/* FALLTHROUGH */
	case EF_SPARCV9_TSO:
		pstate = PSTATE_MM_TSO|PSTATE_IE;
		break;
	case EF_SPARCV9_PSO:
		pstate = PSTATE_MM_PSO|PSTATE_IE;
		break;
	case EF_SPARCV9_RMO:
		pstate = PSTATE_MM_RMO|PSTATE_IE;
		break;
	}
#endif
	tstate = (ASI_PRIMARY_NO_FAULT<<TSTATE_ASI_SHIFT) |
		((pstate)<<TSTATE_PSTATE_SHIFT) | 
		(tf->tf_tstate & TSTATE_CWP);
	if ((fs = l->l_md.md_fpstate) != NULL) {
		/*
		 * We hold an FPU state.  If we own *the* FPU chip state
		 * we must get rid of it, and the only way to do that is
		 * to save it.  In any case, get rid of our FPU state.
		 */
		if (l == fplwp) {
			savefpstate(fs);
			fplwp = NULL;
		}
		free((void *)fs, M_SUBPROC);
		l->l_md.md_fpstate = NULL;
	}
	memset(tf, 0, sizeof *tf);
	tf->tf_tstate = tstate;
	tf->tf_global[1] = (vaddr_t)l->l_proc->p_psstr;
	/* %g4 needs to point to the start of the data segment */
	tf->tf_global[4] = 0; 
	tf->tf_pc = pack->ep_entry & ~3;
	tf->tf_npc = tf->tf_pc + 4;
	stack -= sizeof(struct rwindow);
	tf->tf_out[6] = stack - STACK_OFFSET;
	tf->tf_out[7] = 0UL;
#ifdef NOTDEF_DEBUG
	printf("setregs: setting tf %p sp %p pc %p\n", (long)tf, 
	       (long)tf->tf_out[6], (long)tf->tf_pc);
#ifdef DDB
	Debugger();
#endif
#endif
}

static char *parse_bootfile(char *);
static char *parse_bootargs(char *);

static char *
parse_bootfile(args)
	char *args;
{
	char *cp;

	/*
	 * bootargs is of the form: [kernelname] [args...]
	 * It can be the empty string if we booted from the default
	 * kernel name.
	 */
	cp = args;
	for (cp = args; *cp != 0 && *cp != ' ' && *cp != '\t'; cp++) {
		if (*cp == '-') {
			int c;
			/*
			 * If this `-' is most likely the start of boot
			 * options, we're done.
			 */
			if (cp == args)
				break;
			if ((c = *(cp-1)) == ' ' || c == '\t')
				break;
		}
	}
	/* Now we've separated out the kernel name from the args */
	*cp = '\0';
	return (args);
}

static char *
parse_bootargs(args)
	char *args;
{
	char *cp;

	for (cp = args; *cp != '\0'; cp++) {
		if (*cp == '-') {
			int c;
			/*
			 * Looks like options start here, but check this
			 * `-' is not part of the kernel name.
			 */
			if (cp == args)
				break;
			if ((c = *(cp-1)) == ' ' || c == '\t')
				break;
		}
	}
	return (cp);
}

/*
 * machine dependent system variables.
 */
static int
sysctl_machdep_boot(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	u_int chosen;
	char bootargs[256];
	char *cp;

	if ((chosen = OF_finddevice("/chosen")) == -1)
		return (ENOENT);
	if (node.sysctl_num == CPU_BOOTED_DEVICE)
		cp = "bootpath";
	else
		cp = "bootargs";
	if (OF_getprop(chosen, cp, bootargs, sizeof bootargs) < 0)
		return (ENOENT);

	switch (node.sysctl_num) {
	case CPU_BOOTED_KERNEL:
		cp = parse_bootfile(bootargs);
                if (cp != NULL && cp[0] == '\0')
                        /* Unknown to firmware, return default name */
                        cp = "netbsd";
		break;
	case CPU_BOOT_ARGS:
		cp = parse_bootargs(bootargs);
		break;
	case CPU_BOOTED_DEVICE:
		cp = bootargs;
		break;
	}

	if (cp == NULL || cp[0] == '\0')
		return (ENOENT);

	node.sysctl_data = cp;
	node.sysctl_size = strlen(cp) + 1;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_kernel", NULL,
		       sysctl_machdep_boot, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_KERNEL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "boot_args", NULL,
		       sysctl_machdep_boot, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOT_ARGS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_device", NULL,
		       sysctl_machdep_boot, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_DEVICE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "cpu_arch", NULL,
		       NULL, 9, NULL, 0,
		       CTL_MACHDEP, CPU_ARCH, CTL_EOL);
}

void *
getframe(struct lwp *l, int sig, int *onstack)
{
	struct proc *p = l->l_proc;
	struct sigctx *ctx = &p->p_sigctx;
	struct trapframe64 *tf = l->l_md.md_tf;

	/*
	 * Compute new user stack addresses, subtract off
	 * one signal frame, and align.
	 */
	*onstack = (ctx->ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
	    && (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	if (*onstack)
		return ((caddr_t)ctx->ps_sigstk.ss_sp + ctx->ps_sigstk.ss_size);
	else
		return (void *)((uintptr_t)tf->tf_out[6] + STACK_OFFSET);
}

struct sigframe_siginfo {
	siginfo_t	sf_si;		/* saved siginfo */
	ucontext_t	sf_uc;		/* saved ucontext */
};

static void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	int onstack;
	int sig = ksi->ksi_signo;
	ucontext_t uc;
	long ucsz;
	struct sigframe_siginfo *fp = getframe(l, sig, &onstack);
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct trapframe64 *tf = l->l_md.md_tf;
	struct rwindow *newsp;
	/* Allocate an aligned sigframe */
	fp = (void *)((u_long)(fp - 1) & ~0x0f);

	/* Build stack frame for signal trampoline. */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* handled by sendsig_sigcontext */
	case 1:		/* handled by sendsig_sigcontext */
	default:	/* unknown version */
		printf("sendsig_siginfo: bad version %d\n",
		    ps->sa_sigdesc[sig].sd_vers);
		sigexit(l, SIGILL);
	case 2:
		break;
	}

	uc.uc_flags = _UC_SIGMASK |
	    ((p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK)
		? _UC_SETSTACK : _UC_CLRSTACK);
	uc.uc_sigmask = *mask;
	uc.uc_link = NULL;
	memset(&uc.uc_stack, 0, sizeof(uc.uc_stack));
	cpu_getmcontext(l, &uc.uc_mcontext, &uc.uc_flags);
	ucsz = (char *)&uc.__uc_pad - (char *)&uc;

	/*
	 * Now copy the stack contents out to user space.
	 * We need to make sure that when we start the signal handler,
	 * its %i6 (%fp), which is loaded from the newly allocated stack area,
	 * joins seamlessly with the frame it was in when the signal occurred,
	 * so that the debugger and _longjmp code can back up through it.
	 * Since we're calling the handler directly, allocate a full size
	 * C stack frame.
	 */
	newsp = (struct rwindow *)((u_long)fp - CCFSZ);
	if (copyout(&ksi->ksi_info, &fp->sf_si, sizeof(ksi->ksi_info)) != 0 ||
	    copyout(&uc, &fp->sf_uc, ucsz) != 0 ||
	    copyout(&tf->tf_out[6], &newsp->rw_in[6],
	    sizeof(tf->tf_out[6])) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	tf->tf_pc = (const vaddr_t)catcher;
	tf->tf_npc = (const vaddr_t)catcher + 4;
	tf->tf_out[0] = sig;
	tf->tf_out[1] = (vaddr_t)&fp->sf_si;
	tf->tf_out[2] = (vaddr_t)&fp->sf_uc;
	tf->tf_out[6] = (vaddr_t)newsp - STACK_OFFSET;
	tf->tf_out[7] = (vaddr_t)ps->sa_sigdesc[sig].sd_tramp - 8;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

void
sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
#ifdef COMPAT_16
	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		sendsig_sigcontext(ksi, mask);
	else
#endif
		sendsig_siginfo(ksi, mask);
}

/*
 * Set the lwp to begin execution in the upcall handler.  The upcall
 * handler will then simply call the upcall routine and then exit.
 *
 * Because we have a bunch of different signal trampolines, the first
 * two instructions in the signal trampoline call the upcall handler.
 * Signal dispatch should skip the first two instructions in the signal
 * trampolines.
 */
void 
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted,
	void *sas, void *ap, void *sp, sa_upcall_t upcall)
{
       	struct trapframe64 *tf;
	vaddr_t addr;

	tf = l->l_md.md_tf;
	addr = (vaddr_t) upcall;

	/* Arguments to the upcall... */
	tf->tf_out[0] = type;
	tf->tf_out[1] = (vaddr_t) sas;
	tf->tf_out[2] = nevents;
	tf->tf_out[3] = ninterrupted;
	tf->tf_out[4] = (vaddr_t) ap;

	/*
	 * Ensure the stack is double-word aligned, and provide a
	 * valid C call frame.
	 */
	sp = (void *)(((vaddr_t)sp & ~0xf) - CCFSZ);

	/* Arrange to begin execution at the upcall handler. */
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = (vaddr_t)sp - STACK_OFFSET;
	tf->tf_out[7] = -1;		/* "you lose" if upcall returns */
}

int	waittime = -1;

void
cpu_reboot(howto, user_boot_string)
	register int howto;
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
		extern struct lwp lwp0;
		extern int sparc_clock_time_is_ok;

		/* XXX protect against curlwp->p_stats.foo refs in sync() */
		if (curlwp == NULL)
			curlwp = &lwp0;
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 * Do this only if the TOD clock has already been read out
		 * successfully by inittodr() or set by an explicit call
		 * to resettodr() (e.g. from settimeofday()).
		 */
		if (sparc_clock_time_is_ok)
			resettodr();
	}
	(void) splhigh();		/* ??? */

	/* Stop all secondary cpus */
	sparc64_ipi_halt_cpus();

	/* If rebooting and a dump is requested, do it. */
	if (howto & RB_DUMP)
		dumpsys();

haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

	/* If powerdown was requested, do it. */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		/* Let the OBP do the work. */
		OF_poweroff();
		printf("WARNING: powerdown failed!\n");
		/*
		 * RB_POWERDOWN implies RB_HALT... fall into it...
		 */
	}

	if (howto & RB_HALT) {
		printf("halted\n\n");
		OF_exit();
		panic("PROM exit failed");
	}

	printf("rebooting\n\n");
	if (user_boot_string && *user_boot_string) {
		i = strlen(user_boot_string);
		if (i > sizeof(str))
			OF_boot(user_boot_string);	/* XXX */
		memcpy(str, user_boot_string, i);
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
	OF_boot(str);
	panic("cpu_reboot -- failed");
	/*NOTREACHED*/
}

u_int32_t dumpmag = 0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
long	dumplo = 0;

void
cpu_dumpconf()
{
	const struct bdevsw *bdev;
	register int nblks, dumpblks;

	if (dumpdev == NODEV)
		/* No usable dump device */
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL || bdev->d_psize == NULL)
		/* No usable dump device */
		return;

	nblks = (*bdev->d_psize)(dumpdev);

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

#define	BYTES_PER_DUMP	(PAGE_SIZE)	/* must be a multiple of pagesize */
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
	const struct bdevsw *bdev;
	register int psize;
	daddr_t blkno;
	register int (*dump)	__P((dev_t, daddr_t, caddr_t, size_t));
	int error = 0;
	register struct mem_region *mp;
	extern struct mem_region *mem;

	/* copy registers to memory */
	snapshot(curpcb);
	stackdump();

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL || bdev->d_psize == NULL)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (!dumpspace) {
		printf("\nno address space available, dump not possible\n");
		return;
	}
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	psize = (*bdev->d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}
	blkno = dumplo;
	dump = bdev->d_dump;

	error = pmap_dumpmmu(dump, blkno);
	blkno += pmap_dumpsize();
printf("starting dump, blkno %lld\n", (long long)blkno);
	for (mp = mem; mp->size; mp++) {
		unsigned i = 0, n;
		paddr_t maddr = mp->start;

#if 0
		/* Remind me: why don't we dump page 0 ? */
		if (maddr == 0) {
			/* Skip first page at physical address 0 */
			maddr += PAGE_SIZE;
			i += PAGE_SIZE;
			blkno += btodb(PAGE_SIZE);
		}
#endif
		for (; i < mp->size; i += n) {
			n = mp->size - i;
			if (n > BYTES_PER_DUMP)
				 n = BYTES_PER_DUMP;

			/* print out how many MBs we have dumped */
			if (i && (i % (1024*1024)) == 0)
				printf("%d ", i / (1024*1024));
			pmap_kenter_pa(dumpspace, maddr, VM_PROT_READ);
			pmap_update(pmap_kernel());
			error = (*dump)(dumpdev, blkno,
					(caddr_t)dumpspace, (int)n);
			pmap_kremove(dumpspace, n);
			pmap_update(pmap_kernel());
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

void trapdump __P((struct trapframe64*));
/*
 * dump out a trapframe.
 */
void
trapdump(tf)
	struct trapframe64* tf;
{
	printf("TRAPFRAME: tstate=%llx pc=%llx npc=%llx y=%x\n",
	       (unsigned long long)tf->tf_tstate, (unsigned long long)tf->tf_pc,
	       (unsigned long long)tf->tf_npc, (unsigned)tf->tf_y);
	printf("%%g1-7: %llx %llx %llx %llx %llx %llx %llx\n",
	       (unsigned long long)tf->tf_global[1],
	       (unsigned long long)tf->tf_global[2],
	       (unsigned long long)tf->tf_global[3], 
	       (unsigned long long)tf->tf_global[4],
	       (unsigned long long)tf->tf_global[5],
	       (unsigned long long)tf->tf_global[6], 
	       (unsigned long long)tf->tf_global[7]);
	printf("%%o0-7: %llx %llx %llx %llx\n %llx %llx %llx %llx\n",
	       (unsigned long long)tf->tf_out[0],
	       (unsigned long long)tf->tf_out[1],
	       (unsigned long long)tf->tf_out[2],
	       (unsigned long long)tf->tf_out[3], 
	       (unsigned long long)tf->tf_out[4],
	       (unsigned long long)tf->tf_out[5],
	       (unsigned long long)tf->tf_out[6],
	       (unsigned long long)tf->tf_out[7]);
}
/*
 * get the fp and dump the stack as best we can.  don't leave the
 * current stack page
 */
void
stackdump()
{
	struct frame32 *fp = (struct frame32 *)getfp(), *sfp;
	struct frame64 *fp64;

	sfp = fp;
	printf("Frame pointer is at %p\n", fp);
	printf("Call traceback:\n");
	while (fp && ((u_long)fp >> PGSHIFT) == ((u_long)sfp >> PGSHIFT)) {
		if( ((long)fp) & 1 ) {
			fp64 = (struct frame64*)(((char*)fp)+BIAS);
			/* 64-bit frame */
			printf("%llx(%llx, %llx, %llx, %llx, %llx, %llx, %llx) fp = %llx\n",
			       (unsigned long long)fp64->fr_pc,
			       (unsigned long long)fp64->fr_arg[0],
			       (unsigned long long)fp64->fr_arg[1],
			       (unsigned long long)fp64->fr_arg[2],
			       (unsigned long long)fp64->fr_arg[3],
			       (unsigned long long)fp64->fr_arg[4],
			       (unsigned long long)fp64->fr_arg[5],	
			       (unsigned long long)fp64->fr_arg[6],
			       (unsigned long long)fp64->fr_fp);
			fp = (struct frame32 *)(u_long)fp64->fr_fp;
		} else {
			/* 32-bit frame */
			printf("  pc = %x  args = (%x, %x, %x, %x, %x, %x, %x) fp = %x\n",
			       fp->fr_pc, fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2],
			       fp->fr_arg[3], fp->fr_arg[4], fp->fr_arg[5], fp->fr_arg[6],
			       fp->fr_fp);
			fp = (struct frame32*)(u_long)(u_short)fp->fr_fp;
		}
	}
}


int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	return (ENOEXEC);
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

	memset(mapstore, 0, mapsize);
	map = (struct sparc_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT|BUS_DMA_COHERENT|
				   BUS_DMA_NOWRITE|BUS_DMA_NOCACHE);
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
	if (map->dm_nsegs)
		bus_dmamap_unload(t, map);
	free(map, M_DMAMAP);
}

/*
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 *
 * Most SPARCs have IOMMUs in the bus controllers.  In those cases
 * they only need one segment and will use virtual addresses for DVMA.
 * Those bus controllers should intercept these vectors and should
 * *NEVER* call _bus_dmamap_load() which is used only by devices that
 * bypass DVMA.
 */
int
_bus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	bus_size_t sgsize;
	vaddr_t vaddr = (vaddr_t)buf;
	long incr;
	int i;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
	{ 
#ifdef DEBUG
		printf("_bus_dmamap_load(): error %lu > %lu -- map size exceeded!\n",
		    (unsigned long)buflen, (unsigned long)map->_dm_size);
#ifdef DDB
		Debugger();
#endif
#endif
		return (EINVAL);
	}		

	sgsize = round_page(buflen + ((int)vaddr & PGOFSET));

	/*
	 * We always use just one segment.
	 */
	map->dm_mapsize = buflen;
	i = 0;
	map->dm_segs[i].ds_addr = 0UL;
	map->dm_segs[i].ds_len = 0;

	incr = PAGE_SIZE - (vaddr & PGOFSET);
	while (sgsize > 0) {
		paddr_t pa;
	
		incr = min(sgsize, incr);

		(void) pmap_extract(pmap_kernel(), vaddr, &pa);
		sgsize -= incr;
		vaddr += incr;
		if (map->dm_segs[i].ds_len == 0)
			map->dm_segs[i].ds_addr = pa;
		if (pa == (map->dm_segs[i].ds_addr + map->dm_segs[i].ds_len)
		    && ((map->dm_segs[i].ds_len + incr) <= map->_dm_maxsegsz)) {
			/* Hey, waddyaknow, they're contiguous */
			map->dm_segs[i].ds_len += incr;
			incr = PAGE_SIZE;
			continue;
		}
		if (++i >= map->_dm_segcnt)
			return (E2BIG);
		map->dm_segs[i].ds_addr = pa;
		map->dm_segs[i].ds_len = incr = PAGE_SIZE;
	}
	map->dm_nsegs = i + 1;
	/* Mapping is bus dependent */
	return (0);
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
	bus_dma_segment_t segs[MAX_DMA_SEGS];
	int i;
	size_t len;

	/* Record mbuf for *_unload */
	map->_dm_type = _DM_TYPE_MBUF;
	map->_dm_source = (void *)m;

	i = 0;
	len = 0;
	while (m) {
		vaddr_t vaddr = mtod(m, vaddr_t);
		long buflen = (long)m->m_len;

		len += buflen;
		while (buflen > 0 && i < MAX_DMA_SEGS) {
			paddr_t pa;
			long incr;

			incr = PAGE_SIZE - (vaddr & PGOFSET);
			incr = min(buflen, incr);

			(void) pmap_extract(pmap_kernel(), vaddr, &pa);
			buflen -= incr;
			vaddr += incr;

			if (i > 0 && 
				pa == (segs[i-1].ds_addr + segs[i-1].ds_len) &&
				((segs[i-1].ds_len + incr) <= 
					map->_dm_maxsegsz)) {
				/* Hey, waddyaknow, they're contiguous */
				segs[i-1].ds_len += incr;
				continue;
			}
			segs[i].ds_addr = pa;
			segs[i].ds_len = incr;
			segs[i]._ds_boundary = 0;
			segs[i]._ds_align = 0;
			segs[i]._ds_mlist = NULL;
			i++;
		}
		m = m->m_next;
		if (m && i >= MAX_DMA_SEGS) {
			/* Exceeded the size of our dmamap */
			map->_dm_type = 0;
			map->_dm_source = NULL;
			return E2BIG;
		}
	}

#ifdef DEBUG
	{
		size_t mbuflen, sglen;
		int j;
		int retval;

		mbuflen = 0;
		for (m = (struct mbuf *)map->_dm_source; m; m = m->m_next)
			mbuflen += (long)m->m_len;
		sglen = 0;
		for (j = 0; j < i; j++)
			sglen += segs[j].ds_len;
		if (sglen != mbuflen)
			panic("load_mbuf: sglen %ld != mbuflen %lx\n",
				sglen, mbuflen);
		if (sglen != len)
			panic("load_mbuf: sglen %ld != len %lx\n",
				sglen, len);
		retval = bus_dmamap_load_raw(t, map, segs, i,
			(bus_size_t)len, flags);
		if (map->dm_mapsize != len)
			panic("load_mbuf: mapsize %ld != len %lx\n",
				(long)map->dm_mapsize, len);
		sglen = 0;
		for (j = 0; j < map->dm_nsegs; j++)
			sglen += map->dm_segs[j].ds_len;
		if (sglen != len)
			panic("load_mbuf: dmamap sglen %ld != len %lx\n",
				sglen, len);
		return (retval);
	}
#endif
	return (bus_dmamap_load_raw(t, map, segs, i, (bus_size_t)len, flags));
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
/* 
 * XXXXXXX The problem with this routine is that it needs to 
 * lock the user address space that is being loaded, but there
 * is no real way for us to unlock it during the unload process.
 */
#if 0
	bus_dma_segment_t segs[MAX_DMA_SEGS];
	int i, j;
	size_t len;
	struct proc *p = uio->uio_procp;
	struct pmap *pm;

	if (uio->uio_segflg == UIO_USERSPACE) {
		pm = p->p_vmspace->vm_map.pmap;
	} else
		pm = pmap_kernel();

	i = 0;
	len = 0;
	for (j = 0; j < uio->uio_iovcnt; j++) {
		struct iovec *iov = &uio->uio_iov[j];
		vaddr_t vaddr = (vaddr_t)iov->iov_base;
		bus_size_t buflen = iov->iov_len;

		/*
		 * Lock the part of the user address space involved
		 *    in the transfer.
		 */
		PHOLD(p);
		if (__predict_false(uvm_vslock(p, vaddr, buflen,
			    (uio->uio_rw == UIO_WRITE) ?
			    VM_PROT_WRITE : VM_PROT_READ) != 0)) {
				goto after_vsunlock;
			}
		
		len += buflen;
		while (buflen > 0 && i < MAX_DMA_SEGS) {
			paddr_t pa;
			long incr;

			incr = min(buflen, PAGE_SIZE);
			(void) pmap_extract(pm, vaddr, &pa);
			buflen -= incr;
			vaddr += incr;
			if (segs[i].ds_len == 0)
				segs[i].ds_addr = pa;


			if (i > 0 && pa == (segs[i-1].ds_addr + segs[i-1].ds_len)
			    && ((segs[i-1].ds_len + incr) <= map->_dm_maxsegsz)) {
				/* Hey, waddyaknow, they're contiguous */
				segs[i-1].ds_len += incr;
				continue;
			}
			segs[i].ds_addr = pa;
			segs[i].ds_len = incr;
			segs[i]._ds_boundary = 0;
			segs[i]._ds_align = 0;
			segs[i]._ds_mlist = NULL;
			i++;
		}
		uvm_vsunlock(p, bp->b_data, todo);
		PRELE(p);
 		if (buflen > 0 && i >= MAX_DMA_SEGS) 
			/* Exceeded the size of our dmamap */
			return E2BIG;
	}
	map->_dm_type = DM_TYPE_UIO;
	map->_dm_source = (void *)uio;
	return (bus_dmamap_load_raw(t, map, segs, i, 
				    (bus_size_t)len, flags));
#endif
	return 0;
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
 * Common function for unloading a DMA map.  May be called by
 * bus-specific DMA map unload functions.
 */
void
_bus_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	int i;
	struct vm_page *pg;
	struct pglist *pglist;
	paddr_t pa;

	for (i = 0; i < map->dm_nsegs; i++) {
		if ((pglist = map->dm_segs[i]._ds_mlist) == NULL) {

			/*
			 * We were asked to load random VAs and lost the
			 * PA info so just blow the entire cache away.
			 */
			blast_dcache();
			break;
		}
		TAILQ_FOREACH(pg, pglist, pageq) {
			pa = VM_PAGE_TO_PHYS(pg);

			/* 
			 * We should be flushing a subrange, but we
			 * don't know where the segments starts.
			 */
			dcache_flush_page(pa);
		}
	}

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

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
	int i;
	struct vm_page *pg;
	struct pglist *pglist;

	/*
	 * We sync out our caches, but the bus must do the same.
	 *
	 * Actually a #Sync is expensive.  We should optimize.
	 */
	if ((ops & BUS_DMASYNC_PREREAD) || (ops & BUS_DMASYNC_PREWRITE)) {

		/* 
		 * Don't really need to do anything, but flush any pending
		 * writes anyway. 
		 */
		__asm("membar #Sync" : );
	}
	if (ops & BUS_DMASYNC_POSTREAD) {
		/* Invalidate the vcache */
		for (i = 0; i < map->dm_nsegs; i++) {
			if ((pglist = map->dm_segs[i]._ds_mlist) == NULL)
				/* Should not really happen. */
				continue;
			TAILQ_FOREACH(pg, pglist, pageq) {
				paddr_t start;
				psize_t size = PAGE_SIZE;

				if (offset < PAGE_SIZE) {
					start = VM_PAGE_TO_PHYS(pg) + offset;
					if (size > len)
						size = len;
					cache_flush_phys(start, size, 0);
					len -= size;
					continue;
				}
				offset -= size;
			}
		}
	}
	if (ops & BUS_DMASYNC_POSTWRITE) {
		/* Nothing to do.  Handled by the bus controller. */
	}
}

extern paddr_t   vm_first_phys, vm_num_phys;
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
	struct pglist *pglist;
	int error;

	/* Always round the size. */
	size = round_page(size);
	low = vm_first_phys;
	high = vm_first_phys + vm_num_phys - PAGE_SIZE;

	if ((pglist = malloc(sizeof(*pglist), M_DEVBUF,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	/*
	 * If the bus uses DVMA then ignore boundary and alignment.
	 */
	segs[0]._ds_boundary = boundary;
	segs[0]._ds_align = alignment;
	if (flags & BUS_DMA_DVMA) {
		boundary = 0;
		alignment = 0;
	}

	/*
	 * Allocate pages from the VM system.
	 */
	error = uvm_pglistalloc(size, low, high,
	    alignment, boundary, pglist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return (error);

	/*
	 * Compute the location, size, and number of segments actually
	 * returned by the VM code.
	 */
	segs[0].ds_addr = 0UL; /* UPA does not map things */
	segs[0].ds_len = size;
	*rsegs = 1;

	/*
	 * Simply keep a pointer around to the linked list, so
	 * bus_dmamap_free() can return it.
	 *
	 * NOBODY SHOULD TOUCH THE pageq FIELDS WHILE THESE PAGES
	 * ARE IN OUR CUSTODY.
	 */
	segs[0]._ds_mlist = pglist;

	/* The bus driver should do the actual mapping */
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
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
_bus_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	vaddr_t va, sva;
	int r, cbit;
	size_t oversize;
	u_long align;

	if (nsegs != 1)
		panic("_bus_dmamem_map: nsegs = %d", nsegs);

	cbit = PMAP_NC;
	align = PAGE_SIZE;

	size = round_page(size);

	/*
	 * Find a region of kernel virtual addresses that can accomodate
	 * our aligment requirements.
	 */
	oversize = size + align - PAGE_SIZE;
	r = uvm_map(kernel_map, &sva, oversize, NULL, UVM_UNKNOWN_OFFSET, 0,
	    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
	    UVM_ADV_NORMAL, 0));
	if (r != 0)
		return (ENOMEM);

	/* Compute start of aligned region */
	va = sva;
	va += ((segs[0].ds_addr & (align - 1)) + align - va) & (align - 1);

	/* Return excess virtual addresses */
	if (va != sva)
		uvm_unmap(kernel_map, sva, va);
	if (va + size != sva + oversize)
		uvm_unmap(kernel_map, va + size, sva + oversize);

	*kvap = (caddr_t)va;
	return (0);
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


struct sparc_bus_dma_tag mainbus_dma_tag = {
	NULL,
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};


/*
 * Base bus space handlers.
 */
static int	sparc_bus_map __P(( bus_space_tag_t, bus_addr_t,
				    bus_size_t, int, vaddr_t, bus_space_handle_t *));
static int	sparc_bus_unmap __P((bus_space_tag_t, bus_space_handle_t,
				     bus_size_t));
static int	sparc_bus_subregion __P((bus_space_tag_t, bus_space_handle_t,
					 bus_size_t, bus_size_t,
					 bus_space_handle_t *));
static paddr_t	sparc_bus_mmap __P((bus_space_tag_t, bus_addr_t, off_t, int, int));
static void	*sparc_mainbus_intr_establish __P((bus_space_tag_t, int, int,
						   int (*) __P((void *)),
						   void *, void (*)__P((void))));
static int	sparc_bus_alloc __P((bus_space_tag_t, bus_addr_t, bus_addr_t,
				     bus_size_t, bus_size_t, bus_size_t, int,
				     bus_addr_t *, bus_space_handle_t *));
static void	sparc_bus_free __P((bus_space_tag_t, bus_space_handle_t,
				    bus_size_t));

vaddr_t iobase = IODEV_BASE;
struct extent *io_space = NULL;

int
sparc_bus_map(t, addr, size, flags, unused, hp)
	bus_space_tag_t t;
	bus_addr_t	addr;
	bus_size_t	size;
	vaddr_t unused;
	bus_space_handle_t *hp;
{
	vaddr_t v;
	u_int64_t pa;
	paddr_t	pm_flags = 0;
	vm_prot_t pm_prot = VM_PROT_READ;
	int err, map_little = 0;

	if (iobase == 0UL)
		iobase = IODEV_BASE;
	if (io_space == NULL)
		/*
		 * And set up IOSPACE extents.
		 */
		io_space = extent_create("IOSPACE",
					 (u_long)IODEV_BASE, (u_long)IODEV_END,
					 M_DEVBUF, 0, 0, EX_NOWAIT);


	size = round_page(size);
	if (size == 0) {
		printf("sparc_bus_map: zero size\n");
		return (EINVAL);
	}
	switch (t->type) {
	case PCI_CONFIG_BUS_SPACE:
		/* 
		 * PCI config space is special.
		 *
		 * It's really big and seldom used.  In order not to run
		 * out of IO mappings, config space will not be mapped in,
		 * rather it will be accessed through MMU bypass ASI accesses.
		 */
		if (flags & BUS_SPACE_MAP_LINEAR) return (-1);
		hp->_ptr = addr;
		hp->_asi = ASI_PHYS_NON_CACHED_LITTLE;
		hp->_sasi = ASI_PHYS_NON_CACHED;
		DPRINTF(BSDB_MAP, ("\nsparc_bus_map: type %x flags %x "
			"addr %016llx size %016llx virt %llx\n",
			(int)t->type, (int) flags, (unsigned long long)addr,
			(unsigned long long)size,
			(unsigned long long)hp->_ptr));
		return (0);
		/* FALLTHROUGH */
	case PCI_IO_BUS_SPACE:
		map_little = 1;
		break;
	case PCI_MEMORY_BUS_SPACE:
		map_little = 1;
		break;
	default:
		map_little = 0;
		break;
	}

#ifdef _LP64
	/* If it's not LINEAR don't bother to map it.  Use phys accesses. */
	if ((flags & BUS_SPACE_MAP_LINEAR) == 0) {
		hp->_ptr = addr;
		if (map_little)
			hp->_asi = ASI_PHYS_NON_CACHED_LITTLE;
		else
			hp->_asi = ASI_PHYS_NON_CACHED;
		hp->_sasi = ASI_PHYS_NON_CACHED;
		return (0);
	}
#endif

	if (!(flags & BUS_SPACE_MAP_CACHEABLE)) pm_flags |= PMAP_NC;

	if ((err = extent_alloc(io_space, size, PAGE_SIZE,
		0, EX_NOWAIT|EX_BOUNDZERO, (u_long *)&v)))
			panic("sparc_bus_map: cannot allocate io_space: %d", err);

	/* note: preserve page offset */
	hp->_ptr = (v | ((u_long)addr & PGOFSET));
	hp->_sasi = ASI_PRIMARY;
	if (map_little)
		hp->_asi = ASI_PRIMARY_LITTLE;
	else
		hp->_asi = ASI_PRIMARY;

	pa = addr & ~PAGE_MASK; /* = trunc_page(addr); Will drop high bits */
	if (!(flags&BUS_SPACE_MAP_READONLY))
		pm_prot |= VM_PROT_WRITE;

	DPRINTF(BSDB_MAP, ("\nsparc_bus_map: type %x flags %x "
		"addr %016llx size %016llx virt %llx paddr %016llx\n",
		(int)t->type, (int) flags, (unsigned long long)addr,
		(unsigned long long)size, (unsigned long long)hp->_ptr,
		(unsigned long long)pa));

	do {
		DPRINTF(BSDB_MAP, ("sparc_bus_map: phys %llx virt %p hp %llx\n",
			(unsigned long long)pa, (char *)v,
			(unsigned long long)hp->_ptr));
		pmap_kenter_pa(v, pa | pm_flags, pm_prot);
		v += PAGE_SIZE;
		pa += PAGE_SIZE;
	} while ((size -= PAGE_SIZE) > 0);
	pmap_update(pmap_kernel());
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
	nhandlep->_ptr = handle._ptr + offset;
	nhandlep->_asi = handle._asi;
	nhandlep->_sasi = handle._sasi;
	return (0);
}

int
sparc_bus_unmap(t, bh, size)
	bus_space_tag_t t;
	bus_size_t	size;
	bus_space_handle_t bh;
{
	vaddr_t va = trunc_page((vaddr_t)bh._ptr);
	vaddr_t endva = va + round_page(size);
	int error = 0;

	if (PHYS_ASI(bh._asi)) return (0);

	error = extent_free(io_space, va, size, EX_NOWAIT);
	if (error) printf("sparc_bus_unmap: extent free sez %d\n", error);

	pmap_remove(pmap_kernel(), va, endva);
	return (0);
}

paddr_t
sparc_bus_mmap(t, paddr, off, prot, flags)
	bus_space_tag_t t;
	bus_addr_t	paddr;
	off_t		off;
	int		prot;
	int		flags;
{
	/* Devices are un-cached... although the driver should do that */
	return ((paddr+off)|PMAP_NC);
}


void *
sparc_mainbus_intr_establish(t, pil, level, handler, arg, fastvec)
	bus_space_tag_t t;
	int	pil;
	int	level;
	int	(*handler)__P((void *));
	void	*arg;
	void	(*fastvec)__P((void));	/* ignored */
{
	struct intrhand *ih;

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	intr_establish(pil, ih);
	return (ih);
}

int
sparc_bus_alloc(t, rs, re, s, a, b, f, ap, hp)
	bus_space_tag_t t;
	bus_addr_t	rs;
	bus_addr_t	re;
	bus_size_t	s;
	bus_size_t	a;
	bus_size_t	b;
	int		f;
	bus_addr_t	*ap;
	bus_space_handle_t *hp;
{
	return (ENOTTY);
}

void
sparc_bus_free(t, h, s)
	bus_space_tag_t	t;
	bus_space_handle_t	h;
	bus_size_t	s;
{
	return;
}

struct sparc_bus_space_tag mainbus_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus tag */
	UPA_BUS_SPACE,			/* type */
	sparc_bus_alloc,
	sparc_bus_free,
	sparc_bus_map,			/* bus_space_map */
	sparc_bus_unmap,		/* bus_space_unmap */
	sparc_bus_subregion,		/* bus_space_subregion */
	sparc_bus_mmap,			/* bus_space_mmap */
	sparc_mainbus_intr_establish	/* bus_intr_establish */
};


void
cpu_getmcontext(l, mcp, flags)
	struct lwp *l;
	mcontext_t *mcp;
	unsigned int *flags;
{
	__greg_t *gr = mcp->__gregs;
	__greg_t ras_pc;
	const struct trapframe64 *tf = l->l_md.md_tf;

	/* First ensure consistent stack state (see sendsig). */ /* XXX? */
	write_user_windows();
	if (rwindow_save(l))
		sigexit(l, SIGILL);

	/* For now: Erase any random indicators for optional state. */
	(void)memset(mcp, '0', sizeof (*mcp));

	/* Save general register context. */
#ifdef __arch64__
	gr[_REG_CCR] = (tf->tf_tstate & TSTATE_CCR) >> TSTATE_CCR_SHIFT;
#else
	gr[_REG_PSR] = TSTATECCR_TO_PSR(tf->tf_tstate);
#endif
	gr[_REG_PC]  = tf->tf_pc;
	gr[_REG_nPC] = tf->tf_npc;
	gr[_REG_Y]   = tf->tf_y;
	gr[_REG_G1]  = tf->tf_global[1];
	gr[_REG_G2]  = tf->tf_global[2];
	gr[_REG_G3]  = tf->tf_global[3];
	gr[_REG_G4]  = tf->tf_global[4];
	gr[_REG_G5]  = tf->tf_global[5];
	gr[_REG_G6]  = tf->tf_global[6];
	gr[_REG_G7]  = tf->tf_global[7];
	gr[_REG_O0]  = tf->tf_out[0];
	gr[_REG_O1]  = tf->tf_out[1];
	gr[_REG_O2]  = tf->tf_out[2];
	gr[_REG_O3]  = tf->tf_out[3];
	gr[_REG_O4]  = tf->tf_out[4];
	gr[_REG_O5]  = tf->tf_out[5];
	gr[_REG_O6]  = tf->tf_out[6];
	gr[_REG_O7]  = tf->tf_out[7];
#ifdef __arch64__
	gr[_REG_ASI] = (tf->tf_tstate & TSTATE_ASI) >> TSTATE_ASI_SHIFT;
#if 0 /* not yet supported */
	gr[_REG_FPRS] = ;
#endif
#endif /* __arch64__ */

	if ((ras_pc = (__greg_t)ras_lookup(l->l_proc,
	    (caddr_t) gr[_REG_PC])) != -1) {
		gr[_REG_PC] = ras_pc;
		gr[_REG_nPC] = ras_pc + 4;
	}

	*flags |= _UC_CPU;

	mcp->__gwins = NULL;


	/* Save FP register context, if any. */
	if (l->l_md.md_fpstate != NULL) {
		struct fpstate64 fs, *fsp;
		__fpregset_t *fpr = &mcp->__fpregs;

		/*
		 * If our FP context is currently held in the FPU, take a
		 * private snapshot - lazy FPU context switching can deal
		 * with it later when it becomes necessary.
		 * Otherwise, get it from the process's save area.
		 */
		if (l == fplwp) {
			fsp = &fs;
			savefpstate(fsp);
		} else {
			fsp = l->l_md.md_fpstate;
		}
		memcpy(&fpr->__fpu_fr, fsp->fs_regs, sizeof (fpr->__fpu_fr));
		mcp->__fpregs.__fpu_q = NULL;	/* `Need more info.' */
		mcp->__fpregs.__fpu_fsr = fsp->fs_fsr;
		mcp->__fpregs.__fpu_qcnt = 0 /*fs.fs_qsize*/; /* See above */
		mcp->__fpregs.__fpu_q_entrysize =
		    (unsigned char) sizeof (*mcp->__fpregs.__fpu_q);
		mcp->__fpregs.__fpu_en = 1;
		*flags |= _UC_FPU;
	} else {
		mcp->__fpregs.__fpu_en = 0;
	}

	mcp->__xrs.__xrs_id = 0;	/* Solaris extension? */
}

int
cpu_setmcontext(l, mcp, flags)
	struct lwp *l;
	const mcontext_t *mcp;
	unsigned int flags;
{
	__greg_t *gr = mcp->__gregs;
	struct trapframe64 *tf = l->l_md.md_tf;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l))
		sigexit(l, SIGILL);

	if ((flags & _UC_CPU) != 0) {
		/*
	 	 * Only the icc bits in the psr are used, so it need not be
	 	 * verified.  pc and npc must be multiples of 4.  This is all
	 	 * that is required; if it holds, just do it.
		 */
		if (((gr[_REG_PC] | gr[_REG_nPC]) & 3) != 0 ||
		    gr[_REG_PC] == 0 || gr[_REG_nPC] == 0)
			return (EINVAL);

		/* Restore general register context. */
		/* take only tstate CCR (and ASI) fields */
#ifdef __arch64__
		tf->tf_tstate = (tf->tf_tstate & ~(TSTATE_CCR | TSTATE_ASI)) |
		    ((gr[_REG_CCR] << TSTATE_CCR_SHIFT) & TSTATE_CCR) |
		    ((gr[_REG_ASI] << TSTATE_ASI_SHIFT) & TSTATE_ASI);
#else
		tf->tf_tstate = (tf->tf_tstate & ~TSTATE_CCR) |
		    PSRCC_TO_TSTATE(gr[_REG_PSR]);
#endif
		tf->tf_pc        = (u_int64_t)gr[_REG_PC];
		tf->tf_npc       = (u_int64_t)gr[_REG_nPC];
		tf->tf_y         = (u_int64_t)gr[_REG_Y];
		tf->tf_global[1] = (u_int64_t)gr[_REG_G1];
		tf->tf_global[2] = (u_int64_t)gr[_REG_G2];
		tf->tf_global[3] = (u_int64_t)gr[_REG_G3];
		tf->tf_global[4] = (u_int64_t)gr[_REG_G4];
		tf->tf_global[5] = (u_int64_t)gr[_REG_G5];
		tf->tf_global[6] = (u_int64_t)gr[_REG_G6];
		tf->tf_global[7] = (u_int64_t)gr[_REG_G7];
		tf->tf_out[0]    = (u_int64_t)gr[_REG_O0];
		tf->tf_out[1]    = (u_int64_t)gr[_REG_O1];
		tf->tf_out[2]    = (u_int64_t)gr[_REG_O2];
		tf->tf_out[3]    = (u_int64_t)gr[_REG_O3];
		tf->tf_out[4]    = (u_int64_t)gr[_REG_O4];
		tf->tf_out[5]    = (u_int64_t)gr[_REG_O5];
		tf->tf_out[6]    = (u_int64_t)gr[_REG_O6];
		tf->tf_out[7]    = (u_int64_t)gr[_REG_O7];
		/* %asi restored above; %fprs not yet supported. */

		/* XXX mcp->__gwins */
	}

	/* Restore FP register context, if any. */
	if ((flags & _UC_FPU) != 0 && mcp->__fpregs.__fpu_en != 0) {
		struct fpstate64 *fsp;
		const __fpregset_t *fpr = &mcp->__fpregs;

		/*
		 * If we're the current FPU owner, simply reload it from
		 * the supplied context.  Otherwise, store it into the
		 * process' FPU save area (which is used to restore from
		 * by lazy FPU context switching); allocate it if necessary.
		 */
		if ((fsp = l->l_md.md_fpstate) == NULL) {
			fsp = malloc(sizeof (*fsp), M_SUBPROC, M_WAITOK);
			l->l_md.md_fpstate = fsp;
		} else if (l == fplwp) {
			/* Drop the live context on the floor. */
			savefpstate(fsp);
		}
		/* Note: sizeof fpr->__fpu_fr <= sizeof fsp->fs_regs. */
		memcpy(fsp->fs_regs, &fpr->__fpu_fr, sizeof (fpr->__fpu_fr));
		fsp->fs_fsr = mcp->__fpregs.__fpu_fsr;
		fsp->fs_qsize = 0;

#if 0
		/* Need more info! */
		mcp->__fpregs.__fpu_q = NULL;	/* `Need more info.' */
		mcp->__fpregs.__fpu_qcnt = 0 /*fs.fs_qsize*/; /* See above */
#endif

	}

	/* XXX mcp->__xrs */
	/* XXX mcp->__asrs */

	if (flags & _UC_SETSTACK)
		l->l_proc->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_proc->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	return (0);
}

