/*	$NetBSD: vfs_lookup.c,v 1.231 2022/02/10 10:59:12 hannken Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: vfs_lookup.c,v 1.231 2022/02/10 10:59:12 hannken Exp $");

#ifdef _KERNEL_OPT
#include "opt_magiclinks.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslimits.h>
#include <sys/time.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/vnode_impl.h>
#include <sys/mount.h>
#include <sys/errno.h>
#include <sys/filedesc.h>
#include <sys/hash.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/kauth.h>
#include <sys/ktrace.h>
#include <sys/dirent.h>

#ifndef MAGICLINKS
#define MAGICLINKS 0
#endif

int vfs_magiclinks = MAGICLINKS;

__CTASSERT(MAXNAMLEN == NAME_MAX);

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

////////////////////////////////////////////////////////////

/*
 * Determine the namei hash (for the namecache) for name.
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

////////////////////////////////////////////////////////////

/*
 * Sealed abstraction for pathnames.
 *
 * System-call-layer level code that is going to call namei should
 * first create a pathbuf and adjust all the bells and whistles on it
 * as needed by context.
 */

struct pathbuf {
	char *pb_path;
	char *pb_pathcopy;
	unsigned pb_pathcopyuses;
};

static struct pathbuf *
pathbuf_create_raw(void)
{
	struct pathbuf *pb;

	pb = kmem_alloc(sizeof(*pb), KM_SLEEP);
	pb->pb_path = PNBUF_GET();
	if (pb->pb_path == NULL) {
		kmem_free(pb, sizeof(*pb));
		return NULL;
	}
	pb->pb_pathcopy = NULL;
	pb->pb_pathcopyuses = 0;
	return pb;
}

void
pathbuf_destroy(struct pathbuf *pb)
{
	KASSERT(pb->pb_pathcopyuses == 0);
	KASSERT(pb->pb_pathcopy == NULL);
	PNBUF_PUT(pb->pb_path);
	kmem_free(pb, sizeof(*pb));
}

struct pathbuf *
pathbuf_assimilate(char *pnbuf)
{
	struct pathbuf *pb;

	pb = kmem_alloc(sizeof(*pb), KM_SLEEP);
	pb->pb_path = pnbuf;
	pb->pb_pathcopy = NULL;
	pb->pb_pathcopyuses = 0;
	return pb;
}

struct pathbuf *
pathbuf_create(const char *path)
{
	struct pathbuf *pb;
	int error;

	pb = pathbuf_create_raw();
	if (pb == NULL) {
		return NULL;
	}
	error = copystr(path, pb->pb_path, PATH_MAX, NULL);
	if (error != 0) {
		KASSERT(!"kernel path too long in pathbuf_create");
		/* make sure it's null-terminated, just in case */
		pb->pb_path[PATH_MAX-1] = '\0';
	}
	return pb;
}

int
pathbuf_copyin(const char *userpath, struct pathbuf **ret)
{
	struct pathbuf *pb;
	int error;

	pb = pathbuf_create_raw();
	if (pb == NULL) {
		return ENOMEM;
	}
	error = copyinstr(userpath, pb->pb_path, PATH_MAX, NULL);
	if (error) {
		pathbuf_destroy(pb);
		return error;
	}
	*ret = pb;
	return 0;
}

/*
 * XXX should not exist:
 *   1. whether a pointer is kernel or user should be statically checkable.
 *   2. copyin should be handled by the upper part of the syscall layer,
 *      not in here.
 */
int
pathbuf_maybe_copyin(const char *path, enum uio_seg seg, struct pathbuf **ret)
{
	if (seg == UIO_USERSPACE) {
		return pathbuf_copyin(path, ret);
	} else {
		*ret = pathbuf_create(path);
		if (*ret == NULL) {
			return ENOMEM;
		}
		return 0;
	}
}

/*
 * Get a copy of the path buffer as it currently exists. If this is
 * called after namei starts the results may be arbitrary.
 */
void
pathbuf_copystring(const struct pathbuf *pb, char *buf, size_t maxlen)
{
	strlcpy(buf, pb->pb_path, maxlen);
}

/*
 * These two functions allow access to a saved copy of the original
 * path string. The first copy should be gotten before namei is
 * called. Each copy that is gotten should be put back.
 */

const char *
pathbuf_stringcopy_get(struct pathbuf *pb)
{
	if (pb->pb_pathcopyuses == 0) {
		pb->pb_pathcopy = PNBUF_GET();
		strcpy(pb->pb_pathcopy, pb->pb_path);
	}
	pb->pb_pathcopyuses++;
	return pb->pb_pathcopy;
}

void
pathbuf_stringcopy_put(struct pathbuf *pb, const char *str)
{
	KASSERT(str == pb->pb_pathcopy);
	KASSERT(pb->pb_pathcopyuses > 0);
	pb->pb_pathcopyuses--;
	if (pb->pb_pathcopyuses == 0) {
		PNBUF_PUT(pb->pb_pathcopy);
		pb->pb_pathcopy = NULL;
	}
}


////////////////////////////////////////////////////////////

/*
 * namei: convert a pathname into a pointer to a (maybe-locked) vnode,
 * and maybe also its parent directory vnode, and assorted other guff.
 * See namei(9) for the interface documentation.
 *
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
 * Search a pathname.
 * This is a very central and rather complicated routine.
 *
 * The pathname is pointed to by ni_ptr and is of length ni_pathlen.
 * The starting directory is passed in. The pathname is descended
 * until done, or a symbolic link is encountered. The variable ni_more
 * is clear if the path is completed; it is set to one if a symbolic
 * link needing interpretation is encountered.
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
 * Internal state for a namei operation.
 *
 * cnp is always equal to &ndp->ni_cnp.
 */
struct namei_state {
	struct nameidata *ndp;
	struct componentname *cnp;

	int docache;			/* == 0 do not cache last component */
	int rdonly;			/* lookup read-only flag bit */
	int slashes;

	unsigned attempt_retry:1;	/* true if error allows emul retry */
	unsigned root_referenced:1;	/* true if ndp->ni_rootdir and
					     ndp->ni_erootdir were referenced */
};


/*
 * Initialize the namei working state.
 */
static void
namei_init(struct namei_state *state, struct nameidata *ndp)
{

	state->ndp = ndp;
	state->cnp = &ndp->ni_cnd;

	state->docache = 0;
	state->rdonly = 0;
	state->slashes = 0;

	state->root_referenced = 0;

	KASSERTMSG((state->cnp->cn_cred != NULL), "namei: bad cred/proc");
	KASSERTMSG(((state->cnp->cn_nameiop & (~OPMASK)) == 0),
	    "namei: nameiop contaminated with flags: %08"PRIx32,
	    state->cnp->cn_nameiop);
	KASSERTMSG(((state->cnp->cn_flags & OPMASK) == 0),
	    "name: flags contaminated with nameiops: %08"PRIx32,
	    state->cnp->cn_flags);

	/*
	 * The buffer for name translation shall be the one inside the
	 * pathbuf.
	 */
	state->ndp->ni_pnbuf = state->ndp->ni_pathbuf->pb_path;
}

/*
 * Clean up the working namei state, leaving things ready for return
 * from namei.
 */
static void
namei_cleanup(struct namei_state *state)
{
	KASSERT(state->cnp == &state->ndp->ni_cnd);

	if (state->root_referenced) {
		if (state->ndp->ni_rootdir != NULL)
			vrele(state->ndp->ni_rootdir);
		if (state->ndp->ni_erootdir != NULL)
			vrele(state->ndp->ni_erootdir);
	}
}

