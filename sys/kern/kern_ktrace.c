/*	$NetBSD: kern_ktrace.c,v 1.38 1999/07/25 13:59:08 darrenr Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)kern_ktrace.c	8.5 (Berkeley) 5/14/95
 */

#include "opt_ktrace.h"

#ifdef KTRACE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/ktrace.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/filedesc.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

struct ktr_header *ktrgetheader __P((int));
int ktrops __P((struct proc *, struct proc *, int, int, void *));
int ktrsetchildren __P((struct proc *, struct proc *, int, int, void *));
void ktrwrite __P((struct proc *, void *, struct ktr_header *));
int ktrcanset __P((struct proc *, struct proc *));

void
ktrderef(p)
	struct proc *p;
{
	if (p->p_tracep == NULL)
		return;

	if (p->p_traceflag & KTRFAC_FD) {
		struct file *fp = p->p_tracep;

		FILE_USE(fp);
		closef(fp, NULL);
	} else {
		struct vnode *vp = p->p_tracep;

		vrele(vp);
	}
	p->p_tracep = NULL;
	p->p_traceflag = 0;
}

void
ktradref(p)
	struct proc *p;
{
	if (p->p_traceflag & KTRFAC_FD) {
		struct file *fp = p->p_tracep;

		fp->f_count++;
	} else {
		struct vnode *vp = p->p_tracep;

		VREF(vp);
	}
}

struct ktr_header *
ktrgetheader(type)
	int type;
{
	struct ktr_header *kth;
	struct proc *p = curproc;	/* XXX */

	MALLOC(kth, struct ktr_header *, sizeof(struct ktr_header), 
		M_TEMP, M_WAITOK);
	kth->ktr_type = type;
	microtime(&kth->ktr_time);
	kth->ktr_pid = p->p_pid;
	memcpy(kth->ktr_comm, p->p_comm, MAXCOMLEN);
	/* Note: ktr_len and ktr_buf are left to be filled in by the caller. */
	return (kth);
}

