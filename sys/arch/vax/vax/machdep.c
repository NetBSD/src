/* $NetBSD: machdep.c,v 1.139 2003/09/26 12:02:57 simonb Exp $	 */

/*
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 * 
 * Changed for the VAX port (and for readability) /IC
 * 
 * This code is derived from software contributed to Berkeley by the Systems
 * Programming Group of the University of Utah Computer Science Department.
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
 * from: Utah Hdr: machdep.c 1.63 91/04/24
 * 
 * @(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

/*
 * Copyright (c) 2002, Hugh Graham.
 * Copyright (c) 1994, 1998 Ludd, University of Lule}, Sweden.
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 * 
 * Changed for the VAX port (and for readability) /IC
 * 
 * This code is derived from software contributed to Berkeley by the Systems
 * Programming Group of the University of Utah Computer Science Department.
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
 * from: Utah Hdr: machdep.c 1.63 91/04/24
 * 
 * @(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.139 2003/09/26 12:02:57 simonb Exp $");

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ultrix.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_compat_ibcs2.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/kernel.h>
#include <sys/msgbuf.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/ptrace.h>
#include <sys/savar.h>
#include <sys/ksyms.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#include <machine/sid.h>
#include <machine/pte.h>
#include <machine/mtpr.h>
#include <machine/cpu.h>
#include <machine/macros.h>
#include <machine/nexus.h>
#include <machine/trap.h>
#include <machine/reg.h>
#include <machine/db_machdep.h>
#include <machine/scb.h>
#include <vax/vax/gencons.h>

#ifdef DDB
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include "smg.h"
#include "ksyms.h"

extern vaddr_t virtual_avail, virtual_end;
/*
 * We do these external declarations here, maybe they should be done
 * somewhere else...
 */
char		machine[] = MACHINE;		/* from <machine/param.h> */
char		machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
char		cpu_model[100];
caddr_t		msgbufaddr;
int		physmem;
int		*symtab_start;
int		*symtab_end;
int		symtab_nsyms;
struct cpmbx	*cpmbx;		/* Console program mailbox address */

/*
 * Extent map to manage I/O register space.  We allocate storage for
 * 32 regions in the map.  iomap_ex_malloc_safe will indicate that it's
 * safe to use malloc() to dynamically allocate region descriptors in
 * case we run out.
 */
static long iomap_ex_storage[EXTENT_FIXED_STORAGE_SIZE(32) / sizeof(long)];
static struct extent *iomap_ex;
static int iomap_ex_malloc_safe;

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

#ifdef DEBUG
int iospace_inited = 0;
#endif

struct softintr_head softclock_head = { IPL_SOFTCLOCK };
struct softintr_head softnet_head = { IPL_SOFTNET };
struct softintr_head softserial_head = { IPL_SOFTSERIAL };

void
cpu_startup()
{
	caddr_t		v;
	u_int		base, residual, i, sz;
	vaddr_t		minaddr, maxaddr;
	vsize_t		size;
	extern paddr_t avail_end;
	char pbuf[9];

	/*
	 * Initialize error message buffer.
	 */
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));

	/*
	 * Good {morning,afternoon,evening,night}.
	 * Also call CPU init on systems that need that.
	 */
	printf("%s\n%s\n", version, cpu_model);
        if (dep_call->cpu_conf)
                (*dep_call->cpu_conf)();

	format_bytes(pbuf, sizeof(pbuf), avail_end);
	printf("total memory = %s\n", pbuf);
	panicstr = NULL;
	mtpr(AST_NO, PR_ASTLVL);
	spl0();

	/*
	 * Find out how much space we need, allocate it, and then give
	 * everything true virtual addresses.
	 */

	sz = (u_int) allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
	if (allocsys(v, NULL) - v != sz)
		panic("startup: table size inconsistency");
	/*
	 * Now allocate buffers proper.	 They are different than the above in
	 * that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;		/* # bytes for buffers */

	/* allocate VM for buffers... area is not managed by VM system */
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
		panic("cpu_startup: cannot allocate VM for buffers");

	minaddr = (vaddr_t) buffers;
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	/* now allocate RAM for buffers */
	for (i = 0 ; i < nbuf ; i++) {
		vaddr_t curbuf;
		vsize_t curbufsize;
		struct vm_page *pg;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.	The rest get (base) physical pages.
		 * 
		 * The rest of each buffer occupies virtual space, but has no
		 * physical memory allocated for it.
		 */
		curbuf = (vaddr_t) buffers + i * MAXBSIZE;
		curbufsize = PAGE_SIZE * (i < residual ? base + 1 : base);
		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: "
				    "not enough RAM for buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ | VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	pmap_update(kernel_map->pmap);

	/*
	 * Allocate a submap for exec arguments.  This map effectively limits
	 * the number of processes exec'ing at any time.
	 * At most one process with the full length is allowed.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

#if VAX46 || VAX48 || VAX49 || VAX53 || VAXANY
	/*
	 * Allocate a submap for physio.  This map effectively limits the
	 * number of processes doing physio at any one time.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, FALSE, NULL);
#endif

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * PAGE_SIZE);
	printf("using %u buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */

	bufinit();
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	iomap_ex_malloc_safe = 1;
}

