/*
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 * All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *	$Id: kernfs.h,v 1.4 1993/05/28 16:47:02 cgd Exp $
 */

#ifdef KERNEL
struct kernfs_mount {
	struct vnode	*kf_root;	/* Root node */
};

struct kernfs_node {
	struct kernfs_target *kf_kt;
};

#define KSTRING 256		/* Largest I/O available via this filesystem */
#define UIO_MX 32

struct kernfs_target {
	char *kt_name;
	void *kt_data;
#define KTT_NULL 1
#define KTT_TIME 5
#define KTT_INT 17
#define KTT_STRING 31
#define KTT_HOSTNAME 47
#define KTT_AVENRUN 53
	int kt_tag;
#define KTM_RO_PERMS		(S_IRUSR|S_IRGRP|S_IROTH)
#define KTM_RW_PERMS		(S_IWUSR|S_IRUSR|S_IRGRP|S_IWGRP| \
				 S_IROTH|S_IWOTH)
#define KTM_DIR_PERMS		(S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP| \
				 S_IROTH|S_IXOTH)
#define KTM_MASK_PERMS		(~(S_IWGRP|S_IWOTH))
	u_short kt_maxperms;
	int	kt_vtype;
	u_short kt_perms;
	uid_t	kt_uid;
	gid_t	kt_gid;
};

#define DIR_TARGET(name, data, tag, perms) \
{ name,	data,	tag,	/*max*/perms,	VDIR,	perms & KTM_MASK_PERMS,	0, 0 },
#define REG_TARGET(name, data, tag, perms) \
{ name,	data,	tag,	/*max*/perms,	VREG,	perms & KTM_MASK_PERMS,	0, 0 },
#define BLK_TARGET(name, data, tag, perms) \
{ name,	data,	tag,	/*max*/perms,	VBLK,	perms & KTM_MASK_PERMS,	0, 0 },
#define CHR_TARGET(name, data, tag, perms) \
{ name,	data,	tag,	/*max*/perms,	VCHR,	perms & KTM_MASK_PERMS,	0, 0 },


#define VFSTOKERNFS(mp)	((struct kernfs_mount *)((mp)->mnt_data))
#define	VTOKERN(vp) ((struct kernfs_node *)(vp)->v_data)

extern struct vnodeops kernfs_vnodeops;
extern struct vfsops kernfs_vfsops;

extern struct vnode *rrootdevvp;

struct kernfs_target kernfs_targets[];
#define KERNFS_TARGET_ROOT	0

#endif /* KERNEL */
