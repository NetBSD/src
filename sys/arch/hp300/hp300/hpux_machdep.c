/*	$NetBSD: hpux_machdep.c,v 1.37 2003/09/22 14:35:58 cl Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1990, 1993
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */
/*
 * Copyright (c) 1988 University of Utah.
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
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

/*
 * Machinde-dependent bits for HP-UX binary compatibility.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpux_machdep.c,v 1.37 2003/09/22 14:35:58 cl Exp $");                                                  

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/poll.h> 
#include <sys/proc.h> 
#include <sys/ptrace.h>
#include <sys/signalvar.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/wait.h> 

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/vmparam.h>
#include <m68k/cacheops.h>

#include <uvm/uvm_extern.h>

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/hpux/hpux.h>
#include <compat/hpux/hpux_sig.h>
#include <compat/hpux/hpux_util.h>
#include <compat/hpux/hpux_syscall.h>
#include <compat/hpux/hpux_syscallargs.h>

#include <machine/hpux_machdep.h>

extern	short exframesize[];

struct valtostr {
	int	val;
	const char *str;
};

/*
 * 6.0 and later context.
 * XXX what are the HP-UX "localroot" semantics?  Should we handle
 * XXX diskless systems here?
 */
static const char context_040[] =
    "standalone HP-MC68040 HP-MC68881 HP-MC68020 HP-MC68010 localroot default";

static const char context_fpu[] =
    "standalone HP-MC68881 HP-MC68020 HP-MC68010 localroot default";

static const char context_nofpu[] =
    "standalone HP-MC68020 HP-MC68010 localroot default";

static struct valtostr context_table[] = {
	{ FPU_68060,	&context_040[0] },
	{ FPU_68040,	&context_040[0] },
	{ FPU_68882,	&context_fpu[0] },
	{ FPU_68881,	&context_fpu[0] },
	{ 0, NULL },
};

#define UOFF(f)		((int)&((struct user *)0)->f)
#define HPUOFF(f)	((int)&((struct hpux_user *)0)->f)

/* simplified FP structure */
struct bsdfp {
	int save[54];
	int reg[24];
	int ctrl[3];
};

/*
 * m68k-specific setup for HP-UX executables.
 */
int
hpux_cpu_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	/* struct hpux_exec *hpux_ep = epp->ep_hdr; */

	/* set up command for exec header */
	NEW_VMCMD(&epp->ep_vmcmds, hpux_cpu_vmcmd,
	    sizeof(struct hpux_exec), (long)epp->ep_hdr, NULLVP, 0, 0);
	return (0);
}

/*
 * We need to stash the exec header in the pcb, so we define
 * this vmcmd to do it for us, since vmcmds are executed once
 * we're committed to the exec (i.e. the old program has been unmapped).
 *
 * The address of the header is in ev->ev_addr and the length is
 * in ev->ev_len.
 */
int
hpux_cpu_vmcmd(p, ev)
	struct proc *p;
	struct exec_vmcmd *ev;
{
	struct hpux_exec *execp = (struct hpux_exec *)ev->ev_addr;

#if 0 /* XXX - unable to handle HPUX coredumps */
	/* Make sure we have room. */
	if (ev->ev_len <= sizeof(p->p_addr->u_md.md_exec))
		memcpy(p->p_addr->u_md.md_exec, (caddr_t)ev->ev_addr,
		    ev->ev_len);
#endif

	/* Deal with misc. HP-UX process attributes. */
	if (execp->ha_trsize & HPUXM_VALID) {
		if (execp->ha_trsize & HPUXM_DATAWT)
			p->p_md.mdp_flags &= ~MDP_CCBDATA;

		if (execp->ha_trsize & HPUXM_STKWT)
			p->p_md.mdp_flags &= ~MDP_CCBSTACK;
	}

	return (0);
}

/*
 * Return arch-type for hpux_sys_sysconf()
 */
