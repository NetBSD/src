/*	$NetBSD: trap.c,v 1.3 2002/07/07 22:52:54 fredette Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*	$OpenBSD: trap.c,v 1.30 2001/09/19 20:50:56 mickey Exp $	*/

/*
 * Copyright (c) 1998-2000 Michael Shalayeff
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* #define INTRDEBUG */
/* #define TRAPDEBUG */
/* #define USERTRACE */

#include "opt_kgdb.h"
#include "opt_syscall_debug.h"
#include "opt_ktrace.h"
#include "opt_systrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syscall.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/signal.h>
#include <sys/device.h>

#include <net/netisr.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <uvm/uvm.h>

#include <machine/iomod.h>
#include <machine/cpufunc.h>
#include <machine/reg.h>
#include <machine/autoconf.h>

#include <machine/db_machdep.h>

#include <hppa/hppa/machdep.h>

#if defined(INTRDEBUG) || defined(TRAPDEBUG)
#include <ddb/db_output.h>
#endif

#if defined(DEBUG) || defined(DIAGNOSTIC)
/*
 * 0x6fc1000 is a stwm r1, d(sr0, sp), which is the last
 * instruction in the function prologue that gcc -O0 uses.
 * When we have this instruction we know the relationship
 * between the stack pointer and the gcc -O0 frame pointer 
 * (in r3, loaded with the initial sp) for the body of a
 * function.
 *
 * If the given instruction is a stwm r1, d(sr0, sp) where
 * d > 0, we evaluate to d, else we evaluate to zero.
 */
#define STWM_R1_D_SR0_SP(inst) \
	(((inst) & 0xffffc001) == 0x6fc10000 ? (((inst) & 0x00003ff) >> 1) : 0)
#endif /* DEBUG || DIAGNOSTIC */

const char *trap_type[] = {
	"invalid",
	"HPMC",
	"power failure",
	"recovery counter",
	"external interrupt",
	"LPMC",
	"ITLB miss fault",
	"instruction protection",
	"Illegal instruction",
	"break instruction",
	"privileged operation",
	"privileged register",
	"overflow",
	"conditional",
	"assist exception",
	"DTLB miss",
	"ITLB non-access miss",
	"DTLB non-access miss",
	"data protection/rights/alignment",
	"data break",
	"TLB dirty",
	"page reference",
	"assist emulation",
	"higher-priv transfer",
	"lower-priv transfer",
	"taken branch",
	"data access rights",
	"data protection",
	"unaligned data ref",
};
int trap_types = sizeof(trap_type)/sizeof(trap_type[0]);

int want_resched;
volatile int astpending;

void pmap_hptdump __P((void));
void syscall __P((struct trapframe *frame, int *args));

#ifdef USERTRACE
/*
 * USERTRACE is a crude facility that traces the PC of
 * a single user process.  This tracing is normally
 * activated by the dispatching of a certain syscall
 * with certain arguments - see the activation code in
 * syscall().
 */
u_int rctr_next_iioq;
#endif

static __inline void
userret (struct proc *p, register_t pc, u_quad_t oticks)
{
	int sig;

	/* take pending signals */
	while ((sig = CURSIG(p)) != 0)
		postsig(sig);

	p->p_priority = p->p_usrpri;
	if (want_resched) {
		/*
		 * We're being preempted.
		 */
		preempt(NULL);
		while ((sig = CURSIG(p)) != 0)
			postsig(sig);
	}

	/*
	 * If profiling, charge recent system time to the trapped pc.
	 */
	if (p->p_flag & P_PROFIL) {
		extern int psratio;

		addupc_task(p, pc, (int)(p->p_sticks - oticks) * psratio);
	}

	curcpu()->ci_schedstate.spc_curpriority = p->p_priority;
}

/*
 * This handles some messy kernel debugger details.
 * It dispatches into either kgdb or DDB, and knows
 * about some special things to do, like skipping over
 * break instructions and how to really set up for
 * a single-step.
 */
