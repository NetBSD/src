/*	$NetBSD: vfs_lookup.c,v 1.121 2010/01/08 11:35:10 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: vfs_lookup.c,v 1.121 2010/01/08 11:35:10 pooka Exp $");

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
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/kauth.h>
#include <sys/ktrace.h>

#ifndef MAGICLINKS
#define MAGICLINKS 0
#endif

struct pathname_internal {
	char *pathbuf;
	bool needfree;
};

int vfs_magiclinks = MAGICLINKS;

pool_cache_t pnbuf_cache;	/* pathname buffer cache */

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
	if ((newlen + (sl)) >= MAXPATHLEN)			\
		return 1;					\
	i += VNL(m);						\
	if (termchar != '/')					\
		i++;						\
	(void)memcpy(&tmp[newlen], (s), (sl));			\
	newlen += (sl);						\
	change = 1;						\
	termchar = '/';

static int
symlink_magic(struct proc *p, char *cp, size_t *len)
{
	char *tmp;
	size_t change, i, newlen, slen;
	char termchar = '/';
	char idtmp[11]; /* enough for 32 bit *unsigned* integer */


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
			slen = VNL(MACHINE_ARCH);
			SUBSTITUTE("machine_arch", MACHINE_ARCH, slen);
		} else if (MATCH("machine")) {
			slen = VNL(MACHINE);
			SUBSTITUTE("machine", MACHINE, slen);
		} else if (MATCH("hostname")) {
			SUBSTITUTE("hostname", hostname, hostnamelen);
		} else if (MATCH("osrelease")) {
			slen = strlen(osrelease);
			SUBSTITUTE("osrelease", osrelease, slen);
		} else if (MATCH("emul")) {
			slen = strlen(p->p_emul->e_name);
			SUBSTITUTE("emul", p->p_emul->e_name, slen);
		} else if (MATCH("kernel_ident")) {
			slen = strlen(kernel_ident);
			SUBSTITUTE("kernel_ident", kernel_ident, slen);
		} else if (MATCH("domainname")) {
			SUBSTITUTE("domainname", domainname, domainnamelen);
		} else if (MATCH("ostype")) {
			slen = strlen(ostype);
			SUBSTITUTE("ostype", ostype, slen);
		} else if (MATCH("uid")) {
			slen = snprintf(idtmp, sizeof(idtmp), "%u",
			    kauth_cred_geteuid(kauth_cred_get()));
			SUBSTITUTE("uid", idtmp, slen);
		} else if (MATCH("ruid")) {
			slen = snprintf(idtmp, sizeof(idtmp), "%u",
			    kauth_cred_getuid(kauth_cred_get()));
			SUBSTITUTE("ruid", idtmp, slen);
		} else if (MATCH("gid")) {
			slen = snprintf(idtmp, sizeof(idtmp), "%u",
			    kauth_cred_getegid(kauth_cred_get()));
			SUBSTITUTE("gid", idtmp, slen);
		} else if (MATCH("rgid")) {
			slen = snprintf(idtmp, sizeof(idtmp), "%u",
			    kauth_cred_getgid(kauth_cred_get()));
			SUBSTITUTE("rgid", idtmp, slen);
		} else {
			tmp[newlen++] = '@';
			if (termchar == VC)
				tmp[newlen++] = VO;
		}
	}

	if (change) {
		(void)memcpy(cp, tmp, newlen);
		*len = newlen;
	}
	PNBUF_PUT(tmp);

	return 0;
}

#undef VNL
#undef VO
#undef VC
#undef MATCH
#undef SUBSTITUTE

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

/*
 * Internal state for a namei operation.
 */
struct namei_state {
	struct nameidata *ndp;
	struct componentname *cnp;

	/* used by the pieces of namei */
	struct vnode *namei_startdir; /* The directory namei() starts from. */

	/* used by the pieces of lookup */
	int lookup_alldone;

	int docache;			/* == 0 do not cache last component */
	int rdonly;			/* lookup read-only flag bit */
	struct vnode *dp;		/* the directory we are searching */
	int slashes;
};

/* XXX reorder things to make this decl unnecessary */
static int do_lookup(struct namei_state *state);


/*
 * Initialize the namei working state.
 */
static void
namei_init(struct namei_state *state, struct nameidata *ndp)
{
	state->ndp = ndp;
	state->cnp = &ndp->ni_cnd;

	state->namei_startdir = NULL;

	state->lookup_alldone = 0;

	state->docache = 0;
	state->rdonly = 0;
	state->dp = NULL;
	state->slashes = 0;
}

