/* $NetBSD: cpu_x86_64.c,v 1.2.48.1 2018/05/21 04:36:02 pgoyette Exp $ */

/*-
 * Copyright (c) 2011 Reinoud Zandijk <reinoud@netbsd.org>
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_x86_64.c,v 1.2.48.1 2018/05/21 04:36:02 pgoyette Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/boot_flag.h>
#include <sys/ucontext.h>
#include <sys/utsname.h>
#include <machine/pcb.h>
#include <machine/psl.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <dev/mm.h>
#include <machine/machdep.h>
#include <machine/thunk.h>


#if 0
static void dump_regs(register_t *reg);;

static void
dump_regs(register_t *reg)
{
	int i;

	/* register dump before call */
	const char *name[] = {"RDI", "RSI", "RDX", "RCX", "R8", "R9", "R10",
		"R11", "R12", "R13", "R14", "R15", "RBP", "RAX",
		"GS", "FS", "ES", "DS", "TRAPNO", "ERR", "RIP", "CS",
		"RFLAGS", "RSP", "SS"};

	for (i =0; i < 26; i++)
		printf("reg[%02d] (%6s) = %"PRIx32"\n",
			i, name[i], (uint32_t) reg[i]);
}
#endif


/* from sys/arch/amd64/include/frame.h : KEEP IN SYNC */

/*
 * Signal frame
 */
struct sigframe_siginfo {
	uint64_t	sf_ra;		/* return address for handler */
	siginfo_t	sf_si;		/* actual saved siginfo */
	ucontext_t	sf_uc;		/* actual saved ucontext */
};


/* should be the same as i386 */
/*
 * mcontext extensions to handle signal delivery.
 */
#define _UC_SETSTACK	0x00010000
#define _UC_CLRSTACK	0x00020000
#define _UC_VM		0x00040000
#define	_UC_TLSBASE	0x00080000


void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct pcb *pcb = lwp_getpcb(l);
	struct sigacts *ps = p->p_sigacts;
	struct sigframe_siginfo *fp, frame;
	int sig = ksi->ksi_signo;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	ucontext_t *ucp;
	register_t *reg;
	int onstack, error;
	char *sp;

	KASSERT(mutex_owned(p->p_lock));

	ucp = &pcb->pcb_userret_ucp;
	reg = (register_t *) &ucp->uc_mcontext.__gregs;
#if 0
	thunk_printf("%s: ", __func__);
	thunk_printf("flags %d, ", (int) ksi->ksi_flags);
	thunk_printf("to lwp %d, signo %d, code %d, errno %d\n",
		(int) ksi->ksi_lid,
		ksi->ksi_signo,
		ksi->ksi_code,
		ksi->ksi_errno);
