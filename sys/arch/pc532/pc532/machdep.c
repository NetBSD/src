/*	$NetBSD: machdep.c,v 1.120 2001/04/22 23:30:46 thorpej Exp $	*/

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

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"

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
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/syscallargs.h>
#include <sys/core.h>
#include <sys/kcore.h>

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

vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

extern	char etext[], end[];
#if defined(DDB)
extern char *esym;
#endif
extern	paddr_t avail_start, avail_end;
extern	int nkpde;
extern	int ieee_handler_disable;

static paddr_t	alloc_pages __P((int));
static int	cpu_dump __P((void));
static int	cpu_dumpsize __P((void));
static void	cpu_reset __P((void));
static void	dumpsys __P((void));
void		init532 __P((void));
static void	map __P((pd_entry_t *, vaddr_t, paddr_t, int, int));

/*
 * Machine-dependent startup code
 */
void
cpu_startup()
{
	extern char kernel_text[];
	unsigned i;
	caddr_t v;
	int sz;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];

	/*
	 * Initialize error message buffer (at end of core).
	 */
	msgbuf_vaddr =  uvm_km_valloc(kernel_map, ns532_round_page(MSGBUFSIZE));
	if (msgbuf_vaddr == NULL)
		panic("failed to valloc msgbuf_vaddr");

	/* msgbuf_paddr was init'd in pmap */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_kenter_pa(msgbuf_vaddr + i * NBPG,
		    msgbuf_paddr + i * NBPG, VM_PROT_READ | VM_PROT_WRITE);

	initmsgbuf((caddr_t)msgbuf_vaddr, round_page(MSGBUFSIZE));

	printf(version);
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
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
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
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

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
				   16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    nmbclusters * mclbytes, VM_MAP_INTRSAFE, FALSE, NULL);

	/*
	 * Tell the VM system that writing to kernel text isn't allowed.
	 * If we don't, we might end up COW'ing the text segment!
	 */
	if (uvm_map_protect(kernel_map,
			   ns532_round_page(&kernel_text),
			   ns532_round_page(&etext),
			   UVM_PROT_READ|UVM_PROT_EXEC, TRUE) != 0)
		panic("can't protect kernel text");

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
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

	case CPU_IEEE_DISABLE:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &ieee_handler_disable));

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
	struct reg *regs;
	struct sigframe *fp, frame;
	int onstack;

	regs = p->p_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigframe *)((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
					  p->p_sigctx.ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)regs->r_sp;
	fp--;

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;
	frame.sf_handler = catcher;

	/* Save the register context. */
	frame.sf_sc.sc_fp = regs->r_fp;
	frame.sf_sc.sc_sp = regs->r_sp;
	frame.sf_sc.sc_pc = regs->r_pc;
	frame.sf_sc.sc_ps = regs->r_psr;
	frame.sf_sc.sc_sb = regs->r_sb;
	frame.sf_sc.sc_reg[REG_R7] = regs->r_r7;
	frame.sf_sc.sc_reg[REG_R6] = regs->r_r6;
	frame.sf_sc.sc_reg[REG_R5] = regs->r_r5;
	frame.sf_sc.sc_reg[REG_R4] = regs->r_r4;
	frame.sf_sc.sc_reg[REG_R3] = regs->r_r3;
	frame.sf_sc.sc_reg[REG_R2] = regs->r_r2;
	frame.sf_sc.sc_reg[REG_R1] = regs->r_r1;
	frame.sf_sc.sc_reg[REG_R0] = regs->r_r0;

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save the signal mask. */
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
	regs->r_sp = (int)fp;
	regs->r_pc = (int)p->p_sigctx.ps_sigcode;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
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
	struct reg *regs;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/* Restore the register context. */
	regs = p->p_md.md_regs;

	/*
	 * Check for security violations.
	 */
	if (((context.sc_ps ^ regs->r_psr) & PSL_USERSTATIC) != 0)
		return (EINVAL);

	regs->r_fp  = context.sc_fp;
	regs->r_sp  = context.sc_sp;
	regs->r_pc  = context.sc_pc;
	regs->r_psr = context.sc_ps;
	regs->r_sb  = context.sc_sb;
	regs->r_r7  = context.sc_reg[REG_R7];
	regs->r_r6  = context.sc_reg[REG_R6];
	regs->r_r5  = context.sc_reg[REG_R5];
	regs->r_r4  = context.sc_reg[REG_R4];
	regs->r_r3  = context.sc_reg[REG_R3];
	regs->r_r2  = context.sc_reg[REG_R2];
	regs->r_r1  = context.sc_reg[REG_R1];
	regs->r_r0  = context.sc_reg[REG_R0];

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &context.sc_mask, 0);

	return(EJUSTRETURN);
}

