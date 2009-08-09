/*	$NetBSD: ibcs2_misc.c,v 1.108 2009/08/09 22:49:00 haad Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 * from: Header: sun_misc.c,v 1.16 93/04/07 02:46:27 torek Exp
 *
 *	@(#)sun_misc.c	8.1 (Berkeley) 6/18/93
 */

/*
 * Copyright (c) 1994, 1995, 1998 Scott Bartram
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 * from: Header: sun_misc.c,v 1.16 93/04/07 02:46:27 torek Exp
 *
 *	@(#)sun_misc.c	8.1 (Berkeley) 6/18/93
 */

/*
 * IBCS2 compatibility module.
 *
 * IBCS2 system calls that are implemented differently in BSD are
 * handled here.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibcs2_misc.c,v 1.108 2009/08/09 22:49:00 haad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/prot.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>
#include <sys/kauth.h>
#include <sys/vfs_syscalls.h>

#include <netinet/in.h>
#include <sys/syscallargs.h>

#include <miscfs/specfs/specdev.h>

#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#if defined(__i386__)
#include <i386/include/reg.h>
#endif

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_dirent.h>
#include <compat/ibcs2/ibcs2_fcntl.h>
#include <compat/ibcs2/ibcs2_mman.h>
#include <compat/ibcs2/ibcs2_time.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_timeb.h>
#include <compat/ibcs2/ibcs2_unistd.h>
#include <compat/ibcs2/ibcs2_utsname.h>
#include <compat/ibcs2/ibcs2_util.h>
#include <compat/ibcs2/ibcs2_utime.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>
#include <compat/ibcs2/ibcs2_sysi86.h>
#include <compat/ibcs2/ibcs2_exec.h>

#include <compat/sys/mount.h>

int
ibcs2_sys_ulimit(struct lwp *l, const struct ibcs2_sys_ulimit_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) cmd;
		syscallarg(int) newlimit;
	} */
	struct proc *p = l->l_proc;
	struct ibcs2_sys_sysconf_args sysconf_ua;
#ifdef notyet
	int error;
	struct rlimit rl;
	struct sys_setrlimit_args sra;
#endif
#define IBCS2_GETFSIZE		1
#define IBCS2_SETFSIZE		2
#define IBCS2_GETPSIZE		3
#define IBCS2_GETDTABLESIZE	4

	switch (SCARG(uap, cmd)) {
	case IBCS2_GETFSIZE:
		*retval = p->p_rlimit[RLIMIT_FSIZE].rlim_cur;
		return 0;
	case IBCS2_SETFSIZE:	/* XXX - fix this */
#ifdef notyet
		rl.rlim_cur = SCARG(uap, newlimit);
		SCARG(&sra, which) = RLIMIT_FSIZE;
		SCARG(&sra, rlp) = &rl;
		error = setrlimit(p, &sra, retval);
		if (!error)
			*retval = p->p_rlimit[RLIMIT_FSIZE].rlim_cur;
		else
			DPRINTF(("failed "));
		return error;
#else
		*retval = SCARG(uap, newlimit);
		return 0;
#endif
	case IBCS2_GETPSIZE:
		*retval = p->p_rlimit[RLIMIT_RSS].rlim_cur; /* XXX */
		return 0;
	case IBCS2_GETDTABLESIZE:
		SCARG(&sysconf_ua, name) = IBCS2_SC_OPEN_MAX;
		return ibcs2_sys_sysconf(l, &sysconf_ua, retval);
	default:
		return ENOSYS;
	}
}

int
ibcs2_sys_waitsys(struct lwp *l, const struct ibcs2_sys_waitsys_args *uap, register_t *retval)
{
#if defined(__i386__)
	/* {
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
	} */
#endif
	int error;
	int pid, options, status, was_zombie;

#if defined(__i386__)
#define WAITPID_EFLAGS	0x8c4	/* OF, SF, ZF, PF */
	if ((l->l_md.md_regs->tf_eflags & WAITPID_EFLAGS) == WAITPID_EFLAGS) {
		/* waitpid */
		pid = SCARG(uap, a1);
		options = SCARG(uap, a3);
	} else {
#endif
		/* wait */
		pid = WAIT_ANY;
		options = 0;
#if defined(__i386__)
	}
#endif

	error = do_sys_wait(l, &pid, &status, options, NULL, &was_zombie);
	retval[0] = pid;
	retval[1] = status;
	return error;
}

int
ibcs2_sys_execv(struct lwp *l, const struct ibcs2_sys_execv_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(char **) argp;
	} */
	struct sys_execve_args ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = NULL;

	return sys_execve(l, &ap, retval);
}

int
ibcs2_sys_execve(struct lwp *l, const struct ibcs2_sys_execve_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(char **) argp;
		syscallarg(char **) envp;
	} */
	struct sys_execve_args ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(l, &ap, retval);
}

