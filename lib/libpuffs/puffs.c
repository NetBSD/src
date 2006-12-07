/*	$NetBSD: puffs.c,v 1.17 2006/12/07 23:15:20 pooka Exp $	*/

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
__RCSID("$NetBSD: puffs.c,v 1.17 2006/12/07 23:15:20 pooka Exp $");
#endif /* !lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <puffs.h>
#include <puffsdump.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

static int puffcall(struct puffs_usermount *, struct puffs_req *, size_t *);

#define FILLOP(lower, upper)						\
do {									\
	if (pops->puffs_node_##lower)					\
		opmask[PUFFS_VN_##upper] = 1;				\
} while (/*CONSTCOND*/0)
static void
fillvnopmask(struct puffs_ops *pops, uint8_t *opmask)
{

	memset(opmask, 0, PUFFS_VN_MAX);

	FILLOP(create,   CREATE);
	FILLOP(mknod,    MKNOD);
	FILLOP(open,     OPEN);
	FILLOP(close,    CLOSE);
	FILLOP(access,   ACCESS);
	FILLOP(getattr,  GETATTR);
	FILLOP(setattr,  SETATTR);
	FILLOP(poll,     POLL); /* XXX: not ready in kernel */
	FILLOP(revoke,   REVOKE);
	FILLOP(mmap,     MMAP);
	FILLOP(fsync,    FSYNC);
	FILLOP(seek,     SEEK);
	FILLOP(remove,   REMOVE);
	FILLOP(link,     LINK);
	FILLOP(rename,   RENAME);
	FILLOP(mkdir,    MKDIR);
	FILLOP(rmdir,    RMDIR);
	FILLOP(symlink,  SYMLINK);
	FILLOP(readdir,  READDIR);
	FILLOP(readlink, READLINK);
	FILLOP(reclaim,  RECLAIM);
	FILLOP(inactive, INACTIVE);
	FILLOP(print,    PRINT);
	FILLOP(read,     READ);
	FILLOP(write,    WRITE);

	/* XXX: not implemented in the kernel */
	FILLOP(getextattr, GETEXTATTR);
	FILLOP(setextattr, SETEXTATTR);
	FILLOP(listextattr, LISTEXTATTR);

	/* XXX */
	FILLOP(ioctl1, IOCTL);
	FILLOP(fcntl1, FCNTL);
}
#undef FILLOP

struct puffs_usermount *
puffs_mount(struct puffs_ops *pops, const char *dir, int mntflags,
	const char *puffsname, uint32_t pflags, size_t maxreqlen)
{
	struct puffs_startreq sreq;
	struct puffs_args pargs;
	struct puffs_usermount *pu;
	int fd = 0, rv;

	if (pops->puffs_fs_mount == NULL) {
		errno = EINVAL;
		return NULL;
	}

	fd = open("/dev/puffs", O_RDONLY);
	if (fd == -1)
		return NULL;
	if (fd <= 2)
		warnx("puffs_mount: device fd %d (<= 2), sure this is "
		    "what you want?", fd);

	pargs.pa_vers = 0; /* XXX: for now */
	pargs.pa_flags = PUFFS_FLAG_KERN(pflags);
	pargs.pa_fd = fd;
	pargs.pa_maxreqlen = maxreqlen;
	fillvnopmask(pops, pargs.pa_vnopmask);
	(void)strlcpy(pargs.pa_name, puffsname, sizeof(pargs.pa_name));

	pu = malloc(sizeof(struct puffs_usermount));
	if (!pu)
		return NULL;

	pu->pu_flags = pflags;
	pu->pu_ops = *pops;
	pu->pu_fd = fd;
	if ((pu->pu_rootpath = strdup(dir)) == NULL)
		goto failfree;
	LIST_INIT(&pu->pu_pnodelst);

	if (mount(MOUNT_PUFFS, dir, mntflags, &pargs) == -1)
		goto failfree;
	pu->pu_maxreqlen = pargs.pa_maxreqlen;

	if ((rv = pops->puffs_fs_mount(pu, &sreq.psr_cookie)) != 0) {
		errno = rv;
		goto failfree;
	}

	if ((rv = pops->puffs_fs_statvfs(pu, &sreq.psr_sb, 0)) != 0) {
		errno = rv;
		goto failfree;
	}

	/* tell kernel we're flying */
	if (ioctl(pu->pu_fd, PUFFSSTARTOP, &sreq) == -1)
		goto failfree;

	return pu;

 failfree:
	/* can't unmount() from here for obvious reasons */
	if (fd)
		close(fd);
	free(pu);
	return NULL;
}

int
puffs_mainloop(struct puffs_usermount *pu, int flags)
{
	uint8_t *buf;
	int rv;

	buf = malloc(pu->pu_maxreqlen);
	if (!buf)
		return -1;

	if ((flags & PUFFSLOOP_NODAEMON) == 0)
		if (daemon(0, 0) == -1)
			return -1;

	for (;;)
		if ((rv = puffs_oneop(pu, buf, pu->pu_maxreqlen)) != 0)
			return rv;
}

enum {PUFFCALL_ANSWER, PUFFCALL_IGNORE, PUFFCALL_UNMOUNT};
int
puffs_oneop(struct puffs_usermount *pu, uint8_t *buf, size_t buflen)
{
	struct puffs_getreq *pgr;
	struct puffs_putreq *ppr;
	struct puffs_req *preq;
	int rv, unmounting, pval;

	rv = unmounting = 0;

	/* setup fetch */
	pgr = puffs_makegetreq(pu, buf, buflen, 0);
	if (!pgr)
		return -1;

	/* setup reply essentials */
	ppr = puffs_makeputreq(pu);
	if (!ppr) {
		puffs_destroygetreq(pgr);
		return -1;
	}

	while ((preq = puffs_getreq(pgr)) != NULL && unmounting == 0) {
		pval = puffcall(pu, preq, &preq->preq_buflen);

		/* check if we need to store this reply */
		switch (pval) {
		case PUFFCALL_UNMOUNT:
			unmounting = 1;
			/* FALLTHROUGH */
		case PUFFCALL_ANSWER:
			puffs_putreq(ppr, preq);
			break;
		case PUFFCALL_IGNORE:
			break;

		default:
			assert(/*CONSTCOND*/0);
		}
	}
	puffs_destroygetreq(pgr);

	if (puffs_putputreq(ppr) == -1)
		rv = -1;
	puffs_destroyputreq(ppr);

	return rv;
}

int
puffs_getselectable(struct puffs_usermount *pu)
{

	return pu->pu_fd;
}

int
puffs_setblockingmode(struct puffs_usermount *pu, int mode)
{
	int x;

	x = mode;
	return ioctl(pu->pu_fd, FIONBIO, &x);
}

static int
puffcall(struct puffs_usermount *pu, struct puffs_req *preq, size_t *blenp)
{
	struct puffs_ops *pops = &pu->pu_ops;
	struct puffs_sizeop pop;
	void *auxbuf;
	int error, rv;

	if (PUFFSOP_WANTREPLY(preq->preq_opclass))
		rv = PUFFCALL_ANSWER;
	else
		rv = PUFFCALL_IGNORE;
	auxbuf = preq;

	if (pu->pu_flags & PUFFS_FLAG_OPDUMP)
		puffsdump_req(preq);

	*blenp = 0; /* default: "let kernel decide" */

	if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_VFS) {
		switch (preq->preq_optype) {
		case PUFFS_VFS_UNMOUNT:
		{
			struct puffs_vfsreq_unmount *auxt = auxbuf;

			error = pops->puffs_fs_unmount(pu,
			    auxt->pvfsr_flags, auxt->pvfsr_pid);
			rv = PUFFCALL_UNMOUNT;
			break;
		}
		case PUFFS_VFS_STATVFS:
		{
			struct puffs_vfsreq_statvfs *auxt = auxbuf;

			error = pops->puffs_fs_statvfs(pu,
			    &auxt->pvfsr_sb, auxt->pvfsr_pid);
			break;
		}
		case PUFFS_VFS_SYNC:
		{
			struct puffs_vfsreq_sync *auxt = auxbuf;

			error = pops->puffs_fs_sync(pu,
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
	/* XXX: sync with kernel */
	} else if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_VN) {
		switch (preq->preq_optype) {
		case PUFFS_VN_LOOKUP:
		{
			struct puffs_vnreq_lookup *auxt = auxbuf;

			/* lookup *must* be present */
			error = pops->puffs_node_lookup(pu, preq->preq_cookie,
			    &auxt->pvnr_newnode, &auxt->pvnr_vtype,
			    &auxt->pvnr_size, &auxt->pvnr_rdev,
			    &auxt->pvnr_cn);
			break;
		}

		case PUFFS_VN_CREATE:
		{
			struct puffs_vnreq_create *auxt = auxbuf;
			if (pops->puffs_node_create == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_create(pu,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &auxt->pvnr_cn, &auxt->pvnr_va);
			break;
		}

		case PUFFS_VN_MKNOD:
		{
			struct puffs_vnreq_mknod *auxt = auxbuf;
			if (pops->puffs_node_mknod == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_mknod(pu,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &auxt->pvnr_cn, &auxt->pvnr_va);
			break;
		}

		case PUFFS_VN_OPEN:
		{
			struct puffs_vnreq_open *auxt = auxbuf;
			if (pops->puffs_node_open == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_open(pu,
			    preq->preq_cookie, auxt->pvnr_mode,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_CLOSE:
		{
			struct puffs_vnreq_close *auxt = auxbuf;
			if (pops->puffs_node_close == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_close(pu,
			    preq->preq_cookie, auxt->pvnr_fflag,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_ACCESS:
		{
			struct puffs_vnreq_access *auxt = auxbuf;
			if (pops->puffs_node_access == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_access(pu,
			    preq->preq_cookie, auxt->pvnr_mode,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_GETATTR:
		{
			struct puffs_vnreq_getattr *auxt = auxbuf;
			if (pops->puffs_node_getattr == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pops->puffs_node_getattr(pu,
			    preq->preq_cookie, &auxt->pvnr_va,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_SETATTR:
		{
			struct puffs_vnreq_setattr *auxt = auxbuf;
			if (pops->puffs_node_setattr == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pops->puffs_node_setattr(pu,
			    preq->preq_cookie, &auxt->pvnr_va,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_MMAP:
		{
			struct puffs_vnreq_mmap *auxt = auxbuf;
			if (pops->puffs_node_mmap == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_mmap(pu,
			    preq->preq_cookie, auxt->pvnr_fflags,
			    &auxt->pvnr_cred, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_REVOKE:
		{
			struct puffs_vnreq_revoke *auxt = auxbuf;
			if (pops->puffs_node_revoke == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_revoke(pu,
			    preq->preq_cookie, auxt->pvnr_flags);
			break;
		}

		case PUFFS_VN_FSYNC:
		{
			struct puffs_vnreq_fsync *auxt = auxbuf;
			if (pops->puffs_node_fsync == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_fsync(pu,
			    preq->preq_cookie, &auxt->pvnr_cred,
			    auxt->pvnr_flags, auxt->pvnr_offlo,
			    auxt->pvnr_offhi, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_SEEK:
		{
			struct puffs_vnreq_seek *auxt = auxbuf;
			if (pops->puffs_node_seek == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_seek(pu,
			    preq->preq_cookie, auxt->pvnr_oldoff,
			    auxt->pvnr_newoff, &auxt->pvnr_cred);
			break;
		}

		case PUFFS_VN_REMOVE:
		{
			struct puffs_vnreq_remove *auxt = auxbuf;
			if (pops->puffs_node_remove == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_remove(pu,
			    preq->preq_cookie, auxt->pvnr_cookie_targ,
			    &auxt->pvnr_cn);
			break;
		}

		case PUFFS_VN_LINK:
		{
			struct puffs_vnreq_link *auxt = auxbuf;
			if (pops->puffs_node_link == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_link(pu,
			    preq->preq_cookie, auxt->pvnr_cookie_targ,
			    &auxt->pvnr_cn);
			break;
		}

		case PUFFS_VN_RENAME:
		{
			struct puffs_vnreq_rename *auxt = auxbuf;
			if (pops->puffs_node_rename == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_rename(pu,
			    preq->preq_cookie, auxt->pvnr_cookie_src,
			    &auxt->pvnr_cn_src, auxt->pvnr_cookie_targdir,
			    auxt->pvnr_cookie_targ, &auxt->pvnr_cn_targ);
			break;
		}

		case PUFFS_VN_MKDIR:
		{
			struct puffs_vnreq_mkdir *auxt = auxbuf;
			if (pops->puffs_node_mkdir == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_mkdir(pu,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &auxt->pvnr_cn, &auxt->pvnr_va);
			break;
		}

		case PUFFS_VN_RMDIR:
		{
			struct puffs_vnreq_rmdir *auxt = auxbuf;
			if (pops->puffs_node_rmdir == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_rmdir(pu,
			    preq->preq_cookie, auxt->pvnr_cookie_targ,
			    &auxt->pvnr_cn);
			break;
		}

		case PUFFS_VN_SYMLINK:
		{
			struct puffs_vnreq_symlink *auxt = auxbuf;
			if (pops->puffs_node_symlink == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_symlink(pu,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &auxt->pvnr_cn, &auxt->pvnr_va, auxt->pvnr_link);
			break;
		}

		case PUFFS_VN_READDIR:
		{
			struct puffs_vnreq_readdir *auxt = auxbuf;
			size_t res;

			if (pops->puffs_node_readdir == NULL) {
				error = 0;
				break;
			}

			res = auxt->pvnr_resid;
			error = pops->puffs_node_readdir(pu,
			    preq->preq_cookie, auxt->pvnr_dent,
			    &auxt->pvnr_cred, &auxt->pvnr_offset,
			    &auxt->pvnr_resid);

			/* need to move a bit more */
			*blenp = sizeof(struct puffs_vnreq_readdir) 
			    + (res - auxt->pvnr_resid);
			break;
		}

		case PUFFS_VN_READLINK:
		{
			struct puffs_vnreq_readlink *auxt = auxbuf;
			if (pops->puffs_node_readlink == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pops->puffs_node_readlink(pu,
			    preq->preq_cookie, &auxt->pvnr_cred,
			    auxt->pvnr_link, &auxt->pvnr_linklen);
			break;
		}

		case PUFFS_VN_RECLAIM:
		{
			struct puffs_vnreq_reclaim *auxt = auxbuf;
			if (pops->puffs_node_reclaim == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_reclaim(pu,
			    preq->preq_cookie, auxt->pvnr_pid);
			break;
		}

		case PUFFS_VN_INACTIVE:
		{
			struct puffs_vnreq_inactive *auxt = auxbuf;
			if (pops->puffs_node_inactive == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pops->puffs_node_inactive(pu,
			    preq->preq_cookie, auxt->pvnr_pid,
			    &auxt->pvnr_backendrefs);
			break;
		}

		case PUFFS_VN_PATHCONF:
		{
			struct puffs_vnreq_pathconf *auxt = auxbuf;
			if (pops->puffs_node_pathconf == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_pathconf(pu,
			    preq->preq_cookie, auxt->pvnr_name,
			    &auxt->pvnr_retval);
			break;
		}

		case PUFFS_VN_ADVLOCK:
		{
			struct puffs_vnreq_advlock *auxt = auxbuf;
			if (pops->puffs_node_advlock == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_advlock(pu,
			    preq->preq_cookie, auxt->pvnr_id, auxt->pvnr_op,
			    &auxt->pvnr_fl, auxt->pvnr_flags);
			break;
		}

		case PUFFS_VN_PRINT:
		{
			if (pops->puffs_node_print == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_print(pu,
			    preq->preq_cookie);
			break;
		}

		case PUFFS_VN_READ:
		{
			struct puffs_vnreq_read *auxt = auxbuf;
			size_t res;

			if (pops->puffs_node_read == NULL) {
				error = EIO;
				break;
			}

			res = auxt->pvnr_resid;
			error = pops->puffs_node_read(pu,
			    preq->preq_cookie, auxt->pvnr_data,
			    auxt->pvnr_offset, &auxt->pvnr_resid,
			    &auxt->pvnr_cred, auxt->pvnr_ioflag);

			/* need to move a bit more */
			*blenp = sizeof(struct puffs_vnreq_read)
			    + (res - auxt->pvnr_resid);
			break;
		}

		case PUFFS_VN_WRITE:
		{
			struct puffs_vnreq_write *auxt = auxbuf;

			if (pops->puffs_node_write == NULL) {
				error = EIO;
				break;
			}

			error = pops->puffs_node_write(pu,
			    preq->preq_cookie, auxt->pvnr_data,
			    auxt->pvnr_offset, &auxt->pvnr_resid,
			    &auxt->pvnr_cred, auxt->pvnr_ioflag);

			/* don't need to move data back to the kernel */
			*blenp = sizeof(struct puffs_vnreq_write);
			break;
		}

		case PUFFS_VN_IOCTL:
			error = pops->puffs_node_ioctl1(pu, preq->preq_cookie,
			     (struct puffs_vnreq_ioctl *)auxbuf, &pop);
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
			error = pops->puffs_node_ioctl2(pu, preq->preq_cookie,
			    (struct puffs_vnreq_ioctl *)auxbuf, &pop);
			break;

		case PUFFS_VN_FCNTL:
			error = pops->puffs_node_fcntl1(pu, preq->preq_cookie,
			     (struct puffs_vnreq_fcntl *)auxbuf, &pop);
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
			error = pops->puffs_node_fcntl2(pu, preq->preq_cookie,
			    (struct puffs_vnreq_fcntl *)auxbuf, &pop);
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
		case PUFFS_VN_POLL:
		{
			struct puffs_vnreq_poll *auxt = auxbuf;
			if (pops->puffs_node_poll == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_poll(pu,
			    preq->preq_cookie, preq-);
			break;
		}

		case PUFFS_VN_KQFILTER:
		{
			struct puffs_vnreq_kqfilter *auxt = auxbuf;
			if (pops->puffs_node_kqfilter == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_kqfilter(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_CLOSEEXTATTR:
		{
			struct puffs_vnreq_closeextattr *auxt = auxbuf;
			if (pops->puffs_closeextattr == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_closeextattr(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_GETEXTATTR:
		{
			struct puffs_vnreq_getextattr *auxt = auxbuf;
			if (pops->puffs_getextattr == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_getextattr(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_LISTEXTATTR:
		{
			struct puffs_vnreq_listextattr *auxt = auxbuf;
			if (pops->puffs_listextattr == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_listextattr(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_OPENEXTATTR:
		{
			struct puffs_vnreq_openextattr *auxt = auxbuf;
			if (pops->puffs_openextattr == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_openextattr(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_DELETEEXTATTR:
		{
			struct puffs_vnreq_deleteextattr *auxt = auxbuf;
			if (pops->puffs_deleteextattr == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_deleteextattr(pu,
			    preq->preq_cookie, );
			break;
		}

		case PUFFS_VN_SETEXTATTR:
		{
			struct puffs_vnreq_setextattr *auxt = auxbuf;
			if (pops->puffs_setextattr == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_setextattr(pu,
			    preq->preq_cookie, );
			break;
		}

#endif