int
hpux_cpu_sysconf_arch()
{

	switch (cputype) {
	case CPU_68020:
		return (HPUX_SYSCONF_CPUM020);

	case CPU_68030:
		return (HPUX_SYSCONF_CPUM030);

	default:
		return (HPUX_SYSCONF_CPUM040);
	}
	/* NOTREACHED */
}

/*
 * HP-UX advise(2) system call.
 */
int
hpux_sys_advise(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_advise_args *uap = v;
	int error = 0;

	switch (SCARG(uap, arg)) {
	case 0:
		l->l_proc->p_md.mdp_flags |= MDP_HPUXMMAP; 
		break;

	case 1:
		ICIA();
		break;

	case 2:
		DCIA();
		break;

	default:
		error = EINVAL;
		break;
	}

	return (error);
}

/*
 * HP-UX getcontext(2) system call.
 * Man page lies, behaviour here is based on observed behaviour.
 */
int
hpux_sys_getcontext(lp, v, retval)
	struct lwp *lp; 
	void *v;
	register_t *retval; 
{
	struct hpux_sys_getcontext_args *uap = v;
	const char *str;
	int l, i, error = 0;
	int len; 

	if (SCARG(uap, len) <= 0)
		return (EINVAL);

	for (i = 0; context_table[i].str != NULL; i++)
		if (context_table[i].val == fputype)
			break;
	if (context_table[i].str == NULL)
		str = &context_nofpu[0];
	else
		str = context_table[i].str;

	/* + 1 ... count the terminating \0. */
	l = strlen(str) + 1;
	len = min(SCARG(uap, len), l);

	if (len)
		error = copyout(str, SCARG(uap, buf), len);
	if (error == 0)
		*retval = l;
	return (0);
}

#if 0 /* XXX This really, really doesn't work anymore. --scottr */
/*
 * Brutal hack!  Map HP-UX u-area offsets into BSD k-stack offsets.
 * XXX This probably doesn't work anymore, BTW.  --thorpej
 */
int
hpux_to_bsd_uoff(off, isps, l)
	int *off, *isps; 
	struct lwp *l;
{
	int *ar0 = l->l_md.md_regs;
	struct hpux_fp *hp; 
	struct bsdfp *bp;
	u_int raddr;

	*isps = 0;

	/* u_ar0 field; procxmt puts in U_ar0 */
	if ((int)off == HPUOFF(hpuxu_ar0))
		return(UOFF(U_ar0)); 

	if (fputype) {
		/* FP registers from PCB */
		hp = (struct hpux_fp *)HPUOFF(hpuxu_fp);
		bp = (struct bsdfp *)UOFF(u_pcb.pcb_fpregs);

		if (off >= hp->hpfp_ctrl && off < &hp->hpfp_ctrl[3])
			return((int)&bp->ctrl[off - hp->hpfp_ctrl]);

		if (off >= hp->hpfp_reg && off < &hp->hpfp_reg[24])
			return((int)&bp->reg[off - hp->hpfp_reg]);
	}

	/*
	 * Everything else we recognize comes from the kernel stack,
	 * so we convert off to an absolute address (if not already)
	 * for simplicity.
	 */
	if (off < (int *)ctob(UPAGES))
		off = (int *)((u_int)off + (u_int)l->l_addr);	/* XXX */

	/*
	 * General registers.
	 * We know that the HP-UX registers are in the same order as ours.
	 * The only difference is that their PS is 2 bytes instead of a
	 * padded 4 like ours throwing the alignment off.
	 */
	if (off >= ar0 && off < &ar0[18]) {
		/*
		 * PS: return low word and high word of PC as HP-UX would
		 * (e.g. &u.u_ar0[16.5]).
		 *
		 * XXX we don't do this since HP-UX adb doesn't rely on
		 * it and passing such an offset to procxmt will cause
		 * it to fail anyway.  Instead, we just set the offset
		 * to PS and let hpux_ptrace() shift up the value returned.
		 */
		if (off == &ar0[PS]) {
#if 0
			raddr = (u_int) &((short *)ar0)[PS*2+1];
#else
			raddr = (u_int) &ar0[(int)(off - ar0)];
#endif
			*isps = 1;
		}
		/*
		 * PC: off will be &u.u_ar0[16.5] since HP-UX saved PS
		 * is only 16 bits.
		 */
		else if (off == (int *)&(((short *)ar0)[PS*2+1]))
			raddr = (u_int) &ar0[PC];
		/*
		 * D0-D7, A0-A7: easy
		 */
		else
			raddr = (u_int) &ar0[(int)(off - ar0)];
		return((int)(raddr - (u_int)l->l_addr));	/* XXX */
	}