int
ibcs2_sys_umount(struct lwp *l, const struct ibcs2_sys_umount_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) name;
	} */
	struct sys_unmount_args um;

	SCARG(&um, path) = SCARG(uap, name);
	SCARG(&um, flags) = 0;
	return sys_unmount(l, &um, retval);
}

int
ibcs2_sys_mount(struct lwp *l, const struct ibcs2_sys_mount_args *uap, register_t *retval)
{
#ifdef notyet
	/* {
		syscallarg(char *) special;
		syscallarg(char *) dir;
		syscallarg(int) flags;
		syscallarg(int) fstype;
		syscallarg(char *) data;
		syscallarg(int) len;
	} */
	int oflags = SCARG(uap, flags), nflags, error;
	char fsname[MFSNAMELEN];

	if (oflags & (IBCS2_MS_NOSUB | IBCS2_MS_SYS5))
		return EINVAL;
	if ((oflags & IBCS2_MS_NEWTYPE) == 0)
		return EINVAL;
	nflags = 0;
	if (oflags & IBCS2_MS_RDONLY)
		nflags |= MNT_RDONLY;
	if (oflags & IBCS2_MS_NOSUID)
		nflags |= MNT_NOSUID;
	if (oflags & IBCS2_MS_REMOUNT)
		nflags |= MNT_UPDATE;
	SCARG(uap, flags) = nflags;

	if (error = copyinstr(SCARG(uap, type), fsname, sizeof fsname, NULL))
		return error;

	if (strncmp(fsname, "4.2", sizeof fsname) == 0) {
		SCARG(uap, type) = (void *)STACK_ALLOC();
		if (error = copyout("ffs", SCARG(uap, type), sizeof("ffs")))
			return error;
	} else if (strncmp(fsname, "nfs", sizeof fsname) == 0) {
		struct ibcs2_nfs_args sna;
		struct sockaddr_in sain;
		struct nfs_args na;
		struct sockaddr sa;

		if (error = copyin(SCARG(uap, data), &sna, sizeof sna))
			return error;
		if (error = copyin(sna.addr, &sain, sizeof sain))
			return error;
		memcpy(&sa, &sain, sizeof sa);
		sa.sa_len = sizeof(sain);
		SCARG(uap, data) = STACK_ALLOC();
		na.addr = (void *)((unsigned long)SCARG(uap, data) + sizeof na);
		na.sotype = SOCK_DGRAM;
		na.proto = IPPROTO_UDP;
		na.fh = (nfsv2fh_t *)sna.fh;
		na.flags = sna.flags;
		na.wsize = sna.wsize;
		na.rsize = sna.rsize;
		na.timeo = sna.timeo;
		na.retrans = sna.retrans;
		na.hostname = sna.hostname;

		if (error = copyout(&sa, na.addr, sizeof sa))
			return error;
		if (error = copyout(&na, SCARG(uap, data), sizeof na))
			return error;
	}
	return sys_mount(p, uap, retval);
#else
	return EINVAL;
#endif
}

/*
 * Read iBCS2-style directory entries.  We suck them into kernel space so
 * that they can be massaged before being copied out to user code.  Like
 * SunOS, we squish out `empty' entries.
 *
 * This is quite ugly, but what do you expect from compatibility code?
 */

int
ibcs2_sys_getdents(struct lwp *l, const struct ibcs2_sys_getdents_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(char *) buf;
		syscallarg(int) nbytes;
	} */
	struct dirent *bdp;
	struct vnode *vp;
	char *inp, *tbuf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	char *outp;		/* iBCS2-format */
	int resid, ibcs2_reclen;/* iBCS2-format */
	file_t *fp;
	struct uio auio;
	struct iovec aiov;
	struct ibcs2_dirent idb;
	off_t off;			/* true file offset */
	size_t buflen;
	int error, eofflag;
	off_t *cookiebuf = NULL, *cookie;
	int ncookies;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out1;
	}
	vp = fp->f_data;
	if (vp->v_type != VDIR) {
		error = EINVAL;
		goto out1;
	}
	buflen = min(MAXBSIZE, (size_t)SCARG(uap, nbytes));
	tbuf = malloc(buflen, M_TEMP, M_WAITOK);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	off = fp->f_offset;
