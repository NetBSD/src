/*	$NetBSD: kern_descrip.c,v 1.61.8.1 1999/12/27 18:35:51 wrstuden Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
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
 *	@(#)kern_descrip.c	8.8 (Berkeley) 2/14/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/syslog.h>
#include <sys/unistd.h>
#include <sys/resourcevar.h>
#include <sys/conf.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <vm/vm.h>

/*
 * Descriptor management.
 */
struct filelist filehead;	/* head of list of open files */
int nfiles;			/* actual number of open files */
struct pool file_pool;		/* memory pool for file structures */
struct pool cwdi_pool;		/* memory pool for cwdinfo structures */

static __inline void fd_used __P((struct filedesc *, int));
static __inline void fd_unused __P((struct filedesc *, int));
int finishdup __P((struct proc *, int, int, register_t *));

static __inline void
fd_used(fdp, fd)
	register struct filedesc *fdp;
	register int fd;
{

	if (fd > fdp->fd_lastfile)
		fdp->fd_lastfile = fd;
}

static __inline void
fd_unused(fdp, fd)
	register struct filedesc *fdp;
	register int fd;
{

	if (fd < fdp->fd_freefile)
		fdp->fd_freefile = fd;
#ifdef DIAGNOSTIC
	if (fd > fdp->fd_lastfile)
		panic("fd_unused: fd_lastfile inconsistent");
#endif
	if (fd == fdp->fd_lastfile) {
		do {
			fd--;
		} while (fd >= 0 && fdp->fd_ofiles[fd] == NULL);
		fdp->fd_lastfile = fd;
	}
}

/*
 * System calls on descriptors.
 */

/*
 * Duplicate a file descriptor.
 */
