/*	$NetBSD: kernfs_vnops.c,v 1.91 2003/09/08 10:54:14 itojun Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
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
 *	@(#)kernfs_vnops.c	8.15 (Berkeley) 5/21/95
 */

/*
 * Kernel parameter filesystem (/kern)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kernfs_vnops.c,v 1.91 2003/09/08 10:54:14 itojun Exp $");

#ifdef _KERNEL_OPT
#include "opt_ipsec.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vmmeter.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/dirent.h>
#include <sys/msgbuf.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/kernfs/kernfs.h>

#ifdef IPSEC
#include <sys/mbuf.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#endif

#include <uvm/uvm_extern.h>

#define KSTRING	256		/* Largest I/O available via this filesystem */
#define	UIO_MX 32

#define	READ_MODE	(S_IRUSR|S_IRGRP|S_IROTH)
#define	WRITE_MODE	(S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH)
#define DIR_MODE	(S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
#define UDIR_MODE	(S_IRUSR|S_IXUSR)

#define N(s) sizeof(s)-1, s
const struct kern_target kern_targets[] = {
/* NOTE: The name must be less than UIO_MX-16 chars in length */
     /*        name            data          tag           type  ro/rw */
     { DT_DIR, N("."),         0,            Pkern,        VDIR, DIR_MODE   },
     { DT_DIR, N(".."),        0,            Proot,        VDIR, DIR_MODE   },
     { DT_REG, N("boottime"),  &boottime.tv_sec, Pint,     VREG, READ_MODE  },
			/* XXX cast away const */
     { DT_REG, N("copyright"), (void *)copyright,
     					     Pstring,      VREG, READ_MODE  },
     { DT_REG, N("hostname"),  0,            Phostname,    VREG, WRITE_MODE },
     { DT_REG, N("hz"),        &hz,          Pint,         VREG, READ_MODE  },
#ifdef IPSEC
     { DT_DIR, N("ipsecsa"),   0,	     Pipsecsadir,  VDIR, UDIR_MODE  },
     { DT_DIR, N("ipsecsp"),   0,	     Pipsecspdir,  VDIR, UDIR_MODE  },
#endif
     { DT_REG, N("loadavg"),   0,            Pavenrun,     VREG, READ_MODE  },
     { DT_REG, N("msgbuf"),    0,	     Pmsgbuf,      VREG, READ_MODE  },
     { DT_REG, N("pagesize"),  &uvmexp.pagesize, Pint,     VREG, READ_MODE  },
     { DT_REG, N("physmem"),   &physmem,     Pint,         VREG, READ_MODE  },
#if 0
     { DT_DIR, N("root"),      0,            Pnull,        VDIR, DIR_MODE   },
#endif
     { DT_BLK, N("rootdev"),   &rootdev,     Pdevice,      VBLK, READ_MODE  },
     { DT_CHR, N("rrootdev"),  &rrootdev,    Pdevice,      VCHR, READ_MODE  },
     { DT_REG, N("time"),      0,            Ptime,        VREG, READ_MODE  },
			/* XXX cast away const */
     { DT_REG, N("version"),   (void *)version,
     					     Pstring,      VREG, READ_MODE  },
};
#ifdef IPSEC
const struct kern_target ipsecsa_targets[] = {
/* NOTE: The name must be less than UIO_MX-16 chars in length */
     /*        name            data          tag           type  ro/rw */
     { DT_DIR, N("."),         0,            Pipsecsadir,  VDIR, DIR_MODE   },
     { DT_DIR, N(".."),        0,            Pkern,        VDIR, DIR_MODE   },
};
const struct kern_target ipsecsp_targets[] = {
/* NOTE: The name must be less than UIO_MX-16 chars in length */
     /*        name            data          tag           type  ro/rw */
     { DT_DIR, N("."),         0,            Pipsecspdir,  VDIR, DIR_MODE   },
     { DT_DIR, N(".."),        0,            Pkern,        VDIR, DIR_MODE   },
};
#endif
#undef N
int nkern_targets = sizeof(kern_targets) / sizeof(kern_targets[0]);
#ifdef IPSEC
int nipsecsa_targets = sizeof(ipsecsa_targets) / sizeof(ipsecsa_targets[0]);
int nipsecsp_targets = sizeof(ipsecsp_targets) / sizeof(ipsecsp_targets[0]);
#endif