again:
	aiov.iov_base = tbuf;
	aiov.iov_len = buflen;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	UIO_SETUP_SYSSPACE(&auio);
	/*
	 * First we read into the malloc'ed buffer, then
	 * we massage it into user space, one record at a time.
	 */
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, &cookiebuf,
	    &ncookies);
	if (error)
		goto out;
	inp = tbuf;
	outp = SCARG(uap, buf);
	resid = SCARG(uap, nbytes);
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;
	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("ibcs2_getdents: bad reclen");
		if (cookie && (*cookie >> 32) != 0) {
			compat_offseterr(vp, "ibcs2_getdents");
			error = EINVAL;
			goto out;
		}
		if (bdp->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			if (cookie)
				off = *cookie++;
			else
				off += reclen;
			continue;
		}
		ibcs2_reclen = IBCS2_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < ibcs2_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		if (cookie)
			off = *cookie++; /* each entry points to the next */
		else
			off += reclen;
		/*
		 * Massage in place to make a iBCS2-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.d_ino = (ibcs2_ino_t)bdp->d_fileno;
		idb.d_off = (ibcs2_off_t)off;
		idb.d_reclen = (u_short)ibcs2_reclen;
		strlcpy(idb.d_name, bdp->d_name, sizeof(idb.d_name));
		error = copyout(&idb, outp, ibcs2_reclen);
		if (error)
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past iBCS2-shaped entry */
		outp += ibcs2_reclen;
		resid -= ibcs2_reclen;
	}

	/* if we squished out the whole block, try again */
	if (outp == SCARG(uap, buf))
		goto again;
	fp->f_offset = off;	/* update the vnode offset */

eof:
	*retval = SCARG(uap, nbytes) - resid;
out:
	VOP_UNLOCK(vp, 0);
	if (cookiebuf)
		free(cookiebuf, M_TEMP);
	free(tbuf, M_TEMP);
out1:
	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
ibcs2_sys_read(struct lwp *l, const struct ibcs2_sys_read_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(char *) buf;
		syscallarg(u_int) nbytes;
	} */
	struct dirent *bdp;
	struct vnode *vp;
	char *inp, *tbuf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	char *outp;		/* iBCS2-format */
	int resid, ibcs2_reclen;/* iBCS2-format */
	file_t *fp;
	struct uio auio;
	struct iovec aiov;
	struct ibcs2_direct {
		ibcs2_ino_t ino;
		char name[14];
	} idb;
	size_t buflen;
	int error, eofflag;
	size_t size;
	off_t *cookiebuf = NULL, *cookie;
	off_t off;			/* true file offset */
	int ncookies;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0) {
		if (error == EINVAL)
			return sys_read(l, (const void *)uap, retval);
		else
			return error;
	}
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out1;
	}
	vp = fp->f_data;
	if (vp->v_type != VDIR) {
		fd_putfile(SCARG(uap, fd));
		return sys_read(l, (const void *)uap, retval);
	}
	buflen = min(MAXBSIZE, max(DEV_BSIZE, (size_t)SCARG(uap, nbytes)));
	tbuf = malloc(buflen, M_TEMP, M_WAITOK);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	off = fp->f_offset;
again:
	aiov.iov_base = tbuf;
	aiov.iov_len = buflen;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	UIO_SETUP_SYSSPACE(&auio);
	/*
	 * First we read into the malloc'ed buffer, then
	 * we massage it into user space, one record at a time.
	 */
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, &cookiebuf,
	    &ncookies);
	if (error)
		goto out;
	inp = tbuf;
	outp = SCARG(uap, buf);
	resid = SCARG(uap, nbytes);
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;
	for (cookie = cookiebuf; len > 0 && resid > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("ibcs2_sys_read");
		if (cookie)
			off = *cookie++; /* each entry points to the next */
		else
			off += reclen;
		if ((off >> 32) != 0) {
			error = EINVAL;
			goto out;
		}
		if (bdp->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			continue;
		}
		ibcs2_reclen = 16;
		if (reclen > len || resid < ibcs2_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		/*
		 * Massage in place to make a iBCS2-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 *
		 * TODO: if length(filename) > 14, then break filename into
		 * multiple entries and set inode = 0xffff except last
		 */
		idb.ino = (bdp->d_fileno > 0xfffe) ? 0xfffe : bdp->d_fileno;
		(void)copystr(bdp->d_name, idb.name, 14, &size);
		memset(idb.name + size, 0, 14 - size);
		error = copyout(&idb, outp, ibcs2_reclen);
		if (error)
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past iBCS2-shaped entry */
		outp += ibcs2_reclen;
		resid -= ibcs2_reclen;
	}
	/* if we squished out the whole block, try again */
	if (outp == SCARG(uap, buf))
		goto again;
	fp->f_offset = off;		/* update the vnode offset */
eof:
	*retval = SCARG(uap, nbytes) - resid;
out:
	VOP_UNLOCK(vp, 0);
	if (cookiebuf)
		free(cookiebuf, M_TEMP);
	free(tbuf, M_TEMP);
out1:
	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
ibcs2_sys_mknod(struct lwp *l, const struct ibcs2_sys_mknod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */

	if (S_ISFIFO(SCARG(uap, mode))) {
		struct sys_mkfifo_args ap;
		SCARG(&ap, path) = SCARG(uap, path);
		SCARG(&ap, mode) = SCARG(uap, mode);
		return sys_mkfifo(l, &ap, retval);
	} else {
		return do_sys_mknod(l, SCARG(uap, path), SCARG(uap, mode),
		    SCARG(uap, dev), retval, UIO_USERSPACE);
	}
}

int
ibcs2_sys_getgroups(struct lwp *l, const struct ibcs2_sys_getgroups_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gidsetsize;
		syscallarg(ibcs2_gid_t *) gidset;
	} */
	ibcs2_gid_t iset[16];
	ibcs2_gid_t *gidset;
	unsigned int ngrps;
	int i, n, j;
	int error;

	ngrps = kauth_cred_ngroups(l->l_cred);
	*retval = ngrps;
	if (SCARG(uap, gidsetsize) == 0)
		return 0;
	if (SCARG(uap, gidsetsize) < ngrps)
		return EINVAL;

	gidset = SCARG(uap, gidset);
	for (i = 0; i < (n = ngrps); i += n, gidset += n) {
		n -= i;
		if (n > __arraycount(iset))
			n = __arraycount(iset);
		for (j = 0; j < n; j++)
			iset[j] = kauth_cred_group(l->l_cred, i + j);
		error = copyout(iset, gidset, n * sizeof(iset[0]));
		if (error != 0)
			return error;
	}

	return 0;
}

