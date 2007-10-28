/*	$NetBSD: dispatcher.c,v 1.18 2007/10/28 18:40:30 pooka Exp $	*/

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
__RCSID("$NetBSD: dispatcher.c,v 1.18 2007/10/28 18:40:30 pooka Exp $");
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

static void processresult(struct puffs_cc *, struct puffs_putreq *, int);

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

#ifdef PUFFS_WITH_THREADS
struct puffs_workerargs {
	struct puffs_cc *pwa_pcc;
	struct puffs_putreq *pwa_ppr;
};

static void *
threadlauncher(void *v)
{
	struct puffs_workerargs *ap = v;
	struct puffs_cc *pcc = ap->pwa_pcc;
	struct puffs_putreq *ppr = ap->pwa_ppr;

	free(ap);
	puffs_docc(pcc, ppr);
	return NULL;
}
#endif

static int
dopreq2(struct puffs_usermount *pu, struct puffs_req *preq,
	struct puffs_putreq *ppr)
{
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

	if (puffs_cc_create(pu, preq, type, &pcc) == -1)
		return errno;

	puffs_cc_setcaller(pcc, preq->preq_pid, preq->preq_lid);

#ifdef PUFFS_WITH_THREADS
	if (puffs_usethreads) {
		struct puffs_workerargs *ap;
		pthread_attr_t pattr;
		pthread_t ptid;
		int rv;

		ap = malloc(sizeof(struct puffs_workerargs));
		pcc = malloc(sizeof(struct puffs_cc));
		pcc_init_unreal(pcc, PCC_THREADED);
		pcc->pcc_pu = pu;
		pcc->pcc_preq = preq;

		pthread_attr_init(&pattr);
		pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);

		ap->pwa_pcc = pcc;
		ap->pwa_ppr = ppr;

		rv = pthread_create(&ptid, &pattr, threadlauncher, ap);

		return rv;
	}
#endif
	puffs_docc(pcc, ppr);
	return 0;
}

/* user-visible point to handle a request from */
int
puffs_dopreq(struct puffs_usermount *pu, struct puffs_req *preq,
	struct puffs_putreq *ppr)
{
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
	pex->pex_preq = preq;
	/* mutex_enter */
	TAILQ_INSERT_TAIL(&pu->pu_exq, pex, pex_entries);
	TAILQ_FOREACH(pex, &pu->pu_exq, pex_entries) {
		if (pex->pex_preq->preq_pid == preq->preq_pid
		    && pex->pex_preq->preq_lid == preq->preq_lid) {
			if (pex->pex_preq != preq) {
				/* mutex_exit */
				return 0;
			}
		}
	}

	return dopreq2(pu, preq, ppr);
}

enum {PUFFCALL_ANSWER, PUFFCALL_IGNORE, PUFFCALL_AGAIN};

/* user-visible continuation point */
void
puffs_docc(struct puffs_cc *pcc, struct puffs_putreq *ppr)
{
	struct puffs_usermount *pu = pcc->pcc_pu;
	struct puffs_req *preq;
	struct puffs_cc *pcc_iter;
	struct puffs_executor *pex;
	int found;

	assert((pcc->pcc_flags & PCC_DONE) == 0);
	pcc->pcc_ppr = ppr;

	if (pcc->pcc_flags & PCC_REALCC)
		puffs_cc_continue(pcc);
	else
		puffs_calldispatcher(pcc);

	/* check if we need to schedule FAFs which were stalled */
	found = 0;
	preq = pcc->pcc_preq;
	/* mutex_enter */
	TAILQ_FOREACH(pex, &pu->pu_exq, pex_entries) {
		if (pex->pex_preq->preq_pid == preq->preq_pid
		    && pex->pex_preq->preq_lid == preq->preq_lid) {
			if (found == 0) {
				/* this is us */
				assert(pex->pex_preq == preq);
				TAILQ_REMOVE(&pu->pu_exq, pex, pex_entries);
				free(pex);
				found = 1;
			} else {
				/* down at the mardi gras */
				dopreq2(pu, pex->pex_preq, ppr);
				break;
			}
		}
	}
	/* mutex_exit */

	/* can't do this above due to PCC_BORROWED */
	while ((pcc_iter = LIST_FIRST(&pu->pu_ccnukelst)) != NULL) {
		LIST_REMOVE(pcc_iter, nlst_entries);
		puffs_cc_destroy(pcc_iter);
	}
}

/* library private, but linked from callcontext.c */