#endif

	/* do we need to jump onto the signal stack? */
	onstack = (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
	    && (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	sp = ((char *) reg[24] - 128);/* why -128?*//* RSP */
	if (onstack)
		sp = (char *) l->l_sigstk.ss_sp + l->l_sigstk.ss_size;

	sp -= sizeof(struct sigframe_siginfo);
	/*
	 * Round down the stackpointer to a multiple of 16 for
	 * fxsave and the ABI
	 */
	fp = (struct sigframe_siginfo *) (((unsigned long)sp & ~15) - 8);

	/* set up stack frame */
	frame.sf_ra = (uint64_t) ps->sa_sigdesc[sig].sd_tramp;
	frame.sf_si._info = ksi->ksi_info;

	/* copy our userret context into sf_uc */
	memcpy(&frame.sf_uc, ucp, sizeof(ucontext_t));
	frame.sf_uc.uc_sigmask = *mask;
	frame.sf_uc.uc_link = l->l_ctxlink;
	frame.sf_uc.uc_flags |= (l->l_sigstk.ss_flags & SS_ONSTACK)
	    ? _UC_SETSTACK : _UC_CLRSTACK;
	memset(&frame.sf_uc.uc_stack, 0, sizeof(frame.sf_uc.uc_stack));
	sendsig_reset(l, sig);

	/* copyout our frame to the stackframe */
	mutex_exit(p->p_lock);
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

	/* set catcher and the new stack pointer */
	reg[24] = (register_t) fp;		/* RSP */
	reg[21] = (register_t) catcher;		/* RIP */

	reg[ 0] = sig;				/* RDI */
	reg[ 1] = (uint64_t) &fp->sf_si;	/* RSI */
	reg[ 2] = (uint64_t) &fp->sf_uc;	/* RDX */
	reg[11] = reg[ 2];			/* R15 = RDX */

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp;
	register_t *reg;
	int i;

	/* set up the user context */
	ucp = &pcb->pcb_userret_ucp;
	reg = (register_t *) &ucp->uc_mcontext.__gregs;
	for (i = 0; i < 15; i++)
		reg[i] = 0;

	reg[13] = l->l_proc->p_psstrp;		/* RBX */
	reg[21] = pack->ep_entry;		/* RIP */
	reg[24] = (register_t) stack;		/* RSP */

	/* use given stack */
	ucp->uc_stack.ss_sp   = (void *) stack;
	ucp->uc_stack.ss_size = pack->ep_ssize;

	//dump_regs(reg);
}

void
md_syscall_get_syscallnumber(ucontext_t *ucp, uint32_t *code)
{
	register_t *reg = (register_t *) &ucp->uc_mcontext.__gregs;
	*code = reg[14];			/* RAX */
}

int
md_syscall_getargs(lwp_t *l, ucontext_t *ucp, int nargs, int argsize,
	register_t *args)
{
	register_t *reg = (register_t *) &ucp->uc_mcontext.__gregs;
	register_t *sp = (register_t *) reg[24];/* RSP */
	int ret;

	//dump_regs(reg);

	/*
	 * 1st 6 syscall args are passed in
	 *    rdi, rsi, rdx, r10, r8 and r9
	 */
	args[0] = reg[ 0];		/* RDI */
	args[1] = reg[ 1];		/* RSI */
	args[2] = reg[ 2];		/* RDX */
	args[3] = reg[ 6];		/* R10 (RCX got clobbered) */
	args[4] = reg[ 4];		/* R8  */
	args[5] = reg[ 5];		/* R9  */

	ret = 0;
	if (argsize > 6 * 8) {
		ret = copyin(sp + 1,
			args + 6, argsize - 6 * 8);
	}

	return ret;
}

void
md_syscall_set_returnargs(lwp_t *l, ucontext_t *ucp,
	int error, register_t *rval)
{
	register_t *reg = (register_t *) &ucp->uc_mcontext.__gregs;

	reg[23] &= ~PSL_C;		/* RFLAGS */
	if (error > 0) {
		rval[0] = error;
		reg[23] |= PSL_C;	/* RFLAGS */
	}

	/* set return parameters */
	reg[14]	= rval[0];		/* RAX */
	if (error == 0)
		reg[ 2] = rval[1];	/* RDX */

	//dump_regs(reg);
}

register_t
md_get_pc(ucontext_t *ucp)
{
	register_t *reg = (register_t *) &ucp->uc_mcontext.__gregs;

	return reg[21];			/* RIP */
}

register_t
md_get_sp(ucontext_t *ucp)
{
	register_t *reg = (register_t *) &ucp->uc_mcontext.__gregs;

	return reg[24];			/* RSP */
}

int
md_syscall_check_opcode(ucontext_t *ucp)
{
	uint32_t opcode;

	md_syscall_get_opcode(ucp, &opcode);

	switch (opcode) {
	case 0xff0f:	/* UD1      */
	case 0xff0b:	/* UD2      */
	case 0x80cd:	/* int $80  */
	case 0x340f:	/* sysenter */
	case 0x050f:	/* syscall */
		return 1;
	default:
		return 0;
	}
}


void
md_syscall_get_opcode(ucontext_t *ucp, uint32_t *opcode)
{
	register_t *reg = (register_t *) &ucp->uc_mcontext.__gregs;
//	uint8_t  *p8  = (uint8_t *) (reg[21]);
	uint16_t *p16 = (uint16_t*) (reg[21]);	/* RIP */

	switch (*p16) {
	case 0xff0f:	/* UD1      */
	case 0xff0b:	/* UD2      */
	case 0x80cd:	/* int $80  */
	case 0x340f:	/* sysenter */
	case 0x050f:	/* syscall */
		*opcode = *p16;
		break;
	default:
		*opcode = 0;
	}
}

void
md_syscall_inc_pc(ucontext_t *ucp, uint32_t opcode)
{
	register_t *reg = (register_t *) &ucp->uc_mcontext.__gregs;

	/* advance program counter */
	switch (opcode) {
	case 0xff0f:	/* UD1      */
	case 0xff0b:	/* UD2      */
	case 0x80cd:	/* int $80  */
	case 0x340f:	/* sysenter */
	case 0x050f:	/* syscall */
		reg[21] += 2;	/* RIP */
		break;
	default:
		panic("%s, unknown illegal instruction: opcode = %x\n",
			__func__, (uint32_t) opcode);
	}
}

void
md_syscall_dec_pc(ucontext_t *ucp, uint32_t opcode)
{
	register_t *reg = (register_t *) &ucp->uc_mcontext.__gregs;

	switch (opcode) {
	case 0xff0f:	/* UD1      */
	case 0xff0b:	/* UD2      */
	case 0x80cd:	/* int $80  */
	case 0x340f:	/* sysenter */
	case 0x050f:	/* syscall */
		reg[21] -= 2;	/* RIP */
		break;
	default:
		panic("%s, unknown illegal instruction: opcode = %x\n",
			__func__, (uint32_t) opcode);
	}
}

