/*	$NetBSD: machdep.c,v 1.262.2.4 2013/01/16 05:33:07 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.262.2.4 2013/01/16 05:33:07 yamt Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_modular.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_svr4.h"
#include "opt_compat_sunos.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/proc.h>
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
#include <sys/cpu.h>
#include <sys/module.h>
#include <sys/ksyms.h>

#include <sys/exec_aout.h>

#include <dev/mm.h>

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
#include <sys/bus.h>
#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/pcb.h>
#include <machine/pmap.h>
#include <machine/openfirm.h>
#include <machine/sparc64.h>

#include <sparc64/sparc64/cache.h>

/* #include "fb.h" */
#include "ksyms.h"

int bus_space_debug = 0; /* This may be used by macros elsewhere. */
#ifdef DEBUG
#define DPRINTF(l, s)   do { if (bus_space_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

#if defined(COMPAT_16) || defined(COMPAT_SVR4) || defined(COMPAT_SVR4_32) || defined(COMPAT_SUNOS)
#ifdef DEBUG
/* See <sparc64/sparc64/sigdebug.h> */
int sigdebug = 0x0;
int sigpid = 0;
#endif
#endif

extern vaddr_t avail_end;
#ifdef MODULAR
vaddr_t module_start, module_end;
static struct vm_map module_map_store;
extern struct vm_map *module_map;
#endif

extern	void *msgbufaddr;

/*
 * Maximum number of DMA segments we'll allow in dmamem_load()
 * routines.  Can be overridden in config files, etc.
 */
#ifndef MAX_DMA_SEGS
#define MAX_DMA_SEGS	20
#endif

void	dumpsys(void);
void	stackdump(void);


/*
 * Machine-dependent startup code
 */