//////////////////////////////

/*
 * Get the directory context.
 * Initializes the rootdir and erootdir state and returns a reference
 * to the starting dir.
 */
static struct vnode *
namei_getstartdir(struct namei_state *state)
{
	struct nameidata *ndp = state->ndp;
	struct componentname *cnp = state->cnp;
	struct cwdinfo *cwdi;		/* pointer to cwd state */
	struct lwp *self = curlwp;	/* thread doing namei() */
	struct vnode *rootdir, *erootdir, *curdir, *startdir;

	if (state->root_referenced) {
		if (state->ndp->ni_rootdir != NULL)
			vrele(state->ndp->ni_rootdir);
		if (state->ndp->ni_erootdir != NULL)
			vrele(state->ndp->ni_erootdir);
		state->root_referenced = 0;
	}

	cwdi = self->l_proc->p_cwdi;
	rw_enter(&cwdi->cwdi_lock, RW_READER);

	/* root dir */
	if (cwdi->cwdi_rdir == NULL || (cnp->cn_flags & NOCHROOT)) {
		rootdir = rootvnode;
	} else {
		rootdir = cwdi->cwdi_rdir;
	}

	/* emulation root dir, if any */
	if ((cnp->cn_flags & TRYEMULROOT) == 0) {
		/* if we don't want it, don't fetch it */
		erootdir = NULL;
	} else if (cnp->cn_flags & EMULROOTSET) {
		/* explicitly set emulroot; "/../" doesn't override this */
		erootdir = ndp->ni_erootdir;
	} else if (!strncmp(ndp->ni_pnbuf, "/../", 4)) {
		/* explicit reference to real rootdir */
		erootdir = NULL;
	} else {
		/* may be null */
		erootdir = cwdi->cwdi_edir;
	}

	/* current dir */
	curdir = cwdi->cwdi_cdir;

	if (ndp->ni_pnbuf[0] != '/') {
		if (ndp->ni_atdir != NULL) {
			startdir = ndp->ni_atdir;
		} else {
			startdir = curdir;
		}
		erootdir = NULL;
	} else if (cnp->cn_flags & TRYEMULROOT && erootdir != NULL) {
		startdir = erootdir;
	} else {
		startdir = rootdir;
		erootdir = NULL;
	}

	state->ndp->ni_rootdir = rootdir;
	state->ndp->ni_erootdir = erootdir;

	/*
	 * Get a reference to the start dir so we can safely unlock cwdi.
	 *
	 * Must hold references to rootdir and erootdir while we're running.
	 * A multithreaded process may chroot during namei.
	 */
	if (startdir != NULL)
		vref(startdir);
	if (state->ndp->ni_rootdir != NULL)
		vref(state->ndp->ni_rootdir);
	if (state->ndp->ni_erootdir != NULL)
		vref(state->ndp->ni_erootdir);
	state->root_referenced = 1;

	rw_exit(&cwdi->cwdi_lock);
	return startdir;
}

/*
 * Get the directory context for the nfsd case, in parallel to
 * getstartdir. Initializes the rootdir and erootdir state and
 * returns a reference to the passed-in starting dir.
 */
static struct vnode *
namei_getstartdir_for_nfsd(struct namei_state *state)
{
	KASSERT(state->ndp->ni_atdir != NULL);

	/* always use the real root, and never set an emulation root */
	if (rootvnode == NULL) {
		return NULL;
	}
	state->ndp->ni_rootdir = rootvnode;
	state->ndp->ni_erootdir = NULL;

	vref(state->ndp->ni_atdir);
	KASSERT(! state->root_referenced);
	vref(state->ndp->ni_rootdir);
	state->root_referenced = 1;
	return state->ndp->ni_atdir;
}


/*
 * Ktrace the namei operation.
 */
static void
namei_ktrace(struct namei_state *state)
{
	struct nameidata *ndp = state->ndp;
	struct componentname *cnp = state->cnp;
	struct lwp *self = curlwp;	/* thread doing namei() */
	const char *emul_path;

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
			if (cnp->cn_flags & EMULROOTSET)
				emul_path = ndp->ni_next;
			else
				emul_path = self->l_proc->p_emul->e_path;
			ktrnamei2(emul_path, strlen(emul_path),
			    ndp->ni_pnbuf, ndp->ni_pathlen);
		} else
			ktrnamei(ndp->ni_pnbuf, ndp->ni_pathlen);
	}
}

/*
 * Start up namei. Find the root dir and cwd, establish the starting
 * directory for lookup, and lock it. Also calls ktrace when
 * appropriate.
 */
static int
namei_start(struct namei_state *state, int isnfsd,
	    struct vnode **startdir_ret)
{
	struct nameidata *ndp = state->ndp;
	struct vnode *startdir;

	/* length includes null terminator (was originally from copyinstr) */
	ndp->ni_pathlen = strlen(ndp->ni_pnbuf) + 1;

	/*
	 * POSIX.1 requirement: "" is not a valid file name.
	 */
	if (ndp->ni_pathlen == 1) {
		ndp->ni_erootdir = NULL;
		return ENOENT;
	}

	ndp->ni_loopcnt = 0;

	/* Get starting directory, set up root, and ktrace. */
	if (isnfsd) {
		startdir = namei_getstartdir_for_nfsd(state);
		/* no ktrace */
	} else {
		startdir = namei_getstartdir(state);
		namei_ktrace(state);
	}

	if (startdir == NULL) {
		return ENOENT;
	}

	/* NDAT may feed us with a non directory namei_getstartdir */
	if (startdir->v_type != VDIR) {
		vrele(startdir);
		return ENOTDIR;
	}

	*startdir_ret = startdir;
	return 0;
}

/*
 * Check for being at a symlink that we're going to follow.
 */
static inline int
namei_atsymlink(struct namei_state *state, struct vnode *foundobj)
{
	return (foundobj->v_type == VLNK) &&
		(state->cnp->cn_flags & (FOLLOW|REQUIREDIR));
}

/*
 * Follow a symlink.
 *
 * Updates searchdir. inhibitmagic causes magic symlinks to not be
 * interpreted; this is used by nfsd.
 *
 * Unlocks foundobj on success (ugh)
 */