/* ARGSUSED */
int
sys_dup(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_dup_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct file *fp;
	register struct filedesc *fdp = p->p_fd;
	register int old = SCARG(uap, fd);
	int new;
	int error;

	if ((u_int)old >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[old]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
		return (EBADF);

	FILE_USE(fp);

	if ((error = fdalloc(p, 0, &new)) != 0) {
		FILE_UNUSE(fp, p);
		return (error);
	}

	/* finishdup() will unuse the descriptors for us */
	return (finishdup(p, old, new, retval));
}

/*
 * Duplicate a file descriptor to a particular value.
 */
/* ARGSUSED */
int
sys_dup2(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_dup2_args /* {
		syscallarg(int) from;
		syscallarg(int) to;
	} */ *uap = v;
	struct file *fp;
	register struct filedesc *fdp = p->p_fd;
	register int old = SCARG(uap, from), new = SCARG(uap, to);
	int i, error;

	if ((u_int)old >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[old]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0 ||
	    (u_int)new >= p->p_rlimit[RLIMIT_NOFILE].rlim_cur ||
	    (u_int)new >= maxfiles)
		return (EBADF);
	if (old == new) {
		*retval = new;
		return (0);
	}

	FILE_USE(fp);

	if (new >= fdp->fd_nfiles) {
		if ((error = fdalloc(p, new, &i)) != 0) {
			FILE_UNUSE(fp, p);
			return (error);
		}
		if (new != i)
			panic("dup2: fdalloc");
	} else {
		(void) fdrelease(p, new);
	}

	/* finishdup() will unuse the descriptors for us */
	return (finishdup(p, old, new, retval));
}

int	fcntl_forfs	__P((int, struct proc *, int, void *));

/*
 * The file control system call.
 */
/* ARGSUSED */
int
sys_fcntl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys_fcntl_args /* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	register struct filedesc *fdp = p->p_fd;
	register struct file *fp;
	struct vnode *vp;
	int i, tmp, error = 0, flg = F_POSIX, cmd;
	struct flock fl;
	int newmin;

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
		return (EBADF);

	FILE_USE(fp);

	cmd = SCARG(uap, cmd);
	if ((cmd & F_FSCTL)) {
		error = fcntl_forfs(fd, p, cmd, SCARG(uap, arg));
		goto out;
	}

	switch (cmd) {

	case F_DUPFD:
		newmin = (long)SCARG(uap, arg);
		if ((u_int)newmin >= p->p_rlimit[RLIMIT_NOFILE].rlim_cur ||
		    (u_int)newmin >= maxfiles) {
			error = EINVAL;
			goto out;
		}
		if ((error = fdalloc(p, newmin, &i)) != 0)
			goto out;

		/* finishdup() will unuse the descriptors for us */
		return (finishdup(p, fd, i, retval));

	case F_GETFD:
		*retval = fdp->fd_ofileflags[fd] & UF_EXCLOSE ? 1 : 0;
		break;

	case F_SETFD:
		if ((long)SCARG(uap, arg) & 1)
			fdp->fd_ofileflags[fd] |= UF_EXCLOSE;
		else
			fdp->fd_ofileflags[fd] &= ~UF_EXCLOSE;
		break;

	case F_GETFL:
		*retval = OFLAGS(fp->f_flag);
		break;

	case F_SETFL:
		tmp = FFLAGS((long)SCARG(uap, arg)) & FCNTLFLAGS;
		error = (*fp->f_ops->fo_fcntl)(fp, F_SETFL, (caddr_t)&tmp, p);
		if (error)
			goto out;
		fp->f_flag &= ~FCNTLFLAGS;
		fp->f_flag |= tmp;
		tmp = fp->f_flag & FNONBLOCK;
		error = (*fp->f_ops->fo_ioctl)(fp, FIONBIO, (caddr_t)&tmp, p);
		if (error)
			goto out;
		tmp = fp->f_flag & FASYNC;
		error = (*fp->f_ops->fo_ioctl)(fp, FIOASYNC, (caddr_t)&tmp, p);
		if (error == 0)
			goto out;
		fp->f_flag &= ~FNONBLOCK;
		tmp = 0;
		(void) (*fp->f_ops->fo_ioctl)(fp, FIONBIO, (caddr_t)&tmp, p);
		break;

	case F_GETOWN:
		if (fp->f_type == DTYPE_SOCKET) {
			*retval = ((struct socket *)fp->f_data)->so_pgid;
			goto out;
		}
		error = (*fp->f_ops->fo_ioctl)
			(fp, TIOCGPGRP, (caddr_t)retval, p);
		*retval = -*retval;
		break;

	case F_SETOWN:
		if (fp->f_type == DTYPE_SOCKET) {
			((struct socket *)fp->f_data)->so_pgid =
			    (long)SCARG(uap, arg);
			goto out;
		}
		if ((long)SCARG(uap, arg) <= 0) {
			SCARG(uap, arg) = (void *)(-(long)SCARG(uap, arg));
		} else {
			struct proc *p1 = pfind((long)SCARG(uap, arg));
			if (p1 == 0) {
				error = ESRCH;
				goto out;
			}
			SCARG(uap, arg) = (void *)(long)p1->p_pgrp->pg_id;
		}
		error = (*fp->f_ops->fo_ioctl)
		    (fp, TIOCSPGRP, (caddr_t)&SCARG(uap, arg), p);
		break;

	case F_SETLKW:
		flg |= F_WAIT;
		/* Fall into F_SETLK */

	case F_SETLK:
		if (fp->f_type != DTYPE_VNODE) {
			error = EINVAL;
			goto out;
		}
		vp = (struct vnode *)fp->f_data;
		/* Copy in the lock structure */
		error = copyin((caddr_t)SCARG(uap, arg), (caddr_t)&fl,
		    sizeof(fl));
		if (error)
			goto out;
		if (fl.l_whence == SEEK_CUR)
			fl.l_start += fp->f_offset;
		switch (fl.l_type) {
		case F_RDLCK:
			if ((fp->f_flag & FREAD) == 0) {
				error = EBADF;
				goto out;
			}
			p->p_flag |= P_ADVLOCK;
			error = VOP_ADVLOCK(vp, (caddr_t)p, F_SETLK, &fl, flg);
			goto out;

		case F_WRLCK:
			if ((fp->f_flag & FWRITE) == 0) {
				error = EBADF;
				goto out;
			}
			p->p_flag |= P_ADVLOCK;
			error = VOP_ADVLOCK(vp, (caddr_t)p, F_SETLK, &fl, flg);
			goto out;

		case F_UNLCK:
			error = VOP_ADVLOCK(vp, (caddr_t)p, F_UNLCK, &fl,
			    F_POSIX);
			goto out;

		default:
			error = EINVAL;
			goto out;
		}

	case F_GETLK:
		if (fp->f_type != DTYPE_VNODE) {
			error = EINVAL;
			goto out;
		}
		vp = (struct vnode *)fp->f_data;
		/* Copy in the lock structure */
		error = copyin((caddr_t)SCARG(uap, arg), (caddr_t)&fl,
		    sizeof(fl));
		if (error)
			goto out;
		if (fl.l_whence == SEEK_CUR)
			fl.l_start += fp->f_offset;
		if (fl.l_type != F_RDLCK &&
		    fl.l_type != F_WRLCK &&
		    fl.l_type != F_UNLCK) {
			error = EINVAL;
			goto out;
		}
		error = VOP_ADVLOCK(vp, (caddr_t)p, F_GETLK, &fl, F_POSIX);
		if (error)
			goto out;
		error = copyout((caddr_t)&fl, (caddr_t)SCARG(uap, arg),
		    sizeof(fl));
		break;

	default:
		error = EINVAL;
	}

 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Common code for dup, dup2, and fcntl(F_DUPFD).
 */
