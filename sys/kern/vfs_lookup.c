/*	$NetBSD: vfs_lookup.c,v 1.91 2007/04/26 21:21:44 dsl Exp $	*/

/*
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
 *	@(#)vfs_lookup.c	8.10 (Berkeley) 5/27/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_lookup.c,v 1.91 2007/04/26 21:21:44 dsl Exp $");

#include "opt_ktrace.h"
#include "opt_systrace.h"
#include "opt_magiclinks.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslimits.h>
#include <sys/time.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/errno.h>
#include <sys/filedesc.h>
#include <sys/hash.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/kauth.h>

#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif

#ifndef MAGICLINKS
#define MAGICLINKS 0
#endif

struct pathname_internal {
	char *pathbuf;
	bool needfree;
};

int vfs_magiclinks = MAGICLINKS;

struct pool pnbuf_pool;		/* pathname buffer pool */
struct pool_cache pnbuf_cache;	/* pathname buffer cache */

/*
 * Substitute replacement text for 'magic' strings in symlinks.
 * Returns 0 if successful, and returns non-zero if an error
 * occurs.  (Currently, the only possible error is running out
 * of temporary pathname space.)
 *
 * Looks for "@<string>" and "@<string>/", where <string> is a
 * recognized 'magic' string.  Replaces the "@<string>" with the
 * appropriate replacement text.  (Note that in some cases the
 * replacement text may have zero length.)
 *
 * This would have been table driven, but the variance in
 * replacement strings (and replacement string lengths) made
 * that impractical.
 */
#define	VNL(x)							\
	(sizeof(x) - 1)

#define	VO	'{'
#define	VC	'}'

#define	MATCH(str)						\
	((termchar == '/' && i + VNL(str) == *len) ||		\
	 (i + VNL(str) < *len &&				\
	  cp[i + VNL(str)] == termchar)) &&			\
	!strncmp((str), &cp[i], VNL(str))

#define	SUBSTITUTE(m, s, sl)					\
	if ((newlen + (sl)) > MAXPATHLEN)			\
		return (1);					\
	i += VNL(m);						\
	if (termchar != '/')					\
		i++;						\
	memcpy(&tmp[newlen], (s), (sl));			\
	newlen += (sl);						\
	change = 1;						\
	termchar = '/';

static int
symlink_magic(struct proc *p, char *cp, int *len)
{
	char *tmp;
	int change, i, newlen;
	int termchar = '/';

	tmp = PNBUF_GET();
	for (change = i = newlen = 0; i < *len; ) {
		if (cp[i] != '@') {
			tmp[newlen++] = cp[i++];
			continue;
		}

		i++;

		/* Check for @{var} syntax. */
		if (cp[i] == VO) {
			termchar = VC;
			i++;
		}

		/*
		 * The following checks should be ordered according
		 * to frequency of use.
		 */
		if (MATCH("machine_arch")) {
			SUBSTITUTE("machine_arch", MACHINE_ARCH,
			    sizeof(MACHINE_ARCH) - 1);
		} else if (MATCH("machine")) {
			SUBSTITUTE("machine", MACHINE,
			    sizeof(MACHINE) - 1);
		} else if (MATCH("hostname")) {
			SUBSTITUTE("hostname", hostname,
			    hostnamelen);
		} else if (MATCH("osrelease")) {
			SUBSTITUTE("osrelease", osrelease,
			    strlen(osrelease));
		} else if (MATCH("emul")) {
			SUBSTITUTE("emul", p->p_emul->e_name,
			    strlen(p->p_emul->e_name));
		} else if (MATCH("kernel_ident")) {
			SUBSTITUTE("kernel_ident", kernel_ident,
			    strlen(kernel_ident));
		} else if (MATCH("domainname")) {
			SUBSTITUTE("domainname", domainname,
			    domainnamelen);
		} else if (MATCH("ostype")) {
			SUBSTITUTE("ostype", ostype,
			    strlen(ostype));
		} else if (MATCH("uid")) {
			char uidtmp[11]; /* XXX elad */

			(void)snprintf(uidtmp, sizeof(uidtmp), "%u",
			    kauth_cred_geteuid(kauth_cred_get()));
			SUBSTITUTE("uid", uidtmp, strlen(uidtmp));
		} else {
			tmp[newlen++] = '@';
			if (termchar == VC)
				tmp[newlen++] = VO;
		}
	}

	if (change) {
		memcpy(cp, tmp, newlen);
		*len = newlen;
	}
	PNBUF_PUT(tmp);

	return (0);
}

