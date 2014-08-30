/*  $NetBSD: ops.c,v 1.66.2.2 2014/08/30 19:30:28 martin Exp $ */

/*-
 *  Copyright (c) 2010-2011 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <err.h>
#include <sysexits.h>
#include <syslog.h>
#include <puffs.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/extattr.h>
#include <sys/time.h>
#include <machine/vmparam.h>

#include "perfuse_priv.h"
#include "fuse.h"

extern int perfuse_diagflags;

#if 0
static void print_node(const char *, puffs_cookie_t);
#endif
#ifdef PUFFS_KFLAG_CACHE_FS_TTL
static void perfuse_newinfo_setttl(struct puffs_newinfo *, 
    struct puffs_node *, struct fuse_entry_out *, struct fuse_attr_out *);
#endif /* PUFFS_KFLAG_CACHE_FS_TTL */
static int xchg_msg(struct puffs_usermount *, puffs_cookie_t, 
    perfuse_msg_t *, size_t, enum perfuse_xchg_pb_reply); 
static int mode_access(puffs_cookie_t, const struct puffs_cred *, mode_t);
static int sticky_access(puffs_cookie_t, struct puffs_node *, 
    const struct puffs_cred *);
static void fuse_attr_to_vap(struct perfuse_state *,
    struct vattr *, struct fuse_attr *);
static int node_lookup_common(struct puffs_usermount *, puffs_cookie_t, 
    struct puffs_newinfo *, const char *, const struct puffs_cred *, 
    struct puffs_node **);
static int node_mk_common(struct puffs_usermount *, puffs_cookie_t,
    struct puffs_newinfo *, const struct puffs_cn *pcn, perfuse_msg_t *);
static uint64_t readdir_last_cookie(struct fuse_dirent *, size_t); 
static ssize_t fuse_to_dirent(struct puffs_usermount *, puffs_cookie_t,
    struct fuse_dirent *, size_t);
static void readdir_buffered(puffs_cookie_t, struct dirent *, off_t *, 
    size_t *);
static void node_ref(puffs_cookie_t);
static void node_rele(puffs_cookie_t);
static void requeue_request(struct puffs_usermount *, 
    puffs_cookie_t opc, enum perfuse_qtype);
static int dequeue_requests(puffs_cookie_t opc, enum perfuse_qtype, int);
#define DEQUEUE_ALL 0

/* 
 *  From <sys/vnode>, inside #ifdef _KERNEL section
 */
#define IO_SYNC		(0x40|IO_DSYNC) 
#define IO_DSYNC	0x00200  
#define IO_DIRECT	0x02000

/* 
 *  From <fcntl>, inside #ifdef _KERNEL section
 */
#define F_WAIT		0x010
#define F_FLOCK		0x020
#define OFLAGS(fflags)  ((fflags) - 1)

/* 
 * Borrowed from src/sys/kern/vfs_subr.c and src/sys/sys/vnode.h 
 */
const enum vtype iftovt_tab[16] = { 
	VNON, VFIFO, VCHR, VNON, VDIR, VNON, VBLK, VNON,
        VREG, VNON, VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};
const int vttoif_tab[9] = { 
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK,
        S_IFSOCK, S_IFIFO, S_IFMT,
}; 

#define IFTOVT(mode) (iftovt_tab[((mode) & S_IFMT) >> 12])
#define VTTOIF(indx) (vttoif_tab[(int)(indx)])

#if 0
static void 
print_node(const char *func, puffs_cookie_t opc)
{
	struct puffs_node *pn;
	struct perfuse_node_data *pnd;
	struct vattr *vap;

	pn = (struct puffs_node *)opc;
	pnd = PERFUSE_NODE_DATA(opc);
	vap = &pn->pn_va;

	printf("%s: \"%s\", opc = %p, nodeid = 0x%"PRIx64" ino = %"PRIu64"\n",
	       func, pnd->pnd_name, opc, pnd->pnd_nodeid, vap->va_fileid);

	return;
}
#endif /* PERFUSE_DEBUG */
	
int
perfuse_node_close_common(struct puffs_usermount *pu, puffs_cookie_t opc,
	int mode)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	int op;
	uint64_t fh;
	struct fuse_release_in *fri;
	struct perfuse_node_data *pnd;
	struct puffs_node *pn;
	int error;

	ps = puffs_getspecific(pu);
	pn = (struct puffs_node *)opc;
	pnd = PERFUSE_NODE_DATA(pn);

	if (puffs_pn_getvap(pn)->va_type == VDIR) {
		op = FUSE_RELEASEDIR;
		mode = FREAD;
	} else {
		op = FUSE_RELEASE;
	}

	/*
	 * Destroy the filehandle before sending the 
	 * request to the FUSE filesystem, otherwise 
	 * we may get a second close() while we wait
	 * for the reply, and we would end up closing
	 * the same fh twice instead of closng both.
	 */
	fh = perfuse_get_fh(opc, mode);
	perfuse_destroy_fh(pn, fh);

	/*
	 * release_flags may be set to FUSE_RELEASE_FLUSH
	 * to flush locks. lock_owner must be set in that case
	 *
	 * ps_new_msg() is called with NULL creds, which will
	 * be interpreted as FUSE superuser. We come here from the 
	 * inactive method, which provides no creds, but obviously 	
	 * runs with kernel privilege.
	 */
	pm = ps->ps_new_msg(pu, opc, op, sizeof(*fri), NULL);
	fri = GET_INPAYLOAD(ps, pm, fuse_release_in);
	fri->fh = fh;
	fri->flags = 0;
	fri->release_flags = 0;
	fri->lock_owner = pnd->pnd_lock_owner;
	fri->flags = (fri->lock_owner != 0) ? FUSE_RELEASE_FLUSH : 0;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FH)
		DPRINTF("%s: opc = %p, nodeid = 0x%"PRIx64", fh = 0x%"PRIx64"\n",
			 __func__, (void *)opc, pnd->pnd_nodeid, fri->fh);
#endif

	if ((error = xchg_msg(pu, opc, pm,
			      NO_PAYLOAD_REPLY_LEN, wait_reply)) != 0)
		DERRX(EX_SOFTWARE, "%s: freed fh = 0x%"PRIx64" but filesystem "
		      "returned error = %d", __func__, fh, error);

	ps->ps_destroy_msg(pm);

	return 0;
}

static int
xchg_msg(struct puffs_usermount *pu, puffs_cookie_t opc, perfuse_msg_t *pm,
	size_t len, enum perfuse_xchg_pb_reply wait)
{
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd;
	struct perfuse_trace *pt = NULL;
	int error;

	ps = puffs_getspecific(pu);
	pnd = NULL;
	if ((struct puffs_node *)opc != NULL)
		pnd = PERFUSE_NODE_DATA(opc);

#ifdef PERFUSE_DEBUG
	if ((perfuse_diagflags & PDF_FILENAME) && (opc != 0))
		DPRINTF("file = \"%s\", ino = %"PRIu64" flags = 0x%x\n", 
			perfuse_node_path(ps, opc), 
			((struct puffs_node *)opc)->pn_va.va_fileid,
			PERFUSE_NODE_DATA(opc)->pnd_flags);
#endif
	ps->ps_xchgcount++;
	if (pnd)
		pnd->pnd_inxchg++;

	/*
	 * Record FUSE call start if requested
	 */
	if (perfuse_diagflags & PDF_TRACE)
		pt = perfuse_trace_begin(ps, opc, pm);

	/*
	 * Do actual FUSE exchange
	 */
	if ((error = ps->ps_xchg_msg(pu, pm, len, wait)) != 0)
		ps->ps_destroy_msg(pm);

	/*
	 * Record FUSE call end if requested
	 */
	if (pt != NULL)
		perfuse_trace_end(ps, pt, error);

	ps->ps_xchgcount--;
	if (pnd) {
		pnd->pnd_inxchg--;
		(void)dequeue_requests(opc, PCQ_AFTERXCHG, DEQUEUE_ALL);
	}

	return error;
}

static int
mode_access(puffs_cookie_t opc, const struct puffs_cred *pcr, mode_t mode)
{
	struct puffs_node *pn;
	struct vattr *va;

	/*
	 * pcr is NULL for self open through fsync or readdir.
	 * In both case, access control is useless, as it was
	 * done before, at open time.
	 */
	if (pcr == NULL)
		return 0;

	pn = (struct puffs_node *)opc;
	va = puffs_pn_getvap(pn);
	return puffs_access(va->va_type, va->va_mode, 
			    va->va_uid, va->va_gid,
			    mode, pcr);
}

static int 
sticky_access(puffs_cookie_t opc, struct puffs_node *targ,
	      const struct puffs_cred *pcr)
{
	uid_t uid;
	int sticky, owner;

	/*
	 * This covers the case where the kernel requests a DELETE
	 * or RENAME on its own, and where puffs_cred_getuid would 
	 * return -1. While such a situation should not happen, 
	 * we allow it here. 
	 *
	 * This also allows root to tamper with other users' files
	 * that have the sticky bit.
	 */
	if (puffs_cred_isjuggernaut(pcr))
		return 0;

	if (puffs_cred_getuid(pcr, &uid) != 0)
		DERRX(EX_SOFTWARE, "puffs_cred_getuid fails in %s", __func__);

	sticky = puffs_pn_getvap(opc)->va_mode & S_ISTXT;
	owner = puffs_pn_getvap(targ)->va_uid == uid;

	if (sticky && !owner)
		return EACCES;

	return 0;
}


static void
fuse_attr_to_vap(struct perfuse_state *ps, struct vattr *vap,
	struct fuse_attr *fa)
{
	vap->va_type = IFTOVT(fa->mode);
	vap->va_mode = fa->mode & ALLPERMS;
	vap->va_nlink = fa->nlink;
	vap->va_uid = fa->uid;
	vap->va_gid = fa->gid;
	vap->va_fsid = (long)ps->ps_fsid;
	vap->va_fileid = fa->ino;
	vap->va_size = fa->size;
	vap->va_blocksize = fa->blksize;
	vap->va_atime.tv_sec = (time_t)fa->atime;
	vap->va_atime.tv_nsec = (long) fa->atimensec;
	vap->va_mtime.tv_sec = (time_t)fa->mtime;
	vap->va_mtime.tv_nsec = (long)fa->mtimensec;
	vap->va_ctime.tv_sec = (time_t)fa->ctime;
	vap->va_ctime.tv_nsec = (long)fa->ctimensec;
	vap->va_birthtime.tv_sec = 0;
	vap->va_birthtime.tv_nsec = 0;
	vap->va_gen = 0; 
	vap->va_flags = 0;
	vap->va_rdev = fa->rdev;
	vap->va_bytes = fa->size;
	vap->va_filerev = (u_quad_t)PUFFS_VNOVAL;
	vap->va_vaflags = 0;

	if (vap->va_blocksize == 0)
		vap->va_blocksize = DEV_BSIZE;

	if (vap->va_size == (size_t)PUFFS_VNOVAL) /* XXX */
		vap->va_size = 0;

	return;
}

#ifdef PUFFS_KFLAG_CACHE_FS_TTL
static void 
perfuse_newinfo_setttl(struct puffs_newinfo *pni,
    struct puffs_node *pn, struct fuse_entry_out *feo,
    struct fuse_attr_out *fao)
{
#ifdef PERFUSE_DEBUG
	if ((feo == NULL) && (fao == NULL))
		DERRX(EX_SOFTWARE, "%s: feo and fao NULL", __func__);

	if ((feo != NULL) && (fao != NULL))
		DERRX(EX_SOFTWARE, "%s: feo and fao != NULL", __func__);
#endif /* PERFUSE_DEBUG */

	if (fao != NULL) {
		struct timespec va_ttl;

		va_ttl.tv_sec = fao->attr_valid;
		va_ttl.tv_nsec = fao->attr_valid_nsec;

		puffs_newinfo_setvattl(pni, &va_ttl);
	}

	if (feo != NULL) {
		struct timespec va_ttl;
		struct timespec cn_ttl;
		struct timespec now;
		struct perfuse_node_data *pnd = PERFUSE_NODE_DATA(pn);

		va_ttl.tv_sec = feo->attr_valid;
		va_ttl.tv_nsec = feo->attr_valid_nsec;
		cn_ttl.tv_sec = feo->entry_valid;
		cn_ttl.tv_nsec = feo->entry_valid_nsec;

		puffs_newinfo_setvattl(pni, &va_ttl);
		puffs_newinfo_setcnttl(pni, &cn_ttl);
        
		if (clock_gettime(CLOCK_REALTIME, &now) != 0)
			DERR(EX_OSERR, "clock_gettime failed"); 

                timespecadd(&now, &cn_ttl, &pnd->pnd_cn_expire);
	}

	return;	
}
#endif /* PUFFS_KFLAG_CACHE_FS_TTL */

static int
node_lookup_common(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct puffs_newinfo *pni, const char *path, 
	const struct puffs_cred *pcr, struct puffs_node **pnp)
{
	struct perfuse_state *ps;
	struct perfuse_node_data *oldpnd;
	perfuse_msg_t *pm;
	struct fuse_entry_out *feo;
	struct puffs_node *pn;
	size_t len;
	int error;

	/*
	 * Prevent further lookups if the parent was removed
	 */
	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_REMOVED)
		return ESTALE;

	if (pnp == NULL)
		DERRX(EX_SOFTWARE, "pnp must be != NULL");

	ps = puffs_getspecific(pu);

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FILENAME)
		DPRINTF("%s: opc = %p, file = \"%s\" looking up \"%s\"\n",
			__func__, (void *)opc,
			perfuse_node_path(ps, opc), path);
	
	if (strcmp(path, ".") == 0)
		DERRX(EX_SOFTWARE, "unexpected dot-lookup");  

	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_RECLAIMED) 
		DERRX(EX_SOFTWARE,
		      "looking up reclaimed node opc = %p, name = \"%s\"",
		      opc, path);

	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_INVALID) 
		DERRX(EX_SOFTWARE,
		      "looking up freed node opc = %p, name = \"%s\"",
		      opc, path);
#endif /* PERFUSE_DEBUG */

	len = strlen(path) + 1;
	pm = ps->ps_new_msg(pu, opc, FUSE_LOOKUP, len, pcr);
	(void)strlcpy(_GET_INPAYLOAD(ps, pm, char *), path, len);

	if ((error = xchg_msg(pu, opc, pm, sizeof(*feo), wait_reply)) != 0)
		return error;

	feo = GET_OUTPAYLOAD(ps, pm, fuse_entry_out);

	/* 
	 * Starting with ABI 7.4, inode number 0 means ENOENT, 
	 * with entry_valid / entry_valid_nsec giving negative
	 * cache timeout (which we do not implement yet).
	 */
	if (feo->attr.ino == 0) {
		ps->ps_destroy_msg(pm);
		return ENOENT;
	}

	/*
	 * Check for a known node, not reclaimed, with another name.
	 * It may have been moved, or we can lookup ../
	 */
	if (((oldpnd = perfuse_node_bynodeid(ps, feo->nodeid)) != NULL) &&
	    !(oldpnd->pnd_flags & PND_RECLAIMED)) {
		/*
		 * Save the new node name if not ..
		 */
		if (strncmp(path, "..", len) != 0)
			(void)strlcpy(oldpnd->pnd_name, 
				      path, MAXPATHLEN);
		pn = oldpnd->pnd_pn;

	} else {
		pn = perfuse_new_pn(pu, path, opc);
		PERFUSE_NODE_DATA(pn)->pnd_nodeid = feo->nodeid;
		perfuse_node_cache(ps, pn);
	}

