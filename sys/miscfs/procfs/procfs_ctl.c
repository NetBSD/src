/*	$NetBSD: procfs_ctl.c,v 1.15 1997/04/28 02:28:39 mycroft Exp $	*/

/*
 * Copyright (c) 1993 Jan-Simon Pendry
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)procfs_ctl.c	8.4 (Berkeley) 6/15/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/ptrace.h>
#include <miscfs/procfs/procfs.h>

#define PROCFS_CTL_ATTACH	1
#define PROCFS_CTL_DETACH	2
#define PROCFS_CTL_STEP		3
#define PROCFS_CTL_RUN		4
#define PROCFS_CTL_WAIT		5

static vfs_namemap_t ctlnames[] = {
	/* special /proc commands */
	{ "attach",	PROCFS_CTL_ATTACH },
	{ "detach",	PROCFS_CTL_DETACH },
	{ "step",	PROCFS_CTL_STEP },
	{ "run",	PROCFS_CTL_RUN },
	{ "wait",	PROCFS_CTL_WAIT },
	{ 0 },
};

static vfs_namemap_t signames[] = {
	/* regular signal names */
	{ "hup",	SIGHUP },	{ "int",	SIGINT },
	{ "quit",	SIGQUIT },	{ "ill",	SIGILL },
	{ "trap",	SIGTRAP },	{ "abrt",	SIGABRT },
	{ "iot",	SIGIOT },	{ "emt",	SIGEMT },
	{ "fpe",	SIGFPE },	{ "kill",	SIGKILL },
	{ "bus",	SIGBUS },	{ "segv",	SIGSEGV },
	{ "sys",	SIGSYS },	{ "pipe",	SIGPIPE },
	{ "alrm",	SIGALRM },	{ "term",	SIGTERM },
	{ "urg",	SIGURG },	{ "stop",	SIGSTOP },
	{ "tstp",	SIGTSTP },	{ "cont",	SIGCONT },
	{ "chld",	SIGCHLD },	{ "ttin",	SIGTTIN },
	{ "ttou",	SIGTTOU },	{ "io",		SIGIO },
	{ "xcpu",	SIGXCPU },	{ "xfsz",	SIGXFSZ },
	{ "vtalrm",	SIGVTALRM },	{ "prof",	SIGPROF },
	{ "winch",	SIGWINCH },	{ "info",	SIGINFO },
	{ "usr1",	SIGUSR1 },	{ "usr2",	SIGUSR2 },
	{ 0 },
};

int procfs_control __P((struct proc *, struct proc *, int, int));

/* Macros to clear/set/test flags. */ 
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))

