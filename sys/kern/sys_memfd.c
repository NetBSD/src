/*	$NetBSD: sys_memfd.c,v 1.2 2023/07/10 15:49:18 christos Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Theodore Preduta.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_memfd.c,v 1.2 2023/07/10 15:49:18 christos Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/mman.h>
#include <sys/miscfd.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_object.h>

#define F_SEAL_ANY_WRITE	(F_SEAL_WRITE|F_SEAL_FUTURE_WRITE)
#define MFD_KNOWN_SEALS		(F_SEAL_SEAL|F_SEAL_SHRINK|F_SEAL_GROW \
				|F_SEAL_WRITE|F_SEAL_FUTURE_WRITE)

static const char memfd_prefix[] = "memfd:";

static int memfd_read(file_t *, off_t *, struct uio *, kauth_cred_t, int);
static int memfd_write(file_t *, off_t *, struct uio *, kauth_cred_t, int);
static int memfd_ioctl(file_t *, u_long, void *);
static int memfd_fcntl(file_t *, u_int, void *);
static int memfd_stat(file_t *, struct stat *);
static int memfd_close(file_t *);
static int memfd_mmap(file_t *, off_t *, size_t, int, int *, int *,
    struct uvm_object **, int *);
static int memfd_seek(file_t *, off_t, int, off_t *, int);
static int memfd_truncate(file_t *, off_t);

static const struct fileops memfd_fileops = {
	.fo_name = "memfd",
	.fo_read = memfd_read,
	.fo_write = memfd_write,
	.fo_ioctl = memfd_ioctl,
	.fo_fcntl = memfd_fcntl,
	.fo_poll = fnullop_poll,
	.fo_stat = memfd_stat,
	.fo_close = memfd_close,
	.fo_kqfilter = fnullop_kqfilter,
	.fo_restart = fnullop_restart,
	.fo_mmap = memfd_mmap,
	.fo_seek = memfd_seek,
	.fo_fpathconf = (void *)eopnotsupp,
	.fo_posix_fadvise = (void *)eopnotsupp,
	.fo_truncate = memfd_truncate,
};

/*
 * memfd_create(2).  Creat a file descriptor associated with anonymous
 * memory.
 */
int
sys_memfd_create(struct lwp *l, const struct sys_memfd_create_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const char *) name;
		syscallarg(unsigned int) flags;
	} */
	int error, fd;
	file_t *fp;
	struct memfd *mfd;
	struct proc *p = l->l_proc;
	const unsigned int flags = SCARG(uap, flags);

	KASSERT(NAME_MAX - sizeof(memfd_prefix) > 0); /* sanity check */

	if (flags & ~(MFD_CLOEXEC|MFD_ALLOW_SEALING))
		return EINVAL;

	mfd = kmem_zalloc(sizeof(*mfd), KM_SLEEP);
	mfd->mfd_size = 0;
	mfd->mfd_uobj = uao_create(INT64_MAX - PAGE_SIZE, 0); /* same as tmpfs */
	mutex_init(&mfd->mfd_lock, MUTEX_DEFAULT, IPL_NONE);

	strcpy(mfd->mfd_name, memfd_prefix);
	error = copyinstr(SCARG(uap, name),
	    &mfd->mfd_name[sizeof(memfd_prefix) - 1],
	    sizeof(mfd->mfd_name) - sizeof(memfd_prefix), NULL);
	if (error != 0)
 		goto leave;

	getnanotime(&mfd->mfd_btime);

	if ((flags & MFD_ALLOW_SEALING) == 0)
		mfd->mfd_seals |= F_SEAL_SEAL;

	error = fd_allocfile(&fp, &fd);
	if (error != 0)
		goto leave;

	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_MEMFD;
	fp->f_ops = &memfd_fileops;
	fp->f_memfd = mfd;
	fd_set_exclose(l, fd, (flags & MFD_CLOEXEC) != 0);
	fd_affix(p, fp, fd);

	*retval = fd;
	return 0;

leave:
	uao_detach(mfd->mfd_uobj);
	kmem_free(mfd, sizeof(*mfd));
	return error;
}

static int
memfd_read(file_t *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	int error;
	vsize_t todo;
	struct memfd *mfd = fp->f_memfd;

	if (offp == &fp->f_offset)
		mutex_enter(&fp->f_lock);

	if (*offp < 0) {
		error = EINVAL;
		goto leave;
	}

	/* Trying to read past the end does nothing. */
	if (*offp >= mfd->mfd_size) {
		error = 0;
		goto leave;
	}

	uio->uio_offset = *offp;
	todo = MIN(uio->uio_resid, mfd->mfd_size - *offp);
	error = ubc_uiomove(mfd->mfd_uobj, uio, todo, UVM_ADV_SEQUENTIAL,
	    UBC_READ|UBC_PARTIALOK);

leave:
	if (offp == &fp->f_offset)
		mutex_exit(&fp->f_lock);

	getnanotime(&mfd->mfd_atime);

	return error;
}

static int
memfd_write(file_t *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	int error;
	vsize_t todo;
	struct memfd *mfd = fp->f_memfd;

	if (mfd->mfd_seals & F_SEAL_ANY_WRITE)
		return EPERM;

	if (offp == &fp->f_offset)
		mutex_enter(&fp->f_lock);

	if (*offp < 0) {
		error = EINVAL;
		goto leave;
	}

	uio->uio_offset = *offp;
	todo = uio->uio_resid;

	if (mfd->mfd_seals & F_SEAL_GROW) {
		if (*offp >= mfd->mfd_size) {
			error = EPERM;
			goto leave;
		}

		/* Truncate the write to fit in mfd_size */
		if (*offp + uio->uio_resid >= mfd->mfd_size)
			todo = mfd->mfd_size - *offp;
	} else if (*offp + uio->uio_resid >= mfd->mfd_size) {
		/* Grow to accommodate the write request. */
		error = memfd_truncate(fp, *offp + uio->uio_resid);
		if (error != 0)
			goto leave;
	}

	error = ubc_uiomove(mfd->mfd_uobj, uio, todo, UVM_ADV_SEQUENTIAL,
	    UBC_WRITE|UBC_PARTIALOK);

	getnanotime(&mfd->mfd_mtime);

leave:
	if (offp == &fp->f_offset)
		mutex_exit(&fp->f_lock);

	return error;
}

