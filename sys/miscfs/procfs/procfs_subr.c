/*	$NetBSD: procfs_subr.c,v 1.57 2003/08/07 16:32:42 agc Exp $	*/

/*
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
 *	@(#)procfs_subr.c	8.6 (Berkeley) 5/14/95
 */

/*
 * Copyright (c) 1994 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1993 Jan-Simon Pendry
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
 *	@(#)procfs_subr.c	8.6 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: procfs_subr.c,v 1.57 2003/08/07 16:32:42 agc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/filedesc.h>

#include <miscfs/procfs/procfs.h>

void procfs_hashins __P((struct pfsnode *));
void procfs_hashrem __P((struct pfsnode *));
struct vnode *procfs_hashget __P((pid_t, pfstype, int, struct mount *));

LIST_HEAD(pfs_hashhead, pfsnode) *pfs_hashtbl;
u_long	pfs_ihash;	/* size of hash table - 1 */
#define PFSPIDHASH(pid)	((pid) & pfs_ihash)

struct lock pfs_hashlock;
struct simplelock pfs_hash_slock;

#define	ISSET(t, f)	((t) & (f))

/*
 * allocate a pfsnode/vnode pair.  the vnode is
 * referenced, and locked.
 *
 * the pid, pfs_type, and mount point uniquely
 * identify a pfsnode.  the mount point is needed
 * because someone might mount this filesystem
 * twice.
 *
 * all pfsnodes are maintained on a singly-linked
 * list.  new nodes are only allocated when they cannot
 * be found on this list.  entries on the list are
 * removed when the vfs reclaim entry is called.
 *
 * a single lock is kept for the entire list.  this is
 * needed because the getnewvnode() function can block
 * waiting for a vnode to become free, in which case there
 * may be more than one process trying to get the same
 * vnode.  this lock is only taken if we are going to
 * call getnewvnode, since the kernel itself is single-threaded.
 *
 * if an entry is found on the list, then call vget() to
 * take a reference.  this is done because there may be
 * zero references to it and so it needs to removed from
 * the vnode free list.
 */
