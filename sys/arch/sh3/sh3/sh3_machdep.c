/*	$NetBSD: sh3_machdep.c,v 1.93 2011/07/20 21:30:13 dyoung Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sh3_machdep.c,v 1.93 2011/07/20 21:30:13 dyoung Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_memsize.h"
#include "opt_kstack_debug.h"
#include "opt_ptrace.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/ras.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscallargs.h>
#include <sys/ucontext.h>
#include <sys/cpu.h>

#ifdef KGDB
#include <sys/kgdb.h>
#ifndef KGDB_DEVNAME
#define	KGDB_DEVNAME "nodev"
#endif
const char kgdb_devname[] = KGDB_DEVNAME;
#endif /* KGDB */

#include <uvm/uvm.h>

#include <sh3/cache.h>
#include <sh3/clock.h>
#include <sh3/exception.h>
#include <sh3/locore.h>
#include <sh3/mmu.h>
#include <sh3/pcb.h>
#include <sh3/intr.h>
#include <sh3/ubcreg.h>

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;
int cpu_arch;
int cpu_product;
char cpu_model[120];

struct vm_map *phys_map;

struct pcb *curpcb;

#if !defined(IOM_RAM_BEGIN)
#error "define IOM_RAM_BEGIN"
#elif (IOM_RAM_BEGIN & SH3_P1SEG_BASE) != 0
#error "IOM_RAM_BEGIN is physical address. not P1 address."
#endif

#define	VBR	(uint8_t *)SH3_PHYS_TO_P1SEG(IOM_RAM_BEGIN)
vaddr_t ram_start = SH3_PHYS_TO_P1SEG(IOM_RAM_BEGIN);
/* exception handler holder (sh3/sh3/exception_vector.S) */
extern char sh_vector_generic[], sh_vector_generic_end[];
extern char sh_vector_interrupt[], sh_vector_interrupt_end[];
#ifdef SH3
extern char sh3_vector_tlbmiss[], sh3_vector_tlbmiss_end[];
#endif
#ifdef SH4
extern char sh4_vector_tlbmiss[], sh4_vector_tlbmiss_end[];
#endif
/*
 * These variables are needed by /sbin/savecore
 */
uint32_t dumpmag = 0x8fca0101;	/* magic number */
int dumpsize;			/* pages */
long dumplo;	 		/* blocks */


void
sh_cpu_init(int arch, int product)
{
	/* CPU type */
	cpu_arch = arch;
	cpu_product = product;

#if defined(SH3) && defined(SH4)
	/* Set register addresses */
	sh_devreg_init();
#endif
	/* Cache access ops. */
	sh_cache_init();

	/* MMU access ops. */
	sh_mmu_init();

	/* Hardclock, RTC initialize. */
	machine_clock_init();

	/* ICU initiailze. */
	curcpu()->ci_idepth = -1;
	intc_init();

	/* Exception vector. */
	memcpy(VBR + 0x100, sh_vector_generic,
	    sh_vector_generic_end - sh_vector_generic);
#ifdef SH3
	if (CPU_IS_SH3)
		memcpy(VBR + 0x400, sh3_vector_tlbmiss,
		    sh3_vector_tlbmiss_end - sh3_vector_tlbmiss);
#endif
#ifdef SH4
	if (CPU_IS_SH4)
		memcpy(VBR + 0x400, sh4_vector_tlbmiss,
		    sh4_vector_tlbmiss_end - sh4_vector_tlbmiss);
#endif
	memcpy(VBR + 0x600, sh_vector_interrupt,
	    sh_vector_interrupt_end - sh_vector_interrupt);

	if (!SH_HAS_UNIFIED_CACHE)
		sh_icache_sync_all();

	__asm volatile("ldc %0, vbr" :: "r"(VBR));

	/* kernel stack setup */
	__sh_switch_resume = CPU_IS_SH3 ? sh3_switch_resume : sh4_switch_resume;

	/* Set page size (4KB) */
	uvm_setpagesize();

	/* setup UBC channel A for single-stepping */
#if defined(PTRACE) || defined(DDB)
	_reg_write_2(SH_(BBRA), 0); /* disable channel A */
	_reg_write_2(SH_(BBRB), 0); /* disable channel B */

#ifdef SH3
	if (CPU_IS_SH3) {
		/* A: break after execution, ignore ASID */
		_reg_write_4(SH3_BRCR, (UBC_CTL_A_AFTER_INSN
					| SH3_UBC_CTL_A_MASK_ASID));

		/* A: compare all address bits */
		_reg_write_4(SH3_BAMRA, 0x00000000);
	}
#endif	/* SH3 */

#ifdef SH4
	if (CPU_IS_SH4) {
		/* A: break after execution */
		_reg_write_2(SH4_BRCR, UBC_CTL_A_AFTER_INSN);

		/* A: compare all address bits, ignore ASID */
		_reg_write_1(SH4_BAMRA, SH4_UBC_MASK_NONE | SH4_UBC_MASK_ASID);
	}
#endif	/* SH4 */
#endif
}