static int
memfd_ioctl(file_t *fp, u_long cmd, void *data)
{

	return EINVAL;
}

static int
memfd_fcntl(file_t *fp, u_int cmd, void *data)
{
	struct memfd *mfd = fp->f_memfd;

	switch (cmd) {
	case F_ADD_SEALS:
		if (mfd->mfd_seals & F_SEAL_SEAL)
			return EPERM;

		if (*(int *)data & ~MFD_KNOWN_SEALS)
		        return EINVAL;

		/*
		 * Can only add F_SEAL_WRITE if there are no currently
		 * open mmaps.
		 *
		 * XXX should only disallow if there are no currently
		 * open mmaps with PROT_WRITE.
		 */
		if ((mfd->mfd_seals & F_SEAL_WRITE) == 0 &&
		    (*(int *)data & F_SEAL_WRITE) != 0 &&
		    mfd->mfd_uobj->uo_refs > 1)
			return EBUSY;

		mfd->mfd_seals |= *(int *)data;
		return 0;

	case F_GET_SEALS:
		*(int *)data = mfd->mfd_seals;
		return 0;

	default:
		return EINVAL;
	}
}

static int
memfd_stat(file_t *fp, struct stat *st)
{
	struct memfd *mfd = fp->f_memfd;

	memset(st, 0, sizeof(*st));
	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);
	st->st_size = mfd->mfd_size;

	st->st_mode = S_IREAD;
	if ((mfd->mfd_seals & F_SEAL_ANY_WRITE) == 0)
		st->st_mode |= S_IWRITE;

	st->st_birthtimespec = mfd->mfd_btime;
	st->st_ctimespec = mfd->mfd_mtime;
	st->st_atimespec = mfd->mfd_atime;
	st->st_mtimespec = mfd->mfd_mtime;

	return 0;
}

static int
memfd_close(file_t *fp)
{
	struct memfd *mfd = fp->f_memfd;

	uao_detach(mfd->mfd_uobj);
	mutex_destroy(&mfd->mfd_lock);

	kmem_free(mfd, sizeof(*mfd));
	fp->f_memfd = NULL;

	return 0;
}

static int
memfd_mmap(file_t *fp, off_t *offp, size_t size, int prot, int *flagsp,
    int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	struct memfd *mfd = fp->f_memfd;

	/* uvm_mmap guarantees page-aligned offset and size.  */
	KASSERT(*offp == round_page(*offp));
	KASSERT(size == round_page(size));
	KASSERT(size > 0);

	if (*offp < 0)
		return EINVAL;
	if (*offp + size > mfd->mfd_size)
		return EINVAL;

	if ((mfd->mfd_seals & F_SEAL_ANY_WRITE) &&
	    (prot & VM_PROT_WRITE) && (*flagsp & MAP_PRIVATE) == 0)
		return EPERM;

	uao_reference(fp->f_memfd->mfd_uobj);
	*uobjp = fp->f_memfd->mfd_uobj;

	*maxprotp = prot;
	*advicep = UVM_ADV_RANDOM;

	return 0;
}

static int
memfd_seek(file_t *fp, off_t delta, int whence, off_t *newoffp,
    int flags)
{
	off_t newoff;
	int error;

	switch (whence) {
	case SEEK_CUR:
		newoff = fp->f_offset + delta;
		break;

	case SEEK_END:
		newoff = fp->f_memfd->mfd_size + delta;
		break;

	case SEEK_SET:
		newoff = delta;
		break;

	default:
		error = EINVAL;
		return error;
	}

	if (newoffp)
		*newoffp = newoff;
	if (flags & FOF_UPDATE_OFFSET)
		fp->f_offset = newoff;

	return 0;
}

static int
memfd_truncate(file_t *fp, off_t length)
{
	struct memfd *mfd = fp->f_memfd;
	int error = 0;
	voff_t start, end;

	if (length < 0)
		return EINVAL;
	if (length == mfd->mfd_size)
		return 0;

	if ((mfd->mfd_seals & F_SEAL_SHRINK) && length < mfd->mfd_size)
		return EPERM;
	if ((mfd->mfd_seals & F_SEAL_GROW) && length > mfd->mfd_size)
		return EPERM;

	mutex_enter(&mfd->mfd_lock);

	if (length > mfd->mfd_size)
		ubc_zerorange(mfd->mfd_uobj, mfd->mfd_size,
		    length - mfd->mfd_size, 0);
	else {
		/* length < mfd->mfd_size, so try to get rid of excess pages */
		start = round_page(length);
		end = round_page(mfd->mfd_size);

		if (start < end) { /* we actually have pages to remove */
			rw_enter(mfd->mfd_uobj->vmobjlock, RW_WRITER);
			error = (*mfd->mfd_uobj->pgops->pgo_put)(mfd->mfd_uobj,
			    start, end, PGO_FREE);
			/* pgo_put drops vmobjlock */
		}
	}

	getnanotime(&mfd->mfd_mtime);
	mfd->mfd_size = length;
	mutex_exit(&mfd->mfd_lock);
	return error;
}