int
procfs_control(curp, p, op, sig)
	struct proc *curp;
	struct proc *p;
	int op, sig;
{
	int error;

	/*
	 * Attach - attaches the target process for debugging
	 * by the calling process.
	 */
	switch (op) {
	case PROCFS_CTL_ATTACH:
		/* 
		 * You can't attach to a process if:
		 *      (1) it's the process that's doing the attaching,
		 */
		if (p->p_pid == curp->p_pid)
			return (EINVAL);

		/*
		 *      (2) it's already being traced, or
		 */
		if (ISSET(p->p_flag, P_TRACED))
			return (EBUSY);

		/*
		 *      (3) it's not owned by you, or is set-id on exec
		 *          (unless you're root), or...
		 */
		if ((p->p_cred->p_ruid != curp->p_cred->p_ruid ||
			ISSET(p->p_flag, P_SUGID)) &&
		    (error = suser(curp->p_ucred, &curp->p_acflag)) != 0)
			return (error);

		/*
		 *      (4) ...it's init, which controls the security level
		 *          of the entire system, and the system was not
		 *          compiled with permanently insecure mode turned
		 *          on.
		 */
		if (p == initproc && securelevel > -1)
			return (EPERM);
		break;

	/*
	 * Target process must be stopped, owned by (curp) and
	 * be set up for tracing (P_TRACED flag set).
	 * Allow DETACH to take place at any time for sanity.
	 * Allow WAIT any time, of course.
	 *
	 * Note that the semantics of DETACH are different here
	 * from ptrace(2).
	 */
	case PROCFS_CTL_DETACH:
	case PROCFS_CTL_WAIT:
		/*
		 * You can't do what you want to the process if:
		 *      (1) It's not being traced at all, or
		 */
		if (!ISSET(p->p_flag, P_TRACED))
			return (EPERM);

		/*
		 *      (2) it's not being traced by _you_.
		 */
		if (p->p_pptr != curp)
			return (EBUSY);
		break;

	default:
		/*
		 * You can't do what you want to the process if:
		 *      (1) It's not being traced at all,
		 */
		if (!ISSET(p->p_flag, P_TRACED))
			return (EPERM);

		/*
		 *      (2) it's not being traced by _you_, or
		 */
		if (p->p_pptr != curp)
			return (EBUSY);

		/*
		 *      (3) it's not currently stopped.
		 */
		if (p->p_stat != SSTOP || !ISSET(p->p_flag, P_WAITED))
			return (EBUSY);
		break;
	}

	/* Do single-step fixup if needed. */
	FIX_SSTEP(p);

	switch (op) {
	case PROCFS_CTL_ATTACH:
		/*
		 * Go ahead and set the trace flag.
		 * Save the old parent (it's reset in
		 *   _DETACH, and also in kern_exit.c:wait4()
		 * Reparent the process so that the tracing
		 *   proc gets to see all the action.
		 * Stop the target.
		 */
		SET(p->p_flag, P_TRACED);
		p->p_oppid = p->p_pptr->p_pid;
		if (p->p_pptr != curp)
			proc_reparent(p, curp);
		sig = SIGSTOP;
		goto sendsig;

#ifdef PT_STEP
	case PROCFS_CTL_STEP:
		/*
		 * Step.  Let the target process execute a single instruction.
		 */
#endif
	case PROCFS_CTL_RUN:
	case PROCFS_CTL_DETACH:
#ifdef PT_STEP
		PHOLD(p);
		error = process_sstep(p, op == PROCFS_CTL_STEP);
		PRELE(p);
		if (error)
			return (error);
#endif

		if (op == PROCFS_CTL_DETACH) {
			/* give process back to original parent */
			if (p->p_oppid != p->p_pptr->p_pid) {
				struct proc *pp;
	
				pp = pfind(p->p_oppid);
				proc_reparent(p, pp ? pp : initproc);
			}

			/* not being traced any more */
			p->p_oppid = 0;
			CLR(p->p_flag, P_TRACED|P_WAITED);
		}

	sendsig:
		/* Finally, deliver the requested signal (or none). */
		if (p->p_stat == SSTOP) {
			p->p_xstat = sig;
			setrunnable(p);
		} else {
			if (sig != 0)
				psignal(p, sig);
		}
		return (0);

	case PROCFS_CTL_WAIT:
		/*
		 * Wait for the target process to stop.
		 */
		error = 0;
		while (error == 0 && p->p_stat != SSTOP)
			error = tsleep(p, PWAIT|PCATCH, "procfsx", 0);
		return (error);

	default:
		panic("procfs_control");
	}
}

int
procfs_doctl(curp, p, pfs, uio)
	struct proc *curp;
	struct pfsnode *pfs;
	struct uio *uio;
	struct proc *p;
{
	int xlen;
	int error;
	char msg[PROCFS_CTLLEN+1];
	vfs_namemap_t *nm;

	if (uio->uio_rw != UIO_WRITE)
		return (EOPNOTSUPP);

	xlen = PROCFS_CTLLEN;
	error = vfs_getuserstr(uio, msg, &xlen);
	if (error)
		return (error);

	/*
	 * Map signal names into signal generation
	 * or debug control.  Unknown commands and/or signals
	 * return EOPNOTSUPP.
	 *
	 * Sending a signal while the process is being debugged
	 * also has the side effect of letting the target continue
	 * to run.  There is no way to single-step a signal delivery.
	 */
	error = EOPNOTSUPP;

	nm = vfs_findname(ctlnames, msg, xlen);
	if (nm) {
		error = procfs_control(curp, p, nm->nm_val, 0);
	} else {
		nm = vfs_findname(signames, msg, xlen);
		if (nm) {
			if (ISSET(p->p_flag, P_TRACED) &&
			    p->p_pptr == curp)
				error = procfs_control(curp, p, PROCFS_CTL_RUN,
				    nm->nm_val);
			else {
				psignal(p, nm->nm_val);
				error = 0;
			}
		}
	}

	return (error);
}