#ifdef PERFUSE_DEBUG
	if (PERFUSE_NODE_DATA(pn)->pnd_flags & PND_RECLAIMED) 
		DERRX(EX_SOFTWARE,
		      "reclaimed in lookup opc = %p, name = \"%s\", ck = %p",
		      opc, path, pn);

	if (PERFUSE_NODE_DATA(pn)->pnd_flags & PND_INVALID) 
		DERRX(EX_SOFTWARE,
		      "freed in lookup opc = %p, name = \"%s\", ck = %p",
		      opc, path, pn);
#endif /* PERFUSE_DEBUG */

	fuse_attr_to_vap(ps, &pn->pn_va, &feo->attr);
	pn->pn_va.va_gen = (u_long)(feo->generation);
	PERFUSE_NODE_DATA(pn)->pnd_fuse_nlookup++;

	*pnp = pn;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FILENAME)
		DPRINTF("%s: opc = %p, looked up opc = %p, "
			"nodeid = 0x%"PRIx64" file = \"%s\"\n", __func__, 
			(void *)opc, pn, feo->nodeid, path);
#endif

	if (pni != NULL) {
#ifdef PUFFS_KFLAG_CACHE_FS_TTL
		puffs_newinfo_setva(pni, &pn->pn_va);
		perfuse_newinfo_setttl(pni, pn, feo, NULL);
#endif /* PUFFS_KFLAG_CACHE_FS_TTL */
		puffs_newinfo_setcookie(pni, pn);
		puffs_newinfo_setvtype(pni, pn->pn_va.va_type); 
		puffs_newinfo_setsize(pni, (voff_t)pn->pn_va.va_size);
		puffs_newinfo_setrdev(pni, pn->pn_va.va_rdev);
	}

	if (PERFUSE_NODE_DATA(pn)->pnd_flags & PND_NODELEAK) {
		PERFUSE_NODE_DATA(pn)->pnd_flags &= ~PND_NODELEAK;
		ps->ps_nodeleakcount--;
	}

	ps->ps_destroy_msg(pm);

	return 0;
}


/*
 * Common code for methods that create objects:
 * perfuse_node_mkdir
 * perfuse_node_mknod
 * perfuse_node_symlink
 */
static int
node_mk_common(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn,
	perfuse_msg_t *pm)
{
	struct perfuse_state *ps;
	struct puffs_node *pn;
	struct fuse_entry_out *feo;
	int error;

	ps =  puffs_getspecific(pu);

	if ((error = xchg_msg(pu, opc, pm, sizeof(*feo), wait_reply)) != 0)
		return error;

	feo = GET_OUTPAYLOAD(ps, pm, fuse_entry_out);
	if (feo->nodeid == PERFUSE_UNKNOWN_NODEID)
		DERRX(EX_SOFTWARE, "%s: no nodeid", __func__);

	pn = perfuse_new_pn(pu, pcn->pcn_name, opc);
	PERFUSE_NODE_DATA(pn)->pnd_nodeid = feo->nodeid;
	PERFUSE_NODE_DATA(pn)->pnd_puffs_nlookup++;
	perfuse_node_cache(ps, pn);

	fuse_attr_to_vap(ps, &pn->pn_va, &feo->attr);
	pn->pn_va.va_gen = (u_long)(feo->generation);

	puffs_newinfo_setcookie(pni, pn);
#ifdef PUFFS_KFLAG_CACHE_FS_TTL
	puffs_newinfo_setva(pni, &pn->pn_va);
	perfuse_newinfo_setttl(pni, pn, feo, NULL);
#endif /* PUFFS_KFLAG_CACHE_FS_TTL */


#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FILENAME)
		DPRINTF("%s: opc = %p, file = \"%s\", flags = 0x%x "
			"nodeid = 0x%"PRIx64"\n",
			__func__, (void *)pn, pcn->pcn_name,
			PERFUSE_NODE_DATA(pn)->pnd_flags, feo->nodeid);
#endif
	ps->ps_destroy_msg(pm);
	
	/* Parents is now dirty */
	PERFUSE_NODE_DATA(opc)->pnd_flags |= PND_DIRTY;

	return 0; 
}

static uint64_t
readdir_last_cookie(struct fuse_dirent *fd, size_t fd_len)
{
	size_t len;
	size_t seen = 0;
	char *ndp;

	do {
		len = FUSE_DIRENT_ALIGN(sizeof(*fd) + fd->namelen);
		seen += len;

		if (seen >= fd_len)
			break;

		ndp = (char *)(void *)fd + (size_t)len;
		fd = (struct fuse_dirent *)(void *)ndp;
	} while (1 /* CONSTCOND */);

	return fd->off;
}

static ssize_t
fuse_to_dirent(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct fuse_dirent *fd, size_t fd_len)
{
	struct dirent *dents;
	size_t dents_len;
	ssize_t written;
	uint64_t fd_offset;
	struct fuse_dirent *fd_base;
	size_t len;

	fd_base = fd;
	fd_offset = 0;
	written = 0;
	dents = PERFUSE_NODE_DATA(opc)->pnd_dirent;
	dents_len = (size_t)PERFUSE_NODE_DATA(opc)->pnd_dirent_len;

	do {
		char *ndp;
		size_t reclen;

		reclen = _DIRENT_RECLEN(dents, fd->namelen);

		/*
		 * Check we do not overflow the output buffer
		 * struct fuse_dirent is bigger than struct dirent,
		 * so we should always use fd_len and never reallocate
		 * later. 
		 * If we have to reallocate,try to double the buffer
		 * each time so that we do not have to do it too often.
		 */
		if (written + reclen > dents_len) {
			if (dents_len == 0)
				dents_len = fd_len;
			else
				dents_len = 
				   MAX(2 * dents_len, written + reclen);
	
			dents = PERFUSE_NODE_DATA(opc)->pnd_dirent;
			if ((dents = realloc(dents, dents_len)) == NULL)
				DERR(EX_OSERR, "%s: malloc failed", __func__);

			PERFUSE_NODE_DATA(opc)->pnd_dirent = dents;
			PERFUSE_NODE_DATA(opc)->pnd_dirent_len = dents_len;

			/*
			 * (void *) for delint
			 */
			ndp = (char *)(void *)dents + written;
			dents = (struct dirent *)(void *)ndp;
		}
		
		/*
		 * Filesystem was mounted without -o use_ino
		 * Perform a lookup to find it.
		 */
		if (fd->ino == PERFUSE_UNKNOWN_INO) {
			struct puffs_node *pn;
			struct perfuse_node_data *pnd = PERFUSE_NODE_DATA(opc);

			/* 
			 * Avoid breaking out of fs 
			 * by lookup to .. on root
			 */
			if ((strcmp(fd->name, "..") == 0) && 
			    (pnd->pnd_nodeid == FUSE_ROOT_ID)) {
				fd->ino = FUSE_ROOT_ID;
			} else {
				if (node_lookup_common(pu, opc, NULL, fd->name,
						       NULL, &pn) != 0) {
					DWARNX("node_lookup_common failed");
				} else {
					fd->ino = pn->pn_va.va_fileid;
					(void)perfuse_node_reclaim(pu, pn);
				}
			}
		}

		dents->d_fileno = fd->ino;
		dents->d_reclen = (unsigned short)reclen;
		dents->d_namlen = fd->namelen;
		dents->d_type = fd->type;
		strlcpy(dents->d_name, fd->name, fd->namelen + 1);

#ifdef PERFUSE_DEBUG
		if (perfuse_diagflags & PDF_READDIR)
			DPRINTF("%s: translated \"%s\" ino = %"PRIu64"\n",
				__func__, dents->d_name, dents->d_fileno);
#endif

		dents = _DIRENT_NEXT(dents);
		written += reclen;

		/*
		 * Move to the next record.
		 * fd->off is not the offset, it is an opaque cookie
		 * given by the filesystem to keep state across multiple
		 * readdir() operation.
		 * Use record alignement instead.
		 */
		len = FUSE_DIRENT_ALIGN(sizeof(*fd) + fd->namelen);
#ifdef PERFUSE_DEBUG
		if (perfuse_diagflags & PDF_READDIR)
			DPRINTF("%s: record at %"PRId64"/0x%"PRIx64" "
				"length = %zd/0x%zx. "
				"next record at %"PRId64"/0x%"PRIx64" "
				"max %zd/0x%zx\n",
				__func__, fd_offset, fd_offset, len, len,
				fd_offset + len, fd_offset + len, 
				fd_len, fd_len);
#endif
		fd_offset += len;

		/*
		 * Check if next record is still within the packet
		 * If it is not, we reached the end of the buffer.
		 */
		if (fd_offset >= fd_len)
			break;

		/*
		 * (void *) for delint
		 */
		ndp = (char *)(void *)fd_base + (size_t)fd_offset;
		fd = (struct fuse_dirent *)(void *)ndp;

	} while (1 /* CONSTCOND */);

	/*
	 * Adjust the dirent output length 
	 */
	if (written != -1)
		PERFUSE_NODE_DATA(opc)->pnd_dirent_len = written;
	
	return written;
}

static void
readdir_buffered(puffs_cookie_t opc, struct dirent *dent, off_t *readoff,
	size_t *reslen)
{
	struct dirent *fromdent;
	struct perfuse_node_data *pnd;
	char *ndp;
	
	pnd = PERFUSE_NODE_DATA(opc);

	while (*readoff < pnd->pnd_dirent_len) {
		/*
		 * (void *) for delint
		 */
		ndp = (char *)(void *)pnd->pnd_dirent + (size_t)*readoff;
		fromdent = (struct dirent *)(void *)ndp;

		if (*reslen < _DIRENT_SIZE(fromdent))
			break;

		memcpy(dent, fromdent, _DIRENT_SIZE(fromdent));
		*readoff += _DIRENT_SIZE(fromdent);
		*reslen -= _DIRENT_SIZE(fromdent);
	
		dent = _DIRENT_NEXT(dent);
	}

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_READDIR)
		DPRINTF("%s: readoff = %"PRId64",  "
			"pnd->pnd_dirent_len = %"PRId64"\n",
			__func__, *readoff, pnd->pnd_dirent_len);
#endif
	if (*readoff >=  pnd->pnd_dirent_len) {
		free(pnd->pnd_dirent);
		pnd->pnd_dirent = NULL;
		pnd->pnd_dirent_len = 0;
	}

	return;
}


static void
node_ref(puffs_cookie_t opc)
{
	struct perfuse_node_data *pnd = PERFUSE_NODE_DATA(opc);

#ifdef PERFUSE_DEBUG
	if (pnd->pnd_flags & PND_INVALID)
		DERRX(EX_SOFTWARE, "Use of freed node opc = %p", opc);
#endif /* PERFUSE_DEBUG */

	pnd->pnd_ref++;
	return;
}

static void
node_rele(puffs_cookie_t opc)
{
	struct perfuse_node_data *pnd = PERFUSE_NODE_DATA(opc);

#ifdef PERFUSE_DEBUG
	if (pnd->pnd_flags & PND_INVALID)
		DERRX(EX_SOFTWARE, "Use of freed node opc = %p", opc);
#endif /* PERFUSE_DEBUG */

	pnd->pnd_ref--;

	if (pnd->pnd_ref == 0)
		(void)dequeue_requests(opc, PCQ_REF, DEQUEUE_ALL);
		
	return;
}

static void
requeue_request(struct puffs_usermount *pu, puffs_cookie_t opc,
	enum perfuse_qtype type)
{
	struct perfuse_cc_queue pcq;
	struct perfuse_node_data *pnd;
#ifdef PERFUSE_DEBUG
	struct perfuse_state *ps;

	ps = perfuse_getspecific(pu);
#endif

	pnd = PERFUSE_NODE_DATA(opc);
	pcq.pcq_type = type;
	pcq.pcq_cc = puffs_cc_getcc(pu);
	TAILQ_INSERT_TAIL(&pnd->pnd_pcq, &pcq, pcq_next);

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_REQUEUE)
		DPRINTF("%s: REQUEUE opc = %p, pcc = %p (%s)\n", 
		        __func__, (void *)opc, pcq.pcq_cc,
			perfuse_qtypestr[type]);
#endif

	puffs_cc_yield(pcq.pcq_cc);
	TAILQ_REMOVE(&pnd->pnd_pcq, &pcq, pcq_next);

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_REQUEUE)
		DPRINTF("%s: RESUME opc = %p, pcc = %p (%s)\n",
		        __func__, (void *)opc, pcq.pcq_cc,
			perfuse_qtypestr[type]);
#endif

	return;
}

static int
dequeue_requests(puffs_cookie_t opc, enum perfuse_qtype type, int max)
{
	struct perfuse_cc_queue *pcq;
	struct perfuse_node_data *pnd;
	int dequeued;

	pnd = PERFUSE_NODE_DATA(opc);
	dequeued = 0;
	TAILQ_FOREACH(pcq, &pnd->pnd_pcq, pcq_next) {
		if (pcq->pcq_type != type)
			continue;
	
#ifdef PERFUSE_DEBUG
		if (perfuse_diagflags & PDF_REQUEUE)
			DPRINTF("%s: SCHEDULE opc = %p, pcc = %p (%s)\n",
				__func__, (void *)opc, pcq->pcq_cc,
				 perfuse_qtypestr[type]);
#endif
		puffs_cc_schedule(pcq->pcq_cc);
		
		if (++dequeued == max)
			break;
	}

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_REQUEUE)
		DPRINTF("%s: DONE  opc = %p\n", __func__, (void *)opc);
#endif

	return dequeued;
}
	