int
procfs_allocvp(mp, vpp, pid, pfs_type, fd)
	struct mount *mp;
	struct vnode **vpp;
	pid_t pid;
	pfstype pfs_type;
	int fd;
{
	struct pfsnode *pfs;
	struct vnode *vp;
	int error;

	do {
		if ((*vpp = procfs_hashget(pid, pfs_type, fd, mp)) != NULL)
			return (0);
	} while (lockmgr(&pfs_hashlock, LK_EXCLUSIVE|LK_SLEEPFAIL, 0));

	if ((error = getnewvnode(VT_PROCFS, mp, procfs_vnodeop_p, &vp)) != 0) {
		*vpp = NULL;
		lockmgr(&pfs_hashlock, LK_RELEASE, NULL);
		return (error);
	}

	MALLOC(pfs, void *, sizeof(struct pfsnode), M_TEMP, M_WAITOK);
	vp->v_data = pfs;

	pfs->pfs_pid = pid;
	pfs->pfs_type = pfs_type;
	pfs->pfs_vnode = vp;
	pfs->pfs_flags = 0;
	pfs->pfs_fileno = PROCFS_FILENO(pid, pfs_type, fd);
	pfs->pfs_fd = fd;

	switch (pfs_type) {
	case Proot:	/* /proc = dr-xr-xr-x */
		pfs->pfs_mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
		vp->v_type = VDIR;
		vp->v_flag = VROOT;
		break;

	case Pcurproc:	/* /proc/curproc = lr-xr-xr-x */
	case Pself:	/* /proc/self    = lr-xr-xr-x */
		pfs->pfs_mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
		vp->v_type = VLNK;
		break;

	case Pproc:	/* /proc/N = dr-xr-xr-x */
	case Pfd:
		if (fd == -1) {	/* /proc/N/fd = dr-xr-xr-x */
			pfs->pfs_mode = S_IRUSR|S_IXUSR;
			vp->v_type = VDIR;
		} else {	/* /proc/N/fd/M = [ps-]rw------- */
			struct file *fp;
			struct vnode *vxp;
			struct proc *pown;

			/* XXX can procfs_getfp() ever fail here? */
			if ((error = procfs_getfp(pfs, &pown, &fp)) != 0)
				goto bad;
			FILE_USE(fp);

			pfs->pfs_mode = S_IRUSR|S_IWUSR;
			switch (fp->f_type) {
			case DTYPE_VNODE:
				vxp = (struct vnode *)fp->f_data;

				/*
				 * We make symlinks for directories 
				 * to avoid cycles.
				 */
				if (vxp->v_type == VDIR)
					goto symlink;
				vp->v_type = vxp->v_type;
				break;
			case DTYPE_PIPE:
				vp->v_type = VFIFO;
				break;
			case DTYPE_SOCKET:
				vp->v_type = VSOCK;
				break;
			case DTYPE_KQUEUE:
			case DTYPE_MISC:
			symlink:
				pfs->pfs_mode = S_IRUSR|S_IXUSR|S_IRGRP|
				    S_IXGRP|S_IROTH|S_IXOTH;
				vp->v_type = VLNK;
				break;
			default:
				error = EOPNOTSUPP;
				FILE_UNUSE(fp, pown);
				goto bad;
			}
			FILE_UNUSE(fp, pown);
		}
		break;

	case Pfile:	/* /proc/N/file = -rw------- */
	case Pmem:	/* /proc/N/mem = -rw------- */
	case Pregs:	/* /proc/N/regs = -rw------- */
	case Pfpregs:	/* /proc/N/fpregs = -rw------- */
		pfs->pfs_mode = S_IRUSR|S_IWUSR;
		vp->v_type = VREG;
		break;

	case Pctl:	/* /proc/N/ctl = --w------ */
	case Pnote:	/* /proc/N/note = --w------ */
	case Pnotepg:	/* /proc/N/notepg = --w------ */
		pfs->pfs_mode = S_IWUSR;
		vp->v_type = VREG;
		break;

	case Pmap:	/* /proc/N/map = -r--r--r-- */
	case Pmaps:	/* /proc/N/maps = -r--r--r-- */
	case Pstatus:	/* /proc/N/status = -r--r--r-- */
	case Pstat:	/* /proc/N/stat = -r--r--r-- */
	case Pcmdline:	/* /proc/N/cmdline = -r--r--r-- */
	case Pmeminfo:	/* /proc/meminfo = -r--r--r-- */
	case Pcpuinfo:	/* /proc/cpuinfo = -r--r--r-- */
	case Puptime:	/* /proc/uptime = -r--r--r-- */
		pfs->pfs_mode = S_IRUSR|S_IRGRP|S_IROTH;
		vp->v_type = VREG;
		break;

#ifdef __HAVE_PROCFS_MACHDEP
	PROCFS_MACHDEP_NODETYPE_CASES
		procfs_machdep_allocvp(vp);
		break;
#endif

	default:
		panic("procfs_allocvp");
	}

	procfs_hashins(pfs);
	uvm_vnp_setsize(vp, 0);
	lockmgr(&pfs_hashlock, LK_RELEASE, NULL);

	*vpp = vp;
	return (0);

 bad:
	lockmgr(&pfs_hashlock, LK_RELEASE, NULL);
	FREE(pfs, M_TEMP);
	ungetnewvnode(vp);
	return (error);
}

int
procfs_freevp(vp)
	struct vnode *vp;
{
	struct pfsnode *pfs = VTOPFS(vp);

	procfs_hashrem(pfs);

	FREE(vp->v_data, M_TEMP);
	vp->v_data = 0;
	return (0);
}