/*
 * Clean up the working namei state, leaving things ready for return
 * from namei.
 */
static void
namei_cleanup(struct namei_state *state)
{
	KASSERT(state->cnp == &state->ndp->ni_cnd);

	//KASSERT(state->namei_startdir == NULL); 	// not yet

	/* nothing for now */
	(void)state;
}

//////////////////////////////

/*
 * Start up namei. Early portion.
 *
 * This is divided from namei_start2 by the emul_retry: point.
 */
static void
namei_start1(struct namei_state *state)
{

#ifdef DIAGNOSTIC
	if (!state->cnp->cn_cred)
		panic("namei: bad cred/proc");
	if (state->cnp->cn_nameiop & (~OPMASK))
		panic("namei: nameiop contaminated with flags");
	if (state->cnp->cn_flags & OPMASK)
		panic("namei: flags contaminated with nameiops");
#endif

	/*
	 * Get a buffer for the name to be translated, and copy the
	 * name into the buffer.
	 */
	if ((state->cnp->cn_flags & HASBUF) == 0)
		state->cnp->cn_pnbuf = PNBUF_GET();
}

/*
 * Start up namei. Copy the path, find the root dir and cwd, establish
 * the starting directory for lookup, and lock it.
 */
static int
namei_start2(struct namei_state *state)
{
	struct nameidata *ndp = state->ndp;
	struct componentname *cnp = state->cnp;

	struct cwdinfo *cwdi;		/* pointer to cwd state */
	struct lwp *self = curlwp;	/* thread doing namei() */
	int error;

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
	cwdi = self->l_proc->p_cwdi;
	rw_enter(&cwdi->cwdi_lock, RW_READER);
	state->namei_startdir = cwdi->cwdi_rdir;
	if (state->namei_startdir == NULL)
		state->namei_startdir = rootvnode;
	ndp->ni_rootdir = state->namei_startdir;

	/*
	 * Check if starting from root directory or current directory.
	 */
	if (cnp->cn_pnbuf[0] == '/') {
		if (cnp->cn_flags & TRYEMULROOT) {
			if (cnp->cn_flags & EMULROOTSET) {
				/* Called from (eg) emul_find_interp() */
				state->namei_startdir = ndp->ni_erootdir;
			} else {
				if (cwdi->cwdi_edir == NULL
				    || (cnp->cn_pnbuf[1] == '.' 
					   && cnp->cn_pnbuf[2] == '.' 
					   && cnp->cn_pnbuf[3] == '/')) {
					ndp->ni_erootdir = NULL;
				} else {
					state->namei_startdir = cwdi->cwdi_edir;
					ndp->ni_erootdir = state->namei_startdir;
				}
			}
		} else {
			ndp->ni_erootdir = NULL;
			if (cnp->cn_flags & NOCHROOT)
				state->namei_startdir = ndp->ni_rootdir = rootvnode;
		}
	} else {
		state->namei_startdir = cwdi->cwdi_cdir;
		ndp->ni_erootdir = NULL;
	}
	vref(state->namei_startdir);
	rw_exit(&cwdi->cwdi_lock);

	/*
	 * Ktrace it.
	 */
	if (ktrpoint(KTR_NAMEI)) {
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
				emul_path = self->l_proc->p_emul->e_path;
			ktrnamei2(emul_path, strlen(emul_path),
			    cnp->cn_pnbuf, ndp->ni_pathlen);
		} else
			ktrnamei(cnp->cn_pnbuf, ndp->ni_pathlen);
	}

	vn_lock(state->namei_startdir, LK_EXCLUSIVE | LK_RETRY);

	return 0;
}

/*
 * Undo namei_start: unlock and release the current lookup directory,
 * and discard the path buffer.
 */
static void
namei_end(struct namei_state *state)
{
	vput(state->namei_startdir);
	PNBUF_PUT(state->cnp->cn_pnbuf);
	//state->cnp->cn_pnbuf = NULL; // not yet (just in case) (XXX)
}

/*
 * Check for being at a symlink.
 */
static inline int
namei_atsymlink(struct namei_state *state)
{
	return (state->cnp->cn_flags & ISSYMLINK) != 0;
}

/*
 * Follow a symlink.
 */