/*
 * It is very unlikly that any problem using 16bit groups is written
 * to allow for more than 16 of them, so don't bother trying to
 * support that.
 */
#define COMPAT_NGROUPS16 16

int
ibcs2_sys_setgroups(struct lwp *l, const struct ibcs2_sys_setgroups_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gidsetsize;
		syscallarg(ibcs2_gid_t *) gidset;
	} */

	ibcs2_gid_t iset[COMPAT_NGROUPS16];
	kauth_cred_t ncred;
	int error;
	gid_t grbuf[COMPAT_NGROUPS16];
	unsigned int i, ngroups = SCARG(uap, gidsetsize);

	if (ngroups > COMPAT_NGROUPS16)
		return EINVAL;
	error = copyin(SCARG(uap, gidset), iset, ngroups);
	if (error != 0)
		return error;

	for (i = 0; i < ngroups; i++)
		grbuf[i] = iset[i];

	ncred = kauth_cred_alloc();
	error = kauth_cred_setgroups(ncred, grbuf, SCARG(uap, gidsetsize),
	    -1, UIO_SYSSPACE);
	if (error != 0) {
		kauth_cred_free(ncred);
		return error;
	}

	return kauth_proc_setgroups(l, ncred);
}

int
ibcs2_sys_setuid(struct lwp *l, const struct ibcs2_sys_setuid_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) uid;
	} */
	struct sys_setuid_args sa;

	SCARG(&sa, uid) = (uid_t)SCARG(uap, uid);
	return sys_setuid(l, &sa, retval);
}

int
ibcs2_sys_setgid(struct lwp *l, const struct ibcs2_sys_setgid_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gid;
	} */
	struct sys_setgid_args sa;

	SCARG(&sa, gid) = (gid_t)SCARG(uap, gid);
	return sys_setgid(l, &sa, retval);
}

int
xenix_sys_ftime(struct lwp *l, const struct xenix_sys_ftime_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct xenix_timeb *) tp;
	} */
	struct timeval tv;
	struct xenix_timeb itb;

	microtime(&tv);
	itb.time = tv.tv_sec;
	itb.millitm = (tv.tv_usec / 1000);
	/* NetBSD has no kernel notion of timezone -- fake it. */
	itb.timezone = 0;
	itb.dstflag = 0;
	return copyout(&itb, SCARG(uap, tp), xenix_timeb_len);
}

int
ibcs2_sys_time(struct lwp *l, const struct ibcs2_sys_time_args *uap, register_t *retval)
{
	/* {
		syscallarg(ibcs2_time_t *) tp;
	} */
	struct proc *p = l->l_proc;
	struct timeval tv;

	microtime(&tv);
	*retval = tv.tv_sec;
	if (p->p_emuldata == IBCS2_EXEC_XENIX && SCARG(uap, tp))
		return copyout(&tv.tv_sec, SCARG(uap, tp),
		    sizeof(ibcs2_time_t));
	else
		return 0;
}

int
ibcs2_sys_pathconf(struct lwp *l, const struct ibcs2_sys_pathconf_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) path;
		syscallarg(int) name;
	} */
	struct sys_pathconf_args bsd_ua;

	SCARG(&bsd_ua, path) = SCARG(uap, path);
	/* iBCS2 _PC_* defines are offset by one */
	SCARG(&bsd_ua, name) = SCARG(uap, name) + 1;
	return sys_pathconf(l, &bsd_ua, retval);
}