void
cpu_startup(void)
{
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;
#endif
	char pbuf[9];

#ifdef DEBUG
	pmapdebug = 0;
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	/*identifycpu();*/
	format_bytes(pbuf, sizeof(pbuf), ctob((uint64_t)physmem));
	printf("total memory = %s\n", pbuf);

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

#if 0
	pmap_redzone();
#endif

#ifdef MODULAR
	uvm_map_setup(&module_map_store, module_start, module_end, 0);
	module_map_store.pmap = pmap_kernel();
	module_map = &module_map_store;
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
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct trapframe64 *tf = l->l_md.md_tf;
	struct fpstate64 *fs;
	int64_t tstate;
	int pstate = PSTATE_USER;
#ifdef __arch64__
	Elf_Ehdr *eh = pack->ep_hdr;
#endif

	/* Clear the P_32 flag. */
	l->l_proc->p_flag &= ~PK_32;

	/* Don't allow misaligned code by default */
	l->l_proc->p_md.md_flags &= ~MDP_FIXALIGN;

	/*
	 * Set the registers to 0 except for:
	 *	%o6: stack pointer, built in exec())
	 *	%tstate: (retain icc and xcc and cwp bits)
	 *	%g1: p->p_psstrp (used by crt0)
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
	tstate = ((int64_t)ASI_PRIMARY_NO_FAULT << TSTATE_ASI_SHIFT) |
	    (pstate << TSTATE_PSTATE_SHIFT) | (tf->tf_tstate & TSTATE_CWP);
	if ((fs = l->l_md.md_fpstate) != NULL) {
		/*
		 * We hold an FPU state.  If we own *the* FPU chip state
		 * we must get rid of it, and the only way to do that is
		 * to save it.  In any case, get rid of our FPU state.
		 */
		fpusave_lwp(l, false);
		pool_cache_put(fpstate_cache, fs);
		l->l_md.md_fpstate = NULL;
	}
	memset(tf, 0, sizeof *tf);
	tf->tf_tstate = tstate;
	tf->tf_global[1] = l->l_proc->p_psstrp;
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
parse_bootfile(char *args)
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
parse_bootargs(char *args)
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
	const char *cp;

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

	/*XXXUNCONST*/
	node.sysctl_data = __UNCONST(cp);
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
	struct trapframe64 *tf = l->l_md.md_tf;

	/*
	 * Compute new user stack addresses, subtract off
	 * one signal frame, and align.
	 */
	*onstack = (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
	    && (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	if (*onstack)
		return ((char *)l->l_sigstk.ss_sp + l->l_sigstk.ss_size);
	else
		return (void *)((uintptr_t)tf->tf_out[6] + STACK_OFFSET);
}

struct sigframe_siginfo {
	siginfo_t	sf_si;		/* saved siginfo */
	ucontext_t	sf_uc;		/* saved ucontext */
};

void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	int onstack, error;
	int sig = ksi->ksi_signo;
	ucontext_t uc;
	long ucsz;
	struct sigframe_siginfo *fp = getframe(l, sig, &onstack);
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct trapframe64 *tf = l->l_md.md_tf;
	struct rwindow *newsp;
	/* Allocate an aligned sigframe */
	fp = (void *)((u_long)(fp - 1) & ~0x0f);

	uc.uc_flags = _UC_SIGMASK |
	    ((l->l_sigstk.ss_flags & SS_ONSTACK)
		? _UC_SETSTACK : _UC_CLRSTACK);
	uc.uc_sigmask = *mask;
	uc.uc_link = l->l_ctxlink;
	memset(&uc.uc_stack, 0, sizeof(uc.uc_stack));

	sendsig_reset(l, sig);
	mutex_exit(p->p_lock);	
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
	error = (copyout(&ksi->ksi_info, &fp->sf_si, sizeof(ksi->ksi_info)) != 0 ||
	    copyout(&uc, &fp->sf_uc, ucsz) != 0 ||
	    suword(&newsp->rw_in[6], (uintptr_t)tf->tf_out[6]) != 0);
	mutex_enter(p->p_lock);

	if (error) {
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
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

struct pcb dumppcb;

static void
maybe_dump(int howto)
{
	int s;

	/* Disable interrupts. */
	s = splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

	splx(s);
}

void
cpu_reboot(int howto, char *user_boot_string)
{
	static bool syncdone = false;
	int i;
	static char str[128];
	struct lwp *l;

	l = (curlwp == NULL) ? &lwp0 : curlwp;

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

#if NFB > 0
	fb_unblank();
#endif
	boothowto = howto;

	/* If rebooting and a dump is requested, do it.
	 *
	 * XXX used to dump after vfs_shutdown() and before
	 * detaching devices / shutdown hooks / pmf_system_shutdown().
	 */
	maybe_dump(howto);

	if ((howto & RB_NOSYNC) == 0 && !syncdone) {
		extern struct lwp lwp0;

		if (!syncdone) {
		syncdone = true;
		vfs_shutdown();
			/* XXX used to force unmount as well, here */
			vfs_sync_all(l);
			/*
			 * If we've been adjusting the clock, the todr
			 * will be out of synch; adjust it now.
			 *
			 * resettodr will only do this only if inittodr()
			 * has already been called.
			 *
			 * XXX used to do this after unmounting all
			 * filesystems with vfs_shutdown().
			 */
			resettodr();
		}

		while (vfs_unmountall1(l, false, false) ||
		       config_detach_all(boothowto) ||
		       vfs_unmount_forceone(l))
			;	/* do nothing */
	} else
		suspendsched();

	pmf_system_shutdown(boothowto);

	splhigh();

haltsys:

#ifdef MULTIPROCESSOR
	/* Stop all secondary cpus */
	mp_halt_cpus();
#endif

	/* If powerdown was requested, do it. */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
#ifdef MULTIPROCESSOR
		printf("cpu%d: powered down\n\n", cpu_number());
#else
		printf("powered down\n\n");
#endif
		/* Let the OBP do the work. */
		OF_poweroff();
		printf("WARNING: powerdown failed!\n");
		/*
		 * RB_POWERDOWN implies RB_HALT... fall into it...
		 */
	}

	if (howto & RB_HALT) {
#ifdef MULTIPROCESSOR
		printf("cpu%d: halted\n\n", cpu_number());
#else
		printf("halted\n\n");
#endif
		OF_exit();
		panic("PROM exit failed");
	}

#ifdef MULTIPROCESSOR
	printf("cpu%d: rebooting\n\n", cpu_number());
#else
	printf("rebooting\n\n");
#endif
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

uint32_t dumpmag = 0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
long	dumplo = 0;

void
cpu_dumpconf(void)
{
	int nblks, dumpblks;

	if (dumpdev == NODEV)
		/* No usable dump device */
		return;
	nblks = bdev_size(dumpdev);

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

#define	BYTES_PER_DUMP	MAXPHYS		/* must be a multiple of pagesize */
static vaddr_t dumpspace;

void *
reserve_dumppages(void *p)
{

	dumpspace = (vaddr_t)p;
	return (char *)p + BYTES_PER_DUMP;
}

/*
 * Write a crash dump.
 */
void
dumpsys(void)
{
	const struct bdevsw *bdev;
	int psize;
	daddr_t blkno;
	int (*dump)(dev_t, daddr_t, void *, size_t);
	int j, error = 0;
	uint64_t todo;
	struct mem_region *mp;

	/* copy registers to dumppcb and flush windows */
	memset(&dumppcb, 0, sizeof(struct pcb));
	snapshot(&dumppcb);
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
		printf("\ndump to dev %" PRId32 ",%" PRId32 " not possible ("
		    "partition too small?)\n", major(dumpdev), minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %" PRId32 ",%" PRId32 " offset %ld\n",
	    major(dumpdev), minor(dumpdev), dumplo);

	psize = bdev_size(dumpdev);
	if (psize == -1) {
		printf("dump area unavailable\n");
		return;
	}
	blkno = dumplo;
	dump = bdev->d_dump;

	error = pmap_dumpmmu(dump, blkno);
	blkno += pmap_dumpsize();

	/* calculate total size of dump */
	for (todo = 0, j = 0; j < phys_installed_size; j++)
		todo += phys_installed[j].size;

	for (mp = &phys_installed[0], j = 0; j < phys_installed_size;
			j++, mp = &phys_installed[j]) {
		uint64_t i = 0, n, off;
		paddr_t maddr = mp->start;

		for (; i < mp->size; i += n) {
			n = mp->size - i;
			if (n > BYTES_PER_DUMP)
				 n = BYTES_PER_DUMP;

			/* print out how many MBs we still have to dump */
			if ((todo % (1024*1024)) == 0)
				printf_nolog("\r%6" PRIu64 " M ",
				    todo / (1024*1024));
			for (off = 0; off < n; off += PAGE_SIZE)
				pmap_kenter_pa(dumpspace+off, maddr+off,
				    VM_PROT_READ, 0);
			error = (*dump)(dumpdev, blkno,
					(void *)dumpspace, (size_t)n);
			pmap_kremove(dumpspace, n);
			if (error)
				break;
			maddr += n;
			todo -= n;
			blkno += btodb(n);
		}
	}

	switch (error) {

	case ENXIO:
		printf("- device bad\n");
		break;

	case EFAULT:
		printf("- device not ready\n");
		break;

	case EINVAL:
		printf("- area improper\n");
		break;

	case EIO:
		printf("- i/o error\n");
		break;

	case 0:
		printf("\rdump succeeded\n");
		break;

	default:
		printf("- error %d\n", error);
		break;
	}
}

void trapdump(struct trapframe64*);
/*
 * dump out a trapframe.
 */
void
trapdump(struct trapframe64* tf)
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

static void
get_symbol_and_offset(const char **mod, const char **sym, vaddr_t *offset, vaddr_t pc)
{
	static char symbuf[256];
	unsigned long symaddr;

#if NKSYMS || defined(DDB) || defined(MODULAR)
	if (ksyms_getname(mod, sym, pc,
			  KSYMS_CLOSEST|KSYMS_PROC|KSYMS_ANY) == 0) {
		if (ksyms_getval(*mod, *sym, &symaddr,
				 KSYMS_CLOSEST|KSYMS_PROC|KSYMS_ANY) != 0)
			goto failed;

		*offset = (vaddr_t)(pc - symaddr);
		return;
	}
#endif
 failed:
	snprintf(symbuf, sizeof symbuf, "%llx", (unsigned long long)pc);
	*mod = "netbsd";
	*sym = symbuf;
	*offset = 0;
}

/*
 * get the fp and dump the stack as best we can.  don't leave the
 * current stack page
 */
void
stackdump(void)
{
	struct frame32 *fp = (struct frame32 *)getfp(), *sfp;
	struct frame64 *fp64;
	const char *mod, *sym;
	vaddr_t offset;

	sfp = fp;
	printf("Frame pointer is at %p\n", fp);
	printf("Call traceback:\n");
	while (fp && ((u_long)fp >> PGSHIFT) == ((u_long)sfp >> PGSHIFT)) {
		if( ((long)fp) & 1 ) {
			fp64 = (struct frame64*)(((char*)fp)+BIAS);
			/* 64-bit frame */
			get_symbol_and_offset(&mod, &sym, &offset, fp64->fr_pc);
			printf(" %s:%s+%#llx(%llx, %llx, %llx, %llx, %llx, %llx) fp = %llx\n",
			       mod, sym,
			       (unsigned long long)offset,
			       (unsigned long long)fp64->fr_arg[0],
			       (unsigned long long)fp64->fr_arg[1],
			       (unsigned long long)fp64->fr_arg[2],
			       (unsigned long long)fp64->fr_arg[3],
			       (unsigned long long)fp64->fr_arg[4],
			       (unsigned long long)fp64->fr_arg[5],	
			       (unsigned long long)fp64->fr_fp);
			fp = (struct frame32 *)(u_long)fp64->fr_fp;
		} else {
			/* 32-bit frame */
			get_symbol_and_offset(&mod, &sym, &offset, fp->fr_pc);
			printf(" %s:%s+%#lx(%x, %x, %x, %x, %x, %x) fp = %x\n",
			       mod, sym,
			       (unsigned long)offset,
			       fp->fr_arg[0],
			       fp->fr_arg[1],
			       fp->fr_arg[2],
			       fp->fr_arg[3],
			       fp->fr_arg[4],
			       fp->fr_arg[5],
			       fp->fr_fp);
			fp = (struct frame32*)(u_long)fp->fr_fp;
		}
	}
}


int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
	return (ENOEXEC);
}

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
	bus_size_t maxsegsz, bus_size_t boundary, int flags,
	bus_dmamap_t *dmamp)
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
	map->_dm_maxmaxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT|BUS_DMA_COHERENT|
				   BUS_DMA_NOWRITE|BUS_DMA_NOCACHE);
	map->dm_maxsegsz = maxsegsz;
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
_bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
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
_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *sbuf,
	bus_size_t buflen, struct proc *p, int flags)
{
	bus_size_t sgsize;
	vaddr_t vaddr = (vaddr_t)sbuf;
	long incr;
	int i;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

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
	i = 0;
	map->dm_segs[i].ds_addr = 0UL;
	map->dm_segs[i].ds_len = 0;

	incr = PAGE_SIZE - (vaddr & PGOFSET);
	while (sgsize > 0) {
		paddr_t pa;
	
		incr = min(sgsize, incr);

		(void) pmap_extract(pmap_kernel(), vaddr, &pa);
		if (map->dm_segs[i].ds_len == 0)
			map->dm_segs[i].ds_addr = pa;
		if (pa == (map->dm_segs[i].ds_addr + map->dm_segs[i].ds_len)
		    && ((map->dm_segs[i].ds_len + incr) <= map->dm_maxsegsz)) {
			/* Hey, waddyaknow, they're contiguous */
			map->dm_segs[i].ds_len += incr;
		} else {
			if (++i >= map->_dm_segcnt)
				return (EFBIG);
			map->dm_segs[i].ds_addr = pa;
			map->dm_segs[i].ds_len = incr;
		}
		sgsize -= incr;
		vaddr += incr;
		incr = PAGE_SIZE;
	}
	map->dm_nsegs = i + 1;
	map->dm_mapsize = buflen;
	/* Mapping is bus dependent */
	return (0);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m,
	int flags)
{
	bus_dma_segment_t segs[MAX_DMA_SEGS];
	int i;
	size_t len;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

	if (m->m_pkthdr.len > map->_dm_size)
		return EINVAL;

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

			if (pmap_extract(pmap_kernel(), vaddr, &pa) == FALSE) {
#ifdef DIAGNOSTIC
				printf("_bus_dmamap_load_mbuf: pmap_extract failed %lx\n",
				       vaddr);
#endif
				map->_dm_type = 0;
				map->_dm_source = NULL;
				return EINVAL;
			}

			buflen -= incr;
			vaddr += incr;

			if (i > 0 && 
				pa == (segs[i-1].ds_addr + segs[i-1].ds_len) &&
				((segs[i-1].ds_len + incr) <= 
					map->dm_maxsegsz)) {
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
			return EFBIG;
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
		if (retval == 0) {
			if (map->dm_mapsize != len)
				panic("load_mbuf: mapsize %ld != len %lx\n",
					(long)map->dm_mapsize, len);
			sglen = 0;
			for (j = 0; j < map->dm_nsegs; j++)
				sglen += map->dm_segs[j].ds_len;
			if (sglen != len)
				panic("load_mbuf: dmamap sglen %ld != len %lx\n",
					sglen, len);
		}
		return (retval);
	}
#endif
	return (bus_dmamap_load_raw(t, map, segs, i, (bus_size_t)len, flags));
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
	int flags)
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
	struct proc *p = uio->uio_lwp->l_proc;
	struct pmap *pm;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

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
		if (__predict_false(uvm_vslock(p->p_vmspace, vaddr, buflen,
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
			    && ((segs[i-1].ds_len + incr) <= map->dm_maxsegsz)) {
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
		uvm_vsunlock(p->p_vmspace, bp->b_data, todo);
 		if (buflen > 0 && i >= MAX_DMA_SEGS) 
			/* Exceeded the size of our dmamap */
			return EFBIG;
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
_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map, bus_dma_segment_t *segs,
	int nsegs, bus_size_t size, int flags)
{

	panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * bus-specific DMA map unload functions.
 */
void
_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
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
		TAILQ_FOREACH(pg, pglist, pageq.queue) {
			pa = VM_PAGE_TO_PHYS(pg);

			/* 
			 * We should be flushing a subrange, but we
			 * don't know where the segments starts.
			 */
			dcache_flush_page_all(pa);
		}
	}

	/* Mark the mappings as invalid. */
	map->dm_maxsegsz = map->_dm_maxmaxsegsz;
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

}

/*
 * Common function for DMA map synchronization.  May be called
 * by bus-specific DMA map synchronization functions.
 */
void
_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
	bus_size_t len, int ops)
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
		membar_Sync();
	}
	if (ops & BUS_DMASYNC_POSTREAD) {
		/* Invalidate the vcache */
		for (i = 0; i < map->dm_nsegs; i++) {
			if ((pglist = map->dm_segs[i]._ds_mlist) == NULL)
				/* Should not really happen. */
				continue;
			TAILQ_FOREACH(pg, pglist, pageq.queue) {
				paddr_t start;
				psize_t size = PAGE_SIZE;

				if (offset < PAGE_SIZE) {
					start = VM_PAGE_TO_PHYS(pg) + offset;
					size -= offset;
					if (size > len)
						size = len;
					cache_flush_phys(start, size, 0);
					len -= size;
					if (len == 0)
						goto done;
					offset = 0;
					continue;
				}
				offset -= size;
			}
		}
	}
 done:
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
_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
	bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
	int flags)
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
	 * NOBODY SHOULD TOUCH THE pageq.queue FIELDS WHILE THESE PAGES
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
_bus_dmamem_free(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs)
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
_bus_dmamem_map(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs,
	size_t size, void **kvap, int flags)
{
	vaddr_t va, sva;
	int r;
	size_t oversize;
	u_long align;

	if (nsegs != 1)
		panic("_bus_dmamem_map: nsegs = %d", nsegs);

	align = PAGE_SIZE;

	size = round_page(size);

	/*
	 * Find a region of kernel virtual addresses that can accommodate
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

	*kvap = (void *)va;
	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
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
_bus_dmamem_mmap(bus_dma_tag_t t, bus_dma_segment_t *segs, int nsegs, off_t off,
	int prot, int flags)
{
	int i;

	for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
		if (off & PGOFSET)
			panic("_bus_dmamem_mmap: offset unaligned");
		if (segs[i].ds_addr & PGOFSET)
			panic("_bus_dmamem_mmap: segment unaligned");
		if (segs[i].ds_len & PGOFSET)
			panic("_bus_dmamem_mmap: segment size not multiple"
			    " of page size");
#endif
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		return (atop(segs[i].ds_addr + off));
	}

	/* Page not found. */
	return (-1);
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
static int	sparc_bus_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
	vaddr_t, bus_space_handle_t *);
