/*	$NetBSD: machdep.c,v 1.168.4.1 2007/07/11 20:01:13 mjf Exp $	*/

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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

/*-
 * Copyright (c) 1996 Matthias Pfaller.
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1993 Philip A. Nelson.
 * Copyright (c) 1992 Terrence R. Lambert.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.168.4.1 2007/07/11 20:01:13 mjf Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_compat_netbsd.h"
#include "opt_ns381.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/syscallargs.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/ucontext.h>
#include <sys/ksyms.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/fpu.h>
#include <machine/pmap.h>
#include <machine/icu.h>
#include <machine/kcore.h>

#include <net/netisr.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include "ksyms.h"
/*
 * Support for VERY low debugging ... in case we get NO output.
 * e.g. in case pmap does not work and can't do regular mapped
 * output. In this case use umprintf to display debug messages.
 */
#if VERYLOWDEBUG
#include "umprintf.c"

/* Inform scncnputc the state of mapping. */
int _mapped = 0;
#endif

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
char	cpu_model[] = "ns32532";

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

struct user *proc0paddr;

int	maxphysmem = 0;
int	physmem;
int	boothowto;

vaddr_t msgbuf_vaddr;
paddr_t msgbuf_paddr;

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

extern	char etext[], end[];
#if NKSYMS || defined(DDB) || defined(LKM)
extern char *esym;
#endif
extern	paddr_t avail_start, avail_end;
extern	int nkpde;
extern	int ieee_handler_disable;

static paddr_t	alloc_pages(int);
static int	cpu_dump(void);
static int	cpu_dumpsize(void);
static void	cpu_reset(void);
static void	dumpsys(void);
void		init532(void);
static void	map(pd_entry_t *, vaddr_t, paddr_t, int, int);

/*
 * Machine-dependent startup code
 */
void
cpu_startup(void)
{
	char pbuf[9];
	vaddr_t minaddr, maxaddr;
	int i;

	/*
	 * Initialize error message buffer (at end of core).
	 */
	msgbuf_vaddr =  uvm_km_alloc(kernel_map, ns532_round_page(MSGBUFSIZE),
	    0, UVM_KMF_VAONLY);
	if (msgbuf_vaddr == 0)
		panic("failed to valloc msgbuf_vaddr");

	/* msgbuf_paddr was init'd in pmap */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_kenter_pa(msgbuf_vaddr + i * PAGE_SIZE,
		    msgbuf_paddr + i * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE);
	pmap_update(pmap_kernel());

	initmsgbuf((void *)msgbuf_vaddr, round_page(MSGBUFSIZE));

	printf("%s%s", copyright, version);
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
	    nmbclusters * mclbytes, VM_MAP_INTRSAFE, false, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
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
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "nkpde", NULL,
		       NULL, 0, &nkpde, 0,
		       CTL_MACHDEP, CPU_NKPDE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ieee_disable", NULL,
		       NULL, 0, &ieee_handler_disable, 0,
		       CTL_MACHDEP, CPU_IEEE_DISABLE, CTL_EOL);
}