int
ibcs2_sys_fpathconf(struct lwp *l, const struct ibcs2_sys_fpathconf_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) name;
	} */
	struct sys_fpathconf_args bsd_ua;

	SCARG(&bsd_ua, fd) = SCARG(uap, fd);
	/* iBCS2 _PC_* defines are offset by one */
	SCARG(&bsd_ua, name) = SCARG(uap, name) + 1;
	return sys_fpathconf(l, &bsd_ua, retval);
}

int
ibcs2_sys_sysconf(struct lwp *l, const struct ibcs2_sys_sysconf_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) name;
	} */
	struct proc *p = l->l_proc;
	int mib[2], value, error;
	size_t len;

	switch(SCARG(uap, name)) {
	case IBCS2_SC_ARG_MAX:
		mib[1] = KERN_ARGMAX;
		break;

	case IBCS2_SC_CHILD_MAX:
		*retval = p->p_rlimit[RLIMIT_NPROC].rlim_cur;
		return 0;

	case IBCS2_SC_CLK_TCK:
		*retval = hz;
		return 0;

	case IBCS2_SC_NGROUPS_MAX:
		mib[1] = KERN_NGROUPS;
		break;

	case IBCS2_SC_OPEN_MAX:
		*retval = p->p_rlimit[RLIMIT_NPROC].rlim_cur;
		return 0;

	case IBCS2_SC_JOB_CONTROL:
		mib[1] = KERN_JOB_CONTROL;
		break;

	case IBCS2_SC_SAVED_IDS:
		mib[1] = KERN_SAVED_IDS;
		break;

	case IBCS2_SC_VERSION:
		mib[1] = KERN_POSIX1;
		break;

	case IBCS2_SC_PASS_MAX:
		*retval = 128;		/* XXX - should we create PASS_MAX ? */
		return 0;

	case IBCS2_SC_XOPEN_VERSION:
		*retval = 2;		/* XXX: What should that be? */
		return 0;

	default:
		return EINVAL;
	}

	mib[0] = CTL_KERN;
	len = sizeof(value);
	/*
	 * calling into sysctl with superuser privs, but we don't mind,
	 * 'cause we're only querying a value.
	 */
	error = old_sysctl(&mib[0], 2, &value, &len, NULL, 0, NULL);
	if (error)
		return (error);
	*retval = value;
	return 0;
}

int
ibcs2_sys_alarm(struct lwp *l, const struct ibcs2_sys_alarm_args *uap, register_t *retval)
{
	/* {
		syscallarg(unsigned) sec;
	} */
	struct proc *p = l->l_proc;
	struct itimerval it, oit;
	int error;

	error = dogetitimer(p, ITIMER_REAL, &oit);
	if (error != 0)
		return error;

	timerclear(&it.it_interval);
	it.it_value.tv_sec = SCARG(uap, sec);
	it.it_value.tv_usec = 0;

	error = dosetitimer(p, ITIMER_REAL, &it);
	if (error)
		return error;

	if (oit.it_value.tv_usec)
		oit.it_value.tv_sec++;
	*retval = oit.it_value.tv_sec;
	return 0;
}

int
ibcs2_sys_getmsg(struct lwp *l, const struct ibcs2_sys_getmsg_args *uap, register_t *retval)
{
#ifdef notyet
	/* {
		syscallarg(int) fd;
		syscallarg(struct ibcs2_stropts *) ctl;
		syscallarg(struct ibcs2_stropts *) dat;
		syscallarg(int *) flags;
	} */
#endif

	return 0;
}

int
ibcs2_sys_putmsg(struct lwp *l, const struct ibcs2_sys_putmsg_args *uap, register_t *retval)
{
#ifdef notyet
	/* {
		syscallarg(int) fd;
		syscallarg(struct ibcs2_stropts *) ctl;
		syscallarg(struct ibcs2_stropts *) dat;
		syscallarg(int) flags;
	} */
#endif

	return 0;
}

int
ibcs2_sys_times(struct lwp *l, const struct ibcs2_sys_times_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct tms *) tp;
	} */
	struct tms tms;
	struct timeval t;
	struct rusage ru, *rup;
#define CONVTCK(r)      (r.tv_sec * hz + r.tv_usec / (1000000 / hz))

	ru = l->l_proc->p_stats->p_ru;
	mutex_enter(l->l_proc->p_lock);
	calcru(l->l_proc, &ru.ru_utime, &ru.ru_stime, NULL, NULL);
	rulwps(l->l_proc, &ru);
	mutex_exit(l->l_proc->p_lock);
	tms.tms_utime = CONVTCK(ru.ru_utime);
	tms.tms_stime = CONVTCK(ru.ru_stime);

	rup = &l->l_proc->p_stats->p_cru;
	tms.tms_cutime = CONVTCK(rup->ru_utime);
	tms.tms_cstime = CONVTCK(rup->ru_stime);

	microtime(&t);
	*retval = CONVTCK(t);

	return copyout(&tms, SCARG(uap, tp), sizeof(tms));
}

