/*	$NetBSD: hfsp.h,v 1.1.1.1 2007/03/05 23:01:06 dillo Exp $	*/

/*-
 * Copyright (c) 2005, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yevgeny Binder and Dieter Baron.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */                                     
 
#ifndef _FS_HFSP_HFSP_H_
#define _FS_HFSP_HFSP_H_

#include <sys/vnode.h>
#include <sys/mount.h>

#include <miscfs/genfs/genfs_node.h>

/* XXX remove before release */
/*#define HFSP_DEBUG*/

#ifdef HFSP_DEBUG
	#if defined(_KERNEL) && !defined(_LKM)
		#include "opt_ddb.h"
	#endif /* defined(_KERNEL_) && !defined(_LKM) */
#endif /* HFSP_DEBUG */

#include <fs/hfsp/libhfsp.h>

/* XXX: make these mount options */
#define HFSP_DEFAULT_UID	0
#define HFSP_DEFAULT_GID	0
#define HFSP_DEFAULT_DIR_MODE	0755
#define HFSP_DEFAULT_FILE_MODE	0755

MALLOC_DECLARE(M_HFSPMNT);	/* defined in hfsp_vfsops.c */

struct hfsp_args {
	char *fspec;		/* block special device to mount */
	uint64_t offset; /*number of bytes to start of volume from start of device*/
};

struct hfspmount {
	struct mount *hm_mountp;	/* filesystem vfs structure */
	dev_t hm_dev;				/* device mounted */
	struct vnode *hm_devvp;		/* block device mounted vnode */
	hfsp_volume hm_vol;			/* essential volume information */
	uint64_t offset;		/* number of device bloks to start of volume from start of device*/
};

struct hfspnode {
	struct genfs_node h_gnode;
	LIST_ENTRY(hfspnode) h_hash;/* hash chain */
	struct vnode *h_vnode;		/* vnode associated with this hnode */
	struct hfspmount *h_hmp;	/* mount point associated with this hnode */
	struct vnode *h_devvp;		/* vnode for block I/O */
	dev_t	h_dev;				/* device associated with this hnode */

	union {
		hfsp_file_record_t		file;
		hfsp_folder_record_t	folder;
		struct {
			int16_t			rec_type;
			uint16_t		flags;
			uint32_t		valence;
			hfsp_cnid_t		cnid;
		}; /* convenience for accessing common record info */
	} h_rec; /* catalog record for this hnode */
	
	/*
	 * We cache this vnode's parent CNID here upon vnode creation (i.e., during
	 * hfsp_vop_vget()) for quick access without needing to search the catalog.
	 * Note, however, that this value must also be updated whenever this file
	 * is moved.
	 */
	hfsp_cnid_t		h_parent;

	uint8_t h_fork;

	long	dummy;	/* FOR DEVELOPMENT ONLY */
};

typedef struct {
	uint64_t offset; /* offset to start of volume from start of device */
	struct vnode* devvp; /* vnode for device I/O */
	size_t devblksz; /* device block size (NOT HFS+ allocation block size)*/
} hfsp_libcb_data; /* custom data used in hfsp_volume.cbdata */

typedef struct {
	kauth_cred_t cred;
	struct lwp *l;
	struct vnode *devvp;
} hfsp_libcb_argsopen;

typedef struct {
	struct lwp *l;
} hfsp_libcb_argsclose;

typedef struct {
	kauth_cred_t cred;
	struct lwp *l;
} hfsp_libcb_argsread;

/*
 * Convenience macros
 */

/* Convert mount ptr to hfspmount ptr. */
#define VFSTOHFSP(mp)    ((struct hfspmount *)((mp)->mnt_data))

/* Convert between vnode ptrs and hfsnode ptrs. */
#define VTOH(vp)    ((struct hfspnode *)(vp)->v_data)
#define	HTOV(hp)	((hp)->h_vnode)

/* Get volume's allocation block size given a vnode ptr */
#define HFSP_BLOCKSIZE(vp)    (VTOH(vp)->h_hmp->hm_vol.vh.block_size)


/* Convert special device major/minor */
#define HFSP_CONVERT_RDEV(x)	makedev((x)>>24, (x)&0xffffff)

/*
 * Global variables
 */

extern const struct vnodeopv_desc hfsp_vnodeop_opv_desc;
extern const struct vnodeopv_desc hfsp_specop_opv_desc;
extern const struct vnodeopv_desc hfsp_fifoop_opv_desc;
extern int (**hfsp_specop_p) (void *);
extern int (**hfsp_fifoop_p) (void *);


/*
 * Function prototypes
 */

/* hfsp_nhash.c */
void hfsp_nhashinit (void);
void hfsp_nhashdone (void);
struct vnode *hfsp_nhashget (dev_t, hfsp_cnid_t, uint8_t, int);
void hfsp_nhashinsert (struct hfspnode *);
void hfsp_nhashremove (struct hfspnode *);

/* hfsp_subr.c */
void hfsp_vinit (struct mount *, int (**)(void *), int (**)(void *),
		 struct vnode **);
int hfsp_pread(struct vnode*, void*, size_t, uint64_t, uint64_t, kauth_cred_t);
char* hfsp_unicode_to_ascii(const unichar_t*, uint8_t, char*);
unichar_t* hfsp_ascii_to_unicode(const char*, uint8_t, unichar_t*);

void hfsp_time_to_timespec(uint32_t, struct timespec *);
enum vtype hfsp_catalog_keyed_record_vtype(const hfsp_catalog_keyed_record_t *);

void hfsp_libcb_error(const char*, const char*, int, va_list);
void* hfsp_libcb_malloc(size_t, hfsp_callback_args*);
void* hfsp_libcb_realloc(void*, size_t, hfsp_callback_args*);
void hfsp_libcb_free(void*, hfsp_callback_args*);
int hfsp_libcb_opendev(hfsp_volume*, const char*, uint64_t,hfsp_callback_args*);
void hfsp_libcb_closedev(hfsp_volume*, hfsp_callback_args*);
int hfsp_libcb_read(hfsp_volume*, void*, uint64_t, uint64_t,
	hfsp_callback_args*);

uint16_t be16tohp(void**);
uint32_t be32tohp(void**);
uint64_t be64tohp(void**);


/* hfsp_vfsops.c */
int hfsp_mount (struct mount *, const char *, void *, struct nameidata *,
	struct lwp *);
int hfsp_mountfs (struct vnode *, struct mount *, struct lwp *,
	const char *, uint64_t);
int hfsp_start (struct mount *, int, struct lwp *);
int hfsp_unmount (struct mount *, int, struct lwp *);
int hfsp_root (struct mount *, struct vnode **);
int hfsp_quotactl (struct mount *, int, uid_t, void *, struct lwp *);
int hfsp_statvfs (struct mount *, struct statvfs *, struct lwp *);
int hfsp_sync (struct mount *, int, kauth_cred_t , struct lwp *);
int hfsp_vget (struct mount *, ino_t, struct vnode **);
int hfsp_vget_internal(struct mount *, ino_t, uint8_t, struct vnode **);
int hfsp_fhtovp (struct mount *, struct fid *, struct vnode **);
int hfsp_vptofh (struct vnode *, struct fid *, size_t *);
void hfsp_init (void);
void hfsp_reinit (void);
void hfsp_done (void);
int hfsp_mountroot (void);
int hfsp_extattrctl (struct mount *, int, struct vnode *, int, const char *,
		     struct lwp *);

/* hfsp_vnops.c */
extern int (**hfsp_vnodeop_p) (void *);

#endif /* !_FS_HFSP_HFSP_H_ */