static inline int
namei_follow(struct namei_state *state, int inhibitmagic,
	     struct vnode *searchdir, struct vnode *foundobj,
	     struct vnode **newsearchdir_ret)
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

	vn_lock(foundobj, LK_EXCLUSIVE | LK_RETRY);
	if (foundobj->v_mount->mnt_flag & MNT_SYMPERM) {
		error = VOP_ACCESS(foundobj, VEXEC, cnp->cn_cred);
		if (error != 0) {
			VOP_UNLOCK(foundobj);
			return error;
		}
	}

	/* FUTURE: fix this to not use a second buffer */
	cp = PNBUF_GET();
	aiov.iov_base = cp;
	aiov.iov_len = MAXPATHLEN;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = MAXPATHLEN;
	UIO_SETUP_SYSSPACE(&auio);
	error = VOP_READLINK(foundobj, &auio, cnp->cn_cred);
	VOP_UNLOCK(foundobj);
	if (error) {
		PNBUF_PUT(cp);
		return error;
	}
	linklen = MAXPATHLEN - auio.uio_resid;
	if (linklen == 0) {
		PNBUF_PUT(cp);
		return ENOENT;
	}

	/*
	 * Do symlink substitution, if appropriate, and
	 * check length for potential overflow.
	 *
	 * Inhibit symlink substitution for nfsd.
	 * XXX: This is how it was before; is that a bug or a feature?
	 */
	if ((!inhibitmagic && vfs_magiclinks &&
	     symlink_magic(self->l_proc, cp, &linklen)) ||
	    (linklen + ndp->ni_pathlen >= MAXPATHLEN)) {
		PNBUF_PUT(cp);
		return ENAMETOOLONG;
	}
	if (ndp->ni_pathlen > 1) {
		/* includes a null-terminator */
		memcpy(cp + linklen, ndp->ni_next, ndp->ni_pathlen);
	} else {
		cp[linklen] = '\0';
	}
	ndp->ni_pathlen += linklen;
	memcpy(ndp->ni_pnbuf, cp, ndp->ni_pathlen);
	PNBUF_PUT(cp);

	/* we're now starting from the beginning of the buffer again */
	cnp->cn_nameptr = ndp->ni_pnbuf;

	/*
	 * Check if root directory should replace current directory.
	 */
	if (ndp->ni_pnbuf[0] == '/') {
		vrele(searchdir);
		/* Keep absolute symbolic links inside emulation root */
		searchdir = ndp->ni_erootdir;
		if (searchdir == NULL ||
		    (ndp->ni_pnbuf[1] == '.'
		     && ndp->ni_pnbuf[2] == '.'
		     && ndp->ni_pnbuf[3] == '/')) {
			ndp->ni_erootdir = NULL;
			searchdir = ndp->ni_rootdir;
		}
		vref(searchdir);
		while (cnp->cn_nameptr[0] == '/') {
			cnp->cn_nameptr++;
			ndp->ni_pathlen--;
		}
	}

	*newsearchdir_ret = searchdir;
	return 0;
}

//////////////////////////////

/*
 * Inspect the leading path component and update the state accordingly.
 */