void
puffs_calldispatcher(struct puffs_cc *pcc)
{
	struct puffs_usermount *pu = pcc->pcc_pu;
	struct puffs_ops *pops = &pu->pu_ops;
	struct puffs_req *preq = pcc->pcc_preq;
	void *auxbuf = preq; /* help with typecasting */
	void *opcookie = preq->preq_cookie;
	int error, rv, buildpath;

	assert(pcc->pcc_flags & (PCC_FAKECC | PCC_REALCC | PCC_THREADED));

	if (PUFFSOP_WANTREPLY(preq->preq_opclass))
		rv = PUFFCALL_ANSWER;
	else
		rv = PUFFCALL_IGNORE;

	buildpath = pu->pu_flags & PUFFS_FLAG_BUILDPATH;
	preq->preq_setbacks = 0;

	if (pu->pu_oppre)
		pu->pu_oppre(pcc);

	if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_VFS) {
		switch (preq->preq_optype) {
		case PUFFS_VFS_UNMOUNT:
		{
			struct puffs_vfsmsg_unmount *auxt = auxbuf;
			PUFFS_MAKECID(pcid, &auxt->pvfsr_cid);

			PU_SETSTATE(pu, PUFFS_STATE_UNMOUNTING);
			error = pops->puffs_fs_unmount(pcc,
			    auxt->pvfsr_flags, pcid);
			if (!error)
				PU_SETSTATE(pu, PUFFS_STATE_UNMOUNTED);
			else
				PU_SETSTATE(pu, PUFFS_STATE_RUNNING);
			break;
		}

		case PUFFS_VFS_STATVFS:
		{
			struct puffs_vfsmsg_statvfs *auxt = auxbuf;
			PUFFS_MAKECID(pcid, &auxt->pvfsr_cid);

			error = pops->puffs_fs_statvfs(pcc,
			    &auxt->pvfsr_sb, pcid);
			break;
		}

		case PUFFS_VFS_SYNC:
		{
			struct puffs_vfsmsg_sync *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvfsr_cred);
			PUFFS_MAKECID(pcid, &auxt->pvfsr_cid);

			error = pops->puffs_fs_sync(pcc,
			    auxt->pvfsr_waitfor, pcr, pcid);
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

			error = pops->puffs_fs_fhtonode(pcc, auxt->pvfsr_data,
			    auxt->pvfsr_dsize, &pni);

			break;
		}

		case PUFFS_VFS_VPTOFH:
		{
			struct puffs_vfsmsg_nodetofh *auxt = auxbuf;

			error = pops->puffs_fs_nodetofh(pcc,
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

			pops->puffs_fs_suspend(pcc, auxt->pvfsr_status);
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
			PUFFS_KCIDTOCID(pcn.pcn_cid, &auxt->pvnr_cn_cid);
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
			error = pops->puffs_node_lookup(pcc, opcookie,
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
			PUFFS_KCIDTOCID(pcn.pcn_cid, &auxt->pvnr_cn_cid);

			memset(&pni, 0, sizeof(pni));
			pni.pni_cookie = &auxt->pvnr_newnode;

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			error = pops->puffs_node_create(pcc,
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
			PUFFS_KCIDTOCID(pcn.pcn_cid, &auxt->pvnr_cn_cid);

			memset(&pni, 0, sizeof(pni));
			pni.pni_cookie = &auxt->pvnr_newnode;

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			error = pops->puffs_node_mknod(pcc,
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
			PUFFS_MAKECID(pcid, &auxt->pvnr_cid);

			if (pops->puffs_node_open == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_open(pcc,
			    opcookie, auxt->pvnr_mode, pcr, pcid);
			break;
		}

		case PUFFS_VN_CLOSE:
		{
			struct puffs_vnmsg_close *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);
			PUFFS_MAKECID(pcid, &auxt->pvnr_cid);


			if (pops->puffs_node_close == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_close(pcc,
			    opcookie, auxt->pvnr_fflag, pcr, pcid);
			break;
		}

		case PUFFS_VN_ACCESS:
		{
			struct puffs_vnmsg_access *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);
			PUFFS_MAKECID(pcid, &auxt->pvnr_cid);


			if (pops->puffs_node_access == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_access(pcc,
			    opcookie, auxt->pvnr_mode, pcr, pcid);
			break;
		}

		case PUFFS_VN_GETATTR:
		{
			struct puffs_vnmsg_getattr *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);
			PUFFS_MAKECID(pcid, &auxt->pvnr_cid);


			if (pops->puffs_node_getattr == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pops->puffs_node_getattr(pcc,
			    opcookie, &auxt->pvnr_va, pcr, pcid);
			break;
		}

		case PUFFS_VN_SETATTR:
		{
			struct puffs_vnmsg_setattr *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);
			PUFFS_MAKECID(pcid, &auxt->pvnr_cid);


			if (pops->puffs_node_setattr == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pops->puffs_node_setattr(pcc,
			    opcookie, &auxt->pvnr_va, pcr, pcid);
			break;
		}

		case PUFFS_VN_MMAP:
		{
			struct puffs_vnmsg_mmap *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);
			PUFFS_MAKECID(pcid, &auxt->pvnr_cid);


			if (pops->puffs_node_mmap == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_mmap(pcc,
			    opcookie, auxt->pvnr_prot, pcr, pcid);
			break;
		}

		case PUFFS_VN_FSYNC:
		{
			struct puffs_vnmsg_fsync *auxt = auxbuf;
			PUFFS_MAKECRED(pcr, &auxt->pvnr_cred);
			PUFFS_MAKECID(pcid, &auxt->pvnr_cid);


			if (pops->puffs_node_fsync == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_fsync(pcc, opcookie, pcr,
			    auxt->pvnr_flags, auxt->pvnr_offlo,
			    auxt->pvnr_offhi, pcid);
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

			error = pops->puffs_node_seek(pcc,
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
			PUFFS_KCIDTOCID(pcn.pcn_cid, &auxt->pvnr_cn_cid);

			error = pops->puffs_node_remove(pcc,
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
			PUFFS_KCIDTOCID(pcn.pcn_cid, &auxt->pvnr_cn_cid);

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			error = pops->puffs_node_link(pcc,
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
			PUFFS_KCIDTOCID(pcn_src.pcn_cid,
			    &auxt->pvnr_cn_src_cid);

			pcn_targ.pcn_pkcnp = &auxt->pvnr_cn_targ;
			PUFFS_KCREDTOCRED(pcn_targ.pcn_cred,
			    &auxt->pvnr_cn_targ_cred);
			PUFFS_KCIDTOCID(pcn_targ.pcn_cid,
			    &auxt->pvnr_cn_targ_cid);

			if (buildpath) {
				pn_src = auxt->pvnr_cookie_src;
				pcn_src.pcn_po_full = pn_src->pn_po;

				error = puffs_path_pcnbuild(pu, &pcn_targ,
				    auxt->pvnr_cookie_targdir);
				if (error)
					break;
			}

			error = pops->puffs_node_rename(pcc,
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

					if (puffs_pn_nodewalk(pu,
					    puffs_path_prefixadj, &pi) != NULL)
						error = ENOMEM;
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
			PUFFS_KCIDTOCID(pcn.pcn_cid, &auxt->pvnr_cn_cid);

			memset(&pni, 0, sizeof(pni));
			pni.pni_cookie = &auxt->pvnr_newnode;

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			error = pops->puffs_node_mkdir(pcc,
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
			PUFFS_KCIDTOCID(pcn.pcn_cid, &auxt->pvnr_cn_cid);

			error = pops->puffs_node_rmdir(pcc,
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
			PUFFS_KCIDTOCID(pcn.pcn_cid, &auxt->pvnr_cn_cid);

			memset(&pni, 0, sizeof(pni));
			pni.pni_cookie = &auxt->pvnr_newnode;

			if (buildpath) {
				error = puffs_path_pcnbuild(pu, &pcn, opcookie);
				if (error)
					break;
			}

			error = pops->puffs_node_symlink(pcc,
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
			error = pops->puffs_node_readdir(pcc,
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
			error = pops->puffs_node_readlink(pcc, opcookie, pcr,
			    auxt->pvnr_link, &auxt->pvnr_linklen);
			break;
		}

		case PUFFS_VN_RECLAIM:
		{
			struct puffs_vnmsg_reclaim *auxt = auxbuf;
			PUFFS_MAKECID(pcid, &auxt->pvnr_cid);

			if (pops->puffs_node_reclaim == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_reclaim(pcc, opcookie, pcid);
			break;
		}

		case PUFFS_VN_INACTIVE:
		{
			struct puffs_vnmsg_inactive *auxt = auxbuf;
			PUFFS_MAKECID(pcid, &auxt->pvnr_cid);

			if (pops->puffs_node_inactive == NULL) {
				error = EOPNOTSUPP;
				break;
			}

			error = pops->puffs_node_inactive(pcc, opcookie, pcid);
			break;
		}

		case PUFFS_VN_PATHCONF:
		{
			struct puffs_vnmsg_pathconf *auxt = auxbuf;
			if (pops->puffs_node_pathconf == NULL) {
				error = 0;
				break;
			}

			error = pops->puffs_node_pathconf(pcc,
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

			error = pops->puffs_node_advlock(pcc,
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

			error = pops->puffs_node_print(pcc,
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
			error = pops->puffs_node_read(pcc,
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

			error = pops->puffs_node_write(pcc,
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
			PUFFS_MAKECID(pcid, &auxt->pvnr_cid);

			if (pops->puffs_node_poll == NULL) {
				error = 0;

				/* emulate genfs_poll() */
				auxt->pvnr_events &= (POLLIN | POLLOUT
						    | POLLRDNORM | POLLWRNORM);

				break;
			}

			error = pops->puffs_node_poll(pcc,
			    opcookie, &auxt->pvnr_events, pcid);
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
		pu->pu_oppost(pcc);

	/*
	 * Note, we are calling this from here so that we can run it
	 * off of the continuation stack.  Otherwise puffs_goto() would
	 * not work.
	 */
	processresult(pcc, pcc->pcc_ppr, rv);
}

static void
processresult(struct puffs_cc *pcc, struct puffs_putreq *ppr, int how)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_req *preq = pcc->pcc_preq;
	int pccflags = pcc->pcc_flags;

	/* check if we need to store this reply */
	switch (how) {
	case PUFFCALL_ANSWER:
		if (pu->pu_flags & PUFFS_FLAG_OPDUMP)
			puffsdump_rv(preq);

		puffs_req_putcc(ppr, pcc);
		break;
	case PUFFCALL_IGNORE:
		LIST_INSERT_HEAD(&pu->pu_ccnukelst, pcc, nlst_entries);
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