/*
 * void sh_proc0_init(void):
 *	Setup proc0 u-area.
 */
void
sh_proc0_init()
{
	struct switchframe *sf;
	vaddr_t u;

	/* Steal process0 u-area */
	u = uvm_pageboot_alloc(USPACE);
	memset((void *)u, 0, USPACE);

	/* Setup uarea for lwp0 */
	uvm_lwp_setuarea(&lwp0, u);

	/*
	 * u-area map:
	 * |pcb| .... | .................. |
	 * | PAGE_SIZE | USPACE - PAGE_SIZE |
         *        frame bot        stack bot
	 * current frame ... r6_bank
	 * stack bottom  ... r7_bank
	 * current stack ... r15
	 */
	curpcb = lwp_getpcb(&lwp0);
	lwp0.l_md.md_pcb = curpcb;

	sf = &curpcb->pcb_sf;

#ifdef KSTACK_DEBUG
	memset((char *)(u + sizeof(struct pcb)), 0x5a,
	    PAGE_SIZE - sizeof(struct pcb));
	memset((char *)(u + PAGE_SIZE), 0xa5, USPACE - PAGE_SIZE);
	memset(sf, 0xb4, sizeof(struct switchframe));
#endif /* KSTACK_DEBUG */

	sf->sf_r6_bank = u + PAGE_SIZE;
	sf->sf_r7_bank = sf->sf_r15 = u + USPACE;
	__asm volatile("ldc %0, r6_bank" :: "r"(sf->sf_r6_bank));
	__asm volatile("ldc %0, r7_bank" :: "r"(sf->sf_r7_bank));

	lwp0.l_md.md_regs = (struct trapframe *)sf->sf_r6_bank - 1;
}

void
sh_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	printf("%s%s", copyright, version);
	if (*cpu_model != '\0')
		printf("%s", cpu_model);
#ifdef DEBUG
	printf("general exception handler:\t%d byte\n",
	    sh_vector_generic_end - sh_vector_generic);
	printf("TLB miss exception handler:\t%d byte\n",
#if defined(SH3) && defined(SH4)
	    CPU_IS_SH3 ? sh3_vector_tlbmiss_end - sh3_vector_tlbmiss :
	    sh4_vector_tlbmiss_end - sh4_vector_tlbmiss
#elif defined(SH3)
	    sh3_vector_tlbmiss_end - sh3_vector_tlbmiss
#elif defined(SH4)
	    sh4_vector_tlbmiss_end - sh4_vector_tlbmiss
#endif
	    );
	printf("interrupt exception handler:\t%d byte\n",
	    sh_vector_interrupt_end - sh_vector_interrupt);
#endif /* DEBUG */

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, false, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
}

void
dumpsys(void)
{
}

/*
 * void cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted,
 *     void *sas, void *ap, void *sp, sa_upcall_t upcall):
 *
 * Send an upcall to userland.
 */