int
finishdup(p, old, new, retval)
	struct proc *p;
	int old, new;
	register_t *retval;
{
	struct filedesc *fdp = p->p_fd;
	struct file *fp;

	/*
	 * Note: `old' is already used for us.
	 */

	fp = fdp->fd_ofiles[old];
	fdp->fd_ofiles[new] = fp;
	fdp->fd_ofileflags[new] = fdp->fd_ofileflags[old] &~ UF_EXCLOSE;
	fp->f_count++;
	fd_used(fdp, new);
	*retval = new;
	FILE_UNUSE(fp, p);
	return (0);
}

int
fdrelease(p, fd)
	struct proc *p;
	int fd;
{
	register struct filedesc *fdp = p->p_fd;
	register struct file **fpp, *fp;
	register char *pf;

	fpp = &fdp->fd_ofiles[fd];
	fp = *fpp;
	if (fp == NULL)
		return (EBADF);

	FILE_USE(fp);

	pf = &fdp->fd_ofileflags[fd];
	if (*pf & UF_MAPPED) {
		/* XXX: USELESS? XXXCDC check it */
		p->p_fd->fd_ofileflags[fd] &= ~UF_MAPPED;
	}
	*fpp = NULL;
	*pf = 0;
	fd_unused(fdp, fd);
	return (closef(fp, p));
}

/*
 * Close a file descriptor.
 */
/* ARGSUSED */
int
sys_close(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_close_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	register struct filedesc *fdp = p->p_fd;

	if ((u_int)fd >= fdp->fd_nfiles)
		return (EBADF);
	return (fdrelease(p, fd));
}

/*
 * Return status information about a file descriptor.
 */
/* ARGSUSED */
int
sys___fstat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys___fstat13_args /* {
		syscallarg(int) fd;
		syscallarg(struct stat *) sb;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	register struct filedesc *fdp = p->p_fd;
	register struct file *fp;
	struct stat ub;
	int error;

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
		return (EBADF);

	FILE_USE(fp);

	switch (fp->f_type) {

	case DTYPE_VNODE:
		error = vn_stat((struct vnode *)fp->f_data, &ub, p);
		break;

	case DTYPE_SOCKET:
		error = soo_stat((struct socket *)fp->f_data, &ub);
		break;

	default:
		panic("fstat");
		/*NOTREACHED*/
	}
	if (error == 0)
		error = copyout(&ub, SCARG(uap, sb), sizeof(ub));
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Return pathconf information about a file descriptor.
 */