static int
lookup_parsepath(struct namei_state *state, struct vnode *searchdir)
{
	const char *cp;			/* pointer into pathname argument */
	int error;

	struct componentname *cnp = state->cnp;
	struct nameidata *ndp = state->ndp;

	KASSERT(cnp == &ndp->ni_cnd);

	/*
	 * Search a new directory.
	 *
	 * The last component of the filename is left accessible via
	 * cnp->cn_nameptr for callers that need the name. Callers needing
	 * the name set the SAVENAME flag. When done, they assume
	 * responsibility for freeing the pathname buffer.
	 *
	 * At this point, our only vnode state is that the search dir
	 * is held.
	 */
	error = VOP_PARSEPATH(searchdir, cnp->cn_nameptr, &cnp->cn_namelen);
	if (error) {
		return error;
	}
	cp = cnp->cn_nameptr + cnp->cn_namelen;
	if (cnp->cn_namelen > KERNEL_NAME_MAX) {
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

/*
 * Take care of crossing a mounted-on vnode.  On error, foundobj_ret will be
 * vrele'd, but searchdir is left alone.
 */
static int
lookup_crossmount(struct namei_state *state,
		  struct vnode **searchdir_ret,
		  struct vnode **foundobj_ret,
		  bool *searchdir_locked)
{
	struct componentname *cnp = state->cnp;
	struct vnode *foundobj, *vp;
	struct vnode *searchdir;
	struct mount *mp;
	int error, lktype;

	searchdir = *searchdir_ret;
	foundobj = *foundobj_ret;
	error = 0;

	KASSERT((cnp->cn_flags & NOCROSSMOUNT) == 0);

	/* First, unlock searchdir (oof). */
	if (*searchdir_locked) {
		KASSERT(searchdir != NULL);
		lktype = VOP_ISLOCKED(searchdir);
		VOP_UNLOCK(searchdir);
		*searchdir_locked = false;
	} else {
		lktype = LK_NONE;
	}

	/*
	 * Do an unlocked check to see if the vnode has been mounted on; if
	 * so find the root of the mounted file system.
	 */
	while (foundobj->v_type == VDIR &&
	    (mp = foundobj->v_mountedhere) != NULL &&
	    (cnp->cn_flags & NOCROSSMOUNT) == 0) {
		/*
		 * Try the namecache first.  If that doesn't work, do
		 * it the hard way.
		 */
		if (cache_lookup_mount(foundobj, &vp)) {
			vrele(foundobj);
			foundobj = vp;
		} else {
			/* First get the vnode stable. */
			error = vn_lock(foundobj, LK_SHARED);
			if (error != 0) {
				vrele(foundobj);
				foundobj = NULL;
				break;
			}

			/*
			 * Check to see if something is still mounted on it.
			 */
			if ((mp = foundobj->v_mountedhere) == NULL) {
				VOP_UNLOCK(foundobj);
				break;
			}

			/*
			 * Get a reference to the mountpoint, and unlock
			 * foundobj.
			 */
			error = vfs_busy(mp);
			VOP_UNLOCK(foundobj);
			if (error != 0) {
				vrele(foundobj);
				foundobj = NULL;
				break;
			}

			/*
			 * Now get a reference on the root vnode.
			 * XXX Future - maybe allow only VDIR here.
			 */
			error = VFS_ROOT(mp, LK_NONE, &vp);

			/*
			 * If successful, enter it into the cache while
			 * holding the mount busy (competing with unmount).
			 */
			if (error == 0) {
				cache_enter_mount(foundobj, vp);
			}

			/* Finally, drop references to foundobj & mountpoint. */
			vrele(foundobj);
			vfs_unbusy(mp);
			if (error) {
				foundobj = NULL;
				break;
			}
			foundobj = vp;
		}

		/*
		 * Avoid locking vnodes from two filesystems because
		 * it's prone to deadlock, e.g. when using puffs.
		 * Also, it isn't a good idea to propagate slowness of
		 * a filesystem up to the root directory. For now,
		 * only handle the common case, where foundobj is
		 * VDIR.
		 *
		 * In this case set searchdir to null to avoid using
		 * it again. It is not correct to set searchdir ==
		 * foundobj here as that will confuse the caller.
		 * (See PR 40740.)
		 */
		if (searchdir == NULL) {
			/* already been here once; do nothing further */
		} else if (foundobj->v_type == VDIR) {
			vrele(searchdir);
			*searchdir_ret = searchdir = NULL;
			lktype = LK_NONE;
		}
	}

	/* If searchdir is still around, re-lock it. */
 	if (error == 0 && lktype != LK_NONE) {
		vn_lock(searchdir, lktype | LK_RETRY);
		*searchdir_locked = true;
	}
	*foundobj_ret = foundobj;
	return error;
}

/*
 * Determine the desired locking mode for the directory of a lookup.
 */
static int
lookup_lktype(struct vnode *searchdir, struct componentname *cnp)
{

	/*
	 * If the file system supports VOP_LOOKUP() with a shared lock, and
	 * we are not making any modifications (nameiop LOOKUP) or this is
	 * not the last component then get a shared lock.  Where we can't do
	 * fast-forwarded lookups (for example with layered file systems)
	 * then this is the fallback for reducing lock contention.
	 */
	if ((searchdir->v_mount->mnt_iflag & IMNT_SHRLOOKUP) != 0 &&
	    (cnp->cn_nameiop == LOOKUP || (cnp->cn_flags & ISLASTCN) == 0)) {
		return LK_SHARED;
	} else {
		return LK_EXCLUSIVE;
	}
}

/*
 * Call VOP_LOOKUP for a single lookup; return a new search directory
 * (used when crossing mountpoints up or searching union mounts down) and
 * the found object, which for create operations may be NULL on success.
 *
 * Note that the new search directory may be null, which means the
 * searchdir was unlocked and released. This happens in the common case
 * when crossing a mount point downwards, in order to avoid coupling
 * locks between different file system volumes. Importantly, this can
 * happen even if the call fails. (XXX: this is gross and should be
 * tidied somehow.)
 */
static int
lookup_once(struct namei_state *state,
	    struct vnode *searchdir,
	    struct vnode **newsearchdir_ret,
	    struct vnode **foundobj_ret,
	    bool *newsearchdir_locked_ret)
{
	struct vnode *tmpvn;		/* scratch vnode */
	struct vnode *foundobj;		/* result */
	struct lwp *l = curlwp;
	bool searchdir_locked = false;
	int error, lktype;

	struct componentname *cnp = state->cnp;
	struct nameidata *ndp = state->ndp;

	KASSERT(cnp == &ndp->ni_cnd);
	*newsearchdir_ret = searchdir;

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
			if (searchdir == ndp->ni_rootdir ||
			    searchdir == rootvnode) {
				foundobj = searchdir;
				vref(foundobj);
				*foundobj_ret = foundobj;
				if (cnp->cn_flags & LOCKPARENT) {
					lktype = lookup_lktype(searchdir, cnp);
					vn_lock(searchdir, lktype | LK_RETRY);
					searchdir_locked = true;
				}
				error = 0;
				goto done;
			}
			if (ndp->ni_rootdir != rootvnode) {
				int retval;

				retval = vn_isunder(searchdir, ndp->ni_rootdir, l);
				if (!retval) {
				    /* Oops! We got out of jail! */
				    log(LOG_WARNING,
					"chrooted pid %d uid %d (%s) "
					"detected outside of its chroot\n",
					p->p_pid, kauth_cred_geteuid(l->l_cred),
					p->p_comm);
				    /* Put us at the jail root. */
				    vrele(searchdir);
				    searchdir = NULL;
				    foundobj = ndp->ni_rootdir;
				    vref(foundobj);
				    vref(foundobj);
				    *newsearchdir_ret = foundobj;
				    *foundobj_ret = foundobj;
				    error = 0;
				    goto done;
				}
			}
			if ((searchdir->v_vflag & VV_ROOT) == 0 ||
			    (cnp->cn_flags & NOCROSSMOUNT))
				break;
			tmpvn = searchdir;
			searchdir = searchdir->v_mount->mnt_vnodecovered;
			vref(searchdir);
			vrele(tmpvn);
			*newsearchdir_ret = searchdir;
		}
	}

	lktype = lookup_lktype(searchdir, cnp);

	/*
	 * We now have a segment name to search for, and a directory to search.
	 * Our vnode state here is that "searchdir" is held.
	 */
unionlookup:
	foundobj = NULL;
	if (!searchdir_locked) {
		vn_lock(searchdir, lktype | LK_RETRY);
		searchdir_locked = true;
	}
	error = VOP_LOOKUP(searchdir, &foundobj, cnp);

	if (error != 0) {
		KASSERTMSG((foundobj == NULL),
		    "leaf `%s' should be empty but is %p",
		    cnp->cn_nameptr, foundobj);
#ifdef NAMEI_DIAGNOSTIC
		printf("not found\n");
#endif /* NAMEI_DIAGNOSTIC */

		/*
		 * If ENOLCK, the file system needs us to retry the lookup
		 * with an exclusive lock.  It's likely nothing was found in
		 * cache and/or modifications need to be made.
		 */
		if (error == ENOLCK) {
			KASSERT(VOP_ISLOCKED(searchdir) == LK_SHARED);
			KASSERT(searchdir_locked);
			if (vn_lock(searchdir, LK_UPGRADE | LK_NOWAIT)) {
				VOP_UNLOCK(searchdir);
				searchdir_locked = false;
			}
			lktype = LK_EXCLUSIVE;
			goto unionlookup;
		}

		if ((error == ENOENT) &&
		    (searchdir->v_vflag & VV_ROOT) &&
		    (searchdir->v_mount->mnt_flag & MNT_UNION)) {
			tmpvn = searchdir;
			searchdir = searchdir->v_mount->mnt_vnodecovered;
			vref(searchdir);
			vput(tmpvn);
			searchdir_locked = false;
			*newsearchdir_ret = searchdir;
			goto unionlookup;
		}

		if (error != EJUSTRETURN)
			goto done;

		/*
		 * If this was not the last component, or there were trailing
		 * slashes, and we are not going to create a directory,
		 * then the name must exist.
		 */
		if ((cnp->cn_flags & (REQUIREDIR | CREATEDIR)) == REQUIREDIR) {
			error = ENOENT;
			goto done;
		}

		/*
		 * If creating and at end of pathname, then can consider
		 * allowing file to be created.
		 */
		if (state->rdonly) {
			error = EROFS;
			goto done;
		}

		/*
		 * We return success and a NULL foundobj to indicate
		 * that the entry doesn't currently exist, leaving a
		 * pointer to the (normally, locked) directory vnode
		 * as searchdir.
		 */
		*foundobj_ret = NULL;
		error = 0;
		goto done;
	}
#ifdef NAMEI_DIAGNOSTIC
	printf("found\n");
#endif /* NAMEI_DIAGNOSTIC */

	/* Unlock, unless the caller needs the parent locked. */
	if (searchdir != NULL) {
		KASSERT(searchdir_locked);
		if ((cnp->cn_flags & (ISLASTCN | LOCKPARENT)) !=
		    (ISLASTCN | LOCKPARENT)) {
		    	VOP_UNLOCK(searchdir);
		    	searchdir_locked = false;
		}
	} else {
		KASSERT(!searchdir_locked);
	}

	*foundobj_ret = foundobj;
	error = 0;
done:
	*newsearchdir_locked_ret = searchdir_locked;
	return error;
}

/*
 * Parse out the first path name component that we need to to consider.
 *
 * While doing this, attempt to use the name cache to fast-forward through
 * as many "easy" to find components of the path as possible.
 *
 * We use the namecache's node locks to form a chain, and avoid as many
 * vnode references and locks as possible.  In the ideal case, only the
 * final vnode will have its reference count adjusted and lock taken.
 */
static int
lookup_fastforward(struct namei_state *state, struct vnode **searchdir_ret,
		   struct vnode **foundobj_ret)
{
	struct componentname *cnp = state->cnp;
	struct nameidata *ndp = state->ndp;
	krwlock_t *plock;
	struct vnode *foundobj, *searchdir;
	int error, error2;
	size_t oldpathlen;
	const char *oldnameptr;
	bool terminal;