#if defined(KGDB) || defined(DDB)
static int
trap_kdebug(int type, int code, struct trapframe *frame)
{
	int handled;
	u_int tf_iioq_head_old;
	u_int tf_iioq_tail_old;

	for(;;) {

		/* This trap has not been handled. */
		handled = 0;

		/* Remember the instruction offset queue. */
		tf_iioq_head_old = frame->tf_iioq_head;
		tf_iioq_tail_old = frame->tf_iioq_tail;

#ifdef	KGDB
		/* Let KGDB handle it (if connected) */
		if (!handled)
			handled = kgdb_trap(type, frame);
#endif
#ifdef	DDB
		/* Let DDB handle it. */
		if (!handled)
			handled = kdb_trap(type, code, frame);
#endif

		/* If this trap wasn't handled, return now. */
		if (!handled)
			return(0);

		/*
		 * If the instruction offset queue head changed,
		 * but the offset queue tail didn't, assume that
		 * the user wants to jump to the head offset, and
		 * adjust the tail accordingly.  This should fix 
		 * the kgdb `jump' command, and can help DDB users
		 * who `set' the offset head but forget the tail.
		 */
		if (frame->tf_iioq_head != tf_iioq_head_old &&
		    frame->tf_iioq_tail == tf_iioq_tail_old)
			frame->tf_iioq_tail = frame->tf_iioq_head + 4;

		/*
		 * This is some single-stepping support.
		 * If we're trying to step through a nullified
		 * instruction, just advance by hand and trap
		 * again.  Otherwise, load the recovery counter
		 * with zero.
		 */
		if (frame->tf_ipsw & PSW_R) {
#ifdef TRAPDEBUG
			printf("(single stepping at head 0x%x tail 0x%x)\n", frame->tf_iioq_head, frame->tf_iioq_tail);
#endif
			if (frame->tf_ipsw & PSW_N) {
#ifdef TRAPDEBUG
				printf("(single stepping past nullified)\n");
#endif

				/* Advance the program counter. */
				frame->tf_iioq_head = frame->tf_iioq_tail;
				frame->tf_iioq_tail = frame->tf_iioq_head + 4;

				/* Clear flags. */
				frame->tf_ipsw &= ~(PSW_N|PSW_X|PSW_Y|PSW_Z|PSW_B|PSW_T|PSW_H|PSW_L);

				/* Simulate another trap. */
				type = T_RECOVERY;
				continue;
			}
			frame->tf_rctr = 0;
		}
	
		/* We handled this trap. */
		return (1);
	}
	/* NOTREACHED */
}
#else	/* !KGDB && !DDB */
#define trap_kdebug(t, c, f)	(0)
#endif	/* !KGDB && !DDB */

#ifdef DIAGNOSTIC
/*
 * These functions give a crude usermode backtrace.  They 
 * really only work when code has been compiled without 
 * optimization, as they assume a certain function prologue 
 * sets up a frame pointer and stores the return pointer 
 * and arguments in it.
 */
static void user_backtrace_raw __P((u_int, u_int));
static void
user_backtrace_raw(u_int pc, u_int fp)
{
	int frame_number;
	int arg_number;

	for (frame_number = 0; 
	     frame_number < 100 && pc > HPPA_PC_PRIV_MASK && fp;
	     frame_number++) {

		printf("%3d: pc=%08x%s fp=0x%08x", frame_number, 
		    pc & ~HPPA_PC_PRIV_MASK, USERMODE(pc) ? "" : "**", fp);
		for(arg_number = 0; arg_number < 4; arg_number++)
			printf(" arg%d=0x%08x", arg_number,
			    (int) fuword(HPPA_FRAME_CARG(arg_number, fp)));
		printf("\n");
                pc = fuword(((register_t *) fp) - 5);	/* fetch rp */
		if (pc == -1) { 
			printf("  fuword for pc failed\n");
			break;
		}
                fp = fuword(((register_t *) fp) + 0);	/* fetch previous fp */
		if (fp == -1) { 
			printf("  fuword for fp failed\n");
			break;
		}
	}
	printf("  backtrace stopped with pc %08x fp 0x%08x\n", pc, fp);
}

