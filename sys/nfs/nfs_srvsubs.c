/*	$NetBSD: nfs_srvsubs.c,v 1.1.10.1 2009/05/13 17:22:51 jym Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
 *	@(#)nfs_subs.c	8.8 (Berkeley) 5/22/95
 */

/*
 * Copyright 2000 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nfs_srvsubs.c,v 1.1.10.1 2009/05/13 17:22:51 jym Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/time.h>
#include <sys/dirent.h>
#include <sys/once.h>
#include <sys/kauth.h>
#include <sys/atomic.h>

#include <uvm/uvm_extern.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfsnode.h>
#include <nfs/nfs.h>
#include <nfs/xdr_subs.h>
#include <nfs/nfsm_subs.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsrtt.h>
#include <nfs/nfs_var.h>

#include <miscfs/specfs/specdev.h>

#include <netinet/in.h>

/*
 * Set up nameidata for a lookup() call and do it.
 *
 * If pubflag is set, this call is done for a lookup operation on the
 * public filehandle. In that case we allow crossing mountpoints and
 * absolute pathnames. However, the caller is expected to check that
 * the lookup result is within the public fs, and deny access if
 * it is not.
 */
int
nfs_namei(struct nameidata *ndp, nfsrvfh_t *nsfh, uint32_t len, struct nfssvc_sock *slp, struct mbuf *nam, struct mbuf **mdp, char **dposp, struct vnode **retdirp, struct lwp *l, int kerbflag, int pubflag)
{
	int i, rem;
	struct mbuf *md;
	char *fromcp, *tocp, *cp;
	struct iovec aiov;
	struct uio auio;
	struct vnode *dp;
	int error, rdonly, linklen;
	struct componentname *cnp = &ndp->ni_cnd;

	*retdirp = NULL;

	if ((len + 1) > MAXPATHLEN)
		return (ENAMETOOLONG);
	if (len == 0)
		return (EACCES);
	cnp->cn_pnbuf = PNBUF_GET();

	/*
	 * Copy the name from the mbuf list to ndp->ni_pnbuf
	 * and set the various ndp fields appropriately.
	 */
	fromcp = *dposp;
	tocp = cnp->cn_pnbuf;
	md = *mdp;
	rem = mtod(md, char *) + md->m_len - fromcp;
	for (i = 0; i < len; i++) {
		while (rem == 0) {
			md = md->m_next;
			if (md == NULL) {
				error = EBADRPC;
				goto out;
			}
			fromcp = mtod(md, void *);
			rem = md->m_len;
		}
		if (*fromcp == '\0' || (!pubflag && *fromcp == '/')) {
			error = EACCES;
			goto out;
		}
		*tocp++ = *fromcp++;
		rem--;
	}
	*tocp = '\0';
	*mdp = md;
	*dposp = fromcp;
	len = nfsm_rndup(len)-len;
	if (len > 0) {
		if (rem >= len)
			*dposp += len;
		else if ((error = nfs_adv(mdp, dposp, len, rem)) != 0)
			goto out;
	}

	/*
	 * Extract and set starting directory.
	 */
	error = nfsrv_fhtovp(nsfh, false, &dp, ndp->ni_cnd.cn_cred, slp,
	    nam, &rdonly, kerbflag, pubflag);
	if (error)
		goto out;
	if (dp->v_type != VDIR) {
		vrele(dp);
		error = ENOTDIR;
		goto out;
	}

	if (rdonly)
		cnp->cn_flags |= RDONLY;

	*retdirp = dp;

	if (pubflag) {
		/*
		 * Oh joy. For WebNFS, handle those pesky '%' escapes,
		 * and the 'native path' indicator.
		 */
		cp = PNBUF_GET();
		fromcp = cnp->cn_pnbuf;
		tocp = cp;
		if ((unsigned char)*fromcp >= WEBNFS_SPECCHAR_START) {
			switch ((unsigned char)*fromcp) {
			case WEBNFS_NATIVE_CHAR:
				/*
				 * 'Native' path for us is the same
				 * as a path according to the NFS spec,
				 * just skip the escape char.
				 */
				fromcp++;
				break;
			/*
			 * More may be added in the future, range 0x80-0xff
			 */
			default:
				error = EIO;
				vrele(dp);
				PNBUF_PUT(cp);
				goto out;
			}
		}
		/*
		 * Translate the '%' escapes, URL-style.
		 */
		while (*fromcp != '\0') {
			if (*fromcp == WEBNFS_ESC_CHAR) {
				if (fromcp[1] != '\0' && fromcp[2] != '\0') {
					fromcp++;
					*tocp++ = HEXSTRTOI(fromcp);
					fromcp += 2;
					continue;
				} else {
					error = ENOENT;
					vrele(dp);
					PNBUF_PUT(cp);
					goto out;
				}
			} else
				*tocp++ = *fromcp++;
		}
		*tocp = '\0';
		PNBUF_PUT(cnp->cn_pnbuf);
		cnp->cn_pnbuf = cp;
	}

	ndp->ni_pathlen = (tocp - cnp->cn_pnbuf) + 1;
	ndp->ni_segflg = UIO_SYSSPACE;
	ndp->ni_rootdir = rootvnode;
	ndp->ni_erootdir = NULL;

	if (pubflag) {
		ndp->ni_loopcnt = 0;
		if (cnp->cn_pnbuf[0] == '/')
			dp = rootvnode;
	} else {
		cnp->cn_flags |= NOCROSSMOUNT;
	}

	VREF(dp);
	vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);

    for (;;) {
	cnp->cn_nameptr = cnp->cn_pnbuf;
	ndp->ni_startdir = dp;

	/*
	 * And call lookup() to do the real work
	 */
	error = lookup(ndp);
	if (error) {
		if (ndp->ni_dvp) {
			vput(ndp->ni_dvp);
		}
		PNBUF_PUT(cnp->cn_pnbuf);
		return (error);
	}

	/*
	 * Check for encountering a symbolic link
	 */
	if ((cnp->cn_flags & ISSYMLINK) == 0) {
		if ((cnp->cn_flags & LOCKPARENT) == 0 && ndp->ni_dvp) {
			if (ndp->ni_dvp == ndp->ni_vp) {
				vrele(ndp->ni_dvp);
			} else {
				vput(ndp->ni_dvp);
			}
		}
		if (cnp->cn_flags & (SAVENAME | SAVESTART)) {
			cnp->cn_flags |= HASBUF;
		} else {
			PNBUF_PUT(cnp->cn_pnbuf);
#if defined(DIAGNOSTIC)
			cnp->cn_pnbuf = NULL;
#endif /* defined(DIAGNOSTIC) */
		}
		return (0);
	} else {
		if (!pubflag) {
			error = EINVAL;
			break;
		}
		if (ndp->ni_loopcnt++ >= MAXSYMLINKS) {
			error = ELOOP;
			break;
		}
		if (ndp->ni_vp->v_mount->mnt_flag & MNT_SYMPERM) {
			error = VOP_ACCESS(ndp->ni_vp, VEXEC, cnp->cn_cred);
			if (error != 0)
				break;
		}
		if (ndp->ni_pathlen > 1)
			cp = PNBUF_GET();
		else
			cp = cnp->cn_pnbuf;
		aiov.iov_base = cp;
		aiov.iov_len = MAXPATHLEN;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_rw = UIO_READ;
		auio.uio_resid = MAXPATHLEN;
		UIO_SETUP_SYSSPACE(&auio);
		error = VOP_READLINK(ndp->ni_vp, &auio, cnp->cn_cred);
		if (error) {
badlink:
			if (ndp->ni_pathlen > 1)
				PNBUF_PUT(cp);
			break;
		}
		linklen = MAXPATHLEN - auio.uio_resid;
		if (linklen == 0) {
			error = ENOENT;
			goto badlink;
		}
		if (linklen + ndp->ni_pathlen >= MAXPATHLEN) {
			error = ENAMETOOLONG;
			goto badlink;
		}
		if (ndp->ni_pathlen > 1) {
			memcpy(cp + linklen, ndp->ni_next, ndp->ni_pathlen);
			PNBUF_PUT(cnp->cn_pnbuf);
			cnp->cn_pnbuf = cp;
		} else
			cnp->cn_pnbuf[linklen] = '\0';
		ndp->ni_pathlen += linklen;
		vput(ndp->ni_vp);
		dp = ndp->ni_dvp;

		/*
		 * Check if root directory should replace current directory.
		 */
		if (cnp->cn_pnbuf[0] == '/') {
			vput(dp);
			dp = ndp->ni_rootdir;
			VREF(dp);
			vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);
		}
	}
   }
	vput(ndp->ni_dvp);
	vput(ndp->ni_vp);
	ndp->ni_vp = NULL;