#undef VNL
#undef VO
#undef VC
#undef MATCH
#undef SUBSTITUTE

int
pathname_get(const char *dirp, enum uio_seg segflg, pathname_t *path)
{
	int error;

	if (dirp == NULL)
		return (EFAULT);

	*path = malloc(sizeof(struct pathname_internal), M_TEMP,
	    M_ZERO|M_WAITOK);

	if (segflg == UIO_USERSPACE) {
		(*path)->pathbuf = PNBUF_GET();
		error = copyinstr(dirp, (*path)->pathbuf, MAXPATHLEN,
		    NULL);
		if (error) {
			PNBUF_PUT((*path)->pathbuf);
			free(*path, M_TEMP);
			*path = NULL;
			return (error);
		}
		(*path)->needfree = true;
	} else {
		(*path)->pathbuf = __UNCONST(dirp);
		(*path)->needfree = false;
	}

	return (0);
}

const char *
pathname_path(pathname_t path)
{
	KASSERT(path != NULL);
	return (path->pathbuf);
}

void
pathname_put(pathname_t path)
{
	if (path != NULL) {
		if (path->pathbuf != NULL && path->needfree)
			PNBUF_PUT(path->pathbuf);
		free(path, M_TEMP);
	}
}

/*
 * Convert a pathname into a pointer to a locked vnode.
 *
 * The FOLLOW flag is set when symbolic links are to be followed
 * when they occur at the end of the name translation process.
 * Symbolic links are always followed for all other pathname
 * components other than the last.
 *
 * The segflg defines whether the name is to be copied from user
 * space or kernel space.
 *
 * Overall outline of namei:
 *
 *	copy in name
 *	get starting directory
 *	while (!done && !error) {
 *		call lookup to search path.
 *		if symbolic link, massage name in buffer and continue
 *	}
 */