/* ARGSUSED */
int
sys_fpathconf(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys_fpathconf_args /* {
		syscallarg(int) fd;
		syscallarg(int) name;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	int error = 0;

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
		return (EBADF);

	FILE_USE(fp);

	switch (fp->f_type) {

	case DTYPE_SOCKET:
		if (SCARG(uap, name) != _PC_PIPE_BUF)
			error = EINVAL;
		else
			*retval = PIPE_BUF;
		break;

	case DTYPE_VNODE:
		vp = (struct vnode *)fp->f_data;
		error = VOP_PATHCONF(vp, SCARG(uap, name), retval);
		break;

	default:
		panic("fpathconf");
	}

	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Allocate a file descriptor for the process.
 */
int fdexpand;

int
fdalloc(p, want, result)
	struct proc *p;
	int want;
	int *result;
{
	register struct filedesc *fdp = p->p_fd;
	register int i;
	int lim, last, nfiles;
	struct file **newofile;
	char *newofileflags;

	/*
	 * Search for a free descriptor starting at the higher
	 * of want or fd_freefile.  If that fails, consider
	 * expanding the ofile array.
	 */
	lim = min((int)p->p_rlimit[RLIMIT_NOFILE].rlim_cur, maxfiles);
	for (;;) {
		last = min(fdp->fd_nfiles, lim);
		if ((i = want) < fdp->fd_freefile)
			i = fdp->fd_freefile;
		for (; i < last; i++) {
			if (fdp->fd_ofiles[i] == NULL) {
				fd_used(fdp, i);
				if (want <= fdp->fd_freefile)
					fdp->fd_freefile = i;
				*result = i;
				return (0);
			}
		}

		/*
		 * No space in current array.  Expand?
		 */
		if (fdp->fd_nfiles >= lim)
			return (EMFILE);
		if (fdp->fd_nfiles < NDEXTENT)
			nfiles = NDEXTENT;
		else
			nfiles = 2 * fdp->fd_nfiles;
		MALLOC(newofile, struct file **, nfiles * OFILESIZE,
		    M_FILEDESC, M_WAITOK);
		newofileflags = (char *) &newofile[nfiles];
		/*
		 * Copy the existing ofile and ofileflags arrays
		 * and zero the new portion of each array.
		 */
		memcpy(newofile, fdp->fd_ofiles,
			(i = sizeof(struct file *) * fdp->fd_nfiles));
		memset((char *)newofile + i, 0, nfiles * sizeof(struct file *) - i);
		memcpy(newofileflags, fdp->fd_ofileflags,
			(i = sizeof(char) * fdp->fd_nfiles));
		memset(newofileflags + i, 0, nfiles * sizeof(char) - i);
		if (fdp->fd_nfiles > NDFILE)
			FREE(fdp->fd_ofiles, M_FILEDESC);
		fdp->fd_ofiles = newofile;
		fdp->fd_ofileflags = newofileflags;
		fdp->fd_nfiles = nfiles;
		fdexpand++;
	}
}

/*
 * Check to see whether n user file descriptors
 * are available to the process p.
 */
int
fdavail(p, n)
	struct proc *p;
	register int n;
{
	register struct filedesc *fdp = p->p_fd;
	register struct file **fpp;
	register int i, lim;

	lim = min((int)p->p_rlimit[RLIMIT_NOFILE].rlim_cur, maxfiles);
	if ((i = lim - fdp->fd_nfiles) > 0 && (n -= i) <= 0)
		return (1);
	fpp = &fdp->fd_ofiles[fdp->fd_freefile];
	for (i = min(lim,fdp->fd_nfiles) - fdp->fd_freefile; --i >= 0; fpp++)
		if (*fpp == NULL && --n <= 0)
			return (1);
	return (0);
}

/*
 * Initialize the data structures necessary for managing files.
 */
void
finit()
{

	pool_init(&file_pool, sizeof(struct file), 0, 0, 0, "filepl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_FILE);
	pool_init(&cwdi_pool, sizeof(struct cwdinfo), 0, 0, 0, "cwdipl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_FILEDESC);
}

/*
 * Create a new open file structure and allocate
 * a file decriptor for the process that refers to it.
 */
int
falloc(p, resultfp, resultfd)
	register struct proc *p;
	struct file **resultfp;
	int *resultfd;
{
	register struct file *fp, *fq;
	int error, i;

	if ((error = fdalloc(p, 0, &i)) != 0)
		return (error);
	if (nfiles >= maxfiles) {
		tablefull("file");
		return (ENFILE);
	}
	/*
	 * Allocate a new file descriptor.
	 * If the process has file descriptor zero open, add to the list
	 * of open files at that point, otherwise put it at the front of
	 * the list of open files.
	 */
	nfiles++;
	fp = pool_get(&file_pool, PR_WAITOK);
	memset(fp, 0, sizeof(struct file));
	if ((fq = p->p_fd->fd_ofiles[0]) != NULL) {
		LIST_INSERT_AFTER(fq, fp, f_list);
	} else {
		LIST_INSERT_HEAD(&filehead, fp, f_list);
	}
	p->p_fd->fd_ofiles[i] = fp;
	fp->f_count = 1;
	fp->f_cred = p->p_ucred;
	crhold(fp->f_cred);
	if (resultfp) {
		FILE_USE(fp);
		*resultfp = fp;
	}
	if (resultfd)
		*resultfd = i;
	return (0);
}

/*
 * Free a file descriptor.
 */
void
ffree(fp)
	register struct file *fp;
{

#ifdef DIAGNOSTIC
	if (fp->f_usecount)
		panic("ffree");
#endif

	LIST_REMOVE(fp, f_list);
	crfree(fp->f_cred);
#ifdef DIAGNOSTIC
	fp->f_count = 0;
#endif
	nfiles--;
	pool_put(&file_pool, fp);
}

/*
 * Create an initial cwdinfo structure, using the same current and root
 * directories as p.
 */
struct cwdinfo *
cwdinit(p)
	struct proc *p;
{
	struct cwdinfo *cwdi;

	cwdi = pool_get(&cwdi_pool, PR_WAITOK);

	cwdi->cwdi_cdir = p->p_cwdi->cwdi_cdir;
	VREF(cwdi->cwdi_cdir);
	cwdi->cwdi_rdir = p->p_cwdi->cwdi_rdir;
	if (cwdi->cwdi_rdir)
		VREF(cwdi->cwdi_rdir);
	cwdi->cwdi_cmask =  p->p_cwdi->cwdi_cmask;
	cwdi->cwdi_refcnt = 1;

	return (cwdi);
}

/*
 * Make p2 share p1's cwdinfo.
 */
void
cwdshare(p1, p2)
	struct proc *p1, *p2;
{

	p2->p_cwdi = p1->p_cwdi;
	p1->p_cwdi->cwdi_refcnt++;
}

/*
 * Make this process not share its cwdinfo structure, maintaining
 * all cwdinfo state.
 */
void
cwdunshare(p)
	struct proc *p;
{
	struct cwdinfo *newcwdi;

	if (p->p_cwdi->cwdi_refcnt == 1)
		return;

	newcwdi = cwdinit(p);
	cwdfree(p);
	p->p_cwdi = newcwdi;
}

/*
 * Release a cwdinfo structure.
 */
void
cwdfree(p)
	struct proc *p;
{
	struct cwdinfo *cwdi = p->p_cwdi;

	if (--cwdi->cwdi_refcnt > 0)
		return;

	p->p_cwdi = NULL;

	vrele(cwdi->cwdi_cdir);
	if (cwdi->cwdi_rdir)
		vrele(cwdi->cwdi_rdir);
	pool_put(&cwdi_pool, cwdi);
}

/*
 * Create an initial filedesc structure, using the same current and root
 * directories as p.
 */
struct filedesc *
fdinit(p)
	struct proc *p;
{
	struct filedesc0 *newfdp;

	MALLOC(newfdp, struct filedesc0 *, sizeof(struct filedesc0),
	    M_FILEDESC, M_WAITOK);
	memset(newfdp, 0, sizeof(struct filedesc0));

	fdinit1(newfdp);

	return (&newfdp->fd_fd);
}

/*
 * Initialize a file descriptor table.
 */
void
fdinit1(newfdp)
	struct filedesc0 *newfdp;
{

	newfdp->fd_fd.fd_refcnt = 1;
	newfdp->fd_fd.fd_ofiles = newfdp->fd_dfiles;
	newfdp->fd_fd.fd_ofileflags = newfdp->fd_dfileflags;
	newfdp->fd_fd.fd_nfiles = NDFILE;
}

/*
 * Make p2 share p1's filedesc structure.
 */
void
fdshare(p1, p2)
	struct proc *p1, *p2;
{

	p2->p_fd = p1->p_fd;
	p1->p_fd->fd_refcnt++;
}

/*
 * Make this process not share its filedesc structure, maintaining
 * all file descriptor state.
 */
void
fdunshare(p)
	struct proc *p;
{
	struct filedesc *newfd;

	if (p->p_fd->fd_refcnt == 1)
		return;

	newfd = fdcopy(p);
	fdfree(p);
	p->p_fd = newfd;
}

/*
 * Clear a process's fd table.
 */
void
fdclear(p)
	struct proc *p;
{
	struct filedesc *newfd;

	newfd = fdinit(p);
	fdfree(p);
	p->p_fd = newfd;
}

/*
 * Copy a filedesc structure.
 */
struct filedesc *
fdcopy(p)
	struct proc *p;
{
	register struct filedesc *newfdp, *fdp = p->p_fd;
	register struct file **fpp;
	register int i;

	MALLOC(newfdp, struct filedesc *, sizeof(struct filedesc0),
	    M_FILEDESC, M_WAITOK);
	memcpy(newfdp, fdp, sizeof(struct filedesc));
	newfdp->fd_refcnt = 1;

	/*
	 * If the number of open files fits in the internal arrays
	 * of the open file structure, use them, otherwise allocate
	 * additional memory for the number of descriptors currently
	 * in use.
	 */
	if (newfdp->fd_lastfile < NDFILE) {
		newfdp->fd_ofiles = ((struct filedesc0 *) newfdp)->fd_dfiles;
		newfdp->fd_ofileflags =
		    ((struct filedesc0 *) newfdp)->fd_dfileflags;
		i = NDFILE;
	} else {
		/*
		 * Compute the smallest multiple of NDEXTENT needed
		 * for the file descriptors currently in use,
		 * allowing the table to shrink.
		 */
		i = newfdp->fd_nfiles;
		while (i >= 2 * NDEXTENT && i > newfdp->fd_lastfile * 2)
			i /= 2;
		MALLOC(newfdp->fd_ofiles, struct file **, i * OFILESIZE,
		    M_FILEDESC, M_WAITOK);
		newfdp->fd_ofileflags = (char *) &newfdp->fd_ofiles[i];
	}
	newfdp->fd_nfiles = i;
	memcpy(newfdp->fd_ofiles, fdp->fd_ofiles, i * sizeof(struct file **));
	memcpy(newfdp->fd_ofileflags, fdp->fd_ofileflags, i * sizeof(char));
	fpp = newfdp->fd_ofiles;
	for (i = newfdp->fd_lastfile; i >= 0; i--, fpp++)
		if (*fpp != NULL)
			(*fpp)->f_count++;
	return (newfdp);
}

/*
 * Release a filedesc structure.
 */
void
fdfree(p)
	struct proc *p;
{
	register struct filedesc *fdp = p->p_fd;
	register struct file **fpp, *fp;
	register int i;

	if (--fdp->fd_refcnt > 0)
		return;
	fpp = fdp->fd_ofiles;
	for (i = fdp->fd_lastfile; i >= 0; i--, fpp++) {
		fp = *fpp;
		if (fp != NULL) {
			*fpp = NULL;
			FILE_USE(fp);
			(void) closef(fp, p);
		}
	}
	p->p_fd = NULL;
	if (fdp->fd_nfiles > NDFILE)
		FREE(fdp->fd_ofiles, M_FILEDESC);
	FREE(fdp, M_FILEDESC);
}

/*
 * Internal form of close.
 * Decrement reference count on file structure.
 * Note: p may be NULL when closing a file
 * that was being passed in a message.
 *
 * Note: we expect the caller is holding a usecount, and expects us
 * to drop it (the caller thinks the file is going away forever).
 */
int
closef(fp, p)
	register struct file *fp;
	register struct proc *p;
{
	struct vnode *vp;
	struct flock lf;
	int error;

	if (fp == NULL)
		return (0);

	/*
	 * POSIX record locking dictates that any close releases ALL
	 * locks owned by this process.  This is handled by setting
	 * a flag in the unlock to free ONLY locks obeying POSIX
	 * semantics, and not to free BSD-style file locks.
	 * If the descriptor was in a message, POSIX-style locks
	 * aren't passed with the descriptor.
	 */
	if (p && (p->p_flag & P_ADVLOCK) && fp->f_type == DTYPE_VNODE) {
		lf.l_whence = SEEK_SET;
		lf.l_start = 0;
		lf.l_len = 0;
		lf.l_type = F_UNLCK;
		vp = (struct vnode *)fp->f_data;
		(void) VOP_ADVLOCK(vp, (caddr_t)p, F_UNLCK, &lf, F_POSIX);
	}

	/*
	 * If WANTCLOSE is set, then the reference count on the file
	 * is 0, but there were multiple users of the file.  This can
	 * happen if a filedesc structure is shared by multiple
	 * processes.
	 */
	if (fp->f_iflags & FIF_WANTCLOSE) {
		/*
		 * Another user of the file is already closing, and is
		 * simply waiting for other users of the file to drain.
		 * Release our usecount, and wake up the closer if it
		 * is the only remaining use.
		 */
#ifdef DIAGNOSTIC
		if (fp->f_count != 0)
			panic("closef: wantclose and count != 0");
		if (fp->f_usecount < 2)
			panic("closef: wantclose and usecount < 2");
#endif
		if (--fp->f_usecount == 1)
			wakeup(&fp->f_usecount);
		return (0);
	} else {
		/*
		 * Decrement the reference count.  If we were not the
		 * last reference, then release our use and just
		 * return.
		 */
		if (--fp->f_count > 0) {
#ifdef DIAGNOSTIC
			if (fp->f_usecount < 1)
				panic("closef: no wantclose and usecount < 1");
#endif
			fp->f_usecount--;
			return (0);
		}
		if (fp->f_count < 0)
			panic("closef: count < 0");
	}

	/*
	 * The reference count is now 0.  However, there may be
	 * multiple potential users of this file.  This can happen
	 * if multiple processes shared a single filedesc structure.
	 *
	 * Notify these potential users that the file is closing.
	 * This will prevent them from adding additional uses to
	 * the file.
	 */
	fp->f_iflags |= FIF_WANTCLOSE;

	/*
	 * We expect the caller to add a use to the file.  So, if we
	 * are the last user, usecount will be 1.  If it is not, we
	 * must wait for the usecount to drain.  When it drains back
	 * to 1, we will be awakened so that we may proceed with the
	 * close.
	 */
#ifdef DIAGNOSTIC
	if (fp->f_usecount < 1)
		panic("closef: usecount < 1");
#endif
	while (fp->f_usecount > 1)
		(void) tsleep(&fp->f_usecount, PRIBIO, "closef", 0);
#ifdef DIAGNOSTIC
	if (fp->f_usecount != 1)
		panic("closef: usecount != 1");
#endif

	if ((fp->f_flag & FHASLOCK) && fp->f_type == DTYPE_VNODE) {
		lf.l_whence = SEEK_SET;
		lf.l_start = 0;
		lf.l_len = 0;
		lf.l_type = F_UNLCK;
		vp = (struct vnode *)fp->f_data;
		(void) VOP_ADVLOCK(vp, (caddr_t)fp, F_UNLCK, &lf, F_FLOCK);
	}
	if (fp->f_ops)
		error = (*fp->f_ops->fo_close)(fp, p);
	else
		error = 0;

	/* Nothing references the file now, drop the final use (us). */
	fp->f_usecount--;

	ffree(fp);
	return (error);
}

/*
 * Apply an advisory lock on a file descriptor.
 *
 * Just attempt to get a record lock of the requested type on
 * the entire file (l_whence = SEEK_SET, l_start = 0, l_len = 0).
 */
/* ARGSUSED */
int
sys_flock(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys_flock_args /* {
		syscallarg(int) fd;
		syscallarg(int) how;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	int how = SCARG(uap, how);
	register struct filedesc *fdp = p->p_fd;
	register struct file *fp;
	struct vnode *vp;
	struct flock lf;
	int error = 0;

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
		return (EBADF);

	FILE_USE(fp);

	if (fp->f_type != DTYPE_VNODE) {
		error = EOPNOTSUPP;
		goto out;
	}

	vp = (struct vnode *)fp->f_data;
	lf.l_whence = SEEK_SET;
	lf.l_start = 0;
	lf.l_len = 0;
	if (how & LOCK_UN) {
		lf.l_type = F_UNLCK;
		fp->f_flag &= ~FHASLOCK;
		error = VOP_ADVLOCK(vp, (caddr_t)fp, F_UNLCK, &lf, F_FLOCK);
		goto out;
	}
	if (how & LOCK_EX)
		lf.l_type = F_WRLCK;
	else if (how & LOCK_SH)
		lf.l_type = F_RDLCK;
	else {
		error = EINVAL;
		goto out;
	}
	fp->f_flag |= FHASLOCK;
	if (how & LOCK_NB)
		error = VOP_ADVLOCK(vp, (caddr_t)fp, F_SETLK, &lf, F_FLOCK);
	else
		error = VOP_ADVLOCK(vp, (caddr_t)fp, F_SETLK, &lf,
		    F_FLOCK|F_WAIT);
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * File Descriptor pseudo-device driver (/dev/fd/).
 *
 * Opening minor device N dup()s the file (if any) connected to file
 * descriptor N belonging to the calling process.  Note that this driver
 * consists of only the ``open()'' routine, because all subsequent
 * references to this file will be direct to the other driver.
 */
/* ARGSUSED */
int
filedescopen(dev, mode, type, p)
	dev_t dev;
	int mode, type;
	struct proc *p;
{

	/*
	 * XXX Kludge: set curproc->p_dupfd to contain the value of the
	 * the file descriptor being sought for duplication. The error 
	 * return ensures that the vnode for this device will be released
	 * by vn_open. Open will detect this special error and take the
	 * actions in dupfdopen below. Other callers of vn_open or VOP_OPEN
	 * will simply report the error.
	 */
	p->p_dupfd = minor(dev);
	return (ENODEV);
}

/*
 * Duplicate the specified descriptor to a free descriptor.
 */
int
dupfdopen(p, indx, dfd, mode, error)
	struct proc *p;
	int indx, dfd, mode, error;
{
	struct filedesc *fdp = p->p_fd;
	struct file *wfp;
	struct file *fp;

	/*
	 * If the to-be-dup'd fd number is greater than the allowed number
	 * of file descriptors, or the fd to be dup'd has already been
	 * closed, reject.  Note, check for new == old is necessary as
	 * falloc could allocate an already closed to-be-dup'd descriptor
	 * as the new descriptor.
	 */
	fp = fdp->fd_ofiles[indx];
	if ((u_int)dfd >= fdp->fd_nfiles ||
	    (wfp = fdp->fd_ofiles[dfd]) == NULL ||
	    (wfp->f_iflags & FIF_WANTCLOSE) != 0 ||
	    fp == wfp)
		return (EBADF);

	FILE_USE(wfp);

	/*
	 * There are two cases of interest here.
	 *
	 * For ENODEV simply dup (dfd) to file descriptor
	 * (indx) and return.
	 *
	 * For ENXIO steal away the file structure from (dfd) and
	 * store it in (indx).  (dfd) is effectively closed by
	 * this operation.
	 *
	 * Any other error code is just returned.
	 */
	switch (error) {
	case ENODEV:
		/*
		 * Check that the mode the file is being opened for is a
		 * subset of the mode of the existing descriptor.
		 */
		if (((mode & (FREAD|FWRITE)) | wfp->f_flag) != wfp->f_flag) {
			FILE_UNUSE(wfp, p);
			return (EACCES);
		}
		fdp->fd_ofiles[indx] = wfp;
		fdp->fd_ofileflags[indx] = fdp->fd_ofileflags[dfd];
		wfp->f_count++;
		fd_used(fdp, indx);
		FILE_UNUSE(wfp, p);
		return (0);

	case ENXIO:
		/*
		 * Steal away the file pointer from dfd, and stuff it into indx.
		 */
		fdp->fd_ofiles[indx] = fdp->fd_ofiles[dfd];
		fdp->fd_ofileflags[indx] = fdp->fd_ofileflags[dfd];
		fdp->fd_ofiles[dfd] = NULL;
		fdp->fd_ofileflags[dfd] = 0;
		/*
		 * Complete the clean up of the filedesc structure by
		 * recomputing the various hints.
		 */
		fd_used(fdp, indx);
		fd_unused(fdp, dfd);
		FILE_UNUSE(wfp, p);
		return (0);

	default:
		FILE_UNUSE(wfp, p);
		return (error);
	}
	/* NOTREACHED */
}

/*
 * fcntl call which is being passed to the file's fs.
 */
int
fcntl_forfs(fd, p, cmd, arg)
	int fd, cmd;
	struct proc *p;
	void *arg;
{
	register struct file *fp;
	register struct filedesc *fdp;
	register int error;
	register u_int size;
	caddr_t data, memp;
#define STK_PARAMS	128
	char stkbuf[STK_PARAMS];

	/* fd's value was validated in sys_fcntl before calling this routine */
	fdp = p->p_fd;
	fp = fdp->fd_ofiles[fd];

	if ((fp->f_flag & (FREAD | FWRITE)) == 0)
		return (EBADF);

	/*
	 * Interpret high order word to find amount of data to be
	 * copied to/from the user's address space.
	 */
	size = (size_t)F_PARAM_LEN(cmd);
	if (size > F_PARAM_MAX)
		return (EINVAL);
	memp = NULL;
	if (size > sizeof(stkbuf)) {
		memp = (caddr_t)malloc((u_long)size, M_IOCTLOPS, M_WAITOK);
		data = memp;
	} else
		data = stkbuf;
	if (cmd & F_FSIN) {
		if (size) {
			error = copyin(arg, data, size);
			if (error) {
				if (memp)
					free(memp, M_IOCTLOPS);
				return (error);
			}
		} else
			*(caddr_t *)data = arg;
	} else if ((cmd & F_FSOUT) && size)
		/*
		 * Zero the buffer so the user always
		 * gets back something deterministic.
		 */
		memset(data, 0, size);
	else if (cmd & F_FSVOID)
		*(caddr_t *)data = arg;


	error = (*fp->f_ops->fo_fcntl)(fp, cmd, data, p);

	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (error == 0 && (cmd & F_FSOUT) && size)
		error = copyout(data, arg, size);
	if (memp)
		free(memp, M_IOCTLOPS);
	return (error);
}

/*
 * Close any files on exec?
 */
void
fdcloseexec(p)
	struct proc *p;
{
	register struct filedesc *fdp = p->p_fd;
	register int fd;

	for (fd = 0; fd <= fdp->fd_lastfile; fd++)
		if (fdp->fd_ofileflags[fd] & UF_EXCLOSE)
			(void) fdrelease(p, fd);
}
