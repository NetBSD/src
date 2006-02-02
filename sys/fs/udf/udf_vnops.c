/* $NetBSD: udf_vnops.c,v 1.2 2006/02/02 15:38:35 reinoud Exp $ */

/*
 * Copyright (c) 2006 Reinoud Zandijk
 * All rights reserved.
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */


#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: udf_vnops.c,v 1.2 2006/02/02 15:38:35 reinoud Exp $");
#endif /* not lint */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>	/* defines plimit structure in proc struct */
#include <sys/kernel.h>
#include <sys/file.h>		/* define FWRITE ... */
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/lockf.h>

#include <miscfs/genfs/genfs.h>
#include <uvm/uvm_extern.h>

#include <fs/udf/ecma167-udf.h>
#include <fs/udf/udf_mount.h>
#include "udf.h"
#include "udf_subr.h"
#include "udf_bswap.h"


#define VTOI(vnode) ((struct udf_node *) vnode->v_data)


/* externs */
extern int prtactive;


/* implementations of vnode functions; table follows at end */
/* --------------------------------------------------------------------- */

int
udf_inactive(void *v)
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	if (prtactive && vp->v_usecount != 0)
		vprint("udf_inactive(): pushing active", vp);

	VOP_UNLOCK(vp, 0);

	DPRINTF(LOCKING, ("udf_inactive called for node %p\n", VTOI(vp)));

	/*
	 * Optionally flush metadata to disc. If the file has not been
	 * referenced anymore in a directory we ought to free up the resources
	 * on disc if applicable.
	 */

	return 0;
}

/* --------------------------------------------------------------------- */

int
udf_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct udf_node *node = VTOI(vp);

	if (prtactive && vp->v_usecount != 0)
		vprint("udf_reclaim(): pushing active", vp);

	/* purge old data from namei */
	cache_purge(vp);

	DPRINTF(LOCKING, ("udf_reclaim called for node %p\n", node));
	/* dispose all node knowledge */
	udf_dispose_node(node);

	return 0;
}

/* --------------------------------------------------------------------- */

int
udf_read(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct vnode *vp     = ap->a_vp;
	struct uio   *uio    = ap->a_uio;
	int           ioflag = ap->a_ioflag;
	struct uvm_object    *uobj;
	struct udf_node      *udf_node = VTOI(vp);
	struct file_entry    *fe;
	struct extfile_entry *efe;
	uint64_t file_size;
	vsize_t len;
	void *win;
	int error;
	int flags;

	/* XXX how to deal with xtended attributes (files) */

	DPRINTF(READ, ("udf_read called\n"));

	/* can this happen? some filingsystems have this check */
	if (uio->uio_offset < 0)
		return EINVAL;

	assert(udf_node);
	assert(udf_node->fe || udf_node->efe);

	/* TODO set access time */

	/* get directory filesize */
	if (udf_node->fe) {
		fe = udf_node->fe;
		file_size = udf_rw64(fe->inf_len);
	} else {
		assert(udf_node->efe);
		efe = udf_node->efe;
		file_size = udf_rw64(efe->inf_len);
	};

	if (vp->v_type == VDIR) {
		/* protect against rogue programs reading raw directories */
		if ((ioflag & IO_ALTSEMANTICS) == 0)
			return EISDIR;
	};
	if (vp->v_type == VREG || vp->v_type == VDIR) {
		const int advice = IO_ADV_DECODE(ap->a_ioflag);

		/* read contents using buffercache */
		uobj = &vp->v_uobj;
		flags = UBC_WANT_UNMAP(vp) ? UBC_UNMAP : 0;
		error = 0;
		while (uio->uio_resid > 0) {
			/* reached end? */
			if (file_size <= uio->uio_offset)
				break;

			/* maximise length to file extremity */
			len = MIN(file_size - uio->uio_offset, uio->uio_resid);
			if (len == 0)
				break;

			/* ubc, here we come, prepare to trap */
			win = ubc_alloc(uobj, uio->uio_offset, &len,
					advice, UBC_READ);
			error = uiomove(win, len, uio);
			ubc_release(win, flags);
			if (error)
				break;
		}
		/* TODO note access time */
		return error;
	};

	return EINVAL;
}

/* --------------------------------------------------------------------- */