static void user_backtrace __P((struct trapframe *, struct proc *, int));
static void
user_backtrace(struct trapframe *tf, struct proc *p, int type)
{
	u_int pc, fp, inst;

	/*
	 * Display any trap type that we have.
	 */
	if (type >= 0)
		printf("pid %d (%s) trap #%d\n", 
		    p->p_pid, p->p_comm, type & ~T_USER);

	/*
	 * Assuming that the frame pointer in r3 is valid,
	 * dump out a stack trace.
	 */
	fp = tf->tf_r3;
	printf("pid %d (%s) backtrace, starting with fp 0x%08x\n",
		p->p_pid, p->p_comm, fp);
	user_backtrace_raw(tf->tf_iioq_head, fp);

	/*
	 * In case the frame pointer in r3 is not valid,
	 * assuming the stack pointer is valid and the
	 * faulting function is a non-leaf, if we can
	 * find its prologue we can recover its frame
	 * pointer.
	 */
	pc = tf->tf_iioq_head;
	fp = tf->tf_sp - HPPA_FRAME_SIZE;
	printf("pid %d (%s) backtrace, starting with sp 0x%08x pc 0x%08x\n",
		p->p_pid, p->p_comm, tf->tf_sp, pc);
	for(pc &= ~HPPA_PC_PRIV_MASK; pc > 0; pc -= sizeof(inst)) {
		inst = fuword((register_t *) pc);
		if (inst == -1) {
			printf("  fuword for inst at pc %08x failed\n", pc);
			break;
		}
		/* Check for the prologue instruction that sets sp. */
		if (STWM_R1_D_SR0_SP(inst)) {
			fp = tf->tf_sp - STWM_R1_D_SR0_SP(inst);
			printf("  sp from fp at pc %08x: %08x\n", pc, inst);
			break;
		}
	}
	user_backtrace_raw(tf->tf_iioq_head, fp);
}
#endif /* DIAGNOSTIC */

#ifdef DEBUG
/*
 * This sanity-checks a trapframe.  It is full of various
 * assumptions about what a healthy CPU state should be,
 * with some documented elsewhere, some not.
 */
struct trapframe *sanity_frame;
struct proc *sanity_proc;
int sanity_checked = 0;
void frame_sanity_check __P((struct trapframe *, struct proc *));
void
frame_sanity_check(struct trapframe *tf, struct proc *p)
{
	extern int kernel_text;
	extern int etext;
	extern register_t kpsw;
	extern vaddr_t hpt_base;
	extern vsize_t hpt_mask;
	vsize_t uspace_size;
#define SANITY(e)					\
do {							\
	if (sanity_frame == NULL && !(e)) {		\
		sanity_frame = tf;			\
		sanity_proc = p;			\
		sanity_checked = __LINE__;		\
	}						\
} while (/* CONSTCOND */ 0)

	SANITY((tf->tf_ipsw & kpsw) == kpsw);
	SANITY(tf->tf_hptm == hpt_mask && tf->tf_vtop == hpt_base);
	SANITY((kpsw & PSW_I) == 0 || tf->tf_eiem != 0);
	if (tf->tf_iisq_head == HPPA_SID_KERNEL) {
		/*
		 * If the trap happened in the gateway
		 * page, we take the easy way out and 
		 * assume that the trapframe is okay.
		 */
		if ((tf->tf_iioq_head & ~PAGE_MASK) != SYSCALLGATE) {
			SANITY(!USERMODE(tf->tf_iioq_head));
			SANITY(!USERMODE(tf->tf_iioq_tail));
			SANITY(tf->tf_iioq_head >= (u_int) &kernel_text);
			SANITY(tf->tf_iioq_head < (u_int) &etext);
			SANITY(tf->tf_iioq_tail >= (u_int) &kernel_text);
			SANITY(tf->tf_iioq_tail < (u_int) &etext);
#ifdef HPPA_REDZONE
			uspace_size = HPPA_REDZONE;
#else
			uspace_size = USPACE;
#endif
			SANITY(p == NULL ||
				((tf->tf_sp >= (u_int)(p->p_addr) + NBPG &&
				  tf->tf_sp < (u_int)(p->p_addr) + uspace_size)));
		}
	} else {
		SANITY(USERMODE(tf->tf_iioq_head));
		SANITY(USERMODE(tf->tf_iioq_tail));
		SANITY(p != NULL && tf->tf_cr30 == kvtop((caddr_t)p->p_addr));
	}
#undef SANITY
	if (sanity_frame == tf) {
		trap_kdebug(T_IBREAK, 0, tf);
		sanity_frame = NULL;
		sanity_proc = NULL;
		sanity_checked = 0;
	}
}
#endif /* DEBUG */

