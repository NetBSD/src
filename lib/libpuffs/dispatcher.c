/*	$NetBSD: dispatcher.c,v 1.27 2007/12/16 20:02:57 pooka Exp $	*/

/*
 * Copyright (c) 2006, 2007  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Ulla Tuominen Foundation and the Finnish Cultural Foundation.
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
__RCSID("$NetBSD: dispatcher.c,v 1.27 2007/12/16 20:02:57 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/poll.h>

#include <assert.h>
#include <errno.h>
#ifdef PUFFS_WITH_THREADS
#include <pthread.h>
#endif
#include <puffs.h>
#include <puffsdump.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "puffs_priv.h"

static void processresult(struct puffs_usermount *, int);

/*
 * Set the following to 1 to not handle each request on a separate
 * stack.  This is highly volatile kludge, therefore no external
 * interface.
 */
int puffs_fakecc;

/*
 * Set the following to 1 to handle each request in a separate pthread.
 * This is not exported as it should not be used yet unless having a
 * very good knowledge of what you're signing up for (libpuffs is not
 * threadsafe).
 */
int puffs_usethreads;

static int
dopufbuf2(struct puffs_usermount *pu, struct puffs_framebuf *pb)
{
	struct puffs_req *preq = puffs__framebuf_getdataptr(pb);
	struct puffs_cc *pcc;
	int type;

	if (puffs_fakecc) {
		type = PCC_FAKECC;
	} else if (puffs_usethreads) {
		type = PCC_THREADED;
#ifndef PUFFS_WITH_THREADS
		type = PCC_FAKECC;
#endif
	} else {
		type = PCC_REALCC;
	}

	if (puffs_cc_create(pu, pb, type, &pcc) == -1)
		return errno;

	puffs_cc_setcaller(pcc, preq->preq_pid, preq->preq_lid);

#ifdef PUFFS_WITH_THREADS
	if (puffs_usethreads) {
		pthread_attr_t pattr;
		pthread_t ptid;
		int rv;

		pthread_attr_init(&pattr);
		pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
		pthread_attr_destroy(&pattr);

		ap->pwa_pcc = pcc;

		rv = pthread_create(&ptid, &pattr, puffs_docc, pcc);

		return rv;
	}
#endif
	puffs_docc(pcc);
	return 0;
}

/*
 * User-visible point to handle a request from.
 *
 * <insert text here>
 */
int
puffs_dopufbuf(struct puffs_usermount *pu, struct puffs_framebuf *pb)
{
	struct puffs_req *preq = puffs__framebuf_getdataptr(pb);
	struct puffs_executor *pex;

	/*
	 * XXX: the structure is currently a mess.  anyway, trap
	 * the cacheops here already, since they don't need a cc.
	 * I really should get around to revamping the operation
	 * dispatching code one of these days.
	 *
	 * Do the same for error notifications.
	 */
	if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_CACHE) {
		struct puffs_cacheinfo *pci = (void *)preq;

		if (pu->pu_ops.puffs_cache_write == NULL)
			return 0;

		pu->pu_ops.puffs_cache_write(pu, preq->preq_cookie,
		    pci->pcache_nruns, pci->pcache_runs);
		return 0;
	} else if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_ERROR) {
		struct puffs_error *perr = (void *)preq;

		pu->pu_errnotify(pu, preq->preq_optype,
		    perr->perr_error, perr->perr_str, preq->preq_cookie);
		return 0;
	}

	if (pu->pu_flags & PUFFS_FLAG_OPDUMP)
		puffsdump_req(preq);

	/*
	 * Check if we are already processing an operation from the same
	 * caller.  If so, queue this operation until the previous one
	 * finishes.  This prevents us from executing certain operations
	 * out-of-order (e.g. fsync and reclaim).
	 *
	 * Each preq will only remove its own pex from the tailq.
	 * See puffs_docc() for the details on other-end removal
	 * and dispatching.
	 */
	pex = malloc(sizeof(struct puffs_executor));
	pex->pex_pufbuf = pb;
	PU_LOCK();
	TAILQ_INSERT_TAIL(&pu->pu_exq, pex, pex_entries);
	TAILQ_FOREACH(pex, &pu->pu_exq, pex_entries) {
		struct puffs_req *pb_preq;

		pb_preq = puffs__framebuf_getdataptr(pex->pex_pufbuf);
		if (pb_preq->preq_pid == preq->preq_pid
		    && pb_preq->preq_lid == preq->preq_lid) {
			if (pb_preq != preq) {
				PU_UNLOCK();
				return 0;
			}
		}
	}
	PU_UNLOCK();

	return dopufbuf2(pu, pb);
}