void
perfuse_fs_init(struct puffs_usermount *pu)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	struct fuse_init_in *fii;
	struct fuse_init_out *fio;
	int error;

	ps = puffs_getspecific(pu);
	
        if (puffs_mount(pu, ps->ps_target, ps->ps_mountflags, ps->ps_root) != 0)
                DERR(EX_OSERR, "%s: puffs_mount failed", __func__);

	/*
	 * Linux 2.6.34.1 sends theses flags:
	 * FUSE_ASYNC_READ | FUSE_POSIX_LOCKS | FUSE_ATOMIC_O_TRUNC
	 * FUSE_EXPORT_SUPPORT | FUSE_BIG_WRITES | FUSE_DONT_MASK
	 *
	 * Linux also sets max_readahead at 32 pages (128 kB)
	 *
	 * ps_new_msg() is called with NULL creds, which will
	 * be interpreted as FUSE superuser. 
	 */
	pm = ps->ps_new_msg(pu, 0, FUSE_INIT, sizeof(*fii), NULL);
	fii = GET_INPAYLOAD(ps, pm, fuse_init_in);
	fii->major = FUSE_KERNEL_VERSION;
	fii->minor = FUSE_KERNEL_MINOR_VERSION;
	fii->max_readahead = (unsigned int)(32 * sysconf(_SC_PAGESIZE));
	fii->flags = (FUSE_ASYNC_READ|FUSE_POSIX_LOCKS|FUSE_ATOMIC_O_TRUNC);

	if ((error = xchg_msg(pu, 0, pm, sizeof(*fio), wait_reply)) != 0)
		DERRX(EX_SOFTWARE, "init message exchange failed (%d)", error);

	fio = GET_OUTPAYLOAD(ps, pm, fuse_init_out);
	ps->ps_max_readahead = fio->max_readahead;
	ps->ps_max_write = fio->max_write;

	ps->ps_destroy_msg(pm);

	return;
}

int
perfuse_fs_unmount(struct puffs_usermount *pu, int flags)
{
	perfuse_msg_t *pm;
	struct perfuse_state *ps;
	puffs_cookie_t opc;
	int error;

	ps = puffs_getspecific(pu);
	opc = (puffs_cookie_t)puffs_getroot(pu);

	/*
	 * ps_new_msg() is called with NULL creds, which will
	 * be interpreted as FUSE superuser. 
	 */
	pm = ps->ps_new_msg(pu, opc, FUSE_DESTROY, 0, NULL);

	if ((error = xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, wait_reply)) != 0){
		DWARN("unmount %s", ps->ps_target);
		if (!(flags & MNT_FORCE))
			return error;
		else
			error = 0;
	} else {
		ps->ps_destroy_msg(pm);
	}

	ps->ps_umount(pu);

	if (perfuse_diagflags & PDF_MISC)
		DPRINTF("%s unmounted, exit\n", ps->ps_target);

	return 0;
}

int
perfuse_fs_statvfs(struct puffs_usermount *pu, struct statvfs *svfsb)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	puffs_cookie_t opc;
	struct fuse_statfs_out *fso;
	int error;

	ps = puffs_getspecific(pu);
	opc = (puffs_cookie_t)puffs_getroot(pu);

	/*
	 * ps_new_msg() is called with NULL creds, which will
	 * be interpreted as FUSE superuser. 
	 */
	pm = ps->ps_new_msg(pu, opc, FUSE_STATFS, 0, NULL);

	if ((error = xchg_msg(pu, opc, pm, sizeof(*fso), wait_reply)) != 0)
		return error;

	fso = GET_OUTPAYLOAD(ps, pm, fuse_statfs_out);
	svfsb->f_flag = ps->ps_mountflags;
	svfsb->f_bsize = fso->st.bsize;
	svfsb->f_frsize = fso->st.frsize;
	svfsb->f_iosize = ((struct puffs_node *)opc)->pn_va.va_blocksize;
	svfsb->f_blocks = fso->st.blocks;
	svfsb->f_bfree = fso->st.bfree;
	svfsb->f_bavail = fso->st.bavail;
	svfsb->f_bresvd = fso->st.bfree - fso->st.bavail;
	svfsb->f_files = fso->st.files;
	svfsb->f_ffree = fso->st.ffree;
	svfsb->f_favail = fso->st.ffree;/* files not reserved for root */
	svfsb->f_fresvd = 0;		/* files reserved for root */ 

	svfsb->f_syncreads = ps->ps_syncreads;
	svfsb->f_syncwrites = ps->ps_syncwrites;

	svfsb->f_asyncreads = ps->ps_asyncreads;
	svfsb->f_asyncwrites = ps->ps_asyncwrites;

	(void)memcpy(&svfsb->f_fsidx, &ps->ps_fsid, sizeof(ps->ps_fsid));
	svfsb->f_fsid = (unsigned long)ps->ps_fsid;
	svfsb->f_namemax = MAXPATHLEN;	/* XXX */
	svfsb->f_owner = ps->ps_owner_uid;

	(void)strlcpy(svfsb->f_mntonname, ps->ps_target, _VFS_NAMELEN);

	if (ps->ps_filesystemtype != NULL)
		(void)strlcpy(svfsb->f_fstypename, 
			      ps->ps_filesystemtype, _VFS_NAMELEN);
	else
		(void)strlcpy(svfsb->f_fstypename, "fuse", _VFS_NAMELEN);

	if (ps->ps_source != NULL)
		strlcpy(svfsb->f_mntfromname, ps->ps_source, _VFS_NAMELEN);
	else
		strlcpy(svfsb->f_mntfromname, _PATH_FUSE, _VFS_NAMELEN);

	ps->ps_destroy_msg(pm);

	return 0;
}

int
perfuse_fs_sync(struct puffs_usermount *pu, int waitfor,
	const struct puffs_cred *pcr)
{
	/* 
	 * FUSE does not seem to have a FS sync callback.
	 * Maybe do not even register this callback
	 */
	return puffs_fsnop_sync(pu, waitfor, pcr);
}

/* ARGSUSED0 */
int
perfuse_fs_fhtonode(struct puffs_usermount *pu, void *fid, size_t fidsize,
	struct puffs_newinfo *pni)
{
	DERRX(EX_SOFTWARE, "%s: UNIMPLEMENTED (FATAL)", __func__);
	return 0;
}

/* ARGSUSED0 */
int 
perfuse_fs_nodetofh(struct puffs_usermount *pu, puffs_cookie_t cookie,
	void *fid, size_t *fidsize)
{
	DERRX(EX_SOFTWARE, "%s: UNIMPLEMENTED (FATAL)", __func__);
	return 0;
}

#if 0
/* ARGSUSED0 */
void 
perfuse_fs_extattrctl(struct puffs_usermount *pu, int cmd,
	puffs_cookie_t *cookie, int flags, int namespace, const char *attrname)
{
	DERRX(EX_SOFTWARE, "%s: UNIMPLEMENTED (FATAL)", __func__);
	return 0;
}
#endif /* 0 */

/* ARGSUSED0 */
void
perfuse_fs_suspend(struct puffs_usermount *pu, int status)
{
	return;
}


int
perfuse_node_lookup(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn)
{
	struct perfuse_state *ps;
	struct puffs_node *pn;
	mode_t mode;
	int error;

	ps = puffs_getspecific(pu);
	node_ref(opc);

	/*
	 * Check permissions
	 */
	switch(pcn->pcn_nameiop) {
	case NAMEI_DELETE: /* FALLTHROUGH */
	case NAMEI_RENAME: /* FALLTHROUGH */
	case NAMEI_CREATE:
		if (pcn->pcn_flags & NAMEI_ISLASTCN)
			mode = PUFFS_VEXEC|PUFFS_VWRITE;
		else
			mode = PUFFS_VEXEC;
		break;
	case NAMEI_LOOKUP: /* FALLTHROUGH */
	default:
		mode = PUFFS_VEXEC;
		break;
	}

	if ((error = mode_access(opc, pcn->pcn_cred, mode)) != 0) 
		goto out;

	error = node_lookup_common(pu, (puffs_cookie_t)opc, pni,
				   pcn->pcn_name, pcn->pcn_cred, &pn);

	if (error != 0)
		goto out;

	/*
	 * Kernel would kill us if the filesystem returned the parent
	 * itself. If we want to live, hide that!
	 */
	if ((opc == (puffs_cookie_t)pn) && (strcmp(pcn->pcn_name, ".") != 0)) {
		DERRX(EX_SOFTWARE, "lookup \"%s\" in \"%s\" returned parent",
		      pcn->pcn_name, perfuse_node_path(ps, opc));
		/* NOTREACHED */
		error = ESTALE;
		goto out;
	}

	/*
	 * Removed node
	 */
	if (PERFUSE_NODE_DATA(pn)->pnd_flags & PND_REMOVED) {
		error = ENOENT;
		goto out;
	}

	/*
	 * Check for sticky bit. Unfortunately there is no way to 
	 * do this before creating the puffs_node, since we require
	 * this operation to get the node owner.
	 */
	switch (pcn->pcn_nameiop) {
	case NAMEI_DELETE: /* FALLTHROUGH */
	case NAMEI_RENAME:
		error = sticky_access(opc, pn, pcn->pcn_cred);
		if (error != 0) {
			(void)perfuse_node_reclaim(pu, pn);
			goto out;
		}
		break;
	default:
		break;
	}

	PERFUSE_NODE_DATA(pn)->pnd_puffs_nlookup++;

	error = 0;

out:
	node_rele(opc);
	return error;
}

int
perfuse_node_create(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn,
	const struct vattr *vap)
{
	perfuse_msg_t *pm;
	struct perfuse_state *ps;
	struct fuse_create_in *fci;
	struct fuse_entry_out *feo;
	struct fuse_open_out *foo;
	struct puffs_node *pn;
	const char *name;
	size_t namelen;
	size_t len;
	int error;
	
	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_REMOVED)
		return ENOENT;

	node_ref(opc);

	/*
	 * If create is unimplemented: Check that it does not
	 * already exists, and if not, do mknod and open
	 */
	ps = puffs_getspecific(pu);
	if (ps->ps_flags & PS_NO_CREAT) {
		error = node_lookup_common(pu, opc, NULL, pcn->pcn_name,
					   pcn->pcn_cred, &pn);
		if (error == 0)	{
			(void)perfuse_node_reclaim(pu, pn);
			error = EEXIST;
			goto out;
		}

		error = perfuse_node_mknod(pu, opc, pni, pcn, vap);
		if (error != 0) 
			goto out;

		error = node_lookup_common(pu, opc, NULL, pcn->pcn_name,
					   pcn->pcn_cred, &pn);
		if (error != 0)
			goto out;

		/*
		 * FUSE does the open at create time, while
		 * NetBSD will open in a subsequent operation.
		 * We need to open now, in order to retain FUSE
		 * semantics. The calling process will not get 
		 * a file descriptor before the kernel sends 
		 * the open operation.
		 */
		error = perfuse_node_open(pu, (puffs_cookie_t)pn,
					  FWRITE, pcn->pcn_cred);
		goto out;
	}

	name = pcn->pcn_name;
	namelen = pcn->pcn_namelen + 1;
	len = sizeof(*fci) + namelen;

	/*
	 * flags should use O_WRONLY instead of O_RDWR, but it
	 * breaks when the caller tries to read from file.
	 * 
	 * mode must contain file type (ie: S_IFREG), use VTTOIF(vap->va_type)
	 */
	pm = ps->ps_new_msg(pu, opc, FUSE_CREATE, len, pcn->pcn_cred);
	fci = GET_INPAYLOAD(ps, pm, fuse_create_in);
	fci->flags = O_CREAT | O_TRUNC | O_RDWR;
	fci->mode = vap->va_mode | VTTOIF(vap->va_type);
	fci->umask = 0; 	/* Seems unused by libfuse */
	(void)strlcpy((char*)(void *)(fci + 1), name, namelen);

	len = sizeof(*feo) + sizeof(*foo);
	if ((error = xchg_msg(pu, opc, pm, len, wait_reply)) != 0) {
		/*
		 * create is unimplmented, remember it for later,
		 * and start over using mknod and open instead.
		 */
		if (error == ENOSYS) {
			ps->ps_flags |= PS_NO_CREAT;
			error = perfuse_node_create(pu, opc, pni, pcn, vap);
		}

		goto out;
	}

	feo = GET_OUTPAYLOAD(ps, pm, fuse_entry_out);
	foo = (struct fuse_open_out *)(void *)(feo + 1);
	if (feo->nodeid == PERFUSE_UNKNOWN_NODEID)
		DERRX(EX_SOFTWARE, "%s: no nodeid", __func__);
	
	/*
	 * Save the file handle and inode in node private data 
	 * so that we can reuse it later
	 */
	pn = perfuse_new_pn(pu, name, opc);
	perfuse_new_fh((puffs_cookie_t)pn, foo->fh, FWRITE);
	PERFUSE_NODE_DATA(pn)->pnd_nodeid = feo->nodeid;
	PERFUSE_NODE_DATA(pn)->pnd_puffs_nlookup++;
	perfuse_node_cache(ps, pn);

	fuse_attr_to_vap(ps, &pn->pn_va, &feo->attr);
	pn->pn_va.va_gen = (u_long)(feo->generation);

	puffs_newinfo_setcookie(pni, pn);
#ifdef PUFFS_KFLAG_CACHE_FS_TTL
	puffs_newinfo_setva(pni, &pn->pn_va);
	perfuse_newinfo_setttl(pni, pn, feo, NULL);
#endif /* PUFFS_KFLAG_CACHE_FS_TTL */

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & (PDF_FH|PDF_FILENAME))
		DPRINTF("%s: opc = %p, file = \"%s\", flags = 0x%x "
			"nodeid = 0x%"PRIx64", wfh = 0x%"PRIx64"\n",
			__func__, (void *)pn, pcn->pcn_name,
			PERFUSE_NODE_DATA(pn)->pnd_flags, feo->nodeid, 
			foo->fh);
#endif

	ps->ps_destroy_msg(pm);
	error = 0;

out:
	node_rele(opc);
	return error;
}


int
perfuse_node_mknod(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn,
	const struct vattr *vap)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	struct fuse_mknod_in *fmi;
	const char* path;
	size_t len;
	int error;
	
	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_REMOVED)
		return ENOENT;

	node_ref(opc);

	/*
	 * Only superuser can mknod objects other than 
	 * directories, files, socks, fifo and links.
	 *
	 * Create an object require -WX permission in the parent directory
	 */
	switch (vap->va_type) {
	case VDIR:	/* FALLTHROUGH */
	case VREG:	/* FALLTHROUGH */
	case VFIFO:	/* FALLTHROUGH */
	case VSOCK:
		break;
	default:	/* VNON, VBLK, VCHR, VBAD */
		if (!puffs_cred_isjuggernaut(pcn->pcn_cred)) {
			error = EACCES;
			goto out;
		}
		break;
	}


	ps = puffs_getspecific(pu);
	path = pcn->pcn_name;
	len = sizeof(*fmi) + pcn->pcn_namelen + 1; 

	/*	
	 * mode must contain file type (ie: S_IFREG), use VTTOIF(vap->va_type)
	 */
	pm = ps->ps_new_msg(pu, opc, FUSE_MKNOD, len, pcn->pcn_cred);
	fmi = GET_INPAYLOAD(ps, pm, fuse_mknod_in);
	fmi->mode = vap->va_mode | VTTOIF(vap->va_type);
	fmi->rdev = (uint32_t)vap->va_rdev;
	fmi->umask = 0; 	/* Seems unused bu libfuse */
	(void)strlcpy((char *)(void *)(fmi + 1), path, len - sizeof(*fmi));

	error = node_mk_common(pu, opc, pni, pcn, pm);