int
udf_write(void *v)
{
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;

	DPRINTF(NOTIMPL, ("udf_write called\n"));
	ap = ap;	/* shut up gcc */

	/* TODO implement writing */
	return EROFS;
}


/* --------------------------------------------------------------------- */

/*
 * `Special' bmap functionality that translates all incomming requests to
 * translate to vop_strategy() calls with the same blocknumbers effectively
 * not translating at all.
 */

int
udf_trivial_bmap(void *v)
{
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap = v;
	struct vnode  *vp  = ap->a_vp;	/* our node	*/
	struct vnode **vpp = ap->a_vpp;	/* return node	*/
	daddr_t *bnp  = ap->a_bnp;	/* translated	*/
	daddr_t  bn   = ap->a_bn;	/* origional	*/
	int     *runp = ap->a_runp;
	struct udf_node *udf_node = VTOI(vp);
	uint32_t lb_size;

	/* get logical block size */
	lb_size = udf_rw32(udf_node->ump->logical_vol->lb_size);

	/* could return `-1' to indicate holes/zeros */
	/* translate 1:1 */
	*bnp = bn;

	/* set the vnode to read the data from with strategy on itself */
	if (vpp)
		*vpp = vp;

	/* set runlength of maximum block size */
	if (runp)
		*runp = MAXPHYS / lb_size;	/* or with -1 ? */

	/* return success */
	return 0;
}

/* --------------------------------------------------------------------- */

int
udf_strategy(void *v)
{
	struct vop_strategy_args /* {
		struct vnode *a_vp;
		struct buf *a_bp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct buf   *bp = ap->a_bp;
	struct udf_node *udf_node = VTOI(vp);
	uint32_t lb_size, from, sectors;
	int error;

	DPRINTF(STRATEGY, ("udf_strategy called\n"));

	/* check if we ought to be here */
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		panic("udf_strategy: spec");

	/* only filebuffers ought to be read by this, no descriptors */
	assert(bp->b_blkno >= 0);

	/* get sector size */
	lb_size = udf_rw32(udf_node->ump->logical_vol->lb_size);

	/* calculate sector to start from */
	from = bp->b_blkno;

	/* calculate length to fetch/store in sectors */
	sectors = bp->b_bcount / lb_size;
	assert(bp->b_bcount > 0);

	/* NEVER assume later that this buffer is allready translated */
	/* bp->b_lblkno = bp->b_blkno; */

	DPRINTF(STRATEGY, ("\tread vp %p buf %p (blk no %"PRIu64")"
	    ", sector %d for %d sectors\n",
	    vp, bp, bp->b_blkno, from, sectors));

	/* check assertions: we OUGHT to allways get multiples of this */
	assert(sectors * lb_size == bp->b_bcount);

	/* determine mode */
	error = 0;
	if (bp->b_flags & B_READ) {
		/* read buffer from the udf_node, translate vtop on the way*/
		udf_read_filebuf(udf_node, bp);
		return bp->b_error;
	};

	printf("udf_strategy: can't write yet\n");
	return ENOTSUP;
}

/* --------------------------------------------------------------------- */

