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
 *	Id: procfs_note.c,v 4.1 1993/12/17 10:47:45 jsp Rel
 *
 *	$Id: procfs_note.c,v 1.2 1994/01/20 21:23:06 ws Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <miscfs/procfs/procfs.h>

procfs_namemap_t procfs_signames[] = {
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

pfs_readnote(sig, uio)
	int sig;
	struct uio *uio;
{
	int xlen;
	int error;
	procfs_namemap_t *nm;
	
	/* could do indexing by sig */
	for (nm = procfs_signames; nm->nm_name; nm++)
		if (nm->nm_val == sig)
			break;
	if (!nm->nm_name)
		panic("pfs_readnote");
	
	xlen = strlen(nm->nm_name);
	if (uio->uio_resid < xlen)
		return EMSGSIZE;
	
	if (error = uiomove(nm->nm_name, xlen, uio))
		return error;
	
	return 0;
}

pfs_donote(curp, p, pfs, uio)
	struct proc *curp;
	struct proc *p;
	struct pfsnode *pfs;
	struct uio *uio;
{
	int len = uio->uio_resid;
	int xlen;
	int error;
	char note[PROCFS_NOTELEN+1];
	procfs_namemap_t *nm;
	int sig, mask;
	
	if (pfs->pfs_type == Pnote && uio->uio_rw == UIO_READ) {
		
		mask = p->p_sig & ~p->p_sigmask;
		
		if (p->p_flag&SPPWAIT)
			mask &= ~stopsigmask;
		if (mask == 0)
			return 0;
		sig = ffs((long)mask);
		
		if (error = pfs_readnote(sig, pfs, uio))
			return error;
		
		p->p_sig &= ~mask;
		return 0;
	}
	
	if (uio->uio_rw != UIO_WRITE)
		return (EINVAL);

	xlen = PROCFS_NOTELEN;
	error = procfs_getuserstr(uio, note, &xlen);
	if (error)
		return (error);
	
	/*
	 * Map signal names into signal generation
	 * Unknown signals return EOPNOTSUPP.
	 */
	error = EOPNOTSUPP;
	
	nm = procfs_findname(procfs_signames, note, xlen);
	if (nm) {
		if (pfs->pfs_type == Pnote)
			psignal(p, nm->nm_val);
		else
			gsignal(p->p_pgid, nm->nm_val);
		
		error = 0;
	}
	return (error);
}
