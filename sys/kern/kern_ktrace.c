/*	$NetBSD: kern_ktrace.c,v 1.89 2004/04/30 07:51:59 enami Exp $	*/

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
 *	@(#)kern_ktrace.c	8.5 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_ktrace.c,v 1.89 2004/04/30 07:51:59 enami Exp $");

#include "opt_ktrace.h"
#include "opt_compat_mach.h"

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
#include <sys/ioctl.h>

#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#ifdef KTRACE

void	ktrinitheader(struct ktr_header *, struct proc *, int);
int	ktrwrite(struct proc *, struct ktr_header *);
int	ktrace_common(struct proc *, int, int, int, struct file *);
int	ktrops(struct proc *, struct proc *, int, int, struct file *);
int	ktrsetchildren(struct proc *, struct proc *, int, int,
	    struct file *);
int	ktrcanset(struct proc *, struct proc *);
int	ktrsamefile(struct file *, struct file *);

/*
 * "deep" compare of two files for the purposes of clearing a trace.
 * Returns true if they're the same open file, or if they point at the
 * same underlying vnode/socket.
 */

int
ktrsamefile(struct file *f1, struct file *f2)
{

	return ((f1 == f2) ||
	    ((f1 != NULL) && (f2 != NULL) &&
		(f1->f_type == f2->f_type) &&
		(f1->f_data == f2->f_data)));
}

void
ktrderef(struct proc *p)
{
	struct file *fp = p->p_tracep;
	p->p_traceflag = 0;
	if (fp == NULL)
		return;
	p->p_tracep = NULL;

	simple_lock(&fp->f_slock);
	FILE_USE(fp);

	/*
	 * ktrace file descriptor can't be watched (are not visible to
	 * userspace), so no kqueue stuff here
	 */
	closef(fp, NULL);
}

void
ktradref(struct proc *p)
{
	struct file *fp = p->p_tracep;

	fp->f_count++;
}

void
ktrinitheader(struct ktr_header *kth, struct proc *p, int type)
{

	memset(kth, 0, sizeof(*kth));
	kth->ktr_type = type;
	microtime(&kth->ktr_time);
	kth->ktr_pid = p->p_pid;
	memcpy(kth->ktr_comm, p->p_comm, MAXCOMLEN);
}

void
ktrsyscall(struct proc *p, register_t code, register_t realcode,
    const struct sysent *callp, register_t args[])
{
	struct ktr_header kth;
	struct ktr_syscall *ktp;
	register_t *argp;
	int argsize;
	size_t len;
	u_int i;

	if (callp == NULL)
		callp = p->p_emul->e_sysent;

	argsize = callp[code].sy_argsize;
#ifdef _LP64
	if (p->p_flag & P_32)
		argsize = argsize << 1;
#endif
	len = sizeof(struct ktr_syscall) + argsize;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_SYSCALL);
	ktp = malloc(len, M_TEMP, M_WAITOK);
	ktp->ktr_code = realcode;
	ktp->ktr_argsize = argsize;
	argp = (register_t *)((char *)ktp + sizeof(struct ktr_syscall));
	for (i = 0; i < (argsize / sizeof(*argp)); i++)
		*argp++ = args[i];
	kth.ktr_buf = (caddr_t)ktp;
	kth.ktr_len = len;
	(void) ktrwrite(p, &kth);
	free(ktp, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrsysret(struct proc *p, register_t code, int error, register_t *retval)
{
	struct ktr_header kth;
	struct ktr_sysret ktp;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_SYSRET);
	ktp.ktr_code = code;
	ktp.ktr_eosys = 0;			/* XXX unused */
	ktp.ktr_error = error;
	ktp.ktr_retval = retval ? retval[0] : 0;
	ktp.ktr_retval_1 = retval ? retval[1] : 0;

	kth.ktr_buf = (caddr_t)&ktp;
	kth.ktr_len = sizeof(struct ktr_sysret);

	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrnamei(struct proc *p, char *path)
{
	struct ktr_header kth;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_NAMEI);
	kth.ktr_len = strlen(path);
	kth.ktr_buf = path;

	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktremul(struct proc *p)
{
	struct ktr_header kth;
	const char *emul = p->p_emul->e_name;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_EMUL);
	kth.ktr_len = strlen(emul);
	kth.ktr_buf = (caddr_t)emul;

	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrkmem(struct proc *p, int ktr, const void *buf, size_t len)
{
	struct ktr_header kth;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, ktr);
	kth.ktr_len = len;
	kth.ktr_buf = buf;

	(void)ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrgenio(struct proc *p, int fd, enum uio_rw rw, struct iovec *iov,
    int len, int error)
{
	struct ktr_header kth;
	struct ktr_genio *ktp;
	caddr_t cp;
	int resid = len, cnt;
	int buflen;

	if (error)
		return;

	p->p_traceflag |= KTRFAC_ACTIVE;

	buflen = min(PAGE_SIZE, len + sizeof(struct ktr_genio));

	ktrinitheader(&kth, p, KTR_GENIO);
	ktp = malloc(buflen, M_TEMP, M_WAITOK);
	ktp->ktr_fd = fd;
	ktp->ktr_rw = rw;

	kth.ktr_buf = (caddr_t)ktp;

	cp = (caddr_t)((char *)ktp + sizeof(struct ktr_genio));
	buflen -= sizeof(struct ktr_genio);

	while (resid > 0) {
#if 0 /* XXX NJWLWP */
		KDASSERT(p->p_cpu != NULL);
		KDASSERT(p->p_cpu == curcpu());
#endif
		/* XXX NJWLWP */
		if (curcpu()->ci_schedstate.spc_flags & SPCF_SHOULDYIELD)
			preempt(1);

		cnt = min(iov->iov_len, buflen);
		if (cnt > resid)
			cnt = resid;
		if (copyin(iov->iov_base, cp, cnt))
			break;

		kth.ktr_len = cnt + sizeof(struct ktr_genio);

		if (__predict_false(ktrwrite(p, &kth) != 0))
			break;

		iov->iov_base = (caddr_t)iov->iov_base + cnt;
		iov->iov_len -= cnt;

		if (iov->iov_len == 0)
			iov++;

		resid -= cnt;
	}

	free(ktp, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrpsig(struct proc *p, int sig, sig_t action, const sigset_t *mask,
    const ksiginfo_t *ksi)
{
	struct ktr_header kth;
	struct {
		struct ktr_psig	kp;
		siginfo_t	si;
	} kbuf;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_PSIG);
	kbuf.kp.signo = (char)sig;
	kbuf.kp.action = action;
	kbuf.kp.mask = *mask;
	kth.ktr_buf = (caddr_t)&kbuf;
	if (ksi) {
		kbuf.kp.code = KSI_TRAPCODE(ksi);
		(void)memset(&kbuf.si, 0, sizeof(kbuf.si));
		kbuf.si._info = ksi->ksi_info;
		kth.ktr_len = sizeof(kbuf);
	} else {
		kbuf.kp.code = 0;
		kth.ktr_len = sizeof(struct ktr_psig);
	}
	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrcsw(struct proc *p, int out, int user)
{
	struct ktr_header kth;
	struct ktr_csw kc;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_CSW);
	kc.out = out;
	kc.user = user;
	kth.ktr_buf = (caddr_t)&kc;
	kth.ktr_len = sizeof(struct ktr_csw);

	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktruser(struct proc *p, const char *id, void *addr, size_t len, int ustr)
{
	struct ktr_header kth;
	struct ktr_user *ktp;
	caddr_t user_dta;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_USER);
	ktp = malloc(sizeof(struct ktr_user) + len, M_TEMP, M_WAITOK);
	if (ustr) {
		if (copyinstr(id, ktp->ktr_id, KTR_USER_MAXIDLEN, NULL) != 0)
			ktp->ktr_id[0] = '\0';
	} else
		strncpy(ktp->ktr_id, id, KTR_USER_MAXIDLEN);
	ktp->ktr_id[KTR_USER_MAXIDLEN-1] = '\0';

	user_dta = (caddr_t) ((char *)ktp + sizeof(struct ktr_user));
	if (copyin(addr, (void *) user_dta, len) != 0)
		len = 0;

	kth.ktr_buf = (void *)ktp;
	kth.ktr_len = sizeof(struct ktr_user) + len;
	(void) ktrwrite(p, &kth);

	free(ktp, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;

}

void
ktrmmsg(struct proc *p, const void *msgh, size_t size)
{
	struct ktr_header kth;
	struct ktr_mmsg	*kp;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_MMSG);

	kp = (struct ktr_mmsg *)msgh;
	kth.ktr_buf = (caddr_t)kp;
	kth.ktr_len = size;
	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrmool(struct proc *p, const void *kaddr, size_t size, const void *uaddr)
{
	struct ktr_header kth;
	struct ktr_mool *kp;
	struct ktr_mool *buf;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_MOOL);

	kp = malloc(size + sizeof(*kp), M_TEMP, M_WAITOK);
	kp->uaddr = uaddr;
	kp->size = size;
	buf = kp + 1; /* Skip uaddr and size */
	memcpy(buf, kaddr, size);

	kth.ktr_buf = (caddr_t)kp;
	kth.ktr_len = size + sizeof(*kp);
	(void) ktrwrite(p, &kth);
	free(kp, M_TEMP);

	p->p_traceflag &= ~KTRFAC_ACTIVE;
}


/* Interface and common routines */

int
ktrace_common(struct proc *curp, int ops, int facs, int pid, struct file *fp)
{
	int ret = 0;
	int error = 0;
	int one = 1;
	int descend;
	struct proc *p;
	struct pgrp *pg;

	curp->p_traceflag |= KTRFAC_ACTIVE;
	descend = ops & KTRFLAG_DESCEND;
	facs = facs & ~((unsigned) KTRFAC_ROOT);

	/*
	 * Clear all uses of the tracefile
	 */
	if (KTROP(ops) == KTROP_CLEARFILE) {
		proclist_lock_read();
		LIST_FOREACH(p, &allproc, p_list) {
			if (ktrsamefile(p->p_tracep, fp)) {
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
	 * Mark fp non-blocking, to avoid problems from possible deadlocks.
	 */

	if (fp != NULL) {
		fp->f_flag |= FNONBLOCK;
		(*fp->f_ops->fo_ioctl)(fp, FIONBIO, (caddr_t)&one, curp);
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
	if (pid < 0) {
		/*
		 * by process group
		 */
		pg = pg_find(-pid, PFIND_UNLOCK_FAIL);
		if (pg == NULL) {
			error = ESRCH;
			goto done;
		}
		LIST_FOREACH(p, &pg->pg_members, p_pglist) {
			if (descend)
				ret |= ktrsetchildren(curp, p, ops, facs, fp);
			else
				ret |= ktrops(curp, p, ops, facs, fp);
		}

	} else {
		/*
		 * by pid
		 */
		p = p_find(pid, PFIND_UNLOCK_FAIL);
		if (p == NULL) {
			error = ESRCH;
			goto done;
		}
		if (descend)
			ret |= ktrsetchildren(curp, p, ops, facs, fp);
		else
			ret |= ktrops(curp, p, ops, facs, fp);
	}
	proclist_unlock_read();	/* taken by p{g}_find */
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
sys_fktrace(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fktrace_args /* {
		syscallarg(int) fd;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
	struct proc *curp = l->l_proc;
	struct file *fp = NULL;
	struct filedesc *fdp = curp->p_fd;
	int error;

	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);

	FILE_USE(fp);

	if ((fp->f_flag & FWRITE) == 0)
		error = EBADF;
	else
		error = ktrace_common(curp, SCARG(uap, ops),
		    SCARG(uap, facs), SCARG(uap, pid), fp);

	FILE_UNUSE(fp, curp);

	return error;
}

/*
 * ktrace system call
 */
/* ARGSUSED */
int
sys_ktrace(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ktrace_args /* {
		syscallarg(const char *) fname;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
	struct proc *curp = l->l_proc;
	struct vnode *vp = NULL;
	struct file *fp = NULL;
	int fd;
	int ops = SCARG(uap, ops);
	int error = 0;
	struct nameidata nd;

	ops = KTROP(ops) | (ops & KTRFLAG_DESCEND);

	curp->p_traceflag |= KTRFAC_ACTIVE;
	if ((ops & KTROP_CLEAR) == 0) {
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
		/*
		 * XXX This uses up a file descriptor slot in the
		 * tracing process for the duration of this syscall.
		 * This is not expected to be a problem.  If
		 * falloc(NULL, ...) DTRT we could skip that part, but
		 * that would require changing its interface to allow
		 * the caller to pass in a ucred..
		 *
		 * This will FILE_USE the fp it returns, if any.
		 * Keep it in use until we return.
		 */
		if ((error = falloc(curp, &fp, &fd)) != 0)
			goto done;

		fp->f_flag = FWRITE|FAPPEND;
		fp->f_type = DTYPE_VNODE;
		fp->f_ops = &vnops;
		fp->f_data = (caddr_t)vp;
		FILE_SET_MATURE(fp);
		vp = NULL;
	}
	error = ktrace_common(curp, SCARG(uap, ops), SCARG(uap, facs),
	    SCARG(uap, pid), fp);
done:
	if (vp != NULL)
		(void) vn_close(vp, FWRITE, curp->p_ucred, curp);
	if (fp != NULL) {
		FILE_UNUSE(fp, curp);	/* release file */
		fdrelease(curp, fd); 	/* release fd table slot */
	}
	return (error);
}

int
ktrops(struct proc *curp, struct proc *p, int ops, int facs,
    struct file *fp)
{

	if (!ktrcanset(curp, p))
		return (0);
	if (KTROP(ops) == KTROP_SET) {
		if (p->p_tracep != fp) {
			/*
			 * if trace file already in use, relinquish
			 */
			ktrderef(p);
			p->p_tracep = fp;
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
		p->p_traceflag |= KTRFAC_TRC_EMUL;
#ifdef __HAVE_SYSCALL_INTERN
	(*p->p_emul->e_syscall_intern)(p);
#endif

	return (1);
}

int
ktrsetchildren(struct proc *curp, struct proc *top, int ops, int facs,
    struct file *fp)
{
	struct proc *p;
	int ret = 0;

	p = top;
	for (;;) {
		ret |= ktrops(curp, p, ops, facs, fp);
		/*
		 * If this process has children, descend to them next,
		 * otherwise do any siblings, and if done with this level,
		 * follow back up the tree (but not past top).
		 */
		if (LIST_FIRST(&p->p_children) != NULL) {
			p = LIST_FIRST(&p->p_children);
			continue;
		}
		for (;;) {
			if (p == top)
				return (ret);
			if (LIST_NEXT(p, p_sibling) != NULL) {
				p = LIST_NEXT(p, p_sibling);
				break;
			}
			p = p->p_pptr;
		}
	}
	/*NOTREACHED*/
}

int
ktrwrite(struct proc *p, struct ktr_header *kth)
{
	struct uio auio;
	struct iovec aiov[2];
	int error, tries;
	struct file *fp = p->p_tracep;

	if (fp == NULL)
		return 0;

	if (p->p_traceflag & KTRFAC_TRC_EMUL) {
		/* Add emulation trace before first entry for this process */
		p->p_traceflag &= ~KTRFAC_TRC_EMUL;
		ktremul(p);
	}

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
		aiov[1].iov_base = (void *)kth->ktr_buf;
		aiov[1].iov_len = kth->ktr_len;
		auio.uio_resid += kth->ktr_len;
	}

	simple_lock(&fp->f_slock);
	FILE_USE(fp);

	tries = 0;
	do {
		error = (*fp->f_ops->fo_write)(fp, &fp->f_offset, &auio,
		    fp->f_cred, FOF_UPDATE_OFFSET);
		tries++;
		if (error == EWOULDBLOCK)
			preempt(1);
	} while ((error == EWOULDBLOCK) && (tries < 3));
	FILE_UNUSE(fp, NULL);

	if (__predict_true(error == 0))
		return (0);
	/*
	 * If error encountered, give up tracing on this vnode.  Don't report
	 * EPIPE as this can easily happen with fktrace()/ktruss.
	 */
	if (error != EPIPE)
		log(LOG_NOTICE,
		    "ktrace write failed, errno %d, tracing stopped\n",
		    error);
	proclist_lock_read();
	LIST_FOREACH(p, &allproc, p_list) {
		if (ktrsamefile(p->p_tracep, fp))
			ktrderef(p);
	}
	proclist_unlock_read();

	return (error);
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
ktrcanset(struct proc *callp, struct proc *targetp)
{
	struct pcred *caller = callp->p_cred;
	struct pcred *target = targetp->p_cred;

	if ((caller->pc_ucred->cr_uid == target->p_ruid &&
	    target->p_ruid == target->p_svuid &&
	    caller->p_rgid == target->p_rgid &&	/* XXX */
	    target->p_rgid == target->p_svgid &&
	    (targetp->p_traceflag & KTRFAC_ROOT) == 0 &&
	    (targetp->p_flag & P_SUGID) == 0) ||
	    caller->pc_ucred->cr_uid == 0)
		return (1);

	return (0);
}
#endif /* KTRACE */

/*
 * Put user defined entry to ktrace records.
 */
int
sys_utrace(struct lwp *l, void *v, register_t *retval)
{
#ifdef KTRACE
	struct sys_utrace_args /* {
		syscallarg(const char *) label;
		syscallarg(void *) addr;
		syscallarg(size_t) len;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	if (!KTRPOINT(p, KTR_USER))
		return (0);

	if (SCARG(uap, len) > KTR_USER_MAXLEN)
		return (EINVAL);

	ktruser(p, SCARG(uap, label), SCARG(uap, addr), SCARG(uap, len), 1);

	return (0);
#else /* !KTRACE */
	return ENOSYS;
#endif /* KTRACE */
}