int
udf_readdir(void *v)
{
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
		int *a_eofflag;
		off_t **a_cookies;
		int *a_ncookies;
	} */ *ap = v;
	struct uio *uio = ap->a_uio;
	struct vnode *vp = ap->a_vp;
	struct udf_node *udf_node = VTOI(vp);
	struct file_entry    *fe;
	struct extfile_entry *efe;
	struct fileid_desc *fid;
	struct dirent dirent;
	uint64_t file_size, diroffset, transoffset;
	uint32_t lb_size;
	int error;

	DPRINTF(READDIR, ("udf_readdir called\n"));

	/* This operation only makes sense on directory nodes. */
	if (vp->v_type != VDIR)
		return ENOTDIR;

	/* get directory filesize */
	if (udf_node->fe) {
		fe = udf_node->fe;
		file_size = udf_rw64(fe->inf_len);
	} else {
		assert(udf_node->efe);
		efe = udf_node->efe;
		file_size = udf_rw64(efe->inf_len);
	};

	/*
	 * Add `.' pseudo entry if at offset zero since its not in the fid
	 * stream
	 */
	if (uio->uio_offset == 0) {
		DPRINTF(READDIR, ("\t'.' inserted\n"));
		memset(&dirent, 0, sizeof(struct dirent));
		strcpy(dirent.d_name, ".");
		dirent.d_fileno = udf_calchash(&udf_node->loc);
		dirent.d_type = DT_DIR;
		dirent.d_namlen = strlen(dirent.d_name);
		dirent.d_reclen = _DIRENT_SIZE(&dirent);
		uiomove(&dirent, _DIRENT_SIZE(&dirent), uio);

		/* mark with magic value that we have done the dummy */
		uio->uio_offset = UDF_DIRCOOKIE_DOT;
	};

	/* we are called just as long as we keep on pushing data in */
	error = 0;
	if ((uio->uio_offset < file_size) &&
	    (uio->uio_resid >= sizeof(struct dirent))) {
		/* allocate temporary space for fid */
		lb_size = udf_rw32(udf_node->ump->logical_vol->lb_size);
		fid = malloc(lb_size, M_TEMP, M_WAITOK);

		if (uio->uio_offset == UDF_DIRCOOKIE_DOT)
			uio->uio_offset = 0;

		diroffset   = uio->uio_offset;
		transoffset = diroffset;
		while (diroffset < file_size) {
			DPRINTF(READDIR, ("\tread in fid stream\n"));
			/* transfer a new fid/dirent */
			error = udf_read_fid_stream(vp, &diroffset,
				    fid, &dirent);
			DPRINTFIF(READDIR, error, ("read error in read fid "
			    "stream : %d\n", error));
			if (error)
				break;

			/* 
			 * If there isn't enough space in the uio to return a
			 * whole dirent, break off read
			 */
			if (uio->uio_resid < _DIRENT_SIZE(&dirent))
				break;

			/* remember the last entry we transfered */
			transoffset = diroffset;

			/* skip deleted entries */
			if (fid->file_char & UDF_FILE_CHAR_DEL)
				continue;

			/* skip not visible files */
			if (fid->file_char & UDF_FILE_CHAR_VIS)
				continue;

			/* copy dirent to the caller */
			DPRINTF(READDIR, ("\tread dirent `%s', type %d\n",
			    dirent.d_name, dirent.d_type));
			uiomove(&dirent, _DIRENT_SIZE(&dirent), uio);
		};

		/* pass on last transfered offset */
		uio->uio_offset = transoffset;
		free(fid, M_TEMP);
	};

	if (ap->a_eofflag)
		*ap->a_eofflag = (uio->uio_offset == file_size);

#ifdef DEBUG
	if (udf_verbose & UDF_DEBUG_READDIR) {
		printf("returning offset %d\n", (uint32_t) uio->uio_offset);
		if (ap->a_eofflag)
			printf("returning EOF ? %d\n", *ap->a_eofflag);
		if (error)
			printf("readdir returning error %d\n", error);
	};
#endif

	return error;
}

/* --------------------------------------------------------------------- */