void
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted, void *sas,
    void *ap, void *sp, sa_upcall_t upcall)
{
	struct trapframe *tf;
	struct saframe *sf, frame;

	tf = l->l_md.md_regs;

	/* Build the stack frame. */
#if 0 /* First 4 args in regs (see below). */
	frame.sa_type = type;
	frame.sa_sas = sas;
	frame.sa_events = nevents;
	frame.sa_interrupted = ninterrupted;
#endif
	frame.sa_arg = ap;

	sf = (struct saframe *)sp - 1;
	if (copyout(&frame, sf, sizeof(frame)) != 0) {
		/* Copying onto the stack didn't work.  Die. */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	tf->tf_r4 = type;
	tf->tf_r5 = (int) sas;
	tf->tf_r6 = nevents;
	tf->tf_r7 = ninterrupted;

	tf->tf_spc = (int) upcall;
	tf->tf_pr = 0;		/* no return */
	tf->tf_r15 = (int) sf;
}

/*
 * Get the base address of the signal frame either on the lwp's stack
 * or on the signal stack and set *onstack accordingly.  Caller then
 * just subtracts the size of appropriate struct sigframe_foo.
 */
void *
getframe(const struct lwp *l, int sig, int *onstack)
{
	const struct proc *p = l->l_proc;
	const struct sigaltstack *sigstk= &l->l_sigstk;

	/* Do we need to jump onto the signal stack? */
	*onstack = (sigstk->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
		&& (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	if (*onstack)
		return ((char *)sigstk->ss_sp + sigstk->ss_size);
	else
		return ((void *)l->l_md.md_regs->tf_r15);
}

void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct trapframe *tf = l->l_md.md_regs;
	int sig = ksi->ksi_signo, error;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sigframe_siginfo *fp, frame;
	int onstack;

	fp = getframe(l, sig, &onstack);
	--fp;

	frame.sf_si._info = ksi->ksi_info;
	frame.sf_uc.uc_link = l->l_ctxlink;
	frame.sf_uc.uc_sigmask = *mask;
	frame.sf_uc.uc_flags = _UC_SIGMASK;
	frame.sf_uc.uc_flags |= (l->l_sigstk.ss_flags & SS_ONSTACK)
		? _UC_SETSTACK : _UC_CLRSTACK;
	memset(&frame.sf_uc.uc_stack, 0, sizeof(frame.sf_uc.uc_stack));
	sendsig_reset(l, sig);
	mutex_exit(p->p_lock);
	cpu_getmcontext(l, &frame.sf_uc.uc_mcontext, &frame.sf_uc.uc_flags);
	error = copyout(&frame, fp, sizeof(frame));
	mutex_enter(p->p_lock);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	tf->tf_r4 = sig;		/* "signum" argument for handler */
	tf->tf_r5 = (int)&fp->sf_si;	/* "sip" argument for handler */
	tf->tf_r6 = (int)&fp->sf_uc;	/* "ucp" argument for handler */
 	tf->tf_spc = (int)catcher;
	tf->tf_r15 = (int)fp;
	tf->tf_pr = (int)ps->sa_sigdesc[sig].sd_tramp;

	/* Remember if we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	const struct trapframe *tf = l->l_md.md_regs;
	__greg_t *gr = mcp->__gregs;
	__greg_t ras_pc;

	/* Save register context. */
	gr[_REG_GBR]    = tf->tf_gbr;
	gr[_REG_PC]     = tf->tf_spc;
	gr[_REG_SR]     = tf->tf_ssr;
	gr[_REG_MACL]   = tf->tf_macl;
	gr[_REG_MACH]   = tf->tf_mach;
	gr[_REG_PR]     = tf->tf_pr;
	gr[_REG_R14]    = tf->tf_r14;
	gr[_REG_R13]    = tf->tf_r13;
	gr[_REG_R12]    = tf->tf_r12;
	gr[_REG_R11]    = tf->tf_r11;
	gr[_REG_R10]    = tf->tf_r10;
	gr[_REG_R9]     = tf->tf_r9;
	gr[_REG_R8]     = tf->tf_r8;
	gr[_REG_R7]     = tf->tf_r7;
	gr[_REG_R6]     = tf->tf_r6;
	gr[_REG_R5]     = tf->tf_r5;
	gr[_REG_R4]     = tf->tf_r4;
	gr[_REG_R3]     = tf->tf_r3;
	gr[_REG_R2]     = tf->tf_r2;
	gr[_REG_R1]     = tf->tf_r1;
	gr[_REG_R0]     = tf->tf_r0;
	gr[_REG_R15]    = tf->tf_r15;

	if ((ras_pc = (__greg_t)ras_lookup(l->l_proc,
	    (void *) gr[_REG_PC])) != -1)
		gr[_REG_PC] = ras_pc;

	*flags |= _UC_CPU;

	/* FPU context is currently not handled by the kernel. */
	memset(&mcp->__fpregs, 0, sizeof (mcp->__fpregs));
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe *tf = l->l_md.md_regs;
	const __greg_t *gr = mcp->__gregs;
	struct proc *p = l->l_proc;

	/* Restore register context, if any. */
	if ((flags & _UC_CPU) != 0) {
		/* Check for security violations. */
		if (((tf->tf_ssr ^ gr[_REG_SR]) & PSL_USERSTATIC) != 0)
			return (EINVAL);

		tf->tf_gbr    = gr[_REG_GBR];
		tf->tf_spc    = gr[_REG_PC];
		tf->tf_ssr    = gr[_REG_SR];
		tf->tf_macl   = gr[_REG_MACL];
		tf->tf_mach   = gr[_REG_MACH];
		tf->tf_pr     = gr[_REG_PR];
		tf->tf_r14    = gr[_REG_R14];
		tf->tf_r13    = gr[_REG_R13];
		tf->tf_r12    = gr[_REG_R12];
		tf->tf_r11    = gr[_REG_R11];
		tf->tf_r10    = gr[_REG_R10];
		tf->tf_r9     = gr[_REG_R9];
		tf->tf_r8     = gr[_REG_R8];
		tf->tf_r7     = gr[_REG_R7];
		tf->tf_r6     = gr[_REG_R6];
		tf->tf_r5     = gr[_REG_R5];
		tf->tf_r4     = gr[_REG_R4];
		tf->tf_r3     = gr[_REG_R3];
		tf->tf_r2     = gr[_REG_R2];
		tf->tf_r1     = gr[_REG_R1];
		tf->tf_r0     = gr[_REG_R0];
		tf->tf_r15    = gr[_REG_R15];

		lwp_setprivate(l, (void *)(uintptr_t)gr[_REG_GBR]);
	}

#if 0
	/* XXX: FPU context is currently not handled by the kernel. */
	if (flags & _UC_FPU) {
		/* TODO */;
	}
#endif

	mutex_enter(p->p_lock);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(p->p_lock);

	return (0);
}

/*
 * Clear registers on exec
 */
void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct trapframe *tf;

	l->l_md.md_flags &= ~(MDP_USEDFPU | MDP_SSTEP);

	tf = l->l_md.md_regs;

	tf->tf_ssr = PSL_USERSET;
	tf->tf_spc = pack->ep_entry;
	tf->tf_pr = 0;

	tf->tf_gbr = 0;
	tf->tf_macl = 0;
	tf->tf_mach = 0;

	tf->tf_r0 = 0;
	tf->tf_r1 = 0;
	tf->tf_r2 = 0;
	tf->tf_r3 = 0;
	tf->tf_r4 = fuword((void *)stack);	/* argc */
	tf->tf_r5 = stack + 4;			/* argv */
	tf->tf_r6 = stack + 4 * tf->tf_r4 + 8;	/* envp */
	tf->tf_r7 = 0;
	tf->tf_r8 = 0;
	tf->tf_r9 = l->l_proc->p_psstrp;
	tf->tf_r10 = 0;
	tf->tf_r11 = 0;
	tf->tf_r12 = 0;
	tf->tf_r13 = 0;
	tf->tf_r14 = 0;
	tf->tf_r15 = stack;
}

/*
 * Jump to reset vector.
 */
void
cpu_reset(void)
{

	_cpu_exception_suspend();
	_reg_write_4(SH_(EXPEVT), EXPEVT_RESET_MANUAL);

#ifndef __lint__
	goto *(void *)0xa0000000;
#endif
	/* NOTREACHED */
}

int
cpu_lwp_setprivate(lwp_t *l, void *addr)
{

	l->l_md.md_regs->tf_gbr = (int)addr;
	return 0;
}

/*
 *	u_intN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

uint8_t
bus_space_read_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return *(volatile uint8_t *)(bsh + offset);
}

uint16_t
bus_space_read_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return bswap16(*(volatile uint16_t *)(bsh + offset));
}

uint32_t
bus_space_read_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return bswap32(*(volatile uint32_t *)(bsh + offset));
}

uint16_t
bus_space_read_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return *(volatile uint16_t *)(bsh + offset);
}

uint32_t
bus_space_read_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return *(volatile uint32_t *)(bsh + offset);
}

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */
void
bus_space_read_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_1(tag, bsh, offset);
}

