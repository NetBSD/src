/*	$NetBSD: machdep.c,v 1.17 1997/03/27 21:01:39 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/sysctl.h>
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

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/prom.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <dev/cons.h>

#define	MAXMEM	64*1024*CLSIZE	/* XXX - from cmap.h */
#include <vm/vm_kern.h>

/* the following is used externally (sysctl_hw) */
char machine[] = "mvme68k";		/* cpu "architecture" */

vm_map_t buffer_map;
extern vm_offset_t avail_end;

/*
 * Model information, filled in by the Bug; see locore.s
 */
struct	mvmeprom_brdid  boardid;

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
int	msgbufmapped;		/* set when safe to use msgbuf */
int	maxmem;			/* max memory per process */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */

#ifdef	FPCOPROC
int fputype = FPU_68882;
#else
int fputype = FPU_NONE;
#endif

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

u_long myea; /* from ROM XXXCDC */

extern	u_int lowram;
extern	short exframesize[];

#ifdef COMPAT_HPUX
extern struct emul emul_hpux;
#endif
#ifdef COMPAT_SUNOS
extern struct emul emul_sunos;
#endif
#ifdef COMPAT_SVR4
extern struct emul emul_svr4;
#endif

/* prototypes for local functions */ 
void	identifycpu __P((void));
void	initcpu __P((void));
void	dumpsys __P((void));

/*
 * On the 68020/68030, the value of delay_divisor is roughly
 * 2048 / cpuspeed (where cpuspeed is in MHz).
 *
 * On the 68040/68060(?), the value of delay_divisor is roughly
 * 759 / cpuspeed (where cpuspeed is in MHz).
 */
int	cpuspeed;		/* only used for printing later */
int	delay_divisor = 82;	/* assume some reasonable value to start */

/* Machine-dependent initialization routines. */
void	mvme68k_init __P((void));

#ifdef MVME147
#include <mvme68k/dev/pccreg.h>
void	mvme147_init __P((void));
#endif

#ifdef MVME162
void	mvme162_init __P((void));
#endif

#ifdef MVME167
void	mvme167_init __P((void));
#endif

/*
 * Wrapper around the machine-specific initialization funtion,
 * to save some hair in locore.s
 */
void
mvme68k_init()
{

	switch (machineid) {
#ifdef MVME147
	case MVME_147:
		mvme147_init();
		break;
#endif
#ifdef MVME162
	case MVME_162:
		mvme162_init();
		break;
#endif
#ifdef MVME167
	case MVME_167:
		mvme167_init();
		break;
#endif
	default:
		panic("mvme68k_init: impossible machineid");
	}
}

#ifdef MVME147
/*
 * MVME-147 specific initialization.
 */
void
mvme147_init()
{
	struct pcc *pcc;

	pcc = (struct pcc *)PCC_VADDR(PCC_REG_OFF);

	/*
	 * calibrate delay() using the 6.25 usec counter.
	 * we adjust the delay_divisor until we get the result we want.
	 */
	pcc->t1_cr = PCC_TIMERCLEAR;
	pcc->t1_pload = 0;		/* init value for counter */
	pcc->t1_int = 0;		/* disable interrupt */

	for (delay_divisor = 140; delay_divisor > 0; delay_divisor--) {
		pcc->t1_cr = PCC_TIMERSTART;
		delay(10000);
		pcc->t1_cr = PCC_TIMERSTOP;
		if (pcc->t1_count > 1600)  /* 1600 * 6.25usec == 10000usec */
			break;	/* got it! */
		pcc->t1_cr = PCC_TIMERCLEAR;
		/* retry! */
	}

	/* calculate cpuspeed */
	cpuspeed = 2048 / delay_divisor;
}
#endif /* MVME147 */

#ifdef MVME162
/*
 * MVME-162 specific initialization.
 */
void
mvme162_init()
{

	/* XXX implement XXX */
}
#endif /* MVME162 */

#ifdef MVME167
/*
 * MVME-167 specific initializaion.
 */
void
mvme167_init()
{

	/* XXX implement XXX */
}
#endif /* MVME167 */

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#ifdef DDB
	ddb_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	extern char *kernel_text, *etext;
	register unsigned i;
	register caddr_t v, firstaddr;
	int base, residual;
	vm_offset_t minaddr, maxaddr;
	vm_size_t size;