int	kernfs_lookup	__P((void *));
#define	kernfs_create	genfs_eopnotsupp
#define	kernfs_mknod	genfs_eopnotsupp
int	kernfs_open	__P((void *));
int	kernfs_close	__P((void *));
int	kernfs_access	__P((void *));
int	kernfs_getattr	__P((void *));
int	kernfs_setattr	__P((void *));
int	kernfs_read	__P((void *));
int	kernfs_write	__P((void *));
#define	kernfs_fcntl	genfs_fcntl
#define	kernfs_ioctl	genfs_enoioctl
#define	kernfs_poll	genfs_poll
#define kernfs_revoke	genfs_revoke
#define	kernfs_fsync	genfs_nullop
#define	kernfs_seek	genfs_nullop
#define	kernfs_remove	genfs_eopnotsupp
int	kernfs_link	__P((void *));
#define	kernfs_rename	genfs_eopnotsupp
#define	kernfs_mkdir	genfs_eopnotsupp
#define	kernfs_rmdir	genfs_eopnotsupp
int	kernfs_symlink	__P((void *));
int	kernfs_readdir	__P((void *));
#define	kernfs_readlink	genfs_eopnotsupp
#define	kernfs_abortop	genfs_abortop
int	kernfs_inactive	__P((void *));
int	kernfs_reclaim	__P((void *));
#define	kernfs_lock	genfs_lock
#define	kernfs_unlock	genfs_unlock
#define	kernfs_bmap	genfs_badop
#define	kernfs_strategy	genfs_badop
int	kernfs_print	__P((void *));
#define	kernfs_islocked	genfs_islocked
int	kernfs_pathconf	__P((void *));
#define	kernfs_advlock	genfs_einval
#define	kernfs_blkatoff	genfs_eopnotsupp
#define	kernfs_valloc	genfs_eopnotsupp
#define	kernfs_vfree	genfs_nullop
#define	kernfs_truncate	genfs_eopnotsupp
#define	kernfs_update	genfs_nullop
#define	kernfs_bwrite	genfs_eopnotsupp
#define	kernfs_putpages	genfs_putpages

static int	kernfs_xread __P((struct kernfs_node *, int, char **, size_t, size_t *));
static int	kernfs_xwrite __P((const struct kernfs_node *, char *, size_t));

int (**kernfs_vnodeop_p) __P((void *));
const struct vnodeopv_entry_desc kernfs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, kernfs_lookup },		/* lookup */
	{ &vop_create_desc, kernfs_create },		/* create */
	{ &vop_mknod_desc, kernfs_mknod },		/* mknod */
	{ &vop_open_desc, kernfs_open },		/* open */
	{ &vop_close_desc, kernfs_close },		/* close */
	{ &vop_access_desc, kernfs_access },		/* access */
	{ &vop_getattr_desc, kernfs_getattr },		/* getattr */
	{ &vop_setattr_desc, kernfs_setattr },		/* setattr */
	{ &vop_read_desc, kernfs_read },		/* read */
	{ &vop_write_desc, kernfs_write },		/* write */
	{ &vop_fcntl_desc, kernfs_fcntl },		/* fcntl */
	{ &vop_ioctl_desc, kernfs_ioctl },		/* ioctl */
	{ &vop_poll_desc, kernfs_poll },		/* poll */
	{ &vop_revoke_desc, kernfs_revoke },		/* revoke */
	{ &vop_fsync_desc, kernfs_fsync },		/* fsync */
	{ &vop_seek_desc, kernfs_seek },		/* seek */
	{ &vop_remove_desc, kernfs_remove },		/* remove */
	{ &vop_link_desc, kernfs_link },		/* link */
	{ &vop_rename_desc, kernfs_rename },		/* rename */
	{ &vop_mkdir_desc, kernfs_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, kernfs_rmdir },		/* rmdir */
	{ &vop_symlink_desc, kernfs_symlink },		/* symlink */
	{ &vop_readdir_desc, kernfs_readdir },		/* readdir */
	{ &vop_readlink_desc, kernfs_readlink },	/* readlink */
	{ &vop_abortop_desc, kernfs_abortop },		/* abortop */
	{ &vop_inactive_desc, kernfs_inactive },	/* inactive */
	{ &vop_reclaim_desc, kernfs_reclaim },		/* reclaim */
	{ &vop_lock_desc, kernfs_lock },		/* lock */
	{ &vop_unlock_desc, kernfs_unlock },		/* unlock */
	{ &vop_bmap_desc, kernfs_bmap },		/* bmap */
	{ &vop_strategy_desc, kernfs_strategy },	/* strategy */
	{ &vop_print_desc, kernfs_print },		/* print */
	{ &vop_islocked_desc, kernfs_islocked },	/* islocked */
	{ &vop_pathconf_desc, kernfs_pathconf },	/* pathconf */
	{ &vop_advlock_desc, kernfs_advlock },		/* advlock */
	{ &vop_blkatoff_desc, kernfs_blkatoff },	/* blkatoff */
	{ &vop_valloc_desc, kernfs_valloc },		/* valloc */
	{ &vop_vfree_desc, kernfs_vfree },		/* vfree */
	{ &vop_truncate_desc, kernfs_truncate },	/* truncate */
	{ &vop_update_desc, kernfs_update },		/* update */
	{ &vop_bwrite_desc, kernfs_bwrite },		/* bwrite */
	{ &vop_putpages_desc, kernfs_putpages },	/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc kernfs_vnodeop_opv_desc =
	{ &kernfs_vnodeop_p, kernfs_vnodeop_entries };