static int	sparc_bus_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);
static int	sparc_bus_subregion(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	bus_size_t, bus_space_handle_t *);
static paddr_t	sparc_bus_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
static void	*sparc_mainbus_intr_establish(bus_space_tag_t, int, int,
	int (*)(void *), void *, void (*)(void));
static int	sparc_bus_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t,
	bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
static void	sparc_bus_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);

struct extent *io_space = NULL;

void
bus_space_barrier(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, bus_size_t s, int f)
{
	/*
	 * We have a bit of a problem with the bus_space_barrier()
	 * interface.  It defines a read barrier and a write barrier
	 * which really don't map to the 7 different types of memory
	 * barriers in the SPARC v9 instruction set.
	 */
	if (f == BUS_SPACE_BARRIER_READ)
		/* A load followed by a load to the same location? */
		__asm volatile("membar #Lookaside");
	else if (f == BUS_SPACE_BARRIER_WRITE)
		/* A store followed by a store? */
		__asm volatile("membar #StoreStore");
	else 
		/* A store followed by a load? */
		__asm volatile("membar #StoreLoad|#MemIssue|#Lookaside");
}

int
bus_space_alloc(bus_space_tag_t t, bus_addr_t rs, bus_addr_t re, bus_size_t s,
	bus_size_t a, bus_size_t b, int f, bus_addr_t *ap,
	bus_space_handle_t *hp)
{
	_BS_CALL(t, sparc_bus_alloc)(t, rs, re, s, a, b, f, ap, hp);
}