static inline int
namei_follow(struct namei_state *state)
{
	struct nameidata *ndp = state->ndp;
	struct componentname *cnp = state->cnp;

	struct lwp *self = curlwp;	/* thread doing namei() */
	struct iovec aiov;		/* uio for reading symbolic links */
	struct uio auio;
	char *cp;			/* pointer into pathname argument */
	size_t linklen;
	int error;

	if (ndp->ni_loopcnt++ >= MAXSYMLINKS) {
		return ELOOP;
	}
	if (ndp->ni_vp->v_mount->mnt_flag & MNT_SYMPERM) {
		error = VOP_ACCESS(ndp->ni_vp, VEXEC, cnp->cn_cred);
		if (error != 0)
			return error;
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
		return error;
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
	     symlink_magic(self->l_proc, cp, &linklen)) ||
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
	state->namei_startdir = ndp->ni_dvp;

	/*
	 * Check if root directory should replace current directory.
	 */
	if (cnp->cn_pnbuf[0] == '/') {
		vput(state->namei_startdir);
		/* Keep absolute symbolic links inside emulation root */
		state->namei_startdir = ndp->ni_erootdir;
		if (state->namei_startdir == NULL || (cnp->cn_pnbuf[1] == '.' 
					  && cnp->cn_pnbuf[2] == '.'
					  && cnp->cn_pnbuf[3] == '/')) {
			ndp->ni_erootdir = NULL;
			state->namei_startdir = ndp->ni_rootdir;
		}
		vref(state->namei_startdir);
		vn_lock(state->namei_startdir, LK_EXCLUSIVE | LK_RETRY);
	}

	return 0;
}

//////////////////////////////