static int
kernfs_xread(kfs, off, bufp, len, wrlen)
	struct kernfs_node *kfs;
	int off;
	char **bufp;
	size_t len;
	size_t *wrlen;
{
	const struct kern_target *kt;
#ifdef IPSEC
	struct mbuf *m;
#endif

	kt = kfs->kfs_kt;

	switch (kfs->kfs_type) {
	case Ptime: {
		struct timeval tv;

		microtime(&tv);
		snprintf(*bufp, len, "%ld %ld\n", tv.tv_sec, tv.tv_usec);
		break;
	}

	case Pint: {
		int *ip = kt->kt_data;

		snprintf(*bufp, len, "%d\n", *ip);
		break;
	}

	case Pstring: {
		char *cp = kt->kt_data;

		*bufp = cp;
		break;
	}

	case Pmsgbuf: {
		long n;

		/*
		 * deal with cases where the message buffer has
		 * become corrupted.
		 */
		if (!msgbufenabled || msgbufp->msg_magic != MSG_MAGIC) {
			msgbufenabled = 0;
			return (ENXIO);
		}

		/*
		 * Note that reads of /kern/msgbuf won't necessarily yield
		 * consistent results, if the message buffer is modified
		 * while the read is in progress.  The worst that can happen
		 * is that incorrect data will be read.  There's no way
		 * that this can crash the system unless the values in the
		 * message buffer header are corrupted, but that'll cause
		 * the system to die anyway.
		 */
		if (off >= msgbufp->msg_bufs) {
			*wrlen = 0;
			return (0);
		}
		n = msgbufp->msg_bufx + off;
		if (n >= msgbufp->msg_bufs)
			n -= msgbufp->msg_bufs;
		len = min(msgbufp->msg_bufs - n, msgbufp->msg_bufs - off);
		*bufp = msgbufp->msg_bufc + n;
		*wrlen = len;
		return (0);
	}

	case Phostname: {
		char *cp = hostname;
		int xlen = hostnamelen;

		if (xlen >= (len - 2))
			return (EINVAL);

		memcpy(*bufp, cp, xlen);
		(*bufp)[xlen] = '\n';
		(*bufp)[xlen+1] = '\0';
		len = strlen(*bufp);
		break;
	}

	case Pavenrun:
		averunnable.fscale = FSCALE;
		snprintf(*bufp, len, "%d %d %d %ld\n",
		    averunnable.ldavg[0], averunnable.ldavg[1],
		    averunnable.ldavg[2], averunnable.fscale);
		break;

#ifdef IPSEC
	case Pipsecsa:
		/*
		 * Note that SA configuration could be changed during the
		 * read operation, resulting in garbled output.
		 */
		m = key_setdumpsa_spi(htonl(kfs->kfs_value));
		if (!m)
			return (ENOBUFS);
		if (off >= m->m_pkthdr.len) {
			*wrlen = 0;
			m_freem(m);
			return (0);
		}
		if (len > m->m_pkthdr.len - off)
			len = m->m_pkthdr.len - off;
		m_copydata(m, off, len, *bufp);
		*wrlen = len;
		m_freem(m);
		return (0);

	case Pipsecsp:
		/*
		 * Note that SP configuration could be changed during the
		 * read operation, resulting in garbled output.
		 */
		if (!kfs->kfs_v) {
			struct secpolicy *sp;

			sp = key_getspbyid(kfs->kfs_value);
			if (sp)
				kfs->kfs_v = sp;
			else
				return (ENOENT);
		}
		m = key_setdumpsp((struct secpolicy *)kfs->kfs_v,
		    SADB_X_SPDGET, 0, 0);
		if (!m)
			return (ENOBUFS);
		if (off >= m->m_pkthdr.len) {
			*wrlen = 0;
			m_freem(m);
			return (0);
		}
		if (len > m->m_pkthdr.len - off)
			len = m->m_pkthdr.len - off;
		m_copydata(m, off, len, *bufp);
		*wrlen = len;
		m_freem(m);
		return (0);
#endif

	default:
		*wrlen = 0;
		return (0);
	}

	len = strlen(*bufp);
	if (len <= off)
		*wrlen = 0;
	else {
		*bufp += off;
		*wrlen = len - off;
	}
	return (0);
}