#ifdef BUFFERS_UNMANAGED
	vm_offset_t bufmemp;
	caddr_t buffermem;
	int ix;
#endif
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * Initialize error message buffer (at end of core).
	 * avail_end was pre-decremented in pmap_bootstrap to compensate.
	 */
	for (i = 0; i < btoc(sizeof (struct msgbuf)); i++)
		pmap_enter(pmap_kernel(), (vm_offset_t)msgbufp,
		    avail_end + i * NBPG, VM_PROT_ALL, TRUE);
	msgbufmapped = 1;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	printf("real mem  = %d\n", ctob(physmem));

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
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
	 * Determine how many buffers to allocate.
	 * We just allocate a flat 5%.  Insure a minimum of 16 buffers.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		bufpages = physmem / 20 / CLSIZE;
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
		firstaddr = (caddr_t) kmem_alloc(kernel_map, round_page(size));
		if (firstaddr == 0)
			panic("startup: no room for tables");
#ifdef BUFFERS_UNMANAGED
		buffermem = (caddr_t) kmem_alloc(kernel_map, bufpages*CLBYTES);
		if (buffermem == 0)
			panic("startup: no room for buffers");
#endif
		goto again;
	}
	/*
	 * End of second pass, addresses have been assigned
	 */
	if ((vm_size_t)(v - firstaddr) != size)
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
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
#ifdef BUFFERS_UNMANAGED
	bufmemp = (vm_offset_t) buffermem;
#endif
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
#ifdef BUFFERS_UNMANAGED
		/*
		 * Move the physical pages over from buffermem.
		 */
		for (ix = 0; ix < curbufsize/CLBYTES; ix++) {
			vm_offset_t pa;

			pa = pmap_extract(pmap_kernel(), bufmemp);
			if (pa == 0)
				panic("startup: unmapped buffer");
			pmap_remove(pmap_kernel(), bufmemp, bufmemp+CLBYTES);
			pmap_enter(pmap_kernel(),
				   (vm_offset_t)(curbuf + ix * CLBYTES),
				   pa, VM_PROT_READ|VM_PROT_WRITE, TRUE);
			bufmemp += CLBYTES;
		}
#else
		vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
#endif
	}
#ifdef BUFFERS_UNMANAGED
#if 0
	/*
	 * We would like to free the (now empty) original address range
	 * but too many bad things will happen if we try.
	 */
	kmem_free(kernel_map, (vm_offset_t)buffermem, bufpages*CLBYTES);
#endif
#endif
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
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);
	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];
	callout[i-1].c_next = NULL;

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	printf("avail mem = %d\n", ptoa(cnt.v_free_count));
	printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);

	/*
	 * Tell the VM system that the area before the text segment
	 * is invalid.
	 *
	 * XXX Should just change KERNBASE and VM_MIN_KERNEL_ADDRESS,
	 * XXX but not right now.
	 */
	if (vm_map_protect(kernel_map, 0, round_page(&kernel_text),
	    VM_PROT_NONE, TRUE) != KERN_SUCCESS)
		panic("can't mark pre-text pages off-limits");

	/*
	 * Tell the VM system that writing to the kernel text isn't allowed.
	 * If we don't, we might end up COW'ing the text segment!
	 */
	if (vm_map_protect(kernel_map, trunc_page(&kernel_text),
	    round_page(&etext), VM_PROT_READ|VM_PROT_EXECUTE, TRUE)
	    != KERN_SUCCESS)
		panic("can't protect kernel text");

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

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
 * Set registers on exec.
 * XXX Should clear registers except sp, pc,
 * but would break init; should be fixed soon.
 */
void
setregs(p, pack, stack, retval)
	register struct proc *p;
	struct exec_package *pack;
	u_long stack;
	register_t *retval;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;

	frame->f_pc = pack->ep_entry & ~1;
	frame->f_regs[SP] = stack;
	frame->f_regs[A2] = (int)PS_STRINGS;
#ifdef FPCOPROC
	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