static int
do_namei(struct namei_state *state)
{
	int error;

	struct nameidata *ndp = state->ndp;
	struct componentname *cnp = state->cnp;

	KASSERT(cnp == &ndp->ni_cnd);

	namei_start1(state);

    emul_retry:

	error = namei_start2(state);
	if (error) {
		return error;
	}

	/* Loop through symbolic links */
	for (;;) {
		if (state->namei_startdir->v_mount == NULL) {
			/* Give up if the directory is no longer mounted */
			namei_end(state);
			return (ENOENT);
		}
		cnp->cn_nameptr = cnp->cn_pnbuf;
		ndp->ni_startdir = state->namei_startdir;
		error = do_lookup(state);
		if (error != 0) {
			/* XXX this should use namei_end() */
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
		if (namei_atsymlink(state)) {
			error = namei_follow(state);
			if (error) {
				KASSERT(ndp->ni_dvp != ndp->ni_vp);
				vput(ndp->ni_dvp);
				vput(ndp->ni_vp);
				ndp->ni_vp = NULL;
				PNBUF_PUT(cnp->cn_pnbuf);
				return error;
			}
		}
		else {
			break;
		}
	}

	/*
	 * Done
	 */

	if ((cnp->cn_flags & LOCKPARENT) == 0 && ndp->ni_dvp) {
		if (ndp->ni_dvp == ndp->ni_vp) {
			vrele(ndp->ni_dvp);
		} else {
			vput(ndp->ni_dvp);
		}
	}
	if ((cnp->cn_flags & (SAVENAME | SAVESTART)) == 0) {
		PNBUF_PUT(cnp->cn_pnbuf);
#if defined(DIAGNOSTIC)
		cnp->cn_pnbuf = NULL;
#endif /* defined(DIAGNOSTIC) */
	} else {
		cnp->cn_flags |= HASBUF;
	}

	return 0;
}

int
namei(struct nameidata *ndp)
{
	struct namei_state state;
	int error;

	namei_init(&state, ndp);
	error = do_namei(&state);
	namei_cleanup(&state);

	return error;
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

/*
 * Begin lookup().
 */
static int
lookup_start(struct namei_state *state)
{
	const char *cp;			/* pointer into pathname argument */

	struct componentname *cnp = state->cnp;
	struct nameidata *ndp = state->ndp;

	KASSERT(cnp == &ndp->ni_cnd);

	state->lookup_alldone = 0;
	state->dp = NULL;

	/*
	 * Setup: break out flag bits into variables.
	 */
	state->docache = (cnp->cn_flags & NOCACHE) ^ NOCACHE;
	if (cnp->cn_nameiop == DELETE)
		state->docache = 0;
	state->rdonly = cnp->cn_flags & RDONLY;
	ndp->ni_dvp = NULL;
	cnp->cn_flags &= ~ISSYMLINK;
	state->dp = ndp->ni_startdir;
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

		if (state->dp->v_type != VDIR) {
			vput(state->dp);
			return ENOTDIR;
		}

		/*
		 * If we've exhausted the path name, then just return the
		 * current node.
		 */
		if (cnp->cn_nameptr[0] == '\0') {
			ndp->ni_vp = state->dp;
			cnp->cn_flags |= ISLASTCN;

			/* bleh */
			state->lookup_alldone = 1;
			return 0;
		}
	}

	return 0;
}

static int
lookup_parsepath(struct namei_state *state)
{
	const char *cp;			/* pointer into pathname argument */

	struct componentname *cnp = state->cnp;
	struct nameidata *ndp = state->ndp;

	KASSERT(cnp == &ndp->ni_cnd);

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
		vput(state->dp);
		ndp->ni_dvp = NULL;
		return ENAMETOOLONG;
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
		state->slashes = cp - ndp->ni_next;
		ndp->ni_pathlen -= state->slashes;
		ndp->ni_next = cp;
		cnp->cn_flags |= REQUIREDIR;
	} else {
		state->slashes = 0;
		cnp->cn_flags &= ~REQUIREDIR;
	}
	/*
	 * We do special processing on the last component, whether or not it's
	 * a directory.  Cache all intervening lookups, but not the final one.
	 */
	if (*cp == '\0') {
		if (state->docache)
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

	return 0;
}

static int
lookup_once(struct namei_state *state)
{
	struct vnode *tdp;		/* saved dp */
	struct mount *mp;		/* mount table entry */
	struct lwp *l = curlwp;
	int error;

	struct componentname *cnp = state->cnp;
	struct nameidata *ndp = state->ndp;

	KASSERT(cnp == &ndp->ni_cnd);

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
			if (state->dp == ndp->ni_rootdir || state->dp == rootvnode) {
				ndp->ni_dvp = state->dp;
				ndp->ni_vp = state->dp;
				vref(state->dp);
				return 0;
			}
			if (ndp->ni_rootdir != rootvnode) {
				int retval;

				VOP_UNLOCK(state->dp, 0);
				retval = vn_isunder(state->dp, ndp->ni_rootdir, l);
				vn_lock(state->dp, LK_EXCLUSIVE | LK_RETRY);
				if (!retval) {
				    /* Oops! We got out of jail! */
				    log(LOG_WARNING,
					"chrooted pid %d uid %d (%s) "
					"detected outside of its chroot\n",
					p->p_pid, kauth_cred_geteuid(l->l_cred),
					p->p_comm);
				    /* Put us at the jail root. */
				    vput(state->dp);
				    state->dp = ndp->ni_rootdir;
				    ndp->ni_dvp = state->dp;
				    ndp->ni_vp = state->dp;
				    vref(state->dp);
				    vref(state->dp);
				    vn_lock(state->dp, LK_EXCLUSIVE | LK_RETRY);
				    return 0;
				}
			}
			if ((state->dp->v_vflag & VV_ROOT) == 0 ||
			    (cnp->cn_flags & NOCROSSMOUNT))
				break;
			tdp = state->dp;
			state->dp = state->dp->v_mount->mnt_vnodecovered;
			vput(tdp);
			vref(state->dp);
			vn_lock(state->dp, LK_EXCLUSIVE | LK_RETRY);
		}
	}

	/*
	 * We now have a segment name to search for, and a directory to search.
	 * Again, our only vnode state is that "dp" is held and locked.
	 */