static int
kernfs_xwrite(kfs, buf, len)
	const struct kernfs_node *kfs;
	char *buf;
	size_t len;
{

	switch (kfs->kfs_type) {
	case Phostname:
		if (buf[len-1] == '\n')
			--len;
		memcpy(hostname, buf, len);
		hostname[len] = '\0';
		hostnamelen = (size_t) len;
		return (0);

	default:
		return (EIO);
	}
}


/*
 * vp is the current namei directory
 * ndp is the name to locate in that directory...
 */
int
kernfs_lookup(v)
	void *v;
{
	struct vop_lookup_args /* {
		struct vnode * a_dvp;
		struct vnode ** a_vpp;
		struct componentname * a_cnp;
	} */ *ap = v;
	struct componentname *cnp = ap->a_cnp;
	struct vnode **vpp = ap->a_vpp;
	struct vnode *dvp = ap->a_dvp;
	const char *pname = cnp->cn_nameptr;
	const struct kernfs_node *kfs;
	const struct kern_target *kt;
	int error, i, wantpunlock;
#ifdef IPSEC
	char *ep;
	u_int32_t id;
#endif

	*vpp = NULLVP;
	cnp->cn_flags &= ~PDIRUNLOCK;

	if (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)
		return (EROFS);

	if (cnp->cn_namelen == 1 && *pname == '.') {
		*vpp = dvp;
		VREF(dvp);
		return (0);
	}

	wantpunlock = (~cnp->cn_flags & (LOCKPARENT | ISLASTCN));
	kfs = VTOKERN(dvp);
	switch (kfs->kfs_type) {
	case Pkern:
		/*
		 * Shouldn't get here with .. in the root node.
		 */
		if (cnp->cn_flags & ISDOTDOT)
			return (EIO);

		for (i = 0; i < nkern_targets; i++) {
			kt = &kern_targets[i];
			if (cnp->cn_namelen == kt->kt_namlen &&
			    memcmp(kt->kt_name, pname, cnp->cn_namelen) == 0)
				goto found;
		}
		break;

	found:
		error = kernfs_allocvp(dvp->v_mount, vpp, kt->kt_tag, kt, 0);
		if ((error == 0) && wantpunlock) {
			VOP_UNLOCK(dvp, 0);
			cnp->cn_flags |= PDIRUNLOCK;
		}
		return (error);

#ifdef IPSEC
	case Pipsecsadir:
		for (i = 0; i < nipsecsa_targets; i++) {
			kt = &ipsecsa_targets[i];
			if (cnp->cn_namelen == kt->kt_namlen &&
			    memcmp(kt->kt_name, pname, cnp->cn_namelen) == 0) {
				error = kernfs_allocvp(dvp->v_mount, vpp,
				    kt->kt_tag, kt, 0);
				if ((error == 0) && wantpunlock) {
					VOP_UNLOCK(dvp, 0);
					cnp->cn_flags |= PDIRUNLOCK;
				}
				return (error);
			}
		}

		ep = NULL;
		id = strtoul(pname, &ep, 10);
		if (!ep || *ep || ep == pname)
			break;

		error = kernfs_allocvp(dvp->v_mount, vpp, Pipsecsa, NULL, id);
		if ((error == 0) && wantpunlock) {
			VOP_UNLOCK(dvp, 0);
			cnp->cn_flags |= PDIRUNLOCK;
		}
		return (error);

	case Pipsecspdir:
		for (i = 0; i < nipsecsp_targets; i++) {
			kt = &ipsecsp_targets[i];
			if (cnp->cn_namelen == kt->kt_namlen &&
			    memcmp(kt->kt_name, pname, cnp->cn_namelen) == 0) {
				error = kernfs_allocvp(dvp->v_mount, vpp,
				    kt->kt_tag, kt, 0);
				if ((error == 0) && wantpunlock) {
					VOP_UNLOCK(dvp, 0);
					cnp->cn_flags |= PDIRUNLOCK;
				}
				return (error);
			}
		}

		ep = NULL;
		id = strtoul(pname, &ep, 10);
		if (!ep || *ep || ep == pname)
			break;

		error = kernfs_allocvp(dvp->v_mount, vpp, Pipsecsp, NULL, id);
		if ((error == 0) && wantpunlock) {
			VOP_UNLOCK(dvp, 0);
			cnp->cn_flags |= PDIRUNLOCK;
		}
		return (error);
#endif

	default:
		return (ENOTDIR);
	}

	return (cnp->cn_nameiop == LOOKUP ? ENOENT : EROFS);
}