	/* everything else */
	return (-1);
}
#endif

#define	HSS_RTEFRAME	0x01
#define	HSS_FPSTATE	0x02
#define	HSS_USERREGS	0x04

struct hpuxsigstate {
	int	hss_flags;		/* which of the following are valid */
	struct	frame hss_frame;	/* original exception frame */
	struct	fpframe hss_fpstate;	/* 68881/68882 state info */
};

/*
 * WARNING: code in locore.s assumes the layout shown here for hsf_signum
 * thru hsf_handler so... don't screw with them!
 */
struct hpuxsigframe {
	int	hsf_signum;		   /* signo for handler */
	int	hsf_code;		   /* additional info for handler */
	struct	hpuxsigcontext *hsf_scp;   /* context ptr for handler */
	sig_t	hsf_handler;		   /* handler addr for u_sigc */
	struct	hpuxsigstate hsf_sigstate; /* state of the hardware */
	struct	hpuxsigcontext hsf_sc;	   /* actual context */
};

#ifdef DEBUG
int hpuxsigdebug = 0;
int hpuxsigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

/*
 * Send an interrupt to process.
 */
void
hpux_sendsig(ksiginfo_t *ksi, sigset_t *mask)
{
	u_long code = ksi->ksi_trap;
	int sig = ksi->ksi_signo;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	int onstack;
	struct hpuxsigframe *fp = getframe(l, sig, &onstack), kf;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	short ft = frame->f_format;

	fp--;

#ifdef DEBUG
	if ((hpuxsigdebug & SDB_KSTACK) && p->p_pid == hpuxsigpid)
		printf("hpux_sendsig(%d): sig %d ssp %p usp %p scp %p ft %d\n",
		       p->p_pid, sig, &onstack, fp, &fp->hsf_sc, ft);
#endif

	/* Build the stack frame for the signal trampoline. */
	kf.hsf_signum = bsdtohpuxsig(sig);
	kf.hsf_code = code;
	kf.hsf_scp = &fp->hsf_sc;
	kf.hsf_handler = catcher;

	/*
	 * Save necessary hardware state.  Currently this includes:
	 *	- general registers
	 *	- original exception frame (if not a "normal" frame)
	 *	- FP coprocessor state
	 */
	kf.hsf_sigstate.hss_flags = HSS_USERREGS;
	memcpy(kf.hsf_sigstate.hss_frame.f_regs, frame->f_regs,
	    sizeof(frame->f_regs));
	if (ft >= FMT4) {
#ifdef DEBUG
		if (ft > 15 || exframesize[ft] < 0)
			panic("hpux_sendsig: bogus frame type");
#endif
		kf.hsf_sigstate.hss_flags |= HSS_RTEFRAME;
		kf.hsf_sigstate.hss_frame.f_format = frame->f_format;
		kf.hsf_sigstate.hss_frame.f_vector = frame->f_vector;
		memcpy(&kf.hsf_sigstate.hss_frame.F_u, &frame->F_u,
		    exframesize[ft]);
		/*
		 * Leave an indicator that we need to clean up the kernel
		 * stack.  We do this by setting the "pad word" above the
		 * hardware stack frame to the amount the stack must be
		 * adjusted by.
		 *
		 * N.B. we increment rather than just set f_stackadj in
		 * case we are called from syscall when processing a
		 * sigreturn.  In that case, f_stackadj may be non-zero.
		 */
		frame->f_stackadj += exframesize[ft];
		frame->f_format = frame->f_vector = 0;
#ifdef DEBUG
		if (hpuxsigdebug & SDB_FOLLOW)
			printf("hpux_sendsig(%d): copy out %d of frame %d\n",
			       p->p_pid, exframesize[ft], ft);
#endif
	}

	if (fputype) {
		kf.hsf_sigstate.hss_flags |= HSS_FPSTATE;
		m68881_save(&kf.hsf_sigstate.hss_fpstate);
	}
#ifdef DEBUG
	if ((hpuxsigdebug & SDB_FPSTATE) &&
	    *(char *)&kf.hsf_sigstate.hss_fpstate)
		printf("hpux_sendsig(%d): copy out FP state (%x) to %p\n",
		       p->p_pid, *(u_int *)&kf.hsf_sigstate.hss_fpstate,
		       &kf.hsf_sigstate.hss_fpstate);
#endif

	/* Build the signal context to be used by hpux_sigreturn. */
	kf.hsf_sc.hsc_syscall	= 0;		/* XXX */
	kf.hsf_sc.hsc_action	= 0;		/* XXX */
	kf.hsf_sc.hsc_pad1	= kf.hsf_sc.hsc_pad2 = 0;
	kf.hsf_sc.hsc_sp	= frame->f_regs[SP];
	kf.hsf_sc.hsc_ps	= frame->f_sr;
	kf.hsf_sc.hsc_pc	= frame->f_pc;

	/* Save the signal stack. */
	kf.hsf_sc.hsc_onstack	= p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	bsdtohpuxmask(mask, &kf.hsf_sc.hsc_mask);

	/* How amazingly convenient! */
	kf.hsf_sc._hsc_pad	= 0;
	kf.hsf_sc._hsc_ap	= (int)&fp->hsf_sigstate;

	if (copyout(&kf, fp, sizeof(kf))) {
#ifdef DEBUG
		if ((hpuxsigdebug & SDB_KSTACK) && p->p_pid == hpuxsigpid)
			printf("hpux_sendsig(%d): copyout failed on sig %d\n",
			    p->p_pid, sig);
#endif
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}
#ifdef DEBUG
	if (hpuxsigdebug & SDB_FOLLOW) {
		printf(
		  "hpux_sendsig(%d): sig %d scp %p fp %p sc_sp %x sc_ap %x\n",
		   p->p_pid, sig, kf.hsf_scp, fp,
		   kf.hsf_sc.hsc_sp, kf.hsf_sc._hsc_ap);
	}
#endif

	buildcontext(l, p->p_sigctx.ps_sigcode, fp);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((hpuxsigdebug & SDB_KSTACK) && p->p_pid == hpuxsigpid)
		printf("hpux_sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
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
/* ARGSUSED */
int
hpux_sys_sigreturn(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_sigreturn_args /* {
		syscallarg(struct hpuxsigcontext *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct hpuxsigcontext *scp;
	struct frame *frame;
	struct hpuxsigcontext tsigc;
	struct hpuxsigstate tstate;
	int rf, flags;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
#ifdef DEBUG
	if (hpuxsigdebug & SDB_FOLLOW)
		printf("sigreturn: pid %d, scp %p\n", p->p_pid, scp);
#endif
	if ((int)scp & 1)
		return (EINVAL);

	if (copyin(scp, &tsigc, sizeof(tsigc)))
		return (EFAULT);
	scp = &tsigc;

	/* Make sure the user isn't pulling a fast one on us! */
	if ((scp->hsc_ps & (PSL_MBZ|PSL_IPL|PSL_S)) != 0)
		return (EINVAL);

	/* Restore register context. */
	frame = (struct frame *) l->l_md.md_regs;

	/*
	 * Grab pointer tohardware state information.  We have
	 * Squirreled this away at the end of the context structure,
	 * and HP-UX programs don't know about it.
	 */
	if ((rf = scp->_hsc_ap) == 0)
		goto restore;

	/*
	 * See if there is anything to do before we go to the
	 * expense of copying in close to 1/2K of data
	 */
	flags = fuword((caddr_t)rf);
#ifdef DEBUG
	if (hpuxsigdebug & SDB_FOLLOW)
		printf("hpux_sigreturn(%d): sc_ap %x flags %x\n",
		       p->p_pid, rf, flags);
#endif
	/*
	 * fuword failed (bogus _hsc_ap value).
	 */
	if (flags == -1)
		return (EINVAL);

	if (flags == 0 || copyin((caddr_t)rf, (caddr_t)&tstate, sizeof tstate))
		goto restore;
#ifdef DEBUG
	if ((hpuxsigdebug & SDB_KSTACK) && p->p_pid == hpuxsigpid)
		printf("hpux_sigreturn(%d): ssp %p usp %x scp %p ft %d\n",
		       p->p_pid, &flags, scp->hsc_sp, SCARG(uap, sigcntxp),
		       (flags & HSS_RTEFRAME) ? tstate.hss_frame.f_format : -1);
#endif
	/*
	 * Restore long stack frames.  Note that we do not copy
	 * back the saved SR or PC, they were picked up above from
	 * the sigcontext structure.
	 */
	if (flags & HSS_RTEFRAME) {
		int sz;
		
		/* grab frame type and validate */
		sz = tstate.hss_frame.f_format;
		if (sz > 15 || (sz = exframesize[sz]) < 0 ||
		    frame->f_stackadj < sz)
			return (EINVAL);
		frame->f_stackadj -= sz;
		frame->f_format = tstate.hss_frame.f_format;
		frame->f_vector = tstate.hss_frame.f_vector;
		memcpy(&frame->F_u, &tstate.hss_frame.F_u, sz);
#ifdef DEBUG
		if (hpuxsigdebug & SDB_FOLLOW)
			printf("sigreturn(%d): copy in %d of frame type %d\n",
			       p->p_pid, sz, tstate.hss_frame.f_format);
#endif
	}

	/*
	 * Restore most of the users registers except for A6 and SP
	 * which were handled above.
	 */
	if (flags & HSS_USERREGS)
		memcpy(frame->f_regs, tstate.hss_frame.f_regs,
		    sizeof(frame->f_regs) - (2 * NBPW));

	/*
	 * Finally we restore the original FP context
	 */
	if (flags & HSS_FPSTATE)
		m68881_restore(&tstate.hss_fpstate);

 restore:
	/*
	 * Restore the user supplied information.
	 */

	frame->f_regs[SP] = scp->hsc_sp;
	frame->f_pc = scp->hsc_pc;
	frame->f_sr = scp->hsc_ps;

	if (scp->hsc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	hpuxtobsdmask(scp->hsc_mask, &p->p_sigctx.ps_sigmask);
	sigminusset(&sigcantmask, &p->p_sigctx.ps_sigmask);

#ifdef DEBUG
	if ((hpuxsigdebug & SDB_FPSTATE) && *(char *)&tstate.hss_fpstate)
		printf("sigreturn(%d): copied in FP state (%x) at %p\n",
		       p->p_pid, *(u_int *)&tstate.hss_fpstate,
		       &tstate.hss_fpstate);

	if ((hpuxsigdebug & SDB_FOLLOW) ||
	    ((hpuxsigdebug & SDB_KSTACK) && p->p_pid == hpuxsigpid))
		printf("sigreturn(%d): returns\n", p->p_pid);
#endif
	return (EJUSTRETURN);
}

/*
 * Set registers on exec.
 */
void
hpux_setregs(l, pack, stack)
	struct lwp *l;
	struct exec_package *pack;
	u_long stack;
{
	struct frame *frame;

	setregs(l, pack, stack);

	frame = (struct frame *)l->l_md.md_regs;
	frame->f_regs[D0] = 0;	/* no float card */
	frame->f_regs[D1] = fputype ? 1 : 0;	/* yes/no 68881 */
	frame->f_regs[A0] = 0;	/* not 68010 (bit 31), no FPA (30) */

	l->l_proc->p_md.mdp_flags &= ~MDP_HPUXMMAP;
}