int
udf_lookup(void *v)
{
	struct vop_lookup_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	struct udf_node  *dir_node, *res_node;
	struct udf_mount *ump;
	struct long_ad    icb_loc;
	const char *name;
	int namelen, nameiop, islastcn, lock_parent, mounted_ro;
	int vnodetp;
	int error, found;

	dir_node = VTOI(dvp);
	ump = dir_node->ump;
	*vpp = NULL;

	DPRINTF(LOOKUP, ("udf_lookup called\n"));

	/* simplify/clarification flags */
	nameiop     = cnp->cn_nameiop;
	islastcn    = cnp->cn_flags & ISLASTCN;
	lock_parent = cnp->cn_flags & LOCKPARENT;
	mounted_ro  = dvp->v_mount->mnt_flag & MNT_RDONLY;

	/* check exec/dirread permissions first */
	error = VOP_ACCESS(dvp, VEXEC, cnp->cn_cred, cnp->cn_lwp);
	if (error)
		return error;

	DPRINTF(LOOKUP, ("\taccess ok\n"));

	/*
	 * If requesting a modify on the last path element on a read-only
	 * filingsystem, reject lookup; XXX why is this repeated in every FS ?
	 */
	if (islastcn && mounted_ro && (nameiop == DELETE || nameiop == RENAME))
		return EROFS;

	DPRINTF(LOOKUP, ("\tlooking up cnp->cn_nameptr '%s'\n",
	    cnp->cn_nameptr));
	/* look in the nami cache; returns 0 on success!! */
	error = cache_lookup(dvp, vpp, cnp);
	if (error >= 0)
		return error;

	DPRINTF(LOOKUP, ("\tNOT found in cache\n"));

	/*
	 * Obviously, the file is not (anymore) in the namecache, we have to
	 * search for it. There are three basic cases: '.', '..' and others.
	 * 
	 * Following the guidelines of VOP_LOOKUP manpage and tmpfs.
	 */
	error = 0;
	if ((cnp->cn_namelen == 1) && (cnp->cn_nameptr[0] == '.')) {
		DPRINTF(LOOKUP, ("\tlookup '.'\n"));
		/* special case 1 '.' */
		VREF(dvp);
		*vpp = dvp;
		/* done */
	} else if (cnp->cn_flags & ISDOTDOT) {
		/* special case 2 '..' */
		DPRINTF(LOOKUP, ("\tlookup '..'\n"));

		/* first unlock parent */
		VOP_UNLOCK(dvp, 0);
		cnp->cn_flags |= PDIRUNLOCK;

		/* get our node */
		name    = "..";
		namelen = 2;
		found = udf_lookup_name_in_dir(dvp, name, namelen, &icb_loc);
		if (!found)
			error = ENOENT;

		if (!error) {
			DPRINTF(LOOKUP, ("\tfound '..'\n"));
			/* try to create/reuse the node */
			error = udf_get_node(ump, &icb_loc, &res_node);

			if (!error) {
			DPRINTF(LOOKUP, ("\tnode retrieved/created OK\n"));
				*vpp = res_node->vnode;
			};
		};

		/* see if we're requested to lock the parent */
		if (lock_parent && islastcn) {
			if (vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY) == 0)
				cnp->cn_flags &= ~PDIRUNLOCK;
		};
		/* done */
	} else {
		DPRINTF(LOOKUP, ("\tlookup file\n"));
		/* all other files */
		/* lookup filename in the directory; location icb_loc */
		name    = cnp->cn_nameptr;
		namelen = cnp->cn_namelen;
		found = udf_lookup_name_in_dir(dvp, name, namelen, &icb_loc);
		if (!found) {
			DPRINTF(LOOKUP, ("\tNOT found\n"));
			/*
			 * UGH, didn't find name. If we're creating or naming
			 * on the last name this is OK and we ought to return
			 * EJUSTRETURN if its allowed to be created.
			 */
			error = ENOENT;
			if (islastcn &&
				(nameiop == CREATE || nameiop == RENAME))
					error = 0;
			if (!error) {
				error = VOP_ACCESS(dvp, VWRITE, cnp->cn_cred,
						cnp->cn_lwp);
				if (!error) {
					/* keep the component name */
					cnp->cn_flags |= SAVENAME;
					error = EJUSTRETURN;
				};
			};
			/* done */
		} else {
			/* try to create/reuse the node */
			error = udf_get_node(ump, &icb_loc, &res_node);
			if (!error) {
				/*
				 * If we are not at the last path component
				 * and found a non-directory or non-link entry
				 * (which may itself be pointing to a
				 * directory), raise an error.
				 */
				vnodetp = res_node->vnode->v_type;
				if ((vnodetp != VDIR) && (vnodetp != VLNK)) {
					if (!islastcn)
						error = ENOTDIR;
				}

			};
			if (!error) {
				/*
				 * If LOCKPARENT or ISLASTCN is not set, dvp
				 * is returned unlocked on a successful
				 * lookup.
				 */
				*vpp = res_node->vnode;
				if (!lock_parent || !islastcn)
					VOP_UNLOCK(dvp, 0);
			};
			/* done */
		};
		/* done */
	}	

	/*
	 * Store result in the cache if requested. If we are creating a file,
	 * the file might not be found and thus putting it into the namecache
	 * might be seen as negative caching.
	 */
	if (cnp->cn_flags & MAKEENTRY)
		cache_enter(dvp, *vpp, cnp);

	DPRINTFIF(LOOKUP, error, ("udf_lookup returing error %d\n", error));

	return error;
}

/* --------------------------------------------------------------------- */