unionlookup:
	ndp->ni_dvp = state->dp;
	ndp->ni_vp = NULL;
	error = VOP_LOOKUP(state->dp, &ndp->ni_vp, cnp);
	if (error != 0) {
#ifdef DIAGNOSTIC
		if (ndp->ni_vp != NULL)
			panic("leaf `%s' should be empty", cnp->cn_nameptr);
#endif /* DIAGNOSTIC */
#ifdef NAMEI_DIAGNOSTIC
		printf("not found\n");
#endif /* NAMEI_DIAGNOSTIC */
		if ((error == ENOENT) &&
		    (state->dp->v_vflag & VV_ROOT) &&
		    (state->dp->v_mount->mnt_flag & MNT_UNION)) {
			tdp = state->dp;
			state->dp = state->dp->v_mount->mnt_vnodecovered;
			vput(tdp);
			vref(state->dp);
			vn_lock(state->dp, LK_EXCLUSIVE | LK_RETRY);
			goto unionlookup;
		}

		if (error != EJUSTRETURN)
			return error;

		/*
		 * If this was not the last component, or there were trailing
		 * slashes, and we are not going to create a directory,
		 * then the name must exist.
		 */
		if ((cnp->cn_flags & (REQUIREDIR | CREATEDIR)) == REQUIREDIR) {
			return ENOENT;
		}

		/*
		 * If creating and at end of pathname, then can consider
		 * allowing file to be created.
		 */
		if (state->rdonly) {
			return EROFS;
		}

		/*
		 * We return with ni_vp NULL to indicate that the entry
		 * doesn't currently exist, leaving a pointer to the
		 * (possibly locked) directory vnode in ndp->ni_dvp.
		 */
		if (cnp->cn_flags & SAVESTART) {
			ndp->ni_startdir = ndp->ni_dvp;
			vref(ndp->ni_startdir);
		}
		state->lookup_alldone = 1;
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
		ndp->ni_pathlen -= cnp->cn_consume - state->slashes;
		ndp->ni_next += cnp->cn_consume - state->slashes;
		cnp->cn_consume = 0;
		if (ndp->ni_next[0] == '\0')
			cnp->cn_flags |= ISLASTCN;
	}

	state->dp = ndp->ni_vp;

	/*
	 * "state->dp" and "ndp->ni_dvp" are both locked and held,
	 * and may be the same vnode.
	 */

	/*
	 * Check to see if the vnode has been mounted on;
	 * if so find the root of the mounted file system.
	 */
	while (state->dp->v_type == VDIR && (mp = state->dp->v_mountedhere) &&
	       (cnp->cn_flags & NOCROSSMOUNT) == 0) {
		error = vfs_busy(mp, NULL);
		if (error != 0) {
			vput(state->dp);
			return error;
		}
		KASSERT(ndp->ni_dvp != state->dp);
		VOP_UNLOCK(ndp->ni_dvp, 0);
		vput(state->dp);
		error = VFS_ROOT(mp, &tdp);
		vfs_unbusy(mp, false, NULL);
		if (error) {
			vn_lock(ndp->ni_dvp, LK_EXCLUSIVE | LK_RETRY);
			return error;
		}
		VOP_UNLOCK(tdp, 0);
		ndp->ni_vp = state->dp = tdp;
		vn_lock(ndp->ni_dvp, LK_EXCLUSIVE | LK_RETRY);
		vn_lock(ndp->ni_vp, LK_EXCLUSIVE | LK_RETRY);
	}

	return 0;
}