int
namei(struct nameidata *ndp)
{
	struct cwdinfo *cwdi;		/* pointer to cwd state */
	char *cp;			/* pointer into pathname argument */
	struct vnode *dp;		/* the directory we are searching */
	struct iovec aiov;		/* uio for reading symbolic links */
	struct uio auio;
	int error, linklen;
	struct componentname *cnp = &ndp->ni_cnd;

#ifdef DIAGNOSTIC
	if (!cnp->cn_cred || !cnp->cn_lwp)
		panic("namei: bad cred/proc");
	if (cnp->cn_nameiop & (~OPMASK))
		panic("namei: nameiop contaminated with flags");
	if (cnp->cn_flags & OPMASK)
		panic("namei: flags contaminated with nameiops");
#endif

	/*
	 * Get a buffer for the name to be translated, and copy the
	 * name into the buffer.
	 */
	if ((cnp->cn_flags & HASBUF) == 0)
		cnp->cn_pnbuf = PNBUF_GET();
    emul_retry:
	if (ndp->ni_segflg == UIO_SYSSPACE)
		error = copystr(ndp->ni_dirp, cnp->cn_pnbuf,
			    MAXPATHLEN, &ndp->ni_pathlen);
	else
		error = copyinstr(ndp->ni_dirp, cnp->cn_pnbuf,
			    MAXPATHLEN, &ndp->ni_pathlen);

	/*
	 * POSIX.1 requirement: "" is not a valid file name.
	 */
	if (!error && ndp->ni_pathlen == 1)
		error = ENOENT;

	if (error) {
		PNBUF_PUT(cnp->cn_pnbuf);
		ndp->ni_vp = NULL;
		return (error);
	}
	ndp->ni_loopcnt = 0;

	/*
	 * Get root directory for the translation.
	 */
	/* XXX: SMP access to p_cwdi needs locking and vnodes held */
	cwdi = cnp->cn_lwp->l_proc->p_cwdi;
	dp = cwdi->cwdi_rdir;
	if (dp == NULL)
		dp = rootvnode;
	ndp->ni_rootdir = dp;

	/*
	 * Check if starting from root directory or current directory.
	 */
	if (cnp->cn_pnbuf[0] == '/') {
		if (cnp->cn_flags & TRYEMULROOT) {
			if (cnp->cn_flags & EMULROOTSET) {
				/* Called from (eg) emul_find_interp() */
				dp = ndp->ni_erootdir;
			} else {
				if (cwdi->cwdi_edir == NULL
				    || (cnp->cn_pnbuf[1] == '.' 
					   && cnp->cn_pnbuf[2] == '.' 
					   && cnp->cn_pnbuf[3] == '/')) {
					ndp->ni_erootdir = NULL;
				} else {
					dp = cwdi->cwdi_edir;
					ndp->ni_erootdir = dp;
				}
			}
		} else
			ndp->ni_erootdir = NULL;
	} else {
		dp = cwdi->cwdi_cdir;
		ndp->ni_erootdir = NULL;
	}
	VREF(dp);

#ifdef KTRACE
	if (KTRPOINT(cnp->cn_lwp->l_proc, KTR_NAMEI)) {
		if (ndp->ni_erootdir != NULL) {
			/*
			 * To make any sense, the trace entry need to have the
			 * text of the emulation path prepended.
			 * Usually we can get this from the current process,
			 * but when called from emul_find_interp() it is only
			 * in the exec_package - so we get it passed in ni_next
			 * (this is a hack).
			 */
			const char *emul_path;
			if (cnp->cn_flags & EMULROOTSET)
				emul_path = ndp->ni_next;
			else
				emul_path = cnp->cn_lwp->l_proc->p_emul->e_path;
			ktrnamei2(cnp->cn_lwp, emul_path, strlen(emul_path),
			    cnp->cn_pnbuf, ndp->ni_pathlen);
		} else
			ktrnamei(cnp->cn_lwp, cnp->cn_pnbuf, ndp->ni_pathlen);
	}
#endif
#ifdef SYSTRACE
	if (ISSET(cnp->cn_lwp->l_proc->p_flag, PK_SYSTRACE))
		systrace_namei(ndp);
#endif

	vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);
	/* Loop through symbolic links */
	for (;;) {
		if (!dp->v_mount) {
			/* Give up if the directory is no longer mounted */
			vput(dp);
			PNBUF_PUT(cnp->cn_pnbuf);
			return (ENOENT);
		}
		cnp->cn_nameptr = cnp->cn_pnbuf;
		ndp->ni_startdir = dp;
		error = lookup(ndp);
		if (error != 0) {
			if (ndp->ni_dvp) {
				vput(ndp->ni_dvp);
			}
			if (ndp->ni_erootdir != NULL) {
				/* Retry the whole thing from the normal root */
				cnp->cn_flags &= ~TRYEMULROOT;
				goto emul_retry;
			}
			PNBUF_PUT(cnp->cn_pnbuf);
			return (error);
		}

		/*
		 * Check for symbolic link
		 */
		if ((cnp->cn_flags & ISSYMLINK) == 0) {
			if ((cnp->cn_flags & LOCKPARENT) == 0 && ndp->ni_dvp) {
				if (ndp->ni_dvp == ndp->ni_vp) {
					vrele(ndp->ni_dvp);
				} else {
					vput(ndp->ni_dvp);
				}
			}
			if ((cnp->cn_flags & (SAVENAME | SAVESTART)) == 0)
				PNBUF_PUT(cnp->cn_pnbuf);
			else
				cnp->cn_flags |= HASBUF;
			return (0);
		}

		if (ndp->ni_loopcnt++ >= MAXSYMLINKS) {
			error = ELOOP;
			break;
		}
		if (ndp->ni_vp->v_mount->mnt_flag & MNT_SYMPERM) {
			error = VOP_ACCESS(ndp->ni_vp, VEXEC, cnp->cn_cred,
			    cnp->cn_lwp);
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

		/*
		 * Do symlink substitution, if appropriate, and
		 * check length for potential overflow.
		 */
		if ((vfs_magiclinks &&
		     symlink_magic(cnp->cn_lwp->l_proc, cp, &linklen)) ||
		    (linklen + ndp->ni_pathlen >= MAXPATHLEN)) {
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
			/* Keep absolute symbolic links inside emulation root */
			dp = ndp->ni_erootdir;
			if (dp == NULL || (cnp->cn_pnbuf[1] == '.' 
			    && cnp->cn_pnbuf[2] == '.'
			    && cnp->cn_pnbuf[3] == '/')) {
				ndp->ni_erootdir = NULL;
				dp = ndp->ni_rootdir;
			}
			VREF(dp);
			vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);
		}
	}
	/* Failed to process a symbolic link */
	KASSERT(ndp->ni_dvp != ndp->ni_vp);
	vput(ndp->ni_dvp);
	vput(ndp->ni_vp);
	ndp->ni_vp = NULL;
	PNBUF_PUT(cnp->cn_pnbuf);
	return (error);
}