u_int32_t dumpmag = 0x8fca0101;
int	dumpsize = 0;
long	dumplo = 0;

void
cpu_dumpconf()
{
	const struct bdevsw *bdev;
	int		nblks;

	/*
	 * XXX include the final RAM page which is not included in physmem.
	 */
	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return;
	dumpsize = physmem + 1;
	if (bdev->d_psize != NULL) {
		nblks = (*bdev->d_psize)(dumpdev);
		if (dumpsize > btoc(dbtob(nblks - dumplo)))
			dumpsize = btoc(dbtob(nblks - dumplo));
		else if (dumplo == 0)
			dumplo = nblks - btodb(ctob(dumpsize));
	}
	/*
	 * Don't dump on the first PAGE_SIZE (why PAGE_SIZE?) in case the dump
	 * device includes a disk label.
	 */
	if (dumplo < btodb(PAGE_SIZE))
		dumplo = btodb(PAGE_SIZE);
}

int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct	proc *p;
{
	dev_t consdev;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);

	switch (name[0]) {
	case CPU_PRINTFATALTRAPS:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &cpu_printfataltraps));
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof(consdev)));
	case CPU_BOOTED_DEVICE:
		if (booted_device != NULL)
			return (sysctl_rdstring(oldp, oldlenp, newp,
			    booted_device->dv_xname));
		break;
	case CPU_BOOTED_KERNEL:
		/*
		 * I don't think this is available to the kernel.
		 */
	default:
		break;
	}
	return (EOPNOTSUPP);
}

void
setstatclockrate(hzrate)
	int hzrate;
{
}