out:
	PNBUF_PUT(cnp->cn_pnbuf);
	return (error);
}

/*
 * nfsrv_fhtovp() - convert a fh to a vnode ptr (optionally locked)
 * 	- look up fsid in mount list (if not found ret error)
 *	- get vp and export rights by calling VFS_FHTOVP()
 *	- if cred->cr_uid == 0 or MNT_EXPORTANON set it to credanon
 *	- if not lockflag unlock it with VOP_UNLOCK()
 */
int
nfsrv_fhtovp(nfsrvfh_t *nsfh, int lockflag, struct vnode **vpp,
    kauth_cred_t cred, struct nfssvc_sock *slp, struct mbuf *nam, int *rdonlyp,
    int kerbflag, int pubflag)
{
	struct mount *mp;
	kauth_cred_t credanon;
	int error, exflags;
	struct sockaddr_in *saddr;
	fhandle_t *fhp;

	fhp = NFSRVFH_FHANDLE(nsfh);
	*vpp = (struct vnode *)0;

	if (nfs_ispublicfh(nsfh)) {
		if (!pubflag || !nfs_pub.np_valid)
			return (ESTALE);
		fhp = nfs_pub.np_handle;
	}

	error = netexport_check(&fhp->fh_fsid, nam, &mp, &exflags, &credanon);
	if (error) {
		return error;
	}

	error = VFS_FHTOVP(mp, &fhp->fh_fid, vpp);
	if (error)
		return (error);

	if (!(exflags & (MNT_EXNORESPORT|MNT_EXPUBLIC))) {
		saddr = mtod(nam, struct sockaddr_in *);
		if ((saddr->sin_family == AF_INET) &&
		    ntohs(saddr->sin_port) >= IPPORT_RESERVED) {
			vput(*vpp);
			return (NFSERR_AUTHERR | AUTH_TOOWEAK);
		}
		if ((saddr->sin_family == AF_INET6) &&
		    ntohs(saddr->sin_port) >= IPV6PORT_RESERVED) {
			vput(*vpp);
			return (NFSERR_AUTHERR | AUTH_TOOWEAK);
		}
	}
	/*
	 * Check/setup credentials.
	 */
	if (exflags & MNT_EXKERB) {
		if (!kerbflag) {
			vput(*vpp);
			return (NFSERR_AUTHERR | AUTH_TOOWEAK);
		}
	} else if (kerbflag) {
		vput(*vpp);
		return (NFSERR_AUTHERR | AUTH_TOOWEAK);
	} else if (kauth_cred_geteuid(cred) == 0 || /* NFS maproot, see below */
	    (exflags & MNT_EXPORTANON)) {
		/*
		 * This is used by the NFS maproot option. While we can change
		 * the secmodel on our own host, we can't change it on the
		 * clients. As means of least surprise, we're doing the
		 * traditional thing here.
		 * Should look into adding a "mapprivileged" or similar where
		 * the users can be explicitly specified...
		 * [elad, yamt 2008-03-05]
		 */
		kauth_cred_clone(credanon, cred);
	}
	if (exflags & MNT_EXRDONLY)
		*rdonlyp = 1;
	else
		*rdonlyp = 0;
	if (!lockflag)
		VOP_UNLOCK(*vpp, 0);
	return (0);
}