/*
 * Determine the namei hash (for cn_hash) for name.
 * If *ep != NULL, hash from name to ep-1.
 * If *ep == NULL, hash from name until the first NUL or '/', and
 * return the location of this termination character in *ep.
 *
 * This function returns an equivalent hash to the MI hash32_strn().
 * The latter isn't used because in the *ep == NULL case, determining
 * the length of the string to the first NUL or `/' and then calling
 * hash32_strn() involves unnecessary double-handling of the data.
 */
uint32_t
namei_hash(const char *name, const char **ep)
{
	uint32_t	hash;

	hash = HASH32_STR_INIT;
	if (*ep != NULL) {
		for (; name < *ep; name++)
			hash = hash * 33 + *(const uint8_t *)name;
	} else {
		for (; *name != '\0' && *name != '/'; name++)
			hash = hash * 33 + *(const uint8_t *)name;
		*ep = name;
	}
	return (hash + (hash >> 5));
}

/*
 * Search a pathname.
 * This is a very central and rather complicated routine.
 *
 * The pathname is pointed to by ni_ptr and is of length ni_pathlen.
 * The starting directory is taken from ni_startdir. The pathname is
 * descended until done, or a symbolic link is encountered. The variable
 * ni_more is clear if the path is completed; it is set to one if a
 * symbolic link needing interpretation is encountered.
 *
 * The flag argument is LOOKUP, CREATE, RENAME, or DELETE depending on
 * whether the name is to be looked up, created, renamed, or deleted.
 * When CREATE, RENAME, or DELETE is specified, information usable in
 * creating, renaming, or deleting a directory entry may be calculated.
 * If flag has LOCKPARENT or'ed into it, the parent directory is returned
 * locked.  Otherwise the parent directory is not returned. If the target
 * of the pathname exists and LOCKLEAF is or'ed into the flag the target
 * is returned locked, otherwise it is returned unlocked.  When creating
 * or renaming and LOCKPARENT is specified, the target may not be ".".
 * When deleting and LOCKPARENT is specified, the target may be ".".
 *
 * Overall outline of lookup:
 *
 * dirloop:
 *	identify next component of name at ndp->ni_ptr
 *	handle degenerate case where name is null string
 *	if .. and crossing mount points and on mounted filesys, find parent
 *	call VOP_LOOKUP routine for next component name
 *	    directory vnode returned in ni_dvp, locked.
 *	    component vnode returned in ni_vp (if it exists), locked.
 *	if result vnode is mounted on and crossing mount points,
 *	    find mounted on vnode
 *	if more components of name, do next level at dirloop
 *	return the answer in ni_vp, locked if LOCKLEAF set
 *	    if LOCKPARENT set, return locked parent in ni_dvp
 */