void
consinit()
{
	extern vaddr_t iospace;

	/*
	 * Init I/O memory extent map. Must be done before cninit()
	 * is called; we may want to use iospace in the console routines.
	 *
	 * NOTE: We need to reserve the first vax-page of iospace
	 * for the console routines.
	 */
	KASSERT(iospace != 0);
	iomap_ex = extent_create("iomap", iospace + VAX_NBPG,
	    iospace + ((IOSPSZ * VAX_NBPG) - 1), M_DEVBUF,
	    (caddr_t) iomap_ex_storage, sizeof(iomap_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
#ifdef DEBUG
	iospace_inited = 1;
#endif
	cninit();
#if NKSYMS || defined(DDB) || defined(LKM)
	if (symtab_start != NULL && symtab_nsyms != 0 && symtab_end != NULL) {
		ksyms_init(symtab_nsyms, symtab_start, symtab_end);
	}
#endif
#ifdef DEBUG
	if (sizeof(struct user) > REDZONEADDR)
		panic("struct user inside red zone");
#endif
}

#if defined(COMPAT_13) || defined(COMPAT_ULTRIX)
int
compat_13_sys_sigreturn(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_13_sys_sigreturn_args /* {
		syscallarg(struct sigcontext13 *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct trapframe *scf;
	struct sigcontext13 *ucntx;
	struct sigcontext13 ksc;
	sigset_t mask;

	scf = l->l_addr->u_pcb.framep;
	ucntx = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)ucntx, (caddr_t)&ksc, sizeof(struct sigcontext)))
		return EINVAL;

	/* Compatibility mode? */
	if ((ksc.sc_ps & (PSL_IPL | PSL_IS)) ||
	    ((ksc.sc_ps & (PSL_U | PSL_PREVU)) != (PSL_U | PSL_PREVU)) ||
	    (ksc.sc_ps & PSL_CM)) {
		return (EINVAL);
	}
	if (ksc.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	native_sigset13_to_sigset(&ksc.sc_mask, &mask);
	(void) sigprocmask1(p, SIG_SETMASK, &mask, 0);

	scf->fp = ksc.sc_fp;
	scf->ap = ksc.sc_ap;
	scf->pc = ksc.sc_pc;
	scf->sp = ksc.sc_sp;
	scf->psl = ksc.sc_ps;
	return (EJUSTRETURN);
}
#endif

int
sys___sigreturn14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct trapframe *scf;
	struct sigcontext *ucntx;
	struct sigcontext ksc;

	scf = l->l_addr->u_pcb.framep;
	ucntx = SCARG(uap, sigcntxp);

	if (copyin((caddr_t)ucntx, (caddr_t)&ksc, sizeof(struct sigcontext)))
		return EINVAL;
	/* Compatibility mode? */
	if ((ksc.sc_ps & (PSL_IPL | PSL_IS)) ||
	    ((ksc.sc_ps & (PSL_U | PSL_PREVU)) != (PSL_U | PSL_PREVU)) ||
	    (ksc.sc_ps & PSL_CM)) {
		return (EINVAL);
	}
	if (ksc.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &ksc.sc_mask, 0);

	scf->fp = ksc.sc_fp;
	scf->ap = ksc.sc_ap;
	scf->pc = ksc.sc_pc;
	scf->sp = ksc.sc_sp;
	scf->psl = ksc.sc_ps;
	return (EJUSTRETURN);
}

#if defined(COMPAT_16) || defined(COMPAT_ULTRIX) || defined(COMPAT_IBCS2)

struct otrampframe {
	unsigned	sig;	/* Signal number */
	unsigned	code;	/* Info code */
	unsigned	scp;	/* Pointer to struct sigcontext */
	unsigned	r0, r1, r2, r3, r4, r5; /* Registers saved when
						 * interrupt */
	unsigned	pc;	/* Address of signal handler */
	unsigned	arg;	/* Pointer to first (and only) sigreturn
				 * argument */
};

static void
oldsendsig(int sig, const sigset_t *mask, u_long code)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct	sigacts *ps = p->p_sigacts;
	struct	trapframe *syscf;
	struct	sigcontext *sigctx, gsigctx;
	struct	otrampframe *trampf, gtrampf;
	unsigned	cursp;
	int	onstack;
	sig_t	catcher = SIGACTION(p, sig).sa_handler;

	syscf = l->l_addr->u_pcb.framep;

	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		cursp = ((int)p->p_sigctx.ps_sigstk.ss_sp + p->p_sigctx.ps_sigstk.ss_size);
	else
		cursp = syscf->sp;

	/* Set up positions for structs on stack */
	sigctx = (struct sigcontext *) (cursp - sizeof(struct sigcontext));
	trampf = (struct otrampframe *) ((unsigned)sigctx -
	    sizeof(struct otrampframe));

	 /* Place for pointer to arg list in sigreturn */
	cursp = (unsigned)sigctx - 8;

	gtrampf.arg = (int) sigctx;
	gtrampf.pc = (unsigned) catcher;
	/* r0..r5 are saved by the popr in the sigcode snippet */
	gtrampf.scp = (int) sigctx;
	gtrampf.code = code;
	gtrampf.sig = sig;

	gsigctx.sc_pc = syscf->pc;
	gsigctx.sc_ps = syscf->psl;
	gsigctx.sc_ap = syscf->ap;
	gsigctx.sc_fp = syscf->fp; 
	gsigctx.sc_sp = syscf->sp; 
	gsigctx.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;
	gsigctx.sc_mask = *mask;

#if defined(COMPAT_13) || defined(COMPAT_ULTRIX)
	native_sigset_to_sigset13(mask, &gsigctx.__sc_mask13);
#endif

	if (copyout(&gtrampf, trampf, sizeof(gtrampf)) ||
	    copyout(&gsigctx, sigctx, sizeof(gsigctx)))
		sigexit(l, SIGILL);

	/*
	 * Note the trampoline version numbers are coordinated with
	 * machine-dependent code in libc.
	 */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:
		syscf->pc = (int)p->p_sigctx.ps_sigcode;
		break;
	case 1:
		syscf->pc = (int)ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		/* ``cannot happen'' */
		sigexit(l, SIGILL);
	}
	syscf->psl = PSL_U | PSL_PREVU;
	syscf->ap = cursp;
	syscf->sp = cursp;

	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

#endif

struct trampoline {
	unsigned int narg;	/* Argument count (== 3) */
	unsigned int sig;	/* Signal number */
	unsigned int code;	/* Info code */
	unsigned int scp;	/* Pointer to struct sigcontext */
};

/*
 * Brief description of how sendsig() works:
 * A struct sigcontext is allocated on the user stack. The relevant
 * registers are saved in it. Below it is a struct trampframe constructed, it
 * is actually an argument list for callg. The user
 * stack pointer is put below all structs.
 *
 * The registers will contain when the signal handler is called:
 * pc, psl	- Obvious
 * sp		- An address below all structs
 * fp 		- The address of the signal handler
 * ap		- The address to the callg frame
 *
 * The trampoline code will save r0-r5 before doing anything else.
 */
void
sendsig(int sig, const sigset_t *mask, u_long code)
{
	struct lwp *l = curlwp;
	struct proc *p = curproc;
	struct sigacts *ps = p->p_sigacts;
	struct trampoline tramp;
	struct sigcontext sigctx;
	struct trapframe *tf = l->l_addr->u_pcb.framep;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	int onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;
	int cursp = onstack == 0 ? tf->sp :
	    ((int)p->p_sigctx.ps_sigstk.ss_sp + p->p_sigctx.ps_sigstk.ss_size);

#if defined(COMPAT_16) || defined(COMPAT_ULTRIX) || defined(COMPAT_IBCS2)
	if (ps->sa_sigdesc[sig].sd_vers < 2)
		return oldsendsig(sig, mask, code);
#endif

	/*
	 * The sigcontext struct will be passed back to sigreturn().
	 */
	sigctx.sc_pc = tf->pc;
	sigctx.sc_ps = tf->psl;
	sigctx.sc_ap = tf->ap;
	sigctx.sc_fp = tf->fp;
	sigctx.sc_sp = tf->sp;
	sigctx.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;
	sigctx.sc_mask = *mask;

	/*
	 * Arguments given to the signal handler.
	 */
	tramp.narg = 3;
	tramp.sig = sig;
	tramp.code = code;
	tramp.scp = cursp - sizeof(struct sigcontext);

	if (copyout(&sigctx, (char *)tramp.scp, sizeof(struct sigcontext)) ||
	    copyout(&tramp, (char *)tramp.scp - sizeof(tramp), sizeof(tramp)))
		sigexit(l, SIGILL);

	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 2:
		tf->pc = (int)ps->sa_sigdesc[sig].sd_tramp;
		break;
	default:
		/* Don't know what trampoline version; kill it. */
		sigexit(l, SIGILL);
	}
	tf->psl = PSL_U | PSL_PREVU;
	tf->ap = tramp.scp - sizeof(tramp);
	tf->fp = (int)catcher;
	tf->sp = tf->ap;

	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

int	waittime = -1;
static	volatile int showto; /* Must be volatile to survive MM on -> MM off */

void
cpu_reboot(howto, b)
	register int howto;
	char *b;
{
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr will be out of
		 * synch; adjust it now.
		 */
		resettodr();
	}
	splhigh();		/* extreme priority */
	if (howto & RB_HALT) {
		doshutdownhooks();
		if (dep_call->cpu_halt)
			(*dep_call->cpu_halt) ();
		printf("halting (in tight loop); hit\n\t^P\n\tHALT\n\n");
		for (;;)
			;
	} else {
		showto = howto;
#ifdef notyet
		/*
		 * If we are provided with a bootstring, parse it and send
		 * it to the boot program.
		 */
		if (b)
			while (*b) {
				showto |= (*b == 'a' ? RB_ASKBOOT : (*b == 'd' ?
				    RB_DEBUG : (*b == 's' ? RB_SINGLE : 0)));
				b++;
			}
#endif
		/*
		 * Now it's time to:
		 *  0. Save some registers that are needed in new world.
		 *  1. Change stack to somewhere that will survive MM off.
		 * (RPB page is good page to save things in).
		 *  2. Actually turn MM off.
		 *  3. Dump away memory to disk, if asked.
		 *  4. Reboot as asked.
		 * The RPB page is _always_ first page in memory, we can
		 * rely on that.
		 */
#ifdef notyet
		asm("	movl	%sp, (0x80000200)
			movl	0x80000200, %sp
			mfpr	$0x10, -(%sp)	# PR_PCBB
			mfpr	$0x11, -(%sp)	# PR_SCBB
			mfpr	$0xc, -(%sp)	# PR_SBR
			mfpr	$0xd, -(%sp)	# PR_SLR
			mtpr	$0, $0x38	# PR_MAPEN
		");
#endif

		if (showto & RB_DUMP)
			dumpsys();
		if (dep_call->cpu_reboot)
			(*dep_call->cpu_reboot)(showto);

		/* cpus that don't handle reboots get the standard reboot. */
		while ((mfpr(PR_TXCS) & GC_RDY) == 0)
			;

		mtpr(GC_CONS|GC_BTFL, PR_TXDB);
	}
	asm("movl %0, %%r5":: "g" (showto)); /* How to boot */
	asm("movl %0, %%r11":: "r"(showto)); /* ??? */
	asm("halt");
	panic("Halt sket sej");
}

void
dumpsys()
{
	const struct bdevsw *bdev;

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return;
	/*
	 * For dumps during autoconfiguration, if dump device has already
	 * configured...
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
	printf("dump ");
	switch ((*bdev->d_dump) (dumpdev, 0, 0, 0)) {

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
}

int
process_read_regs(l, regs)
	struct lwp    *l;
	struct reg     *regs;
{
	struct trapframe *tf = l->l_addr->u_pcb.framep;

	bcopy(&tf->r0, &regs->r0, 12 * sizeof(int));
	regs->ap = tf->ap;
	regs->fp = tf->fp;
	regs->sp = tf->sp;
	regs->pc = tf->pc;
	regs->psl = tf->psl;
	return 0;
}

int
process_write_regs(l, regs)
	struct lwp    *l;
	struct reg     *regs;
{
	struct trapframe *tf = l->l_addr->u_pcb.framep;

	bcopy(&regs->r0, &tf->r0, 12 * sizeof(int));
	tf->ap = regs->ap;
	tf->fp = regs->fp;
	tf->sp = regs->sp;
	tf->pc = regs->pc;
	tf->psl = (regs->psl|PSL_U|PSL_PREVU) &
	    ~(PSL_MBZ|PSL_IS|PSL_IPL1F|PSL_CM); /* Allow compat mode? */
	return 0;
}

int
process_set_pc(l, addr)
	struct	lwp *l;
	caddr_t addr;
{
	struct	trapframe *tf;
	void	*ptr;

	if ((l->l_flag & L_INMEM) == 0)
		return (EIO);

	ptr = (char *) l->l_addr->u_pcb.framep;
	tf = ptr;

	tf->pc = (unsigned) addr;

	return (0);
}

int
process_sstep(l, sstep)
	struct lwp    *l;
{
	void	       *ptr;
	struct trapframe *tf;

	if ((l->l_flag & L_INMEM) == 0)
		return (EIO);

	ptr = l->l_addr->u_pcb.framep;
	tf = ptr;

	if (sstep)
		tf->psl |= PSL_T;
	else
		tf->psl &= ~PSL_T;

	return (0);
}

#undef PHYSMEMDEBUG
/*
 * Allocates a virtual range suitable for mapping in physical memory.
 * This differs from the bus_space routines in that it allocates on
 * physical page sizes instead of logical sizes. This implementation
 * uses resource maps when allocating space, which is allocated from 
 * the IOMAP submap. The implementation is similar to the uba resource
 * map handling. Size is given in pages.
 * If the page requested is bigger than a logical page, space is
 * allocated from the kernel map instead.
 *
 * It is known that the first page in the iospace area is unused; it may
 * be use by console device drivers (before the map system is inited).
 */
vaddr_t
vax_map_physmem(phys, size)
	paddr_t phys;
	int size;
{
	vaddr_t addr;
	int error;
	static int warned = 0;

#ifdef DEBUG
	if (!iospace_inited)
		panic("vax_map_physmem: called before rminit()?!?");
#endif
	if (size >= LTOHPN) {
		addr = uvm_km_valloc(kernel_map, size * VAX_NBPG);
		if (addr == 0)
			panic("vax_map_physmem: kernel map full");
	} else {
		error = extent_alloc(iomap_ex, size * VAX_NBPG, VAX_NBPG, 0,
		    EX_FAST | EX_NOWAIT |
		    (iomap_ex_malloc_safe ? EX_MALLOCOK : 0), &addr);
		if (error) {
			if (warned++ == 0) /* Warn only once */
				printf("vax_map_physmem: iomap too small");
			return 0;
		}
	}
	ioaccess(addr, phys, size);
#ifdef PHYSMEMDEBUG
	printf("vax_map_physmem: alloc'ed %d pages for paddr %lx, at %lx\n",
	    size, phys, addr);
#endif
	return addr | (phys & VAX_PGOFSET);
}

/*
 * Unmaps the previous mapped (addr, size) pair.
 */
void
vax_unmap_physmem(addr, size)
	vaddr_t addr;
	int size;
{
#ifdef PHYSMEMDEBUG
	printf("vax_unmap_physmem: unmapping %d pages at addr %lx\n", 
	    size, addr);
#endif
	addr &= ~VAX_PGOFSET;
	iounaccess(addr, size);
	if (size >= LTOHPN)
		uvm_km_free(kernel_map, addr, size * VAX_NBPG);
	else if (extent_free(iomap_ex, addr, size * VAX_NBPG,
			     EX_NOWAIT |
			     (iomap_ex_malloc_safe ? EX_MALLOCOK : 0)))
		printf("vax_unmap_physmem: addr 0x%lx size %dvpg: "
		    "can't free region\n", addr, size);
}

void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct softintr_handler *sh;
	struct softintr_head *shd;

	switch (ipl) {
	case IPL_SOFTCLOCK: shd = &softclock_head; break;
	case IPL_SOFTNET: shd = &softnet_head; break;
	case IPL_SOFTSERIAL: shd = &softserial_head; break;
	default: panic("softintr_establish: unsupported soft IPL");
	}

	sh = malloc(sizeof(*sh), M_SOFTINTR, M_NOWAIT);
	if (sh == NULL)
		return NULL;

	LIST_INSERT_HEAD(&shd->shd_intrs, sh, sh_link);
	sh->sh_head = shd;
	sh->sh_pending = 0;
	sh->sh_func = func;
	sh->sh_arg = arg;

	return sh;
}