void
trap(type, frame)
	int type;
	struct trapframe *frame;
{
	struct proc *p = curproc;
	struct pcb *pcbp;
	register vaddr_t va;
	register struct vm_map *map;
	struct vmspace *vm;
	register vm_prot_t vftype;
	register pa_space_t space;
	u_int opcode;
	int ret;
	const char *tts;
	int type_raw;
#ifdef DIAGNOSTIC
	extern int emergency_stack_start, emergency_stack_end;
#endif

	type_raw = type & ~T_USER;
	opcode = frame->tf_iir;
	if (type_raw == T_ITLBMISS || type_raw == T_ITLBMISSNA) {
		va = frame->tf_iioq_head;
		space = frame->tf_iisq_head;
		vftype = VM_PROT_READ;	/* XXX VM_PROT_EXECUTE ??? */
	} else {
		va = frame->tf_ior;
		space = frame->tf_isr;
		vftype = inst_store(opcode) ? VM_PROT_WRITE : VM_PROT_READ;
	}

#ifdef DIAGNOSTIC
	/*
	 * If we are on the emergency stack, then we either got
	 * a fault on the kernel stack, or we're just handling
	 * a trap for the machine check handler (which also 
	 * runs on the emergency stack).
	 *
	 * We *very crudely* differentiate between the two cases
	 * by checking the faulting instruction: if it is the
	 * function prologue instruction that stores the old
	 * frame pointer and updates the stack pointer, we assume
	 * that we faulted on the kernel stack.
	 *
	 * In this case, not completing that instruction will
	 * probably confuse backtraces in kgdb/ddb.  Completing
	 * it would be difficult, because we already faulted on
	 * that part of the stack, so instead we fix up the 
	 * frame as if the function called has just returned.
	 * This has peculiar knowledge about what values are in
	 * what registers during the "normal gcc -g" prologue.
	 */
	if (&type >= &emergency_stack_start &&
	    &type < &emergency_stack_end &&
	    type != T_IBREAK && STWM_R1_D_SR0_SP(opcode)) {
		/* Restore the caller's frame pointer. */
		frame->tf_r3 = frame->tf_r1;
		/* Restore the caller's instruction offsets. */
		frame->tf_iioq_head = frame->tf_rp;
		frame->tf_iioq_tail = frame->tf_iioq_head + 4;
		goto dead_end;
	}
#endif /* DIAGNOSTIC */
		
#ifdef DEBUG
	frame_sanity_check(frame, p);
#endif /* DEBUG */

	/* If this is a trap, not an interrupt, reenable interrupts. */
	if (type_raw != T_INTERRUPT)
		mtctl(frame->tf_eiem, CR_EIEM);

	if (frame->tf_flags & TFF_LAST)
		p->p_md.md_regs = frame;

	if ((type & ~T_USER) > trap_types)
		tts = "reserved";
	else
		tts = trap_type[type & ~T_USER];

#ifdef TRAPDEBUG
	if (type_raw != T_INTERRUPT && type_raw != T_IBREAK)
		printf("trap: %d, %s for %x:%x at %x:%x, fp=%p, rp=%x\n",
		    type, tts, space, (u_int)va, frame->tf_iisq_head,
		    frame->tf_iioq_head, frame, frame->tf_rp);
	else if (type_raw == T_IBREAK)
		printf("trap: break instruction %x:%x at %x:%x, fp=%p\n",
		    break5(opcode), break13(opcode),
		    frame->tf_iisq_head, frame->tf_iioq_head, frame);

