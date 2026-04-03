/*	$NetBSD: m68k_trap.c,v 1.9 2026/04/03 14:59:55 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
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
 * from: Utah $Hdr: trap.c 1.32 91/04/06$
 *
 *	@(#)trap.c      7.15 (Berkeley) 8/2/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: m68k_trap.c,v 1.9 2026/04/03 14:59:55 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_m68k_arch.h"
#include "opt_fpu_emulate.h"
#include "opt_compat_sunos.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/userret.h>
#include <sys/kauth.h>
#include <sys/ras.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#ifdef DEBUG
#include <dev/cons.h>
#endif

#include <uvm/uvm_extern.h>

#include <m68k/cpu.h>
#include <m68k/cacheops.h>

#include <machine/trap.h>
#include <machine/cpu.h>
#include <machine/pcb.h>

#ifdef FPU_EMULATE
#include <m68k/fpe/fpu_emulate.h>
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef COMPAT_SUNOS
#include <compat/sunos/sunos_syscall.h>
#include <compat/sunos/sunos_exec.h>
#endif

void	trap(struct frame *, int, u_int, u_int);

volatile int astpending;

static const char * const trap_descriptions[] = {
	[T_BUSERR]	=	"Bus error",
	[T_ADDRERR]	=	"Address error",
	[T_ILLINST]	=	"Illegal instruction",
	[T_ZERODIV]	=	"Zero divide",
	[T_CHKINST]	=	"CHK instruction",
	[T_TRAPVINST]	=	"TRAPV instruction",
	[T_PRIVINST]	=	"Privilege violation",
	[T_TRACE]	=	"Trace trap",
	[T_MMUFLT]	=	"MMU fault",
	[T_FMTERR]	=	"Format error",
	[T_FPERR]	=	"Floating Point exception",
	[T_COPERR]	=	"Coprocessor violation",
	[T_ASTFLT]	=	"Async system trap",
	[T_BREAKPOINT]	=	"Breakpoint trap",
	[T_FPEMULI]	=	"FPU instruction",
	[T_FPEMULD]	=	"FPU data format",
};
static const unsigned int trap_description_count =
    __arraycount(trap_descriptions);

const char *
trap_desc(int type)
{
	static char typestr[sizeof("trap type XXXXXXXX")];

	if (type < 0) {
		return "stray trap";
	} else if (type >= trap_description_count ||
		   trap_descriptions[type] == NULL) {
		snprintf(typestr, sizeof(typestr), "trap type %d", type);
		return typestr;
	}
	return trap_descriptions[type];
}

/*
 * Size of various exception stack frames (minus the standard 8 bytes)
 */
const short exframesize[] = {
	FMT0SIZE,	/* type 0 - normal (68020/030/040/060) */
	FMT1SIZE,	/* type 1 - throwaway (68020/030/040) */
	FMT2SIZE,	/* type 2 - normal 6-word (68020/030/040/060) */
	FMT3SIZE,	/* type 3 - FP post-instruction (68040/060) */
	FMT4SIZE,	/* type 4 - access error/fp disabled (68060) */
	-1, -1,		/* type 5-6 - undefined */
	FMT7SIZE,	/* type 7 - access error (68040) */
	FMT8SIZE,	/* type 8 - bus fault (68010) */
	FMT9SIZE,	/* type 9 - coprocessor mid-instruction (68020/030) */
	FMTASIZE,	/* type A - short bus fault (68020/030) */
	FMTBSIZE,	/* type B - long bus fault (68020/030) */
	-1, -1, -1, -1	/* type C-F - undefined */
};

#if defined(M68010)
#if defined(M68020) || defined(M68030) || defined(M68040) || defined(M68060)
/*
 * We actually could make this "work" by forcing the cputype check
 * in the 68020/68030 case to be unsigned, but there are other very
 * practical reasons why we can't mix them anyway, so why bother?
 */
#error Cannnot mix 68010 with 68020+
#endif

#define	KDFAULT(c)	(((c) & (SSW1_IF|SSW1_FCMASK)) == (FC_SUPERD))
#define	WRFAULT(c)	(((c) & (SSW1_IF|SSW1_DF|SSW1_RW)) == (0))

#else /* ! M68010 */

#ifdef M68060
#if defined(M68020) || defined(M68030) || defined(M68040)
#define	CPU_IS_60	(cputype == CPU_68060)
#else
#define	CPU_IS_60	1
#endif
#else
#define	CPU_IS_60	0
#endif