	/*
	 * Eat as many path name components as possible before giving up and
	 * letting lookup_once() handle it.  Remember the starting point in
	 * case we can't get vnode references and need to roll back.
	 */
	plock = NULL;
	searchdir = *searchdir_ret;
	oldnameptr = cnp->cn_nameptr;
	oldpathlen = ndp->ni_pathlen;
	terminal = false;
	for (;;) {
		foundobj = NULL;

		/*
		 * Get the next component name.  There should be no slashes
		 * here, and we shouldn't have looped around if we were
		 * done.
		 */
		KASSERT(cnp->cn_nameptr[0] != '/');
		KASSERT(cnp->cn_nameptr[0] != '\0');
		if ((error = lookup_parsepath(state, searchdir)) != 0) {
			break;
		}

		/*
		 * Can't deal with DOTDOT lookups if NOCROSSMOUNT or the
		 * lookup is chrooted.
		 */
		if ((cnp->cn_flags & ISDOTDOT) != 0) {
			if ((searchdir->v_vflag & VV_ROOT) != 0 &&
			    (cnp->cn_flags & NOCROSSMOUNT)) {
			    	error = EOPNOTSUPP;
				break;
			}
			if (ndp->ni_rootdir != rootvnode) {
			    	error = EOPNOTSUPP;
				break;
			}
		}

		/*
		 * Can't deal with last component when modifying; this needs
		 * searchdir locked and VOP_LOOKUP() called (which can and
		 * does modify state, despite the name).  NB: this case means
		 * terminal is never set true when LOCKPARENT.
		 */
		if ((cnp->cn_flags & ISLASTCN) != 0) {
			if (cnp->cn_nameiop != LOOKUP ||
			    (cnp->cn_flags & LOCKPARENT) != 0) {
				error = EOPNOTSUPP;
				break;
			}
		}

		/*
		 * Good, now look for it in cache.  cache_lookup_linked()
		 * will fail if there's nothing there, or if there's no
		 * ownership info for the directory, or if the user doesn't
		 * have permission to look up files in this directory.
		 */
		if (!cache_lookup_linked(searchdir, cnp->cn_nameptr,
		    cnp->cn_namelen, &foundobj, &plock, cnp->cn_cred)) {
			error = EOPNOTSUPP;
			break;
		}
		KASSERT(plock != NULL && rw_lock_held(plock));

		/*
		 * Scored a hit.  Negative is good too (ENOENT).  If there's
		 * a '-o union' mount here, punt and let lookup_once() deal
		 * with it.
		 */
		if (foundobj == NULL) {
			if ((searchdir->v_vflag & VV_ROOT) != 0 &&
			    (searchdir->v_mount->mnt_flag & MNT_UNION) != 0) {
			    	error = EOPNOTSUPP;
			} else {
				error = ENOENT;
				terminal = ((cnp->cn_flags & ISLASTCN) != 0);
			}
			break;
		}

		/*
		 * Stop and get a hold on the vnode if we've encountered
		 * something other than a dirctory.
		 */
		if (foundobj->v_type != VDIR) {
			error = vcache_tryvget(foundobj);
			if (error != 0) {
				foundobj = NULL;
				error = EOPNOTSUPP;
			} else {
				terminal = (foundobj->v_type != VLNK &&
				    (cnp->cn_flags & ISLASTCN) != 0);
			}
			break;
		}

		/*
		 * Try to cross mountpoints, bearing in mind that they can
		 * be stacked.  If at any point we can't go further, stop
		 * and try to get a reference on the vnode.  If we are able
		 * to get a ref then lookup_crossmount() will take care of
		 * it, otherwise we'll fall through to lookup_once().
		 */
		if (foundobj->v_mountedhere != NULL) {
			while (foundobj->v_mountedhere != NULL &&
			    (cnp->cn_flags & NOCROSSMOUNT) == 0 &&
			    cache_cross_mount(&foundobj, &plock)) {
				KASSERT(foundobj != NULL);
				KASSERT(foundobj->v_type == VDIR);
			}
			if (foundobj->v_mountedhere != NULL) {
				error = vcache_tryvget(foundobj);
				if (error != 0) {
					foundobj = NULL;
					error = EOPNOTSUPP;
				}
				break;
			} else {
				searchdir = NULL;
			}
		}

		/*
		 * Time to stop if we found the last component & traversed
		 * all mounts.
		 */
		if ((cnp->cn_flags & ISLASTCN) != 0) {
			error = vcache_tryvget(foundobj);
			if (error != 0) {
				foundobj = NULL;
				error = EOPNOTSUPP;
			} else {
				terminal = (foundobj->v_type != VLNK);
			}
			break;
		}

		/*
		 * Otherwise, we're still in business.  Set the found VDIR
		 * vnode as the search dir for the next component and
		 * continue on to it.
		 */
		cnp->cn_nameptr = ndp->ni_next;
		searchdir = foundobj;
	}

	if (terminal) {
		/*
		 * If we exited the loop above having successfully located
		 * the last component with a zero error code, and it's not a
		 * symbolic link, then the parent directory is not needed.
		 * Release reference to the starting parent and make the
		 * terminal parent disappear into thin air.
		 */
		KASSERT(plock != NULL);
		rw_exit(plock);
		vrele(*searchdir_ret);
		*searchdir_ret = NULL;
	} else if (searchdir != *searchdir_ret) {
		/*
		 * Otherwise we need to return the parent.  If we ended up
		 * with a new search dir, ref it before dropping the
		 * namecache's lock.  The lock prevents both searchdir and
		 * foundobj from disappearing.  If we can't ref the new
		 * searchdir, we have a bit of a problem.  Roll back the
		 * fastforward to the beginning and let lookup_once() take
		 * care of it.
		 */
		if (searchdir == NULL) {
			/*
			 * It's possible for searchdir to be NULL in the
			 * case of a root vnode being reclaimed while
			 * trying to cross a mount.
			 */
			error2 = EOPNOTSUPP;
		} else {
			error2 = vcache_tryvget(searchdir);
		}
		KASSERT(plock != NULL);
		rw_exit(plock);
		if (__predict_true(error2 == 0)) {
			/* Returning new searchdir, and maybe new foundobj. */
			vrele(*searchdir_ret);
			*searchdir_ret = searchdir;
		} else {
			/* Returning nothing. */
			if (foundobj != NULL) {
				vrele(foundobj);
				foundobj = NULL;
			}
			cnp->cn_nameptr = oldnameptr;
			ndp->ni_pathlen = oldpathlen;
			error = lookup_parsepath(state, *searchdir_ret);
			if (error == 0) {
				error = EOPNOTSUPP;
			}
		}
	} else if (plock != NULL) {
		/* Drop any namecache lock still held. */
		rw_exit(plock);
	}

	KASSERT(error == 0 ? foundobj != NULL : foundobj == NULL);
	*foundobj_ret = foundobj;
	return error;
}

//////////////////////////////

/*
 * Do a complete path search from a single root directory.
 * (This is called up to twice if TRYEMULROOT is in effect.)
 */