void
bus_space_read_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_2(tag, bsh, offset);
}

void
bus_space_read_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = bus_space_read_4(tag, bsh, offset);
}

void
bus_space_read_multi_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = *(volatile uint16_t *)(bsh + offset);
}

void
bus_space_read_multi_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{

	while (count--)
		*addr++ = *(volatile uint32_t *)(bsh + offset);
}

/*
 *	int bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart,
 *	    bus_addr_t rend, bus_size_t size, bus_size_t align,
 *	    bus_size_t boundary, int flags, bus_addr_t *addrp,
 *	    bus_space_handle_t *bshp);
 *
 * Allocate a region of bus space.
 */
int sh_memio_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t,
    bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);

#define	bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)			\
	sh_memio_alloc((t), (rs), (re), (s), (a), (b), (f), (ap), (hp))

/*
 *	int bus_space_free(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * Free a region of bus space.
 */
void sh_memio_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);

#define	bus_space_free(t, h, s)						\
	sh_memio_free((t), (h), (s))

/*
 *	int bus_space_unmap(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * Unmap a region of bus space.
 */
void sh_memio_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);

#define	bus_space_unmap(t, h, s)					\
	sh_memio_unmap((t), (h), (s))

/*
 *	int bus_space_subregion(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t offset, bus_size_t size,
 *	    bus_space_handle_t *nbshp);
 *
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */
int sh_memio_subregion(bus_space_tag_t, bus_space_handle_t,
    bus_size_t, bus_size_t, bus_space_handle_t *);