#ifdef M68040
#if defined(M68020) || defined(M68030) || defined(M68060)
#define	CPU_IS_40	(cputype == CPU_68040)
#else
#define	CPU_IS_40	1
#endif
#else
#define	CPU_IS_40	0
#endif

#if defined(M68020) || defined(M68030)
#if defined(M68040) || defined(M68060)
#define	CPU_IS_2030	(cputype <= CPU_68030)
#else
#define	CPU_IS_2030	1
#endif
#else
#define	CPU_IS_2030	0
#endif

#define	KDFAULT_60(c)	(CPU_IS_60 && ((c) & FSLW_TM_SV))
#define	WRFAULT_60(c)	(CPU_IS_60 && ((c) & FSLW_RW_W))

#define	KDFAULT_40(c)	(CPU_IS_40 && \
			 ((c) & SSW4_TMMASK) == SSW4_TMKD)
#define	WRFAULT_40(c)	(CPU_IS_40 && \
			 ((c) & (SSW4_LK|SSW4_RW)) != SSW4_RW)

#define	KDFAULT_2030(c)	(CPU_IS_2030 && \
			 ((c) & (SSW_DF|SSW_FCMASK)) == (SSW_DF|FC_SUPERD))
#define	WRFAULT_2030(c)	(CPU_IS_2030 && \
			 (((c) & SSW_DF) != 0 && \
			 ((((c) & SSW_RW) == 0) || (((c) & SSW_RM) != 0))))

#define	KDFAULT(c)	(KDFAULT_60(c) || KDFAULT_40(c) || KDFAULT_2030(c))
#define	WRFAULT(c)	(WRFAULT_60(c) || WRFAULT_40(c) || WRFAULT_2030(c))

#endif /* M68010 */

#ifdef DEBUG
int mmudebug = 0;
int mmupid = -1;
#define MDB_ISPID(pid)	((pid) == mmupid)
#define MDB_FOLLOW	1
#define MDB_WBFOLLOW	2
#define MDB_WBFAILED	4
#define MDB_CPFAULT	8
#endif

#ifdef M68040
#ifdef DEBUG
struct writebackstats {
	int calls;
	int cpushes;
	int move16s;
	int wb1s, wb2s, wb3s;
	int wbsize[4];
} wbstats;

static const char *f7sz[] = { "longword", "byte", "word", "line" };
static const char *f7tt[] = { "normal", "MOVE16", "AFC", "ACK" };
static const char *f7tm[] = { "d-push", "u-data", "u-code", "M-data",
		 "M-code", "k-data", "k-code", "RES" };
static const char wberrstr[] =
    "WARNING: pid %d(%s) writeback [%s] failed, pc=%x fa=%x wba=%x wbd=%x\n";

static void
dumpssw(u_short ssw)
{
	printf(" SSW: %x: ", ssw);
	if (ssw & SSW4_CP)
		printf("CP,");
	if (ssw & SSW4_CU)
		printf("CU,");
	if (ssw & SSW4_CT)
		printf("CT,");
	if (ssw & SSW4_CM)
		printf("CM,");
	if (ssw & SSW4_MA)
		printf("MA,");
	if (ssw & SSW4_ATC)
		printf("ATC,");
	if (ssw & SSW4_LK)
		printf("LK,");
	if (ssw & SSW4_RW)
		printf("RW,");
	printf(" SZ=%s, TT=%s, TM=%s\n",
	       f7sz[(ssw & SSW4_SZMASK) >> 5],
	       f7tt[(ssw & SSW4_TTMASK) >> 3],
	       f7tm[ssw & SSW4_TMMASK]);
}

static void
dumpwb(int num, u_short s, u_int a, u_int d)
{
	struct proc *p = curproc;
	paddr_t pa;

	printf(" writeback #%d: VA %x, data %x, SZ=%s, TT=%s, TM=%s\n",
	       num, a, d, f7sz[(s & SSW4_SZMASK) >> 5],
	       f7tt[(s & SSW4_TTMASK) >> 3], f7tm[s & SSW4_TMMASK]);
	printf("               PA ");
	if (pmap_extract(p->p_vmspace->vm_map.pmap, (vaddr_t)a, &pa) == false)
		printf("<invalid address>");
	else {
		u_long val;
		if (ufetch_long((void *)a, &val) != 0)
			val = (u_long)-1;
		printf("%lx, current value %lx", pa, val);
	}
	printf("\n");
}
#endif /* DEBUG  */

/* Because calling memcpy() for 16 bytes is *way* too much overhead ... */
static inline void
fastcopy16(u_int *dst, const u_int *src)
{
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst = *src;
}

extern int suline(void *, void *);

