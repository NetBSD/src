/*	$NetBSD: machdep.c,v 1.34 1998/04/03 01:56:34 mark Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * machdep.c
 *
 * Machine dependant functions for kernel setup
 *
 * This file needs a lot of work. 
 *
 * Created      : 17/09/94
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/map.h>
#include <sys/exec.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/msgbuf.h>
#include <vm/vm.h>
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

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <vm/vm_kern.h>

#include <machine/signal.h>
#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/katelib.h>
#include <machine/pte.h>
#include <machine/undefined.h>
#include <machine/bootconfig.h>

#include "ipkdb.h"
#include "md.h"

extern int physmem;

#ifndef PMAP_STATIC_L1S
extern int max_processes;
#endif	/* !PMAP_STATIC_L1S */
#if 0
extern int cpu_cache;
int cpu_ctrl;
#endif
#if NMD > 0 && defined(MEMORY_DISK_HOOKS) && !defined(MINIROOTSIZE)
extern u_int memory_disc_size;		/* Memory disc size */
#endif	/* NMD && MEMORY_DISK_HOOKS && !MINIROOTSIZE */

/* XXX - this needs to be properly defined elsewhere */
typedef struct {
	vm_offset_t physical;
	vm_offset_t virtual;
} pv_addr_t;

pv_addr_t systempage;
pv_addr_t kernelstack;

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

extern pt_entry_t msgbufpte;
caddr_t	msgbufaddr;
extern vm_offset_t msgbufphys;

#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif

int kernel_debug = 0;

struct user *proc0paddr;

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

int cold = 1;

/* Prototypes */

void consinit		__P((void));

void map_section	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa,
			     int cacheable));
void map_pagetable	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry		__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry_nc	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));
void map_entry_ro	__P((vm_offset_t pt, vm_offset_t va, vm_offset_t pa));

void pmap_bootstrap		__P((vm_offset_t kernel_l1pt));
u_long strtoul			__P((const char *s, char **ptr, int base));
caddr_t allocsys		__P((caddr_t v));
void data_abort_handler		__P((trapframe_t *frame));
void prefetch_abort_handler	__P((trapframe_t *frame));
void zero_page_readonly		__P((void));
void zero_page_readwrite	__P((void));
extern void configure		__P((void));
extern pt_entry_t *pmap_pte	__P((pmap_t pmap, vm_offset_t va));
extern void pmap_postinit	__P((void));
extern void dumpsys	__P((void));
#ifdef PMAP_DEBUG
extern void pmap_debug	__P((int level));
#endif	/* PMAP_DEBUG */

/*
 * Debug function just to park the CPU
 */

void
halt()
{
	while (1)
		cpu_sleep(0);
}


/* Sync the discs and unmount the filesystems */

void
bootsync(void)
{
	static int bootsyncdone = 0;

	if (bootsyncdone) return;

	bootsyncdone = 1;

	/* Make sure we can still manage to do things */
	if (GetCPSR() & I32_bit) {
		/*
		 * If we get here then boot has been called without RB_NOSYNC
		 * and interrupts were disabled. This means the boot() call
		 * did not come from a user process e.g. shutdown, but must
		 * have come from somewhere in the kernel.
		 */

		IRQenable;
		printf("Warning IRQ's disabled during boot()\n");
	}

	vfs_shutdown();
}

/*
 * Estimated loop for n microseconds
 */

/* Need to re-write this to use the timers */

/* One day soon I will actually do this */

int delaycount = 50;

void
delay(n)
	u_int n;
{
	u_int i;

	if (n == 0) return;
	while (--n > 0) {
		if (cputype == ID_SA110)	/* XXX - Seriously gross hack */
			for (i = delaycount; --i;);
		else
			for (i = 8; --i;);
	}
}


/*
 * A few functions that are used to help construct the page tables
 * during the bootstrap process.
 */

void
map_section(pagetable, va, pa, cacheable)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
	int cacheable;
{
#ifdef	DIAGNOSTIC
	if ((va & 0xfffff) != 0)
		panic("initarm: Cannot allocate 1MB section on non 1MB boundry\n");
#endif	/* DIAGNOSTIC */

	if (cacheable)
		((u_int *)pagetable)[(va >> PDSHIFT)] =
		    L1_SEC((pa & PD_MASK), PT_C);
	else
		((u_int *)pagetable)[(va >> PDSHIFT)] =
		    L1_SEC((pa & PD_MASK), 0);
}


