/*	$NetBSD: puffs.c,v 1.19 2006/12/29 15:28:11 pooka Exp $	*/

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
__RCSID("$NetBSD: puffs.c,v 1.19 2006/12/29 15:28:11 pooka Exp $");
#endif /* !lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <puffs.h>
#include <puffsdump.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "puffs_priv.h"

static int do_buildpath(struct puffs_cn *, void *);

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
}
#undef FILLOP

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

int
puffs_getstate(struct puffs_usermount *pu)
{

	return pu->pu_state;
}

void
puffs_setstacksize(struct puffs_usermount *pu, size_t ss)
{

	pu->pu_cc_stacksize = ss;
}

/*
 * in case the server wants to use some other scheme for paths, it
 * can (*must*) call this in mount.
 */
int
puffs_setrootpath(struct puffs_usermount *pu, const char *rootpath)
{
	struct puffs_node *pnr;

	pnr = pu->pu_pn_root;
	if (pnr == NULL) {
		errno = ENOENT;
		return -1;
	}

	free(pnr->pn_path);
	pnr->pn_path = strdup(rootpath);
	if (pnr->pn_path == NULL)
		return -1;
	pnr->pn_plen = strlen(pnr->pn_path) + 1; /* yes, count nul */

	return 0;
}


enum {PUFFCALL_ANSWER, PUFFCALL_IGNORE, PUFFCALL_AGAIN};

struct puffs_usermount *
puffs_mount(struct puffs_ops *pops, const char *dir, int mntflags,
	const char *puffsname, void *priv, uint32_t pflags, size_t maxreqlen)
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
	free(pops);
	pu->pu_fd = fd;
	pu->pu_privdata = priv;
	pu->pu_cc_stacksize = PUFFS_CC_STACKSIZE_DEFAULT;
	LIST_INIT(&pu->pu_pnodelst);

	pu->pu_state = PUFFS_STATE_MOUNTING;
	if (mount(MOUNT_PUFFS, dir, mntflags, &pargs) == -1)
		goto failfree;
	pu->pu_maxreqlen = pargs.pa_maxreqlen;

	if ((rv = pops->puffs_fs_mount(pu, &sreq.psr_cookie,
	    &sreq.psr_sb)) != 0) {
		errno = rv;
		goto failfree;
	}

	/* tell kernel we're flying */
	if (ioctl(pu->pu_fd, PUFFSSTARTOP, &sreq) == -1)
		goto failfree;

	pu->pu_state = PUFFS_STATE_RUNNING;
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
	struct puffs_getreq *pgr;
	struct puffs_putreq *ppr;
	int rv;

	rv = -1;
	pgr = puffs_makegetreq(pu, pu->pu_maxreqlen, 0);
	if (pgr == NULL)
		return -1;

	ppr = puffs_makeputreq(pu);
	if (ppr == NULL) {
		puffs_destroygetreq(pgr);
		return -1;
	}

	if ((flags & PUFFSLOOP_NODAEMON) == 0)
		if (daemon(0, 0) == -1)
			goto out;

	/* XXX: should be a bit more robust with errors here */
	while (puffs_getstate(pu) == PUFFS_STATE_RUNNING
	    || puffs_getstate(pu) == PUFFS_STATE_UNMOUNTING) {
		puffs_resetputreq(ppr);

		if (puffs_handlereqs(pu, pgr, ppr, 0) == -1) {
			rv = -1;
			break;
		}
		if (puffs_putputreq(ppr) == -1) {
			rv = -1;
			break;
		}
	}

 out:
	puffs_destroyputreq(ppr);
	puffs_destroygetreq(pgr);
	return rv;
}

int
puffs_handlereqs(struct puffs_usermount *pu, struct puffs_getreq *pgr,
	struct puffs_putreq *ppr, int maxops)
{
	struct puffs_req *preq;
	int pval;

	puffs_setmaxgetreq(pgr, maxops);
	if (puffs_loadgetreq(pgr) == -1)
		return -1;

	/* interlink pgr and ppr for diagnostic asserts */
	pgr->pgr_nppr++;
	ppr->ppr_pgr = pgr;

	pval = 0;
	while ((preq = puffs_getreq(pgr)) != NULL
	    && pu->pu_state != PUFFS_STATE_UNMOUNTED)
		pval = puffs_dopreq(pu, ppr, preq);

	return pval;
}

int
puffs_dopreq(struct puffs_usermount *pu, struct puffs_putreq *ppr,
	struct puffs_req *preq)
{
	struct puffs_cc *pcc;
	int rv;

	if (pu->pu_flags & PUFFS_FLAG_OPDUMP)
		puffsdump_req(preq);