int
m68040_writeback(struct frame *fp, int docachepush)
{
	struct fmt7 *f = &fp->f_fmt7;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct pcb *pcb = lwp_getpcb(l);
	int err = 0;
	u_int fa = 0;
	void *oonfault = pcb->pcb_onfault;
	paddr_t pa;

#ifdef DEBUG
	if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid)) {
		printf(" pid=%d, fa=%x,", p->p_pid, f->f_fa);
		dumpssw(f->f_ssw);
	}
	wbstats.calls++;
#endif
	/*
	 * Deal with special cases first.
	 */
	if ((f->f_ssw & SSW4_TMMASK) == SSW4_TMDCP) {
		/*
		 * Dcache push fault.
		 * Line-align the address and write out the push data to
		 * the indicated physical address.
		 */
#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid)) {
			printf(" pushing %s to PA %x, data %x",
			       f7sz[(f->f_ssw & SSW4_SZMASK) >> 5],
			       f->f_fa, f->f_pd0);
			if ((f->f_ssw & SSW4_SZMASK) == SSW4_SZLN)
				printf("/%x/%x/%x",
				       f->f_pd1, f->f_pd2, f->f_pd3);
			printf("\n");
		}
		if (f->f_wb1s & SSW4_WBSV)
			panic("writeback: cache push with WB1S valid");
		wbstats.cpushes++;