enum {PUFFCALL_ANSWER, PUFFCALL_IGNORE, PUFFCALL_AGAIN};

void *
puffs_docc(void *arg)
{
	struct puffs_cc *pcc = arg;
	struct puffs_usermount *pu = pcc->pcc_pu;
	struct puffs_req *preq;
	struct puffs_cc *pcc_iter;
	struct puffs_executor *pex, *pex_n;
	int found;

	assert((pcc->pcc_flags & PCC_DONE) == 0);

	if (pcc->pcc_flags & PCC_REALCC)
		puffs_cc_continue(pcc);
	else
		puffs_calldispatcher(pcc);

	if ((pcc->pcc_flags & PCC_DONE) == 0) {
		assert(pcc->pcc_flags & PCC_REALCC);
		return NULL;
	}

	/* check if we need to schedule FAFs which were stalled */
	found = 0;
	preq = puffs__framebuf_getdataptr(pcc->pcc_pb);
	PU_LOCK();
	for (pex = TAILQ_FIRST(&pu->pu_exq); pex; pex = pex_n) {
		struct puffs_req *pb_preq;

		pb_preq = puffs__framebuf_getdataptr(pex->pex_pufbuf);
		pex_n = TAILQ_NEXT(pex, pex_entries);
		if (pb_preq->preq_pid == preq->preq_pid
		    && pb_preq->preq_lid == preq->preq_lid) {
			if (found == 0) {
				/* this is us */
				assert(pb_preq == preq);
				TAILQ_REMOVE(&pu->pu_exq, pex, pex_entries);
				free(pex);
				found = 1;
			} else {
				/* down at the mardi gras */
				PU_UNLOCK();
				dopufbuf2(pu, pex->pex_pufbuf);
				PU_LOCK();
				break;
			}
		}
	}

	if (!PUFFSOP_WANTREPLY(preq->preq_opclass))
		puffs_framebuf_destroy(pcc->pcc_pb);

	/* can't do this above due to PCC_BORROWED */
	while ((pcc_iter = LIST_FIRST(&pu->pu_ccnukelst)) != NULL) {
		LIST_REMOVE(pcc_iter, nlst_entries);
		puffs_cc_destroy(pcc_iter);
	}
	PU_UNLOCK();

	return NULL;
}

/* library private, but linked from callcontext.c */