	pcc = puffs_cc_create(pu, preq);
	rv = puffs_docc(ppr, pcc);

	if ((pcc->pcc_flags & PCC_DONE) == 0)
		return 0;

	return rv;
}

int
puffs_docc(struct puffs_putreq *ppr, struct puffs_cc *pcc)
{
	int rv;

	assert((pcc->pcc_flags & PCC_DONE) == 0);

	puffs_cc_continue(pcc);
	rv = pcc->pcc_rv;

	if ((pcc->pcc_flags & PCC_DONE) == 0)
		rv = PUFFCALL_AGAIN;

	/* check if we need to store this reply */
	switch (rv) {
	case PUFFCALL_ANSWER:
		puffs_putreq_cc(ppr, pcc);
		break;
	case PUFFCALL_AGAIN:
	case PUFFCALL_IGNORE:
		break;
	default:
		assert(/*CONSTCOND*/0);
	}

	return 0;
}

/* library private, but linked from callcontext.c */

void
puffs_calldispatcher(struct puffs_cc *pcc)
{
	struct puffs_usermount *pu = pcc->pcc_pu;
	struct puffs_ops *pops = &pu->pu_ops;
	struct puffs_req *preq = pcc->pcc_preq;
	void *auxbuf = preq; /* help with typecasting */
	int error, rv, buildpath;

	assert(pcc->pcc_flags & (PCC_ONCE | PCC_REALCC));

	if (PUFFSOP_WANTREPLY(preq->preq_opclass))
		rv = PUFFCALL_ANSWER;
	else
		rv = PUFFCALL_IGNORE;

	buildpath = pu->pu_flags & PUFFS_FLAG_BUILDPATH;

	if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_VFS) {
		switch (preq->preq_optype) {
		case PUFFS_VFS_UNMOUNT:
		{
			struct puffs_vfsreq_unmount *auxt = auxbuf;

			pu->pu_state = PUFFS_STATE_UNMOUNTING;
			error = pops->puffs_fs_unmount(pcc,
			    auxt->pvfsr_flags, auxt->pvfsr_pid);
			if (!error)
				pu->pu_state = PUFFS_STATE_UNMOUNTED;
			else
				pu->pu_state = PUFFS_STATE_RUNNING;
			break;
		}
		case PUFFS_VFS_STATVFS:
		{
			struct puffs_vfsreq_statvfs *auxt = auxbuf;

			error = pops->puffs_fs_statvfs(pcc,
			    &auxt->pvfsr_sb, auxt->pvfsr_pid);
			break;
		}
		case PUFFS_VFS_SYNC:
		{
			struct puffs_vfsreq_sync *auxt = auxbuf;

			error = pops->puffs_fs_sync(pcc,
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
			struct puffs_cn pcn;

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			if (buildpath) {
				if (pcn.pcn_flags & PUFFS_ISDOTDOT) {
					buildpath = 0;
				} else {
					error = do_buildpath(&pcn,
					    preq->preq_cookie);
					if (error)
						break;
				}
			}

			/* lookup *must* be present */
			error = pops->puffs_node_lookup(pcc, preq->preq_cookie,
			    &auxt->pvnr_newnode, &auxt->pvnr_vtype,
			    &auxt->pvnr_size, &auxt->pvnr_rdev, &pcn);

			if (buildpath) {
				if (error) {
					free(pcn.pcn_fullpath);
				} else {
					struct puffs_node *pn;

					/*
					 * did we get a new node or a
					 * recycled node?
					 * XXX: mapping assumption
					 */
					pn = auxt->pvnr_newnode;
					if (pn->pn_path == NULL) {
						pn->pn_path = pcn.pcn_fullpath;
						pn->pn_plen
						    = pcn.pcn_fullplen;
					}
				}
			}

			break;
		}

		case PUFFS_VN_CREATE:
		{
			struct puffs_vnreq_create *auxt = auxbuf;
			struct puffs_cn pcn;
			if (pops->puffs_node_create == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			if (buildpath) {
				error = do_buildpath(&pcn, preq->preq_cookie);
				if (error)
					break;
			}

			error = pops->puffs_node_create(pcc,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &pcn, &auxt->pvnr_va);

			if (buildpath) {
				if (error) {
					free(pcn.pcn_fullpath);
				} else {
					struct puffs_node *pn;

					pn = auxt->pvnr_newnode;
					pn->pn_path = pcn.pcn_fullpath;
					pn->pn_plen = pcn.pcn_fullplen;
				}
			}

			break;
		}

		case PUFFS_VN_MKNOD:
		{
			struct puffs_vnreq_mknod *auxt = auxbuf;
			struct puffs_cn pcn;
			if (pops->puffs_node_mknod == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			if (buildpath) {
				error = do_buildpath(&pcn, preq->preq_cookie);
				if (error)
					break;
			}

			error = pops->puffs_node_mknod(pcc,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &pcn, &auxt->pvnr_va);

			if (buildpath) {
				if (error) {
					free(pcn.pcn_fullpath);
				} else {
					struct puffs_node *pn;

					pn = auxt->pvnr_newnode;
					pn->pn_path = pcn.pcn_fullpath;
					pn->pn_plen = pcn.pcn_fullplen;
				}
			}

			break;
		}

		case PUFFS_VN_OPEN:
		{
			struct puffs_vnreq_open *auxt = auxbuf;
			if (pops->puffs_node_open == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_open(pcc,
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

			error = pops->puffs_node_close(pcc,
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

			error = pops->puffs_node_access(pcc,
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

			error = pops->puffs_node_getattr(pcc,
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

			error = pops->puffs_node_setattr(pcc,
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

			error = pops->puffs_node_mmap(pcc,
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

			error = pops->puffs_node_revoke(pcc,
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

			error = pops->puffs_node_fsync(pcc,
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

			error = pops->puffs_node_seek(pcc,
			    preq->preq_cookie, auxt->pvnr_oldoff,
			    auxt->pvnr_newoff, &auxt->pvnr_cred);
			break;
		}

		case PUFFS_VN_REMOVE:
		{
			struct puffs_vnreq_remove *auxt = auxbuf;
			struct puffs_cn pcn;
			if (pops->puffs_node_remove == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;

			error = pops->puffs_node_remove(pcc,
			    preq->preq_cookie, auxt->pvnr_cookie_targ, &pcn);
			break;
		}

		case PUFFS_VN_LINK:
		{
			struct puffs_vnreq_link *auxt = auxbuf;
			struct puffs_cn pcn;
			if (pops->puffs_node_link == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;

			error = pops->puffs_node_link(pcc,
			    preq->preq_cookie, auxt->pvnr_cookie_targ, &pcn);
			break;
		}

		case PUFFS_VN_RENAME:
		{
			struct puffs_vnreq_rename *auxt = auxbuf;
			struct puffs_cn pcn_src, pcn_targ;
			struct puffs_node *pn_src;

			if (pops->puffs_node_rename == NULL) {
				error = 0;
				break;
			}

			pcn_src.pcn_pkcnp = &auxt->pvnr_cn_src;
			pcn_targ.pcn_pkcnp = &auxt->pvnr_cn_targ;
			if (buildpath) {
				pn_src = auxt->pvnr_cookie_src;
				pcn_src.pcn_fullpath = pn_src->pn_path;
				pcn_src.pcn_fullplen = pn_src->pn_plen;

				error = do_buildpath(&pcn_targ,
				    auxt->pvnr_cookie_targdir);
				if (error)
					break;
			}

			error = pops->puffs_node_rename(pcc,
			    preq->preq_cookie, auxt->pvnr_cookie_src,
			    &pcn_src, auxt->pvnr_cookie_targdir,
			    auxt->pvnr_cookie_targ, &pcn_targ);

			if (buildpath) {
				if (error) {
					free(pcn_targ.pcn_fullpath);
				} else {
					free(pn_src->pn_path);
					pn_src->pn_path = pcn_targ.pcn_fullpath;
					pn_src->pn_plen = pcn_targ.pcn_fullplen;
				}
			}
			break;
		}

		case PUFFS_VN_MKDIR:
		{
			struct puffs_vnreq_mkdir *auxt = auxbuf;
			struct puffs_cn pcn;
			if (pops->puffs_node_mkdir == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			if (buildpath) {
				error = do_buildpath(&pcn, preq->preq_cookie);
				if (error)
					break;
			}

			error = pops->puffs_node_mkdir(pcc,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &pcn, &auxt->pvnr_va);

			if (buildpath) {
				if (error) {
					free(pcn.pcn_fullpath);
				} else {
					struct puffs_node *pn;

					pn = auxt->pvnr_newnode;
					pn->pn_path = pcn.pcn_fullpath;
					pn->pn_plen = pcn.pcn_fullplen;
				}
			}

			break;
		}

		case PUFFS_VN_RMDIR:
		{
			struct puffs_vnreq_rmdir *auxt = auxbuf;
			struct puffs_cn pcn;
			if (pops->puffs_node_rmdir == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;

			error = pops->puffs_node_rmdir(pcc,
			    preq->preq_cookie, auxt->pvnr_cookie_targ, &pcn);
			break;
		}

		case PUFFS_VN_SYMLINK:
		{
			struct puffs_vnreq_symlink *auxt = auxbuf;
			struct puffs_cn pcn;
			if (pops->puffs_node_symlink == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			if (buildpath) {
				error = do_buildpath(&pcn, preq->preq_cookie);
				if (error)
					break;
			}

			error = pops->puffs_node_symlink(pcc,
			    preq->preq_cookie, &auxt->pvnr_newnode,
			    &pcn, &auxt->pvnr_va, auxt->pvnr_link);

			if (buildpath) {
				if (error) {
					free(pcn.pcn_fullpath);
				} else {
					struct puffs_node *pn;

					pn = auxt->pvnr_newnode;
					pn->pn_path = pcn.pcn_fullpath;
					pn->pn_plen = pcn.pcn_fullplen;
				}
			}

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
			error = pops->puffs_node_readdir(pcc,
			    preq->preq_cookie, auxt->pvnr_dent,
			    &auxt->pvnr_cred, &auxt->pvnr_offset,
			    &auxt->pvnr_resid);

			/* need to move a bit more */
			preq->preq_buflen = sizeof(struct puffs_vnreq_readdir) 
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

			error = pops->puffs_node_readlink(pcc,
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

			error = pops->puffs_node_reclaim(pcc,
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

			error = pops->puffs_node_inactive(pcc,
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

			error = pops->puffs_node_pathconf(pcc,
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

			error = pops->puffs_node_advlock(pcc,
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

			error = pops->puffs_node_print(pcc,
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
			error = pops->puffs_node_read(pcc,
			    preq->preq_cookie, auxt->pvnr_data,
			    auxt->pvnr_offset, &auxt->pvnr_resid,
			    &auxt->pvnr_cred, auxt->pvnr_ioflag);

			/* need to move a bit more */
			preq->preq_buflen = sizeof(struct puffs_vnreq_read)
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

			error = pops->puffs_node_write(pcc,
			    preq->preq_cookie, auxt->pvnr_data,
			    auxt->pvnr_offset, &auxt->pvnr_resid,
			    &auxt->pvnr_cred, auxt->pvnr_ioflag);

			/* don't need to move data back to the kernel */
			preq->preq_buflen = sizeof(struct puffs_vnreq_write);
			break;
		}

/* holy bitrot, ryydman! */
#if 0
		case PUFFS_VN_IOCTL:
			error = pops->puffs_node_ioctl1(pcc, preq->preq_cookie,
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
			error = pops->puffs_node_ioctl2(pcc, preq->preq_cookie,
			    (struct puffs_vnreq_ioctl *)auxbuf, &pop);
			break;

		case PUFFS_VN_FCNTL:
			error = pops->puffs_node_fcntl1(pcc, preq->preq_cookie,
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
			error = pops->puffs_node_fcntl2(pcc, preq->preq_cookie,
			    (struct puffs_vnreq_fcntl *)auxbuf, &pop);
			break;
#endif

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

	pcc->pcc_rv = rv;
	pcc->pcc_flags |= PCC_DONE;
}

static int
do_buildpath(struct puffs_cn *pcn, void *parent)
{
	struct puffs_node *pn_parent;
	size_t plen;

	pn_parent = parent; /* XXX */
	assert(pn_parent->pn_path != NULL);

	/* +1 not for nul but for / */
	plen = pn_parent->pn_plen + pcn->pcn_namelen + 1;
	pcn->pcn_fullpath = malloc(plen);
	if (!pcn->pcn_fullpath)
		return errno;

	strcpy(pcn->pcn_fullpath, pn_parent->pn_path);
	strcat(pcn->pcn_fullpath, "/");
	strcat(pcn->pcn_fullpath, pcn->pcn_name);
	pcn->pcn_fullpath[plen-1] = '\0'; /* paranoia */
	pcn->pcn_fullplen = plen;

	return 0;
}


#if 0
		case PUFFS_VN_POLL:
		{
			struct puffs_vnreq_poll *auxt = auxbuf;
			if (pops->puffs_node_poll == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_poll(pcc,
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

			error = pops->puffs_node_kqfilter(pcc,
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

			error = pops->puffs_closeextattr(pcc,
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

			error = pops->puffs_getextattr(pcc,
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

			error = pops->puffs_listextattr(pcc,
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

			error = pops->puffs_openextattr(pcc,
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

			error = pops->puffs_deleteextattr(pcc,
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

			error = pops->puffs_setextattr(pcc,
			    preq->preq_cookie, );
			break;
		}

#endif