int
udf_getattr(void *v)
{
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct udf_node *udf_node = VTOI(vp);
	struct udf_mount *ump = udf_node->ump;
	struct vattr *vap = ap->a_vap;
	struct file_entry *fe;
	struct extfile_entry *efe;
	struct timestamp *atime, *mtime, *attrtime;
	uint32_t nlink;
	uint64_t filesize, blkssize;
	uid_t uid;
	gid_t gid;

	DPRINTF(CALL, ("udf_getattr called\n"));

	/* get descriptor information */
	if (udf_node->fe) {
		fe = udf_node->fe;
		nlink    = udf_rw16(fe->link_cnt);
		uid      = (uid_t)udf_rw32(fe->uid);
		gid      = (gid_t)udf_rw32(fe->gid);
		filesize = udf_rw64(fe->inf_len);
		blkssize = udf_rw64(fe->logblks_rec);
		atime    = &fe->atime;
		mtime    = &fe->mtime;
		attrtime = &fe->attrtime;
	} else {
		assert(udf_node->efe);
		efe = udf_node->efe;
		nlink    = udf_rw16(efe->link_cnt);
		uid      = (uid_t)udf_rw32(efe->uid);
		gid      = (gid_t)udf_rw32(efe->gid);
		filesize = udf_rw64(efe->inf_len);	/* XXX or obj_size? */
		blkssize = udf_rw64(efe->logblks_rec);
		atime    = &efe->atime;
		mtime    = &efe->mtime;
		attrtime = &efe->attrtime;
	};

	/* do the uid/gid translation game */
	if ((uid == (uid_t) -1) && (gid == (gid_t) -1)) {
		uid = ump->mount_args.anon_uid;
		gid = ump->mount_args.anon_gid;
	};
	
	/* fill in struct vattr with values from the node */
	VATTR_NULL(vap);
	vap->va_type      = vp->v_type;
	vap->va_mode      = udf_getaccessmode(udf_node);
	vap->va_nlink     = nlink;
	vap->va_uid       = uid;
	vap->va_gid       = gid;
	vap->va_fsid      = vp->v_mount->mnt_stat.f_fsidx.__fsid_val[0];
	vap->va_fileid    = udf_calchash(&udf_node->loc);   /* inode hash XXX */
	vap->va_size      = filesize;
	vap->va_blocksize = udf_node->ump->discinfo.sector_size;  /* wise? */

	/*
	 * BUG-ALERT: UDF doesn't count '.' as an entry, so we'll have to add
	 * 1 to the link count if its a directory we're requested attributes
	 * of.
	 */
	if (vap->va_type == VDIR)
		vap->va_nlink++;

	/* access times */
	udf_timestamp_to_timespec(ump, atime,    &vap->va_atime);
	udf_timestamp_to_timespec(ump, mtime,    &vap->va_mtime);
	udf_timestamp_to_timespec(ump, attrtime, &vap->va_ctime);

	vap->va_gen       = 1;		/* no multiple generations yes (!?) */
	vap->va_flags     = 0;		/* no flags */
	vap->va_rdev      = udf_node->rdev;
	vap->va_bytes     = udf_node->ump->discinfo.sector_size * blkssize;
	vap->va_filerev   = 1;		/* TODO file revision numbers? */
	vap->va_vaflags   = 0;		/* TODO which va_vaflags? */

	return 0;
}

/* --------------------------------------------------------------------- */

int
udf_setattr(void *v)
{
	struct vop_setattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct udf_node  *udf_node = VTOI(vp);
	struct udf_mount *ump = udf_node->ump;
	struct vattr *vap = ap->a_vap;
	uid_t uid, nobody_uid;
	gid_t gid, nobody_gid;

	/* shut up gcc for now */
	ap = ap;
	vp = vp;
	uid = uid;
	gid = gid;
	udf_node = udf_node;

	DPRINTF(NOTIMPL, ("udf_setattr called\n"));

	/*
	 * BUG-ALERT: UDF doesn't count '.' as an entry, so we'll have to
	 * subtract 1 to the link count if its a directory we're setting
	 * attributes on. See getattr.
	 */
	if (vap->va_type == VDIR)
		vap->va_nlink--;

	/* do the uid/gid translation game */
	nobody_uid = ump->mount_args.nobody_uid;
	nobody_gid = ump->mount_args.nobody_gid;
	if ((uid == nobody_uid) && (gid == nobody_gid)) {
		uid = (uid_t) -1;
		gid = (gid_t) -1;
	};
	
	/* TODO implement setattr!! NOT IMPLEMENTED yet */
	return 0;
}