#define	bus_space_subregion(t, h, o, s, nhp)				\
	sh_memio_subregion((t), (h), (o), (s), (nhp))

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
void
bus_space_read_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{
	uint8_t *p = (uint8_t *)(bsh + offset);

	while (count--)
		*addr++ = *p++;
}

void
bus_space_read_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{
	uint16_t *p = (uint16_t *)(bsh + offset);

	while (count--)
		*addr++ = bswap16(*p++);
}

void
bus_space_read_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{
	uint32_t *p = (uint32_t *)(bsh + offset);

	while (count--)
		*addr++ = bswap32(*p++);
}

/*
 *	void bus_space_read_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
void
bus_space_read_region_stream_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{
	uint8_t *p = (uint8_t *)(bsh + offset);

	while (count--)
		*addr++ = *p++;
}

void
bus_space_read_region_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{
	uint16_t *p = (uint16_t *)(bsh + offset);

	while (count--)
		*addr++ = *p++;
}

void
bus_space_read_region_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{
	uint32_t *p = (uint32_t *)(bsh + offset);

	while (count--)
		*addr++ = *p++;
}

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */
void
bus_space_write_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{
	uint8_t *p = (uint8_t *)(bsh + offset);

	while (count--)
		*p++ = *addr++;
}

void
bus_space_write_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{
	uint16_t *p = (uint16_t *)(bsh + offset);

	while (count--)
		*p++ = bswap16(*addr++);
}

void
bus_space_write_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{
	uint32_t *p = (uint32_t *)(bsh + offset);

	while (count--)
		*p++ = bswap32(*addr++);
}