out:
	node_rele(opc);
	return error;	
}


int
perfuse_node_open(struct puffs_usermount *pu, puffs_cookie_t opc, int mode,
	const struct puffs_cred *pcr)
{
	return perfuse_node_open2(pu, opc, mode, pcr, NULL);
}

int
perfuse_node_open2(struct puffs_usermount *pu, puffs_cookie_t opc, int mode,
	const struct puffs_cred *pcr, int *oflags)
{
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd;
	perfuse_msg_t *pm;
	mode_t fmode;
	int op;
	struct fuse_open_in *foi;
	struct fuse_open_out *foo;
	struct puffs_node *pn;
	int error;
	
	ps = puffs_getspecific(pu);
	pn = (struct puffs_node *)opc;
	pnd = PERFUSE_NODE_DATA(opc);
	error = 0;

	if (pnd->pnd_flags & PND_REMOVED)
		return ENOENT;

	node_ref(opc);

	if (puffs_pn_getvap(pn)->va_type == VDIR)
		op = FUSE_OPENDIR;
	else
		op = FUSE_OPEN;

	/*
	 * libfuse docs says
	 * - O_CREAT and O_EXCL should never be set.
	 * - O_TRUNC may be used if mount option atomic_o_trunc is used XXX
	 *
	 * O_APPEND makes no sense since FUSE always sends
	 * the file offset for write operations. If the 
	 * filesystem uses pwrite(), O_APPEND would cause
	 * the offset to be ignored and cause file corruption.
	 */
	mode &= ~(O_CREAT|O_EXCL|O_APPEND);

	/*
	 * Do not open twice, and do not reopen for reading
	 * if we already have write handle.
	 */
	switch (mode & (FREAD|FWRITE)) {
	case FREAD:
		if (pnd->pnd_flags & (PND_RFH|PND_WFH))
			goto out;
		break;
	case FWRITE:
		if (pnd->pnd_flags & PND_WFH)
			goto out;
		break;
	case FREAD|FWRITE:
		if (pnd->pnd_flags & PND_WFH)
			goto out;

		/*
		 * Corner case: if already open for reading (PND_RFH)
		 * and re-opening FREAD|FWRITE, we need to reopen, 
		 * but only for writing. Note the change on mode 
		 * will only affect perfuse_new_fh()
		 */
		if (pnd->pnd_flags & PND_RFH)
			mode &= ~FREAD;
		break;
	default:
		DWARNX("open without either FREAD nor FWRITE");
		error = EPERM;
		goto out;
	}
	
	/*
	 * Queue open on a node so that we do not open
	 * twice. This would be better with read and
	 * write distinguished.
	 */
	while (pnd->pnd_flags & PND_INOPEN)
		requeue_request(pu, opc, PCQ_OPEN);
	pnd->pnd_flags |= PND_INOPEN;

	/*
	 * Convert PUFFS mode to FUSE mode: convert FREAD/FWRITE
	 * to O_RDONLY/O_WRONLY while perserving the other options.
	 */
	fmode = mode & ~(FREAD|FWRITE);
	fmode |= (mode & FWRITE) ? O_RDWR : O_RDONLY;

	pm = ps->ps_new_msg(pu, opc, op, sizeof(*foi), pcr);
	foi = GET_INPAYLOAD(ps, pm, fuse_open_in);
	foi->flags = fmode;
	foi->unused = 0;

	if ((error = xchg_msg(pu, opc, pm, sizeof(*foo), wait_reply)) != 0)
		goto out;

	foo = GET_OUTPAYLOAD(ps, pm, fuse_open_out);

	/*
	 * Save the file handle in node private data 
	 * so that we can reuse it later
	 */
	perfuse_new_fh(opc, foo->fh, mode);

	/*
	 * Set direct I/O if the filesystems forces it
	 */
	if ((foo->open_flags & FUSE_FOPEN_DIRECT_IO) && (oflags != NULL))
		*oflags |= PUFFS_OPEN_IO_DIRECT;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & (PDF_FH|PDF_FILENAME))
		DPRINTF("%s: opc = %p, file = \"%s\", "
			"nodeid = 0x%"PRIx64", %s%sfh = 0x%"PRIx64"\n",
			__func__, (void *)opc, perfuse_node_path(ps, opc),
			pnd->pnd_nodeid, mode & FREAD ? "r" : "",
			mode & FWRITE ? "w" : "", foo->fh);
#endif

	ps->ps_destroy_msg(pm);
out:

	pnd->pnd_flags &= ~PND_INOPEN;
	(void)dequeue_requests(opc, PCQ_OPEN, DEQUEUE_ALL);

	node_rele(opc);
	return error;
}

/* ARGSUSED0 */
int
perfuse_node_close(struct puffs_usermount *pu, puffs_cookie_t opc, int flags,
	const struct puffs_cred *pcr)
{
	struct perfuse_node_data *pnd;

	pnd = PERFUSE_NODE_DATA(opc);

	if (!(pnd->pnd_flags & PND_OPEN))
		return EBADF;

	/* 
	 * Actual close is postponed at inactive time.
	 */
	return 0;
}

int
perfuse_node_access(struct puffs_usermount *pu, puffs_cookie_t opc, int mode,
	const struct puffs_cred *pcr)
{
	perfuse_msg_t *pm;
	struct perfuse_state *ps;
	struct fuse_access_in *fai;
	int error;
	
	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_REMOVED)
		return ENOENT;

	node_ref(opc);

	/* 
	 * If we previously detected the filesystem does not 
	 * implement access(), short-circuit the call and skip 
	 * to libpuffs access() emulation.
	 */
	ps = puffs_getspecific(pu);
	if (ps->ps_flags & PS_NO_ACCESS) {
		const struct vattr *vap;

		vap = puffs_pn_getvap((struct puffs_node *)opc);

		error = puffs_access(IFTOVT(vap->va_mode), 
				     vap->va_mode & ACCESSPERMS, 
				     vap->va_uid, vap->va_gid,
				     (mode_t)mode, pcr); 
		goto out;
	}

	/*
	 * Plain access call 
	 */
	pm = ps->ps_new_msg(pu, opc, FUSE_ACCESS, sizeof(*fai), pcr);
	fai = GET_INPAYLOAD(ps, pm, fuse_access_in);
	fai->mask = 0;
	fai->mask |= (mode & PUFFS_VREAD) ? R_OK : 0;
	fai->mask |= (mode & PUFFS_VWRITE) ? W_OK : 0;
	fai->mask |= (mode & PUFFS_VEXEC) ? X_OK : 0;

	error = xchg_msg(pu, opc, pm, NO_PAYLOAD_REPLY_LEN, wait_reply);

	ps->ps_destroy_msg(pm);

	/*
	 * If unimplemented, start over with emulation
	 */
	if (error == ENOSYS) {
		ps->ps_flags |= PS_NO_ACCESS;
		error = perfuse_node_access(pu, opc, mode, pcr);
	}

out:
	node_rele(opc);
	return error;
}

int
perfuse_node_getattr(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct vattr *vap, const struct puffs_cred *pcr)
{
	return perfuse_node_getattr_ttl(pu, opc, vap, pcr, NULL);
}

int
perfuse_node_getattr_ttl(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct vattr *vap, const struct puffs_cred *pcr,
	struct timespec *va_ttl)
{
	perfuse_msg_t *pm = NULL;
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd = PERFUSE_NODE_DATA(opc);
	struct fuse_getattr_in *fgi;
	struct fuse_attr_out *fao;
	int error = 0;
	
	if ((pnd->pnd_flags & PND_REMOVED) && !(pnd->pnd_flags & PND_OPEN))
		return ENOENT;

	node_ref(opc);

	/* 
	 * Serialize size access, see comment in perfuse_node_setattr().
	 */
	while (pnd->pnd_flags & PND_INRESIZE)
		requeue_request(pu, opc, PCQ_RESIZE);
	pnd->pnd_flags |= PND_INRESIZE;

	ps = puffs_getspecific(pu);
		
	/*
	 * FUSE_GETATTR_FH must be set in fgi->flags 
	 * if we use for fgi->fh
	 */
	pm = ps->ps_new_msg(pu, opc, FUSE_GETATTR, sizeof(*fgi), pcr);
	fgi = GET_INPAYLOAD(ps, pm, fuse_getattr_in);
	fgi->getattr_flags = 0; 
	fgi->dummy = 0;
	fgi->fh = 0;

	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_OPEN) {
		fgi->fh = perfuse_get_fh(opc, FREAD);
		fgi->getattr_flags |= FUSE_GETATTR_FH;
	}

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_RESIZE)
		DPRINTF(">> %s %p %" PRIu64 "\n", __func__, (void *)opc,
		    vap->va_size);
#endif

	if ((error = xchg_msg(pu, opc, pm, sizeof(*fao), wait_reply)) != 0)
		goto out;

	fao = GET_OUTPAYLOAD(ps, pm, fuse_attr_out);

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_RESIZE)
		DPRINTF("<< %s %p %" PRIu64 " -> %" PRIu64 "\n", __func__,
		    (void *)opc, vap->va_size, fao->attr.size);
#endif

	/* 
	 * We set birthtime, flags, filerev,vaflags to 0. 
	 * This seems the best bet, since the information is
	 * not available from filesystem.
	 */
	fuse_attr_to_vap(ps, vap, &fao->attr);

	if (va_ttl != NULL) {
		va_ttl->tv_sec = fao->attr_valid;
		va_ttl->tv_nsec = fao->attr_valid_nsec;
	}

	ps->ps_destroy_msg(pm);
	error = 0;
out:

	pnd->pnd_flags &= ~PND_INRESIZE;
	(void)dequeue_requests(opc, PCQ_RESIZE, DEQUEUE_ALL);

	node_rele(opc);
	return error;
}

int
perfuse_node_setattr(struct puffs_usermount *pu, puffs_cookie_t opc,
	const struct vattr *vap, const struct puffs_cred *pcr)
{
	return perfuse_node_setattr_ttl(pu, opc, 
					__UNCONST(vap), pcr, NULL, 0);
}

int
perfuse_node_setattr_ttl(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct vattr *vap, const struct puffs_cred *pcr,
	struct timespec *va_ttl, int xflag)
{
	perfuse_msg_t *pm;
	uint64_t fh;
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd;
	struct fuse_setattr_in *fsi;
	struct fuse_attr_out *fao;
	struct vattr *old_va;
	enum perfuse_xchg_pb_reply reply;
	int error;
#ifdef PERFUSE_DEBUG
	struct vattr *old_vap;
	int resize_debug = 0;
#endif
	ps = puffs_getspecific(pu);
	pnd = PERFUSE_NODE_DATA(opc);

	/*
	 * The only operation we can do once the file is removed 
	 * is to resize it, and we can do it only if it is open.
	 * Do not even send the operation to the filesystem: the
	 * file is not there anymore.
	 */
	if (pnd->pnd_flags & PND_REMOVED) {
		if (!(pnd->pnd_flags & PND_OPEN))
			return ENOENT;
		
		return 0;
	}

	old_va = puffs_pn_getvap((struct puffs_node *)opc);

	/*
	 * Check for permission to change size
	 * It is always allowed if we already have a write file handle
	 */
	if ((vap->va_size != (u_quad_t)PUFFS_VNOVAL) &&
	    !(pnd->pnd_flags & PND_WFH) &&
	    (error = mode_access(opc, pcr, PUFFS_VWRITE)) != 0)
		return error;

	/*
	 * Check for permission to change dates
	 */
	if (((vap->va_atime.tv_sec != (time_t)PUFFS_VNOVAL) ||
	     (vap->va_mtime.tv_sec != (time_t)PUFFS_VNOVAL)) &&
	    (puffs_access_times(old_va->va_uid, old_va->va_gid,
				old_va->va_mode, 0, pcr) != 0))
		return EACCES;

	/*
	 * Check for permission to change owner and group
	 */
	if (((vap->va_uid != (uid_t)PUFFS_VNOVAL) ||
	     (vap->va_gid != (gid_t)PUFFS_VNOVAL)) &&
	    (puffs_access_chown(old_va->va_uid, old_va->va_gid,
				vap->va_uid, vap->va_gid, pcr)) != 0)
		return EACCES;

	/*
	 * Check for permission to change permissions
	 */
	if ((vap->va_mode != (mode_t)PUFFS_VNOVAL) &&
	    (puffs_access_chmod(old_va->va_uid, old_va->va_gid,
				old_va->va_type, vap->va_mode, pcr)) != 0)
		return EACCES;
	
	node_ref(opc);
	
	if (pnd->pnd_flags & PND_WFH)
		fh = perfuse_get_fh(opc, FWRITE);
	else
		fh = FUSE_UNKNOWN_FH;

	/*
	 * fchmod() sets mode and fh, and it may carry
	 * a resize as well. That may break if the 
	 * filesystem does chmod then resize, and fails
	 * because it does not have permission anymore.
	 * We work this around by splitting into two setattr.
	 */
	if ((vap->va_size != (u_quad_t)PUFFS_VNOVAL) &&
	    (vap->va_mode != (mode_t)PUFFS_VNOVAL) &&
	    (fh != FUSE_UNKNOWN_FH)) {
		struct vattr resize_va;

		(void)memcpy(&resize_va, vap, sizeof(resize_va));
		resize_va.va_mode = (mode_t)PUFFS_VNOVAL;
		if ((error = perfuse_node_setattr_ttl(pu, opc, &resize_va,
						      pcr, va_ttl, xflag)) != 0)
			goto out2;

		vap->va_size = (u_quad_t)PUFFS_VNOVAL;
	}

	pm = ps->ps_new_msg(pu, opc, FUSE_SETATTR, sizeof(*fsi), pcr);
	fsi = GET_INPAYLOAD(ps, pm, fuse_setattr_in);
	fsi->valid = 0;

	/*
	 * Get a fh if the node is open for writing
	 */
	if (fh != FUSE_UNKNOWN_FH) {
		fsi->fh = fh;
		fsi->valid |= FUSE_FATTR_FH;
	}


	if (vap->va_size != (u_quad_t)PUFFS_VNOVAL) {
		fsi->size = vap->va_size;
		fsi->valid |= FUSE_FATTR_SIZE;

		/* 
		 * Serialize anything that can touch file size
		 * to avoid reordered GETATTR and SETATTR.
		 * Out of order SETATTR can report stale size,
		 * which will cause the kernel to truncate the file.
		 * XXX Probably useless now we have a lock on GETATTR
		 */
		while (pnd->pnd_flags & PND_INRESIZE)
			requeue_request(pu, opc, PCQ_RESIZE);
		pnd->pnd_flags |= PND_INRESIZE;
	}

