/*
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1993 Jan-Simon Pendry
 * All rights reserved.
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
 * From:
 *	Id: procfs_ctl.c,v 4.1 1993/12/17 10:47:45 jsp Rel
 *
 *	$Id: procfs_ctl.c,v 1.7 1994/01/20 21:23:05 ws Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/ptrace.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <miscfs/procfs/procfs.h>

/*
 * True iff process (p) is in trace wait state
 */
#define TRACE_WAIT_P(p) \
	((p)->p_stat == SSTOP && \
	 ((p)->p_flag & STRC))

#define PROCFS_CTL_ATTACH	1
#define PROCFS_CTL_DETACH	2
#define PROCFS_CTL_STEP		3
#define PROCFS_CTL_RUN		4
#define PROCFS_CTL_WAIT		5

static procfs_namemap_t ctlnames[] = {
	/* special /proc commands */
	{ "attach",	PROCFS_CTL_ATTACH },
	{ "detach",	PROCFS_CTL_DETACH },
	{ "step",	PROCFS_CTL_STEP },
	{ "run",	PROCFS_CTL_RUN },
	{ 0 },
};

extern procfs_namemap_t procfs_signames[];

static int
procfs_control(curp, p, op)
	struct proc *curp;
	struct proc *p;
	int op;
{
	int error;

	/*
	 * Target process must be stopped and
	 * be set up for tracing (SFSTRC flag set).
	 * Of course ATTACH is allowed any time.
	 * Allow DETACH to take place at any time for sanity.
	 */
	switch (op) {
	case PROCFS_CTL_ATTACH:
	case PROCFS_CTL_DETACH:
		break;

	default:
		if (!TRACE_WAIT_P(p))
			return (EBUSY);
	}

	/*
	 * do single-step fixup if needed
	 */
	FIX_SSTEP(p);

	switch (op) {
	case PROCFS_CTL_ATTACH:
		/*
		 * Attach - attaches the target process for debugging
		 * by the calling process.
		 */
		/* check whether already being traced */
		if (p->p_flag & STRC)
			return (EBUSY);
		
		/* can't trace yourself! */
		/* DO WE REALLY NEED THIS RESTRICTION??? */
		if (curp && p->p_pid == curp->p_pid)
			return (EINVAL);
		
		/*
		 * Go ahead and set the trace flag.
		 * Save the old parent (it's reset in
		 *   _DETACH, and also in kern_exit.c:wait4()
		 * Reparent the process so that the tracing
		 *   proc gets to see all the action.
		 * Stop the target.
		 */
		p->p_flag |= STRC|SFSTRC;
		psignal(p, SIGSTOP);
		break;

	/*
	 * Detach.  Cleans up the target process, reparent it if possible
	 * and set it running once more.
	 */
	case PROCFS_CTL_DETACH:
		/* if not being traced, then this is a painless no-op */
		if ((p->p_flag & SFSTRC) == 0)
			return (0);

		if (error = process_sstep(p, 0))
			return error;
		
		/* not being traced any more */
		p->p_flag &= ~(STRC|SFSTRC);
		setrun(p);
		return 0;
		
	/*
	 * Step.  Let the target process execute a single instruction.
	 */
	case PROCFS_CTL_STEP:
		/* Fall through */
	/*
	 * Run.  Let the target process continue running until a breakpoint
	 * or some other trap.
	 */
	case PROCFS_CTL_RUN:
		if (error = process_sstep(p, op == PROCFS_CTL_STEP))
			return error;
		setrun(p);
		break;

	default:
		panic("procfs_control");
	}
	/*
	 * Wait for the target process to stop.
	 */
	while ((p->p_stat != SSTOP) &&
	       (p->p_flag & SFSTRC)) {
		if (error = tsleep((caddr_t) p,
				   PWAIT|PCATCH, "procfs", 0))
			return error;
	}
	if (!TRACE_WAIT_P(p))
		return EBUSY;
	
	return 0;
}

pfs_doctl(curp, p, pfs, uio)
	struct proc *curp;
	struct pfsnode *pfs;
	struct uio *uio;
	struct proc *p;
{
	int len = uio->uio_resid;
	int xlen;
	int error;
	char msg[PROCFS_CTLLEN+1];
	procfs_namemap_t *nm;
	
	/*
	 * If we are debugging, you might read the stop signal.
	 */
	if (uio->uio_rw == UIO_READ && TRACE_WAIT_P(p)) {
		if (p->p_xstat) {
			if (error = pfs_readnote(p->p_xstat, uio))
				return error;
			
			p->p_xstat = 0;
		}
		return 0;
	}
	
	if (uio->uio_rw != UIO_WRITE)
		return (EOPNOTSUPP);

	xlen = PROCFS_CTLLEN;
	error = procfs_getuserstr(uio, msg, &xlen);
	if (error)
		return (error);
	
	/*
	 * Map debug control.  Unknown commands return EOPNOTSUPP.
	 */
	error = EOPNOTSUPP;

	nm = procfs_findname(ctlnames, msg, xlen);
	if (nm) {
		error = procfs_control(curp, p, nm->nm_val);
	} else if (TRACE_WAIT_P(p)) {
		/*
		 * If we are debugging, it might be a signal.
		 */
		nm = procfs_findname(procfs_signames, msg, xlen);
		if (nm) {
			p->p_xstat = nm->nm_val;
			error = 0;
		}
	}
	
	return (error);
}