	{
		extern int etext;
		if (frame < (struct trapframe *)&etext) {
			printf("trap: bogus frame ptr %p\n", frame);
			goto dead_end;
		}
	}
#endif
	switch (type) {
	case T_NONEXIST:
	case T_NONEXIST|T_USER:
#if !defined(DDB) && !defined(KGDB)
		/* we've got screwed up by the central scrutinizer */
		panic ("trap: elvis has just left the building!");
		break;
#else
		goto dead_end;
#endif
	case T_RECOVERY|T_USER:
#ifdef USERTRACE
		for(;;) {
			if (frame->tf_iioq_head != rctr_next_iioq)
				printf("-%08x\nr %08x",
					rctr_next_iioq - 4,
					frame->tf_iioq_head);
			rctr_next_iioq = frame->tf_iioq_head + 4;
			if (frame->tf_ipsw & PSW_N) {
				/* Advance the program counter. */
				frame->tf_iioq_head = frame->tf_iioq_tail;
				frame->tf_iioq_tail = frame->tf_iioq_head + 4;
				/* Clear flags. */
				frame->tf_ipsw &= ~(PSW_N|PSW_X|PSW_Y|PSW_Z|PSW_B|PSW_T|PSW_H|PSW_L);
				/* Simulate another trap. */
				continue;
			}
			break;
		}
		frame->tf_rctr = 0;
		break;
#endif /* USERTRACE */
	case T_RECOVERY:
#if !defined(DDB) && !defined(KGDB)
		/* XXX will implement later */
		printf ("trap: handicapped");
		break;
#else
		goto dead_end;
#endif

	case T_EMULATION | T_USER:
#ifdef FPEMUL
		hppa_fpu_emulate(frame, p);
#else  /* !FPEMUL */
		/*
		 * We don't have FPU emulation, so signal the
		 * process with a SIGFPE.
		 */
		trapsignal(p, SIGFPE, frame->tf_iioq_head);
#endif /* !FPEMUL */
		break;

#ifdef DIAGNOSTIC
	case T_EXCEPTION:
		panic("FPU/SFU emulation botch");

		/* these just can't happen ever */
	case T_PRIV_OP:
	case T_PRIV_REG:
		/* these just can't make it to the trap() ever */
	case T_HPMC:      case T_HPMC | T_USER:
	case T_EMULATION:
#endif
	case T_IBREAK:
	case T_DATALIGN:
	case T_DBREAK:
	dead_end:
		if (type & T_USER) {
#ifdef DEBUG
			user_backtrace(frame, p, type);
#endif
			trapsignal(p, SIGILL, frame->tf_iioq_head);
			break;
		}
		if (trap_kdebug(type, va, frame))
			return;
		else if (type == T_DATALIGN)
			panic ("trap: %s at 0x%x", tts, (u_int) va);
		else
			panic ("trap: no debugger for \"%s\" (%d)", tts, type);
		break;

	case T_IBREAK | T_USER:
	case T_DBREAK | T_USER:
		/* pass to user debugger */
		break;

	case T_EXCEPTION | T_USER:	/* co-proc assist trap */
		trapsignal(p, SIGFPE, va);
		break;

	case T_OVERFLOW | T_USER:
		trapsignal(p, SIGFPE, va);
		break;
		
	case T_CONDITION | T_USER:
		break;

	case T_ILLEGAL | T_USER:
#ifdef DEBUG
		user_backtrace(frame, p, type);
#endif
		trapsignal(p, SIGILL, va);
		break;

	case T_PRIV_OP | T_USER:
#ifdef DEBUG
		user_backtrace(frame, p, type);
#endif
		trapsignal(p, SIGILL, va);
		break;

	case T_PRIV_REG | T_USER:
#ifdef DEBUG
		user_backtrace(frame, p, type);
#endif
		trapsignal(p, SIGILL, va);
		break;

		/* these should never got here */
	case T_HIGHERPL | T_USER:
	case T_LOWERPL | T_USER:
		trapsignal(p, SIGSEGV, va);
		break;

	case T_IPROT | T_USER:
	case T_DPROT | T_USER:
		trapsignal(p, SIGSEGV, va);
		break;

	case T_DATACC:   	case T_USER | T_DATACC:
	case T_ITLBMISS:	case T_USER | T_ITLBMISS:
	case T_DTLBMISS:	case T_USER | T_DTLBMISS:
	case T_ITLBMISSNA:	case T_USER | T_ITLBMISSNA:
	case T_DTLBMISSNA:	case T_USER | T_DTLBMISSNA:
	case T_TLB_DIRTY:	case T_USER | T_TLB_DIRTY:
		va = hppa_trunc_page(va);
		vm = p->p_vmspace;

		if (!vm) {
#ifdef TRAPDEBUG
			printf("trap: no vm, p=%p\n", p);
#endif
			goto dead_end;
		}

		/*
		 * it could be a kernel map for exec_map faults
		 */
		if (!(type & T_USER) && space == HPPA_SID_KERNEL)
			map = kernel_map;
		else
			map = &vm->vm_map;

		if (map->pmap->pmap_space != space) {
#ifdef TRAPDEBUG
			printf("trap: space missmatch %d != %d\n",
			    space, map->pmap->pmap_space);
#endif
			/* actually dump the user, crap the kernel */
			goto dead_end;
		}

		/* Never call uvm_fault in interrupt context. */
		KASSERT(hppa_intr_depth == 0);

		ret = uvm_fault(map, va, 0, vftype);

#ifdef TRAPDEBUG
		printf("uvm_fault(%p, %x, %d, %d)=%d\n",
		    map, (u_int)va, 0, vftype, ret);
#endif

		/*
		 * If this was a stack access we keep track of the maximum
		 * accessed stack size.  Also, if uvm_fault gets a protection
		 * failure it is due to accessing the stack region outside
		 * the current limit and we need to reflect that as an access
		 * error.
		 */
		if (va >= (vaddr_t)vm->vm_maxsaddr + vm->vm_ssize) {
			if (ret == 0) {
				vsize_t nss = btoc(va - USRSTACK + NBPG);
				if (nss > vm->vm_ssize)
					vm->vm_ssize = nss;
			} else if (ret == EACCES)
				ret = EFAULT;
		}

		if (ret != 0) {
			if (type & T_USER) {
printf("trapsignal: uvm_fault(%p, %x, %d, %d)=%d\n",
	map, (u_int)va, 0, vftype, ret);
#ifdef DEBUG
				user_backtrace(frame, p, type);
#endif
				trapsignal(p, SIGSEGV, frame->tf_ior);
			} else {
				if (p && p->p_addr->u_pcb.pcb_onfault) {
#ifdef PMAPDEBUG
					printf("trap: copyin/out %d\n",ret);
#endif
					pcbp = &p->p_addr->u_pcb;
					frame->tf_iioq_tail = 4 +
					    (frame->tf_iioq_head =
						pcbp->pcb_onfault);
					pcbp->pcb_onfault = 0;
					break;
				}
#if 1
if (trap_kdebug (type, va, frame))
	return;
#else
				panic("trap: uvm_fault(%p, %x, %d, %d): %d",
				    map, va, 0, vftype, ret);
#endif
			}
		}
		break;

	case T_DATALIGN | T_USER:
#ifdef DEBUG
		user_backtrace(frame, p, type);
#endif
		trapsignal(p, SIGBUS, va);
		break;

	case T_INTERRUPT:
	case T_INTERRUPT|T_USER:
		hppa_intr(frame);
		mtctl(frame->tf_eiem, CR_EIEM);
#if 0
if (trap_kdebug (type, va, frame))
return;
#endif
		break;
	case T_LOWERPL:
	case T_DPROT:
	case T_IPROT:
	case T_OVERFLOW:
	case T_CONDITION:
	case T_ILLEGAL:
	case T_HIGHERPL:
	case T_TAKENBR:
	case T_POWERFAIL:
	case T_LPMC:
	case T_PAGEREF:
	case T_DATAPID:  	case T_DATAPID  | T_USER:
		if (0 /* T-chip */) {
			break;
		}
		/* FALLTHROUGH to unimplemented */
	default:
#if 1
if (trap_kdebug (type, va, frame))
	return;
#endif
		panic ("trap: unimplemented \'%s\' (%d)", tts, type);
	}