void *
getframe(struct lwp *l, int sig, int *onstack)
{
	struct proc *p = l->l_proc;
	stack_t *ss = &p->p_sigctx.ps_sigstk;

	/* Do we need to jump onto the signal stack? */
	*onstack = (ss->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (*onstack)
		return (void *)ss->ss_sp + ss->ss_size;
	else
		return (void *)l->l_md.md_regs->r_sp;
}

struct sigframe_siginfo {
	int sf_ra;
	int sf_sig;
	siginfo_t *sf_sip;
	ucontext_t *sf_ucp;
	siginfo_t sf_si;
	ucontext_t sf_uc;
};

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
static void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	int sig = ksi->ksi_signo;
	int onstack;
	struct sigframe_siginfo frame, *fp = getframe(l, sig, &onstack);
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct reg *regs = l->l_md.md_regs;

	fp--;

	/* Build stack frame for signal trampoline. */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* handled by sendsig_sigcontext */
	case 1:		/* handled by sendsig_sigcontext */
	default:	/* unknown version */
		printf("sendsig_siginfo: bad version %d\n",
		    ps->sa_sigdesc[sig].sd_vers);
		sigexit(l, SIGILL);
	case 2:
		frame.sf_ra = (int)ps->sa_sigdesc[sig].sd_tramp;
		break;
	}

	frame.sf_sig = sig;
	frame.sf_sip = &fp->sf_si;
	frame.sf_ucp = &fp->sf_uc;
	frame.sf_si._info = ksi->ksi_info;
	frame.sf_uc.uc_flags = _UC_SIGMASK
	    | (p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK)
	    ? _UC_SETSTACK : _UC_CLRSTACK;
	frame.sf_uc.uc_sigmask = *mask;
	frame.sf_uc.uc_link = l->l_ctxlink;
	(void)memset(&frame.sf_uc.uc_stack, 0, sizeof(frame.sf_uc.uc_stack));
	cpu_getmcontext(l, &frame.sf_uc.uc_mcontext, &frame.sf_uc.uc_flags);

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.  We invoke the handler
	 * directly, only returning via the trampoline.  Note the
	 * trampoline version numbers are coordinated with machine-
	 * dependent code in libc.
	 */
	regs->r_sp = (int)fp;
	regs->r_pc = (int)catcher;

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

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	struct reg *regs = l->l_md.md_regs;

	(void)memcpy(mcp->__gregs, regs, sizeof (mcp->__gregs));
	mcp->__gregs[_REG_PS] = regs->r_psr;
	*flags |= _UC_CPU;

#ifdef NS381
	{
		extern struct lwp *fpu_lwp;

		/* If we're the FPU owner, dump its state to the PCB first. */
		if (fpu_lwp == l)
			save_fpu_context(&l->l_addr->u_pcb);

		mcp->__fpregs.__fpr_psr = l->l_addr->u_pcb.pcb_fsr;
		(void)memcpy(mcp->__fpregs.__fpr_regs,
		    l->l_addr->u_pcb.pcb_freg,
		    sizeof (mcp->__fpregs.__fpr_regs));
		*flags |= _UC_FPU;
	}
#endif
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct reg *regs = l->l_md.md_regs;

	/* Restore CPU context, if any. */
	if (flags & _UC_CPU) {
		/* Check for security violations. */
		if (((mcp->__gregs[_REG_PS] ^ regs->r_psr) & PSL_USERSTATIC)
		    != 0)
			return (EINVAL);
		(void)memcpy(regs, mcp->__gregs, sizeof (*regs));
		regs->r_mod = 0;
		regs->r_psr = mcp->__gregs[_REG_PS];
	}

#ifdef NS381
	/* Restore FPU context, if any. */
	if (flags & _UC_FPU) {
		extern struct lwp *fpu_lwp;

		l->l_addr->u_pcb.pcb_fsr = mcp->__fpregs.__fpr_psr;
		(void)memcpy(l->l_addr->u_pcb.pcb_freg,
		    mcp->__fpregs.__fpr_regs,
		    sizeof (l->l_addr->u_pcb.pcb_freg));
		/* If we're the FPU owner, force a reload. */
		if (fpu_lwp == l)
			restore_fpu_context(&l->l_addr->u_pcb);
	}
#endif
	if (flags & _UC_SETSTACK)
		l->l_proc->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_proc->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	return (0);
}

int waittime = -1;
static struct switchframe dump_sf;

void
cpu_reboot(int howto, char *bootstr)
{
	int s;

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* If "always halt" was specified as a boot flag, obey. */
	if ((boothowto & RB_HALT) != 0)
		howto |= RB_HALT;

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
	s = splhigh();

	/* If rebooting and a dump is requested do it. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP) {
		int *fp;
#if !defined(DDB) && defined(STACK_DUMP)
		/* dump the stack! */
		{
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
		if (curpcb) {
			curpcb->pcb_ksp = (long) &dump_sf;
			sprd(fp,  curpcb->pcb_kfp);
			smr(ptb0, curpcb->pcb_ptb);
			sprd(fp, fp);
			dump_sf.sf_fp = fp[0];
			dump_sf.sf_pc = fp[1];
			dump_sf.sf_pl = s;
			dump_sf.sf_lwp = curlwp;
		}
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
uint32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * cpu_dumpsize: calculate size of machine-dependent kernel core dump headers.
 */
static int
cpu_dumpsize(void)
{
	int size;

	size = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t));
	if (roundup(size, dbtob(1)) != dbtob(1))
		return -1;

	return (1);
}