#endif
		/*
		 * XXX there are security problems if we attempt to do a
		 * cache push after a signal handler has been called.
		 */
		if (docachepush) {
			pmap_enter(pmap_kernel(), (vaddr_t)vmmap,
			    trunc_page(f->f_fa), VM_PROT_WRITE,
			    VM_PROT_WRITE|PMAP_WIRED);
			pmap_update(pmap_kernel());
			fa = (u_int)&vmmap[m68k_page_offset(f->f_fa) & ~0xF];
			fastcopy16((void *)fa, (void *)&f->f_pd0);
			(void) pmap_extract(pmap_kernel(), (vaddr_t)fa, &pa);
			DCFL(pa);
			pmap_remove(pmap_kernel(), (vaddr_t)vmmap,
				    (vaddr_t)&vmmap[PAGE_SIZE]);
			pmap_update(pmap_kernel());
		} else
			printf("WARNING: pid %d(%s) uid %d: CPUSH not done\n",
			       p->p_pid, p->p_comm, kauth_cred_geteuid(l->l_cred));
	} else if ((f->f_ssw & (SSW4_RW|SSW4_TTMASK)) == SSW4_TTM16) {
		/*
		 * MOVE16 fault.
		 * Line-align the address and write out the push data to
		 * the indicated virtual address.
		 */
#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid))
			printf(" MOVE16 to VA %x(%x), data %x/%x/%x/%x\n",
			       f->f_fa, f->f_fa & ~0xF, f->f_pd0, f->f_pd1,
			       f->f_pd2, f->f_pd3);
		if (f->f_wb1s & SSW4_WBSV)
			panic("writeback: MOVE16 with WB1S valid");
		wbstats.move16s++;
#endif
		if (KDFAULT(f->f_wb1s))
			fastcopy16((void *)(f->f_fa & ~0xF), (void *)&f->f_pd0);
		else
			err = suline((void *)(f->f_fa & ~0xF), (void *)&f->f_pd0);
		if (err) {
			fa = f->f_fa & ~0xF;
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED)
				printf(wberrstr, p->p_pid, p->p_comm,
				       "MOVE16", fp->f_pc, f->f_fa,
				       f->f_fa & ~0xF, f->f_pd0);
#endif
		}
	} else if (f->f_wb1s & SSW4_WBSV) {
		/*
		 * Writeback #1.
		 * Position the "memory-aligned" data and write it out.
		 */
		u_int wb1d = f->f_wb1d;
		int off;

#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid))
			dumpwb(1, f->f_wb1s, f->f_wb1a, f->f_wb1d);
		wbstats.wb1s++;
		wbstats.wbsize[(f->f_wb2s&SSW4_SZMASK)>>5]++;
#endif
		off = (f->f_wb1a & 3) * 8;
		switch (f->f_wb1s & SSW4_SZMASK) {
		case SSW4_SZLW:
			if (off)
				wb1d = (wb1d >> (32 - off)) | (wb1d << off);
			if (KDFAULT(f->f_wb1s))
				*(long *)f->f_wb1a = wb1d;
			else
				err = ustore_long((void *)f->f_wb1a, wb1d);
			break;
		case SSW4_SZB:
			off = 24 - off;
			if (off)
				wb1d >>= off;
			if (KDFAULT(f->f_wb1s))
				*(char *)f->f_wb1a = wb1d;
			else
				err = ustore_char((void *)f->f_wb1a, wb1d);
			break;
		case SSW4_SZW:
			off = (off + 16) % 32;
			if (off)
				wb1d = (wb1d >> (32 - off)) | (wb1d << off);
			if (KDFAULT(f->f_wb1s))
				*(short *)f->f_wb1a = wb1d;
			else
				err = ustore_short((void *)f->f_wb1a, wb1d);
			break;
		}
		if (err) {
			fa = f->f_wb1a;
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED)
				printf(wberrstr, p->p_pid, p->p_comm,
				       "#1", fp->f_pc, f->f_fa,
				       f->f_wb1a, f->f_wb1d);
#endif
		}
	}
	/*
	 * Deal with the "normal" writebacks.
	 *
	 * XXX writeback2 is known to reflect a LINE size writeback after
	 * a MOVE16 was already dealt with above.  Ignore it.
	 */
	if (err == 0 && (f->f_wb2s & SSW4_WBSV) &&
	    (f->f_wb2s & SSW4_SZMASK) != SSW4_SZLN) {
#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid))
			dumpwb(2, f->f_wb2s, f->f_wb2a, f->f_wb2d);
		wbstats.wb2s++;
		wbstats.wbsize[(f->f_wb2s&SSW4_SZMASK)>>5]++;
#endif
		switch (f->f_wb2s & SSW4_SZMASK) {
		case SSW4_SZLW:
			if (KDFAULT(f->f_wb2s))
				*(long *)f->f_wb2a = f->f_wb2d;
			else
				err = ustore_long((void *)f->f_wb2a, f->f_wb2d);
			break;
		case SSW4_SZB:
			if (KDFAULT(f->f_wb2s))
				*(char *)f->f_wb2a = f->f_wb2d;
			else
				err = ustore_char((void *)f->f_wb2a, f->f_wb2d);
			break;
		case SSW4_SZW:
			if (KDFAULT(f->f_wb2s))
				*(short *)f->f_wb2a = f->f_wb2d;
			else
				err = ustore_short((void *)f->f_wb2a, f->f_wb2d);
			break;
		}
		if (err) {
			fa = f->f_wb2a;
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED) {
				printf(wberrstr, p->p_pid, p->p_comm,
				       "#2", fp->f_pc, f->f_fa,
				       f->f_wb2a, f->f_wb2d);
				dumpssw(f->f_ssw);
				dumpwb(2, f->f_wb2s, f->f_wb2a, f->f_wb2d);
			}
#endif
		}
	}
	if (err == 0 && (f->f_wb3s & SSW4_WBSV)) {
#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid))
			dumpwb(3, f->f_wb3s, f->f_wb3a, f->f_wb3d);
		wbstats.wb3s++;
		wbstats.wbsize[(f->f_wb3s&SSW4_SZMASK)>>5]++;
#endif
		switch (f->f_wb3s & SSW4_SZMASK) {
		case SSW4_SZLW:
			if (KDFAULT(f->f_wb3s))
				*(long *)f->f_wb3a = f->f_wb3d;
			else
				err = ustore_long((void *)f->f_wb3a, f->f_wb3d);
			break;
		case SSW4_SZB:
			if (KDFAULT(f->f_wb3s))
				*(char *)f->f_wb3a = f->f_wb3d;
			else
				err = ustore_char((void *)f->f_wb3a, f->f_wb3d);
			break;
		case SSW4_SZW:
			if (KDFAULT(f->f_wb3s))
				*(short *)f->f_wb3a = f->f_wb3d;
			else
				err = ustore_short((void *)f->f_wb3a, f->f_wb3d);
			break;
#ifdef DEBUG
		case SSW4_SZLN:
			panic("writeback: wb3s indicates LINE write");
#endif
		}
		if (err) {
			fa = f->f_wb3a;
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED)
				printf(wberrstr, p->p_pid, p->p_comm,
				       "#3", fp->f_pc, f->f_fa,
				       f->f_wb3a, f->f_wb3d);
#endif
		}
	}
	pcb->pcb_onfault = oonfault;
	if (err)
		err = SIGSEGV;
	return err;
}
#endif /* M68040 */

/*
 * This is called by the trap15 and trace trap handlers for
 * supervisor-mode trace and breakpoint traps.  This is separate
 * from trap() so that breakpoints in trap() will work.
 *
 * If we have both DDB and KGDB, let KGDB see it first, because
 * KGDB will just return 0 if not connected.
 *
 * A fallback hook for machine-specific handling is also provided.
 */