/* --------------------------------------------------------------------- */

/*
 * Return POSIX pathconf information for UDF file systems.
 */
int
udf_pathconf(void *v)
{
	struct vop_pathconf_args /* {
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct udf_node *udf_node = VTOI(vp);
	uint32_t bits;

	DPRINTF(CALL, ("udf_pathconf called\n"));

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = (1<<16)-1;	/* 16 bits */
		return 0;
	case _PC_NAME_MAX:
		*ap->a_retval = NAME_MAX;
		return 0;
	case _PC_PATH_MAX:
		*ap->a_retval = PATH_MAX;
		return 0;
	case _PC_PIPE_BUF:
		*ap->a_retval = PIPE_BUF;
		return 0;
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return 0;
	case _PC_NO_TRUNC:
		*ap->a_retval = 1;
		return 0;
	case _PC_SYNC_IO:
		*ap->a_retval = 0;     /* synchronised is off for performance */
		return 0;
	case _PC_FILESIZEBITS:
		bits = 32;
		if (udf_node)
			bits = 32 * vp->v_mount->mnt_dev_bshift;
		*ap->a_retval = bits;
		return 0;
	};

	return EINVAL;
}


/* --------------------------------------------------------------------- */

int
udf_open(void *v)
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;

	DPRINTF(CALL, ("udf_open called\n"));
	ap = ap;		/* shut up gcc */

	return 0;
}


/* --------------------------------------------------------------------- */

int
udf_close(void *v)
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct udf_node *udf_node = VTOI(vp);

	DPRINTF(CALL, ("udf_close called\n"));
	udf_node = udf_node;	/* shut up gcc */

	simple_lock(&vp->v_interlock);
		if (vp->v_usecount > 1) {
			/* TODO update times */
		};
	simple_unlock(&vp->v_interlock);

	return 0;
}


/* --------------------------------------------------------------------- */

int
udf_access(void *v)
{
	struct vop_access_args /* {
		struct vnode *a_vp;
		int a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode    *vp   = ap->a_vp;
	mode_t	         mode = ap->a_mode;
	struct ucred    *cred = ap->a_cred;
	struct udf_node *udf_node = VTOI(vp);
	struct file_entry    *fe;
	struct extfile_entry *efe;
	mode_t   node_mode;
	uid_t uid;
	gid_t gid;

	DPRINTF(CALL, ("udf_access called\n"));

	/* get access mode to compare to */
	node_mode = udf_getaccessmode(udf_node);

	/* get uid/gid pair */
	if (udf_node->fe) {
		fe = udf_node->fe;
		uid = (uid_t)udf_rw32(fe->uid);
		gid = (gid_t)udf_rw32(fe->gid);
	} else {
		assert(udf_node->efe);
		efe = udf_node->efe;
		uid = (uid_t)udf_rw32(efe->uid);
		gid = (gid_t)udf_rw32(efe->gid);
	};

	/* check if we are allowed to write */
	switch (vp->v_type) {
	case VDIR:
	case VLNK:
	case VREG:
		/*
		 * normal nodes: check if we're on a read-only mounted
		 * filingsystem and bomb out if we're trying to write.
		 */
		if ((mode & VWRITE) && (vp->v_mount->mnt_flag & MNT_RDONLY))
			return EROFS;
		break;
	case VBLK:
	case VCHR:
	case VSOCK:
	case VFIFO:
		/*
		 * special nodes: even on read-only mounted filingsystems
		 * these are allowed to be written to if permissions allow.
		 */
		break;
	default:
		/* no idea what this is */
		return EINVAL;
	}

	/* TODO support for chflags checking i.e. IMMUTABLE flag */

	/* ask the generic vaccess to advice on security */
	return vaccess(vp->v_type, node_mode, uid, gid, mode, cred);
}

/* --------------------------------------------------------------------- */

int
udf_create(void *v)
{
	struct vop_create_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	struct componentname *cnp = ap->a_cnp;

	DPRINTF(NOTIMPL, ("udf_create called\n"));

	/* error out */
	PNBUF_PUT(cnp->cn_pnbuf);
	vput(ap->a_dvp);
	return ENOTSUP;
}

