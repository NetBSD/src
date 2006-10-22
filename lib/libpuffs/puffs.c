/*	$NetBSD: puffs.c,v 1.1 2006/10/22 22:52:21 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: puffs.c,v 1.1 2006/10/22 22:52:21 pooka Exp $");
#endif /* !lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <errno.h>
#include <fcntl.h>
#include <puffs.h>
#include <puffsdump.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

static int puffcall(struct puffs_usermount *, struct puffs_req *);

#define eret(a) do {errno=(a);goto failfree;}while/*NOTREACHED*/(/*CONSTCOND*/0)

struct puffs_usermount *
puffs_mount(struct puffs_vfsops *pvfs, struct puffs_vnops *pvn,
	const char *dir, int mntflags, const char *puffsname,
	uint32_t pflags, size_t maxreqlen)
{
	struct puffs_args pargs;
	struct puffs_vfsreq_start sreq;
	struct puffs_usermount *pu;
	int fd;

	pu = NULL;

	if (pvfs->puffs_start == NULL)
		 eret(EINVAL);

	fd = open("/dev/puffs", O_RDONLY);
	if (fd == -1)
		return NULL;

	pargs.pa_vers = 0; /* XXX: for now */
	pargs.pa_flags = pflags;
	pargs.pa_fd = fd;
	pargs.pa_maxreqlen = maxreqlen;
	(void)strlcpy(pargs.pa_name, puffsname, sizeof(pargs.pa_name));

	pu = malloc(sizeof(struct puffs_usermount));
	if (!pu)
		eret(ENOMEM);

	pu->pu_flags = pflags;
	pu->pu_pvfs = *pvfs;
	pu->pu_pvn = *pvn;
	pu->pu_fd = fd;
	if ((pu->pu_rootpath = strdup(dir)) == NULL)
		eret(ENOMEM);
	LIST_INIT(&pu->pu_pnodelst);

	if (mount(MOUNT_PUFFS, dir, mntflags, &pargs) == -1)
		return NULL;
	pu->pu_maxreqlen = pargs.pa_maxreqlen;

	if (pu->pu_pvfs.puffs_start(pu, &sreq) != 0)
		return NULL;

	/* tell kernel we're flying */
	if (ioctl(pu->pu_fd, PUFFSMOUNTOP, &sreq) == -1)
		return NULL;

	return pu;
 failfree:
	free(pu);
	return NULL;
}

int
puffs_mainloop(struct puffs_usermount *pu)
{
	uint8_t *buf;
	int rv;

	buf = malloc(pu->pu_maxreqlen);
	if (!buf)
		return -1;

	for (;;)
		if ((rv = puffs_oneop(pu, buf, pu->pu_maxreqlen)) != 0)
			return rv;
}

int
puffs_oneop(struct puffs_usermount *pu, uint8_t *buf, size_t buflen)
{
	struct puffs_req preq;
	int rv;

	preq.preq_aux = buf;
	preq.preq_auxlen = buflen;

	/* get op from kernel */
	if (ioctl(pu->pu_fd, PUFFSGETOP, &preq) == -1)
		return -1;

	/* deal with it */
	rv = puffcall(pu, &preq);

	/* stuff result back to the kernel */
	if (ioctl(pu->pu_fd, PUFFSPUTOP, &preq) == -1)
		return -1;

	return rv;
}

int
puffs_getselectable(struct puffs_usermount *pu)
{

	return pu->pu_fd;
}

int
puffs_setblockingmode(struct puffs_usermount *pu, int block)
{
	int x;

	x = !block;
	return ioctl(pu->pu_fd, FIONBIO, &x);
}