#endif
#ifdef COMPAT_HPUX
	p->p_md.md_flags &= ~MDP_HPUXMMAP;
	if (p->p_emul == &emul_hpux) {
		frame->f_regs[A0] = 0; /* not 68010 (bit 31), no FPA (30) */
		retval[0] = 0;		/* no float card */
#ifdef FPCOPROC
		retval[1] = 1;		/* yes 68881 */
#else
		retval[1] = 0;		/* no 68881 */
#endif
	}
	/*
	 * XXX This doesn't have much to do with setting registers but
	 * I didn't want to muck up kern_exec.c with this code, so I
	 * stuck it here.
	 *
	 * Ensure we perform the right action on traps type 1 and 2:
	 * If our parent is an HPUX process and we are being traced, turn
	 * on HPUX style interpretation.  Else if we were using the HPUX
	 * style interpretation, revert to the BSD interpretation.
	 *
	 * Note that we do this by changing the trap instruction in the
	 * global "sigcode" array which then gets copied out to the user's
	 * sigcode in the stack.  Since we are changing it in the global
	 * array we must always reset it, even for non-HPUX processes.
	 *
	 * Note also that implementing it in this way creates a potential
	 * race where we could have tweaked it for process A which then
	 * blocks in the copyout to the stack and process B comes along
	 * and untweaks it causing A to wind up with the wrong setting
	 * when the copyout continues.  However, since we have already
	 * copied something out to this user stack page (thereby faulting
	 * it in), this scenerio is extremely unlikely.
	 */
	{
		extern short sigcodetrap[];

		if ((p->p_pptr->p_emul == &emul_hpux) &&
		    (p->p_flag & P_TRACED)) {
			p->p_md.md_flags |= MDP_HPUXTRACE;
			*sigcodetrap = 0x4E42;
		} else {
			p->p_md.md_flags &= ~MDP_HPUXTRACE;
			*sigcodetrap = 0x4E41;
		}
	}
#endif
}

/*
 * Info for CTL_HW
 */
char	cpu_model[120];
extern	char version[];

void
identifycpu()
{
	char board_str[16];
	char cpu_str[32];
	char mmu_str[16];
	char fpu_str[16];
	int len = 0;

	bzero(cpu_model, sizeof(cpu_model));
	bzero(board_str, sizeof(board_str));
	bzero(cpu_str, sizeof(cpu_str));
	bzero(mmu_str, sizeof(mmu_str));
	bzero(fpu_str, sizeof(cpu_str));

	/* Fill in the CPU string. */
	switch (cputype) {
#ifdef M68020
	case CPU_68020:
		sprintf(cpu_str, "MC68020 CPU");
		sprintf(fpu_str, "MC68881 FPU");	/* XXX */
		break;
#endif

#ifdef M68030
	case CPU_68030:
		sprintf(cpu_str, "MC68030 CPU+MMU");
		sprintf(fpu_str, "MC68882 FPU");	/* XXX */
		break;
#endif

#ifdef M68040
	case CPU_68040:
		sprintf(cpu_str, "MC68040 CPU+MMU+FPU");
		break;
#endif

#ifdef M68060
	case CPU_68060:
		sprintf(cpu_str, "MC68060 CPU+MMU+FPU");
		break;
#endif

	default:
		printf("unknown CPU type");
		panic("startup");
	}

	/* Fill in the MMU string; only need to handle one case. */
	switch (mmutype) {
	case MMU_68851:
		sprintf(mmu_str, "MC68851 MMU");
		break;
	}

	/* XXX Find out FPU type and fill in string here. */

	/* Fill in board model string. */
	switch (machineid) {
#ifdef MVME147
	case MVME_147: {
		char *suffix = (char *)&boardid.suffix;
		len = sprintf(board_str, "%x", machineid);
		if (suffix[0] != '\0') {
			board_str[len++] = suffix[0];
			if (suffix[1] != '\0')
				board_str[len++] = suffix[1];
		}
		break; }
#endif

#if defined(MVME162) || defined(MVME167) || defined(MVME177)
	case MVME_162:
	case MVME_167:
	case MVME_177: {
		int i;
		char c;
		for (i = 0; i < sizeof(boardid.longname); i++) {
			c = boardid.longname[i];
			if (c == '\0' || c == ' ')
				break;
			board_str[i] = c;
		}
		break; }
#endif
	default:
		printf("unknown machine type: 0x%x\n", machineid);
		panic("startup");
	}

	len = sprintf(cpu_model, "Motorola MVME-%s: %dMHz %s", board_str,
	    cpuspeed, cpu_str);

	if (mmu_str[0] != '\0')
		len += sprintf(cpu_model + len, ", %s", mmu_str);

	if (fpu_str[0] != '\0')
		len += sprintf(cpu_model + len, ", %s", fpu_str);

#if defined(M68040) || defined(M68060)
	switch (cputype) {
	case CPU_68040:
	case CPU_68060:		/* XXX is this right? */
		strcat(cpu_model, ", 4k on-chip physical I/D caches");
	}
#endif

	printf("%s\n", cpu_model);
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

/* See: sig_machdep.c */

int	waittime = -1;

void
cpu_reboot(howto, bootstr)
	register int howto;
	char *bootstr;
{
	extern int cold;

	/* take a snap shot before clobbering any registers */
	if (curproc && curproc->p_addr)
		savectx(curproc->p_addr);

	/* Save the RB_SBOOT flag. */
	howto |= (boothowto & RB_SBOOT);

	/* If system is hold, just halt. */
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
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested, do it. */
	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

#if defined(PANICWAIT) && !defined(DDB)
	if ((howto & RB_HALT) == 0 && panicstr) {
		printf("hit any key to reboot...\n");
		(void)cngetc();
		printf("\n");
	}
#endif

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		printf("halted\n\n");
		doboot(RB_HALT);
		/* NOTREACHED */
	}

	printf("rebooting...\n");
	delay(1000000);
	doboot(RB_AUTOBOOT);
	/*NOTREACHED*/
}

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

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

	/*
	 * XXX include the final RAM page which is not included in physmem.
	 */
	dumpsize = physmem + 1;

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
 * Dump physical memory onto the dump device.
 */