int waittime = -1;
static struct switchframe dump_sf;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
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
			dump_sf.sf_p  = curproc;
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
u_long	dumpmag = 0x8fca0101;	/* magic number */
int 	dumpsize = 0;		/* pages */
long	dumplo = 0; 		/* blocks */

/*
 * cpu_dumpsize: calculate size of machine-dependent kernel core dump headers.
 */
static int
cpu_dumpsize()
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
cpu_dump()
{
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	long buf[dbtob(1) / sizeof (long)];
	kcore_seg_t	*segp;
	cpu_kcore_hdr_t	*cpuhdrp;

        dump = bdevsw[major(dumpdev)].d_dump;

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

	return (dump(dumpdev, dumplo, (caddr_t)buf, dbtob(1)));
}

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first NBPG of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{
	int nblks, dumpblks;	/* size of dump area */
	int maj;

	if (dumpdev == NODEV)
		return;
	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdevsw[maj].d_psize == NULL)
		goto bad;
	nblks = (*bdevsw[maj].d_psize)(dumpdev);
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
	unsigned bytes, i, n;
	int maddr, psize;
	daddr_t blkno;
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int error;

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

	/* XXX should purge all outstanding keystrokes. */

	if ((error = cpu_dump()) != 0)
		goto err;

	bytes = ctob(physmem);
	maddr = 0;
	blkno = dumplo + cpu_dumpsize();
	dump = bdevsw[major(dumpdev)].d_dump;
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
		error = (*dump)(dumpdev, blkno, (caddr_t)dumpspace, n);
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
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct reg *r = p->p_md.md_regs;
	struct pcb *pcbp = &p->p_addr->u_pcb;
	extern struct proc *fpu_proc;

	if (p == fpu_proc)
		fpu_proc = 0;

	memset(r, 0, sizeof(*r));
	r->r_sp  = stack;
	r->r_pc  = pack->ep_entry;
	r->r_psr = PSL_USERSET;
	r->r_r7  = (int)PS_STRINGS;

	pcbp->pcb_fsr = FPC_UEN;
	memset(pcbp->pcb_freg, 0, sizeof(pcbp->pcb_freg));
}

/*
 * Allocate memory pages.
 */
static paddr_t
alloc_pages(pages)
	int pages;
{
	paddr_t p = avail_start;
	avail_start += pages * NBPG;
	memset((caddr_t) p, 0, pages * NBPG);
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
	vaddr_t virtual;
	paddr_t physical;
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
		if (physical != (paddr_t) -1) {
			pt[ix2] = (pt_entry_t) (physical | protection | PG_V);
			physical += NBPG;
			size -= NBPG;
		} else {
			size -= (PTES_PER_PTP - ix2) * NBPG;
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
 * The cpu config register gets set.
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
init532()
{
	extern void main __P((void *));
	extern int inttab[];
#ifdef DDB
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
#ifdef DDB
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
	proc0.p_addr = proc0paddr;

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
	__asm __volatile("jump @1f; 1:");

	/* Initialize the pmap module. */
	pmap_bootstrap(avail_start + KERNBASE);

	/* Construct an empty syscframe for proc0. */
	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_onstack = (struct reg *)
			      ((u_int)proc0.p_addr + USPACE) - 1;

	/* Switch to proc0's stack. */
	lprd(sp, curpcb->pcb_onstack);
	lprd(fp, 0);

	main(curpcb->pcb_onstack);
	panic("main returned to init532\n");
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
	register struct queue *elem = v1, *head = v2;
	register struct queue *next;

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
	register struct queue *elem = v;
	register struct queue *next, *prev;

	next = elem->q_next;
	prev = elem->q_prev;
	next->q_prev = prev;
	prev->q_next = next;
	elem->q_prev = 0;
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
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
	cninit();

#ifdef KGDB
	if (boothowto & RB_KDB) {
		extern int kgdb_debug_init;
		kgdb_debug_init = 1;
	}
#endif
#if defined (DDB)
	ddb_init(*(int *)end, ((int *)end) + 1, (int *)esym);
        if(boothowto & RB_KDB)
                Debugger();
#endif
}

static void
cpu_reset()
{
	/* Mask all ICU interrupts. */
	splhigh();

	/* Disable CPU interrupts. */
	di();

	/* Alias kernel memory at 0. */
	PDP_BASE[0] = PDP_BASE[pdei(KERNBASE)];
	tlbflush();

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
	memcpy((void *)0, (void *)0x10000000, 0x1f00);

	/* Jump into ROM copy. */
	__asm __volatile("jump @0");
}

/*
 * Network software interrupt routine
 */
void
softnet(arg)
	void *arg;
{
	register int isr;

	di(); isr = netisr; netisr = 0; ei();
	if (isr == 0) return;

#define DONETISR(bit, fn)		\
    do {				\
	if (isr & (1 << bit))		\
		fn();			\
    } while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}