int
kernfs_open(v)
	void *v;
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);
#ifdef IPSEC
	struct mbuf *m;
	struct secpolicy *sp;
#endif

	switch (kfs->kfs_type) {
#ifdef IPSEC
	case Pipsecsa:
		m = key_setdumpsa_spi(htonl(kfs->kfs_value));
		if (m) {
			m_freem(m);
			return (0);
		} else
			return (ENOENT);

	case Pipsecsp:
		sp = key_getspbyid(kfs->kfs_value);
		if (sp) {
			kfs->kfs_v = sp;
			return (0);
		} else
			return (ENOENT);
#endif

	default:
		return (0);
	}
}

int
kernfs_close(v)
	void *v;
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);

	switch (kfs->kfs_type) {
#ifdef IPSEC
	case Pipsecsp:
		key_freesp((struct secpolicy *)kfs->kfs_v);
		break;
#endif

	default:
		break;
	}

	return (0);
}

int
kernfs_access(v)
	void *v;
{
	struct vop_access_args /* {
		struct vnode *a_vp;
		int a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vattr va;
	int error;

	if ((error = VOP_GETATTR(ap->a_vp, &va, ap->a_cred, ap->a_p)) != 0)
		return (error);

	return (vaccess(va.va_type, va.va_mode, va.va_uid, va.va_gid,
	    ap->a_mode, ap->a_cred));
}

int
kernfs_getattr(v)
	void *v;
{
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);
	struct vattr *vap = ap->a_vap;
	int error = 0;
	char strbuf[KSTRING], *buf;
	size_t nread, total;

	VATTR_NULL(vap);
	vap->va_type = ap->a_vp->v_type;
	vap->va_uid = 0;
	vap->va_gid = 0;
	vap->va_mode = kfs->kfs_mode;
	vap->va_fileid = kfs->kfs_fileno;
	vap->va_flags = 0;
	vap->va_size = 0;
	vap->va_blocksize = DEV_BSIZE;
	/*
	 * Make all times be current TOD.  Avoid microtime(9), it's slow.
	 * We don't guard the read from time(9) with splclock(9) since we
	 * don't actually need to be THAT sure the access is atomic. 
	 */
	TIMEVAL_TO_TIMESPEC(&time, &vap->va_ctime);
	vap->va_atime = vap->va_mtime = vap->va_ctime;
	vap->va_gen = 0;
	vap->va_flags = 0;
	vap->va_rdev = 0;
	vap->va_bytes = 0;

	switch (kfs->kfs_type) {
	case Pkern:
		vap->va_nlink = 4;
		vap->va_bytes = vap->va_size = DEV_BSIZE;
		break;

	case Proot:
		vap->va_nlink = 1;
		vap->va_bytes = vap->va_size = DEV_BSIZE;
		break;

	case Pnull:
	case Ptime:
	case Pint:
	case Pstring:
	case Phostname:
	case Pavenrun:
	case Pdevice:
	case Pmsgbuf:
#ifdef IPSEC
	case Pipsecsa:
	case Pipsecsp:
#endif
		vap->va_nlink = 1;
		total = 0;
		do {
			buf = strbuf;
			error = kernfs_xread(kfs, total, &buf,
			    sizeof(strbuf), &nread);
			total += nread;
		} while (error == 0 && nread != 0);
		vap->va_bytes = vap->va_size = total;
		break;

#ifdef IPSEC
	case Pipsecsadir:
	case Pipsecspdir:
		vap->va_nlink = 2;
		vap->va_bytes = vap->va_size = DEV_BSIZE;
		break;
#endif

	default:
		error = EINVAL;
		break;
	}

	return (error);
}