/*
 * cpu_dump: dump machine-dependent kernel core dump headers.
 */
static int
cpu_dump(void)
{
	const struct bdevsw *bdev;
	int (*dump)(dev_t, daddr_t, void *, size_t);
	long buf[dbtob(1) / sizeof (long)];
	kcore_seg_t	*segp;
	cpu_kcore_hdr_t	*cpuhdrp;

	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return (ENXIO);

	dump = bdev->d_dump;

	segp = (kcore_seg_t *)buf;
	cpuhdrp =
	    (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(*segp)) / sizeof (long)];

	/*
	 * Generate a segment header.
	 */
	CORE_SETMAGIC(*segp, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	segp->c_size = dbtob(1) - ALIGN(sizeof(*segp));

	/*
	 * Add the machine-dependent header info
	 */
	cpuhdrp->ptd = proc0paddr->u_pcb.pcb_ptb;
	cpuhdrp->core_seg.start = 0;
	cpuhdrp->core_seg.size = ctob(physmem);

	return (dump(dumpdev, dumplo, (void *)buf, dbtob(1)));
}

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
	int nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL) {
		dumpdev = NODEV;
		return;
	}
	if (bdev->d_psize == NULL)
		goto bad;
	nblks = (*bdev->d_psize)(dumpdev);
	if (nblks <= ctod(1))
		goto bad;

	dumpblks = cpu_dumpsize();
	if (dumpblks < 0)
		goto bad;
	dumpblks += ctod(physmem);

	/* If dump won't fit (incl. room for possible label), punt. */
	if (dumpblks > (nblks - ctod(1)))
		goto bad;

	/* Put dump at end of partition */
	dumplo = nblks - dumpblks;

	/* dumpsize is in page units, and doesn't include headers. */
	dumpsize = physmem;
	return;

bad:
	dumpsize = 0;
	return;
}

/*
 * Dump the kernel's image to the swap partition.
 */
#define BYTES_PER_DUMP  PAGE_SIZE /* must be a multiple of pagesize XXX small */
static vaddr_t dumpspace;

vaddr_t
reserve_dumppages(vaddr_t p)
{

	dumpspace = p;
	return (p + BYTES_PER_DUMP);
}

void
dumpsys(void)
{
	const struct bdevsw *bdev;
	unsigned bytes, i, n;
	int maddr, psize;
	daddr_t blkno;
	int (*dump)(dev_t, daddr_t, void *, size_t);
	int error;

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

	/* XXX should purge all outstanding keystrokes. */

	if ((error = cpu_dump()) != 0)
		goto err;

	bytes = ctob(physmem);
	maddr = 0;
	blkno = dumplo + cpu_dumpsize();
	dump = bdev->d_dump;
	error = 0;
	for (i = 0; i < bytes; i += n) {
		/* Print out how many MBs we to go. */
		n = bytes - i;
		if (n && (n % (1024*1024)) == 0)
			printf("%d ", n / (1024 * 1024));

		/* Limit size for next transfer. */
		if (n > BYTES_PER_DUMP)
			n =  BYTES_PER_DUMP;

		(void) pmap_map(dumpspace, maddr, maddr + n, VM_PROT_READ);
		error = (*dump)(dumpdev, blkno, (void *)dumpspace, n);
		if (error)
			break;
		maddr += n;
		blkno += btodb(n);			/* XXX? */

		/* XXX should look for keystrokes, to cancel. */
	}

err:
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

/*
 * Clear registers on exec
 */
void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct proc *p = l->l_proc;
	struct reg *r = l->l_md.md_regs;
	struct pcb *pcbp = &l->l_addr->u_pcb;
	extern struct lwp *fpu_lwp;

	if (l == fpu_lwp)
		fpu_lwp = 0;

	memset(r, 0, sizeof(*r));
	r->r_sp  = stack;
	r->r_pc  = pack->ep_entry;
	r->r_psr = PSL_USERSET;
	r->r_r7  = (int)p->p_psstr;

	pcbp->pcb_fsr = FPC_UEN;
	memset(pcbp->pcb_freg, 0, sizeof(pcbp->pcb_freg));
}