void
map_pagetable(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
#ifdef	DIAGNOSTIC
	if ((pa & 0xc00) != 0)
		panic("pagetables should be group allocated on pageboundry");
#endif	/* DIAGNOSTIC */

	((u_int *)pagetable)[(va >> PDSHIFT) + 0] =
	     L1_PTE((pa & PG_FRAME) + 0x000);
	((u_int *)pagetable)[(va >> PDSHIFT) + 1] =
	     L1_PTE((pa & PG_FRAME) + 0x400);
	((u_int *)pagetable)[(va >> PDSHIFT) + 2] =
	     L1_PTE((pa & PG_FRAME) + 0x800);
	((u_int *)pagetable)[(va >> PDSHIFT) + 3] =
	     L1_PTE((pa & PG_FRAME) + 0xc00);
}


void
map_entry(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
/*
	((u_int *)pagetable)[((va >> PGSHIFT) & 0x000003ff)] =
	    L2_PTE((pa & PG_FRAME), AP_KRW));
*/
	WriteWord(pagetable + ((va >> 10) & 0x00000ffc),
	    L2_PTE((pa & PG_FRAME), AP_KRW));
}


void
map_entry_nc(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
/*
	((u_int *)pagetable)[((va >> PGSHIFT) & 0x000003ff)] =
	    L2_PTE_NC_NB((pa & PG_FRAME), AP_KRW));
*/
	WriteWord(pagetable + ((va >> 10) & 0x00000ffc),
	    L2_PTE_NC_NB((pa & PG_FRAME), AP_KRW));
}


void
map_entry_ro(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
/*
	((u_int *)pagetable)[((va >> PGSHIFT) & 0x000003ff)] =
	    L2_PTE((pa & PG_FRAME), AP_KR));
*/

	WriteWord(pagetable + ((va >> 10) & 0x00000ffc),
	    L2_PTE((pa & PG_FRAME), AP_KR));
}


/*
 * void cpu_startup(void)
 *
 * Machine dependant startup code. 
 *
 */

void
cpu_startup()
{
	int loop;
	vm_offset_t minaddr;
	vm_offset_t maxaddr;
	caddr_t sysbase;
	caddr_t size;
	vm_size_t bufsize;
	int base, residual;

	proc0paddr = (struct user *)kernelstack.virtual;
	proc0.p_addr = proc0paddr;

	/* Set the cpu control register */
	cpu_setup(boot_args);

	/* All domains MUST be clients, permissions are VERY important */
	cpu_domains(DOMAIN_CLIENT);

	/* Lock down zero page */
	zero_page_readonly();

	/*
	 * Give pmap a chance to set up a few more things now the vm
	 * is initialised
	 */
	pmap_postinit();

	/*
	 * Initialize error message buffer (at end of core).
	 */

	/* msgbufphys was setup during the secondary boot strap */
	for (loop = 0; loop < btoc(MSGBUFSIZE); ++loop)
		pmap_enter(pmap_kernel(),
		    (vm_offset_t)((caddr_t)msgbufaddr + loop * NBPG),
		    msgbufphys + loop * NBPG, VM_PROT_ALL, TRUE);
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));

	/*
	 * Identify ourselves for the msgbuf (everything printed earlier will
	 * not be buffered).
	 */
	printf(version);

	printf("real mem = %d (%d pages)\n", arm_page_to_byte(physmem), physmem);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
                                    
	size = allocsys((caddr_t)0);
	sysbase = (caddr_t)kmem_alloc(kernel_map, round_page(size));
	if (sysbase == 0)
		panic("cpu_startup: no room for system tables %d bytes required", (u_int)size);
	if ((caddr_t)((allocsys(sysbase) - sysbase)) != size)
		panic("cpu_startup: system table size inconsistency");

   	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */


	bufsize = MAXBSIZE * nbuf;
	printf("cpu_startup: buffer VM size = %ld\n", bufsize);
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t *)&buffers,
				   &maxaddr, bufsize, TRUE);
	minaddr = (vm_offset_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(bufsize),
	    (vm_offset_t)0, &minaddr, bufsize, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");

	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}

	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (loop = 0; loop < nbuf; ++loop) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */

		curbuf = (vm_offset_t)buffers + loop * MAXBSIZE;
		curbufsize = CLBYTES * (loop < residual ? base+1 : base);
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
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);

	/*
	 * Initialise callouts
	 */
	callfree = callout;

	for (loop = 1; loop < ncallout; ++loop)
		callout[loop - 1].c_next = &callout[loop];

	printf("avail mem = %d (%d pages)\n", (int)ptoa(cnt.v_free_count),
	    (int)ptoa(cnt.v_free_count) / NBPG);
	printf("using %d buffers containing %d bytes of memory\n",
	    nbuf, bufpages * CLBYTES);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */

	bufinit();

	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_flags = 0;
	curpcb->pcb_und_sp = (u_int)proc0.p_addr + USPACE_UNDEF_STACK_TOP;
	curpcb->pcb_sp = (u_int)proc0.p_addr + USPACE_SVC_STACK_TOP;
	curpcb->pcb_pagedir = (pd_entry_t *)pmap_extract(kernel_pmap,
	    (vm_offset_t)(kernel_pmap)->pm_pdir);
	    
	proc0.p_md.md_regs = (struct trapframe *)curpcb->pcb_sp - 1;

	/*
	 * Configure the hardware
	 */
	configure();

	cold = 0;	/* We are warm now ... */
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