int
lookup(struct nameidata *ndp)
{
	const char *cp;			/* pointer into pathname argument */
	struct vnode *dp = 0;		/* the directory we are searching */
	struct vnode *tdp;		/* saved dp */
	struct mount *mp;		/* mount table entry */
	int docache;			/* == 0 do not cache last component */
	int rdonly;			/* lookup read-only flag bit */
	int error = 0;
	int slashes;
	struct componentname *cnp = &ndp->ni_cnd;
	struct lwp *l = cnp->cn_lwp;

	/*
	 * Setup: break out flag bits into variables.
	 */
	docache = (cnp->cn_flags & NOCACHE) ^ NOCACHE;
	if (cnp->cn_nameiop == DELETE)
		docache = 0;
	rdonly = cnp->cn_flags & RDONLY;
	ndp->ni_dvp = NULL;
	cnp->cn_flags &= ~ISSYMLINK;
	dp = ndp->ni_startdir;
	ndp->ni_startdir = NULLVP;

	/*
	 * If we have a leading string of slashes, remove them, and just make
	 * sure the current node is a directory.
	 */
	cp = cnp->cn_nameptr;
	if (*cp == '/') {
		do {
			cp++;
		} while (*cp == '/');
		ndp->ni_pathlen -= cp - cnp->cn_nameptr;
		cnp->cn_nameptr = cp;

		if (dp->v_type != VDIR) {
			error = ENOTDIR;
			vput(dp);
			goto bad;
		}

		/*
		 * If we've exhausted the path name, then just return the
		 * current node.
		 */
		if (cnp->cn_nameptr[0] == '\0') {
			ndp->ni_vp = dp;
			cnp->cn_flags |= ISLASTCN;
			goto terminal;
		}
	}

dirloop:
	/*
	 * Search a new directory.
	 *
	 * The cn_hash value is for use by vfs_cache.
	 * The last component of the filename is left accessible via
	 * cnp->cn_nameptr for callers that need the name. Callers needing
	 * the name set the SAVENAME flag. When done, they assume
	 * responsibility for freeing the pathname buffer.
	 *
	 * At this point, our only vnode state is that "dp" is held and locked.
	 */
	cnp->cn_consume = 0;
	cp = NULL;
	cnp->cn_hash = namei_hash(cnp->cn_nameptr, &cp);
	cnp->cn_namelen = cp - cnp->cn_nameptr;
	if (cnp->cn_namelen > NAME_MAX) {
		vput(dp);
		error = ENAMETOOLONG;
		ndp->ni_dvp = NULL;
		goto bad;
	}
#ifdef NAMEI_DIAGNOSTIC
	{ char c = *cp;
	*(char *)cp = '\0';
	printf("{%s}: ", cnp->cn_nameptr);
	*(char *)cp = c; }
#endif /* NAMEI_DIAGNOSTIC */
	ndp->ni_pathlen -= cnp->cn_namelen;
	ndp->ni_next = cp;
	/*
	 * If this component is followed by a slash, then move the pointer to
	 * the next component forward, and remember that this component must be
	 * a directory.
	 */
	if (*cp == '/') {
		do {
			cp++;
		} while (*cp == '/');
		slashes = cp - ndp->ni_next;
		ndp->ni_pathlen -= slashes;
		ndp->ni_next = cp;
		cnp->cn_flags |= REQUIREDIR;
	} else {
		slashes = 0;
		cnp->cn_flags &= ~REQUIREDIR;
	}
	/*
	 * We do special processing on the last component, whether or not it's
	 * a directory.  Cache all intervening lookups, but not the final one.
	 */
	if (*cp == '\0') {
		if (docache)
			cnp->cn_flags |= MAKEENTRY;
		else
			cnp->cn_flags &= ~MAKEENTRY;
		cnp->cn_flags |= ISLASTCN;
	} else {
		cnp->cn_flags |= MAKEENTRY;
		cnp->cn_flags &= ~ISLASTCN;
	}
	if (cnp->cn_namelen == 2 &&
	    cnp->cn_nameptr[1] == '.' && cnp->cn_nameptr[0] == '.')
		cnp->cn_flags |= ISDOTDOT;
	else
		cnp->cn_flags &= ~ISDOTDOT;

	/*
	 * Handle "..": two special cases.
	 * 1. If at root directory (e.g. after chroot)
	 *    or at absolute root directory
	 *    then ignore it so can't get out.
	 * 1a. If at the root of the emulation filesystem go to the real
	 *    root. So "/../<path>" is always absolute.
	 * 1b. If we have somehow gotten out of a jail, warn
	 *    and also ignore it so we can't get farther out.
	 * 2. If this vnode is the root of a mounted
	 *    filesystem, then replace it with the
	 *    vnode which was mounted on so we take the
	 *    .. in the other file system.
	 */
	if (cnp->cn_flags & ISDOTDOT) {
		struct proc *p = l->l_proc;

		for (;;) {
			if (dp == ndp->ni_rootdir || dp == rootvnode) {
				ndp->ni_dvp = dp;
				ndp->ni_vp = dp;
				VREF(dp);
				goto nextname;
			}
			if (ndp->ni_rootdir != rootvnode) {
				int retval;

				VOP_UNLOCK(dp, 0);
				retval = vn_isunder(dp, ndp->ni_rootdir, l);
				vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);
				if (!retval) {
				    /* Oops! We got out of jail! */
				    log(LOG_WARNING,
					"chrooted pid %d uid %d (%s) "
					"detected outside of its chroot\n",
					p->p_pid, kauth_cred_geteuid(l->l_cred),
					p->p_comm);
				    /* Put us at the jail root. */
				    vput(dp);
				    dp = ndp->ni_rootdir;
				    ndp->ni_dvp = dp;
				    ndp->ni_vp = dp;
				    VREF(dp);
				    VREF(dp);
				    vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);
				    goto nextname;
				}
			}
			if ((dp->v_flag & VROOT) == 0 ||
			    (cnp->cn_flags & NOCROSSMOUNT))
				break;
			tdp = dp;
			dp = dp->v_mount->mnt_vnodecovered;
			vput(tdp);
			VREF(dp);
			vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);
		}
	}

	/*
	 * We now have a segment name to search for, and a directory to search.
	 * Again, our only vnode state is that "dp" is held and locked.
	 */