/*ARGSUSED*/
int
kernfs_setattr(v)
	void *v;
{

	/*
	 * Silently ignore attribute changes.
	 * This allows for open with truncate to have no
	 * effect until some data is written.  I want to
	 * do it this way because all writes are atomic.
	 */
	return (0);
}

int
kernfs_read(v)
	void *v;
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct uio *uio = ap->a_uio;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);
	char strbuf[KSTRING], *buf;
	off_t off;
	size_t len;
	int error;

	if (ap->a_vp->v_type == VDIR)
		return (EOPNOTSUPP);

	off = uio->uio_offset;
	buf = strbuf;
	if ((error = kernfs_xread(kfs, off, &buf, sizeof(strbuf), &len)) == 0)
		error = uiomove(buf, len, uio);
	return (error);
}

int
kernfs_write(v)
	void *v;
{
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);
	struct uio *uio = ap->a_uio;
	int error, xlen;
	char strbuf[KSTRING];

	if (uio->uio_offset != 0)
		return (EINVAL);

	xlen = min(uio->uio_resid, KSTRING-1);
	if ((error = uiomove(strbuf, xlen, uio)) != 0)
		return (error);

	if (uio->uio_resid != 0)
		return (EIO);

	strbuf[xlen] = '\0';
	xlen = strlen(strbuf);
	return (kernfs_xwrite(kfs, strbuf, xlen));
}

int
kernfs_readdir(v)
	void *v;
{
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
		int *a_eofflag;
		off_t **a_cookies;
		int a_*ncookies;
	} */ *ap = v;
	struct uio *uio = ap->a_uio;
	struct dirent d;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);
	const struct kern_target *kt;
	off_t i;
	int error;
	off_t *cookies = NULL;
	int ncookies = 0, n;
#ifdef IPSEC
	struct secasvar *sav, *sav2;
	struct secpolicy *sp;