int
ibcs2_sys_stime(struct lwp *l, const struct ibcs2_sys_stime_args *uap, register_t *retval)
{
	/* {
		syscallarg(long *) timep;
	} */
	struct timeval tv;
	int error;

	error = copyin(SCARG(uap, timep), &tv.tv_sec, sizeof(long));
	if (error)
		return error;
	tv.tv_usec = 0;
	return settimeofday1(&tv, false, NULL, l, true);
}

int
ibcs2_sys_utime(struct lwp *l, const struct ibcs2_sys_utime_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct ibcs2_utimbuf *) buf;
	} */
	int error;
	struct timeval *tptr;
	struct timeval tp[2];

	if (SCARG(uap, buf)) {
		struct ibcs2_utimbuf ubuf;

		error = copyin(SCARG(uap, buf), &ubuf, sizeof(ubuf));
		if (error)
			return error;
		tp[0].tv_sec = ubuf.actime;
		tp[0].tv_usec = 0;
		tp[1].tv_sec = ubuf.modtime;
		tp[1].tv_usec = 0;
		tptr = tp;
	} else
		tptr = NULL;

	return do_sys_utimes(l, NULL, SCARG(uap, path), FOLLOW,
			    tptr, UIO_SYSSPACE);
}

int
ibcs2_sys_nice(struct lwp *l, const struct ibcs2_sys_nice_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) incr;
	} */
	struct proc *p = l->l_proc;
	struct sys_setpriority_args sa;

	SCARG(&sa, which) = PRIO_PROCESS;
	SCARG(&sa, who) = 0;
	SCARG(&sa, prio) = p->p_nice - NZERO + SCARG(uap, incr);
	if (sys_setpriority(l, &sa, retval) != 0)
		return EPERM;
	*retval = p->p_nice - NZERO;
	return 0;
}

/*
 * iBCS2 getpgrp, setpgrp, setsid, and setpgid
 */

int
ibcs2_sys_pgrpsys(struct lwp *l, const struct ibcs2_sys_pgrpsys_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) type;
		syscallarg(void *) dummy;
		syscallarg(int) pid;
		syscallarg(int) pgid;
	} */
	struct proc *p = l->l_proc;

	switch (SCARG(uap, type)) {
	case 0:			/* getpgrp */
		mutex_enter(proc_lock);
		*retval = p->p_pgrp->pg_id;
		mutex_exit(proc_lock);
		return 0;

	case 1:			/* setpgrp */
	    {
		struct sys_setpgid_args sa;

		SCARG(&sa, pid) = 0;
		SCARG(&sa, pgid) = 0;
		sys_setpgid(l, &sa, retval);
		mutex_enter(proc_lock);
		*retval = p->p_pgrp->pg_id;
		mutex_exit(proc_lock);
		return 0;
	    }

	case 2:			/* setpgid */
	    {
		struct sys_setpgid_args sa;

		SCARG(&sa, pid) = SCARG(uap, pid);
		SCARG(&sa, pgid) = SCARG(uap, pgid);
		return sys_setpgid(l, &sa, retval);
	    }

	case 3:			/* setsid */
		return sys_setsid(l, NULL, retval);

	default:
		return EINVAL;
	}
}

/*
 * See http://docsrv.sco.com:507/en/man/html.S/plock.S.html
 *
 * XXX - need to check for nested calls
 */

int
ibcs2_sys_plock(struct lwp *l, const struct ibcs2_sys_plock_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) cmd;
	} */
#define IBCS2_UNLOCK	0
#define IBCS2_PROCLOCK	1
#define IBCS2_TEXTLOCK	2
#define IBCS2_DATALOCK	4

	/*
	 * NOTE: This is a privileged operation. Normally it would require root
	 * access. When implementing, please make sure to use an appropriate
	 * kauth(9) request. See the man-page for more information.
	 */

	switch(SCARG(uap, cmd)) {
	case IBCS2_UNLOCK:
	case IBCS2_PROCLOCK:
	case IBCS2_TEXTLOCK:
	case IBCS2_DATALOCK:
		return 0;	/* XXX - TODO */
	}
	return EINVAL;
}

/*
 * See http://docsrv.sco.com:507/en/man/html.S/uadmin.S.html
 */
int
ibcs2_sys_uadmin(struct lwp *l, const struct ibcs2_sys_uadmin_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) cmd;
		syscallarg(int) func;
		syscallarg(void *) data;
	} */
	int error;

#define SCO_A_REBOOT        1
#define SCO_A_SHUTDOWN      2
#define SCO_A_REMOUNT       4
#define SCO_A_CLOCK         8
#define SCO_A_SETCONFIG     128
#define SCO_A_GETDEV        130