unionlookup:
	ndp->ni_dvp = dp;
	ndp->ni_vp = NULL;
	error = VOP_LOOKUP(dp, &ndp->ni_vp, cnp);
	if (error != 0) {
#ifdef DIAGNOSTIC
		if (ndp->ni_vp != NULL)
			panic("leaf `%s' should be empty", cnp->cn_nameptr);
#endif /* DIAGNOSTIC */
#ifdef NAMEI_DIAGNOSTIC
		printf("not found\n");
#endif /* NAMEI_DIAGNOSTIC */
		if ((error == ENOENT) &&
		    (dp->v_flag & VROOT) &&
		    (dp->v_mount->mnt_flag & MNT_UNION)) {
			tdp = dp;
			dp = dp->v_mount->mnt_vnodecovered;
			vput(tdp);
			VREF(dp);
			vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);
			goto unionlookup;
		}

		if (error != EJUSTRETURN)
			goto bad;

		/*
		 * If this was not the last component, or there were trailing
		 * slashes, and we are not going to create a directory,
		 * then the name must exist.
		 */
		if ((cnp->cn_flags & (REQUIREDIR | CREATEDIR)) == REQUIREDIR) {
			error = ENOENT;
			goto bad;
		}

		/*
		 * If creating and at end of pathname, then can consider
		 * allowing file to be created.
		 */
		if (rdonly) {
			error = EROFS;
			goto bad;
		}

		/*
		 * We return with ni_vp NULL to indicate that the entry
		 * doesn't currently exist, leaving a pointer to the
		 * (possibly locked) directory vnode in ndp->ni_dvp.
		 */
		if (cnp->cn_flags & SAVESTART) {
			ndp->ni_startdir = ndp->ni_dvp;
			VREF(ndp->ni_startdir);
		}
		return (0);
	}