void
bus_space_free(bus_space_tag_t t, bus_space_handle_t h, bus_size_t s)
{
	_BS_CALL(t, sparc_bus_free)(t, h, s);
}

int
bus_space_map(bus_space_tag_t t, bus_addr_t a, bus_size_t s, int f,
	bus_space_handle_t *hp)
{
	_BS_CALL(t, sparc_bus_map)(t, a, s, f, 0, hp);
}

void
bus_space_unmap(bus_space_tag_t t, bus_space_handle_t h, bus_size_t s)
{
	_BS_VOID_CALL(t, sparc_bus_unmap)(t, h, s);
}

int
bus_space_subregion(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	bus_size_t s, bus_space_handle_t *hp)
{
	_BS_CALL(t, sparc_bus_subregion)(t, h, o, s, hp);
}

paddr_t
bus_space_mmap(bus_space_tag_t t, bus_addr_t a, off_t o, int p, int f)
{
	_BS_CALL(t, sparc_bus_mmap)(t, a, o, p, f);
}

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */
void
bus_space_read_multi_1(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, uint8_t * a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_1(t, h, o);
}

void
bus_space_read_multi_2(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, uint16_t * a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_2(t, h, o);
}

void
bus_space_read_multi_4(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, uint32_t * a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_4(t, h, o);
}