void
trap_kdebug(int type, struct trapframe *tf)
{
#ifdef KGDB
	/* Let KGDB handle it (if connected) */
	if (kgdb_trap(type, tf)) {
		return;
	}
#endif
#ifdef DDB
	/* Let DDB handle it. */
	if (kdb_trap(type, tf)) {
		return;
	}
#endif
#ifdef MACHINE_KDEBUG_TRAP_FALLBACK
	MACHINE_KDEBUG_TRAP_FALLBACK(type, tf);
#else
	panic("%s trap", type == -1 ? "stray" : "unexpected BPT");
#endif
}

void	straytrap(struct trapframe);	/* called from badtrap[] */

void
straytrap(struct trapframe tf)
{
	printf("unexpected trap format=%d vector=0x%x pc=0x%x\n",
	    tf.tf_format, tf.tf_vector, tf.tf_pc);

	trap_kdebug(-1, &tf);
}

/*
 * trap and syscall both need the following work done before returning
 * to user mode.
 *
 * The 68040 thing here is ugly, but it's intended to optimize it away
 * on platforms that wouldn't otherwise need to pay the penalty.
 */
#ifdef M68040
#define	USERRET(l, fp, ot, fa, ft)	userret0((l), (fp), (ot), (fa), (ft))
#else
#define	USERRET(l, fp, ot, fa, ft)	userret0((l), (fp), (ot))
#endif

static void
userret0(struct lwp *l, struct frame *fp, u_quad_t oticks
#ifdef M68040
    , u_int faultaddr, int fromtrap
#endif
      )
{
	struct proc *p = l->l_proc;
#ifdef M68040
	int sig;
	int beenhere = 0;

 again:
#endif

	/* Invoke MI userret code */
	mi_userret(l);

	/*
	 * If profiling, charge system time to the trapped pc.
	 */
	if (p->p_stflag & PST_PROFIL) {
		extern int psratio;

		addupc_task(l, fp->f_pc,
		    (int)(p->p_sticks - oticks) * psratio);
	}

#ifdef M68040
	/*
	 * Deal with user mode writebacks (from trap, or from sigreturn).
	 * If any writeback fails, go back and attempt signal delivery.
	 * unless we have already been here and attempted the writeback
	 * (e.g. bad address with user ignoring SIGSEGV).  In that case
	 * we just return to the user without successfully completing
	 * the writebacks.  Maybe we should just drop the sucker?
	 */
	if (CPU_IS_40 && fp->f_format == FMT7) {
		if (beenhere) {
#ifdef DEBUG
			if (mmudebug & MDB_WBFAILED) {
				printf(fromtrap ?
		"pid %d(%s): writeback aborted, pc=%x, fa=%x\n" :
		"pid %d(%s): writeback aborted in sigreturn, pc=%x\n",
				    p->p_pid, p->p_comm, fp->f_pc, faultaddr);
			}
#endif
		} else if ((sig = m68040_writeback(fp, fromtrap))) {
			ksiginfo_t ksi;
			beenhere = 1;
			oticks = p->p_sticks;
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = sig;
			ksi.ksi_addr = (void *)faultaddr;
			ksi.ksi_code = BUS_OBJERR;
			trapsignal(l, &ksi);
			goto again;
		}
	}
#endif /* M68040 */
}

void
userret(struct lwp *l, struct frame *f, u_quad_t t)
{
	USERRET(l, f, t, 0, 0);
}

/*
 * trap() is called by way of the trap stubs (fault[] and faultstkadj[])
 * to handle most types of processor traps, including events such as
 * ASTs.
 *
 * System calls have their own fast path.
 */