void
softintr_disestablish(void *arg)
{
	struct softintr_handler *sh = arg;
	LIST_REMOVE(sh, sh_link);
	free(sh, M_SOFTINTR);
}

#include <dev/bi/bivar.h>
/*
 * This should be somewhere else.
 */
void
bi_intr_establish(void *icookie, int vec, void (*func)(void *), void *arg, 
	struct evcnt *ev) 
{  
	scb_vecalloc(vec, func, arg, SCB_ISTACK, ev);
}

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
/*
 * Called from locore.
 */
void	krnlock(void);
void	krnunlock(void);

void
krnlock()
{
	KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
}

void
krnunlock()
{
	KERNEL_UNLOCK();
}
#endif

/*
 * This is an argument list pushed onto the stack, and given to
 * a CALLG instruction in the trampoline.
 */
struct saframe {
	int	sa_type;
	void	*sa_sas;
	int	sa_events;
	int	sa_interrupted;
	void	*sa_ap;

	int	sa_argc;
};

void
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted,
    void *sas, void *ap, void *sp, sa_upcall_t upcall)
{
	struct proc *p = l->l_proc;
	struct trapframe *tf = l->l_addr->u_pcb.framep;
	struct saframe *sf, frame;
	extern char sigcode[], upcallcode[];

	frame.sa_type = type;
	frame.sa_sas = sas;
	frame.sa_events = nevents;
	frame.sa_interrupted = ninterrupted;
	frame.sa_ap = ap;
	frame.sa_argc = 5;

	sf = ((struct saframe *)sp) - 1;
	if (copyout(&frame, sf, sizeof(frame)) != 0) {
		/* Copying onto the stack didn't work, die. */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	tf->r0 = (long) upcall;
	tf->sp = (long) sf;
	tf->pc = (long) ((caddr_t)p->p_sigctx.ps_sigcode +
			 ((caddr_t)upcallcode - (caddr_t)sigcode));
	tf->psl = (long)PSL_U | PSL_PREVU;
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	const struct trapframe *tf = l->l_addr->u_pcb.framep;
	__greg_t *gr = mcp->__gregs;

	gr[_REG_R0] = tf->r0;
	gr[_REG_R1] = tf->r1;
	gr[_REG_R2] = tf->r2;
	gr[_REG_R3] = tf->r3;
	gr[_REG_R4] = tf->r4;
	gr[_REG_R5] = tf->r5;
	gr[_REG_R6] = tf->r6;
	gr[_REG_R7] = tf->r7;
	gr[_REG_R8] = tf->r8;
	gr[_REG_R9] = tf->r9;
	gr[_REG_R10] = tf->r10;
	gr[_REG_R11] = tf->r11;
	gr[_REG_AP] = tf->ap;
	gr[_REG_FP] = tf->fp;
	gr[_REG_SP] = tf->sp;
	gr[_REG_PC] = tf->pc;
	gr[_REG_PSL] = tf->psl;
	*flags |= _UC_CPU;
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe *tf = l->l_addr->u_pcb.framep;
	__greg_t *gr = mcp->__gregs;

	if ((flags & _UC_CPU) == 0)
		return 0;

	if ((gr[_REG_PSL] & (PSL_IPL | PSL_IS)) ||
	    ((gr[_REG_PSL] & (PSL_U | PSL_PREVU)) != (PSL_U | PSL_PREVU)) ||
	    (gr[_REG_PSL] & PSL_CM))
		return (EINVAL);

	tf->r0 = gr[_REG_R0];
	tf->r1 = gr[_REG_R1];
	tf->r2 = gr[_REG_R2];
	tf->r3 = gr[_REG_R3];
	tf->r4 = gr[_REG_R4];
	tf->r5 = gr[_REG_R5];
	tf->r6 = gr[_REG_R6];
	tf->r7 = gr[_REG_R7];
	tf->r8 = gr[_REG_R8];
	tf->r9 = gr[_REG_R9];
	tf->r10 = gr[_REG_R10];
	tf->r11 = gr[_REG_R11];
	tf->ap = gr[_REG_AP];
	tf->fp = gr[_REG_FP];
	tf->sp = gr[_REG_SP];
	tf->pc = gr[_REG_PC];
	tf->psl = gr[_REG_PSL];
	return 0;
}

/*
 * Generic routines for machines with "console program mailbox".
 */
void
generic_halt()
{
	if (cpmbx == NULL)  /* Too late to complain here, but avoid panic */
		asm("halt");

	if (cpmbx->user_halt != UHALT_DEFAULT) {
		if (cpmbx->mbox_halt != 0)
			cpmbx->mbox_halt = 0;   /* let console override */
	} else if (cpmbx->mbox_halt != MHALT_HALT)
		cpmbx->mbox_halt = MHALT_HALT;  /* the os decides */

	asm("halt");
}

void
generic_reboot(int arg)
{
	if (cpmbx == NULL)  /* Too late to complain here, but avoid panic */
		asm("halt");

	if (cpmbx->user_halt != UHALT_DEFAULT) {
		if (cpmbx->mbox_halt != 0)
			cpmbx->mbox_halt = 0;
	} else if (cpmbx->mbox_halt != MHALT_REBOOT)
		cpmbx->mbox_halt = MHALT_REBOOT;

	asm("halt");
}

