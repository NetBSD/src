/*	$NetBSD: sys_process.c,v 1.66.2.12 2002/04/01 21:31:33 nathanw Exp $	*/

/*-
 * Copyright (c) 1994 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	from: @(#)sys_process.c	8.1 (Berkeley) 6/10/93
 */

/*
 * References:
 *	(1) Bach's "The Design of the UNIX Operating System",
 *	(2) sys/miscfs/procfs from UCB's 4.4BSD-Lite distribution,
 *	(3) the "4.4BSD Programmer's Reference Manual" published
 *		by USENIX and O'Reilly & Associates.
 * The 4.4BSD PRM does a reasonably good job of documenting what the various
 * ptrace() requests should actually do, and its text is quoted several times
 * in this file.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_process.c,v 1.66.2.12 2002/04/01 21:31:33 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <machine/reg.h>

#include <miscfs/procfs/procfs.h>

/* Macros to clear/set/test flags. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))

/*
 * Process debugging system call.
 */
int
sys_ptrace(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_ptrace_args /* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(caddr_t) addr;
		syscallarg(int) data;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct lwp *lt;
	struct proc *t;				/* target process */
	struct uio uio;
	struct iovec iov;
	struct ptrace_io_desc piod;
	int s, error, write, tmp;

	/* "A foolish consistency..." XXX */
	if (SCARG(uap, req) == PT_TRACE_ME)
		t = p;
	else {

		/* Find the process we're supposed to be operating on. */
		if ((t = pfind(SCARG(uap, pid))) == NULL)
			return (ESRCH);
	}

	/* Can't trace a process that's currently exec'ing. */
	if ((t->p_flag & P_INEXEC) != 0)
		return EAGAIN;

	/* Make sure we can operate on it. */
	switch (SCARG(uap, req)) {
	case  PT_TRACE_ME:
		/* Saying that you're being traced is always legal. */
		break;

	case  PT_ATTACH:
		/*
		 * You can't attach to a process if:
		 *	(1) it's the process that's doing the attaching,
		 */
		if (t->p_pid == p->p_pid)
			return (EINVAL);

		/*
		 *  (2) it's a system process
		 */
		if (t->p_flag & P_SYSTEM)
			return (EPERM);

		/*
		 *	(3) it's already being traced, or
		 */
		if (ISSET(t->p_flag, P_TRACED))
			return (EBUSY);

		/*
		 *	(4) it's not owned by you, or is set-id on exec
		 *	    (unless you're root), or...
		 */
		if ((t->p_cred->p_ruid != p->p_cred->p_ruid ||
			ISSET(t->p_flag, P_SUGID)) &&
		    (error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (error);

		/*
		 *	(5) ...it's init, which controls the security level
		 *	    of the entire system, and the system was not
		 *	    compiled with permanently insecure mode turned on
		 */
		if (t == initproc && securelevel > -1)
			return (EPERM);

		/*
		 * (6) the tracer is chrooted, and its root directory is
		 * not at or above the root directory of the tracee
		 */

		if (!proc_isunder(t, p))
			return EPERM;
		break;

	case  PT_READ_I:
	case  PT_READ_D:
	case  PT_WRITE_I:
	case  PT_WRITE_D:
	case  PT_CONTINUE:
	case  PT_IO:
	case  PT_KILL:
	case  PT_DETACH:
#ifdef PT_STEP
	case  PT_STEP:
#endif
#ifdef PT_GETREGS
	case  PT_GETREGS:
#endif
#ifdef PT_SETREGS
	case  PT_SETREGS:
#endif
#ifdef PT_GETFPREGS
	case  PT_GETFPREGS:
#endif
#ifdef PT_SETFPREGS
	case  PT_SETFPREGS:
#endif

#ifdef __HAVE_PTRACE_MACHDEP
	PTRACE_MACHDEP_REQUEST_CASES
#endif

		/*
		 * You can't do what you want to the process if:
		 *	(1) It's not being traced at all,
		 */
		if (!ISSET(t->p_flag, P_TRACED))
			return (EPERM);

		/*
		 *	(2) it's being traced by procfs (which has
		 *	    different signal delivery semantics),
		 */
		if (ISSET(t->p_flag, P_FSTRACE))
			return (EBUSY);

		/*
		 *	(3) it's not being traced by _you_, or
		 */
		if (t->p_pptr != p)
			return (EBUSY);

		/*
		 *	(4) it's not currently stopped.
		 */
		if (t->p_stat != SSTOP || !ISSET(t->p_flag, P_WAITED))
			return (EBUSY);
		break;

	default:			/* It was not a legal request. */
		return (EINVAL);
	}

	/* Do single-step fixup if needed. */
	FIX_SSTEP(t);

	/*
	 * XXX NJWLWP
	 *
	 * The entire ptrace interface needs work to be useful to a
	 * process with multiple LWPs. For the moment, we'll kluge
	 * this; memory access will be fine, but register access will
	 * be weird.
	 */

	lt = proc_representative_lwp(t);

	/* Now do the operation. */
	write = 0;
	*retval = 0;
	tmp = 0;

	switch (SCARG(uap, req)) {
	case  PT_TRACE_ME:
		/* Just set the trace flag. */
		SET(t->p_flag, P_TRACED);
		t->p_oppid = t->p_pptr->p_pid;
		return (0);

	case  PT_WRITE_I:		/* XXX no separate I and D spaces */
	case  PT_WRITE_D:
		write = 1;
		tmp = SCARG(uap, data);
	case  PT_READ_I:		/* XXX no separate I and D spaces */
	case  PT_READ_D:
		/* write = 0 done above. */
		iov.iov_base = (caddr_t)&tmp;
		iov.iov_len = sizeof(tmp);
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = (off_t)(long)SCARG(uap, addr);
		uio.uio_resid = sizeof(tmp);
		uio.uio_segflg = UIO_SYSSPACE;
		uio.uio_rw = write ? UIO_WRITE : UIO_READ;
		uio.uio_procp = p;
		error = procfs_domem(p, t, NULL, &uio);
		if (!write)
			*retval = tmp;
		return (error);
		
	case  PT_IO:
		error = copyin(SCARG(uap, addr), &piod, sizeof(piod));
		if (error)
			return (error);
		iov.iov_base = piod.piod_addr;
		iov.iov_len = piod.piod_len;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = (off_t)(long)piod.piod_offs;
		uio.uio_resid = piod.piod_len;
		uio.uio_segflg = UIO_USERSPACE;
		uio.uio_procp = p;
		switch (piod.piod_op) {
		case PIOD_READ_D:
		case PIOD_READ_I:
			uio.uio_rw = UIO_READ;
			break;
		case PIOD_WRITE_D:
		case PIOD_WRITE_I:
			uio.uio_rw = UIO_WRITE;
			break;
		default:
			return (EINVAL);
		}
		error = procfs_domem(p, t, NULL, &uio);
		piod.piod_len -= uio.uio_resid;
		(void) copyout(&piod, SCARG(uap, addr), sizeof(piod));
		return (error);

#ifdef PT_STEP
	case  PT_STEP:
		/*
		 * From the 4.4BSD PRM:
		 * "Execution continues as in request PT_CONTINUE; however
		 * as soon as possible after execution of at least one
		 * instruction, execution stops again. [ ... ]"
		 */
#endif
	case  PT_CONTINUE:
	case  PT_DETACH:
		/*
		 * From the 4.4BSD PRM:
		 * "The data argument is taken as a signal number and the
		 * child's execution continues at location addr as if it
		 * incurred that signal.  Normally the signal number will
		 * be either 0 to indicate that the signal that caused the
		 * stop should be ignored, or that value fetched out of
		 * the process's image indicating which signal caused
		 * the stop.  If addr is (int *)1 then execution continues
		 * from where it stopped."
		 */

		/* Check that the data is a valid signal number or zero. */
		if (SCARG(uap, data) < 0 || SCARG(uap, data) >= NSIG)
			return (EINVAL);

		PHOLD(lt);

#ifdef PT_STEP
		/*
		 * Arrange for a single-step, if that's requested and possible.
		 */
		error = process_sstep(lt, SCARG(uap, req) == PT_STEP);
		if (error)
			goto relebad;
#endif

		/* If the address parameter is not (int *)1, set the pc. */
		if ((int *)SCARG(uap, addr) != (int *)1)
			if ((error = process_set_pc(lt, SCARG(uap, addr))) != 0)
				goto relebad;

		PRELE(lt);

		if (SCARG(uap, req) == PT_DETACH) {
			/* give process back to original parent or init */
			if (t->p_oppid != t->p_pptr->p_pid) {
				struct proc *pp;

				pp = pfind(t->p_oppid);
				proc_reparent(t, pp ? pp : initproc);
			}

			/* not being traced any more */
			t->p_oppid = 0;
			CLR(t->p_flag, P_TRACED|P_WAITED);
		}

	sendsig:
		/* Finally, deliver the requested signal (or none). */
		if (t->p_stat == SSTOP) {
			t->p_xstat = SCARG(uap, data);
			SCHED_LOCK(s);
			setrunnable(proc_unstop(t));
			SCHED_UNLOCK(s);
		} else {
			if (SCARG(uap, data) != 0)
				psignal(t, SCARG(uap, data));
		}
		return (0);

	relebad:
		PRELE(lt);
		return (error);

	case  PT_KILL:
		/* just send the process a KILL signal. */
		SCARG(uap, data) = SIGKILL;
		goto sendsig;	/* in PT_CONTINUE, above. */

	case  PT_ATTACH:
		/*
		 * Go ahead and set the trace flag.
		 * Save the old parent (it's reset in
		 *   _DETACH, and also in kern_exit.c:wait4()
		 * Reparent the process so that the tracing
		 *   proc gets to see all the action.
		 * Stop the target.
		 */
		SET(t->p_flag, P_TRACED);
		t->p_oppid = t->p_pptr->p_pid;
		if (t->p_pptr != p)
			proc_reparent(t, p);
		SCARG(uap, data) = SIGSTOP;
		goto sendsig;

#ifdef PT_SETREGS
	case  PT_SETREGS:
		write = 1;
#endif
#ifdef PT_GETREGS
	case  PT_GETREGS:
		/* write = 0 done above. */
#endif
#if defined(PT_SETREGS) || defined(PT_GETREGS)
		if (!procfs_validregs(lt, NULL))
			return (EINVAL);
		else {
			iov.iov_base = SCARG(uap, addr);
			iov.iov_len = sizeof(struct reg);
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_offset = 0;
			uio.uio_resid = sizeof(struct reg);
			uio.uio_segflg = UIO_USERSPACE;
			uio.uio_rw = write ? UIO_WRITE : UIO_READ;
			uio.uio_procp = p;
			return (procfs_doregs(p, lt, NULL, &uio));
		}
#endif

#ifdef PT_SETFPREGS
	case  PT_SETFPREGS:
		write = 1;
#endif
#ifdef PT_GETFPREGS
	case  PT_GETFPREGS:
		/* write = 0 done above. */
#endif
#if defined(PT_SETFPREGS) || defined(PT_GETFPREGS)
		if (!procfs_validfpregs(lt, NULL))
			return (EINVAL);
		else {
			iov.iov_base = SCARG(uap, addr);
			iov.iov_len = sizeof(struct fpreg);
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_offset = 0;
			uio.uio_resid = sizeof(struct fpreg);
			uio.uio_segflg = UIO_USERSPACE;
			uio.uio_rw = write ? UIO_WRITE : UIO_READ;
			uio.uio_procp = p;
			return (procfs_dofpregs(p, lt, NULL, &uio));
		}
#endif

#ifdef __HAVE_PTRACE_MACHDEP
	PTRACE_MACHDEP_REQUEST_CASES
		return (ptrace_machdep_dorequest(p, lt,
		    SCARG(uap, req), SCARG(uap, addr),
		    SCARG(uap, data)));
#endif
	}

#ifdef DIAGNOSTIC
	panic("ptrace: impossible");
#endif
	return 0;
}