void
trap(struct frame *fp, int type, unsigned int code, unsigned int v)
{
	struct lwp *l;
	struct proc *p;
	struct pcb *pcb;
	void *onfault;
	ksiginfo_t ksi;
	int s;
	int rv;
	u_quad_t sticks;
	static int panicking __diagused;

	curcpu()->ci_data.cpu_ntrap++;
	l = curlwp;
	p = l->l_proc;
	pcb = lwp_getpcb(l);
	onfault = pcb->pcb_onfault;

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_trap = type & ~T_USER;

	if (USERMODE(fp->f_sr)) {
		type |= T_USER;
		sticks = p->p_sticks;
		l->l_md.md_regs = fp->f_regs;
	} else {
		sticks = 0;
		/* XXX Detect trap recursion? */
	}

	switch (type) {
	default:
	dopanic:
		/*
		 * Let the kernel debugger see the trap frame that
		 * caused us to panic.  This is a convenience so
		 * one can see registers at the point of failure.
		 */
		s = splhigh();
		panicking = 1;
		printf("trap type=0x%x, code=0x%x, v=0x%x\n", type, code, v);
		printf("%s pc=0x%x\n",
		    (type & T_USER) ? "user" : "kernel", fp->f_pc);
		rv = 0;
#ifdef KGDB
		/* If connected, step or cont returns 1 */
		rv = kgdb_trap(type, (db_regs_t *)fp));
#endif
#ifdef DDB
		if (rv == 0) {
			(void)kdb_trap(type, (db_regs_t *)fp);
		}
#endif
		splx(s);
		if (panicstr) {
			printf("trap during panic!\n");
#ifdef DEBUG
			/* XXX should be a machine-dependent hook */
			printf("(press a key)\n");
			cnpollc(true);
			(void)cngetc();
			cnpollc(false);
#endif
		}
		regdump((struct trapframe *)fp, 128);
		type &= ~T_USER;
		panic("%s", trap_desc(type));
		/* NOTREACHED */
		break;

	case T_BUSERR:		/* kernel bus error */
		if (onfault == NULL) {
			goto dopanic;
		}
		rv = EFAULT;
		/* FALLTHROUGH */

	copyfault:
		/*
		 * If we have arranged to catch this fault in any of the
		 * copy to/from user space routines, set PC to return to
		 * indicated location and set flag informing buserror code
		 * that it may need to clean up stack frame.
		 */
		fp->f_stackadj = exframesize[fp->f_format];
		fp->f_format = fp->f_vector = 0;
		fp->f_pc = (int)onfault;
		fp->f_regs[D0] = rv;
		goto done;

	case T_BUSERR|T_USER:	/* bus error */
	case T_ADDRERR|T_USER:	/* address error */
		ksi.ksi_addr = (void *)v;
		ksi.ksi_signo = SIGBUS;
		ksi.ksi_code = (type == (T_BUSERR|T_USER)) ?
		    BUS_OBJERR : BUS_ADRERR;
		break;

	case T_COPERR:		/* kernel coprocessor violation */
	case T_FMTERR|T_USER:	/* do all RTE errors come in as T_USER? */
	case T_FMTERR:		/* ...just in case... */
	/*
	 * The user has most likely trashed the RTE or FP state info
	 * in the stack frame of a signal handler.
	 */
		printf("pid %d: kernel %s exception\n", p->p_pid,
		    type==T_COPERR ? "coprocessor" : "format");
		type |= T_USER;

		mutex_enter(p->p_lock);
		SIGACTION(p, SIGILL).sa_handler = SIG_DFL;
		sigdelset(&p->p_sigctx.ps_sigignore, SIGILL);
		sigdelset(&p->p_sigctx.ps_sigcatch, SIGILL);
		sigdelset(&l->l_sigmask, SIGILL);
		mutex_exit(p->p_lock);

		ksi.ksi_signo = SIGILL;
		ksi.ksi_addr = (void *)(int)fp->f_format;
				/* XXX was ILL_RESAD_FAULT */
		ksi.ksi_code = (type == T_COPERR) ?
		    ILL_COPROC : ILL_ILLOPC;
		break;

	case T_COPERR|T_USER:	/* user coprocessor violation */
		/* What is a proper response here? */
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_code = FPE_FLTINV;
		break;

	case T_FPERR|T_USER:	/* 68881 exceptions */
	/*
	 * We pass along the 68881 status register which the trap stub
	 * helpfully stashed in the code argument slot for us.
	 */
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_code = fpsr2siginfocode(code);
		break;

	/*
	 * FPU faults in supervisor mode.
	 */
	case T_ILLINST:	/* fnop generates this, apparently. */
	case T_FPEMULI:
	case T_FPEMULD:
	    {
		extern label_t *nofault;

		/* Check to see if we're probing for the FPU. */
		if (nofault) {
			longjmp(nofault);
		}
		if (type == T_ILLINST) {
			printf("Kernel Illegal Instruction trap.\n");
		} else {
			printf("Kernel FPU trap.\n");
		}
		goto dopanic;
	    }

	case T_FPEMULI|T_USER:	/* unimplemented FP instruction */
	case T_FPEMULD|T_USER:	/* unimplemented FP data type */
		switch (fputype) {
		case FPU_NONE:
#ifdef FPU_EMULATE
			if (fpu_emulate(fp, &pcb->pcb_fpregs, &ksi) == 0) {
				/* XXX Deal with tracing? (f_sr & PSL_T) */;
			}
#else
			uprintf("pid %d killed: no floating point support\n",
			    p->p_pid);
			goto illegal_instruction;
#endif
			break;
#if defined(M68040) || defined(M68060)
		case FPU_68040:
		case FPU_68060:
			/* XXX need to FSAVE */
			printf("pid %d(%s): "
			       "unimplemented FP %s at 0x%x (EA 0x%x)\n",
			    p->p_pid, p->p_comm,
			    fp->f_format == 2 ? "instruction" : "data type",
			    fp->f_pc, fp->f_fmt2.f_iaddr);
			/* XXX need to FRESTORE */
			ksi.ksi_addr = (void *)(int)fp->f_format;
			ksi.ksi_signo = SIGFPE;
			ksi.ksi_code = FPE_FLTINV;
			break;
#endif
		default:
			goto illegal_instruction;
		}
		break;

	case T_ILLINST|T_USER:	/* illegal instruction fault */
	case T_PRIVINST|T_USER:	/* privileged instruction fault */
	illegal_instruction:
		ksi.ksi_addr = (void *)(int)fp->f_format;
				/* XXX was ILL_PRIVIN_FAULT */
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = (type == (T_PRIVINST|T_USER)) ?
		    ILL_PRVOPC : ILL_ILLOPC;
		break;

	case T_ZERODIV|T_USER:	/* Integer divide by zero */
		ksi.ksi_addr = (void *)(int)fp->f_format;
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_code = FPE_INTDIV;
		break;

	case T_CHKINST|T_USER:	/* CHK instruction trap */
		ksi.ksi_addr = (void *)(int)fp->f_format;
				/* XXX was FPE_SUBRNG_TRAP */
		ksi.ksi_signo = SIGFPE;
		break;

	case T_TRAPVINST|T_USER: /* TRAPV instruction trap */
		ksi.ksi_addr = (void *)(int)fp->f_format;
				/* XXX was FPE_INTOVF_TRAP */
		ksi.ksi_signo = SIGFPE;
		break;

	/*
	 * XXX Trace traps are a nightmare.
	 *
	 *	HP-UX uses trap #1 for breakpoints,
	 *	NetBSD/m68k uses trap #2,
	 *	SUN 3.x uses trap #15,
	 *	DDB and KGDB uses trap #15 (for kernel breakpoints;
	 *	handled elsewhere).
	 *
	 * NetBSD and HP-UX traps both get mapped by the trap stubs into
	 * T_TRACE.  SUN 3.x traps get passed through as T_TRAP15 and are
	 * not really supported yet.
	 *
	 * XXX We should never get kernel-mode T_TRAP15 because the
	 * XXX trap stubs now give them special treatment.
	 */
	case T_TRAP15:		/* kernel breakpoint */
#ifdef DEBUG
		printf("unexpected kernel trace trap, type = %d\n", type);
		printf("program counter = 0x%x\n", fp->f_pc);
#endif
		fp->f_sr &= ~PSL_T;
		goto done;

	case T_TRACE|T_USER:	/* user trace trap */
#ifdef COMPAT_SUNOS
		/*
		 * SunOS uses Trap #2 for a "CPU cache flush".
		 * Just flush the on-chip caches and return.
		 */
		if (p->p_emul == &emul_sunos) {
#ifndef __mc68010__
			ICIA();
			DCIU();
#endif /* ! __mc68010__ */
			/* get out fast */
			goto done;
		}
#endif
		/* FALLTHROUGH */
	case T_TRACE:		/* tracing a trap instruction */
	case T_TRAP15|T_USER:	/* SUN user trace trap */
		/*
		 * Don't go stepping into a RAS.
		 */
		if (p->p_raslist != NULL &&
		    (ras_lookup(p, (void *)fp->f_pc) != (void *)-1)) {
			goto out;
		}
		fp->f_sr &= ~PSL_T;
		ksi.ksi_addr = (void *)fp->f_pc;
		ksi.ksi_signo = SIGTRAP;
		if (type == (T_TRAP15|T_USER)) {
			ksi.ksi_code = TRAP_BRKPT;
		} else {
			ksi.ksi_code = TRAP_TRACE;
		}
		break;

	case T_ASTFLT:		/* system async trap, cannot happen */
		goto dopanic;

	case T_ASTFLT|T_USER:	/* user async trap */
		astpending = 0;
		if (l->l_pflag & LP_OWEUPC) {
			l->l_pflag &= ~LP_OWEUPC;
			ADDUPROF(l);
		}
		goto out;

	case T_MMUFLT:		/* kernel mode page fault */
		/* Hacks to avoid calling VM code from debugger. */
#ifdef DDB
		if (db_recover != 0) {
			goto dopanic;
		}
#endif
#ifdef KGDB
		if (kgdb_recover != 0) {
			goto dopanic;
		}
#endif
		/* FALLTHROUGH */

	case T_MMUFLT|T_USER:	/* page fault */
	    {
	    	vaddr_t va;
		struct vmspace *vm = p->p_vmspace;
		struct vm_map *map;
		vm_prot_t ftype;
		extern struct vm_map *kernel_map;

		onfault = pcb->pcb_onfault;
#ifdef DEBUG
		if ((mmudebug & MDB_WBFOLLOW) || MDB_ISPID(p->p_pid)) {
			printf("trap: T_MMUFLT "
			       "pid=%d, code=0x%x, v=0x%x, pc=0x%x, sr=0x%x\n",
			    p->p_pid, code, v, fp->f_pc, fp->f_sr);
		}
#endif
	    	/*
		 * It is only a kernel address space fault iff:
		 *	1. (type & T_USER) == 0  and
		 *	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space data fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
	    	if ((type & T_USER) == 0 &&
		    (onfault == NULL || KDFAULT(code))) {
			map = kernel_map;
		} else {
			/* XXX KASSERT(vm != NULL)? */
			map = vm ? &vm->vm_map : kernel_map;
		}

		if (WRFAULT(code)) {
			ftype = VM_PROT_WRITE;
		} else {
			ftype = VM_PROT_READ;
		}

		va = trunc_page((vaddr_t)v);

#if 0	/* XXX keep this around? */
		/* NULL pointer check. */
		if (map == kernel_map && va == 0 && onfault == 0) {
			printf("trap: bad kernel %s access at 0x%x\n",
			    (ftype & VM_PROT_WRITE) ? "read/write" :
			    "read", v);
			goto dopanic;
		}
#endif

#ifdef DIAGNOSTIC
		if (intr_depth && !panicking) {
			printf("trap: calling pmap_fault() from interrupt!\n");
			goto dopanic;
		}
#endif

		pcb->pcb_onfault = NULL;
		rv = pmap_fault(map, va, ftype);
		pcb->pcb_onfault = onfault;
#ifdef DEBUG
		if (rv && MDB_ISPID(p->p_pid)) {
			printf("pmap_fault(%p, 0x%lx, 0x%x) -> 0x%x\n",
			    map, va, ftype, rv);
		}
#endif
		/*
		 * If this was a stack access we keep track of the maximum
		 * accessed stack size.  Also, if vm_fault gets a protection
		 * failure it is due to accessing the stack region outside
		 * the current limit and we need to reflect that as an access
		 * error.
		 */
	    	if (rv == 0) {
			if (map != kernel_map &&
			    (void *)va >= vm->vm_maxsaddr) {
				uvm_grow(p, va);
			}
			if (type == T_MMUFLT) {
#ifdef M68040
				if (CPU_IS_40) {
					(void)m68040_writeback(fp, 1);
				}
#endif
				goto done;
			}
			goto out;
		}
		/* vvv redundant with the switch() below? vvv */
		if (rv == EACCES) {
			ksi.ksi_code = SEGV_ACCERR;
			rv = EFAULT;
		} else {
			ksi.ksi_code = SEGV_MAPERR;
		}
		/* ^^^                                    ^^^ */
		if (type == T_MMUFLT) {
			if (onfault) {
#ifdef DEBUG
				if (mmudebug & MDB_CPFAULT) {
					printf("trap: copyfault pcb_onfault\n");
					Debugger();
				}
#endif
				goto copyfault;
			}
			printf("pmap_fault(%p, 0x%lx, 0x%x) -> 0x%x\n",
			    map, va, ftype, rv);
			printf("  type 0x%x, code [mmu,,ssw]: 0x%x\n",
			    type, code);
			goto dopanic;
		}
		ksi.ksi_addr = (void *)v;
		switch (rv) {
		case ENOMEM:
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			    p->p_pid, p->p_comm,
			    l->l_cred ?
			    kauth_cred_geteuid(l->l_cred) : -1);
			ksi.ksi_signo = SIGKILL;
			break;
		case EINVAL:
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_code = BUS_ADRERR;
			break;
		case EACCES:
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_ACCERR;
			break;
		default:
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_MAPERR;
			break;
		}
		break;
	    }
	}
	if (ksi.ksi_signo) {
		trapsignal(l, &ksi);
	}
	if (type & T_USER) {
 out:		USERRET(l, fp, sticks, v, 1);
	}

 done:
 	/* XXX Detect trap recursion? */;
}