void
dumpsys()
{
	daddr_t blkno;		/* current block to write */
				/* dump routine */
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int pg;			/* page being dumped */
	vm_offset_t maddr;	/* PA being dumped */
	int error;		/* error code from (*dump)() */

	/* Don't put dump messages in msgbuf. */
	msgbufmapped = 0;

	/* Make sure dump device is valid. */
	if (dumpdev == NODEV)
		return;
	if (dumpsize == 0) {  
		cpu_dumpconf();
		if (dumpsize == 0)
			return;
	}
	if (dumplo < 0)
		return;
	dump = bdevsw[major(dumpdev)].d_dump;
	blkno = dumplo;

	printf("\ndumping to dev 0x%x, offset %d\n", dumpdev, dumplo);

	printf("dump ");
	maddr = lowram;
	for (pg = 0; pg < dumpsize; pg++) {
#define	NPGMB	(1024*1024/NBPG)
		/* print out how many MBs we have dumped */
		if (pg && (pg % NPGMB) == 0)
			printf("%d ", pg / NPGMB);
#undef NPGMB
		pmap_enter(pmap_kernel(), (vm_offset_t)vmmap, maddr,
		    VM_PROT_READ, TRUE);

		error = (*dump)(dumpdev, blkno, vmmap, NBPG);
		switch (error) {
		case 0:
			maddr += NBPG;
			blkno += btodb(NBPG);
			break;

		case ENXIO:
			printf("device bad\n");
			return;

		case EFAULT:
			printf("device not ready\n");
			return;

		case EINVAL:
			printf("area improper\n");
			return;

		case EIO:
			printf("i/o error\n");
			return;

		case EINTR:
			printf("aborted from console\n");
			return;

		default:
			printf("error %d\n", error);
			return;
		}
	}
	printf("succeeded\n");
}

void
initcpu()
{
#ifdef MAPPEDCOPY
	extern u_int mappedcopysize;

	/*
	 * Initialize lower bound for doing copyin/copyout using
	 * page mapping (if not already set).  We don't do this on
	 * VAC machines as it loses big time.
	 */
	if (mappedcopysize == 0) {
		mappedcopysize = NBPG;
	}
#endif
}

straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
}

int	*nofault;

int
badaddr(addr, nbytes)
	register caddr_t addr;
	int nbytes;
{
	register int i;
	label_t faultbuf;

#ifdef lint
	i = *addr; if (i) return (0);
#endif

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}

	switch (nbytes) {
	case 1:
		i = *(volatile char *)addr;
		break;

	case 2:
		i = *(volatile short *)addr;
		break;

	case 4:
		i = *(volatile int *)addr;
		break;

	default:
		panic("badaddr: bad request");
	}
	nofault = (int *) 0;
	return (0);
}

/* XXX wrapper for locore.s; used only my level 7 autovector */
void
nmintr(frame)
	struct frame frame;
{
	nmihand(&frame);
}