static int
namei_oneroot(struct namei_state *state,
	 int neverfollow, int inhibitmagic, int isnfsd)
{
	struct nameidata *ndp = state->ndp;
	struct componentname *cnp = state->cnp;
	struct vnode *searchdir, *foundobj;
	bool searchdir_locked = false;
	int error;

	error = namei_start(state, isnfsd, &searchdir);
	if (error) {
		ndp->ni_dvp = NULL;
		ndp->ni_vp = NULL;
		return error;
	}
	KASSERT(searchdir->v_type == VDIR);

	/*
	 * Setup: break out flag bits into variables.
	 */
	state->docache = (cnp->cn_flags & NOCACHE) ^ NOCACHE;
	if (cnp->cn_nameiop == DELETE)
		state->docache = 0;
	state->rdonly = cnp->cn_flags & RDONLY;

	/*
	 * Keep going until we run out of path components.
	 */
	cnp->cn_nameptr = ndp->ni_pnbuf;

	/* drop leading slashes (already used them to choose startdir) */
	while (cnp->cn_nameptr[0] == '/') {
		cnp->cn_nameptr++;
		ndp->ni_pathlen--;
	}
	/* was it just "/"? */
	if (cnp->cn_nameptr[0] == '\0') {
		foundobj = searchdir;
		searchdir = NULL;
		cnp->cn_flags |= ISLASTCN;

		/* bleh */
		goto skiploop;
	}

	for (;;) {
		KASSERT(searchdir != NULL);
		KASSERT(!searchdir_locked);

		/*
		 * Parse out the first path name component that we need to
		 * to consider.  While doing this, attempt to use the name
		 * cache to fast-forward through as many "easy" to find
		 * components of the path as possible.
		 */
		error = lookup_fastforward(state, &searchdir, &foundobj);

		/*
		 * If we didn't get a good answer from the namecache, then
		 * go directly to the file system.
		 */
		if (error == EOPNOTSUPP) {
			error = lookup_once(state, searchdir, &searchdir,
			    &foundobj, &searchdir_locked);
		}

		/*
		 * If the vnode we found is mounted on, then cross the mount
		 * and get the root vnode in foundobj.  If this encounters
		 * an error, it will dispose of foundobj, but searchdir is
		 * untouched.
		 */
		if (error == 0 && foundobj != NULL &&
		    foundobj->v_type == VDIR &&
		    foundobj->v_mountedhere != NULL &&
		    (cnp->cn_flags & NOCROSSMOUNT) == 0) {
		    	error = lookup_crossmount(state, &searchdir,
		    	    &foundobj, &searchdir_locked);
		}

		if (error) {
			if (searchdir != NULL) {
				if (searchdir_locked) {
					searchdir_locked = false;
					vput(searchdir);
				} else {
					vrele(searchdir);
				}
			}
			ndp->ni_dvp = NULL;
			ndp->ni_vp = NULL;
			/*
			 * Note that if we're doing TRYEMULROOT we can
			 * retry with the normal root. Where this is
			 * currently set matches previous practice,
			 * but the previous practice didn't make much
			 * sense and somebody should sit down and
			 * figure out which cases should cause retry
			 * and which shouldn't. XXX.
			 */
			state->attempt_retry = 1;
			return (error);
		}

		if (foundobj == NULL) {
			/*
			 * Success with no object returned means we're
			 * creating something and it isn't already
			 * there. Break out of the main loop now so
			 * the code below doesn't have to test for
			 * foundobj == NULL.
			 */
			/* lookup_once can't have dropped the searchdir */
			KASSERT(searchdir != NULL ||
			    (cnp->cn_flags & ISLASTCN) != 0);
			break;
		}

		/*
		 * Check for symbolic link. If we've reached one,
		 * follow it, unless we aren't supposed to. Back up
		 * over any slashes that we skipped, as we will need
		 * them again.
		 */
		if (namei_atsymlink(state, foundobj)) {
			/* Don't need searchdir locked any more. */
			if (searchdir_locked) {
				searchdir_locked = false;
				VOP_UNLOCK(searchdir);
			}
			ndp->ni_pathlen += state->slashes;
			ndp->ni_next -= state->slashes;
			if (neverfollow) {
				error = EINVAL;
			} else if (searchdir == NULL) {
				/*
				 * dholland 20160410: lookup_once only
				 * drops searchdir if it crossed a
				 * mount point. Therefore, if we get
				 * here it means we crossed a mount
				 * point to a mounted filesystem whose
				 * root vnode is a symlink. In theory
				 * we could continue at this point by
				 * using the pre-crossing searchdir
				 * (e.g. just take out an extra
				 * reference on it before calling
				 * lookup_once so we still have it),
				 * but this will make an ugly mess and
				 * it should never happen in practice
				 * as only badly broken filesystems
				 * have non-directory root vnodes. (I
				 * have seen this sort of thing with
				 * NFS occasionally but even then it
				 * means something's badly wrong.)
				 */
				error = ENOTDIR;
			} else {
				/*
				 * dholland 20110410: if we're at a
				 * union mount it might make sense to
				 * use the top of the union stack here
				 * rather than the layer we found the
				 * symlink in. (FUTURE)
				 */
				error = namei_follow(state, inhibitmagic,
						     searchdir, foundobj,
						     &searchdir);
			}
			if (error) {
				KASSERT(searchdir != foundobj);
				if (searchdir != NULL) {
					vrele(searchdir);
				}
				vrele(foundobj);
				ndp->ni_dvp = NULL;
				ndp->ni_vp = NULL;
				return error;
			}
			vrele(foundobj);
			foundobj = NULL;

			/*
			 * If we followed a symlink to `/' and there
			 * are no more components after the symlink,
			 * we're done with the loop and what we found
			 * is the searchdir.
			 */
			if (cnp->cn_nameptr[0] == '\0') {
				KASSERT(searchdir != NULL);
				foundobj = searchdir;
				searchdir = NULL;
				cnp->cn_flags |= ISLASTCN;
				break;
			}

			continue;
		}

		/*
		 * Not a symbolic link.
		 *
		 * Check for directory, if the component was
		 * followed by a series of slashes.
		 */
		if ((foundobj->v_type != VDIR) &&
		    (cnp->cn_flags & REQUIREDIR)) {
			KASSERT(foundobj != searchdir);
			if (searchdir) {
				if (searchdir_locked) {
					searchdir_locked = false;
					vput(searchdir);
				} else {
					vrele(searchdir);
				}
			} else {
				KASSERT(!searchdir_locked);
			}
			vrele(foundobj);
			ndp->ni_dvp = NULL;
			ndp->ni_vp = NULL;
			state->attempt_retry = 1;
			return ENOTDIR;
		}

		/*
		 * Stop if we've reached the last component.
		 */
		if (cnp->cn_flags & ISLASTCN) {
			break;
		}

		/*
		 * Continue with the next component.
		 */
		cnp->cn_nameptr = ndp->ni_next;
		if (searchdir != NULL) {
			if (searchdir_locked) {
				searchdir_locked = false;
				vput(searchdir);
			} else {
				vrele(searchdir);
			}
		}
		searchdir = foundobj;
		foundobj = NULL;
	}

	KASSERT((cnp->cn_flags & LOCKPARENT) == 0 || searchdir == NULL ||
	    VOP_ISLOCKED(searchdir) == LK_EXCLUSIVE);

 skiploop:

	if (foundobj != NULL) {
		if (foundobj == ndp->ni_erootdir) {
			/*
			 * We are about to return the emulation root.
			 * This isn't a good idea because code might
			 * repeatedly lookup ".." until the file
			 * matches that returned for "/" and loop
			 * forever.  So convert it to the real root.
			 */
			if (searchdir != NULL) {
				if (searchdir_locked) {
					vput(searchdir);
					searchdir_locked = false;
				} else {
					vrele(searchdir);
				}
				searchdir = NULL;
			}
			vrele(foundobj);
			foundobj = ndp->ni_rootdir;
			vref(foundobj);
		}

		/*
		 * If the caller requested the parent node (i.e. it's
		 * a CREATE, DELETE, or RENAME), and we don't have one
		 * (because this is the root directory, or we crossed
		 * a mount point), then we must fail.
		 *
		 * 20210604 dholland when NONEXCLHACK is set (open
		 * with O_CREAT but not O_EXCL) skip this logic. Since
		 * we have a foundobj, open will not be creating, so
		 * it doesn't actually need or use the searchdir, so
		 * it's ok to return it even if it's on a different
		 * volume, and it's also ok to return NULL; by setting
		 * NONEXCLHACK the open code promises to cope with
		 * those cases correctly. (That is, it should do what
		 * it would do anyway, that is, just release the
		 * searchdir, except not crash if it's null.) This is
		 * needed because otherwise opening mountpoints with
		 * O_CREAT but not O_EXCL fails... which is a silly
		 * thing to do but ought to work. (This whole issue
		 * came to light because 3rd party code wanted to open
		 * certain procfs nodes with O_CREAT for some 3rd
		 * party reason, and it failed.)
		 *
		 * Note that NONEXCLHACK is properly a different
		 * nameiop (it is partway between LOOKUP and CREATE)
		 * but it was stuffed in as a flag instead to make the
		 * resulting patch less invasive for pullup. Blah.
		 */
		if (cnp->cn_nameiop != LOOKUP &&
		    (searchdir == NULL ||
		     searchdir->v_mount != foundobj->v_mount) &&
		    (cnp->cn_flags & NONEXCLHACK) == 0) {
			if (searchdir) {
				if (searchdir_locked) {
					vput(searchdir);
					searchdir_locked = false;
				} else {
					vrele(searchdir);
				}
				searchdir = NULL;
			}
			vrele(foundobj);
			foundobj = NULL;
			ndp->ni_dvp = NULL;
			ndp->ni_vp = NULL;
			state->attempt_retry = 1;

			switch (cnp->cn_nameiop) {
			    case CREATE:
				return EEXIST;
			    case DELETE:
			    case RENAME:
				return EBUSY;
			    default:
				break;
			}
			panic("Invalid nameiop\n");
		}

		/*
		 * Disallow directory write attempts on read-only lookups.
		 * Prefers EEXIST over EROFS for the CREATE case.
		 */
		if (state->rdonly &&
		    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
			if (searchdir) {
				if (searchdir_locked) {
					vput(searchdir);
					searchdir_locked = false;
				} else {
					vrele(searchdir);
				}
				searchdir = NULL;
			}
			vrele(foundobj);
			foundobj = NULL;
			ndp->ni_dvp = NULL;
			ndp->ni_vp = NULL;
			state->attempt_retry = 1;
			return EROFS;
		}

		/* Lock the leaf node if requested. */
		if ((cnp->cn_flags & (LOCKLEAF | LOCKPARENT)) == LOCKPARENT &&
		    searchdir == foundobj) {
			/*
			 * Note: if LOCKPARENT but not LOCKLEAF is
			 * set, and searchdir == foundobj, this code
			 * necessarily unlocks the parent as well as
			 * the leaf. That is, just because you specify
			 * LOCKPARENT doesn't mean you necessarily get
			 * a locked parent vnode. The code in
			 * vfs_syscalls.c, and possibly elsewhere,
			 * that uses this combination "knows" this, so
			 * it can't be safely changed. Feh. XXX
			 */
			KASSERT(searchdir_locked);
		    	VOP_UNLOCK(searchdir);
		    	searchdir_locked = false;
		} else if ((cnp->cn_flags & LOCKLEAF) != 0 &&
		    (searchdir != foundobj ||
		    (cnp->cn_flags & LOCKPARENT) == 0)) {
			const int lktype = (cnp->cn_flags & LOCKSHARED) != 0 ?
			    LK_SHARED : LK_EXCLUSIVE;
			vn_lock(foundobj, lktype | LK_RETRY);
		}
	}

	/*
	 * Done.
	 */

	/*
	 * If LOCKPARENT is not set, the parent directory isn't returned.
	 */
	if ((cnp->cn_flags & LOCKPARENT) == 0 && searchdir != NULL) {
		vrele(searchdir);
		searchdir = NULL;
	}

	ndp->ni_dvp = searchdir;
	ndp->ni_vp = foundobj;
	return 0;
}