#ifdef NAMEI_DIAGNOSTIC
	printf("found\n");
#endif /* NAMEI_DIAGNOSTIC */

	/*
	 * Take into account any additional components consumed by the
	 * underlying filesystem.  This will include any trailing slashes after
	 * the last component consumed.
	 */
	if (cnp->cn_consume > 0) {
		ndp->ni_pathlen -= cnp->cn_consume - slashes;
		ndp->ni_next += cnp->cn_consume - slashes;
		cnp->cn_consume = 0;
		if (ndp->ni_next[0] == '\0')
			cnp->cn_flags |= ISLASTCN;
	}

	dp = ndp->ni_vp;

	/*
	 * "dp" and "ndp->ni_dvp" are both locked and held,
	 * and may be the same vnode.
	 */

	/*
	 * Check to see if the vnode has been mounted on;
	 * if so find the root of the mounted file system.
	 */
	while (dp->v_type == VDIR && (mp = dp->v_mountedhere) &&
	       (cnp->cn_flags & NOCROSSMOUNT) == 0) {
		if (vfs_busy(mp, 0, 0))
			continue;

		KASSERT(ndp->ni_dvp != dp);
		VOP_UNLOCK(ndp->ni_dvp, 0);
		vput(dp);
		error = VFS_ROOT(mp, &tdp);
		vfs_unbusy(mp);
		if (error) {
			vn_lock(ndp->ni_dvp, LK_EXCLUSIVE | LK_RETRY);
			goto bad;
		}
		VOP_UNLOCK(tdp, 0);
		ndp->ni_vp = dp = tdp;
		vn_lock(ndp->ni_dvp, LK_EXCLUSIVE | LK_RETRY);
		vn_lock(ndp->ni_vp, LK_EXCLUSIVE | LK_RETRY);
	}

	/*
	 * Check for symbolic link.  Back up over any slashes that we skipped,
	 * as we will need them again.
	 */
	if ((dp->v_type == VLNK) && (cnp->cn_flags & (FOLLOW|REQUIREDIR))) {
		ndp->ni_pathlen += slashes;
		ndp->ni_next -= slashes;
		cnp->cn_flags |= ISSYMLINK;
		return (0);
	}

	/*
	 * Check for directory, if the component was followed by a series of
	 * slashes.
	 */
	if ((dp->v_type != VDIR) && (cnp->cn_flags & REQUIREDIR)) {
		error = ENOTDIR;
		KASSERT(dp != ndp->ni_dvp);
		vput(dp);
		goto bad;
	}

nextname:

	/*
	 * Not a symbolic link.  If this was not the last component, then
	 * continue at the next component, else return.
	 */
	if (!(cnp->cn_flags & ISLASTCN)) {
		cnp->cn_nameptr = ndp->ni_next;
		if (ndp->ni_dvp == dp) {
			vrele(ndp->ni_dvp);
		} else {
			vput(ndp->ni_dvp);
		}
		goto dirloop;
	}