/*
 * Level 7 interrupts are caused by e.g. the ABORT switch.
 *
 * If we have DDB, then break into DDB on ABORT.  In a production
 * environment, bumping the ABORT switch would be bad, so we enable
 * panic'ing on ABORT with the kernel option "PANICBUTTON".
 */
void
nmihand(frame)
	struct frame *frame;
{

	mvme68k_abort("ABORT SWITCH");
}

/*
 * Common code for handling ABORT signals from buttons, switches,
 * serial lines, etc.
 */
void
mvme68k_abort(cp)
	const char *cp;
{
#ifdef DDB
	printf("%s\n", cp);
	Debugger();
#else
#ifdef PANICBUTTON
	panic(cp);
#else
	printf("%s ignored\n", cp);
#endif /* PANICBUTTON */
#endif /* DDB */
}

regdump(fp, sbytes)
	struct frame *fp; /* must not be register */
	int sbytes;
{
	static int doingdump = 0;
	register int i;
	int s;
	extern char *hexstr();

	if (doingdump)
		return;
	s = splhigh();
	doingdump = 1;
	printf("pid = %d, pc = %s, ",
	       curproc ? curproc->p_pid : -1, hexstr(fp->f_pc, 8));
	printf("ps = %s, ", hexstr(fp->f_sr, 4));
	printf("sfc = %s, ", hexstr(getsfc(), 4));
	printf("dfc = %s\n", hexstr(getdfc(), 4));
	printf("Registers:\n     ");
	for (i = 0; i < 8; i++)
		printf("        %d", i);
	printf("\ndreg:");
	for (i = 0; i < 8; i++)
		printf(" %s", hexstr(fp->f_regs[i], 8));
	printf("\nareg:");
	for (i = 0; i < 8; i++)
		printf(" %s", hexstr(fp->f_regs[i+8], 8));
	if (sbytes > 0) {
		if (fp->f_sr & PSL_S) {
			printf("\n\nKernel stack (%s):",
			       hexstr((int)(((int *)&fp)-1), 8));
			dumpmem(((int *)&fp)-1, sbytes, 0);
		} else {
			printf("\n\nUser stack (%s):", hexstr(fp->f_regs[SP], 8));
			dumpmem((int *)fp->f_regs[SP], sbytes, 1);
		}
	}
	doingdump = 0;
	splx(s);
}

#define KSADDR	((int *)((u_int)curproc->p_addr + USPACE - NBPG))

dumpmem(ptr, sz, ustack)
	register int *ptr;
	int sz, ustack;
{
	register int i, val;
	extern char *hexstr();

	for (i = 0; i < sz; i++) {
		if ((i & 7) == 0)
			printf("\n%s: ", hexstr((int)ptr, 6));
		else
			printf(" ");
		if (ustack == 1) {
			if ((val = fuword(ptr++)) == -1)
				break;
		} else {
			if (ustack == 0 &&
			    (ptr < KSADDR || ptr > KSADDR+(NBPG/4-1)))
				break;
			val = *ptr++;
		}
		printf("%s", hexstr(val, 8));
	}
	printf("\n");
}

char *
hexstr(val, len)
	register int val;
	int len;
{
	static char nbuf[9];
	register int x, i;

	if (len > 8)
		return("");
	nbuf[len] = '\0';
	for (i = len-1; i >= 0; --i) {
		x = val & 0xF;
		if (x > 9)
			nbuf[i] = x - 10 + 'A';
		else
			nbuf[i] = x + '0';
		val >>= 4;
	}
	return(nbuf);
}

/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 * 
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 */
cpu_exec_aout_makecmds(p, epp)
    struct proc *p;
    struct exec_package *epp;
{
    int error = ENOEXEC;
#ifdef COMPAT_SUNOS
    extern sunos_exec_aout_makecmds
        __P((struct proc *, struct exec_package *));
    if ((error = sunos_exec_aout_makecmds(p, epp)) == 0)
        return 0;
#endif
    return error;
}

void
myetheraddr(ether)
	char *ether;
{
	int e = myea;

	ether[0] = 0x08;
	ether[1] = 0x00;
	ether[2] = 0x3e;
	e = e >> 8;
	ether[5] = e & 0xff;
	e = e >> 8;
	ether[4] = e & 0xff;
	e = e >> 8;
	ether[3] = e;
}