#define SCO_AD_HALT         0
#define SCO_AD_BOOT         1
#define SCO_AD_IBOOT        2
#define SCO_AD_PWRDOWN      3
#define SCO_AD_PWRNAP       4

#define SCO_AD_PANICBOOT    1

#define SCO_AD_GETBMAJ      0
#define SCO_AD_GETCMAJ      1


	switch(SCARG(uap, cmd)) {
	case SCO_A_REBOOT:
	case SCO_A_SHUTDOWN:
		error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_REBOOT,
		    0, NULL, NULL, NULL);
		if (error)
			return (error);

		switch(SCARG(uap, func)) {
		case SCO_AD_HALT:
		case SCO_AD_PWRDOWN:
		case SCO_AD_PWRNAP:
			cpu_reboot(RB_HALT, NULL);
		case SCO_AD_BOOT:
		case SCO_AD_IBOOT:
			cpu_reboot(RB_AUTOBOOT, NULL);
		}
		return EINVAL;
	case SCO_A_REMOUNT:
	case SCO_A_CLOCK:
	case SCO_A_SETCONFIG:
	case SCO_A_GETDEV:
		/*
		 * NOTE: These are all privileged operations, that otherwise
		 * would require root access or similar. When implementing,
		 * please use appropriate kauth(9) requests. See the man-page
		 * for more information.
		 */

		if (SCARG(uap, cmd) != SCO_A_GETDEV)
			return 0;
		else
			return EINVAL;	/* XXX - TODO */
	}
	return EINVAL;
}

int
ibcs2_sys_sysfs(struct lwp *l, const struct ibcs2_sys_sysfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) cmd;
		syscallarg(void *) d1;
		syscallarg(char *) buf;
	} */

#define IBCS2_GETFSIND        1
#define IBCS2_GETFSTYP        2
#define IBCS2_GETNFSTYP       3

	switch(SCARG(uap, cmd)) {
	case IBCS2_GETFSIND:
	case IBCS2_GETFSTYP:
	case IBCS2_GETNFSTYP:
		break;
	}
	return EINVAL;		/* XXX - TODO */
}

int
xenix_sys_rdchk(struct lwp *l, const struct xenix_sys_rdchk_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	file_t *fp;
	int nbytes;
	int error;

	if ((fp = fd_getfile(SCARG(uap, fd))) == NULL)
		return (EBADF);
	error = (*fp->f_ops->fo_ioctl)(fp, FIONREAD, &nbytes);
	fd_putfile(SCARG(uap, fd));

	if (error != 0)
		return error;

	*retval = nbytes ? 1 : 0;
	return 0;
}

int
xenix_sys_chsize(struct lwp *l, const struct xenix_sys_chsize_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(long) size;
	} */
	struct sys_ftruncate_args sa;

	SCARG(&sa, fd) = SCARG(uap, fd);
	SCARG(&sa, PAD) = 0;
	SCARG(&sa, length) = SCARG(uap, size);
	return sys_ftruncate(l, &sa, retval);
}

int
xenix_sys_nap(struct lwp *l, const struct xenix_sys_nap_args *uap, register_t *retval)
{
	/* {
		syscallarg(long) millisec;
	} */
	int error;
	struct timespec rqt;
	struct timespec rmt;

	rqt.tv_sec = 0;
	rqt.tv_nsec = SCARG(uap, millisec) * 1000;
	error = nanosleep1(l, &rqt, &rmt);
	/* If interrupted we can either report EINTR, or the time left */
	if (error != 0 && error != EINTR)
		return error;
	*retval = rmt.tv_nsec / 1000;
	return 0;
}

/*
 * mmap compat code borrowed from svr4/svr4_misc.c
 */