void
bus_space_read_multi_8(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, uint64_t * a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_8(t, h, o);
}

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const uintN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */
void
bus_space_write_multi_1(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	const uint8_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_1(t, h, o, *a++);
}

void
bus_space_write_multi_2(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	const uint16_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_2(t, h, o, *a++);
}

void
bus_space_write_multi_4(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	const uint32_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_4(t, h, o, *a++);
}

void
bus_space_write_multi_8(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	const uint64_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_8(t, h, o, *a++);
}

/*
 *	void bus_space_set_multi_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *	    bus_size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
void
bus_space_set_multi_stream_1(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o, uint8_t v,
	bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_stream_1(t, h, o, v);
}

void
bus_space_set_multi_stream_2(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o, uint16_t v,
	bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_stream_2(t, h, o, v);
}

void
bus_space_set_multi_stream_4(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o, uint32_t v,
	bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_stream_4(t, h, o, v);
}

void
bus_space_set_multi_stream_8(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o, uint64_t v,
	bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_stream_8(t, h, o, v);
}

/*
 *	void bus_space_copy_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    bus_size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */
void
bus_space_copy_region_stream_1(bus_space_tag_t t, bus_space_handle_t h1,
	bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1++, o2++)
	    bus_space_write_stream_1(t, h1, o1, bus_space_read_stream_1(t, h2, o2));
}

void
bus_space_copy_region_stream_2(bus_space_tag_t t, bus_space_handle_t h1,
	bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=2, o2+=2)
	    bus_space_write_stream_2(t, h1, o1, bus_space_read_stream_2(t, h2, o2));
}

void
bus_space_copy_region_stream_4(bus_space_tag_t t, bus_space_handle_t h1,
	bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=4, o2+=4)
	    bus_space_write_stream_4(t, h1, o1, bus_space_read_stream_4(t, h2, o2));
}

void
bus_space_copy_region_stream_8(bus_space_tag_t t, bus_space_handle_t h1,
	bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=8, o2+=8)
	    bus_space_write_stream_8(t, h1, o1, bus_space_read_8(t, h2, o2));
}

/*
 *	void bus_space_set_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    uintN_t *addr, bus_size_t count);
 *
 */
void
bus_space_set_region_stream_1(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, const uint8_t v, bus_size_t c)
{
	for (; c; c--, o++)
		bus_space_write_stream_1(t, h, o, v);
}

void
bus_space_set_region_stream_2(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, const uint16_t v, bus_size_t c)
{
	for (; c; c--, o+=2)
		bus_space_write_stream_2(t, h, o, v);
}

void
bus_space_set_region_stream_4(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, const uint32_t v, bus_size_t c)
{
	for (; c; c--, o+=4)
		bus_space_write_stream_4(t, h, o, v);
}

void
bus_space_set_region_stream_8(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, const uint64_t v, bus_size_t c)
{
	for (; c; c--, o+=8)
		bus_space_write_stream_8(t, h, o, v);
}


/*
 *	void bus_space_read_multi_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t *addr, bus_size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */
void
bus_space_read_multi_stream_1(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	uint8_t *a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_stream_1(t, h, o);
}

void
bus_space_read_multi_stream_2(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	uint16_t *a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_stream_2(t, h, o);
}

void
bus_space_read_multi_stream_4(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	uint32_t *a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_stream_4(t, h, o);
}

void
bus_space_read_multi_stream_8(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	uint64_t *a, bus_size_t c)
{
	while (c-- > 0)
		*a++ = bus_space_read_stream_8(t, h, o);
}

/*
 *	void bus_space_read_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    uintN_t *addr, bus_size_t count);
 *
 */
void
bus_space_read_region_stream_1(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, uint8_t *a, bus_size_t c)
{
	for (; c; a++, c--, o++)
		*a = bus_space_read_stream_1(t, h, o);
}
void
bus_space_read_region_stream_2(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, uint16_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=2)
		*a = bus_space_read_stream_2(t, h, o);
 }