	/*
 	 * Setting mtime without atime or vice versa leads to
	 * dates being reset to Epoch on glusterfs. If one
	 * is missing, use the old value.
 	 */
	if ((vap->va_mtime.tv_sec != (time_t)PUFFS_VNOVAL) || 
	    (vap->va_atime.tv_sec != (time_t)PUFFS_VNOVAL)) {
		
		if (vap->va_atime.tv_sec != (time_t)PUFFS_VNOVAL) {
			fsi->atime = vap->va_atime.tv_sec;
			fsi->atimensec = (uint32_t)vap->va_atime.tv_nsec;
		} else {
			fsi->atime = old_va->va_atime.tv_sec;
			fsi->atimensec = (uint32_t)old_va->va_atime.tv_nsec;
		}

		if (vap->va_mtime.tv_sec != (time_t)PUFFS_VNOVAL) {
			fsi->mtime = vap->va_mtime.tv_sec;
			fsi->mtimensec = (uint32_t)vap->va_mtime.tv_nsec;
		} else {
			fsi->mtime = old_va->va_mtime.tv_sec;
			fsi->mtimensec = (uint32_t)old_va->va_mtime.tv_nsec;
		}

		fsi->valid |= (FUSE_FATTR_MTIME|FUSE_FATTR_ATIME);
	}

	if (vap->va_mode != (mode_t)PUFFS_VNOVAL) {
		fsi->mode = vap->va_mode; 
		fsi->valid |= FUSE_FATTR_MODE;
	}
	
	if (vap->va_uid != (uid_t)PUFFS_VNOVAL) {
		fsi->uid = vap->va_uid;
		fsi->valid |= FUSE_FATTR_UID;
	}

	if (vap->va_gid != (gid_t)PUFFS_VNOVAL) {
		fsi->gid = vap->va_gid;
		fsi->valid |= FUSE_FATTR_GID;
	}

	if (pnd->pnd_lock_owner != 0) {
		fsi->lock_owner = pnd->pnd_lock_owner;
		fsi->valid |= FUSE_FATTR_LOCKOWNER;
	}

	/*
	 * ftruncate() sends only va_size, and metadata cache
	 * flush adds va_atime and va_mtime. Some FUSE
	 * filesystems will attempt to detect ftruncate by 
	 * checking for FATTR_SIZE being set without
	 * FATTR_UID|FATTR_GID|FATTR_ATIME|FATTR_MTIME|FATTR_MODE
	 * 
	 * Try to adapt and remove FATTR_ATIME|FATTR_MTIME
	 * if we suspect a ftruncate().
	 */ 
	if ((vap->va_size != (u_quad_t)PUFFS_VNOVAL) &&
	    ((vap->va_mode == (mode_t)PUFFS_VNOVAL) &&
	     (vap->va_uid == (uid_t)PUFFS_VNOVAL) &&
	     (vap->va_gid == (gid_t)PUFFS_VNOVAL))) {
		fsi->atime = 0;
		fsi->atimensec = 0;
		fsi->mtime = 0;
		fsi->mtimensec = 0;
		fsi->valid &= ~(FUSE_FATTR_ATIME|FUSE_FATTR_MTIME);
	}
		    
	/*
	 * If nothing remain, discard the operation.
	 */
	if (!(fsi->valid & (FUSE_FATTR_SIZE|FUSE_FATTR_ATIME|FUSE_FATTR_MTIME|
			    FUSE_FATTR_MODE|FUSE_FATTR_UID|FUSE_FATTR_GID))) {
		error = 0;
		ps->ps_destroy_msg(pm);
		goto out;
	}

#ifdef PERFUSE_DEBUG
	old_vap = puffs_pn_getvap((struct puffs_node *)opc);

	if ((perfuse_diagflags & PDF_RESIZE) &&
	    (old_vap->va_size != (u_quad_t)PUFFS_VNOVAL)) {
		resize_debug = 1;

		DPRINTF(">> %s %p %" PRIu64 " -> %" PRIu64 "\n", __func__,
		    (void *)opc,
		    puffs_pn_getvap((struct puffs_node *)opc)->va_size, 
		    fsi->size);
	}
#endif

	/*
	 * Do not honour FAF when changing size. How do
	 * you want such a thing to work?
	 */
	reply = wait_reply;
#ifdef PUFFS_SETATTR_FAF
	if ((xflag & PUFFS_SETATTR_FAF) && !(fsi->valid & FUSE_FATTR_SIZE))
		reply = no_reply;
#endif
	if ((error = xchg_msg(pu, opc, pm, sizeof(*fao), reply)) != 0)
		goto out;

	if (reply == no_reply)
		goto out;

	/*
	 * Copy back the new values 
	 */
	fao = GET_OUTPAYLOAD(ps, pm, fuse_attr_out);

#ifdef PERFUSE_DEBUG
	if (resize_debug)
		DPRINTF("<< %s %p %" PRIu64 " -> %" PRIu64 "\n", __func__,
		    (void *)opc, old_vap->va_size, fao->attr.size);
#endif

	fuse_attr_to_vap(ps, old_va, &fao->attr);

	if (va_ttl != NULL) {
		va_ttl->tv_sec = fao->attr_valid;
		va_ttl->tv_nsec = fao->attr_valid_nsec;
		(void)memcpy(vap, old_va, sizeof(*vap));
	}

	ps->ps_destroy_msg(pm);
	error = 0;

out:
	if (pnd->pnd_flags & PND_INRESIZE) {
		pnd->pnd_flags &= ~PND_INRESIZE;
		(void)dequeue_requests(opc, PCQ_RESIZE, DEQUEUE_ALL);
	}

out2:
	node_rele(opc);
	return error;
}

int
perfuse_node_poll(struct puffs_usermount *pu, puffs_cookie_t opc, int *events)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	struct fuse_poll_in *fpi;
	struct fuse_poll_out *fpo;
	int error;

	node_ref(opc);
	ps = puffs_getspecific(pu);
	/*
	 * kh is set if FUSE_POLL_SCHEDULE_NOTIFY is set.
	 *
	 * XXX ps_new_msg() is called with NULL creds, which will
	 * be interpreted as FUSE superuser. We have no way to 
	 * know the requesting process' credential, but since poll
	 * is supposed to operate on a file that has been open,
	 * permission should have already been checked at open time.
	 * That still may breaks on filesystems that provides odd 
	 * semantics.
 	 */
	pm = ps->ps_new_msg(pu, opc, FUSE_POLL, sizeof(*fpi), NULL);
	fpi = GET_INPAYLOAD(ps, pm, fuse_poll_in);
	fpi->fh = perfuse_get_fh(opc, FREAD);
	fpi->kh = 0;
	fpi->flags = 0;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FH)
		DPRINTF("%s: opc = %p, nodeid = 0x%"PRIx64", "
			"fh = 0x%"PRIx64"\n", __func__, (void *)opc,	
			PERFUSE_NODE_DATA(opc)->pnd_nodeid, fpi->fh);
#endif
	if ((error = xchg_msg(pu, opc, pm, sizeof(*fpo), wait_reply)) != 0)
		goto out;

	fpo = GET_OUTPAYLOAD(ps, pm, fuse_poll_out);
	*events = fpo->revents;

	ps->ps_destroy_msg(pm);
	error = 0;

out:
	node_rele(opc);
	return error;
}

/* ARGSUSED2 */
int
perfuse_node_fsync(struct puffs_usermount *pu, puffs_cookie_t opc,
	const struct puffs_cred *pcr, int flags, off_t offlo, off_t offhi)
{
	int op;
	perfuse_msg_t *pm;
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd;
	struct fuse_fsync_in *ffi;
	uint64_t fh;
	int error = 0;
	
	pm = NULL;
	ps = puffs_getspecific(pu);
	pnd = PERFUSE_NODE_DATA(opc);

	/*
	 * No need to sync a removed node
	 */
	if (pnd->pnd_flags & PND_REMOVED)
		return 0;

	/*
	 * We do not sync closed files. They have been 
	 * sync at inactive time already.
	 */
	if (!(pnd->pnd_flags & PND_OPEN))
		return 0;

	node_ref(opc);

	if (puffs_pn_getvap((struct puffs_node *)opc)->va_type == VDIR) 
		op = FUSE_FSYNCDIR;
	else 		/* VREG but also other types such as VLNK */
		op = FUSE_FSYNC;

	/*
	 * Do not sync if there are no change to sync
	 * XXX remove that test on files if we implement mmap
	 */
#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_SYNC)
		DPRINTF("%s: TEST opc = %p, file = \"%s\" is %sdirty\n", 
			__func__, (void*)opc, perfuse_node_path(ps, opc),
			pnd->pnd_flags & PND_DIRTY ? "" : "not ");
#endif
	if (!(pnd->pnd_flags & PND_DIRTY))
		goto out;

	/*
	 * It seems NetBSD can call fsync without open first
	 * glusterfs complain in such a situation:
	 * "FSYNC() ERR => -1 (Invalid argument)"
	 * The file will be closed at inactive time.
	 * 
	 * We open the directory for reading in order to sync.
	 * This sounds rather counterintuitive, but it works.
	 */
	if (!(pnd->pnd_flags & PND_WFH)) {
		if ((error = perfuse_node_open(pu, opc, FREAD, pcr)) != 0)
			goto out;
	}

	if (op == FUSE_FSYNCDIR)
		fh = perfuse_get_fh(opc, FREAD);
	else
		fh = perfuse_get_fh(opc, FWRITE);
	
	/*
	 * If fsync_flags  is set, meta data should not be flushed.
	 */
	pm = ps->ps_new_msg(pu, opc, op, sizeof(*ffi), pcr);
	ffi = GET_INPAYLOAD(ps, pm, fuse_fsync_in);
	ffi->fh = fh;
	ffi->fsync_flags = (flags & FFILESYNC) ? 0 : 1;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FH)
		DPRINTF("%s: opc = %p, nodeid = 0x%"PRIx64", fh = 0x%"PRIx64"\n",
			__func__, (void *)opc,
			PERFUSE_NODE_DATA(opc)->pnd_nodeid, ffi->fh);
#endif

	if ((error = xchg_msg(pu, opc, pm, 
			      NO_PAYLOAD_REPLY_LEN, wait_reply)) != 0)
		goto out;	

	/*
	 * No reply beyond fuse_out_header: nothing to do on success
	 * just clear the dirty flag
	 */
	pnd->pnd_flags &= ~PND_DIRTY;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_SYNC)
		DPRINTF("%s: CLEAR opc = %p, file = \"%s\"\n", 
			__func__, (void*)opc, perfuse_node_path(ps, opc));
#endif

	ps->ps_destroy_msg(pm);
	error = 0;

out:
	/*
	 * ENOSYS is not returned to kernel,
	 */
	if (error == ENOSYS)
		error = 0;

	node_rele(opc);
	return error;
}

int
perfuse_node_remove(struct puffs_usermount *pu, puffs_cookie_t opc,
	puffs_cookie_t targ, const struct puffs_cn *pcn)
{
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd;
	perfuse_msg_t *pm;
	char *path;
	const char *name;
	size_t len;
	int error;
	
	pnd = PERFUSE_NODE_DATA(opc);

	if ((pnd->pnd_flags & PND_REMOVED) ||
	    (PERFUSE_NODE_DATA(targ)->pnd_flags & PND_REMOVED))
		return ENOENT;

#ifdef PERFUSE_DEBUG
	if (targ == NULL)
		DERRX(EX_SOFTWARE, "%s: targ is NULL", __func__);

	if (perfuse_diagflags & (PDF_FH|PDF_FILENAME))
		DPRINTF("%s: opc = %p, remove opc = %p, file = \"%s\"\n",
			__func__, (void *)opc, (void *)targ, pcn->pcn_name);
#endif
	node_ref(opc);
	node_ref(targ);

	/*
	 * Await for all operations on the deleted node to drain, 
	 * as the filesystem may be confused to have it deleted
	 * during a getattr
	 */
	while (PERFUSE_NODE_DATA(targ)->pnd_inxchg)
		requeue_request(pu, targ, PCQ_AFTERXCHG);

	ps = puffs_getspecific(pu);
	pnd = PERFUSE_NODE_DATA(opc);
	name = pcn->pcn_name;
	len = pcn->pcn_namelen + 1;

	pm = ps->ps_new_msg(pu, opc, FUSE_UNLINK, len, pcn->pcn_cred);
	path = _GET_INPAYLOAD(ps, pm, char *);
	(void)strlcpy(path, name, len);

	if ((error = xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, wait_reply)) != 0)
		goto out;

	perfuse_cache_flush(targ);
	PERFUSE_NODE_DATA(targ)->pnd_flags |= PND_REMOVED;

	if (!(PERFUSE_NODE_DATA(targ)->pnd_flags & PND_OPEN))
		puffs_setback(puffs_cc_getcc(pu), PUFFS_SETBACK_NOREF_N2);

	/*
	 * The parent directory needs a sync
	 */
	PERFUSE_NODE_DATA(opc)->pnd_flags |= PND_DIRTY;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FILENAME)
		DPRINTF("%s: remove nodeid = 0x%"PRIx64" file = \"%s\"\n",
			__func__, PERFUSE_NODE_DATA(targ)->pnd_nodeid,
			pcn->pcn_name);
#endif
	ps->ps_destroy_msg(pm);
	error = 0;

out:
	node_rele(opc);
	node_rele(targ);
	return error;
}

int
perfuse_node_link(struct puffs_usermount *pu, puffs_cookie_t opc,
	puffs_cookie_t targ, const struct puffs_cn *pcn)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	const char *name;
	size_t len;
	struct puffs_node *pn;
	struct fuse_link_in *fli;
	int error;

	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_REMOVED)
		return ENOENT;
	
	node_ref(opc);
	node_ref(targ);
	ps = puffs_getspecific(pu);
	pn = (struct puffs_node *)targ;
	name = pcn->pcn_name;
	len =  sizeof(*fli) + pcn->pcn_namelen + 1;

	pm = ps->ps_new_msg(pu, opc, FUSE_LINK, len, pcn->pcn_cred);
	fli = GET_INPAYLOAD(ps, pm, fuse_link_in);
	fli->oldnodeid = PERFUSE_NODE_DATA(pn)->pnd_nodeid;
	(void)strlcpy((char *)(void *)(fli + 1), name, len - sizeof(*fli));

	if ((error = xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, wait_reply)) != 0)
		goto out;

	ps->ps_destroy_msg(pm);
	error = 0;

out:
	node_rele(opc);
	node_rele(targ);
	return error;
}