/*
 * Do namei; wrapper layer that handles TRYEMULROOT.
 */
static int
namei_tryemulroot(struct namei_state *state,
	 int neverfollow, int inhibitmagic, int isnfsd)
{
	int error;

	struct nameidata *ndp = state->ndp;
	struct componentname *cnp = state->cnp;
	const char *savepath = NULL;

	KASSERT(cnp == &ndp->ni_cnd);

	if (cnp->cn_flags & TRYEMULROOT) {
		savepath = pathbuf_stringcopy_get(ndp->ni_pathbuf);
	}

    emul_retry:
	state->attempt_retry = 0;

	error = namei_oneroot(state, neverfollow, inhibitmagic, isnfsd);
	if (error) {
		/*
		 * Once namei has started up, the existence of ni_erootdir
		 * tells us whether we're working from an emulation root.
		 * The TRYEMULROOT flag isn't necessarily authoritative.
		 */
		if (ndp->ni_erootdir != NULL && state->attempt_retry) {
			/* Retry the whole thing using the normal root */
			cnp->cn_flags &= ~TRYEMULROOT;
			state->attempt_retry = 0;

			/* kinda gross */
			strcpy(ndp->ni_pathbuf->pb_path, savepath);
			pathbuf_stringcopy_put(ndp->ni_pathbuf, savepath);
			savepath = NULL;

			goto emul_retry;
		}
	}
	if (savepath != NULL) {
		pathbuf_stringcopy_put(ndp->ni_pathbuf, savepath);
	}
	return error;
}

/*
 * External interface.
 */
int
namei(struct nameidata *ndp)
{
	struct namei_state state;
	int error;

	namei_init(&state, ndp);
	error = namei_tryemulroot(&state,
				  0/*!neverfollow*/, 0/*!inhibitmagic*/,
				  0/*isnfsd*/);
	namei_cleanup(&state);

	if (error) {
		/* make sure no stray refs leak out */
		KASSERT(ndp->ni_dvp == NULL);
		KASSERT(ndp->ni_vp == NULL);
	}

	return error;
}

////////////////////////////////////////////////////////////

/*
 * External interface used by nfsd. This is basically different from
 * namei only in that it has the ability to pass in the "current
 * directory", and uses an extra flag "neverfollow" for which there's
 * no physical flag defined in namei.h. (There used to be a cut&paste
 * copy of about half of namei in nfsd to allow these minor
 * adjustments to exist.)
 *
 * XXX: the namei interface should be adjusted so nfsd can just use
 * ordinary namei().
 */
int
lookup_for_nfsd(struct nameidata *ndp, struct vnode *forcecwd, int neverfollow)
{
	struct namei_state state;
	int error;

	KASSERT(ndp->ni_atdir == NULL);
	ndp->ni_atdir = forcecwd;

	namei_init(&state, ndp);
	error = namei_tryemulroot(&state,
				  neverfollow, 1/*inhibitmagic*/, 1/*isnfsd*/);
	namei_cleanup(&state);

	if (error) {
		/* make sure no stray refs leak out */
		KASSERT(ndp->ni_dvp == NULL);
		KASSERT(ndp->ni_vp == NULL);
	}

	return error;
}

/*
 * A second external interface used by nfsd. This turns out to be a
 * single lookup used by the WebNFS code (ha!) to get "index.html" or
 * equivalent when asked for a directory. It should eventually evolve
 * into some kind of namei_once() call; for the time being it's kind
 * of a mess. XXX.
 *
 * dholland 20110109: I don't think it works, and I don't think it
 * worked before I started hacking and slashing either, and I doubt
 * anyone will ever notice.
 */

/*
 * Internals. This calls lookup_once() after setting up the assorted
 * pieces of state the way they ought to be.
 */