void
bus_space_read_region_stream_4(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, uint32_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=4)
		*a = bus_space_read_stream_4(t, h, o);
}
void
bus_space_read_region_stream_8(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, uint64_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=8)
		*a = bus_space_read_stream_8(t, h, o);
}

/*
 *	void bus_space_write_multi_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const uintN_t *addr, bus_size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */
void
bus_space_write_multi_stream_1(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	const uint8_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_stream_1(t, h, o, *a++);
}

void
bus_space_write_multi_stream_2(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	const uint16_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_stream_2(t, h, o, *a++);
}

void
bus_space_write_multi_stream_4(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	const uint32_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_stream_4(t, h, o, *a++);
}

void
bus_space_write_multi_stream_8(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o,
	const uint64_t *a, bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_stream_8(t, h, o, *a++);
}

/*
 *	void bus_space_copy_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    bus_size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */
void
bus_space_copy_region_1(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
	bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1++, o2++)
	    bus_space_write_1(t, h1, o1, bus_space_read_1(t, h2, o2));
}

void
bus_space_copy_region_2(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
	bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=2, o2+=2)
	    bus_space_write_2(t, h1, o1, bus_space_read_2(t, h2, o2));
}

void
bus_space_copy_region_4(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
	bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=4, o2+=4)
	    bus_space_write_4(t, h1, o1, bus_space_read_4(t, h2, o2));
}

void
bus_space_copy_region_8(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
	bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=8, o2+=8)
	    bus_space_write_8(t, h1, o1, bus_space_read_8(t, h2, o2));
}

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    uintN_t *addr, bus_size_t count);
 *
 */
void
bus_space_set_region_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	const uint8_t v, bus_size_t c)
{
	for (; c; c--, o++)
		bus_space_write_1(t, h, o, v);
}

void
bus_space_set_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	const uint16_t v, bus_size_t c)
{
	for (; c; c--, o+=2)
		bus_space_write_2(t, h, o, v);
}

void
bus_space_set_region_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	const uint32_t v, bus_size_t c)
{
	for (; c; c--, o+=4)
		bus_space_write_4(t, h, o, v);
}

void
bus_space_set_region_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	const uint64_t v, bus_size_t c)
{
	for (; c; c--, o+=8)
		bus_space_write_8(t, h, o, v);
}


/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *	    bus_size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
void
bus_space_set_multi_1(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o, uint8_t v,
	bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_1(t, h, o, v);
}

void
bus_space_set_multi_2(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o, uint16_t v,
	bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_2(t, h, o, v);
}

void
bus_space_set_multi_4(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o, uint32_t v,
	bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_4(t, h, o, v);
}

void
bus_space_set_multi_8(bus_space_tag_t t,
	bus_space_handle_t h, bus_size_t o, uint64_t v,
	bus_size_t c)
{
	while (c-- > 0)
		bus_space_write_8(t, h, o, v);
}

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    uintN_t *addr, bus_size_t count);
 *
 */
void
bus_space_write_region_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	const uint8_t *a, bus_size_t c)
{
	for (; c; a++, c--, o++)
		bus_space_write_1(t, h, o, *a);
}

void
bus_space_write_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	const uint16_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=2)
		bus_space_write_2(t, h, o, *a);
}

void
bus_space_write_region_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	const uint32_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=4)
		bus_space_write_4(t, h, o, *a);
}

void
bus_space_write_region_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	const uint64_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=8)
		bus_space_write_8(t, h, o, *a);
}


/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    uintN_t *addr, bus_size_t count);
 *
 */
void
bus_space_read_region_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	uint8_t *a, bus_size_t c)
{
	for (; c; a++, c--, o++)
		*a = bus_space_read_1(t, h, o);
}
void
bus_space_read_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	uint16_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=2)
		*a = bus_space_read_2(t, h, o);
 }
void
bus_space_read_region_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	uint32_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=4)
		*a = bus_space_read_4(t, h, o);
}
void
bus_space_read_region_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
	uint64_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=8)
		*a = bus_space_read_8(t, h, o);
}

/*
 *	void bus_space_write_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t off,
 *	    uintN_t *addr, bus_size_t count);
 *
 */
void
bus_space_write_region_stream_1(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, const uint8_t *a, bus_size_t c)
{
	for (; c; a++, c--, o++)
		bus_space_write_stream_1(t, h, o, *a);
}

void
bus_space_write_region_stream_2(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, const uint16_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=2)
		bus_space_write_stream_2(t, h, o, *a);
}

void
bus_space_write_region_stream_4(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, const uint32_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=4)
		bus_space_write_stream_4(t, h, o, *a);
}

void
bus_space_write_region_stream_8(bus_space_tag_t t, bus_space_handle_t h,
	bus_size_t o, const uint64_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=8)
		bus_space_write_stream_8(t, h, o, *a);
}

/*
 * Allocate a new bus tag and have it inherit the methods of the
 * given parent.
 */
bus_space_tag_t
bus_space_tag_alloc(bus_space_tag_t parent, void *cookie)
{
	struct sparc_bus_space_tag *sbt;

	sbt = malloc(sizeof(struct sparc_bus_space_tag),
		     M_DEVBUF, M_NOWAIT|M_ZERO);
	if (sbt == NULL)
		return (NULL);

	if (parent) {
		memcpy(sbt, parent, sizeof(*sbt));
		sbt->parent = parent;
		sbt->ranges = NULL;
		sbt->nranges = 0;
	}

	sbt->cookie = cookie;
	return (sbt);
}