int
perfuse_node_rename(struct puffs_usermount *pu, puffs_cookie_t opc,
	puffs_cookie_t src, const struct puffs_cn *pcn_src,
	puffs_cookie_t targ_dir, puffs_cookie_t targ,
	const struct puffs_cn *pcn_targ)
{
	struct perfuse_state *ps;
	struct perfuse_node_data *dstdir_pnd;
	perfuse_msg_t *pm;
	struct fuse_rename_in *fri;
	const char *newname;
	const char *oldname;
	char *np;
	int error;
	size_t len;
	size_t newname_len;
	size_t oldname_len;
	
	if ((PERFUSE_NODE_DATA(opc)->pnd_flags & PND_REMOVED) ||
	    (PERFUSE_NODE_DATA(src)->pnd_flags & PND_REMOVED) ||
	    (PERFUSE_NODE_DATA(targ_dir)->pnd_flags & PND_REMOVED))
		return ENOENT;

	node_ref(opc);
	node_ref(src);

	/*
	 * Await for all operations on the deleted node to drain, 
	 * as the filesystem may be confused to have it deleted
	 * during a getattr
	 */
	if ((struct puffs_node *)targ != NULL) {
		node_ref(targ);
		while (PERFUSE_NODE_DATA(targ)->pnd_inxchg)
			requeue_request(pu, targ, PCQ_AFTERXCHG);
	} else {
		while (PERFUSE_NODE_DATA(src)->pnd_inxchg)
			requeue_request(pu, src, PCQ_AFTERXCHG);
	}

	ps = puffs_getspecific(pu);
	newname =  pcn_targ->pcn_name;
	newname_len = pcn_targ->pcn_namelen + 1;
	oldname =  pcn_src->pcn_name;
	oldname_len = pcn_src->pcn_namelen + 1;

	len = sizeof(*fri) + oldname_len + newname_len;
	pm = ps->ps_new_msg(pu, opc, FUSE_RENAME, len, pcn_targ->pcn_cred);
	fri = GET_INPAYLOAD(ps, pm, fuse_rename_in);
	fri->newdir = PERFUSE_NODE_DATA(targ_dir)->pnd_nodeid;
	np = (char *)(void *)(fri + 1);
	(void)strlcpy(np, oldname, oldname_len);
	np += oldname_len;
	(void)strlcpy(np, newname, newname_len);

	if ((error = xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, wait_reply)) != 0)
		goto out;

		
	/*
	 * Record new parent nodeid
	 */
	dstdir_pnd = PERFUSE_NODE_DATA(targ_dir);
	PERFUSE_NODE_DATA(src)->pnd_parent_nodeid = dstdir_pnd->pnd_nodeid;

	if (opc != targ_dir)
		dstdir_pnd->pnd_flags |= PND_DIRTY;

	if (strcmp(newname, "..") != 0)
		(void)strlcpy(PERFUSE_NODE_DATA(src)->pnd_name,
		    newname, MAXPATHLEN);
	else
		PERFUSE_NODE_DATA(src)->pnd_name[0] = 0; /* forget name */

	PERFUSE_NODE_DATA(opc)->pnd_flags |= PND_DIRTY;

	if ((struct puffs_node *)targ != NULL) {
		perfuse_cache_flush(targ);
		PERFUSE_NODE_DATA(targ)->pnd_flags |= PND_REMOVED;
	}

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FILENAME)
		DPRINTF("%s: nodeid = 0x%"PRIx64" file = \"%s\" renamed \"%s\" "
			"nodeid = 0x%"PRIx64" -> nodeid = 0x%"PRIx64" \"%s\"\n",
	 		__func__, PERFUSE_NODE_DATA(src)->pnd_nodeid,
			pcn_src->pcn_name, pcn_targ->pcn_name,
			PERFUSE_NODE_DATA(opc)->pnd_nodeid,
			PERFUSE_NODE_DATA(targ_dir)->pnd_nodeid,
			perfuse_node_path(ps, targ_dir));
#endif

	ps->ps_destroy_msg(pm);
	error = 0;

out:
	node_rele(opc);
	node_rele(src);
	if ((struct puffs_node *)targ != NULL)
		node_rele(targ);

	return error;
}

int
perfuse_node_mkdir(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn,
	const struct vattr *vap)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	struct fuse_mkdir_in *fmi;
	const char *path;
	size_t len;
	int error;
	
	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_REMOVED)
		return ENOENT;

	node_ref(opc);
	ps = puffs_getspecific(pu);
	path = pcn->pcn_name;
	len = sizeof(*fmi) + pcn->pcn_namelen + 1; 

	pm = ps->ps_new_msg(pu, opc, FUSE_MKDIR, len, pcn->pcn_cred);
	fmi = GET_INPAYLOAD(ps, pm, fuse_mkdir_in);
	fmi->mode = vap->va_mode;
	fmi->umask = 0; 	/* Seems unused by libfuse? */
	(void)strlcpy((char *)(void *)(fmi + 1), path, len - sizeof(*fmi));

	error = node_mk_common(pu, opc, pni, pcn, pm);

	node_rele(opc);
	return error;
}


int
perfuse_node_rmdir(struct puffs_usermount *pu, puffs_cookie_t opc,
	puffs_cookie_t targ, const struct puffs_cn *pcn)
{
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd;
	perfuse_msg_t *pm;
	char *path;
	const char *name;
	size_t len;
	int error;
	
	pnd = PERFUSE_NODE_DATA(opc);

	if ((pnd->pnd_flags & PND_REMOVED) ||
	    (PERFUSE_NODE_DATA(targ)->pnd_flags & PND_REMOVED))
		return ENOENT;

	node_ref(opc);
	node_ref(targ);

	/*
	 * Await for all operations on the deleted node to drain, 
	 * as the filesystem may be confused to have it deleted
	 * during a getattr
	 */
	while (PERFUSE_NODE_DATA(targ)->pnd_inxchg)
		requeue_request(pu, targ, PCQ_AFTERXCHG);

	ps = puffs_getspecific(pu);
	name = pcn->pcn_name;
	len = pcn->pcn_namelen + 1;

	pm = ps->ps_new_msg(pu, opc, FUSE_RMDIR, len, pcn->pcn_cred);
	path = _GET_INPAYLOAD(ps, pm, char *);
	(void)strlcpy(path, name, len);

	if ((error = xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, wait_reply)) != 0)
		goto out;

	perfuse_cache_flush(targ);
	PERFUSE_NODE_DATA(targ)->pnd_flags |= PND_REMOVED;

	if (!(PERFUSE_NODE_DATA(targ)->pnd_flags & PND_OPEN))
		puffs_setback(puffs_cc_getcc(pu), PUFFS_SETBACK_NOREF_N2);

	/*
	 * The parent directory needs a sync
	 */
	PERFUSE_NODE_DATA(opc)->pnd_flags |= PND_DIRTY;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FILENAME)
		DPRINTF("%s: remove nodeid = 0x%"PRIx64" file = \"%s\"\n",
			__func__, PERFUSE_NODE_DATA(targ)->pnd_nodeid,
			perfuse_node_path(ps, targ));
#endif
	ps->ps_destroy_msg(pm);
	error = 0;

out:
	node_rele(opc);
	node_rele(targ);
	return error;
}

/* vap is unused */
/* ARGSUSED4 */
int
perfuse_node_symlink(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn_src,
	const struct vattr *vap, const char *link_target)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	char *np;
	const char *path;
	size_t path_len;
	size_t linkname_len;
	size_t len;
	int error;
	
	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_REMOVED)
		return ENOENT;

	node_ref(opc);
	ps = puffs_getspecific(pu);
	path = pcn_src->pcn_name;
	path_len = pcn_src->pcn_namelen + 1;
	linkname_len = strlen(link_target) + 1;
	len = path_len + linkname_len;

	pm = ps->ps_new_msg(pu, opc, FUSE_SYMLINK, len, pcn_src->pcn_cred);
	np = _GET_INPAYLOAD(ps, pm, char *);
	(void)strlcpy(np, path, path_len);
	np += path_len;
	(void)strlcpy(np, link_target, linkname_len);

	error = node_mk_common(pu, opc, pni, pcn_src, pm);

	node_rele(opc);
	return error;
}

/* ARGSUSED4 */
int 
perfuse_node_readdir(struct puffs_usermount *pu, puffs_cookie_t opc,
	struct dirent *dent, off_t *readoff, size_t *reslen,
	const struct puffs_cred *pcr, int *eofflag, off_t *cookies,
	size_t *ncookies)
{
	perfuse_msg_t *pm;
	uint64_t fh;
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd;
	struct fuse_read_in *fri;
	struct fuse_out_header *foh;
	struct fuse_dirent *fd;
	size_t foh_len;
	int error;
	size_t fd_maxlen;
	
	error = 0;
	node_ref(opc);
	ps = puffs_getspecific(pu);

	/*
	 * readdir state is kept at node level, and several readdir
	 * requests can be issued at the same time on the same node.
	 * We need to queue requests so that only one is in readdir
	 * code at the same time.
	 */
	pnd = PERFUSE_NODE_DATA(opc);
	while (pnd->pnd_flags & PND_INREADDIR)
		requeue_request(pu, opc, PCQ_READDIR);
	pnd->pnd_flags |= PND_INREADDIR;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_READDIR)
		DPRINTF("%s: READDIR opc = %p enter critical section\n",
			__func__, (void *)opc);
#endif
	/*
	 * Re-initialize pnd->pnd_fd_cookie on the first readdir for a node
	 */
	if (*readoff == 0)
		pnd->pnd_fd_cookie = 0;

	/*
	 * Do we already have the data bufered?
	 */
	if (pnd->pnd_dirent != NULL)
		goto out;
	pnd->pnd_dirent_len = 0;
	
	/*
	 * It seems NetBSD can call readdir without open first
	 * libfuse will crash if it is done that way, hence open first.
	 */
	if (!(pnd->pnd_flags & PND_OPEN)) {
		if ((error = perfuse_node_open(pu, opc, FREAD, pcr)) != 0)
			goto out;
	}

	fh = perfuse_get_fh(opc, FREAD);

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FH)
		DPRINTF("%s: opc = %p, nodeid = 0x%"PRIx64", "
			"rfh = 0x%"PRIx64"\n", __func__, (void *)opc,
			PERFUSE_NODE_DATA(opc)->pnd_nodeid, fh);
#endif

	pnd->pnd_all_fd = NULL;
	pnd->pnd_all_fd_len = 0;
	fd_maxlen = ps->ps_max_readahead - sizeof(*foh);

	do {
		size_t fd_len;
		char *afdp;
		
		pm = ps->ps_new_msg(pu, opc, FUSE_READDIR, sizeof(*fri), pcr);

		/*
		 * read_flags, lock_owner and flags are unused in libfuse
		 */
		fri = GET_INPAYLOAD(ps, pm, fuse_read_in);
		fri->fh = fh;
		fri->offset = pnd->pnd_fd_cookie;
		fri->size = (uint32_t)fd_maxlen;
		fri->read_flags = 0;
		fri->lock_owner = 0;
		fri->flags = 0;

		if ((error = xchg_msg(pu, opc, pm, 	
				      UNSPEC_REPLY_LEN, wait_reply)) != 0)
			goto out;
		
		/* 
		 * There are many puffs_framebufs calls later,
		 * therefore foh will not be valid for a long time.
		 * Just get the length and forget it.
		 */
		foh = GET_OUTHDR(ps, pm);
		foh_len = foh->len;

		/*
		 * Empty read: we reached the end of the buffer.
		 */
		if (foh_len == sizeof(*foh)) {
			ps->ps_destroy_msg(pm);
			*eofflag = 1;
			break;
		}

		/*
		 * Check for corrupted message.
		 */
		if (foh_len < sizeof(*foh) + sizeof(*fd)) {
			ps->ps_destroy_msg(pm);
			DWARNX("readdir reply too short");
			error = EIO;
			goto out;
		}

		
		fd = GET_OUTPAYLOAD(ps, pm, fuse_dirent);
		fd_len = foh_len - sizeof(*foh);

		pnd->pnd_all_fd = realloc(pnd->pnd_all_fd, 
					  pnd->pnd_all_fd_len + fd_len);
		if (pnd->pnd_all_fd  == NULL)
			DERR(EX_OSERR, "%s: malloc failed", __func__);

		afdp = (char *)(void *)pnd->pnd_all_fd + pnd->pnd_all_fd_len;
		(void)memcpy(afdp, fd, fd_len);

		pnd->pnd_all_fd_len += fd_len;

		/*
		 * The fd->off field is used as a cookie for
		 * resuming the next readdir() where this one was left.
	 	 */
		pnd->pnd_fd_cookie = readdir_last_cookie(fd, fd_len);

		ps->ps_destroy_msg(pm);
	} while (1 /* CONSTCOND */);

	if (pnd->pnd_all_fd != NULL) {
		if (fuse_to_dirent(pu, opc, pnd->pnd_all_fd, 
				   pnd->pnd_all_fd_len) == -1)
			error = EIO;
	}

out:
	if (pnd->pnd_all_fd != NULL) {
		free(pnd->pnd_all_fd);
		pnd->pnd_all_fd = NULL;
		pnd->pnd_all_fd_len = 0;
	}

	if (error == 0)
		readdir_buffered(opc, dent, readoff, reslen);

	/*
	 * Schedule queued readdir requests
	 */
	pnd->pnd_flags &= ~PND_INREADDIR;
	(void)dequeue_requests(opc, PCQ_READDIR, DEQUEUE_ALL);

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_READDIR)
		DPRINTF("%s: READDIR opc = %p exit critical section\n",
			__func__, (void *)opc);
#endif

	node_rele(opc);
	return error;
}

int
perfuse_node_readlink(struct puffs_usermount *pu, puffs_cookie_t opc,
	const struct puffs_cred *pcr, char *linkname, size_t *linklen)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	int error;
	size_t len;
	struct fuse_out_header *foh;
	
	if (PERFUSE_NODE_DATA(opc)->pnd_flags & PND_REMOVED)
		return ENOENT;

	node_ref(opc);
	ps = puffs_getspecific(pu);

	pm = ps->ps_new_msg(pu, opc, FUSE_READLINK, 0, pcr);

	if ((error = xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, wait_reply)) != 0)
		goto out;

	foh = GET_OUTHDR(ps, pm);
	len = foh->len - sizeof(*foh);
	if (len > *linklen)
		DERRX(EX_PROTOCOL, "path len = %zd too long", len);
	if (len == 0)
		DERRX(EX_PROTOCOL, "path len = %zd too short", len);
		
	(void)memcpy(linkname, _GET_OUTPAYLOAD(ps, pm, char *), len);

	/*
	 * FUSE filesystems return a NUL terminated string, we 
	 * do not want the trailing \0
	 */
	while (len > 0 && linkname[len - 1] == '\0')
		len--;

	*linklen = len;

	ps->ps_destroy_msg(pm);
	error = 0;

out:
	node_rele(opc);
	return error;
}