static int
do_lookup_for_nfsd_index(struct namei_state *state)
{
	int error;

	struct componentname *cnp = state->cnp;
	struct nameidata *ndp = state->ndp;
	struct vnode *startdir;
	struct vnode *foundobj;
	bool startdir_locked;
	const char *cp;			/* pointer into pathname argument */

	KASSERT(cnp == &ndp->ni_cnd);

	startdir = state->ndp->ni_atdir;

	cnp->cn_nameptr = ndp->ni_pnbuf;
	state->docache = 1;
	state->rdonly = cnp->cn_flags & RDONLY;
	ndp->ni_dvp = NULL;

	error = VOP_PARSEPATH(startdir, cnp->cn_nameptr, &cnp->cn_namelen);
	if (error) {
		return error;
	}

	cp = cnp->cn_nameptr + cnp->cn_namelen;
	KASSERT(cnp->cn_namelen <= KERNEL_NAME_MAX);
	ndp->ni_pathlen -= cnp->cn_namelen;
	ndp->ni_next = cp;
	state->slashes = 0;
	cnp->cn_flags &= ~REQUIREDIR;
	cnp->cn_flags |= MAKEENTRY|ISLASTCN;

	if (cnp->cn_namelen == 2 &&
	    cnp->cn_nameptr[1] == '.' && cnp->cn_nameptr[0] == '.')
		cnp->cn_flags |= ISDOTDOT;
	else
		cnp->cn_flags &= ~ISDOTDOT;

	/*
	 * Because lookup_once can change the startdir, we need our
	 * own reference to it to avoid consuming the caller's.
	 */
	vref(startdir);
	error = lookup_once(state, startdir, &startdir, &foundobj,
	    &startdir_locked);

	KASSERT((cnp->cn_flags & LOCKPARENT) == 0);
	if (startdir_locked) {
		VOP_UNLOCK(startdir);
		startdir_locked = false;
	}

	/*
	 * If the vnode we found is mounted on, then cross the mount and get
	 * the root vnode in foundobj.  If this encounters an error, it will
	 * dispose of foundobj, but searchdir is untouched.
	 */
	if (error == 0 && foundobj != NULL &&
	    foundobj->v_type == VDIR &&
	    foundobj->v_mountedhere != NULL &&
	    (cnp->cn_flags & NOCROSSMOUNT) == 0) {
		error = lookup_crossmount(state, &startdir, &foundobj,
		    &startdir_locked);
	}

	/* Now toss startdir and see if we have an error. */
	if (startdir != NULL)
		vrele(startdir);
	if (error)
		foundobj = NULL;
	else if (foundobj != NULL && (cnp->cn_flags & LOCKLEAF) != 0)
		vn_lock(foundobj, LK_EXCLUSIVE | LK_RETRY);

	ndp->ni_vp = foundobj;
	return (error);
}

/*
 * External interface. The partitioning between this function and the
 * above isn't very clear - the above function exists mostly so code
 * that uses "state->" can be shuffled around without having to change
 * it to "state.".
 */
int
lookup_for_nfsd_index(struct nameidata *ndp, struct vnode *startdir)
{
	struct namei_state state;
	int error;

	KASSERT(ndp->ni_atdir == NULL);
	ndp->ni_atdir = startdir;

	/*
	 * Note: the name sent in here (is not|should not be) allowed
	 * to contain a slash.
	 */
	if (strlen(ndp->ni_pathbuf->pb_path) > KERNEL_NAME_MAX) {
		return ENAMETOOLONG;
	}
	if (strchr(ndp->ni_pathbuf->pb_path, '/')) {
		return EINVAL;
	}

	ndp->ni_pathlen = strlen(ndp->ni_pathbuf->pb_path) + 1;
	ndp->ni_pnbuf = NULL;
	ndp->ni_cnd.cn_nameptr = NULL;

	namei_init(&state, ndp);
	error = do_lookup_for_nfsd_index(&state);
	namei_cleanup(&state);

	return error;
}

////////////////////////////////////////////////////////////

/*
 * Reacquire a path name component.
 * dvp is locked on entry and exit.
 * *vpp is locked on exit unless it's NULL.
 */
int
relookup(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, int dummy)
{
	int rdonly;			/* lookup read-only flag bit */
	int error = 0;
#ifdef DEBUG
	size_t newlen;			/* DEBUG: check name len */
	const char *cp;			/* DEBUG: check name ptr */
#endif /* DEBUG */

	(void)dummy;

	/*
	 * Setup: break out flag bits into variables.
	 */
	rdonly = cnp->cn_flags & RDONLY;

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
#if 0
	cp = NULL;
	newhash = namei_hash(cnp->cn_nameptr, &cp);
	if ((uint32_t)newhash != (uint32_t)cnp->cn_hash)
		panic("relookup: bad hash");
#endif
	error = VOP_PARSEPATH(dvp, cnp->cn_nameptr, &newlen);
	if (error) {
		panic("relookup: parsepath failed with error %d", error);
	}
	if (cnp->cn_namelen != newlen)
		panic("relookup: bad len");
	cp = cnp->cn_nameptr + cnp->cn_namelen;
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
	*vpp = NULL;
	error = VOP_LOOKUP(dvp, vpp, cnp);
	if ((error) != 0) {
		KASSERTMSG((*vpp == NULL),
		    "leaf `%s' should be empty but is %p",
		    cnp->cn_nameptr, *vpp);
		if (error != EJUSTRETURN)
			goto bad;
	}

	/*
	 * Check for symbolic link
	 */
	KASSERTMSG((*vpp == NULL || (*vpp)->v_type != VLNK ||
		(cnp->cn_flags & FOLLOW) == 0),
	    "relookup: symlink found");

	/*
	 * Check for read-only lookups.
	 */
	if (rdonly && cnp->cn_nameiop != LOOKUP) {
		error = EROFS;
		if (*vpp) {
			vrele(*vpp);
		}
		goto bad;
	}
	/*
	 * Lock result.
	 */
	if (*vpp && *vpp != dvp) {
		error = vn_lock(*vpp, LK_EXCLUSIVE);
		if (error != 0) {
			vrele(*vpp);
			goto bad;
		}
	}
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
	return nameiat_simple_kernel(NULL, path, sflags, vp_ret);
}

int
nameiat_simple_kernel(struct vnode *dvp, const char *path,
	namei_simple_flags_t sflags, struct vnode **vp_ret)
{
	struct nameidata nd;
	struct pathbuf *pb;
	int err;

	pb = pathbuf_create(path);
	if (pb == NULL) {
		return ENOMEM;
	}

	NDINIT(&nd,
		LOOKUP,
		namei_simple_convert_flags(sflags),
		pb);

	if (dvp != NULL)
		NDAT(&nd, dvp);

	err = namei(&nd);
	if (err != 0) {
		pathbuf_destroy(pb);
		return err;
	}
	*vp_ret = nd.ni_vp;
	pathbuf_destroy(pb);
	return 0;
}

int
namei_simple_user(const char *path, namei_simple_flags_t sflags,
	struct vnode **vp_ret)
{
	return nameiat_simple_user(NULL, path, sflags, vp_ret);
}

int
nameiat_simple_user(struct vnode *dvp, const char *path,
	namei_simple_flags_t sflags, struct vnode **vp_ret)
{
	struct pathbuf *pb;
	struct nameidata nd;
	int err;

	err = pathbuf_copyin(path, &pb);
	if (err) {
		return err;
	}

	NDINIT(&nd,
		LOOKUP,
		namei_simple_convert_flags(sflags),
		pb);

	if (dvp != NULL)
		NDAT(&nd, dvp);

	err = namei(&nd);
	if (err != 0) {
		pathbuf_destroy(pb);
		return err;
	}
	*vp_ret = nd.ni_vp;
	pathbuf_destroy(pb);
	return 0;
}