#endif

	if (uio->uio_resid < UIO_MX)
		return (EINVAL);
	if (uio->uio_offset < 0)
		return (EINVAL);

	error = 0;
	i = uio->uio_offset;
	memset(&d, 0, sizeof(d));
	d.d_reclen = UIO_MX;
	ncookies = uio->uio_resid / UIO_MX;

	switch (kfs->kfs_type) {
	case Pkern:
		if (i >= nkern_targets)
			return (0);

		if (ap->a_ncookies) {
			ncookies = min(ncookies, (nkern_targets - i));
			cookies = malloc(ncookies * sizeof(off_t), M_TEMP,
			    M_WAITOK);
			*ap->a_cookies = cookies;
		}

		n = 0;
		for (; i < nkern_targets && uio->uio_resid >= UIO_MX; i++) {
			kt = &kern_targets[i];
			d.d_namlen = kt->kt_namlen;
			if (i < 2)
				d.d_fileno = KERNFS_FILENO(&kern_targets[0],
				    kern_targets[0].kt_tag, 0);
			else
				d.d_fileno = KERNFS_FILENO(kt, kt->kt_tag, 0);
			memcpy(d.d_name, kt->kt_name, kt->kt_namlen + 1);
			d.d_type = kt->kt_type;
			if ((error = uiomove((caddr_t)&d, UIO_MX, uio)) != 0)
				break;
			if (cookies)
				*cookies++ = i + 1;
			n++;
		}
		ncookies = n;
		break;

	case Proot:
		if (i >= 2)
			return 0;

		if (ap->a_ncookies) {
			ncookies = min(ncookies, (2 - i));
			cookies = malloc(ncookies * sizeof(off_t), M_TEMP,
			    M_WAITOK);
			*ap->a_cookies = cookies;
		}

		n = 0;
		for (; i < 2 && uio->uio_resid >= UIO_MX; i++) {
			kt = &kern_targets[i];
			d.d_namlen = kt->kt_namlen;
			d.d_fileno = KERNFS_FILENO(kt, kt->kt_tag, 0);
			memcpy(d.d_name, kt->kt_name, kt->kt_namlen + 1);
			d.d_type = kt->kt_type;
			if ((error = uiomove((caddr_t)&d, UIO_MX, uio)) != 0)
				break;
			if (cookies)
				*cookies++ = i + 1;
			n++;
		}
		ncookies = n;
		break;

#ifdef IPSEC
	case Pipsecsadir:
		/* count SA in the system */
		n = 0;
		TAILQ_FOREACH(sav, &satailq, tailq) {
			for (sav2 = TAILQ_FIRST(&satailq);
			    sav2 != sav;
			    sav2 = TAILQ_NEXT(sav2, tailq)) {
				if (sav->spi == sav2->spi) {
					/* multiple SA with same SPI */
					break;
				}
			}
			if (sav == sav2 || sav->spi != sav2->spi)
				n++;
		}

		if (i >= nipsecsa_targets + n)
			return (0);

		if (ap->a_ncookies) {
			ncookies = min(ncookies, (n - i));
			cookies = malloc(ncookies * sizeof(off_t), M_TEMP,
			    M_WAITOK);
			*ap->a_cookies = cookies;
		}

		n = 0;
		for (; i < nipsecsa_targets && uio->uio_resid >= UIO_MX; i++) {
			kt = &ipsecsa_targets[i];
			d.d_namlen = kt->kt_namlen;
			d.d_fileno = KERNFS_FILENO(kt, kt->kt_tag, 0);
			memcpy(d.d_name, kt->kt_name, kt->kt_namlen + 1);
			d.d_type = kt->kt_type;
			if ((error = uiomove((caddr_t)&d, UIO_MX, uio)) != 0)
				break;
			if (cookies)
				*cookies++ = i + 1;
			n++;
		}
		if (error) {
			ncookies = n;
			break;
		}

		TAILQ_FOREACH(sav, &satailq, tailq) {
			for (sav2 = TAILQ_FIRST(&satailq);
			    sav2 != sav;
			    sav2 = TAILQ_NEXT(sav2, tailq)) {
				if (sav->spi == sav2->spi) {
					/* multiple SA with same SPI */
					break;
				}
			}
			if (sav != sav2 && sav->spi == sav2->spi)
				continue;
			if (uio->uio_resid < UIO_MX)
				break;
			d.d_fileno = KERNFS_FILENO(kfs->kfs_kt, kfs->kfs_type,
			    kfs->kfs_cookie);
			d.d_namlen = snprintf(d.d_name, sizeof(d.d_name),
			    "%u", ntohl(sav->spi));
			d.d_type = DT_REG;
			if ((error = uiomove((caddr_t)&d, UIO_MX, uio)) != 0)
				break;
			if (cookies)
				*cookies++ = i + 1;
			n++;
			i++;
		}
		ncookies = n;
		break;

	case Pipsecspdir:
		/* count SP in the system */
		n = 0;
		TAILQ_FOREACH(sp, &sptailq, tailq)
			n++;

		if (i >= 2 + n)
			return (0);

		if (ap->a_ncookies) {
			ncookies = min(ncookies, (n - i));
			cookies = malloc(ncookies * sizeof(off_t), M_TEMP,
			    M_WAITOK);
			*ap->a_cookies = cookies;
		}

		n = 0;
		for (; i < nipsecsp_targets && uio->uio_resid >= UIO_MX; i++) {
			kt = &ipsecsp_targets[i];
			d.d_namlen = kt->kt_namlen;
			d.d_fileno = KERNFS_FILENO(kt, kt->kt_tag, 0);
			memcpy(d.d_name, kt->kt_name, kt->kt_namlen + 1);
			d.d_type = kt->kt_type;
			if ((error = uiomove((caddr_t)&d, UIO_MX, uio)) != 0)
				break;
			if (cookies)
				*cookies++ = i + 1;
			n++;
		}
		if (error) {
			ncookies = n;
			break;
		}

		TAILQ_FOREACH(sp, &sptailq, tailq) {
			if (uio->uio_resid < UIO_MX)
				break;
			d.d_fileno = KERNFS_FILENO(kfs->kfs_kt, kfs->kfs_type,
			    kfs->kfs_cookie);
			d.d_namlen = snprintf(d.d_name, sizeof(d.d_name),
			    "%u", sp->id);
			d.d_type = DT_REG;
			if ((error = uiomove((caddr_t)&d, UIO_MX, uio)) != 0)
				break;
			if (cookies)
				*cookies++ = i + 1;
			n++;
			i++;
		}
		ncookies = n;
		break;
#endif

	default:
		error = ENOTDIR;
		break;
	}

	if (ap->a_ncookies) {
		if (error) {
			if (cookies)
				free(*ap->a_cookies, M_TEMP);
			*ap->a_ncookies = 0;
			*ap->a_cookies = NULL;
		} else
			*ap->a_ncookies = ncookies;
	}

	uio->uio_offset = i;
	return (error);
}