/* --------------------------------------------------------------------- */

int
udf_mknod(void *v)
{
	struct vop_mknod_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;

	DPRINTF(NOTIMPL, ("udf_mknod called\n"));

	/* error out */
	PNBUF_PUT(ap->a_cnp->cn_pnbuf);
	vput(ap->a_dvp);
	return ENOTSUP;
}

/* --------------------------------------------------------------------- */

int
udf_remove(void *v)
{
	struct vop_remove_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode *vp  = ap->a_vp;

	DPRINTF(NOTIMPL, ("udf_remove called\n"));

	/* error out */
	vput(dvp);
	vput(vp);

	return ENOTSUP;
}

/* --------------------------------------------------------------------- */

int
udf_link(void *v)
{
	struct vop_link_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode *vp  = ap->a_vp;
	struct componentname *cnp = ap->a_cnp;

	DPRINTF(NOTIMPL, ("udf_link called\n"));

	/* error out */
	/* XXX or just VOP_ABORTOP(dvp, a_cnp); ? */
	if (VOP_ISLOCKED(vp))
		VOP_UNLOCK(vp, 0);
	PNBUF_PUT(cnp->cn_pnbuf);
	vput(dvp);

	return ENOTSUP;
}

/* --------------------------------------------------------------------- */

int
udf_symlink(void *v)
{
	struct vop_symlink_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	
	DPRINTF(NOTIMPL, ("udf_symlink called\n"));

	/* error out */
	VOP_ABORTOP(dvp, cnp);
	vput(dvp);

	return ENOTSUP;
}

/* --------------------------------------------------------------------- */

int
udf_readlink(void *v)
{
	struct vop_readlink_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
	} */ *ap = v;
#ifdef notyet
	struct vnode *vp = ap->a_vp;
	struct uio *uio = ap->a_uio;
	struct ucred *cred = ap->a_cred;
#endif

	ap = ap;	/* shut up gcc */

	DPRINTF(NOTIMPL, ("udf_readlink called\n"));

	/* TODO read `file' contents and process pathcomponents into a path */
	return ENOTSUP;
}

/* --------------------------------------------------------------------- */

int
udf_rename(void *v)
{
	struct vop_rename_args /* {
		struct vnode *a_fdvp;
		struct vnode *a_fvp;
		struct componentname *a_fcnp;
		struct vnode *a_tdvp;
		struct vnode *a_tvp;
		struct componentname *a_tcnp;
	} */ *ap = v;
	struct vnode *tvp = ap->a_tvp;
	struct vnode *tdvp = ap->a_tdvp;
	struct vnode *fvp = ap->a_fvp;
	struct vnode *fdvp = ap->a_fdvp;

	DPRINTF(NOTIMPL, ("udf_rename called\n"));

	/* error out */
	if (tdvp == tvp)
		vrele(tdvp);
	else
		vput(tdvp);
	if (tvp != NULL)
		vput(tvp);

	/* release source nodes. */
	vrele(fdvp);
	vrele(fvp);

	return ENOTSUP;
}

/* --------------------------------------------------------------------- */

int
udf_mkdir(void *v)
{
	struct vop_mkdir_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;

	DPRINTF(NOTIMPL, ("udf_mkdir called\n"));

	/* error out */
	PNBUF_PUT(cnp->cn_pnbuf);
	vput(dvp);

	return ENOTSUP;
}

/* --------------------------------------------------------------------- */

int
udf_rmdir(void *v)
{
	struct vop_rmdir_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	int error;

	cnp = cnp;	/* shut up gcc */

	DPRINTF(NOTIMPL, ("udf_rmdir called\n"));

	error = ENOTSUP;
	/* error out */
	if (error != 0) {
		vput(dvp);
		vput(vp);
	}

	return error;
}

/* --------------------------------------------------------------------- */

int
udf_fsync(void *v)
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_flags;
		off_t offlo;
		off_t offhi;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	DPRINTF(NOTIMPL, ("udf_fsync called\n"));

	vp = vp;	/* shut up gcc */

	return 0;
}

/* --------------------------------------------------------------------- */