#define valloc(name, type, num) \
	(caddr_t)(name) = (type *)v; \
	v = (caddr_t)((name) + (num));

	valloc(callout, struct callout, ncallout);

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
		if (physmem < arm_byte_to_page(2 * 1024 * 1024))
			bufpages = physmem / (10 * CLSIZE);
		else
			bufpages = (arm_byte_to_page(2 * 1024 * 1024)
			         + physmem) / (20 * CLSIZE);

	/*
	 * XXX stopgap measure to prevent wasting too much KVM on
	 * the sparsely filled buffer cache.
	 */
	if (bufpages * MAXBSIZE > VM_MAX_KERNEL_BUF)
		bufpages = VM_MAX_KERNEL_BUF / MAXBSIZE;

#ifdef DIAGNOSTIC
	if (bufpages == 0)
		panic("bufpages = 0\n");
#endif

	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}

	/*
	 * XXX stopgap measure to prevent wasting too much KVM on
	 * the sparsely filled buffer cache.
	 */
	if (nbuf * MAXBSIZE > VM_MAX_KERNEL_BUF)
		nbuf = VM_MAX_KERNEL_BUF / MAXBSIZE;

	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) & ~1;       /* force even */
		if (nswbuf > 256)
			nswbuf = 256;           /* sanity */
	}

	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);

	return(v);
}

/*
 * Initialise the console
 */

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;
	cninit();
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
	register struct trapframe *tf;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= -1)
		printf("setregs: ip=%08x sp=%08x proc=%08x\n",
		    (u_int) pack->ep_entry, (u_int) stack, (u_int) p);
#endif

	tf = p->p_md.md_regs;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= -1)
		printf("mdregs=%08x pc=%08x lr=%08x sp=%08x\n",
		    (u_int) tf, tf->tf_pc, tf->tf_usr_lr, tf->tf_usr_sp);
#endif

	tf->tf_r11 = 0;				/* bottom of the fp chain */
	tf->tf_r12 = stack;			/* needed by crt0.c */
	tf->tf_pc = pack->ep_entry;
	tf->tf_usr_lr = pack->ep_entry;
	tf->tf_svc_lr = 0x77777777;		/* Something we can see */
	tf->tf_usr_sp = stack;
	tf->tf_r10 = 0xaa55aa55;		/* Something we can see */
	tf->tf_spsr = PSR_USR32_MODE;

	p->p_addr->u_pcb.pcb_flags = 0;
}


/*
 * Modify the current mapping for zero page to make it read only
 *
 * This routine is only used until things start forking. Then new
 * system pages are mapped read only in pmap_enter().
 */

void
zero_page_readonly()
{
	WriteWord(0xefc00000, L2_PTE((systempage.physical & PG_FRAME), AP_KR));
	cpu_tlb_flushID_SE(0x00000000);
}


/*
 * Modify the current mapping for zero page to make it read/write
 *
 * This routine is only used until things start forking. Then system
 * pages belonging to user processes are never made writable.
 */

void
zero_page_readwrite()
{
	WriteWord(0xefc00000, L2_PTE((systempage.physical & PG_FRAME), AP_KRW));
	cpu_tlb_flushID_SE(0x00000000);
}


/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn resets the signal mask, the stack, and the
 * frame pointer, it returns to the user specified pc.
 */