int
kernfs_inactive(v)
	void *v;
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	const struct kernfs_node *kfs = VTOKERN(ap->a_vp);
#ifdef IPSEC
	struct mbuf *m;
	struct secpolicy *sp;
#endif

	VOP_UNLOCK(vp, 0);
	switch (kfs->kfs_type) {
#ifdef IPSEC
	case Pipsecsa:
		m = key_setdumpsa_spi(htonl(kfs->kfs_value));
		if (m)
			m_freem(m);
		else
			vgone(vp);
		break;
	case Pipsecsp:
		sp = key_getspbyid(kfs->kfs_value);
		if (sp)
			key_freesp(sp);
		else {
			/* should never happen as we hold a refcnt */
			vgone(vp);
		}
		break;
#endif
	default:
		break;
	}
	return (0);
}

int
kernfs_reclaim(v)
	void *v;
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap = v;

	return (kernfs_freevp(ap->a_vp));
}

/*
 * Return POSIX pathconf information applicable to special devices.
 */
int
kernfs_pathconf(v)
	void *v;
{
	struct vop_pathconf_args /* {
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	} */ *ap = v;

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = LINK_MAX;
		return (0);
	case _PC_MAX_CANON:
		*ap->a_retval = MAX_CANON;
		return (0);
	case _PC_MAX_INPUT:
		*ap->a_retval = MAX_INPUT;
		return (0);
	case _PC_PIPE_BUF:
		*ap->a_retval = PIPE_BUF;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_VDISABLE:
		*ap->a_retval = _POSIX_VDISABLE;
		return (0);
	case _PC_SYNC_IO:
		*ap->a_retval = 1;
		return (0);
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

/*
 * Print out the contents of a /dev/fd vnode.
 */
/* ARGSUSED */
int
kernfs_print(v)
	void *v;
{

	printf("tag VT_KERNFS, kernfs vnode\n");
	return (0);
}

int
kernfs_link(v) 
	void *v;
{
	struct vop_link_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;  
		struct componentname *a_cnp;
	} */ *ap = v;
 
	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
	vput(ap->a_dvp);
	return (EROFS);
}

int
kernfs_symlink(v)
	void *v;
{
	struct vop_symlink_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap = v;
  
	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
	vput(ap->a_dvp);
	return (EROFS);
}