void
puffs_calldispatcher(struct puffs_cc *pcc)
{
	struct puffs_usermount *pu = pcc->pcc_pu;
	struct puffs_ops *pops = &pu->pu_ops;
	struct puffs_req *preq = puffs__framebuf_getdataptr(pcc->pcc_pb);
	void *auxbuf = preq; /* help with typecasting */
	void *opcookie = preq->preq_cookie;
	int error, rv, buildpath;

	assert(pcc->pcc_flags & (PCC_FAKECC | PCC_REALCC | PCC_THREADED));
	assert(pcc == puffs_cc_getcc(pu)); /* remove me soon */

	if (PUFFSOP_WANTREPLY(preq->preq_opclass))
		rv = PUFFCALL_ANSWER;
	else
		rv = PUFFCALL_IGNORE;

	buildpath = pu->pu_flags & PUFFS_FLAG_BUILDPATH;
	preq->preq_setbacks = 0;

	if (pu->pu_oppre)
		pu->pu_oppre(pu);

	if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_VFS) {
		switch (preq->preq_optype) {
		case PUFFS_VFS_UNMOUNT:
		{
			struct puffs_vfsmsg_unmount *auxt = auxbuf;

			PU_SETSTATE(pu, PUFFS_STATE_UNMOUNTING);
			error = pops->puffs_fs_unmount(pu, auxt->pvfsr_flags);
			if (!error)
				PU_SETSTATE(pu, PUFFS_STATE_UNMOUNTED);
			else
				PU_SETSTATE(pu, PUFFS_STATE_RUNNING);
			break;
		}

		case PUFFS_VFS_STATVFS:
		{
			struct puffs_vfsmsg_statvfs *auxt = auxbuf;

			error = pops->puffs_fs_statvfs(pu, &auxt->pvfsr_sb);
			break;
		}

		case PUFFS_VFS_SYNC:
		{
			struct puffs_vfsmsg_sync *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvfsr_cred);

			error = pops->puffs_fs_sync(pu,
			    auxt->pvfsr_waitfor, pcr);
			break;
		}

		case PUFFS_VFS_FHTOVP:
		{
			struct puffs_vfsmsg_fhtonode *auxt = auxbuf;
			struct puffs_newinfo pni;

			pni.pni_cookie = &auxt->pvfsr_fhcookie;
			pni.pni_vtype = &auxt->pvfsr_vtype;
			pni.pni_size = &auxt->pvfsr_size;
			pni.pni_rdev = &auxt->pvfsr_rdev;

			error = pops->puffs_fs_fhtonode(pu, auxt->pvfsr_data,
			    auxt->pvfsr_dsize, &pni);

			break;
		}

		case PUFFS_VFS_VPTOFH:
		{
			struct puffs_vfsmsg_nodetofh *auxt = auxbuf;

			error = pops->puffs_fs_nodetofh(pu,
			    auxt->pvfsr_fhcookie, auxt->pvfsr_data,
			    &auxt->pvfsr_dsize);

			break;
		}

		case PUFFS_VFS_SUSPEND:
		{
			struct puffs_vfsmsg_suspend *auxt = auxbuf;

			error = 0;
			if (pops->puffs_fs_suspend == NULL)
				break;

			pops->puffs_fs_suspend(pu, auxt->pvfsr_status);
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
			struct puffs_vnmsg_lookup *auxt = auxbuf;
			struct puffs_newinfo pni;
			struct puffs_cn pcn;

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			PUFFS_KCREDTOCRED(pcn.pcn_cred, &auxt->pvnr_cn_cred);
			pni.pni_cookie = &auxt->pvnr_newnode;
			pni.pni_vtype = &auxt->pvnr_vtype;
			pni.pni_size = &auxt->pvnr_size;
			pni.pni_rdev = &auxt->pvnr_rdev;

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			/* lookup *must* be present */
			error = pops->puffs_node_lookup(pu, opcookie,
			    &pni, &pcn);

			if (buildpath) {
				if (error) {
					pu->pu_pathfree(pu, &pcn.pcn_po_full);
				} else {
					struct puffs_node *pn;

					/*
					 * did we get a new node or a
					 * recycled node?
					 */
					pn = PU_CMAP(pu, auxt->pvnr_newnode);
					if (pn->pn_po.po_path == NULL)
						pn->pn_po = pcn.pcn_po_full;
					else
						pu->pu_pathfree(pu,
						    &pcn.pcn_po_full);
				}
			}

			break;
		}

		case PUFFS_VN_CREATE:
		{
			struct puffs_vnmsg_create *auxt = auxbuf;
			struct puffs_newinfo pni;
			struct puffs_cn pcn;

			if (pops->puffs_node_create == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			PUFFS_KCREDTOCRED(pcn.pcn_cred, &auxt->pvnr_cn_cred);

			memset(&pni, 0, sizeof(pni));
			pni.pni_cookie = &auxt->pvnr_newnode;

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			error = pops->puffs_node_create(pu,
			    opcookie, &pni, &pcn, &auxt->pvnr_va);

			if (buildpath) {
				if (error) {
					pu->pu_pathfree(pu, &pcn.pcn_po_full);
				} else {
					struct puffs_node *pn;

					pn = PU_CMAP(pu, auxt->pvnr_newnode);
					pn->pn_po = pcn.pcn_po_full;
				}
			}

			break;
		}

		case PUFFS_VN_MKNOD:
		{
			struct puffs_vnmsg_mknod *auxt = auxbuf;
			struct puffs_newinfo pni;
			struct puffs_cn pcn;

			if (pops->puffs_node_mknod == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			PUFFS_KCREDTOCRED(pcn.pcn_cred, &auxt->pvnr_cn_cred);

			memset(&pni, 0, sizeof(pni));
			pni.pni_cookie = &auxt->pvnr_newnode;

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			error = pops->puffs_node_mknod(pu,
			    opcookie, &pni, &pcn, &auxt->pvnr_va);

			if (buildpath) {
				if (error) {
					pu->pu_pathfree(pu, &pcn.pcn_po_full);
				} else {
					struct puffs_node *pn;

					pn = PU_CMAP(pu, auxt->pvnr_newnode);
					pn->pn_po = pcn.pcn_po_full;
				}
			}

			break;
		}

		case PUFFS_VN_OPEN:
		{
			struct puffs_vnmsg_open *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);

			if (pops->puffs_node_open == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_open(pu,
			    opcookie, auxt->pvnr_mode, pcr);
			break;
		}

		case PUFFS_VN_CLOSE:
		{
			struct puffs_vnmsg_close *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);

			if (pops->puffs_node_close == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_close(pu,
			    opcookie, auxt->pvnr_fflag, pcr);
			break;
		}

		case PUFFS_VN_ACCESS:
		{
			struct puffs_vnmsg_access *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);

			if (pops->puffs_node_access == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_access(pu,
			    opcookie, auxt->pvnr_mode, pcr);
			break;
		}

		case PUFFS_VN_GETATTR:
		{
			struct puffs_vnmsg_getattr *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);

			if (pops->puffs_node_getattr == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pops->puffs_node_getattr(pu,
			    opcookie, &auxt->pvnr_va, pcr);
			break;
		}

		case PUFFS_VN_SETATTR:
		{
			struct puffs_vnmsg_setattr *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);

			if (pops->puffs_node_setattr == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pops->puffs_node_setattr(pu,
			    opcookie, &auxt->pvnr_va, pcr);
			break;
		}

		case PUFFS_VN_MMAP:
		{
			struct puffs_vnmsg_mmap *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);

			if (pops->puffs_node_mmap == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_mmap(pu,
			    opcookie, auxt->pvnr_prot, pcr);
			break;
		}

		case PUFFS_VN_FSYNC:
		{
			struct puffs_vnmsg_fsync *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);

			if (pops->puffs_node_fsync == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_fsync(pu, opcookie, pcr,
			    auxt->pvnr_flags, auxt->pvnr_offlo,
			    auxt->pvnr_offhi);
			break;
		}

		case PUFFS_VN_SEEK:
		{
			struct puffs_vnmsg_seek *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);

			if (pops->puffs_node_seek == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_seek(pu,
			    opcookie, auxt->pvnr_oldoff,
			    auxt->pvnr_newoff, pcr);
			break;
		}

		case PUFFS_VN_REMOVE:
		{
			struct puffs_vnmsg_remove *auxt = auxbuf;
			struct puffs_cn pcn;
			if (pops->puffs_node_remove == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			PUFFS_KCREDTOCRED(pcn.pcn_cred, &auxt->pvnr_cn_cred);

			error = pops->puffs_node_remove(pu,
			    opcookie, auxt->pvnr_cookie_targ, &pcn);
			break;
		}

		case PUFFS_VN_LINK:
		{
			struct puffs_vnmsg_link *auxt = auxbuf;
			struct puffs_cn pcn;
			if (pops->puffs_node_link == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			PUFFS_KCREDTOCRED(pcn.pcn_cred, &auxt->pvnr_cn_cred);

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			error = pops->puffs_node_link(pu,
			    opcookie, auxt->pvnr_cookie_targ, &pcn);
			if (buildpath)
				pu->pu_pathfree(pu, &pcn.pcn_po_full);

			break;
		}

		case PUFFS_VN_RENAME:
		{
			struct puffs_vnmsg_rename *auxt = auxbuf;
			struct puffs_cn pcn_src, pcn_targ;
			struct puffs_node *pn_src;

			if (pops->puffs_node_rename == NULL) {
				error = 0;
				break;
			}

			pcn_src.pcn_pkcnp = &auxt->pvnr_cn_src;
			PUFFS_KCREDTOCRED(pcn_src.pcn_cred,
			    &auxt->pvnr_cn_src_cred);

			pcn_targ.pcn_pkcnp = &auxt->pvnr_cn_targ;
			PUFFS_KCREDTOCRED(pcn_targ.pcn_cred,
			    &auxt->pvnr_cn_targ_cred);

			if (buildpath) {
				pn_src = auxt->pvnr_cookie_src;
				pcn_src.pcn_po_full = pn_src->pn_po;

				error = puffs_path_pcnbuild(pu, &pcn_targ,
				    auxt->pvnr_cookie_targdir);
				if (error)
					break;
			}

			error = pops->puffs_node_rename(pu,
			    opcookie, auxt->pvnr_cookie_src,
			    &pcn_src, auxt->pvnr_cookie_targdir,
			    auxt->pvnr_cookie_targ, &pcn_targ);

			if (buildpath) {
				if (error) {
					pu->pu_pathfree(pu,
					    &pcn_targ.pcn_po_full);
				} else {
					struct puffs_pathinfo pi;
					struct puffs_pathobj po_old;

					/* handle this node */
					po_old = pn_src->pn_po;
					pn_src->pn_po = pcn_targ.pcn_po_full;

					if (pn_src->pn_va.va_type != VDIR) {
						pu->pu_pathfree(pu, &po_old);
						break;
					}

					/* handle all child nodes for DIRs */
					pi.pi_old = &pcn_src.pcn_po_full;
					pi.pi_new = &pcn_targ.pcn_po_full;

					PU_LOCK();
					if (puffs_pn_nodewalk(pu,
					    puffs_path_prefixadj, &pi) != NULL)
						error = ENOMEM;
					PU_UNLOCK();
					pu->pu_pathfree(pu, &po_old);
				}
			}
			break;
		}

		case PUFFS_VN_MKDIR:
		{
			struct puffs_vnmsg_mkdir *auxt = auxbuf;
			struct puffs_newinfo pni;
			struct puffs_cn pcn;

			if (pops->puffs_node_mkdir == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			PUFFS_KCREDTOCRED(pcn.pcn_cred, &auxt->pvnr_cn_cred);

			memset(&pni, 0, sizeof(pni));
			pni.pni_cookie = &auxt->pvnr_newnode;

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			error = pops->puffs_node_mkdir(pu,
			    opcookie, &pni, &pcn, &auxt->pvnr_va);

			if (buildpath) {
				if (error) {
					pu->pu_pathfree(pu, &pcn.pcn_po_full);
				} else {
					struct puffs_node *pn;

					pn = PU_CMAP(pu, auxt->pvnr_newnode);
					pn->pn_po = pcn.pcn_po_full;
				}
			}

			break;
		}

		case PUFFS_VN_RMDIR:
		{
			struct puffs_vnmsg_rmdir *auxt = auxbuf;
			struct puffs_cn pcn;
			if (pops->puffs_node_rmdir == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			PUFFS_KCREDTOCRED(pcn.pcn_cred, &auxt->pvnr_cn_cred);

			error = pops->puffs_node_rmdir(pu,
			    opcookie, auxt->pvnr_cookie_targ, &pcn);
			break;
		}

		case PUFFS_VN_SYMLINK:
		{
			struct puffs_vnmsg_symlink *auxt = auxbuf;
			struct puffs_newinfo pni;
			struct puffs_cn pcn;

			if (pops->puffs_node_symlink == NULL) {
				error = 0;
				break;
			}

			pcn.pcn_pkcnp = &auxt->pvnr_cn;
			PUFFS_KCREDTOCRED(pcn.pcn_cred, &auxt->pvnr_cn_cred);

			memset(&pni, 0, sizeof(pni));
			pni.pni_cookie = &auxt->pvnr_newnode;

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			error = pops->puffs_node_symlink(pu,
			    opcookie, &pni, &pcn,
			    &auxt->pvnr_va, auxt->pvnr_link);

			if (buildpath) {
				if (error) {
					pu->pu_pathfree(pu, &pcn.pcn_po_full);
				} else {
					struct puffs_node *pn;

					pn = PU_CMAP(pu, auxt->pvnr_newnode);
					pn->pn_po = pcn.pcn_po_full;
				}
			}

			break;
		}

		case PUFFS_VN_READDIR:
		{
			struct puffs_vnmsg_readdir *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);
			struct dirent *dent;
			off_t *cookies;
			size_t res, origcookies;

			if (pops->puffs_node_readdir == NULL) {
				error = 0;
				break;
			}

			if (auxt->pvnr_ncookies) {
				/* LINTED: pvnr_data is __aligned() */
				cookies = (off_t *)auxt->pvnr_data;
				origcookies = auxt->pvnr_ncookies;
			} else {
				cookies = NULL;
				origcookies = 0;
			}
			/* LINTED: dentoff is aligned in the kernel */
			dent = (struct dirent *)
			    (auxt->pvnr_data + auxt->pvnr_dentoff);

			res = auxt->pvnr_resid;
			error = pops->puffs_node_readdir(pu,
			    opcookie, dent, &auxt->pvnr_offset,
			    &auxt->pvnr_resid, pcr, &auxt->pvnr_eofflag,
			    cookies, &auxt->pvnr_ncookies);

			/* much easier to track non-working NFS */
			assert(auxt->pvnr_ncookies <= origcookies);

			/* need to move a bit more */
			preq->preq_buflen = sizeof(struct puffs_vnmsg_readdir) 
			    + auxt->pvnr_dentoff + (res - auxt->pvnr_resid);
			break;
		}

		case PUFFS_VN_READLINK:
		{
			struct puffs_vnmsg_readlink *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);

			if (pops->puffs_node_readlink == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			/*LINTED*/
			error = pops->puffs_node_readlink(pu, opcookie, pcr,
			    auxt->pvnr_link, &auxt->pvnr_linklen);
			break;
		}

		case PUFFS_VN_RECLAIM:
		{

			if (pops->puffs_node_reclaim == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_reclaim(pu, opcookie);
			break;
		}

		case PUFFS_VN_INACTIVE:
		{

			if (pops->puffs_node_inactive == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pops->puffs_node_inactive(pu, opcookie);
			break;
		}

		case PUFFS_VN_PATHCONF:
		{
			struct puffs_vnmsg_pathconf *auxt = auxbuf;
			if (pops->puffs_node_pathconf == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_pathconf(pu,
			    opcookie, auxt->pvnr_name,
			    &auxt->pvnr_retval);
			break;
		}

		case PUFFS_VN_ADVLOCK:
		{
			struct puffs_vnmsg_advlock *auxt = auxbuf;
			if (pops->puffs_node_advlock == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_advlock(pu,
			    opcookie, auxt->pvnr_id, auxt->pvnr_op,
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
			    opcookie);
			break;
		}

		case PUFFS_VN_READ:
		{
			struct puffs_vnmsg_read *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);
			size_t res;

			if (pops->puffs_node_read == NULL) {
				error = EIO;
				break;
			}

			res = auxt->pvnr_resid;
			error = pops->puffs_node_read(pu,
			    opcookie, auxt->pvnr_data,
			    auxt->pvnr_offset, &auxt->pvnr_resid,
			    pcr, auxt->pvnr_ioflag);

			/* need to move a bit more */
			preq->preq_buflen = sizeof(struct puffs_vnmsg_read)
			    + (res - auxt->pvnr_resid);
			break;
		}

		case PUFFS_VN_WRITE:
		{
			struct puffs_vnmsg_write *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);

			if (pops->puffs_node_write == NULL) {
				error = EIO;
				break;
			}

			error = pops->puffs_node_write(pu,
			    opcookie, auxt->pvnr_data,
			    auxt->pvnr_offset, &auxt->pvnr_resid,
			    pcr, auxt->pvnr_ioflag);

			/* don't need to move data back to the kernel */
			preq->preq_buflen = sizeof(struct puffs_vnmsg_write);
			break;
		}

		case PUFFS_VN_POLL:
		{
			struct puffs_vnmsg_poll *auxt = auxbuf;

			if (pops->puffs_node_poll == NULL) {
				error = 0;

				/* emulate genfs_poll() */
				auxt->pvnr_events &= (POLLIN | POLLOUT
						    | POLLRDNORM | POLLWRNORM);

				break;
			}

			error = pops->puffs_node_poll(pu,
			    opcookie, &auxt->pvnr_events);
			break;
		}

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
	pcc->pcc_flags |= PCC_DONE;

	if (pu->pu_oppost)
		pu->pu_oppost(pu);

	/*
	 * Note, we are calling this from here so that we can run it
	 * off of the continuation stack.  Otherwise puffs_goto() would
	 * not work.
	 */
	processresult(pu, rv);
}

static void
processresult(struct puffs_usermount *pu, int how)
{
	struct puffs_cc *pcc = puffs_cc_getcc(pu);
	struct puffs_req *preq = puffs__framebuf_getdataptr(pcc->pcc_pb);
	int pccflags = pcc->pcc_flags;

	/* check if we need to store this reply */
	switch (how) {
	case PUFFCALL_ANSWER:
		if (pu->pu_flags & PUFFS_FLAG_OPDUMP)
			puffsdump_rv(preq);

		puffs_framev_enqueue_justsend(pu, pu->pu_fd,
		    pcc->pcc_pb, 0, 0);
		/*FALLTHROUGH*/

	case PUFFCALL_IGNORE:
		PU_LOCK();
		LIST_INSERT_HEAD(&pu->pu_ccnukelst, pcc, nlst_entries);
		PU_UNLOCK();
		break;

	case PUFFCALL_AGAIN:
		if ((pcc->pcc_flags & PCC_REALCC) == 0)
			assert(pcc->pcc_flags & PCC_DONE);
		break;

	default:
		assert(/*CONSTCOND*/0);
	}

	/* who needs information when you're living on borrowed time? */
	if (pccflags & PCC_BORROWED) {
		assert((pccflags & PCC_THREADED) == 0);
		puffs_cc_yield(pcc); /* back to borrow source */
	}
}