	if (type & T_USER)
		userret(p, p->p_md.md_regs->tf_iioq_head, 0);

#ifdef DEBUG
	frame_sanity_check(frame, p);
	if (frame->tf_flags & TFF_LAST && curproc != NULL)
		frame_sanity_check(curproc->p_md.md_regs, curproc);
#endif /* DEBUG */
}

void
child_return(arg)
	void *arg;
{
	struct proc *p = arg;

	userret(p, p->p_md.md_regs->tf_iioq_head, 0);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, SYS_fork, 0, 0);
#endif
#ifdef DEBUG
	frame_sanity_check(p->p_md.md_regs, p);
#endif /* DEBUG */
}

/*
 * call actual syscall routine
 * from the low-level syscall handler:
 * - all HPPA_FRAME_NARGS syscall's arguments supposed to be copied onto
 *   our stack, this wins compared to copyin just needed amount anyway
 * - register args are copied onto stack too
 */
void
syscall(frame, args)
	struct trapframe *frame;
	int *args;
{
	register struct proc *p;
	register const struct sysent *callp;
	int nsys, code, argsize, error;
	int tmp;
	int rval[2];

	uvmexp.syscalls++;

#ifdef DEBUG
	frame_sanity_check(frame, curproc);
#endif /* DEBUG */

	if (!USERMODE(frame->tf_iioq_head))
		panic("syscall");

	p = curproc;
	p->p_md.md_regs = frame;
	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;
	code = frame->tf_t1;

	/*
	 * Restarting a system call is touchy on the HPPA, 
	 * because syscall arguments are passed in registers 
	 * and the program counter of the syscall "point" 
	 * isn't easily divined.  
	 *
	 * We handle the first problem by assuming that we
	 * will have to restart this system call, so we
	 * stuff the first four words of the original arguments 
	 * back into the frame as arg0...arg3, which is where
	 * we found them in the first place.  Any further
	 * arguments are (still) on the user's stack and the 
	 * syscall code will fetch them from there (again).
	 *
	 * The program counter problem is addressed below.
	 */
	frame->tf_arg0 = args[0];
	frame->tf_arg1 = args[1];
	frame->tf_arg2 = args[2];
	frame->tf_arg3 = args[3];

	/*
	 * Some special handling for the syscall(2) and 
	 * __syscall(2) system calls.
	 */
	switch (code) {
	case SYS_syscall:
		code = *args;
		args += 1;
		break;
	case SYS___syscall:
		if (callp != sysent)
			break;
		/*
		 * NB: even though __syscall(2) takes a quad_t
		 * containing the system call number, because
		 * our argument copying word-swaps 64-bit arguments,
		 * the least significant word of that quad_t
		 * is the first word in the argument array.
		 */
		code = *args;
		args += 2;
	}

	/*
	 * Stacks growing from lower addresses to higher
	 * addresses are not really such a good idea, because
	 * it makes it impossible to overlay a struct on top
	 * of C stack arguments (the arguments appear in 
	 * reversed order).
	 *
	 * You can do the obvious thing (as locore.S does) and 
	 * copy argument words one by one, laying them out in 
	 * the "right" order in the destination buffer, but this 
	 * ends up word-swapping multi-word arguments (like off_t).
	 * 
	 * To compensate, we have some automatically-generated
	 * code that word-swaps these multi-word arguments.
	 * Right now the script that generates this code is
	 * in Perl, because I don't know awk.
	 *
	 * FIXME - this works only on native binaries and
	 * will probably screw up any and all emulation.
	 */
	switch (code) {
	/*
	 * BEGIN automatically generated
	 * by /home/fredette/project/hppa/makescargfix.pl
	 * do not edit!
	 */
	case SYS_pread:
		/*
		 * 	syscallarg(int) fd;
		 * 	syscallarg(void *) buf;
		 * 	syscallarg(size_t) nbyte;
		 * 	syscallarg(int) pad;
		 * 	syscallarg(off_t) offset;
		 */
		tmp = args[4];
		args[4] = args[4 + 1];
		args[4 + 1] = tmp;
		break;
	case SYS_pwrite:
		/*
		 * 	syscallarg(int) fd;
		 * 	syscallarg(const void *) buf;
		 * 	syscallarg(size_t) nbyte;
		 * 	syscallarg(int) pad;
		 * 	syscallarg(off_t) offset;
		 */
		tmp = args[4];
		args[4] = args[4 + 1];
		args[4 + 1] = tmp;
		break;
	case SYS_mmap:
		/*
		 * 	syscallarg(void *) addr;
		 * 	syscallarg(size_t) len;
		 * 	syscallarg(int) prot;
		 * 	syscallarg(int) flags;
		 * 	syscallarg(int) fd;
		 * 	syscallarg(long) pad;
		 * 	syscallarg(off_t) pos;
		 */
		tmp = args[6];
		args[6] = args[6 + 1];
		args[6 + 1] = tmp;
		break;
	case SYS_lseek:
		/*
		 * 	syscallarg(int) fd;
		 * 	syscallarg(int) pad;
		 * 	syscallarg(off_t) offset;
		 */
		tmp = args[2];
		args[2] = args[2 + 1];
		args[2 + 1] = tmp;
		break;
	case SYS_truncate:
		/*
		 * 	syscallarg(const char *) path;
		 * 	syscallarg(int) pad;
		 * 	syscallarg(off_t) length;
		 */
		tmp = args[2];
		args[2] = args[2 + 1];
		args[2 + 1] = tmp;
		break;
	case SYS_ftruncate:
		/*
		 * 	syscallarg(int) fd;
		 * 	syscallarg(int) pad;
		 * 	syscallarg(off_t) length;
		 */
		tmp = args[2];
		args[2] = args[2 + 1];
		args[2 + 1] = tmp;
		break;
	case SYS_preadv:
		/*
		 * 	syscallarg(int) fd;
		 * 	syscallarg(const struct iovec *) iovp;
		 * 	syscallarg(int) iovcnt;
		 * 	syscallarg(int) pad;
		 * 	syscallarg(off_t) offset;
		 */
		tmp = args[4];
		args[4] = args[4 + 1];
		args[4 + 1] = tmp;
		break;
	case SYS_pwritev:
		/*
		 * 	syscallarg(int) fd;
		 * 	syscallarg(const struct iovec *) iovp;
		 * 	syscallarg(int) iovcnt;
		 * 	syscallarg(int) pad;
		 * 	syscallarg(off_t) offset;
		 */
		tmp = args[4];
		args[4] = args[4 + 1];
		args[4 + 1] = tmp;
		break;
	default:
		break;
	/*
	 * END automatically generated
	 * by /home/fredette/project/hppa/makescargfix.pl
	 * do not edit!
	 */
	}

#ifdef USERTRACE
	if (0) {
		user_backtrace(frame, p, -1);
		frame->tf_ipsw |= PSW_R;
		frame->tf_rctr = 0;
		printf("r %08x", frame->tf_iioq_head);
		rctr_next_iioq = frame->tf_iioq_head + 4;
	}
#endif

	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;	/* bad syscall # */
	else
		callp += code;
	argsize = callp->sy_argsize;

	if ((error = trace_enter(p, code, args, rval)) != 0)
		goto bad;

	rval[0] = 0;
	rval[1] = 0;
	switch (error = (*callp->sy_call)(p, args, rval)) {
	case 0:
		p = curproc;			/* changes on exec() */
		frame = p->p_md.md_regs;
		frame->tf_ret0 = rval[0];
		frame->tf_ret1 = rval[1];
		frame->tf_t1 = 0;
		break;
	case ERESTART:
		/*
		 * Now we have to wind back the instruction
		 * offset queue to the point where the system
		 * call will be made again.  This is inherently
		 * tied to the SYSCALL macro.
		 *
		 * Currently, the part of the SYSCALL macro
		 * that we want to rerun reads as:
		 *
		 *	ldil	L%SYSCALLGATE, r1
		 *	ble	4(sr7, r1)
		 *	ldi	__CONCAT(SYS_,x), t1
		 *	ldw	HPPA_FRAME_ERP(sr0,sp), rp
		 *
		 * And our offset queue head points to the
		 * final ldw instruction.  So we need to 
		 * subtract twelve to reach the ldil.
		 */
		frame->tf_iioq_head -= 12;
		frame->tf_iioq_tail = frame->tf_iioq_head + 4;
		break;
	case EJUSTRETURN:
		p = curproc;
		break;
	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		frame->tf_t1 = error;
		break;
	}

	trace_exit(p, code, args, rval, error);

	userret(p, frame->tf_iioq_head, 0);
#ifdef DEBUG
	frame_sanity_check(frame, p);
#endif /* DEBUG */
}