/*
 *	void bus_space_write_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */
void
bus_space_write_region_stream_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{
	uint8_t *p = (uint8_t *)(bsh + offset);

	while (count--)
		*p++ = *addr++;
}

void
bus_space_write_region_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{
	uint16_t *p = (uint16_t *)(bsh + offset);

	while (count--)
		*p++ = *addr++;
}

void
bus_space_write_region_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{
	uint32_t *p = (uint32_t *)(bsh + offset);

	while (count--)
		*p++ = *addr++;
}

/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */
void
bus_space_write_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value)
{

	*(volatile uint8_t *)(bsh + offset) = value;
}

void
bus_space_write_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value)
{

	*(volatile uint16_t *)(bsh + offset) = bswap16(value);
}

void
bus_space_write_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value)
{

	*(volatile uint32_t *)(bsh + offset) = bswap32(value);
}

void
bus_space_write_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value)
{

	*(volatile uint16_t *)(bsh + offset) = value;
}

void
bus_space_write_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value)
{

	*(volatile uint32_t *)(bsh + offset) = value;
}

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */
void
bus_space_write_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_1(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_2(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_4(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_stream_2(tag, bsh, offset, *addr++);
}

void
bus_space_write_multi_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{

	while (count--)
		bus_space_write_stream_4(tag, bsh, offset, *addr++);
}

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
void
bus_space_set_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, size_t count)
{

	while (count--)
		bus_space_write_1(tag, bsh, offset, val);
}

void
bus_space_set_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, size_t count)
{

	while (count--)
		bus_space_write_2(tag, bsh, offset, val);
}

void
bus_space_set_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, size_t count)
{

	while (count--)
		bus_space_write_4(tag, bsh, offset, val);
}

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
void
bus_space_set_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, size_t count)
{
	volatile uint8_t *addr = (void *)(bsh + offset);

	while (count--)
		*addr++ = val;
}

void
bus_space_set_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, size_t count)
{
	volatile uint16_t *addr = (void *)(bsh + offset);

	val = bswap16(val);
	while (count--)
		*addr++ = val;
}

void
bus_space_set_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, size_t count)
{
	volatile uint32_t *addr = (void *)(bsh + offset);

	val = bswap32(val);
	while (count--)
		*addr++ = val;
}

/*
 *	void bus_space_set_region_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
void
bus_space_set_region_stream_1(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{
	volatile uint8_t *addr = (void *)(bsh + offset);

	while (count--)
		*addr++ = val;
}

void
bus_space_set_region_stream_2(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count)
{
	volatile uint16_t *addr = (void *)(bsh + offset);

	while (count--)
		*addr++ = val;
}

void
bus_space_set_region_stream_4(bus_space_tag_t tag, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count)
{
	volatile uint32_t *addr = (void *)(bsh + offset);

	while (count--)
		*addr++ = val;
}

/*
 *	void bus_space_copy_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */
void
bus_space_copy_region_1(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, size_t c)
{
	volatile uint8_t *addr1 = (void *)(h1 + o1);
	volatile uint8_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (c--)
			*addr2++ = *addr1++;
	} else {		/* dest after src: copy backwards */
		addr1 += c - 1;
		addr2 += c - 1;
		while (c--)
			*addr2-- = *addr1--;
	}
}

void
bus_space_copy_region_2(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, size_t c)
{
	volatile uint16_t *addr1 = (void *)(h1 + o1);
	volatile uint16_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (c--)
			*addr2++ = *addr1++;
	} else {		/* dest after src: copy backwards */
		addr1 += c - 1;
		addr2 += c - 1;
		while (c--)
			*addr2-- = *addr1--;
	}
}

void
bus_space_copy_region_4(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, size_t c)
{
	volatile uint32_t *addr1 = (void *)(h1 + o1);
	volatile uint32_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (c--)
			*addr2++ = *addr1++;
	} else {		/* dest after src: copy backwards */
		addr1 += c - 1;
		addr2 += c - 1;
		while (c--)
			*addr2-- = *addr1--;
	}
}