/*
 * Generic routine to translate an address using OpenPROM `ranges'.
 */
int
bus_space_translate_address_generic(struct openprom_range *ranges, int nranges,
    bus_addr_t *bap)
{
	int i, space = BUS_ADDR_IOSPACE(*bap);

	for (i = 0; i < nranges; i++) {
		struct openprom_range *rp = &ranges[i];

		if (rp->or_child_space != space)
			continue;

		/* We've found the connection to the parent bus. */
		*bap = BUS_ADDR(rp->or_parent_space,
		    rp->or_parent_base + BUS_ADDR_PADDR(*bap));
		return (0);
	}

	return (EINVAL);
}

int
sparc_bus_map(bus_space_tag_t t, bus_addr_t addr, bus_size_t size,
	int flags, vaddr_t unused, bus_space_handle_t *hp)
{
	vaddr_t v;
	uint64_t pa;
	paddr_t	pm_flags = 0;
	vm_prot_t pm_prot = VM_PROT_READ;
	int err, map_little = 0;

	if (io_space == NULL)
		/*
		 * And set up IOSPACE extents.
		 */
		io_space = extent_create("IOSPACE",
					 (u_long)IODEV_BASE, (u_long)IODEV_END,
					 0, 0, EX_NOWAIT);


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
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (-1);
		hp->_ptr = addr;
		hp->_asi = ASI_PHYS_NON_CACHED_LITTLE;
		hp->_sasi = ASI_PHYS_NON_CACHED;
		DPRINTF(BSDB_MAP, ("\n%s: config type %x flags %x "
			"addr %016llx size %016llx virt %llx\n", __func__,
			(int)t->type, (int) flags, (unsigned long long)addr,
			(unsigned long long)size,
			(unsigned long long)hp->_ptr));
		return (0);
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

	if (!(flags & BUS_SPACE_MAP_CACHEABLE))
		pm_flags |= PMAP_NC;

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

	pa = trunc_page(addr);
	if (!(flags&BUS_SPACE_MAP_READONLY))
		pm_prot |= VM_PROT_WRITE;

	DPRINTF(BSDB_MAP, ("\n%s: type %x flags %x addr %016llx prot %02x "
		"pm_flags %x size %016llx virt %llx paddr %016llx\n", __func__,
		(int)t->type, (int)flags, (unsigned long long)addr, pm_prot,
		(int)pm_flags, (unsigned long long)size,
		(unsigned long long)hp->_ptr, (unsigned long long)pa));

	do {
		DPRINTF(BSDB_MAP, ("%s: phys %llx virt %p hp %llx\n",
			__func__, 
			(unsigned long long)pa, (char *)v,
			(unsigned long long)hp->_ptr));
		pmap_kenter_pa(v, pa | pm_flags, pm_prot, 0);
		v += PAGE_SIZE;
		pa += PAGE_SIZE;
	} while ((size -= PAGE_SIZE) > 0);
	return (0);
}

int
sparc_bus_subregion(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, bus_size_t size, bus_space_handle_t *nhandlep)
{
	nhandlep->_ptr = handle._ptr + offset;
	nhandlep->_asi = handle._asi;
	nhandlep->_sasi = handle._sasi;
	return (0);
}

int
sparc_bus_unmap(bus_space_tag_t t, bus_space_handle_t bh, bus_size_t size)
{
	vaddr_t va = trunc_page((vaddr_t)bh._ptr);
	vaddr_t endva = va + round_page(size);
	int error = 0;

	if (PHYS_ASI(bh._asi)) return (0);

	error = extent_free(io_space, va, size, EX_NOWAIT);
	if (error) printf("sparc_bus_unmap: extent_free returned %d\n", error);

	pmap_remove(pmap_kernel(), va, endva);
	return (0);
}

paddr_t
sparc_bus_mmap(bus_space_tag_t t, bus_addr_t paddr, off_t off, int prot,
	int flags)
{
	/* Devices are un-cached... although the driver should do that */
	return ((paddr+off)|PMAP_NC);
}


void *
sparc_mainbus_intr_establish(bus_space_tag_t t, int pil, int level,
	int	(*handler)(void *), void *arg, void	(*fastvec)(void) /* ignored */)
{
	struct intrhand *ih;

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	intr_establish(pil, level != IPL_VM, ih);
	return (ih);
}

int
sparc_bus_alloc(bus_space_tag_t t, bus_addr_t rs, bus_addr_t re, bus_size_t s,
	bus_size_t a, bus_size_t b, int f, bus_addr_t *ap, bus_space_handle_t *hp)
{
	return (ENOTTY);
}

void
sparc_bus_free(bus_space_tag_t t, bus_space_handle_t h, bus_size_t s)
{
	return;
}