static int
puffcall(struct puffs_usermount *pu, struct puffs_req *preq)
{
	struct puffs_sizeop pop;
	int error, rv;

	rv = 0;

	if (pu->pu_flags & PUFFSFLAG_OPDUMP)
		puffsdump_req(preq);

	if (preq->preq_opclass == PUFFSOP_VFS) {
		switch (preq->preq_optype) {
		case PUFFS_VFS_UNMOUNT:
		{
			struct puffs_vfsreq_unmount *auxt = preq->preq_aux;

			error = pu->pu_pvfs.puffs_unmount(pu,
			    auxt->pvfsr_flags, auxt->pvfsr_pid);
			rv = 1;
			break;
		}
		case PUFFS_VFS_STATVFS:
		{
			struct puffs_vfsreq_statvfs *auxt = preq->preq_aux;

			error = pu->pu_pvfs.puffs_statvfs(pu,
			    &auxt->pvfsr_sb, auxt->pvfsr_pid);
			break;
		}
		case PUFFS_VFS_SYNC:
		{
			struct puffs_vfsreq_sync *auxt = preq->preq_aux;

			error = pu->pu_pvfs.puffs_sync(pu,
			    auxt->pvfsr_waitfor, &auxt->pvfsr_cred,
			    auxt->pvfsr_pid);
			break;
		}
		default:
			/*
			 * I guess the kernel sees this one coming
			 */
			error = EINVAL;
			break;
		}

	/* XXX: audit return values */
	} else if (preq->preq_opclass == PUFFSOP_VN) {
		switch (preq->preq_optype) {
		case PUFFS_VN_LOOKUP:
		{
			struct puffs_vnreq_lookup *auxt = preq->preq_aux;

			/* lookup *must* be present */
			error = pu->pu_pvn.puffs_lookup(pu, preq->preq_cookie,
			    &auxt->pvnr_newnode, &auxt->pvnr_vtype,
			    &auxt->pvnr_cn);
			break;
		}

		case PUFFS_VN_CREATE:
		{
			struct puffs_vnreq_create *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_create == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_create(pu,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &auxt->pvnr_cn, &auxt->pvnr_va);
			break;
		}

		case PUFFS_VN_MKNOD:
		{
			struct puffs_vnreq_mknod *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_mknod == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_mknod(pu,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &auxt->pvnr_cn, &auxt->pvnr_va);
			break;
		}

		case PUFFS_VN_OPEN:
		{
			struct puffs_vnreq_open *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_open == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_open(pu,
			    preq->preq_cookie, auxt->pvnr_mode,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_CLOSE:
		{
			struct puffs_vnreq_close *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_close == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_close(pu,
			    preq->preq_cookie, auxt->pvnr_fflag,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_ACCESS:
		{
			struct puffs_vnreq_access *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_access == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_access(pu,
			    preq->preq_cookie, auxt->pvnr_mode,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_GETATTR:
		{
			struct puffs_vnreq_getattr *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_getattr == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_getattr(pu,
			    preq->preq_cookie, &auxt->pvnr_va,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_SETATTR:
		{
			struct puffs_vnreq_setattr *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_setattr == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_setattr(pu,
			    preq->preq_cookie, &auxt->pvnr_va,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

/* notyet */
#if 0
		case PUFFS_VN_POLL:
		{
			struct puffs_vnreq_poll *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_poll == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_poll(pu,
			    preq->preq_cookie, preq-);
			break;
		}

		case PUFFS_VN_KQFILTER:
		{
			struct puffs_vnreq_kqfilter *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_kqfilter == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_kqfilter(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_MMAP:
		{
			struct puffs_vnreq_mmap *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_mmap == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_mmap(pu,
			    preq->preq_cookie, );
			break;
		}
#endif

		case PUFFS_VN_REVOKE:
		{
			struct puffs_vnreq_revoke *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_revoke == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_revoke(pu,
			    preq->preq_cookie, auxt->pvnr_flags);
			break;
		}

		case PUFFS_VN_FSYNC:
		{
			struct puffs_vnreq_fsync *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_fsync == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_fsync(pu,
			    preq->preq_cookie, &auxt->pvnr_cred,
			    auxt->pvnr_flags, auxt->pvnr_offlo,
			    auxt->pvnr_offhi, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_SEEK:
		{
			struct puffs_vnreq_seek *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_seek == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_seek(pu,
			    preq->preq_cookie, auxt->pvnr_oldoff,
			    auxt->pvnr_newoff, &auxt->pvnr_cred);
			break;
		}

		case PUFFS_VN_REMOVE:
		{
			struct puffs_vnreq_remove *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_remove == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_remove(pu,
			    preq->preq_cookie, auxt->pvnr_cookie_targ,
			    &auxt->pvnr_cn);
			break;
		}

		case PUFFS_VN_LINK:
		{
			struct puffs_vnreq_link *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_link == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_link(pu,
			    preq->preq_cookie, auxt->pvnr_cookie_targ,
			    &auxt->pvnr_cn);
			break;
		}

		case PUFFS_VN_RENAME:
		{
			struct puffs_vnreq_rename *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_rename == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_rename(pu,
			    preq->preq_cookie, auxt->pvnr_cookie_src,
			    &auxt->pvnr_cn_src, auxt->pvnr_cookie_targdir,
			    auxt->pvnr_cookie_targ, &auxt->pvnr_cn_targ);
			break;
		}

		case PUFFS_VN_MKDIR:
		{
			struct puffs_vnreq_mkdir *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_mkdir == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_mkdir(pu,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &auxt->pvnr_cn, &auxt->pvnr_va);
			break;
		}

		case PUFFS_VN_RMDIR:
		{
			struct puffs_vnreq_rmdir *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_rmdir == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_rmdir(pu,
			    preq->preq_cookie, auxt->pvnr_cookie_targ,
			    &auxt->pvnr_cn);
			break;
		}

		case PUFFS_VN_SYMLINK:
		{
			struct puffs_vnreq_symlink *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_symlink == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_symlink(pu,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &auxt->pvnr_cn, &auxt->pvnr_va, auxt->pvnr_link);
			break;
		}

		case PUFFS_VN_READDIR:
		{
			struct puffs_vnreq_readdir *auxt = preq->preq_aux;
			size_t res;

			if (pu->pu_pvn.puffs_readdir == NULL) {
				error = 0;
				break;
			}

			res = auxt->pvnr_resid;
			error = pu->pu_pvn.puffs_readdir(pu,
			    preq->preq_cookie, auxt->pvnr_dent,
			    &auxt->pvnr_cred, &auxt->pvnr_offset,
			    &auxt->pvnr_resid);

			/* need to move a bit more */
			preq->preq_auxlen += res - auxt->pvnr_resid;
			break;
		}

		case PUFFS_VN_READLINK:
		{
			struct puffs_vnreq_readlink *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_readlink == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pu->pu_pvn.puffs_readlink(pu,
			    preq->preq_cookie, &auxt->pvnr_cred,
			    auxt->pvnr_link, &auxt->pvnr_linklen);
			break;
		}

		case PUFFS_VN_RECLAIM:
		{
			struct puffs_vnreq_reclaim *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_reclaim == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_reclaim(pu,
			    preq->preq_cookie, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_PATHCONF:
		{
			struct puffs_vnreq_pathconf *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_pathconf == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_pathconf(pu,
			    preq->preq_cookie, auxt->pvnr_name,
			    &auxt->pvnr_retval);
			break;
		}

		case PUFFS_VN_ADVLOCK:
		{
			struct puffs_vnreq_advlock *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_advlock == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_advlock(pu,
			    preq->preq_cookie, auxt->pvnr_id, auxt->pvnr_op,
			    &auxt->pvnr_fl, auxt->pvnr_flags);
			break;
		}

		case PUFFS_VN_PRINT:
		{
			struct puffs_vnreq_print *auxt = preq->preq_aux;
			(void)auxt;
			if (pu->pu_pvn.puffs_print == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_print(pu,
			    preq->preq_cookie);
			break;
		}

		case PUFFS_VN_READ:
		{
			struct puffs_vnreq_read *auxt = preq->preq_aux;
			size_t res;

			if (pu->pu_pvn.puffs_read == NULL) {
				error = EIO;
				break;
			}

			res = auxt->pvnr_resid;
			error = pu->pu_pvn.puffs_read(pu,
			    preq->preq_cookie, auxt->pvnr_data,
			    auxt->pvnr_offset, &auxt->pvnr_resid,
			    &auxt->pvnr_cred, auxt->pvnr_ioflag);

			/* need to move a bit more */
			preq->preq_auxlen += res - auxt->pvnr_resid;
			break;
		}

		case PUFFS_VN_WRITE:
		{
			struct puffs_vnreq_write *auxt = preq->preq_aux;

			if (pu->pu_pvn.puffs_write == NULL) {
				error = EIO;
				break;
			}

			error = pu->pu_pvn.puffs_write(pu,
			    preq->preq_cookie, auxt->pvnr_data,
			    auxt->pvnr_offset, &auxt->pvnr_resid,
			    &auxt->pvnr_cred, auxt->pvnr_ioflag);

			/* don't need to move data back to the kernel */
			preq->preq_auxlen = sizeof(struct puffs_vnreq_write);
			break;
		}

		case PUFFS_VN_IOCTL:
			error = pu->pu_pvn.puffs_ioctl1(pu, preq->preq_cookie,
			     (struct puffs_vnreq_ioctl *)preq->preq_aux, &pop);
			if (error != 0)
				break;
			pop.pso_reqid = preq->preq_id;

			/* let the kernel do it's intermediate duty */
			error = ioctl(pu->pu_fd, PUFFSSIZEOP, &pop);
			/*
			 * XXX: I don't actually know what the correct
			 * thing to do in case of an error is, so I'll
			 * just ignore it for the time being.
			 */
			error = pu->pu_pvn.puffs_ioctl2(pu, preq->preq_cookie,
			    (struct puffs_vnreq_ioctl *)preq->preq_aux, &pop);
			break;

		case PUFFS_VN_FCNTL:
			error = pu->pu_pvn.puffs_fcntl1(pu, preq->preq_cookie,
			     (struct puffs_vnreq_fcntl *)preq->preq_aux, &pop);
			if (error != 0)
				break;
			pop.pso_reqid = preq->preq_id;

			/* let the kernel do it's intermediate duty */
			error = ioctl(pu->pu_fd, PUFFSSIZEOP, &pop);
			/*
			 * XXX: I don't actually know what the correct
			 * thing to do in case of an error is, so I'll
			 * just ignore it for the time being.
			 */
			error = pu->pu_pvn.puffs_fcntl2(pu, preq->preq_cookie,
			    (struct puffs_vnreq_fcntl *)preq->preq_aux, &pop);
			break;

		default:
			printf("inval op %d\n", preq->preq_optype);
			error = EINVAL;
			break;
		}
	} else {
		/*
		 * this one also
		 */
		error = EINVAL;
	}

	preq->preq_rv = error;
	return rv;
}


#if 0
		case PUFFS_VN_IOCTL:
		{
			struct puffs_vnreq_ioctl *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_ioctl == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_ioctl(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_FCNTL:
		{
			struct puffs_vnreq_fcntl *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_fcntl == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_fcntl(pu,
			    preq->preq_cookie, );
			break;
		}


		case PUFFS_VN_ABORTOP:
		{
			struct puffs_vnreq_abortop *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_abortop == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_abortop(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_INACTIVE:
		{
			struct puffs_vnreq_inactive *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_inactive == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_inactive(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_LOCK:
		{
			struct puffs_vnreq_lock *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_lock == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_lock(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_UNLOCK:
		{
			struct puffs_vnreq_unlock *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_unlock == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_unlock(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_ISLOCKED:
		{
			struct puffs_vnreq_islocked *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_islocked == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_islocked(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_LEASE:
		{
			struct puffs_vnreq_lease *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_lease == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_lease(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_WHITEOUT:
		{
			struct puffs_vnreq_whiteout *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_whiteout == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_whiteout(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_GETPAGES:
		{
			struct puffs_vnreq_getpages *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_getpages == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_getpages(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_PUTPAGES:
		{
			struct puffs_vnreq_putpages *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_putpages == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_putpages(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_CLOSEEXTATTR:
		{
			struct puffs_vnreq_closeextattr *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_closeextattr == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_closeextattr(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_GETEXTATTR:
		{
			struct puffs_vnreq_getextattr *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_getextattr == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_getextattr(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_LISTEXTATTR:
		{
			struct puffs_vnreq_listextattr *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_listextattr == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_listextattr(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_OPENEXTATTR:
		{
			struct puffs_vnreq_openextattr *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_openextattr == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_openextattr(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_DELETEEXTATTR:
		{
			struct puffs_vnreq_deleteextattr *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_deleteextattr == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_deleteextattr(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_SETEXTATTR:
		{
			struct puffs_vnreq_setextattr *auxt = preq->preq_aux;
			if (pu->pu_pvn.puffs_setextattr == NULL) {
				error = 0;
				break;
			}

			error = pu->pu_pvn.puffs_setextattr(pu,
			    preq->preq_cookie, );
			break;
		}

#endif
