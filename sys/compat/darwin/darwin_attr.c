/*	$NetBSD: darwin_attr.c,v 1.3 2004/05/02 12:32:22 pk Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_attr.c,v 1.3 2004/05/02 12:32:22 pk Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/lwp.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/stat.h>
#include <sys/syscallargs.h>

#include <compat/common/compat_util.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_attr.h>
#include <compat/darwin/darwin_syscallargs.h>

#define DARWIN_ATTR_MAXBUFLEN	4096

static int darwin_attr_append(const char *, size_t, char **, size_t *);

#define ATTR_APPEND(x, bp, len) \
    darwin_attr_append((char *)&(x), sizeof(x), &(bp), &(len))


static int
darwin_attr_append(x, size, bp, len) 
	const char *x;
	size_t size;
	char **bp;
	size_t *len;
{
	if (*len < size)
		return -1;

	(void)memcpy(*bp, x, size);

	*bp += size;
	*len -= size;

	return 0;
}

int
darwin_sys_getattrlist(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_getattrlist_args /* {
		syscallarg(const char *) path;
		syscallarg(struct darwin_attrlist *) alist;
		syscallarg(void *) attributes;
		syscallarg(size_t) buflen;
		syscallarg(unsigned long) options;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct darwin_attrlist kalist;
	char *buf;
	char *bp;
	size_t len;
	size_t shift = 0;
	int null = 0;
	int error = 0;
	int follow = 0;
	u_long *whole_len_p = NULL;
	darwin_attrreference_t *cmn_name_p = NULL;
	darwin_attrreference_t *vol_mountpoint_p = NULL;
	darwin_attrreference_t *vol_name_p = NULL;
	darwin_attrreference_t *vol_mounteddevice_p = NULL;
	struct sys___stat13_args cup1;
	struct stat *ust;
	struct stat st;
	struct compat_20_sys_statfs_args cup2;
	struct statfs12 *uf;
	struct statfs12 f;
	struct nameidata nd;
	struct vnode *vp;
	struct ucred *cred;
	const char *path;
	caddr_t sg = stackgap_init(p, 0);
	int fl;

	if ((error = copyin(SCARG(uap, alist), &kalist, sizeof(kalist))) != 0)
		return error;

	if (kalist.bitmapcount != DARWIN_ATTR_BIT_MAP_COUNT)
		return EINVAL;

	len = SCARG(uap, buflen);
	if (len > DARWIN_ATTR_MAXBUFLEN)
		return E2BIG;

	if ((SCARG(uap, options) & DARWIN_FSOPT_NOFOLLOW) != 0)
		follow = FOLLOW;

#ifdef DEBUG_DARWIN
	printf("getattrlist: %08x %08x %08x %08x %08x\n", 
	    kalist.commonattr, kalist.volattr, kalist.dirattr,
	    kalist.fileattr, kalist.forkattr);
#endif

	/*
	 * Lookup emulation shadow tree once
	 */
	fl = CHECK_ALT_FL_EXISTS;
	if (follow == 0)
		fl |= CHECK_ALT_FL_SYMLINK;
	(void)emul_find(p, &sg, p->p_emul->e_path, SCARG(uap, path), &path, fl);

	/* 
	 * Get the informations for path: file related info
	 */ 
	ust = stackgap_alloc(p, &sg, sizeof(st));
	SCARG(&cup1, path) = path;
	SCARG(&cup1, ub) = ust;
	if (follow) {
		if ((error = sys___stat13(l, &cup1, retval)) != 0)
			return error;
	} else {
		if ((error = sys___lstat13(l, &cup1, retval)) != 0)
			return error;
	}

	if ((error = copyin(ust, &st, sizeof(st))) != 0)
		return error;

	/*
	 * filesystem related info
	 */
	uf = stackgap_alloc(p, &sg, sizeof(f));
	SCARG(&cup2, path) = path;
	SCARG(&cup2, buf) = uf;
	if ((error = compat_20_sys_statfs(l, &cup2, retval)) != 0)
	 	return error;

	if ((error = copyin(uf, &f, sizeof(f))) != 0)
		return error;

	/* 
	 * Prepare the buffer 
	 */
	buf = malloc(len, M_TEMP, M_WAITOK);
	bp = buf;

	/*
	 * vnode structure
	 */

	cred = crdup(p->p_ucred);
	cred->cr_uid = p->p_cred->p_ruid;
	cred->cr_gid = p->p_cred->p_rgid;

	NDINIT(&nd, LOOKUP, follow | LOCKLEAF, UIO_USERSPACE, path, p);
	if ((error = namei(&nd)) != 0) 
		goto out2;	

	vp = nd.ni_vp;
	if ((error = VOP_ACCESS(vp, VREAD | VEXEC, cred, p)) != 0)
		goto out3;

	/* 
	 * Buffer whole length: is always present
	 */
	if (1) {
		u_long whole_len;

		whole_len = 0;
		whole_len_p = (u_long *)bp;
		if (ATTR_APPEND(whole_len, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_NAME) {
		darwin_attrreference_t dar;

		cmn_name_p = (darwin_attrreference_t *)bp;
		if (ATTR_APPEND(dar, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_DEVID) {
		dev_t device;

		device = st.st_dev;
		if (ATTR_APPEND(device, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_FSID) {
		fsid_t fs;
		fs = f.f_fsid;
		if (ATTR_APPEND(fs, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_OBJTYPE) {
		darwin_fsobj_type_t dft;

		dft = vp->v_type;
		if (ATTR_APPEND(dft, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_OBJTAG) {
		darwin_fsobj_tag_t dft;

		dft = vp->v_tag;
		if (ATTR_APPEND(dft, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_OBJID) {
		darwin_fsobj_id_t dfi;

		dfi.fid_objno = st.st_ino;
		dfi.fid_generation = 0; /* XXX root can read real value */
		if (ATTR_APPEND(dfi, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_OBJPERMANENTID) {
		darwin_fsobj_id_t dfi;

		dfi.fid_objno = st.st_ino; /* This is not really persistent */
		dfi.fid_generation = 0; /* XXX root can read real value */
		if (ATTR_APPEND(dfi, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_PAROBJID) {
		darwin_fsobj_id_t dfi;

		dfi.fid_objno = 0; 	/* XXX do me */
		dfi.fid_generation = 0; /* XXX root can read real value */
		if (ATTR_APPEND(dfi, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_SCRIPT) {
		darwin_text_encoding_t dte;

		dte = DARWIN_US_ASCII;
		if (ATTR_APPEND(dte, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_CRTIME) {
		if (ATTR_APPEND(st.st_ctimespec, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_MODTIME) {
		if (ATTR_APPEND(st.st_mtimespec, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_CHGTIME) {
		if (ATTR_APPEND(st.st_ctimespec, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_ACCTIME) {
		if (ATTR_APPEND(st.st_atimespec, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_BKUPTIME) {
		struct timespec ts;

		/* XXX no way I can do that one */

		(void)bzero(&ts, sizeof(ts));
		if (ATTR_APPEND(ts, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_FNDRINFO) { /* XXX */
		char data[32];

		(void)bzero(&data, sizeof(data));
		if (ATTR_APPEND(data, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_OWNERID) {
		uid_t uid;

		uid = st.st_uid;
		if (ATTR_APPEND(uid, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_GRPID) {
		gid_t gid;

		gid = st.st_gid;
		if (ATTR_APPEND(gid, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_ACCESSMASK) {
		mode_t mode;

		mode = st.st_mode;
		if (ATTR_APPEND(mode, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_NAMEDATTRCOUNT) { 
		/* Data is unsigned long. Unsupported in Darwin */
		error = EINVAL;
		goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_NAMEDATTRLIST) {
		/* Data is darwin_attrreference_t. Unsupported in Darwin */

		error = EINVAL;
		goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_FLAGS) {
		unsigned long flags;

		flags = st.st_flags;	/* XXX need convertion */
		if (ATTR_APPEND(flags, bp, len) != 0)
			goto out3;
	}

	if (kalist.commonattr & DARWIN_ATTR_CMN_USERACCESS) {
		unsigned long ua = 0;
		struct sys_access_args cup3;
		register_t rv;

		SCARG(&cup3, path) = SCARG(uap, path);

		SCARG(&cup3, flags) = R_OK;
		if (sys_access(l, &cup3, &rv) == 0)
			ua |= R_OK;
			
		SCARG(&cup3, flags) = W_OK;
		if (sys_access(l, &cup3, &rv) == 0)
			ua |= W_OK;

		SCARG(&cup3, flags) = X_OK;
		if (sys_access(l, &cup3, &rv) == 0)
			ua |= X_OK;

		if (ATTR_APPEND(ua, bp, len) != 0)
			goto out3;
	}


	if (kalist.volattr & DARWIN_ATTR_VOL_INFO) {
		/* Nothing added, just skip */
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_FSTYPE) {
		unsigned long fstype;

		fstype = f.f_type;
		if (ATTR_APPEND(fstype, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_SIGNATURE) {
		unsigned long sign;

		/* 
		 * XXX Volume signature, used to distinguish
		 * between volumes inside the same filesystem. 
		 */
		sign = f.f_fsid.__fsid_val[0];
		if (ATTR_APPEND(sign, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_SIZE) {
		off_t size;

		size = f.f_blocks * f.f_bsize;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_SPACEFREE) {
		off_t free;

		free = f.f_bfree * f.f_bsize;
		if (ATTR_APPEND(free, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_SPACEAVAIL) {
		off_t avail;

		avail = f.f_bavail * f.f_bsize;
		if (ATTR_APPEND(avail, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_MINALLOCATION) {
		off_t min;

		min = f.f_bsize; /* XXX proably wrong */
		if (ATTR_APPEND(min, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_ALLOCATIONCLUMP) {
		off_t clump;

		clump = f.f_bsize; /* XXX proably wrong */
		if (ATTR_APPEND(clump, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_IOBLOCKSIZE) {
		unsigned long size;

		size = f.f_iosize;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_OBJCOUNT) {
		unsigned long cnt;

		cnt = f.f_files;
		if (ATTR_APPEND(cnt, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_FILECOUNT) {
		unsigned long cnt;

		cnt = f.f_files; /* XXX only files */
		if (ATTR_APPEND(cnt, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_DIRCOUNT) {
		unsigned long cnt;

		cnt = 0; /* XXX wrong, of course */
		if (ATTR_APPEND(cnt, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_MAXOBJCOUNT) {
		unsigned long cnt;

		cnt = f.f_files + f.f_ffree;
		if (ATTR_APPEND(cnt, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_MOUNTPOINT) {
		darwin_attrreference_t dar;

		vol_mountpoint_p = (darwin_attrreference_t *)bp;
		if (ATTR_APPEND(dar, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_NAME) {
		darwin_attrreference_t dar;

		vol_name_p = (darwin_attrreference_t *)bp;
		if (ATTR_APPEND(dar, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_MOUNTFLAGS) {
		unsigned long flags;

		flags = f.f_flags; /* XXX need convertion? */
		if (ATTR_APPEND(flags, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_MOUNTEDDEVICE) {
		darwin_attrreference_t dar;

		vol_mounteddevice_p = (darwin_attrreference_t *)bp;
		if (ATTR_APPEND(dar, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_ENCODINGSUSED) {
		unsigned long long data;

		/* 
		 * XXX bitmap of encoding used in this volume 
		 */
		(void)bzero(&data, sizeof(data));
		if (ATTR_APPEND(data, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_CAPABILITIES) { /* XXX */
		darwin_vol_capabilities_attr_t data;

		(void)bzero(&data, sizeof(data));
		if (ATTR_APPEND(data, bp, len) != 0)
			goto out3;
	}

	if (kalist.volattr & DARWIN_ATTR_VOL_ATTRIBUTES) { /* XXX */
		darwin_vol_attributes_attr_t data;

		(void)bzero(&data, sizeof(data));
		if (ATTR_APPEND(data, bp, len) != 0)
			goto out3;
	}

	if (kalist.dirattr & DARWIN_ATTR_DIR_LINKCOUNT) {
		unsigned long cnt;

		cnt = st.st_nlink;
		if (ATTR_APPEND(cnt, bp, len) != 0)
			goto out3;
	}

	if (kalist.dirattr & DARWIN_ATTR_DIR_ENTRYCOUNT) { /* XXX */
		unsigned long data;

		(void)bzero(&data, sizeof(data));
		if (ATTR_APPEND(data, bp, len) != 0)
			goto out3;
	}

	if (kalist.dirattr & DARWIN_ATTR_DIR_MOUNTSTATUS) { /* XXX */
		unsigned long data;

		(void)bzero(&data, sizeof(data));
		if (ATTR_APPEND(data, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_LINKCOUNT) {
		unsigned long cnt;

		cnt = st.st_nlink;
		if (ATTR_APPEND(cnt, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_TOTALSIZE) {
		off_t size;

		size = st.st_size;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_ALLOCSIZE) {
		off_t size;

		size = st.st_blocks * f.f_bsize;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_IOBLOCKSIZE) {
		unsigned long size;

		size = st.st_blksize;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_CLUMPSIZE) {
		unsigned long size;

		size = st.st_blksize; /* XXX probably wrong */
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_DEVTYPE) {
		unsigned long type;

		type = st.st_rdev;
		if (ATTR_APPEND(type, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_FILETYPE) {
		unsigned long data;

		/* Reserved, returns 0 */
		(void)bzero(&data, sizeof(data));
		if (ATTR_APPEND(data, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_FORKCOUNT) { /* XXX */
		unsigned long cnt;

		cnt = 1; /* Only one fork, of course */
		if (ATTR_APPEND(cnt, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_FORKLIST) {
		/* Unsupported in Darwin */
		error = EINVAL;
		goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_DATALENGTH) { /* All forks */
		off_t size;

		size = st.st_size;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_DATAALLOCSIZE) { /* All forks */
		off_t size;

		size = st.st_blocks * f.f_bsize;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_DATAEXTENTS) { 
		darwin_extentrecord data;

		/* Obsolete in Darwin */
		(void)bzero(&data, sizeof(data));
		if (ATTR_APPEND(data, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_RSRCLENGTH) {
		off_t size;

		size = 0;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_RSRCALLOCSIZE) {
		off_t size;

		size = 0;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.fileattr & DARWIN_ATTR_FILE_RSRCEXTENTS) {
		darwin_extentrecord data;

		/* Obsolete in Darwin */
		(void)bzero(&data, sizeof(data));
		if (ATTR_APPEND(data, bp, len) != 0)
			goto out3;
	}

	if (kalist.forkattr & DARWIN_ATTR_FORK_TOTALSIZE) {
		off_t size;

		size = st.st_size;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	if (kalist.forkattr & DARWIN_ATTR_FORK_ALLOCSIZE) {
		off_t size;

		size = st.st_blocks * f.f_bsize;
		if (ATTR_APPEND(size, bp, len) != 0)
			goto out3;
	}

	/* 
	 * Now the variable length fields 
	 */
	
	if (cmn_name_p) {		/* DARWIN_ATTR_CMN_NAME */
		cmn_name_p->attr_dataoffset = (u_long)bp - (u_long)cmn_name_p;
		cmn_name_p->attr_length = nd.ni_cnd.cn_namelen;
		if (darwin_attr_append(nd.ni_cnd.cn_nameptr, 
		    cmn_name_p->attr_length, &bp, &len) != 0)
			goto out3;

		/* word alignement */
		shift = (((u_long)bp & ~0x3UL) + 4) - (u_long)bp;
		if (darwin_attr_append((char *)null, shift, &bp, &len) != 0)
			goto out3;
	}

	if (vol_mountpoint_p) {		/* DARWIN_ATTR_VOL_MOUNTPOINT */
		vol_mountpoint_p->attr_dataoffset = 
		    (u_long)bp - (u_long)vol_mountpoint_p;
		vol_mountpoint_p->attr_length = strlen(f.f_mntonname);
		if (darwin_attr_append(f.f_mntonname, 
		    vol_mountpoint_p->attr_length, &bp, &len) != 0)
			goto out3;

		/* word alignement */
		shift = (((u_long)bp & ~0x3UL) + 4) - (u_long)bp;
		if (darwin_attr_append((char *)null, shift, &bp, &len) != 0)
			goto out3;
	}

	if (vol_mounteddevice_p) {	/* DARWIN_ATTR_VOL_MOUNTEDDEVICE */
		vol_mounteddevice_p->attr_dataoffset = 
		    (u_long)bp - (u_long)vol_mounteddevice_p;
		vol_mounteddevice_p->attr_length = strlen(f.f_mntfromname);
		if (darwin_attr_append(f.f_mntfromname,
		    vol_mounteddevice_p->attr_length, &bp, &len) != 0)
			goto out3;

		/* word alignement */
		shift = (((u_long)bp & ~0x3UL) + 4) - (u_long)bp;
		if (darwin_attr_append((char *)null, shift, &bp, &len) != 0)
			goto out3;
	}

	if (vol_name_p) {		/* DARWIN_ATTR_VOL_NAME */
		char name[] = "Volume"; /* XXX We have no volume names... */
		size_t namelen = strlen(name);

		vol_name_p->attr_dataoffset = (u_long)bp - (u_long)vol_name_p;
		vol_name_p->attr_length = namelen;
		if (darwin_attr_append(name, namelen, &bp, &len) != 0)
			goto out3;

		/* word alignement */
		shift = (((u_long)bp & ~0x3UL) + 4) - (u_long)bp;
		if (darwin_attr_append((char *)null, shift, &bp, &len) != 0)
			goto out3;
	}

	/* And, finnally, the whole buffer length */
	if (whole_len_p) {
		*whole_len_p = SCARG(uap, buflen) - len;
	}

	/*
	 * We are done! Copyout the stuff and get away
	 */
	if (error == 0)
		error = copyout(buf, SCARG(uap, attributes), *whole_len_p);
out3:
	vput(vp);
out2:
	crfree(cred);
	free(buf, M_TEMP);

	return error;
}