int 
perfuse_node_reclaim(struct puffs_usermount *pu, puffs_cookie_t opc)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	struct perfuse_node_data *pnd;
	struct fuse_forget_in *ffi;
	int nlookup;
	struct timespec now;
	
	if (opc == 0)
		return 0;

	ps = puffs_getspecific(pu);
	pnd = PERFUSE_NODE_DATA(opc);

	/*
	 * Never forget the root.
	 */
	if (pnd->pnd_nodeid == FUSE_ROOT_ID)
		return 0;

	/*
	 * There is a race condition between reclaim and lookup.
	 * When looking up an already known node, the kernel cannot
	 * hold a reference on the result until it gets the PUFFS
	 * reply. It mayy therefore reclaim the node after the 
	 * userland looked it up, and before it gets the reply. 
	 * On rely, the kernel re-creates the node, but at that 
	 * time the node has been reclaimed in userland.
	 *
	 * In order to avoid this, we refuse reclaiming nodes that
	 * are too young since the last lookup - and that we do 
	 * not have removed on our own, of course. 
	 */
	if (clock_gettime(CLOCK_REALTIME, &now) != 0)
		DERR(EX_OSERR, "clock_gettime failed"); 

	if (timespeccmp(&pnd->pnd_cn_expire, &now, >) && 
	    !(pnd->pnd_flags & PND_REMOVED)) {
		if (!(pnd->pnd_flags & PND_NODELEAK)) {
			ps->ps_nodeleakcount++;
			pnd->pnd_flags |= PND_NODELEAK;
		}
		DWARNX("possible leaked node:: opc = %p \"%s\"",
		       opc, pnd->pnd_name);
		return 0;
	}

	node_ref(opc);
	pnd->pnd_flags |= PND_RECLAIMED;
	pnd->pnd_puffs_nlookup--;
	nlookup = pnd->pnd_puffs_nlookup;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_RECLAIM)
		DPRINTF("%s (nodeid %"PRId64") reclaimed\n", 
			perfuse_node_path(ps, opc), pnd->pnd_nodeid);
#endif

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_RECLAIM)
		DPRINTF("%s (nodeid %"PRId64") is %sreclaimed, nlookup = %d "
			"%s%s%s%s, pending ops:%s%s%s\n", 
		        perfuse_node_path(ps, opc), pnd->pnd_nodeid,
		        pnd->pnd_flags & PND_RECLAIMED ? "" : "not ",
			pnd->pnd_puffs_nlookup,
			pnd->pnd_flags & PND_OPEN ? "open " : "not open",
			pnd->pnd_flags & PND_RFH ? "r" : "",
			pnd->pnd_flags & PND_WFH ? "w" : "",
			pnd->pnd_flags & PND_BUSY ? "" : " none",
			pnd->pnd_flags & PND_INREADDIR ? " readdir" : "",
			pnd->pnd_flags & PND_INWRITE ? " write" : "",
			pnd->pnd_flags & PND_INOPEN ? " open" : "");
#endif
	/*
	 * Make sure it is not looked up again 
	 */
	if (!(pnd->pnd_flags & PND_REMOVED))
		perfuse_cache_flush(opc);

	/*
	 * Purge any activity on the node, while checking
	 * that it remains eligible for a reclaim.
	 */
	while (pnd->pnd_ref > 1)
		requeue_request(pu, opc, PCQ_REF);

	/*
	 * reclaim cancel?
	 */
	if (pnd->pnd_puffs_nlookup > nlookup) {
		pnd->pnd_flags &= ~PND_RECLAIMED;
		perfuse_node_cache(ps, opc);
		node_rele(opc);
		return 0;
	}


#ifdef PERFUSE_DEBUG
	if ((pnd->pnd_flags & PND_OPEN) ||
	       !TAILQ_EMPTY(&pnd->pnd_pcq))
		DERRX(EX_SOFTWARE, "%s: opc = %p \"%s\": still open",
		      __func__, opc, pnd->pnd_name);

	if ((pnd->pnd_flags & PND_BUSY) ||
	       !TAILQ_EMPTY(&pnd->pnd_pcq))
		DERRX(EX_SOFTWARE, "%s: opc = %p: queued operations",
		      __func__, opc);

	if (pnd->pnd_inxchg != 0)
		DERRX(EX_SOFTWARE, "%s: opc = %p: ongoing operations",
		      __func__, opc);
#endif

	/*
	 * Send the FORGET message
	 *
	 * ps_new_msg() is called with NULL creds, which will
	 * be interpreted as FUSE superuser. This is obviously
	 * fine since we operate with kernel creds here.
	 */
	pm = ps->ps_new_msg(pu, opc, FUSE_FORGET, 
		      sizeof(*ffi), NULL);
	ffi = GET_INPAYLOAD(ps, pm, fuse_forget_in);
	ffi->nlookup = pnd->pnd_fuse_nlookup;

	/*
	 * No reply is expected, pm is freed in xchg_msg
	 */
	(void)xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, no_reply);

	perfuse_destroy_pn(pu, opc);

	return 0;
}

int 
perfuse_node_inactive(struct puffs_usermount *pu, puffs_cookie_t opc)
{
	struct perfuse_node_data *pnd;
	int error;

	if (opc == 0)
		return 0;

	node_ref(opc);
	pnd = PERFUSE_NODE_DATA(opc);

	if (!(pnd->pnd_flags & (PND_OPEN|PND_REMOVED)))
		goto out;

	/*
	 * Make sure all operation are finished
	 * There can be an ongoing write. Other
	 * operation wait for all data before 
	 * the close/inactive.
	 */
	while (pnd->pnd_flags & PND_INWRITE)
		requeue_request(pu, opc, PCQ_AFTERWRITE);

	/*
	 * The inactive operation may be cancelled,	
	 * If no open is in progress, set PND_INOPEN
	 * so that a new open will be queued.
	 */
	if (pnd->pnd_flags & PND_INOPEN) 
		goto out;

	pnd->pnd_flags |= PND_INOPEN;

	/*
	 * Sync data
	 */
	if (pnd->pnd_flags & PND_DIRTY) {
		if ((error = perfuse_node_fsync(pu, opc, NULL, 0, 0, 0)) != 0)
			DWARN("%s: perfuse_node_fsync failed error = %d",
			      __func__, error);
	}

	
	/*
	 * Close handles
	 */
	if (pnd->pnd_flags & PND_WFH) {
		if ((error = perfuse_node_close_common(pu, opc, FWRITE)) != 0)
			DWARN("%s: close write FH failed error = %d",
			      __func__, error);
	}

	if (pnd->pnd_flags & PND_RFH) {
		if ((error = perfuse_node_close_common(pu, opc, FREAD)) != 0)
			DWARN("%s: close read FH failed error = %d",
			      __func__, error);
	}

	/*
	 * This will cause a reclaim to be sent
	 */
	if (pnd->pnd_flags & PND_REMOVED)
		puffs_setback(puffs_cc_getcc(pu), PUFFS_SETBACK_NOREF_N1);

	/*
	 * Schedule awaiting operations
	 */
	pnd->pnd_flags &= ~PND_INOPEN;
	(void)dequeue_requests(opc, PCQ_OPEN, DEQUEUE_ALL);

	/*
	 * errors are ignored, since the kernel ignores the return code.
	 */
out:
	node_rele(opc);
	return 0;
}


/* ARGSUSED0 */
int 
perfuse_node_print(struct puffs_usermount *pu, puffs_cookie_t opc)
{
	DERRX(EX_SOFTWARE, "%s: UNIMPLEMENTED (FATAL)", __func__);
	return 0;
}

/* ARGSUSED0 */
int
perfuse_node_pathconf(struct puffs_usermount *pu, puffs_cookie_t opc,
	int name, int *retval)
{
	DERRX(EX_SOFTWARE, "%s: UNIMPLEMENTED (FATAL)", __func__);
	return 0;
}

int
perfuse_node_advlock(struct puffs_usermount *pu, puffs_cookie_t opc,
	void *id, int op, struct flock *fl, int flags)
{
	struct perfuse_state *ps;
	int fop;
	perfuse_msg_t *pm;
	uint64_t fh;
	struct fuse_lk_in *fli;
	struct fuse_out_header *foh;
	struct fuse_lk_out *flo;
	uint32_t owner;
	size_t len;
	int error;
	
	node_ref(opc);

	/*
	 * Make sure we do have a filehandle, as the FUSE filesystem
	 * expect one. E.g.: if we provide none, GlusterFS logs an error
	 * "0-glusterfs-fuse: xl is NULL"
	 *
	 * We need the read file handle if the file is open read only,
	 * in order to support shared locks on read-only files.
	 * NB: The kernel always sends advlock for read-only
	 * files at exit time when the process used lock, see
	 * sys_exit -> exit1 -> fd_free -> fd_close -> VOP_ADVLOCK
	 */
	if ((fh = perfuse_get_fh(opc, FREAD)) == FUSE_UNKNOWN_FH) {
		error = EBADF;
		goto out;
	}

	ps = puffs_getspecific(pu);

	if (op == F_GETLK)
		fop = FUSE_GETLK;
	else
		fop = (flags & F_WAIT) ? FUSE_SETLKW : FUSE_SETLK;
			
	/*
	 * XXX ps_new_msg() is called with NULL creds, which will
	 * be interpreted as FUSE superuser. We have no way to 
	 * know the requesting process' credential, but since advlock()
	 * is supposed to operate on a file that has been open(),
	 * permission should have already been checked at open() time.
	 */
	pm = ps->ps_new_msg(pu, opc, fop, sizeof(*fli), NULL);
	fli = GET_INPAYLOAD(ps, pm, fuse_lk_in);
	fli->fh = fh;
	fli->owner = (uint64_t)(vaddr_t)id;
	fli->lk.start = fl->l_start;
	fli->lk.end = fl->l_start + fl->l_len;
	fli->lk.type = fl->l_type;
	fli->lk.pid = fl->l_pid;
	fli->lk_flags = (flags & F_FLOCK) ? FUSE_LK_FLOCK : 0;

	owner = (uint32_t)(vaddr_t)id;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FH)
		DPRINTF("%s: opc = %p, nodeid = 0x%"PRIx64", fh = 0x%"PRIx64"\n",
			__func__, (void *)opc,
			PERFUSE_NODE_DATA(opc)->pnd_nodeid, fli->fh);
#endif

	if ((error = xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, wait_reply)) != 0)
		goto out;

	foh = GET_OUTHDR(ps, pm);
	len = foh->len - sizeof(*foh);

	/*
	 * Save or clear the lock
	 */
	switch (op) {
	case F_GETLK:
		if (len != sizeof(*flo))
			DERRX(EX_SOFTWARE, 
			      "%s: Unexpected lock reply len %zd",
			      __func__, len);

		flo = GET_OUTPAYLOAD(ps, pm, fuse_lk_out);
		fl->l_start = flo->lk.start;
		fl->l_len = flo->lk.end - flo->lk.start;
		fl->l_pid = flo->lk.pid;
		fl->l_type = flo->lk.type;
		fl->l_whence = SEEK_SET;	/* libfuse hardcodes it */

		PERFUSE_NODE_DATA(opc)->pnd_lock_owner = flo->lk.pid;
		break;
	case F_UNLCK:
		owner = 0;
		/* FALLTHROUGH */
	case F_SETLK: 
		/* FALLTHROUGH */
	case F_SETLKW: 
		if (error != 0)
			PERFUSE_NODE_DATA(opc)->pnd_lock_owner = owner;

		if (len != 0)
			DERRX(EX_SOFTWARE, 
			      "%s: Unexpected unlock reply len %zd",
			      __func__, len);

		break;
	default:
		DERRX(EX_SOFTWARE, "%s: Unexpected op %d", __func__, op);
		break;
	}

	ps->ps_destroy_msg(pm);
	error = 0;

out:
	node_rele(opc);
	return error;
}

int
perfuse_node_read(struct puffs_usermount *pu, puffs_cookie_t opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr, int ioflag)
{
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd;
	const struct vattr *vap;
	perfuse_msg_t *pm;
	struct fuse_read_in *fri;
	struct fuse_out_header *foh;
	size_t readen;
	int error;
	
	ps = puffs_getspecific(pu);
	pnd = PERFUSE_NODE_DATA(opc);
	vap = puffs_pn_getvap((struct puffs_node *)opc);

	/*
	 * NetBSD turns that into a getdents(2) output
	 * We just do a EISDIR as this feature is of little use.
	 */
	if (vap->va_type == VDIR)
		return EISDIR;

	do {
		size_t max_read;

		max_read = ps->ps_max_readahead - sizeof(*foh);
		/*
		 * flags may be set to FUSE_READ_LOCKOWNER 
		 * if lock_owner is provided.
		 */
		pm = ps->ps_new_msg(pu, opc, FUSE_READ, sizeof(*fri), pcr);
		fri = GET_INPAYLOAD(ps, pm, fuse_read_in);
		fri->fh = perfuse_get_fh(opc, FREAD);
		fri->offset = offset;
		fri->size = (uint32_t)MIN(*resid, max_read);
		fri->read_flags = 0; /* XXX Unused by libfuse? */
		fri->lock_owner = pnd->pnd_lock_owner;
		fri->flags = 0;
		fri->flags |= (fri->lock_owner != 0) ? FUSE_READ_LOCKOWNER : 0;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_FH)
		DPRINTF("%s: opc = %p, nodeid = 0x%"PRIx64", fh = 0x%"PRIx64"\n",
			__func__, (void *)opc, pnd->pnd_nodeid, fri->fh);
#endif
		error = xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, wait_reply);
		if (error  != 0)
			return error;

		foh = GET_OUTHDR(ps, pm);
		readen = foh->len - sizeof(*foh);

#ifdef PERFUSE_DEBUG
		if (readen > *resid)
			DERRX(EX_SOFTWARE, "%s: Unexpected big read %zd",
			      __func__, readen);
#endif

		(void)memcpy(buf,  _GET_OUTPAYLOAD(ps, pm, char *), readen);

		buf += readen;
		offset += readen;
		*resid -= readen;

		ps->ps_destroy_msg(pm);
	} while ((*resid != 0) && (readen != 0));

	if (ioflag & (IO_SYNC|IO_DSYNC))
		ps->ps_syncreads++;
	else
		ps->ps_asyncreads++;

	return 0;
}

int
perfuse_node_write(struct puffs_usermount *pu, puffs_cookie_t opc,
	uint8_t *buf, off_t offset, size_t *resid, 
	const struct puffs_cred *pcr, int ioflag)
{
	return perfuse_node_write2(pu, opc, buf, offset, resid, pcr, ioflag, 0);
}