int
procfs_rw(v)
	void *v;
{
	struct vop_read_args *ap = v;
	struct vnode *vp = ap->a_vp;
	struct uio *uio = ap->a_uio;
	struct proc *curp = uio->uio_procp;
	struct pfsnode *pfs = VTOPFS(vp);
	struct lwp *l;
	struct proc *p;

	p = PFIND(pfs->pfs_pid);
	if (p == 0)
		return (EINVAL);

	/* XXX NJWLWP
	 * The entire procfs interface needs work to be useful to
	 * a process with multiple LWPs. For the moment, we'll 
	 * just kluge this and fail on others.
	 */
	l = proc_representative_lwp(p);
	
	switch (pfs->pfs_type) {
	case Pregs:
	case Pfpregs:
	case Pmem:
#if defined(__HAVE_PROCFS_MACHDEP) && defined(PROCFS_MACHDEP_PROTECT_CASES)
	PROCFS_MACHDEP_PROTECT_CASES
#endif
		/*
		 * Do not allow init to be modified while in secure mode; it
		 * could be duped into changing the security level.
		 */
		if (uio->uio_rw == UIO_WRITE &&
		    p == initproc && securelevel > -1)
			return (EPERM);
		break;

	default:
		break;
	}

	switch (pfs->pfs_type) {
	case Pnote:
	case Pnotepg:
		return (procfs_donote(curp, p, pfs, uio));

	case Pregs:
		return (procfs_doregs(curp, l, pfs, uio));

	case Pfpregs:
		return (procfs_dofpregs(curp, l, pfs, uio));

	case Pctl:
		return (procfs_doctl(curp, l, pfs, uio));

	case Pstatus:
		return (procfs_dostatus(curp, l, pfs, uio));

	case Pstat:
		return (procfs_do_pid_stat(curp, l, pfs, uio));

	case Pmap:
		return (procfs_domap(curp, p, pfs, uio, 0));

	case Pmaps:
		return (procfs_domap(curp, p, pfs, uio, 1));

	case Pmem:
		return (procfs_domem(curp, p, pfs, uio));

	case Pcmdline:
		return (procfs_docmdline(curp, p, pfs, uio));

	case Pmeminfo:
		return (procfs_domeminfo(curp, p, pfs, uio));

	case Pcpuinfo:
		return (procfs_docpuinfo(curp, p, pfs, uio));

	case Pfd:
		return (procfs_dofd(curp, p, pfs, uio));

	case Puptime:
		return (procfs_douptime(curp, p, pfs, uio));

#ifdef __HAVE_PROCFS_MACHDEP
	PROCFS_MACHDEP_NODETYPE_CASES
		return (procfs_machdep_rw(curp, l, pfs, uio));
#endif

	default:
		return (EOPNOTSUPP);
	}
}

/*
 * Get a string from userland into (buf).  Strip a trailing
 * nl character (to allow easy access from the shell).
 * The buffer should be *buflenp + 1 chars long.  vfs_getuserstr
 * will automatically add a nul char at the end.
 *
 * Returns 0 on success or the following errors
 *
 * EINVAL:    file offset is non-zero.
 * EMSGSIZE:  message is longer than kernel buffer
 * EFAULT:    user i/o buffer is not addressable
 */
int
vfs_getuserstr(uio, buf, buflenp)
	struct uio *uio;
	char *buf;
	int *buflenp;
{
	int xlen;
	int error;

	if (uio->uio_offset != 0)
		return (EINVAL);

	xlen = *buflenp;

	/* must be able to read the whole string in one go */
	if (xlen < uio->uio_resid)
		return (EMSGSIZE);
	xlen = uio->uio_resid;

	if ((error = uiomove(buf, xlen, uio)) != 0)
		return (error);

	/* allow multiple writes without seeks */
	uio->uio_offset = 0;

	/* cleanup string and remove trailing newline */
	buf[xlen] = '\0';
	xlen = strlen(buf);
	if (xlen > 0 && buf[xlen-1] == '\n')
		buf[--xlen] = '\0';
	*buflenp = xlen;

	return (0);
}

const vfs_namemap_t *
vfs_findname(nm, buf, buflen)
	const vfs_namemap_t *nm;
	const char *buf;
	int buflen;
{

	for (; nm->nm_name; nm++)
		if (memcmp(buf, nm->nm_name, buflen+1) == 0)
			return (nm);

	return (0);
}

/*
 * Initialize pfsnode hash table.
 */
void
procfs_hashinit()
{
	lockinit(&pfs_hashlock, PINOD, "pfs_hashlock", 0, 0);
	pfs_hashtbl = hashinit(desiredvnodes / 4, HASH_LIST, M_UFSMNT,
	    M_WAITOK, &pfs_ihash);
	simple_lock_init(&pfs_hash_slock);
}

