/* $NetBSD: machdep.c,v 1.121 2002/02/24 01:04:27 matt Exp $	 */

/*
 * Copyright (c) 1994, 1998 Ludd, University of Lule}, Sweden.
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
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

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ultrix.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
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
#include <sys/syscallargs.h>
#include <sys/ptrace.h>

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

extern int virtual_avail, virtual_end;
/*
 * We do these external declarations here, maybe they should be done
 * somewhere else...
 */
char		machine[] = MACHINE;		/* from <machine/param.h> */
char		machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
char		cpu_model[100];
caddr_t		msgbufaddr;
int		physmem;
int		dumpsize = 0;
int		*symtab_start;
int		*symtab_end;
int		symtab_nsyms;

#define	IOMAPSZ	100
static	struct map iomap[IOMAPSZ];

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
	int		base, residual, i, sz;
	vaddr_t		minaddr, maxaddr;
	vsize_t		size;
	extern unsigned int avail_end;
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

	sz = (int) allocsys(NULL, NULL);
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
		curbufsize = NBPG * (i < residual ? base + 1 : base);
		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: "
				    "not enough RAM for buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ | VM_PROT_WRITE);
			curbuf += NBPG;
			curbufsize -= NBPG;
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
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */

	bufinit();
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

long	dumplo = 0;
long	dumpmag = 0x8fca0101;

void
cpu_dumpconf()
{
	int		nblks;

	/*
	 * XXX include the final RAM page which is not included in physmem.
	 */
	dumpsize = physmem + 1;
	if (dumpdev != NODEV && bdevsw[major(dumpdev)].d_psize) {
		nblks = (*bdevsw[major(dumpdev)].d_psize) (dumpdev);
		if (dumpsize > btoc(dbtob(nblks - dumplo)))
			dumpsize = btoc(dbtob(nblks - dumplo));
		else if (dumplo == 0)
			dumplo = nblks - btodb(ctob(dumpsize));
	}
	/*
	 * Don't dump on the first NBPG (why NBPG?) in case the dump
	 * device includes a disk label.
	 */
	if (dumplo < btodb(NBPG))
		dumplo = btodb(NBPG);
}

int
cpu_sysctl(a, b, c, d, e, f, g)
	int	*a;
	u_int	b;
	void	*c, *e;
	size_t	*d, f;
	struct	proc *g;
{
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
	/*
	 * Init I/O memory resource map. Must be done before cninit()
	 * is called; we may want to use iospace in the console routines.
	 */
	rminit(iomap, IOSPSZ, (long)1, "iomap", IOMAPSZ);
#ifdef DEBUG
	iospace_inited = 1;
#endif
	cninit();
#if defined(DDB) && !defined(__ELF__)
	{
#ifndef __ELF__
		extern int end; /* Contains pointer to symsize also */
#endif
		if (symtab_start != NULL)
			ddb_init(symtab_nsyms, symtab_start, symtab_end);
#ifndef __ELF__
		else {
			extern paddr_t esym;
			ddb_init(*(int *)&end, ((int *)&end) + 1, (void *)esym);
		}
#endif
	}
#ifdef DEBUG
	if (sizeof(struct user) > REDZONEADDR)
		panic("struct user inside red zone");
#endif
#endif
}

#if defined(COMPAT_13) || defined(COMPAT_ULTRIX)
int
compat_13_sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_13_sys_sigreturn_args /* {
		syscallarg(struct sigcontext13 *) sigcntxp;
	} */ *uap = v;
	struct trapframe *scf;
	struct sigcontext13 *cntx;
	sigset_t mask;

	scf = p->p_addr->u_pcb.framep;
	cntx = SCARG(uap, sigcntxp);
	if (uvm_useracc((caddr_t)cntx, sizeof (*cntx), B_READ) == 0)
		return EINVAL;

	/* Compatibility mode? */
	if ((cntx->sc_ps & (PSL_IPL | PSL_IS)) ||
	    ((cntx->sc_ps & (PSL_U | PSL_PREVU)) != (PSL_U | PSL_PREVU)) ||
	    (cntx->sc_ps & PSL_CM)) {
		return (EINVAL);
	}
	if (cntx->sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	native_sigset13_to_sigset(&cntx->sc_mask, &mask);
	(void) sigprocmask1(p, SIG_SETMASK, &mask, 0);

	scf->fp = cntx->sc_fp;
	scf->ap = cntx->sc_ap;
	scf->pc = cntx->sc_pc;
	scf->sp = cntx->sc_sp;
	scf->psl = cntx->sc_ps;
	return (EJUSTRETURN);
}
#endif