void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	int mask;
	u_long code;
{
	struct proc *p = curproc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int oonstack;
	extern char sigcode[], esigcode[];

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("Sendsig: sig=%d mask=%08x catcher=%08x code=%08x\n",
		    sig, mask, (u_int)catcher, (u_int)code);
#endif

	tf = p->p_md.md_regs;
	oonstack = psp->ps_sigstk.ss_flags & SA_ONSTACK;

	/*
	 * Allocate space for the signal handler context.
	 */

	if ((psp->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(psp->ps_sigstk.ss_sp +
		    psp->ps_sigstk.ss_size - sizeof(struct sigframe));
		psp->ps_sigstk.ss_flags |= SA_ONSTACK;
	} else {
		fp = (struct sigframe *)tf->tf_usr_sp - 1;
	}

	/* 
	 * Build the argument list for the signal handler.
	 */

	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;
	frame.sf_handler = catcher;

	/*
	 * Build the signal context to be used by sigreturn.
	 */

	frame.sf_sc.sc_onstack = oonstack;
	frame.sf_sc.sc_mask   = mask;
	frame.sf_sc.sc_r0     = tf->tf_r0;
	frame.sf_sc.sc_r1     = tf->tf_r1;
	frame.sf_sc.sc_r2     = tf->tf_r2;
	frame.sf_sc.sc_r3     = tf->tf_r3;
	frame.sf_sc.sc_r4     = tf->tf_r4;
	frame.sf_sc.sc_r5     = tf->tf_r5;
	frame.sf_sc.sc_r6     = tf->tf_r6;
	frame.sf_sc.sc_r7     = tf->tf_r7;
	frame.sf_sc.sc_r8     = tf->tf_r8;
	frame.sf_sc.sc_r9     = tf->tf_r9;
	frame.sf_sc.sc_r10    = tf->tf_r10;
	frame.sf_sc.sc_r11    = tf->tf_r11;
	frame.sf_sc.sc_r12    = tf->tf_r12;
	frame.sf_sc.sc_usr_sp = tf->tf_usr_sp;
	frame.sf_sc.sc_usr_lr = tf->tf_usr_lr;
	frame.sf_sc.sc_svc_lr = tf->tf_svc_lr;
	frame.sf_sc.sc_pc     = tf->tf_pc;
	frame.sf_sc.sc_spsr   = tf->tf_spsr;

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

	tf->tf_r0 = frame.sf_signum;
	tf->tf_r1 = frame.sf_code;
	tf->tf_r2 = (u_int)frame.sf_scp;
	tf->tf_r3 = (u_int)frame.sf_handler;
	tf->tf_usr_sp = (int)fp;
	tf->tf_pc = (int)(((char *)PS_STRINGS) - (esigcode - sigcode));

	/* XXX - should just be a data purge and icache flush */
	cpu_cache_syncI();

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("Sendsig: sig=%d pc=%08x\n", sig, tf->tf_pc);
#endif
}


/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psr to gain improper privileges or to cause
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
	struct sigcontext *scp, context;
/*	register struct sigframe *fp;*/
	register struct trapframe *tf;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("sigreturn: context=%08x\n", (int)SCARG(uap, sigcntxp));
#endif

	tf = p->p_md.md_regs;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */

	scp = SCARG(uap, sigcntxp);

	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/*
	 * Check for security violations.
	 */

	/* Make sure the processor mode has not been tampered with */

	if ((context.sc_spsr & PSR_MODE) != PSR_USR32_MODE)
		return(EINVAL);

	if (context.sc_onstack & 01)
		p->p_sigacts->ps_sigstk.ss_flags |= SA_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SA_ONSTACK;
	p->p_sigmask = context.sc_mask & ~sigcantmask;

	/*
	 * Restore signal context.
	 */

	tf->tf_r0    = context.sc_r0;
	tf->tf_r1    = context.sc_r1;
	tf->tf_r2    = context.sc_r2;
	tf->tf_r3    = context.sc_r3;
	tf->tf_r4    = context.sc_r4;
	tf->tf_r5    = context.sc_r5;
	tf->tf_r6    = context.sc_r6;
	tf->tf_r7    = context.sc_r7;
	tf->tf_r8    = context.sc_r8;
	tf->tf_r9    = context.sc_r9;
	tf->tf_r10   = context.sc_r10;
	tf->tf_r11   = context.sc_r11;
	tf->tf_r12   = context.sc_r12;
	tf->tf_usr_sp = context.sc_usr_sp;
	tf->tf_usr_lr = context.sc_usr_lr;
	tf->tf_svc_lr = context.sc_svc_lr;
	tf->tf_pc    = context.sc_pc;
	tf->tf_spsr  = context.sc_spsr;

	return (EJUSTRETURN);
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
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_DEBUG:
		return(sysctl_int(oldp, oldlenp, newp, newlen, &kernel_debug));

	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/* 
 * Function to identify and process different types of boot argument
 */

int
get_bootconf_option(opts, opt, type, result)
	char *opts;
	char *opt;
	int type;
	void *result;
{
	char *ptr;
	char *optstart;
	int not;

	ptr = opts;

	while (*ptr) {
		/* Find start of option */
		while (*ptr == ' ' || *ptr == '\t')
			++ptr;

		if (*ptr == 0)
			break;

		not = 0;

		/* Is it a negate option */
		if ((type & BOOTOPT_TYPE_MASK) == BOOTOPT_TYPE_BOOLEAN && *ptr == '!') {
			not = 1;
			++ptr;
		}

		/* Find the end of option */
		optstart = ptr;
		while (*ptr != 0 && *ptr != ' ' && *ptr != '\t' && *ptr != '=')
			++ptr;

		if ((*ptr == '=')
		    || (*ptr != '=' && ((type & BOOTOPT_TYPE_MASK) == BOOTOPT_TYPE_BOOLEAN))) {
			/* compare the option */
			if (strncmp(optstart, opt, (ptr - optstart)) == 0) {
				/* found */

				if (*ptr == '=')
					++ptr;

				switch(type & BOOTOPT_TYPE_MASK) {
				case BOOTOPT_TYPE_BOOLEAN :
					if (*(ptr - 1) == '=')
						*((int *)result) = ((u_int)strtoul(ptr, NULL, 10) != 0);
					else
						*((int *)result) = !not;
					break;
				case BOOTOPT_TYPE_STRING :
					*((char **)result) = ptr;
					break;			
				case BOOTOPT_TYPE_INT :
					*((int *)result) = (u_int)strtoul(ptr, NULL, 10);
					break;
				case BOOTOPT_TYPE_BININT :
					*((int *)result) = (u_int)strtoul(ptr, NULL, 2);
					break;
				case BOOTOPT_TYPE_HEXINT :
					*((int *)result) = (u_int)strtoul(ptr, NULL, 16);
					break;
				default:
					return(0);
				}
				return(1);
			}
		}
		/* skip to next option */
		while (*ptr != ' ' && *ptr != '\t' && *ptr != 0)
			++ptr;
	}
	return(0);
}

void
parse_mi_bootargs(args)
	char *args;
{
	int integer;

	if (get_bootconf_option(args, "single", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-s", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_SINGLE;
	if (get_bootconf_option(args, "kdb", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-k", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_KDB;
	if (get_bootconf_option(args, "ask", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-a", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_ASKNAME;

#ifdef PMAP_DEBUG
	if (get_bootconf_option(args, "pmapdebug", BOOTOPT_TYPE_INT, &integer)) {
		pmap_debug_level = integer;
		pmap_debug(pmap_debug_level);
	}
#endif	/* PMAP_DEBUG */

	if (get_bootconf_option(args, "nbuf", BOOTOPT_TYPE_INT, &integer))
		bufpages = integer;

#ifndef PMAP_STATIC_L1S
	if (get_bootconf_option(args, "maxproc", BOOTOPT_TYPE_INT, &integer)) {
		max_processes = integer;
		if (max_processes < 16)
			max_processes = 16;
		/* Limit is PDSIZE * (max_processes + 1) <= 4MB */
		if (max_processes > 255)
			max_processes = 255;
	}
#endif	/* !PMAP_STATUC_L1S */
#if NMD > 0 && defined(MEMORY_DISK_HOOKS) && !defined(MINIROOTSIZE)
	if (get_bootconf_option(args, "memorydisc", BOOTOPT_TYPE_INT, &integer)
	    || get_bootconf_option(args, "memorydisk", BOOTOPT_TYPE_INT, &integer)) {
		memory_disc_size = integer;
		memory_disc_size *= 1024;
		if (memory_disc_size < 32*1024)
			memory_disc_size = 32*1024;
		if (memory_disc_size > 2048*1024)
			memory_disc_size = 2048*1024;
	}
#endif	/* NMD && MEMORY_DISK_HOOKS && !MINIROOTSIZE */
}

/* End of machdep.c */