terminal:
	if (dp == ndp->ni_erootdir) {
		/*
		 * We are about to return the emulation root.
		 * This isn't a good idea because code might repeatedly
		 * lookup ".." until the file matches that returned
		 * for "/" and loop forever.
		 * So convert it to the real root.
		 */
		if (ndp->ni_dvp == dp)
			vrele(dp);
		else
			if (ndp->ni_dvp != NULL)
				vput(ndp->ni_dvp);
		ndp->ni_dvp = NULL;
		vput(dp);
		dp = ndp->ni_rootdir;
		VREF(dp);
		vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);
		ndp->ni_vp = dp;
	}

	/*
	 * If the caller requested the parent node (i.e.
	 * it's a CREATE, DELETE, or RENAME), and we don't have one
	 * (because this is the root directory), then we must fail.
	 */
	if (ndp->ni_dvp == NULL && cnp->cn_nameiop != LOOKUP) {
		switch (cnp->cn_nameiop) {
		case CREATE:
			error = EEXIST;
			break;
		case DELETE:
		case RENAME:
			error = EBUSY;
			break;
		default:
			KASSERT(0);
		}
		vput(dp);
		goto bad;
	}

	/*
	 * Disallow directory write attempts on read-only file systems.
	 */
	if (rdonly &&
	    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {

		/*
		 * Disallow directory write attempts on read-only
		 * file systems.
		 */
		error = EROFS;
		if (dp != ndp->ni_dvp) {
			vput(dp);
		}
		goto bad;
	}
	if (ndp->ni_dvp != NULL) {
		if (cnp->cn_flags & SAVESTART) {
			ndp->ni_startdir = ndp->ni_dvp;
			VREF(ndp->ni_startdir);
		}
	}
	if ((cnp->cn_flags & LOCKLEAF) == 0) {
		VOP_UNLOCK(dp, 0);
	}
	return (0);

bad:
	ndp->ni_vp = NULL;
	return (error);
}

/*
 * Reacquire a path name component.
 * dvp is locked on entry and exit.
 * *vpp is locked on exit unless it's NULL.
 */
int
relookup(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp)
{
	int rdonly;			/* lookup read-only flag bit */
	int error = 0;
#ifdef DEBUG
	uint32_t newhash;		/* DEBUG: check name hash */
	const char *cp;			/* DEBUG: check name ptr/len */
#endif /* DEBUG */

	/*
	 * Setup: break out flag bits into variables.
	 */
	rdonly = cnp->cn_flags & RDONLY;
	cnp->cn_flags &= ~ISSYMLINK;

	/*
	 * Search a new directory.
	 *
	 * The cn_hash value is for use by vfs_cache.
	 * The last component of the filename is left accessible via
	 * cnp->cn_nameptr for callers that need the name. Callers needing
	 * the name set the SAVENAME flag. When done, they assume
	 * responsibility for freeing the pathname buffer.
	 */
#ifdef DEBUG
	cp = NULL;
	newhash = namei_hash(cnp->cn_nameptr, &cp);
	if ((uint32_t)newhash != (uint32_t)cnp->cn_hash)
		panic("relookup: bad hash");
	if (cnp->cn_namelen != cp - cnp->cn_nameptr)
		panic("relookup: bad len");
	while (*cp == '/')
		cp++;
	if (*cp != 0)
		panic("relookup: not last component");
#endif /* DEBUG */

	/*
	 * Check for degenerate name (e.g. / or "")
	 * which is a way of talking about a directory,
	 * e.g. like "/." or ".".
	 */
	if (cnp->cn_nameptr[0] == '\0')
		panic("relookup: null name");

	if (cnp->cn_flags & ISDOTDOT)
		panic("relookup: lookup on dot-dot");

	/*
	 * We now have a segment name to search for, and a directory to search.
	 */
	if ((error = VOP_LOOKUP(dvp, vpp, cnp)) != 0) {
#ifdef DIAGNOSTIC
		if (*vpp != NULL)
			panic("leaf `%s' should be empty", cnp->cn_nameptr);
#endif
		if (error != EJUSTRETURN)
			goto bad;
	}

#ifdef DIAGNOSTIC
	/*
	 * Check for symbolic link
	 */
	if (*vpp && (*vpp)->v_type == VLNK && (cnp->cn_flags & FOLLOW))
		panic("relookup: symlink found");
#endif

	/*
	 * Check for read-only file systems.
	 */
	if (rdonly && cnp->cn_nameiop != LOOKUP) {
		error = EROFS;
		if (*vpp) {
			vput(*vpp);
		}
		goto bad;
	}
	if (cnp->cn_flags & SAVESTART)
		VREF(dvp);
	return (0);

bad:
	*vpp = NULL;
	return (error);
}