int
sys___sigreturn14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct trapframe *scf;
	struct sigcontext *cntx;

	scf = p->p_addr->u_pcb.framep;
	cntx = SCARG(uap, sigcntxp);

	if (uvm_useracc((caddr_t)cntx, sizeof (*cntx), B_READ) == 0)
		return EINVAL;
	/* Compatibility mode? */
	if ((cntx->sc_ps & (PSL_IPL | PSL_IS)) ||
	    ((cntx->sc_ps & (PSL_U | PSL_PREVU)) != (PSL_U | PSL_PREVU)) ||
	    (cntx->sc_ps & PSL_CM)) {
		return (EINVAL);
	}
	if (cntx->sc_onstack & 01)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &cntx->sc_mask, 0);

	scf->fp = cntx->sc_fp;
	scf->ap = cntx->sc_ap;
	scf->pc = cntx->sc_pc;
	scf->sp = cntx->sc_sp;
	scf->psl = cntx->sc_ps;
	return (EJUSTRETURN);
}

struct trampframe {
	unsigned	sig;	/* Signal number */
	unsigned	code;	/* Info code */
	unsigned	scp;	/* Pointer to struct sigcontext */
	unsigned	r0, r1, r2, r3, r4, r5; /* Registers saved when
						 * interrupt */
	unsigned	pc;	/* Address of signal handler */
	unsigned	arg;	/* Pointer to first (and only) sigreturn
				 * argument */
};

void
sendsig(catcher, sig, mask, code)
	sig_t		catcher;
	int		sig;
	sigset_t	*mask;
	u_long		code;
{
	struct	proc	*p = curproc;
	struct	trapframe *syscf;
	struct	sigcontext *sigctx, gsigctx;
	struct	trampframe *trampf, gtrampf;
	unsigned	cursp;
	int	onstack;

	syscf = p->p_addr->u_pcb.framep;

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
	trampf = (struct trampframe *) ((unsigned)sigctx -
	    sizeof(struct trampframe));

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
		sigexit(p, SIGILL);

	syscf->pc = (int)p->p_sigctx.ps_sigcode;
	syscf->psl = PSL_U | PSL_PREVU;
	syscf->ap = cursp;
	syscf->sp = cursp;

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

	if (dumpdev == NODEV)
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
	switch ((*bdevsw[major(dumpdev)].d_dump) (dumpdev, 0, 0, 0)) {

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
process_read_regs(p, regs)
	struct proc    *p;
	struct reg     *regs;
{
	struct trapframe *tf = p->p_addr->u_pcb.framep;

	bcopy(&tf->r0, &regs->r0, 12 * sizeof(int));
	regs->ap = tf->ap;
	regs->fp = tf->fp;
	regs->sp = tf->sp;
	regs->pc = tf->pc;
	regs->psl = tf->psl;
	return 0;
}

int
process_write_regs(p, regs)
	struct proc    *p;
	struct reg     *regs;
{
	struct trapframe *tf = p->p_addr->u_pcb.framep;

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
process_set_pc(p, addr)
	struct	proc *p;
	caddr_t addr;
{
	struct	trapframe *tf;
	void	*ptr;

	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	ptr = (char *) p->p_addr->u_pcb.framep;
	tf = ptr;

	tf->pc = (unsigned) addr;

	return (0);
}

int
process_sstep(p, sstep)
	struct proc    *p;
{
	void	       *ptr;
	struct trapframe *tf;

	if ((p->p_flag & P_INMEM) == 0)
		return (EIO);

	ptr = p->p_addr->u_pcb.framep;
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
	extern vaddr_t iospace;
	vaddr_t addr;
	int pageno;
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
		pageno = rmalloc(iomap, size);
		if (pageno == 0) {
			if (warned++ == 0) /* Warn only once */
				printf("vax_map_physmem: iomap too small");
			return 0;
		}
		addr = iospace + (pageno * VAX_NBPG);
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
	extern vaddr_t iospace;
	int pageno = (addr - iospace) / VAX_NBPG;
#ifdef PHYSMEMDEBUG
	printf("vax_unmap_physmem: unmapping %d pages at addr %lx\n", 
	    size, addr);
#endif
	iounaccess(addr, size);
	if (size >= LTOHPN)
		uvm_km_free(kernel_map, addr, size * VAX_NBPG);
	else
		rmfree(iomap, size, pageno);
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