void
procfs_hashreinit()
{
	struct pfsnode *pp;
	struct pfs_hashhead *oldhash, *hash;
	u_long i, oldmask, mask, val;

	hash = hashinit(desiredvnodes / 4, HASH_LIST, M_UFSMNT, M_WAITOK,
	    &mask);

	simple_lock(&pfs_hash_slock);
	oldhash = pfs_hashtbl;
	oldmask = pfs_ihash;
	pfs_hashtbl = hash;
	pfs_ihash = mask;
	for (i = 0; i <= oldmask; i++) {
		while ((pp = LIST_FIRST(&oldhash[i])) != NULL) {
			LIST_REMOVE(pp, pfs_hash);
			val = PFSPIDHASH(pp->pfs_pid);
			LIST_INSERT_HEAD(&hash[val], pp, pfs_hash);
		}
	}
	simple_unlock(&pfs_hash_slock);
	hashdone(oldhash, M_UFSMNT);
}

/*
 * Free pfsnode hash table.
 */
void
procfs_hashdone()
{
	hashdone(pfs_hashtbl, M_UFSMNT);
}

struct vnode *
procfs_hashget(pid, type, fd, mp)
	pid_t pid;
	pfstype type;
	int fd;
	struct mount *mp;
{
	struct pfs_hashhead *ppp;
	struct pfsnode *pp;
	struct vnode *vp;

loop:
	simple_lock(&pfs_hash_slock);
	ppp = &pfs_hashtbl[PFSPIDHASH(pid)];
	LIST_FOREACH(pp, ppp, pfs_hash) {
		vp = PFSTOV(pp);
		if (pid == pp->pfs_pid && pp->pfs_type == type &&
		    pp->pfs_fd == fd && vp->v_mount == mp) {
			simple_lock(&vp->v_interlock);
			simple_unlock(&pfs_hash_slock);
			if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK))
				goto loop;
			return (vp);
		}
	}
	simple_unlock(&pfs_hash_slock);
	return (NULL);
}

/*
 * Insert the pfsnode into the hash table and lock it.
 */
void
procfs_hashins(pp)
	struct pfsnode *pp;
{
	struct pfs_hashhead *ppp;

	/* lock the pfsnode, then put it on the appropriate hash list */
	lockmgr(&pp->pfs_vnode->v_lock, LK_EXCLUSIVE, (struct simplelock *)0);

	simple_lock(&pfs_hash_slock);
	ppp = &pfs_hashtbl[PFSPIDHASH(pp->pfs_pid)];
	LIST_INSERT_HEAD(ppp, pp, pfs_hash);
	simple_unlock(&pfs_hash_slock);
}

/*
 * Remove the pfsnode from the hash table.
 */
void
procfs_hashrem(pp)
	struct pfsnode *pp;
{
	simple_lock(&pfs_hash_slock);
	LIST_REMOVE(pp, pfs_hash);
	simple_unlock(&pfs_hash_slock);
}

void
procfs_revoke_vnodes(p, arg)
	struct proc *p;
	void *arg;
{
	struct pfsnode *pfs, *pnext;
	struct vnode *vp;
	struct mount *mp = (struct mount *)arg;
	struct pfs_hashhead *ppp;

	if (!(p->p_flag & P_SUGID))
		return;

	ppp = &pfs_hashtbl[PFSPIDHASH(p->p_pid)];
	for (pfs = LIST_FIRST(ppp); pfs; pfs = pnext) {
		vp = PFSTOV(pfs);
		pnext = LIST_NEXT(pfs, pfs_hash);
		if (vp->v_usecount > 0 && pfs->pfs_pid == p->p_pid &&
		    vp->v_mount == mp)
			VOP_REVOKE(vp, REVOKEALL);
	}
}

int
procfs_getfp(pfs, pown, fp)
	struct pfsnode *pfs;
	struct proc **pown;
	struct file **fp;
{
	struct proc *p = PFIND(pfs->pfs_pid);

	if (p == NULL)
		return ESRCH;

	if (pfs->pfs_fd == -1)
		return EINVAL;

	if ((*fp = fd_getfile(p->p_fd, pfs->pfs_fd)) == NULL)
		return EBADF;

	*pown = p;
	return 0;
}