int
udf_advlock(void *v)
{
	struct vop_advlock_args /* {
		struct vnode *a_vp;
		void *a_id;
		int a_op;
		struct flock *a_fl;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct udf_node *udf_node = VTOI(vp);
	struct file_entry    *fe;
	struct extfile_entry *efe;
	uint64_t file_size;

	DPRINTF(LOCKING, ("udf_advlock called\n"));

	/* get directory filesize */
	if (udf_node->fe) {
		fe = udf_node->fe;
		file_size = udf_rw64(fe->inf_len);
	} else {
		assert(udf_node->efe);
		efe = udf_node->efe;
		file_size = udf_rw64(efe->inf_len);
	};

	return lf_advlock(ap, &udf_node->lockf, file_size);
}

/* --------------------------------------------------------------------- */


/* Global vfs vnode data structures for udfs */
int (**udf_vnodeop_p) __P((void *));

const struct vnodeopv_entry_desc udf_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, udf_lookup },	/* lookup */
	{ &vop_create_desc, udf_create },	/* create */	/* TODO */
	{ &vop_mknod_desc, udf_mknod },		/* mknod */	/* TODO */
	{ &vop_open_desc, udf_open },		/* open */
	{ &vop_close_desc, udf_close },		/* close */
	{ &vop_access_desc, udf_access },	/* access */
	{ &vop_getattr_desc, udf_getattr },	/* getattr */
	{ &vop_setattr_desc, udf_setattr },	/* setattr */	/* TODO */
	{ &vop_read_desc, udf_read },		/* read */
	{ &vop_write_desc, udf_write },		/* write */	/* WRITE */
	{ &vop_lease_desc, genfs_lease_check },	/* lease */	/* TODO? */
	{ &vop_fcntl_desc, genfs_fcntl },	/* fcntl */	/* TODO? */
	{ &vop_ioctl_desc, genfs_enoioctl },	/* ioctl */	/* TODO? */
	{ &vop_poll_desc, genfs_poll },		/* poll */	/* TODO/OK? */
	{ &vop_kqfilter_desc, genfs_kqfilter },	/* kqfilter */	/* ? */
	{ &vop_revoke_desc, genfs_revoke },	/* revoke */	/* TODO? */
	{ &vop_mmap_desc, genfs_mmap },		/* mmap */	/* OK? */
	{ &vop_fsync_desc, udf_fsync },		/* fsync */	/* TODO */
	{ &vop_seek_desc, genfs_seek },		/* seek */
	{ &vop_remove_desc, udf_remove },	/* remove */ 	/* TODO */
	{ &vop_link_desc, udf_link },		/* link */	/* TODO */
	{ &vop_rename_desc, udf_rename },	/* rename */ 	/* TODO */
	{ &vop_mkdir_desc, udf_mkdir },		/* mkdir */ 	/* TODO */
	{ &vop_rmdir_desc, udf_rmdir },		/* rmdir */	/* TODO */
	{ &vop_symlink_desc, udf_symlink },	/* symlink */	/* TODO */
	{ &vop_readdir_desc, udf_readdir },	/* readdir */
	{ &vop_readlink_desc, udf_readlink },	/* readlink */	/* TEST ME */
	{ &vop_abortop_desc, genfs_abortop },	/* abortop */	/* TODO/OK? */
	{ &vop_inactive_desc, udf_inactive },	/* inactive */
	{ &vop_reclaim_desc, udf_reclaim },	/* reclaim */
	{ &vop_lock_desc, genfs_lock },		/* lock */
	{ &vop_unlock_desc, genfs_unlock },	/* unlock */
	{ &vop_bmap_desc, udf_trivial_bmap },	/* bmap */	/* 1:1 bmap */
	{ &vop_strategy_desc, udf_strategy },	/* strategy */
/*	{ &vop_print_desc, udf_print },	*/	/* print */
	{ &vop_islocked_desc, genfs_islocked },	/* islocked */
	{ &vop_pathconf_desc, udf_pathconf },	/* pathconf */
	{ &vop_advlock_desc, udf_advlock },	/* advlock */	/* TEST ME */
	{ &vop_bwrite_desc, vn_bwrite },	/* bwrite */	/* ->strategy */
	{ &vop_getpages_desc, genfs_getpages },	/* getpages */
	{ &vop_putpages_desc, genfs_putpages },	/* putpages */
	{ NULL, NULL }
};


const struct vnodeopv_desc udf_vnodeop_opv_desc = {
	&udf_vnodeop_p, udf_vnodeop_entries
};