int
ibcs2_sys_mmap(struct lwp *l, const struct ibcs2_sys_mmap_args *uap, register_t *retval)
{
	/* {
		syscallarg(ibcs2_void *) addr;
		syscallarg(ibcs2_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(ibcs2_off_t) off;
	} */
	struct sys_mmap_args mm;

#define _MAP_NEW	0x80000000 /* XXX why? */

	if (SCARG(uap, prot) & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
		return EINVAL;
	if (SCARG(uap, len) == 0)
		return EINVAL;

	SCARG(&mm, prot) = SCARG(uap, prot);
	SCARG(&mm, len) = SCARG(uap, len);
	SCARG(&mm, flags) = SCARG(uap, flags) & ~_MAP_NEW;
	SCARG(&mm, fd) = SCARG(uap, fd);
	SCARG(&mm, addr) = SCARG(uap, addr);
	SCARG(&mm, pos) = SCARG(uap, off);

	return sys_mmap(l, &mm, retval);
}

int
ibcs2_sys_memcntl(struct lwp *l, const struct ibcs2_sys_memcntl_args *uap, register_t *retval)
{
	/* {
		syscallarg(ibcs2_void *) addr;
		syscallarg(ibcs2_size_t) len;
		syscallarg(int) cmd;
		syscallarg(ibcs2_void *) arg;
		syscallarg(int) attr;
		syscallarg(int) mask;
	} */

	switch (SCARG(uap, cmd)) {
	case IBCS2_MC_SYNC:
		{
			struct sys___msync13_args msa;

			SCARG(&msa, addr) = SCARG(uap, addr);
			SCARG(&msa, len) = SCARG(uap, len);
			SCARG(&msa, flags) = (int)SCARG(uap, arg);

			return sys___msync13(l, &msa, retval);
		}
#ifdef IBCS2_MC_ADVISE		/* supported? */
	case IBCS2_MC_ADVISE:
		{
			struct sys_madvise_args maa;

			SCARG(&maa, addr) = SCARG(uap, addr);
			SCARG(&maa, len) = SCARG(uap, len);
			SCARG(&maa, behav) = (int)SCARG(uap, arg);

			return sys_madvise(l, &maa, retval);
		}
#endif
	case IBCS2_MC_LOCK:
	case IBCS2_MC_UNLOCK:
	case IBCS2_MC_LOCKAS:
	case IBCS2_MC_UNLOCKAS:
		return EOPNOTSUPP;
	default:
		return ENOSYS;
	}
}

int
ibcs2_sys_gettimeofday(struct lwp *l, const struct ibcs2_sys_gettimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct timeval *) tp;
	} */

	if (SCARG(uap, tp)) {
		struct timeval atv;

		microtime(&atv);
		return copyout(&atv, SCARG(uap, tp), sizeof (atv));
	}

	return 0;
}

int
ibcs2_sys_settimeofday(struct lwp *l, const struct ibcs2_sys_settimeofday_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct timeval *) tp;
	} */
	struct compat_50_sys_settimeofday_args ap;

	SCARG(&ap, tv) = SCARG(uap, tp);
	SCARG(&ap, tzp) = NULL;
	return compat_50_sys_settimeofday(l, &ap, retval);
}

int
ibcs2_sys_scoinfo(struct lwp *l, const struct ibcs2_sys_scoinfo_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct scoutsname *) bp;
		syscallarg(int) len;
	} */
	struct scoutsname uts;

	(void)memset(&uts, 0, sizeof(uts));
	(void)strncpy(uts.sysname, ostype, 8);
	(void)strncpy(uts.nodename, hostname, 8);
	(void)strncpy(uts.release, osrelease, 15);
	(void)strncpy(uts.kid, "kernel id 1", 19);
	(void)strncpy(uts.machine, machine, 8);
	(void)strncpy(uts.bustype, "pci", 8);
	(void)strncpy(uts.serial, "1234", 9);
	uts.origin = 0;
	uts.oem = 0;
	(void)strncpy(uts.nusers, "unlim", 8);
	uts.ncpu = 1;

	return copyout(&uts, SCARG(uap, bp), sizeof(uts));
}

#define X_LK_UNLCK  0
#define X_LK_LOCK   1
#define X_LK_NBLCK 20
#define X_LK_RLCK   3
#define X_LK_NBRLCK 4
#define X_LK_GETLK  5
#define X_LK_SETLK  6
#define X_LK_SETLKW 7
#define X_LK_TESTLK 8

int
xenix_sys_locking(struct lwp *l, const struct xenix_sys_locking_args *uap, register_t *retval)
{
	/* {
	      syscallarg(int) fd;
	      syscallarg(int) blk;
	      syscallarg(int) size;
	} */
	struct flock fl;
	int cmd;

	switch SCARG(uap, blk) {
	case X_LK_GETLK:
	case X_LK_SETLK:
	case X_LK_SETLKW:
		return ibcs2_sys_fcntl(l, (const void *)uap, retval);
	}

	switch SCARG(uap, blk) {
	case X_LK_UNLCK:
		cmd = F_SETLK;
		fl.l_type = F_UNLCK;
		break;
	case X_LK_LOCK:
		cmd = F_SETLKW;
		fl.l_type = F_WRLCK;
		break;
	case X_LK_RLCK:
		cmd = F_SETLKW;
		fl.l_type = F_RDLCK;
		break;
	case X_LK_NBRLCK:
		cmd = F_SETLK;
		fl.l_type = F_RDLCK;
		break;
	case X_LK_NBLCK:
		cmd = F_SETLK;
		fl.l_type = F_WRLCK;
		break;
	default:
		return EINVAL;
	}
	fl.l_len = SCARG(uap, size);
	fl.l_start = 0;
	fl.l_whence = SEEK_CUR;

	return do_fcntl_lock(SCARG(uap, fd), cmd, &fl);
}