/*
 * Allocate memory pages.
 */
static paddr_t
alloc_pages(int pages)
{
	paddr_t p = avail_start;
	avail_start += pages * PAGE_SIZE;
	memset((void *) p, 0, pages * PAGE_SIZE);
	return(p);
}

/*
 * Map physical to virtual addresses in the kernel page table directory.
 * If -1 is passed as physical address, empty second level page tables
 * are allocated for the selected virtual address range.
 */
static void
map(pd_entry_t *pd, vaddr_t virtual, paddr_t physical, int protection, int size)
{
	u_int ix1 = pdei(virtual);
	u_int ix2 = ptei(virtual);
	pt_entry_t *pt = (pt_entry_t *) (pd[ix1] & PG_FRAME);

	while (size > 0) {
		if (pt == 0) {
			pt = (pt_entry_t *) alloc_pages(1);
			pd[ix1] = (pd_entry_t) pt | PG_V | PG_KW;
		}
		if (physical != (paddr_t) -1) {
			pt[ix2] = (pt_entry_t) (physical | protection | PG_V);
			physical += PAGE_SIZE;
			size -= PAGE_SIZE;
		} else {
			size -= (PTES_PER_PTP - ix2) * PAGE_SIZE;
			ix2 = PTES_PER_PTP - 1;
		}
		if (++ix2 == PTES_PER_PTP) {
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
 *	0xdfc00000-0xdfffefff:	Kernel level two page tables
 *	0xdffff000-0xf7ffffff:	Kernel level one page table
 *	0xe0000000-0xff7fffff:	Kernel code and data
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
 * The CPU config register gets set.
 *
 * avail_start, avail_end, physmem and proc0paddr are set
 * to the correct values.
 *
 * The last action is to switch stacks and call main.
 */

#define	VA(x)	((vaddr_t)(x))
#define PA(x)	((paddr_t)(x))
#define kppa(x)	PA(ns532_round_page(x) & 0xffffff)
#define kvpa(x) VA(ns532_round_page(x))

void
init532(void)
{
	extern void main(void *);
	extern int inttab[];
#if NKSYMS || defined(DDB) || defined(LKM)
	extern char *esym;
#endif
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
#if NKSYMS || defined(DDB) || defined(LKM)
	if (esym) {
		avail_start = kppa(esym);
		esym += KERNBASE;
	} else
#endif
		avail_start = kppa(end);

	avail_end = ram_size((void *)avail_start);
	if (maxphysmem != 0 && avail_end > maxphysmem)
		avail_end = maxphysmem;
	physmem     = btoc(avail_end);
	nkpde = max(min(nkpde, NKPTP_MAX), NKPTP_MIN);

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

	/* Recursively map in the page directory */
	pd[PDSLOT_PTE] = (pd_entry_t)pd | PG_V | PG_KW;

	/* Map interrupt stack. */
	map(pd, VA(0xffc00000), alloc_pages(1), PG_KW, 0x001000);

	/* Map Duarts and Parity. */
	map(pd, VA(0xffc80000), PA(0x28000000), PG_KW | PG_N, 0x001000);

	/* Map SCSI Polled (Reduced space). */
	map(pd, VA(0xffd00000), PA(0x30000000), PG_KW | PG_N, 0x100000);

	/* Map SCSI DMA (Reduced space). */
	map(pd, VA(0xffe00000), PA(0x38000000), PG_KW | PG_N, 0x0ff000);

	/* Map SCSI DMA (With A22 "EOP"). */
	map(pd, VA(0xffeff000), PA(0x38400000), PG_KW | PG_N, 0x001000);

	/* Map EPROM (for realtime clock). */
	map(pd, VA(0xfff00000), PA(0x10000000), PG_KW | PG_N, 0x040000);

	/* Map the ICU. */
	map(pd, VA(0xfffff000), PA(0xfffff000), PG_KW | PG_N, 0x001000);

	/* Map UAREA for proc0. */
	proc0paddr = (struct user *)alloc_pages(UPAGES);
	proc0paddr->u_pcb.pcb_ptb = (int) pd;
	proc0paddr = (struct user *) ((vaddr_t)proc0paddr + KERNBASE);
	lwp0.l_addr = proc0paddr;

	/* Allocate second level page tables for kernel virtual address space */
	map(pd, VM_MIN_KERNEL_ADDRESS, PA(-1), 0, nkpde << PDSHIFT);
	/* Map monitor scratch area R/W. */
	map(pd, KERNBASE,        PA(0x00000000), PG_KW, 0x2000);
	/* Map kernel text R/O. */
	map(pd, KERNBASE+0x2000, PA(0x00002000), PG_KR, kppa(etext) - 0x2000);
	/* Map kernel data+bss R/W. */
	map(pd, kvpa(etext), kppa(etext), PG_KW, avail_start - kppa(etext));

	/* Alias the mapping at KERNBASE to 0 */
	pd[pdei(0)] = pd[pdei(KERNBASE)];

#if VERYLOWDEBUG
	umprintf ("enabling mapping\n");
#endif

	/* Load the ptb registers and start mapping. */
	load_ptb(pd);
	lmr(mcr, 3);

#if VERYLOWDEBUG
	/* Let scncnputc know which form to use. */
	_mapped = 1;
	umprintf ("done\n");
#endif

#if VERYLOWDEBUG
	umprintf ("Just before jump to high memory.\n");
#endif

	/* Jump to high memory */
	__asm volatile("jump @1f; 1:");

	/* Initialize the pmap module. */
	pmap_bootstrap(avail_start + KERNBASE);

	/* Construct an empty syscframe for proc0. */
	curpcb = &lwp0.l_addr->u_pcb;
	curpcb->pcb_onstack = (struct reg *)
			      ((u_int)lwp0.l_addr + USPACE) - 1;

	/* Switch to lwp0's stack. */
	lprd(sp, curpcb->pcb_onstack);
	lprd(fp, 0);

	main(curpcb->pcb_onstack);
	panic("main returned to init532");
}

/*
 * cpu_exec_aout_makecmds():
 *	CPU-dependent a.out format hook for execve().
 *
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 * On the ns532 there are no binary compatibility options (yet),
 * Any takers for Sinix, Genix, SVR2/32000 or Minix?
 */
int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{

	return ENOEXEC;
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{
	cninit();

#ifdef KGDB
	if (boothowto & RB_KDB) {
		extern int kgdb_debug_init;
		kgdb_debug_init = 1;
	}
#endif
#if NKSYMS || defined(DDB) || defined(LKM)
	ksyms_init(*(int *)end, ((int *)end) + 1, (int *)esym);
#endif
#if defined (DDB)
	if(boothowto & RB_KDB)
		Debugger();
#endif
}

static void
cpu_reset(void)
{

	/* Mask all ICU interrupts. */
	splhigh();

	/* Disable CPU interrupts. */
	di();

	/* Alias kernel memory at 0. */
	PDP_BASE[0] = PDP_BASE[pdei(KERNBASE)];
	tlbflush();

	/* Jump to low memory. */
	__asm volatile(
		"addr	1f(pc),r0;"
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
	memcpy((void *)0, (void *)0x10000000, 0x1f00);

	/* Jump into ROM copy. */
	__asm volatile("jump @0");
}

/*
 * Network software interrupt routine
 */
void
softnet(void *arg)
{
	int isr;

	di(); isr = netisr; netisr = 0; ei();
	if (isr == 0) return;

#define DONETISR(bit, fn)		\
do {					\
	if (isr & (1 << bit))		\
		fn();			\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}