/*
 * WebNFS: check if a filehandle is a public filehandle. For v3, this
 * means a length of 0, for v2 it means all zeroes.
 */
int
nfs_ispublicfh(const nfsrvfh_t *nsfh)
{
	const char *cp = (const void *)(NFSRVFH_DATA(nsfh));
	int i;

	if (NFSRVFH_SIZE(nsfh) == 0) {
		return true;
	}
	if (NFSRVFH_SIZE(nsfh) != NFSX_V2FH) {
		return false;
	}
	for (i = 0; i < NFSX_V2FH; i++)
		if (*cp++ != 0)
			return false;
	return true;
}

int
nfsrv_composefh(struct vnode *vp, nfsrvfh_t *nsfh, bool v3)
{
	int error;
	size_t fhsize;

	fhsize = NFSD_MAXFHSIZE;
	error = vfs_composefh(vp, (void *)NFSRVFH_DATA(nsfh), &fhsize);
	if (NFSX_FHTOOBIG_P(fhsize, v3)) {
		error = EOPNOTSUPP;
	}
	if (error != 0) {
		return error;
	}
	if (!v3 && fhsize < NFSX_V2FH) {
		memset((char *)NFSRVFH_DATA(nsfh) + fhsize, 0,
		    NFSX_V2FH - fhsize);
		fhsize = NFSX_V2FH;
	}
	if ((fhsize % NFSX_UNSIGNED) != 0) {
		return EOPNOTSUPP;
	}
	nsfh->nsfh_size = fhsize;
	return 0;
}

int
nfsrv_comparefh(const nfsrvfh_t *fh1, const nfsrvfh_t *fh2)
{

	if (NFSRVFH_SIZE(fh1) != NFSRVFH_SIZE(fh2)) {
		return NFSRVFH_SIZE(fh2) - NFSRVFH_SIZE(fh1);
	}
	return memcmp(NFSRVFH_DATA(fh1), NFSRVFH_DATA(fh2), NFSRVFH_SIZE(fh1));
}

void
nfsrv_copyfh(nfsrvfh_t *fh1, const nfsrvfh_t *fh2)
{
	size_t size;

	fh1->nsfh_size = size = NFSRVFH_SIZE(fh2);
	memcpy(NFSRVFH_DATA(fh1), NFSRVFH_DATA(fh2), size);
}