void
ktrsyscall(v, code, argsize, args)
	void *v;
	register_t code;
	size_t argsize;
	register_t args[];
{
	struct	ktr_header *kth;
	struct	ktr_syscall *ktp;
	struct proc *p = curproc;	/* XXX */
	register_t *argp;
	int	len = sizeof(struct ktr_syscall) + argsize;
	int i;

	p->p_traceflag |= KTRFAC_ACTIVE;
	kth = ktrgetheader(KTR_SYSCALL);
	MALLOC(ktp, struct ktr_syscall *, len, M_TEMP, M_WAITOK);
	ktp->ktr_code = code;
	ktp->ktr_argsize = argsize;
	argp = (register_t *)((char *)ktp + sizeof(struct ktr_syscall));
	for (i = 0; i < (argsize / sizeof(*argp)); i++)
		*argp++ = args[i];
	kth->ktr_buf = (caddr_t)ktp;
	kth->ktr_len = len;
	ktrwrite(p, v, kth);
	FREE(ktp, M_TEMP);
	FREE(kth, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrsysret(v, code, error, retval)
	void *v;
	register_t code;
	int error;
	register_t retval;
{
	struct ktr_header *kth;
	struct ktr_sysret ktp;
	struct proc *p = curproc;	/* XXX */

	p->p_traceflag |= KTRFAC_ACTIVE;
	kth = ktrgetheader(KTR_SYSRET);
	ktp.ktr_code = code;
	ktp.ktr_eosys = 0;			/* XXX unused */
	ktp.ktr_error = error;
	ktp.ktr_retval = retval;		/* what about val2 ? */

	kth->ktr_buf = (caddr_t)&ktp;
	kth->ktr_len = sizeof(struct ktr_sysret);

	ktrwrite(p, v, kth);
	FREE(kth, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrnamei(v, path)
	void *v;
	char *path;
{
	struct ktr_header *kth;
	struct proc *p = curproc;	/* XXX */

	p->p_traceflag |= KTRFAC_ACTIVE;
	kth = ktrgetheader(KTR_NAMEI);
	kth->ktr_len = strlen(path);
	kth->ktr_buf = path;

	ktrwrite(p, v, kth);
	FREE(kth, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktremul(v, p, emul)
	void *v;
	struct proc *p;
	char *emul;
{
	struct ktr_header *kth;

	p->p_traceflag |= KTRFAC_ACTIVE;
	kth = ktrgetheader(KTR_EMUL);
	kth->ktr_len = strlen(emul);
	kth->ktr_buf = emul;

	ktrwrite(p, v, kth);
	FREE(kth, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrgenio(v, fd, rw, iov, len, error)
	void *v;
	int fd;
	enum uio_rw rw;
	struct iovec *iov;
	int len, error;
{
	struct ktr_header *kth;
	struct ktr_genio *ktp;
	caddr_t cp;
	int resid = len, cnt;
	struct proc *p = curproc;	/* XXX */
	
	if (error)
		return;
	p->p_traceflag |= KTRFAC_ACTIVE;
	kth = ktrgetheader(KTR_GENIO);
	MALLOC(ktp, struct ktr_genio *, sizeof(struct ktr_genio) + len,
		M_TEMP, M_WAITOK);
	ktp->ktr_fd = fd;
	ktp->ktr_rw = rw;
	cp = (caddr_t)((char *)ktp + sizeof(struct ktr_genio));
	while (resid > 0) {
		if ((cnt = iov->iov_len) > resid)
			cnt = resid;
		if (copyin(iov->iov_base, cp, (unsigned)cnt))
			goto done;
		cp += cnt;
		resid -= cnt;
		iov++;
	}
	kth->ktr_buf = (caddr_t)ktp;
	kth->ktr_len = sizeof(struct ktr_genio) + len;

	ktrwrite(p, v, kth);
done:
	FREE(kth, M_TEMP);
	FREE(ktp, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrpsig(v, sig, action, mask, code)
	void *v;
	int sig;
	sig_t action;
	sigset_t *mask;
	int code;
{
	struct ktr_header *kth;
	struct ktr_psig	kp;
	struct proc *p = curproc;	/* XXX */

	p->p_traceflag |= KTRFAC_ACTIVE;
	kth = ktrgetheader(KTR_PSIG);
	kp.signo = (char)sig;
	kp.action = action;
	kp.mask = *mask;
	kp.code = code;
	kth->ktr_buf = (caddr_t)&kp;
	kth->ktr_len = sizeof(struct ktr_psig);

	ktrwrite(p, v, kth);
	FREE(kth, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrcsw(v, out, user)
	void *v;
	int out, user;
{
	struct ktr_header *kth;
	struct	ktr_csw kc;
	struct proc *p = curproc;	/* XXX */

	p->p_traceflag |= KTRFAC_ACTIVE;
	kth = ktrgetheader(KTR_CSW);
	kc.out = out;
	kc.user = user;
	kth->ktr_buf = (caddr_t)&kc;
	kth->ktr_len = sizeof(struct ktr_csw);

	ktrwrite(p, v, kth);
	FREE(kth, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

/* Interface and common routines */

/*
 * ktrace system call
 */
/* ARGSUSED */
int
sys_fktrace(curp, v, retval)
	struct proc *curp;
	void *v;
	register_t *retval;
{
	struct sys_fktrace_args /* {
		syscallarg(int) fd;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
	struct file *fp = NULL;
	struct proc *p;
	struct filedesc *fdp = curp->p_fd;
	struct pgrp *pg;
	int facs;
	int ops;
	int descend;
	int ret = 0;
	int error = 0;

	if (((u_int)SCARG(uap, fd)) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL ||
	    (fp->f_flag & FWRITE) == 0)
		return (EBADF);

	ops = KTROP(SCARG(uap, ops)) | KTRFLAG_FD;
	descend = SCARG(uap, ops) & KTRFLAG_DESCEND;
	facs = SCARG(uap, facs) & ~((unsigned) KTRFAC_ROOT);
	curp->p_traceflag |= KTRFAC_ACTIVE;

	/*
	 * Clear all uses of the tracefile
	 */
	if (KTROP(ops) == KTROP_CLEARFILE) {
		proclist_lock_read();
		for (p = allproc.lh_first; p != 0; p = p->p_list.le_next) {
			if (p->p_tracep == fp) {
				if (ktrcanset(curp, p))
					ktrderef(p);
				else
					error = EPERM;
			}
		}
		proclist_unlock_read();
		goto done;
	}
	/*
	 * need something to (un)trace (XXX - why is this here?)
	 */
	if (!facs) {
		error = EINVAL;
		goto done;
	}
	/* 
	 * do it
	 */
	if (SCARG(uap, pid) < 0) {
		/*
		 * by process group
		 */
		pg = pgfind(-SCARG(uap, pid));
		if (pg == NULL) {
			error = ESRCH;
			goto done;
		}
		for (p = pg->pg_members.lh_first; p != 0; p = p->p_pglist.le_next)
			if (descend)
				ret |= ktrsetchildren(curp, p, ops, facs, fp);
			else 
				ret |= ktrops(curp, p, ops, facs, fp);
					
	} else {
		/*
		 * by pid
		 */
		p = pfind(SCARG(uap, pid));
		if (p == NULL) {
			error = ESRCH;
			goto done;
		}
		if (descend)
			ret |= ktrsetchildren(curp, p, ops, facs, fp);
		else
			ret |= ktrops(curp, p, ops, facs, fp);
	}
	if (!ret)
		error = EPERM;
done:
	curp->p_traceflag &= ~KTRFAC_ACTIVE;
	return (error);
}

/*
 * ktrace system call
 */
/* ARGSUSED */
int
sys_ktrace(curp, v, retval)
	struct proc *curp;
	void *v;
	register_t *retval;
{
	struct sys_ktrace_args /* {
		syscallarg(const char *) fname;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
	struct vnode *vp = NULL;
	struct proc *p;
	struct pgrp *pg;
	int facs = SCARG(uap, facs) & ~((unsigned) KTRFAC_ROOT);
	int ops = KTROP(SCARG(uap, ops));
	int descend = SCARG(uap, ops) & KTRFLAG_DESCEND;
	int ret = 0;
	int error = 0;
	struct nameidata nd;

	curp->p_traceflag |= KTRFAC_ACTIVE;
	if (ops != KTROP_CLEAR) {
		/*
		 * an operation which requires a file argument.
		 */
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, fname),
		    curp);
		if ((error = vn_open(&nd, FREAD|FWRITE, 0)) != 0) {
			curp->p_traceflag &= ~KTRFAC_ACTIVE;
			return (error);
		}
		vp = nd.ni_vp;
		VOP_UNLOCK(vp, 0);
		if (vp->v_type != VREG) {
			(void) vn_close(vp, FREAD|FWRITE, curp->p_ucred, curp);
			curp->p_traceflag &= ~KTRFAC_ACTIVE;
			return (EACCES);
		}
	}
	/*
	 * Clear all uses of the tracefile
	 */
	if (KTROP(ops) == KTROP_CLEARFILE) {
		proclist_lock_read();
		for (p = allproc.lh_first; p != 0; p = p->p_list.le_next) {
			if (p->p_tracep == vp &&
			    !ktrops(curp, p, KTROP_CLEAR, ~0, vp))
				error = EPERM;
		}
		proclist_unlock_read();
		goto done;
	}
	/*
	 * need something to (un)trace (XXX - why is this here?)
	 */
	if (!facs) {
		error = EINVAL;
		goto done;
	}
	/* 
	 * do it
	 */
	if (SCARG(uap, pid) < 0) {
		/*
		 * by process group
		 */
		pg = pgfind(-SCARG(uap, pid));
		if (pg == NULL) {
			error = ESRCH;
			goto done;
		}
		for (p = pg->pg_members.lh_first; p != 0; p = p->p_pglist.le_next)
			if (descend)
				ret |= ktrsetchildren(curp, p, ops, facs, vp);
			else 
				ret |= ktrops(curp, p, ops, facs, vp);
					
	} else {
		/*
		 * by pid
		 */
		p = pfind(SCARG(uap, pid));
		if (p == NULL) {
			error = ESRCH;
			goto done;
		}
		if (descend)
			ret |= ktrsetchildren(curp, p, ops, facs, vp);
		else
			ret |= ktrops(curp, p, ops, facs, vp);
	}
	if (!ret)
		error = EPERM;
done:
	if (vp != NULL)
		(void) vn_close(vp, FWRITE, curp->p_ucred, curp);
	curp->p_traceflag &= ~KTRFAC_ACTIVE;
	return (error);
}

int
ktrops(curp, p, ops, facs, v)
	struct proc *p, *curp;
	int ops, facs;
	void *v;
{

	if (!ktrcanset(curp, p))
		return (0);
	if (KTROP(ops) == KTROP_SET) {
		if (p->p_tracep != v) { 
			/*
			 * if trace file already in use, relinquish
			 */
			ktrderef(p);
			if (ops & KTRFLAG_FD)
				p->p_traceflag = KTRFAC_FD;
			p->p_tracep = v;
			ktradref(p);
		}
		p->p_traceflag |= facs;
		if (curp->p_ucred->cr_uid == 0)
			p->p_traceflag |= KTRFAC_ROOT;
	} else {	
		/* KTROP_CLEAR */
		if (((p->p_traceflag &= ~facs) & KTRFAC_MASK) == 0) {
			/* no more tracing */
			ktrderef(p);
		}
	}

	/*
	 * Emit an emulation record, every time there is a ktrace
	 * change/attach request. 
	 */
	if (KTRPOINT(p, KTR_EMUL))
		ktremul(p->p_tracep, p, p->p_emul->e_name);

	return (1);
}

int
ktrsetchildren(curp, top, ops, facs, v)
	struct proc *curp, *top;
	int ops, facs;
	void *v;
{
	struct proc *p;
	int ret = 0;

	p = top;
	for (;;) {
		ret |= ktrops(curp, p, ops, facs, v);
		/*
		 * If this process has children, descend to them next,
		 * otherwise do any siblings, and if done with this level,
		 * follow back up the tree (but not past top).
		 */
		if (p->p_children.lh_first)
			p = p->p_children.lh_first;
		else for (;;) {
			if (p == top)
				return (ret);
			if (p->p_sibling.le_next) {
				p = p->p_sibling.le_next;
				break;
			}
			p = p->p_pptr;
		}
	}
	/*NOTREACHED*/
}

void
ktrwrite(p, v, kth)
	struct proc *p;
	void *v;
	struct ktr_header *kth;
{
	struct uio auio;
	struct iovec aiov[2];
	int error;

	if (v == NULL)
		return;
	auio.uio_iov = &aiov[0];
	auio.uio_offset = 0;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_WRITE;
	aiov[0].iov_base = (caddr_t)kth;
	aiov[0].iov_len = sizeof(struct ktr_header);
	auio.uio_resid = sizeof(struct ktr_header);
	auio.uio_iovcnt = 1;
	auio.uio_procp = (struct proc *)0;
	if (kth->ktr_len > 0) {
		auio.uio_iovcnt++;
		aiov[1].iov_base = kth->ktr_buf;
		aiov[1].iov_len = kth->ktr_len;
		auio.uio_resid += kth->ktr_len;
	}
	if (p->p_traceflag & KTRFAC_FD) {
		struct file *fp = v;

		FILE_USE(fp);
		error = (*fp->f_ops->fo_write)(fp, &fp->f_offset, &auio,
		    fp->f_cred, FOF_UPDATE_OFFSET);
		FILE_UNUSE(fp, NULL);
	}
	else {
		struct vnode *vp = v;

		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		error = VOP_WRITE(vp, &auio, IO_UNIT|IO_APPEND, p->p_ucred);
		VOP_UNLOCK(vp, 0);
	}
	if (!error)
		return;
	/*
	 * If error encountered, give up tracing on this vnode.  Don't report
	 * EPIPE as this can easily happen with fktrace()/ktruss.
	 */
	if (error != EPIPE)
		log(LOG_NOTICE,
		    "ktrace write failed, errno %d, tracing stopped\n",
		    error);
	proclist_lock_read();
	for (p = allproc.lh_first; p != 0; p = p->p_list.le_next) {
		if (p->p_tracep == v)
			ktrderef(p);
	}
	proclist_unlock_read();
}

/*
 * Return true if caller has permission to set the ktracing state
 * of target.  Essentially, the target can't possess any
 * more permissions than the caller.  KTRFAC_ROOT signifies that
 * root previously set the tracing status on the target process, and 
 * so, only root may further change it.
 *
 * TODO: check groups.  use caller effective gid.
 */
int
ktrcanset(callp, targetp)
	struct proc *callp, *targetp;
{
	struct pcred *caller = callp->p_cred;
	struct pcred *target = targetp->p_cred;

	if ((caller->pc_ucred->cr_uid == target->p_ruid &&
	     target->p_ruid == target->p_svuid &&
	     caller->p_rgid == target->p_rgid &&	/* XXX */
	     target->p_rgid == target->p_svgid &&
	     (targetp->p_traceflag & KTRFAC_ROOT) == 0) ||
	     caller->pc_ucred->cr_uid == 0)
		return (1);

	return (0);
}

#endif