static int
do_lookup(struct namei_state *state)
{
	int error = 0;

	struct componentname *cnp = state->cnp;
	struct nameidata *ndp = state->ndp;

	KASSERT(cnp == &ndp->ni_cnd);

	error = lookup_start(state);
	if (error) {
		goto bad;
	}
	// XXX: this case should not be necessary given proper handling
	// of slashes elsewhere.
	if (state->lookup_alldone) {
		goto terminal;
	}

dirloop:
	error = lookup_parsepath(state);
	if (error) {
		goto bad;
	}

	error = lookup_once(state);
	if (error) {
		goto bad;
	}
	// XXX ought to be able to avoid this case too
	if (state->lookup_alldone) {
		/* this should NOT be "goto terminal;" */
		return 0;
	}

	/*
	 * Check for symbolic link.  Back up over any slashes that we skipped,
	 * as we will need them again.
	 */
	if ((state->dp->v_type == VLNK) && (cnp->cn_flags & (FOLLOW|REQUIREDIR))) {
		ndp->ni_pathlen += state->slashes;
		ndp->ni_next -= state->slashes;
		cnp->cn_flags |= ISSYMLINK;
		return (0);
	}

	/*
	 * Check for directory, if the component was followed by a series of
	 * slashes.
	 */
	if ((state->dp->v_type != VDIR) && (cnp->cn_flags & REQUIREDIR)) {
		error = ENOTDIR;
		KASSERT(state->dp != ndp->ni_dvp);
		vput(state->dp);
		goto bad;
	}

	/*
	 * Not a symbolic link.  If this was not the last component, then
	 * continue at the next component, else return.
	 */
	if (!(cnp->cn_flags & ISLASTCN)) {
		cnp->cn_nameptr = ndp->ni_next;
		if (ndp->ni_dvp == state->dp) {
			vrele(ndp->ni_dvp);
		} else {
			vput(ndp->ni_dvp);
		}
		goto dirloop;
	}

terminal:
	if (state->dp == ndp->ni_erootdir) {
		/*
		 * We are about to return the emulation root.
		 * This isn't a good idea because code might repeatedly
		 * lookup ".." until the file matches that returned
		 * for "/" and loop forever.
		 * So convert it to the real root.
		 */
		if (ndp->ni_dvp == state->dp)
			vrele(state->dp);
		else
			if (ndp->ni_dvp != NULL)
				vput(ndp->ni_dvp);
		ndp->ni_dvp = NULL;
		vput(state->dp);
		state->dp = ndp->ni_rootdir;
		vref(state->dp);
		vn_lock(state->dp, LK_EXCLUSIVE | LK_RETRY);
		ndp->ni_vp = state->dp;
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
		vput(state->dp);
		goto bad;
	}

	/*
	 * Disallow directory write attempts on read-only lookups.
	 * Prefers EEXIST over EROFS for the CREATE case.
	 */
	if (state->rdonly &&
	    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
		error = EROFS;
		if (state->dp != ndp->ni_dvp) {
			vput(state->dp);
		}
		goto bad;
	}
	if (ndp->ni_dvp != NULL) {
		if (cnp->cn_flags & SAVESTART) {
			ndp->ni_startdir = ndp->ni_dvp;
			vref(ndp->ni_startdir);
		}
	}
	if ((cnp->cn_flags & LOCKLEAF) == 0) {
		VOP_UNLOCK(state->dp, 0);
	}
	return (0);

bad:
	ndp->ni_vp = NULL;
	return (error);
}

/*
 * Externally visible interfaces used by nfsd (bletch, yuk, XXX)
 *
 * The "index" version differs from the "main" version in that it's
 * called from a different place in a different context. For now I
 * want to be able to shuffle code in from one call site without
 * affecting the other.
 */

int
lookup_for_nfsd(struct nameidata *ndp, struct vnode *dp, int neverfollow)
{
	struct namei_state state;
	int error;

	struct iovec aiov;
	struct uio auio;
	int linklen;
	char *cp;

	/* For now at least we don't have to frob the state */
	namei_init(&state, ndp);

	/*
	 * BEGIN wodge of code from nfsd
	 */

	vref(dp);
	vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);

    for (;;) {

	state.cnp->cn_nameptr = state.cnp->cn_pnbuf;
	state.ndp->ni_startdir = dp;

	/*
	 * END wodge of code from nfsd
	 */

	error = do_lookup(&state);
	if (error) {
		/* BEGIN from nfsd */
		if (ndp->ni_dvp) {
			vput(ndp->ni_dvp);
		}
		PNBUF_PUT(state.cnp->cn_pnbuf);
		/* END from nfsd */
		namei_cleanup(&state);
		return error;
	}

	/*
	 * BEGIN wodge of code from nfsd
	 */

	/*
	 * Check for encountering a symbolic link
	 */
	if ((state.cnp->cn_flags & ISSYMLINK) == 0) {
		if ((state.cnp->cn_flags & LOCKPARENT) == 0 && state.ndp->ni_dvp) {
			if (state.ndp->ni_dvp == state.ndp->ni_vp) {
				vrele(state.ndp->ni_dvp);
			} else {
				vput(state.ndp->ni_dvp);
			}
		}
		if (state.cnp->cn_flags & (SAVENAME | SAVESTART)) {
			state.cnp->cn_flags |= HASBUF;
		} else {
			PNBUF_PUT(state.cnp->cn_pnbuf);
#if defined(DIAGNOSTIC)
			state.cnp->cn_pnbuf = NULL;
#endif /* defined(DIAGNOSTIC) */
		}
		return (0);
	} else {
		if (neverfollow) {
			error = EINVAL;
			goto out;
		}
		if (state.ndp->ni_loopcnt++ >= MAXSYMLINKS) {
			error = ELOOP;
			goto out;
		}
		if (state.ndp->ni_vp->v_mount->mnt_flag & MNT_SYMPERM) {
			error = VOP_ACCESS(ndp->ni_vp, VEXEC, state.cnp->cn_cred);
			if (error != 0)
				goto out;
		}
		if (state.ndp->ni_pathlen > 1)
			cp = PNBUF_GET();
		else
			cp = state.cnp->cn_pnbuf;
		aiov.iov_base = cp;
		aiov.iov_len = MAXPATHLEN;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_rw = UIO_READ;
		auio.uio_resid = MAXPATHLEN;
		UIO_SETUP_SYSSPACE(&auio);
		error = VOP_READLINK(ndp->ni_vp, &auio, state.cnp->cn_cred);
		if (error) {
badlink:
			if (ndp->ni_pathlen > 1)
				PNBUF_PUT(cp);
			goto out;
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
			PNBUF_PUT(state.cnp->cn_pnbuf);
			state.cnp->cn_pnbuf = cp;
		} else
			state.cnp->cn_pnbuf[linklen] = '\0';
		state.ndp->ni_pathlen += linklen;
		vput(state.ndp->ni_vp);
		dp = state.ndp->ni_dvp;

		/*
		 * Check if root directory should replace current directory.
		 */
		if (state.cnp->cn_pnbuf[0] == '/') {
			vput(dp);
			dp = ndp->ni_rootdir;
			vref(dp);
			vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);
		}
	}

    }
 out:
	vput(state.ndp->ni_vp);
	vput(state.ndp->ni_dvp);
	state.ndp->ni_vp = NULL;
	PNBUF_PUT(state.cnp->cn_pnbuf);

	/*
	 * END wodge of code from nfsd
	 */
	namei_cleanup(&state);

	return error;
}