/* ARGSUSED7 */
int
perfuse_node_write2(struct puffs_usermount *pu, puffs_cookie_t opc,
	uint8_t *buf, off_t offset, size_t *resid,
	const struct puffs_cred *pcr, int ioflag, int xflag)
{
	struct perfuse_state *ps;
	struct perfuse_node_data *pnd;
	struct vattr *vap;
	perfuse_msg_t *pm;
	struct fuse_write_in *fwi;
	struct fuse_write_out *fwo;
	size_t data_len;
	size_t payload_len;
	size_t written;
	int inresize;
	int error;
	
	ps = puffs_getspecific(pu);
	pnd = PERFUSE_NODE_DATA(opc);
	vap = puffs_pn_getvap((struct puffs_node *)opc);
	written = 0;
	inresize = 0;
	error = 0;

	if (vap->va_type == VDIR) 
		return EISDIR;

	node_ref(opc);

	/*
	 * We need to queue write requests in order to avoid
	 * dequeueing PCQ_AFTERWRITE when there are pending writes.
	 */
	while (pnd->pnd_flags & PND_INWRITE)
		requeue_request(pu, opc, PCQ_WRITE);
	pnd->pnd_flags |= PND_INWRITE;

	/* 
	 * Serialize size access, see comment in perfuse_node_setattr().
	 */
	if ((u_quad_t)offset + *resid > vap->va_size) {
		while (pnd->pnd_flags & PND_INRESIZE)
			requeue_request(pu, opc, PCQ_RESIZE);
		pnd->pnd_flags |= PND_INRESIZE;
		inresize = 1;
	}

	/*
	 * append flag: re-read the file size so that 
	 * we get the latest value.
	 */
	if (ioflag & PUFFS_IO_APPEND) {
		if ((error = perfuse_node_getattr(pu, opc, vap, pcr)) != 0)
			goto out;

		offset = vap->va_size;
	}

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_RESIZE)
		DPRINTF(">> %s %p %" PRIu64 "\n", __func__,
			(void *)opc, vap->va_size);
#endif

	do {
		size_t max_write;
		/*
		 * There is a writepage flag when data
		 * is aligned to page size. Use it for
		 * everything but the data after the last
		 * page boundary.
		 */
		max_write = ps->ps_max_write - sizeof(*fwi); 

		data_len = MIN(*resid, max_write);
		if (data_len > (size_t)sysconf(_SC_PAGESIZE))
			data_len = data_len & ~(sysconf(_SC_PAGESIZE) - 1);

		payload_len = data_len + sizeof(*fwi);

		/*
		 * flags may be set to FUSE_WRITE_CACHE (XXX usage?)
		 * or FUSE_WRITE_LOCKOWNER, if lock_owner is provided.
		 * write_flags is set to 1 for writepage.
		 */
		pm = ps->ps_new_msg(pu, opc, FUSE_WRITE, payload_len, pcr);
		fwi = GET_INPAYLOAD(ps, pm, fuse_write_in);
		fwi->fh = perfuse_get_fh(opc, FWRITE);
		fwi->offset = offset;
		fwi->size = (uint32_t)data_len;
		fwi->write_flags = (fwi->size % sysconf(_SC_PAGESIZE)) ? 0 : 1;
		fwi->lock_owner = pnd->pnd_lock_owner;
		fwi->flags = 0;
		fwi->flags |= (fwi->lock_owner != 0) ? FUSE_WRITE_LOCKOWNER : 0;
		fwi->flags |= (ioflag & IO_DIRECT) ? 0 : FUSE_WRITE_CACHE; 
		(void)memcpy((fwi + 1), buf, data_len);


#ifdef PERFUSE_DEBUG
		if (perfuse_diagflags & PDF_FH)
			DPRINTF("%s: opc = %p, nodeid = 0x%"PRIx64", "
				"fh = 0x%"PRIx64"\n", __func__, 
				(void *)opc, pnd->pnd_nodeid, fwi->fh);
#endif
		if ((error = xchg_msg(pu, opc, pm, 
				      sizeof(*fwo), wait_reply)) != 0)
			goto out;

		fwo = GET_OUTPAYLOAD(ps, pm, fuse_write_out);
		written = fwo->size;
		ps->ps_destroy_msg(pm);

#ifdef PERFUSE_DEBUG
		if (written > *resid)
			DERRX(EX_SOFTWARE, "%s: Unexpected big write %zd",
			      __func__, written);
#endif
		*resid -= written;
		offset += written;
		buf += written;

	} while (*resid != 0);

	/* 
	 * puffs_ops(3) says
	 *  "everything must be written or an error will be generated" 
	 */
	if (*resid != 0)
		error = EFBIG;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_RESIZE) {
		if (offset > (off_t)vap->va_size)
			DPRINTF("<< %s %p %" PRIu64 " -> %lld\n", __func__, 
				(void *)opc, vap->va_size, (long long)offset);
		else
			DPRINTF("<< %s %p \n", __func__, (void *)opc);
	}
#endif

	/*
	 * Update file size if we wrote beyond the end
	 */
	if (offset > (off_t)vap->va_size) 
		vap->va_size = offset;

	if (inresize) {
#ifdef PERFUSE_DEBUG
		if (!(pnd->pnd_flags & PND_INRESIZE))
			DERRX(EX_SOFTWARE, "file write grow without resize");
#endif
		pnd->pnd_flags &= ~PND_INRESIZE;
		(void)dequeue_requests(opc, PCQ_RESIZE, DEQUEUE_ALL);
	}


	/*
	 * Statistics
	 */
	if (ioflag & (IO_SYNC|IO_DSYNC))
		ps->ps_syncwrites++;
	else
		ps->ps_asyncwrites++;

	/*
	 * Remember to sync the file
	 */
	pnd->pnd_flags |= PND_DIRTY;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_SYNC)
		DPRINTF("%s: DIRTY opc = %p, file = \"%s\"\n", 
			__func__, (void*)opc, perfuse_node_path(ps, opc));
#endif

out:
	/*
	 * VOP_PUTPAGE causes FAF write where kernel does not 
	 * check operation result. At least warn if it failed.
	 */
#ifdef PUFFS_WRITE_FAF
	if (error && (xflag & PUFFS_WRITE_FAF))
		DWARN("Data loss caused by FAF write failed on \"%s\"",
		      pnd->pnd_name);
#endif /* PUFFS_WRITE_FAF */

	/*
	 * If there are no more queued write, we can resume
	 * an operation awaiting write completion.
	 */ 
	pnd->pnd_flags &= ~PND_INWRITE;
	if (dequeue_requests(opc, PCQ_WRITE, 1) == 0)
		(void)dequeue_requests(opc, PCQ_AFTERWRITE, DEQUEUE_ALL);

	node_rele(opc);
	return error;
}

/* ARGSUSED0 */
void
perfuse_cache_write(struct puffs_usermount *pu, puffs_cookie_t opc, size_t size,
	struct puffs_cacherun *runs)
{
	return;
}

/* ARGSUSED4 */
int
perfuse_node_getextattr(struct puffs_usermount *pu, puffs_cookie_t opc,
	int attrns, const char *attrname, size_t *attrsize, uint8_t *attr,
	size_t *resid, const struct puffs_cred *pcr)
{
	struct perfuse_state *ps;
	char fuse_attrname[LINUX_XATTR_NAME_MAX + 1];
	perfuse_msg_t *pm;
	struct fuse_getxattr_in *fgi;
	struct fuse_getxattr_out *fgo;
	struct fuse_out_header *foh;
	size_t attrnamelen;
	size_t len;
	char *np;
	int error;

	/* system namespace attrs are not accessible to non root users */
	if (attrns == EXTATTR_NAMESPACE_SYSTEM && !puffs_cred_isjuggernaut(pcr))
		return EPERM;

	node_ref(opc);
	ps = puffs_getspecific(pu);
	attrname = perfuse_native_ns(attrns, attrname, fuse_attrname);
	attrnamelen = strlen(attrname) + 1;
	len = sizeof(*fgi) + attrnamelen;

	pm = ps->ps_new_msg(pu, opc, FUSE_GETXATTR, len, pcr);
	fgi = GET_INPAYLOAD(ps, pm, fuse_getxattr_in);
	fgi->size = (unsigned int)((resid != NULL) ? *resid : 0);
	np = (char *)(void *)(fgi + 1);
	(void)strlcpy(np, attrname, attrnamelen);
	
	if ((error = xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, wait_reply)) != 0)
		goto out;

	/*
	 * We just get fuse_getattr_out with list size if we requested
	 * a null size.
	 */
	if (resid == NULL) {
		fgo = GET_OUTPAYLOAD(ps, pm, fuse_getxattr_out);

		if (attrsize != NULL)
			*attrsize = fgo->size;

		ps->ps_destroy_msg(pm);
		error = 0;
		goto out;
	}

	/*
	 * And with a non null requested size, we get the list just 
	 * after the header
	 */
	foh = GET_OUTHDR(ps, pm);
	np = (char *)(void *)(foh + 1);
	len = foh->len - sizeof(*foh);

	if (attrsize != NULL)
		*attrsize = len;

	if (resid != NULL) {
		if (*resid < len) {
			error = ERANGE;
			ps->ps_destroy_msg(pm);
			goto out;
		}

		(void)memcpy(attr, np, len);
		*resid -= len;
	}
	
	ps->ps_destroy_msg(pm);
	error = 0;

out:
	node_rele(opc);
	return error;
}

int
perfuse_node_setextattr(struct puffs_usermount *pu, puffs_cookie_t opc,
	int attrns, const char *attrname, uint8_t *attr, size_t *resid,
	const struct puffs_cred *pcr)
{
	struct perfuse_state *ps;
	char fuse_attrname[LINUX_XATTR_NAME_MAX + 1];
	perfuse_msg_t *pm;
	struct fuse_setxattr_in *fsi;
	size_t attrnamelen;
	size_t len;
	char *np;
	int error;
	
	/* system namespace attrs are not accessible to non root users */
	if (attrns == EXTATTR_NAMESPACE_SYSTEM && !puffs_cred_isjuggernaut(pcr))
		return EPERM;

	node_ref(opc);
	ps = puffs_getspecific(pu);
	attrname = perfuse_native_ns(attrns, attrname, fuse_attrname);
	attrnamelen = strlen(attrname) + 1;
	len = sizeof(*fsi) + attrnamelen + *resid;

	pm = ps->ps_new_msg(pu, opc, FUSE_SETXATTR, len, pcr);
	fsi = GET_INPAYLOAD(ps, pm, fuse_setxattr_in);
	fsi->size = (unsigned int)*resid;
	fsi->flags = 0;
	np = (char *)(void *)(fsi + 1);
	(void)strlcpy(np, attrname, attrnamelen);
	np += attrnamelen;
	(void)memcpy(np, (char *)attr, *resid);

	if ((error = xchg_msg(pu, opc, pm, 
			      NO_PAYLOAD_REPLY_LEN, wait_reply)) != 0)
		goto out;

	ps->ps_destroy_msg(pm);
	*resid = 0;
	error = 0;

out:
	node_rele(opc);
	return error;
}

/* ARGSUSED2 */
int
perfuse_node_listextattr(struct puffs_usermount *pu, puffs_cookie_t opc,
	int attrns, size_t *attrsize, uint8_t *attrs, size_t *resid, int flag,
	const struct puffs_cred *pcr)
{
	struct perfuse_state *ps;
	perfuse_msg_t *pm;
	struct fuse_getxattr_in *fgi;
	struct fuse_getxattr_out *fgo;
	struct fuse_out_header *foh;
	char *np;
	size_t len, puffs_len, i, attrlen, outlen;
	int error;
	
	/* system namespace attrs are not accessible to non root users */
	if (attrns == EXTATTR_NAMESPACE_SYSTEM && !puffs_cred_isjuggernaut(pcr))
		return EPERM;

	node_ref(opc);

	ps = puffs_getspecific(pu);
	len = sizeof(*fgi);

	pm = ps->ps_new_msg(pu, opc, FUSE_LISTXATTR, len, pcr);
	fgi = GET_INPAYLOAD(ps, pm, fuse_getxattr_in);
	if (resid != NULL)
		fgi->size = (unsigned int)*resid;
	else
		fgi->size = 0;
	
	if ((error = xchg_msg(pu, opc, pm, UNSPEC_REPLY_LEN, wait_reply)) != 0)
		goto out;

	/*
	 * We just get fuse_getattr_out with list size if we requested
	 * a null size.
	 */
	if (resid == NULL) {
		fgo = GET_OUTPAYLOAD(ps, pm, fuse_getxattr_out);

		if (attrsize != NULL)
			*attrsize = fgo->size;

		ps->ps_destroy_msg(pm);

		error = 0;
		goto out;
	}

	/*
	 * And with a non null requested size, we get the list just 
	 * after the header
	 */
	foh = GET_OUTHDR(ps, pm);
	np = (char *)(void *)(foh + 1);
	puffs_len = foh->len - sizeof(*foh);

	if (attrsize != NULL)
		*attrsize = puffs_len;

	if (attrs != NULL) {
		if (*resid < puffs_len) {
			error = ERANGE;
			ps->ps_destroy_msg(pm);
			goto out;
		}

		outlen = 0;
	
		for (i = 0; i < puffs_len; i += attrlen + 1) {
			attrlen = strlen(np + i);

			/*
			 * Filter attributes per namespace
			 */
			if (!perfuse_ns_match(attrns, np + i))
				continue;

#ifdef PUFFS_EXTATTR_LIST_LENPREFIX
			/* 
			 * Convert the FUSE reply to length prefixed strings
			 * if this is what the kernel wants.
			 */
			if (flag & PUFFS_EXTATTR_LIST_LENPREFIX) {
				(void)memcpy(attrs + outlen + 1,
					     np + i, attrlen);
				*(attrs + outlen) = (uint8_t)attrlen;
			} else 
#endif /* PUFFS_EXTATTR_LIST_LENPREFIX */
			(void)memcpy(attrs + outlen, np + i, attrlen + 1);
			outlen += attrlen + 1;
		}	

		*resid -= outlen;
	}

	ps->ps_destroy_msg(pm);
	error = 0;

out:
	node_rele(opc);
	return error;
}

int
perfuse_node_deleteextattr(struct puffs_usermount *pu, puffs_cookie_t opc,
	int attrns, const char *attrname, const struct puffs_cred *pcr)
{
	struct perfuse_state *ps;
	char fuse_attrname[LINUX_XATTR_NAME_MAX + 1];
	perfuse_msg_t *pm;
	size_t attrnamelen;
	char *np;
	int error;
	
	/* system namespace attrs are not accessible to non root users */
	if (attrns == EXTATTR_NAMESPACE_SYSTEM && !puffs_cred_isjuggernaut(pcr))
		return EPERM;

	node_ref(opc);

	ps = puffs_getspecific(pu);
	attrname = perfuse_native_ns(attrns, attrname, fuse_attrname);
	attrnamelen = strlen(attrname) + 1;

	pm = ps->ps_new_msg(pu, opc, FUSE_REMOVEXATTR, attrnamelen, pcr);
	np = _GET_INPAYLOAD(ps, pm, char *);
	(void)strlcpy(np, attrname, attrnamelen);
	
	error = xchg_msg(pu, opc, pm, NO_PAYLOAD_REPLY_LEN, wait_reply);
	
	ps->ps_destroy_msg(pm);

	node_rele(opc);
	return error;
}