struct sparc_bus_space_tag mainbus_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus tag */
	NULL,				/* ranges */
	0,				/* nranges */
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
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	__greg_t *gr = mcp->__gregs;
	__greg_t ras_pc;
	const struct trapframe64 *tf = l->l_md.md_tf;

	/* First ensure consistent stack state (see sendsig). */ /* XXX? */
	write_user_windows();
	if (rwindow_save(l)) {
		mutex_enter(l->l_proc->p_lock);
		sigexit(l, SIGILL);
	}

	/* For now: Erase any random indicators for optional state. */
	(void)memset(mcp, 0, sizeof (*mcp));

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
	    (void *) gr[_REG_PC])) != -1) {
		gr[_REG_PC] = ras_pc;
		gr[_REG_nPC] = ras_pc + 4;
	}

	*flags |= (_UC_CPU|_UC_TLSBASE);

	mcp->__gwins = NULL;


	/* Save FP register context, if any. */
	if (l->l_md.md_fpstate != NULL) {
		struct fpstate64 *fsp;
		__fpregset_t *fpr = &mcp->__fpregs;

		/*
		 * If our FP context is currently held in the FPU, take a
		 * private snapshot - lazy FPU context switching can deal
		 * with it later when it becomes necessary.
		 * Otherwise, get it from the process's save area.
		 */
		fpusave_lwp(l, true);
		fsp = l->l_md.md_fpstate;
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
cpu_mcontext_validate(struct lwp *l, const mcontext_t *mc)
{
	const __greg_t *gr = mc->__gregs;

	/*
 	 * Only the icc bits in the psr are used, so it need not be
 	 * verified.  pc and npc must be multiples of 4.  This is all
 	 * that is required; if it holds, just do it.
	 */
	if (((gr[_REG_PC] | gr[_REG_nPC]) & 3) != 0 ||
	    gr[_REG_PC] == 0 || gr[_REG_nPC] == 0)
		return EINVAL;

	return 0;
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	const __greg_t *gr = mcp->__gregs;
	struct trapframe64 *tf = l->l_md.md_tf;
	struct proc *p = l->l_proc;
	int error;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l)) {
		mutex_enter(p->p_lock);
		sigexit(l, SIGILL);
	}

	if ((flags & _UC_CPU) != 0) {
		error = cpu_mcontext_validate(l, mcp);
		if (error)
			return error;

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
		tf->tf_pc        = (uint64_t)gr[_REG_PC];
		tf->tf_npc       = (uint64_t)gr[_REG_nPC];
		tf->tf_y         = (uint64_t)gr[_REG_Y];
		tf->tf_global[1] = (uint64_t)gr[_REG_G1];
		tf->tf_global[2] = (uint64_t)gr[_REG_G2];
		tf->tf_global[3] = (uint64_t)gr[_REG_G3];
		tf->tf_global[4] = (uint64_t)gr[_REG_G4];
		tf->tf_global[5] = (uint64_t)gr[_REG_G5];
		tf->tf_global[6] = (uint64_t)gr[_REG_G6];
		/* done in lwp_setprivate */
		/* tf->tf_global[7] = (uint64_t)gr[_REG_G7]; */
		tf->tf_out[0]    = (uint64_t)gr[_REG_O0];
		tf->tf_out[1]    = (uint64_t)gr[_REG_O1];
		tf->tf_out[2]    = (uint64_t)gr[_REG_O2];
		tf->tf_out[3]    = (uint64_t)gr[_REG_O3];
		tf->tf_out[4]    = (uint64_t)gr[_REG_O4];
		tf->tf_out[5]    = (uint64_t)gr[_REG_O5];
		tf->tf_out[6]    = (uint64_t)gr[_REG_O6];
		tf->tf_out[7]    = (uint64_t)gr[_REG_O7];
		/* %asi restored above; %fprs not yet supported. */

		/* XXX mcp->__gwins */

		if (flags & _UC_TLSBASE)
			lwp_setprivate(l, (void *)(uintptr_t)gr[_REG_G7]);
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
			fsp = pool_cache_get(fpstate_cache, PR_WAITOK);
			l->l_md.md_fpstate = fsp;
		} else {
			/* Drop the live context on the floor. */
			fpusave_lwp(l, false);
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

	mutex_enter(p->p_lock);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(p->p_lock);

	return 0;
}

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
void
cpu_need_resched(struct cpu_info *ci, int flags)
{

	ci->ci_want_resched = 1;
	ci->ci_want_ast = 1;

#ifdef MULTIPROCESSOR
	if (ci == curcpu())
		return;
	/* Just interrupt the target CPU, so it can notice its AST */
	if ((flags & RESCHED_IMMED) != 0 &&
	    ci->ci_data.cpu_onproc != ci->ci_data.cpu_idlelwp)
		sparc64_send_ipi(ci->ci_cpuid, sparc64_ipi_nop, 0, 0);
#endif
}

/*
 * Notify an LWP that it has a signal pending, process as soon as possible.
 */
void
cpu_signotify(struct lwp *l)
{
	struct cpu_info *ci = l->l_cpu;

	ci->ci_want_ast = 1;
#ifdef MULTIPROCESSOR
	if (ci != curcpu())
		sparc64_send_ipi(ci->ci_cpuid, sparc64_ipi_nop, 0, 0);
#endif
}

bool
cpu_intr_p(void)
{

	return curcpu()->ci_idepth >= 0;
}

#ifdef MODULAR
void
module_init_md(void)
{
}
#endif

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	return pmap_pa_exists(pa) ? 0 : EFAULT;
}

int
mm_md_kernacc(void *ptr, vm_prot_t prot, bool *handled)
{
	/* XXX: Don't know where PROMs are on Ultras.  Think it's at f000000 */
	const vaddr_t prom_vstart = 0xf000000, prom_vend = 0xf0100000;
	const vaddr_t msgbufpv = (vaddr_t)msgbufp, v = (vaddr_t)ptr;
	const size_t msgbufsz = msgbufp->msg_bufs +
	    offsetof(struct kern_msgbuf, msg_bufc);

	*handled = (v >= msgbufpv && v < msgbufpv + msgbufsz) ||
	    (v >= prom_vstart && v < prom_vend && (prot & VM_PROT_WRITE) == 0);
	return 0;
}

int
mm_md_readwrite(dev_t dev, struct uio *uio)
{

	return ENXIO;
}