int
lookup_for_nfsd_index(struct nameidata *ndp)
{
	struct namei_state state;
	int error;

	/* For now at least we don't have to frob the state */
	namei_init(&state, ndp);
	error = do_lookup(&state);
	namei_cleanup(&state);

	return error;
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
	 * Check for read-only lookups.
	 */
	if (rdonly && cnp->cn_nameiop != LOOKUP) {
		error = EROFS;
		if (*vpp) {
			vput(*vpp);
		}
		goto bad;
	}
	if (cnp->cn_flags & SAVESTART)
		vref(dvp);
	return (0);

bad:
	*vpp = NULL;
	return (error);
}

/*
 * namei_simple - simple forms of namei.
 *
 * These are wrappers to allow the simple case callers of namei to be
 * left alone while everything else changes under them.
 */

/* Flags */
struct namei_simple_flags_type {
	int dummy;
};
static const struct namei_simple_flags_type ns_nn, ns_nt, ns_fn, ns_ft;
const namei_simple_flags_t NSM_NOFOLLOW_NOEMULROOT = &ns_nn;
const namei_simple_flags_t NSM_NOFOLLOW_TRYEMULROOT = &ns_nt;
const namei_simple_flags_t NSM_FOLLOW_NOEMULROOT = &ns_fn;
const namei_simple_flags_t NSM_FOLLOW_TRYEMULROOT = &ns_ft;

static
int
namei_simple_convert_flags(namei_simple_flags_t sflags)
{
	if (sflags == NSM_NOFOLLOW_NOEMULROOT)
		return NOFOLLOW | 0;
	if (sflags == NSM_NOFOLLOW_TRYEMULROOT)
		return NOFOLLOW | TRYEMULROOT;
	if (sflags == NSM_FOLLOW_NOEMULROOT)
		return FOLLOW | 0;
	if (sflags == NSM_FOLLOW_TRYEMULROOT)
		return FOLLOW | TRYEMULROOT;
	panic("namei_simple_convert_flags: bogus sflags\n");
	return 0;
}

int
namei_simple_kernel(const char *path, namei_simple_flags_t sflags,
			struct vnode **vp_ret)
{
	struct nameidata nd;
	int err;

	NDINIT(&nd,
		LOOKUP,
		namei_simple_convert_flags(sflags),
		UIO_SYSSPACE,
		path);
	err = namei(&nd);
	if (err != 0) {
		return err;
	}
	*vp_ret = nd.ni_vp;
	return 0;
}

int
namei_simple_user(const char *path, namei_simple_flags_t sflags,
			struct vnode **vp_ret)
{
	struct nameidata nd;
	int err;

	NDINIT(&nd,
		LOOKUP,
		namei_simple_convert_flags(sflags),
		UIO_USERSPACE,
		path);
	err = namei(&nd);
	if (err != 0) {
		return err;
	}
	*vp_ret = nd.ni_vp;
	return 0;
}
