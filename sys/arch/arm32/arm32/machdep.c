/*	$NetBSD: machdep.c,v 1.75 2000/03/06 03:15:47 mark Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Machine dependant functions for kernel setup
 *
 * Created      : 17/09/94
 */

#include "opt_compat_netbsd.h"
#include "opt_footbridge.h"
#include "opt_md.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/map.h>
#include <sys/exec.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/msgbuf.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <vm/vm_kern.h>

#include <uvm/uvm_extern.h>

#include <machine/signal.h>
#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/katelib.h>
#include <machine/pte.h>
#include <machine/bootconfig.h>

#include "ipkdb.h"
#include "md.h"
#include "opt_mdsize.h"

vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

extern int physmem;

#ifndef PMAP_STATIC_L1S
extern int max_processes;
#endif	/* !PMAP_STATIC_L1S */
#if NMD > 0 && defined(MEMORY_DISK_HOOKS) && !defined(MINIROOTSIZE)
extern u_int memory_disc_size;		/* Memory disc size */
#endif	/* NMD && MEMORY_DISK_HOOKS && !MINIROOTSIZE */

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
	if (((va | pa) & (L1_SEC_SIZE - 1)) != 0)
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

vm_size_t
map_chunk(pd, pt, va, pa, size, acc, flg)
	vm_offset_t pd;
	vm_offset_t pt;
	vm_offset_t va;
	vm_offset_t pa;
	vm_size_t size;
	u_int acc;
	u_int flg;
{
	pd_entry_t *l1pt = (pd_entry_t *)pd;
	pt_entry_t *l2pt = (pt_entry_t *)pt;
	vm_size_t remain;
	u_int loop;

	remain = (size + (NBPG - 1)) & ~(NBPG - 1);
#ifdef VERBOSE_INIT_ARM
	printf("map_chunk: pa=%lx va=%lx sz=%lx rem=%lx acc=%x flg=%x\n",
	    pa, va, size, remain, acc, flg);
	printf("map_chunk: ");
#endif
	size = remain;

	while (remain > 0) {
		/* Can we do a section mapping ? */
		if (l1pt && !((pa | va) & (L1_SEC_SIZE - 1))
		    && remain >= L1_SEC_SIZE) {
#ifdef VERBOSE_INIT_ARM
			printf("S");
#endif
			l1pt[(va >> PDSHIFT)] = L1_SECPTE(pa, acc, flg);
			va += L1_SEC_SIZE;
			pa += L1_SEC_SIZE;
			remain -= L1_SEC_SIZE;
		} else
		/* Can we do a large page mapping ? */
		if (!((pa | va) & (L2_LPAGE_SIZE - 1))
		    && (remain >= L2_LPAGE_SIZE)) {
#ifdef VERBOSE_INIT_ARM
			printf("L");
#endif
			for (loop = 0; loop < 16; ++loop)
				l2pt[((va >> PGSHIFT) & 0x3f0) + loop] =
				    L2_LPTE(pa, acc, flg);
			va += L2_LPAGE_SIZE;
			pa += L2_LPAGE_SIZE;
			remain -= L2_LPAGE_SIZE;
		} else
		/* All we can do is a small page mapping */
		{
#ifdef VERBOSE_INIT_ARM
			printf("P");
#endif
			l2pt[((va >> PGSHIFT) & 0x3ff)] = L2_SPTE(pa, acc, flg);
			va += NBPG;
			pa += NBPG;
			remain -= NBPG;
		}
	}
#ifdef VERBOSE_INIT_ARM
	printf("\n");
#endif
	return(size);
}


void
map_entry(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
	((pt_entry_t *)pagetable)[((va >> PGSHIFT) & 0x000003ff)] =
	    L2_PTE((pa & PG_FRAME), AP_KRW);
}


void
map_entry_nc(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
	((pt_entry_t *)pagetable)[((va >> PGSHIFT) & 0x000003ff)] =
	    L2_PTE_NC_NB((pa & PG_FRAME), AP_KRW);
}


void
map_entry_ro(pagetable, va, pa)
	vm_offset_t pagetable;
	vm_offset_t va;
	vm_offset_t pa;
{
	((pt_entry_t *)pagetable)[((va >> PGSHIFT) & 0x000003ff)] =
	    L2_PTE((pa & PG_FRAME), AP_KR);
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
	char pbuf[9];

	proc0paddr = (struct user *)kernelstack.pv_va;
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
		    msgbufphys + loop * NBPG, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));

	/*
	 * Identify ourselves for the msgbuf (everything printed earlier will
	 * not be buffered).
	 */
	printf(version);

	format_bytes(pbuf, sizeof(pbuf), arm_page_to_byte(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	size = allocsys(NULL, NULL);
	sysbase = (caddr_t)uvm_km_zalloc(kernel_map, round_page(size));
	if (sysbase == 0)
		panic(
		    "cpu_startup: no room for system tables; %d bytes required",
		    (u_int)size);
	if ((caddr_t)((allocsys(sysbase, NULL) - sysbase)) != size)
		panic("cpu_startup: system table size inconsistency");

   	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	bufsize = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vm_offset_t *)&buffers, round_page(bufsize),
	    NULL, UVM_UNKNOWN_OFFSET,
	    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
	    UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
		panic("cpu_startup: cannot allocate UVM space for buffers");
	minaddr = (vm_offset_t)buffers;
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}

	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (loop = 0; loop < nbuf; ++loop) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;
		struct vm_page *pg;

		curbuf = (vm_offset_t) buffers + (loop * MAXBSIZE);
		curbufsize = NBPG * ((loop < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for buffer cache");
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

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 nmbclusters * mclbytes, VM_MAP_INTRSAFE,
				 FALSE, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_flags = 0;
	curpcb->pcb_und_sp = (u_int)proc0.p_addr + USPACE_UNDEF_STACK_TOP;
	curpcb->pcb_sp = (u_int)proc0.p_addr + USPACE_SVC_STACK_TOP;
	(void) pmap_extract(kernel_pmap, (vaddr_t)(kernel_pmap)->pm_pdir,
	    (paddr_t *)&curpcb->pcb_pagedir);

	proc0.p_md.md_regs = (struct trapframe *)curpcb->pcb_sp - 1;
}

#ifndef FOOTBRIDGE
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
#endif

/*
 * Clear registers on exec
 */

void
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct trapframe *tf;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= -1)
		printf("setregs: ip=%08lx sp=%08lx proc=%p\n",
		    pack->ep_entry, stack, p);
#endif

	tf = p->p_md.md_regs;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= -1)
		printf("mdregs=%08x pc=%08x lr=%08x sp=%08x\n",
		    (u_int) tf, tf->tf_pc, tf->tf_usr_lr, tf->tf_usr_sp);
#endif

	tf->tf_r0 = (u_int)PS_STRINGS;
	tf->tf_r1 = 0;
	tf->tf_r2 = 0;
	tf->tf_r3 = 0;
	tf->tf_r4 = 0;
	tf->tf_r5 = 0;
	tf->tf_r6 = 0;
	tf->tf_r7 = 0;
	tf->tf_r8 = 0;
	tf->tf_r9 = 0;
	tf->tf_r10 = 0;
	tf->tf_r11 = 0;				/* bottom of the fp chain */
#ifdef COMPAT_13
	tf->tf_r12 = stack;			/* needed by pre 1.4 crt0.c */
#else
	tf->tf_r12 = 0;
#endif
	tf->tf_usr_sp = stack;
	tf->tf_usr_lr = pack->ep_entry;
	tf->tf_svc_lr = 0x77777777;		/* Something we can see */
	tf->tf_pc = pack->ep_entry;
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
	WriteWord(PROCESS_PAGE_TBLS_BASE + 0,
	    L2_PTE((systempage.pv_pa & PG_FRAME), AP_KR));
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
	WriteWord(PROCESS_PAGE_TBLS_BASE + 0,
	    L2_PTE((systempage.pv_pa & PG_FRAME), AP_KRW));
	cpu_tlb_flushID_SE(0x00000000);
}


/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user specified pc.
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
		fp = (struct sigframe *)tf->tf_usr_sp;
	fp--;

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;
	frame.sf_handler = catcher;

	/* Save register context. */
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
	tf->tf_r0 = frame.sf_signum;
	tf->tf_r1 = frame.sf_code;
	tf->tf_r2 = (int)frame.sf_scp;
	tf->tf_r3 = (int)frame.sf_handler;
	tf->tf_usr_sp = (int)fp;
	tf->tf_pc = (int)psp->ps_sigcode;
	cpu_cache_syncI();

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
 * psr to gain improper privileges or to cause
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

	/* Make sure the processor mode has not been tampered with. */
	if ((context.sc_spsr & PSR_MODE) != PSR_USR32_MODE)
		return (EINVAL);

	/* Restore register context. */
	tf = p->p_md.md_regs;
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

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &context.sc_mask, 0);

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

/*	if (get_bootconf_option(args, "nbuf", BOOTOPT_TYPE_INT, &integer))
		bufpages = integer;*/

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
