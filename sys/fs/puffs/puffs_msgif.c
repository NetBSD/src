/*	$NetBSD: puffs_msgif.c,v 1.40.6.1 2007/11/06 23:31:13 matt Exp $	*/

/*
 * Copyright (c) 2005, 2006, 2007  Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: puffs_msgif.c,v 1.40.6.1 2007/11/06 23:31:13 matt Exp $");

#include <sys/param.h>
#include <sys/fstrans.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <sys/proc.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

/*
 * waitq data structures
 */

/*
 * While a request is going to userspace, park the caller within the
 * kernel.  This is the kernel counterpart of "struct puffs_req".
 */
struct puffs_msgpark {
	struct puffs_req	*park_preq;	/* req followed by buf	*/

	size_t			park_copylen;	/* userspace copylength	*/
	size_t			park_maxlen;	/* max size in comeback */

	parkdone_fn		park_done;	/* "biodone" a'la puffs	*/
	void			*park_donearg;

	int			park_flags;
	int			park_refcount;

	kcondvar_t		park_cv;
	kmutex_t		park_mtx;

	TAILQ_ENTRY(puffs_msgpark) park_entries;
};
#define PARKFLAG_WAITERGONE	0x01
#define PARKFLAG_DONE		0x02
#define PARKFLAG_ONQUEUE1	0x04
#define PARKFLAG_ONQUEUE2	0x08
#define PARKFLAG_CALL		0x10
#define PARKFLAG_WANTREPLY	0x20

static struct pool_cache parkpc;
static struct pool parkpool;

static int
makepark(void *arg, void *obj, int flags)
{
	struct puffs_msgpark *park = obj;

	mutex_init(&park->park_mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&park->park_cv, "puffsrpl");

	return 0;
}

static void
nukepark(void *arg, void *obj)
{
	struct puffs_msgpark *park = obj;

	cv_destroy(&park->park_cv);
	mutex_destroy(&park->park_mtx);
}

void
puffs_msgif_init()
{

	pool_init(&parkpool, sizeof(struct puffs_msgpark), 0, 0, 0,
	    "puffprkl", &pool_allocator_nointr, IPL_NONE);
	pool_cache_init(&parkpc, &parkpool, makepark, nukepark, NULL);
}

void
puffs_msgif_destroy()
{

	pool_cache_destroy(&parkpc);
	pool_destroy(&parkpool);
}

static int alloced;

static struct puffs_msgpark *
puffs_msgpark_alloc(int waitok)
{
	struct puffs_msgpark *park;

	park = pool_cache_get(&parkpc, waitok ? PR_WAITOK : PR_NOWAIT);
	if (park == NULL)
		return park;

	park->park_refcount = 1;
	park->park_preq = NULL;
	park->park_flags = PARKFLAG_WANTREPLY;

	return park;
}

static void
puffs_msgpark_reference(struct puffs_msgpark *park)
{

	KASSERT(mutex_owned(&park->park_mtx));
	park->park_refcount++;
}

/*
 * Release reference to park structure.
 */
static void
puffs_msgpark_release1(struct puffs_msgpark *park, int howmany)
{
	struct puffs_req *preq = park->park_preq;
	int refcnt;

	KASSERT(mutex_owned(&park->park_mtx));
	refcnt = park->park_refcount -= howmany;
	mutex_exit(&park->park_mtx);

	KASSERT(refcnt >= 0);

	if (refcnt == 0) {
		alloced--;
		if (preq)
			kmem_free(preq, park->park_maxlen);
		pool_cache_put(&parkpc, park);
	}
}
#define puffs_msgpark_release(a) puffs_msgpark_release1(a, 1)

#ifdef PUFFSDEBUG
static void
parkdump(struct puffs_msgpark *park)
{

	DPRINTF(("park %p, preq %p, id %" PRIu64 "\n"
	    "\tcopy %zu, max %zu - done: %p/%p\n"
	    "\tflags 0x%08x, refcount %d, cv/mtx: %p/%p\n",
	    park, park->park_preq, park->park_preq->preq_id,
	    park->park_copylen, park->park_maxlen,
	    park->park_done, park->park_donearg,
	    park->park_flags, park->park_refcount,
	    &park->park_cv, &park->park_mtx));
}

static void
parkqdump(struct puffs_wq *q, int dumpall)
{
	struct puffs_msgpark *park;
	int total = 0;

	TAILQ_FOREACH(park, q, park_entries) {
		if (dumpall)
			parkdump(park);
		total++;
	}
	DPRINTF(("puffs waitqueue at %p dumped, %d total\n", q, total));

}
#endif /* PUFFSDEBUG */

/*
 * A word about locking in the park structures: the lock protects the
 * fields of the *park* structure (not preq) and acts as an interlock
 * in cv operations.  The lock is always internal to this module and
 * callers do not need to worry about it.
 */

int
puffs_msgmem_alloc(size_t len, struct puffs_msgpark **ppark, void **mem,
	int cansleep)
{
	struct puffs_msgpark *park;
	void *m;

	m = kmem_zalloc(len, cansleep ? KM_SLEEP : KM_NOSLEEP);
	if (m == NULL) {
		KASSERT(cansleep == 0);
		return ENOMEM;
	}

	park = puffs_msgpark_alloc(cansleep);
	if (park == NULL) {
		KASSERT(cansleep == 0);
		kmem_free(m, len);
		return ENOMEM;
	}

	park->park_preq = m;
	park->park_maxlen = len;

	*ppark = park;
	*mem = m;

	return 0;
}

void
puffs_msgmem_release(struct puffs_msgpark *park)
{

	if (park == NULL)
		return;

	mutex_enter(&park->park_mtx);
	puffs_msgpark_release(park);
}

void
puffs_msg_setfaf(struct puffs_msgpark *park)
{

	park->park_flags &= ~PARKFLAG_WANTREPLY;
}

/*
 * kernel-user-kernel waitqueues
 */

static int touser(struct puffs_mount *, struct puffs_msgpark *);

static uint64_t
puffs_getmsgid(struct puffs_mount *pmp)
{
	uint64_t rv;

	mutex_enter(&pmp->pmp_lock);
	rv = pmp->pmp_nextmsgid++;
	mutex_exit(&pmp->pmp_lock);

	return rv;
}

/* vfs request */
int
puffs_msg_vfs(struct puffs_mount *pmp, struct puffs_msgpark *park, int optype)
{

	park->park_preq->preq_opclass = PUFFSOP_VFS; 
	park->park_preq->preq_optype = optype;

	park->park_copylen = park->park_maxlen;

	return touser(pmp, park);
}

/*
 * vnode level request
 */
int
puffs_msg_vn(struct puffs_mount *pmp, struct puffs_msgpark *park,
	int optype, size_t delta, struct vnode *vp_opc, struct vnode *vp_aux)
{
	struct puffs_req *preq;
	void *cookie = VPTOPNC(vp_opc);
	struct puffs_node *pnode;
	int rv;

	park->park_preq->preq_opclass = PUFFSOP_VN; 
	park->park_preq->preq_optype = optype;
	park->park_preq->preq_cookie = cookie;

	KASSERT(delta < park->park_maxlen); /* "<=" wouldn't make sense */
	park->park_copylen = park->park_maxlen - delta;

	rv = touser(pmp, park);

	/*
	 * Check if the user server requests that inactive be called
	 * when the time is right.
	 */
	preq = park->park_preq;
	if (preq->preq_setbacks & PUFFS_SETBACK_INACT_N1) {
		pnode = vp_opc->v_data;
		pnode->pn_stat |= PNODE_DOINACT;
	}
	if (preq->preq_setbacks & PUFFS_SETBACK_INACT_N2) {
		/* if no vp_aux, just ignore */
		if (vp_aux) {
			pnode = vp_aux->v_data;
			pnode->pn_stat |= PNODE_DOINACT;
		}
	}
	if (preq->preq_setbacks & PUFFS_SETBACK_NOREF_N1) {
		pnode = vp_opc->v_data;
		pnode->pn_stat |= PNODE_NOREFS;
	}
	if (preq->preq_setbacks & PUFFS_SETBACK_NOREF_N2) {
		/* if no vp_aux, just ignore */
		if (vp_aux) {
			pnode = vp_aux->v_data;
			pnode->pn_stat |= PNODE_NOREFS;
		}
	}

	return rv;
}

void
puffs_msg_vncall(struct puffs_mount *pmp, struct puffs_msgpark *park,
	int optype, size_t delta, parkdone_fn donefn, void *donearg,
	struct vnode *vp_opc)
{
	void *cookie = VPTOPNC(vp_opc);

	park->park_preq->preq_opclass = PUFFSOP_VN; 
	park->park_preq->preq_optype = optype;
	park->park_preq->preq_cookie = cookie;

	KASSERT(delta < park->park_maxlen);
	park->park_copylen = park->park_maxlen - delta;
	park->park_done = donefn;
	park->park_donearg = donearg;
	park->park_flags |= PARKFLAG_CALL;

	(void) touser(pmp, park);
}

int
puffs_msg_raw(struct puffs_mount *pmp, struct puffs_msgpark *park)
{

	park->park_copylen = park->park_maxlen;

	return touser(pmp, park);
}

void
puffs_msg_errnotify(struct puffs_mount *pmp, uint8_t type, int error,
	const char *str, void *cookie)
{
	struct puffs_msgpark *park;
	struct puffs_error *perr;

	puffs_msgmem_alloc(sizeof(struct puffs_error), &park, (void **)&perr,1);

	perr->perr_error = error;
	strlcpy(perr->perr_str, str, sizeof(perr->perr_str));

	park->park_preq->preq_opclass |= PUFFSOP_ERROR | PUFFSOPFLAG_FAF;
	park->park_preq->preq_optype = type;
	park->park_preq->preq_cookie = cookie;

	park->park_copylen = park->park_maxlen;

	(void)touser(pmp, park);
}

/*
 * Wait for the userspace ping-pong game in calling process context,
 * unless a FAF / async call, in which case just enqueues the request
 * and return immediately.
 */
static int
touser(struct puffs_mount *pmp, struct puffs_msgpark *park)
{
	struct lwp *l = curlwp;
	struct mount *mp;
	struct puffs_req *preq;
	int rv = 0;

	mp = PMPTOMP(pmp);
	preq = park->park_preq;
	preq->preq_buflen = park->park_maxlen;
	KASSERT(preq->preq_id == 0);

	if ((park->park_flags & PARKFLAG_WANTREPLY) == 0)
		preq->preq_opclass |= PUFFSOPFLAG_FAF;
	else
		preq->preq_id = puffs_getmsgid(pmp);

	/* fill in caller information */
	preq->preq_pid = l->l_proc->p_pid;
	preq->preq_lid = l->l_lid;

	/*
	 * To support cv_sig, yet another movie: check if there are signals
	 * pending and we are issueing a non-FAF.  If so, return an error
	 * directly UNLESS we are issueing INACTIVE.  In that case, convert
	 * it to a FAF, fire off to the file server and return an error.
	 * Yes, this is bordering disgusting.  Barfbags are on me.
	 */
	if ((park->park_flags & PARKFLAG_WANTREPLY)
	   && (park->park_flags & PARKFLAG_CALL) == 0
	   && (l->l_flag & LW_PENDSIG) != 0 && sigispending(l, 0)) {
		if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_VN
		    && preq->preq_optype == PUFFS_VN_INACTIVE) {
			park->park_preq->preq_opclass |= PUFFSOPFLAG_FAF;
			park->park_flags &= ~PARKFLAG_WANTREPLY;
			DPRINTF(("puffs touser: converted to FAF %p\n", park));
			rv = EINTR;
		} else {
			return EINTR;
		}
	}

	/*
	 * test for suspension lock.
	 *
	 * Note that we *DO NOT* keep the lock, since that might block
	 * lock acquiring PLUS it would give userlandia control over
	 * the lock.  The operation queue enforces a strict ordering:
	 * when the fs server gets in the op stream, it knows things
	 * are in order.  The kernel locks can't guarantee that for
	 * userspace, in any case.
	 *
	 * BUT: this presents a problem for ops which have a consistency
	 * clause based on more than one operation.  Unfortunately such
	 * operations (read, write) do not reliably work yet.
	 *
	 * Ya, Ya, it's wrong wong wrong, me be fixink this someday.
	 *
	 * XXX: and there is one more problem.  We sometimes need to
	 * take a lazy lock in case the fs is suspending and we are
	 * executing as the fs server context.  This might happen
	 * e.g. in the case that the user server triggers a reclaim
	 * in the kernel while the fs is suspending.  It's not a very
	 * likely event, but it needs to be fixed some day.
	 */

	/*
	 * MOREXXX: once PUFFS_WCACHEINFO is enabled, we can't take
	 * the mutex here, since getpages() might be called locked.
	 */
	fstrans_start(mp, FSTRANS_NORMAL);
	mutex_enter(&pmp->pmp_lock);
	fstrans_done(mp);

	if (pmp->pmp_status != PUFFSTAT_RUNNING) {
		mutex_exit(&pmp->pmp_lock);
		return ENXIO;
	}

#ifdef PUFFSDEBUG
	parkqdump(&pmp->pmp_msg_touser, puffsdebug > 1);
	parkqdump(&pmp->pmp_msg_replywait, puffsdebug > 1);
#endif

	mutex_enter(&park->park_mtx);
	TAILQ_INSERT_TAIL(&pmp->pmp_msg_touser, park, park_entries);
	park->park_flags |= PARKFLAG_ONQUEUE1;
	puffs_mp_reference(pmp);
	pmp->pmp_msg_touser_count++;
	mutex_exit(&pmp->pmp_lock);

	DPRINTF(("touser: req %" PRIu64 ", preq: %p, park: %p, "
	    "c/t: 0x%x/0x%x, f: 0x%x\n", preq->preq_id, preq, park,
	    preq->preq_opclass, preq->preq_optype, park->park_flags));

	cv_broadcast(&pmp->pmp_msg_waiter_cv);
	selnotify(pmp->pmp_sel, 0);

	if ((park->park_flags & PARKFLAG_WANTREPLY)
	    && (park->park_flags & PARKFLAG_CALL) == 0) {
		int error;

		error = cv_wait_sig(&park->park_cv, &park->park_mtx);
		DPRINTF(("puffs_touser: waiter for %p woke up with %d\n",
		    park, error));
		if (error) {
			park->park_flags |= PARKFLAG_WAITERGONE;
			if (park->park_flags & PARKFLAG_DONE) {
				rv = preq->preq_rv;
			} else {
				/*
				 * ok, we marked it as going away, but
				 * still need to do queue ops.  take locks
				 * in correct order.
				 *
				 * We don't want to release our reference
				 * if it's on replywait queue to avoid error
				 * to file server.  putop() code will DTRT.
				 */
				mutex_exit(&park->park_mtx);
				mutex_enter(&pmp->pmp_lock);
				mutex_enter(&park->park_mtx);

				/* remove from queue1 */
				if (park->park_flags & PARKFLAG_ONQUEUE1) {
					TAILQ_REMOVE(&pmp->pmp_msg_touser,
					    park, park_entries);
					pmp->pmp_msg_touser_count--;
					park->park_flags &= ~PARKFLAG_ONQUEUE1;
				}

				/*
				 * If it's waiting for a response already,
				 * boost reference count.  Park will get
				 * nuked once the response arrives from
				 * the file server.
				 */
				if (park->park_flags & PARKFLAG_ONQUEUE2)
					puffs_msgpark_reference(park);

				mutex_exit(&pmp->pmp_lock);

				rv = error;
			}
		} else {
			rv = preq->preq_rv;
		}

		/*
		 * retake the lock and release.  This makes sure (haha,
		 * I'm humorous) that we don't process the same vnode in
		 * multiple threads due to the locks hacks we have in
		 * puffs_lock().  In reality this is well protected by
		 * the biglock, but once that's gone, well, hopefully
		 * this will be fixed for real.  (and when you read this
		 * comment in 2017 and subsequently barf, my condolences ;).
		 */
		if (rv == 0 && !fstrans_is_owner(mp)) {
			fstrans_start(mp, FSTRANS_NORMAL);
			fstrans_done(mp);
		}

	} else {
		/*
		 * Take extra reference for FAF, i.e. don't free us
		 * immediately upon return to the caller, but rather
		 * only when the message has been transported.
		 */
		puffs_msgpark_reference(park);
	}

	mutex_exit(&park->park_mtx);

	mutex_enter(&pmp->pmp_lock);
	puffs_mp_release(pmp);
	mutex_exit(&pmp->pmp_lock);

	return rv;
}

/*
 * Get next request in the outgoing queue.  "maxsize" controls the
 * size the caller can accommodate and "nonblock" signals if this
 * should block while waiting for input.  Handles all locking internally.
 */
int
puffs_msgif_getout(void *this, size_t maxsize, int nonblock,
	uint8_t **data, size_t *dlen, void **parkptr)
{
	struct puffs_mount *pmp = this;
	struct puffs_msgpark *park;
	struct puffs_req *preq;
	int error;

	error = 0;
	mutex_enter(&pmp->pmp_lock);
	puffs_mp_reference(pmp);
	for (;;) {
		/* RIP? */
		if (pmp->pmp_status != PUFFSTAT_RUNNING) {
			error = ENXIO;
			break;
		}

		/* need platinum yendorian express card? */
		if (TAILQ_EMPTY(&pmp->pmp_msg_touser)) {
			DPRINTF(("puffs_getout: no outgoing op, "));
			if (nonblock) {
				DPRINTF(("returning EWOULDBLOCK\n"));
				error = EWOULDBLOCK;
				break;
			}
			DPRINTF(("waiting ...\n"));

			error = cv_wait_sig(&pmp->pmp_msg_waiter_cv,
			    &pmp->pmp_lock);
			if (error)
				break;
			else
				continue;
		}

		park = TAILQ_FIRST(&pmp->pmp_msg_touser);
		if (park == NULL)
			continue;

		mutex_enter(&park->park_mtx);
		puffs_msgpark_reference(park);

		DPRINTF(("puffs_getout: found park at %p, ", park));

		/* If it's a goner, don't process any furher */
		if (park->park_flags & PARKFLAG_WAITERGONE) {
			DPRINTF(("waitergone!\n"));
			puffs_msgpark_release(park);
			continue;
		}

		/* check size */
		preq = park->park_preq;
		if (maxsize < preq->preq_frhdr.pfr_len) {
			DPRINTF(("buffer too small\n"));
			puffs_msgpark_release(park);
			error = E2BIG;
			break;
		}

		DPRINTF(("returning\n"));

		/*
		 * Ok, we found what we came for.  Release it from the
		 * outgoing queue but do not unlock.  We will unlock
		 * only after we "releaseout" it to avoid complications:
		 * otherwise it is (theoretically) possible for userland
		 * to race us into "put" before we have a change to put
		 * this baby on the receiving queue.
		 */
		TAILQ_REMOVE(&pmp->pmp_msg_touser, park, park_entries);
		KASSERT(park->park_flags & PARKFLAG_ONQUEUE1);
		park->park_flags &= ~PARKFLAG_ONQUEUE1;
		mutex_exit(&park->park_mtx);

		pmp->pmp_msg_touser_count--;
		KASSERT(pmp->pmp_msg_touser_count >= 0);

		break;
	}
	puffs_mp_release(pmp);
	mutex_exit(&pmp->pmp_lock);

	if (error == 0) {
		*data = (uint8_t *)preq;
		preq->preq_frhdr.pfr_len = park->park_copylen;
		preq->preq_frhdr.pfr_alloclen = park->park_maxlen;
		preq->preq_frhdr.pfr_type = preq->preq_opclass; /* yay! */
		*dlen = preq->preq_frhdr.pfr_len;
		*parkptr = park;
	}

	return error;
}

/*
 * Release outgoing structure.  Now, depending on the success of the
 * outgoing send, it is either going onto the result waiting queue
 * or the death chamber.
 */
void
puffs_msgif_releaseout(void *this, void *parkptr, int status)
{
	struct puffs_mount *pmp = this;
	struct puffs_msgpark *park = parkptr;

	DPRINTF(("puffs_releaseout: returning park %p, errno %d: " ,
	    park, status));
	mutex_enter(&pmp->pmp_lock);
	mutex_enter(&park->park_mtx);
	if (park->park_flags & PARKFLAG_WANTREPLY) {
		if (status == 0) {
			DPRINTF(("enqueue replywait\n"));
			TAILQ_INSERT_TAIL(&pmp->pmp_msg_replywait, park,
			    park_entries);
			park->park_flags |= PARKFLAG_ONQUEUE2;
		} else {
			DPRINTF(("error path!\n"));
			park->park_preq->preq_rv = status;
			park->park_flags |= PARKFLAG_DONE;
			cv_signal(&park->park_cv);
		}
		puffs_msgpark_release(park);
	} else {
		DPRINTF(("release\n"));
		puffs_msgpark_release1(park, 2);
	}
	mutex_exit(&pmp->pmp_lock);
}

/*
 * XXX: locking with this one?
 */
void
puffs_msgif_incoming(void *this, void *buf)
{
	struct puffs_mount *pmp = this;
	struct puffs_req *preq = buf;
	struct puffs_frame *pfr = &preq->preq_frhdr;
	struct puffs_msgpark *park;
	int release, wgone;

	/* XXX */
	if (PUFFSOP_OPCLASS(preq->preq_opclass) != PUFFSOP_VN
	    && PUFFSOP_OPCLASS(preq->preq_opclass) != PUFFSOP_VFS)
		return;

	mutex_enter(&pmp->pmp_lock);

	/* Locate waiter */
	TAILQ_FOREACH(park, &pmp->pmp_msg_replywait, park_entries) {
		if (park->park_preq->preq_id == preq->preq_id)
			break;
	}
	if (park == NULL) {
		DPRINTF(("puffs_msgif_income: no request: %" PRIu64 "\n",
		    preq->preq_id));
		mutex_exit(&pmp->pmp_lock);
		return; /* XXX send error */
	}

	mutex_enter(&park->park_mtx);
	puffs_msgpark_reference(park);
	if (pfr->pfr_len > park->park_maxlen) {
		DPRINTF(("puffs_msgif_income: invalid buffer length: "
		    "%zu (req %" PRIu64 ", \n", pfr->pfr_len, preq->preq_id));
		park->park_preq->preq_rv = EPROTO;
		cv_signal(&park->park_cv);
		puffs_msgpark_release(park);
		mutex_exit(&pmp->pmp_lock);
		return; /* XXX: error */
	}
	wgone = park->park_flags & PARKFLAG_WAITERGONE;

	KASSERT(park->park_flags & PARKFLAG_ONQUEUE2);
	TAILQ_REMOVE(&pmp->pmp_msg_replywait, park, park_entries);
	park->park_flags &= ~PARKFLAG_ONQUEUE2;
	mutex_exit(&pmp->pmp_lock);

	if (wgone) {
		DPRINTF(("puffs_putop: bad service - waiter gone for "
		    "park %p\n", park));
		release = 2;
	} else {
		if (park->park_flags & PARKFLAG_CALL) {
			DPRINTF(("puffs_msgif_income: call for %p, arg %p\n",
			    park->park_preq, park->park_donearg));
			park->park_done(pmp, buf, park->park_donearg);
			release = 2;
		} else {
			/* XXX: yes, I know */
			memcpy(park->park_preq, buf, pfr->pfr_len);
			release = 1;
		}
	}

	if (!wgone) {
		DPRINTF(("puffs_putop: flagging done for "
		    "park %p\n", park));
		cv_signal(&park->park_cv);
	}

	park->park_flags |= PARKFLAG_DONE;
	puffs_msgpark_release1(park, release);
}

/*
 * We're dead, kaput, RIP, slightly more than merely pining for the
 * fjords, belly-up, fallen, lifeless, finished, expired, gone to meet
 * our maker, ceased to be, etcetc.  YASD.  It's a dead FS!
 *
 * Caller must hold puffs mutex.
 */
void
puffs_userdead(struct puffs_mount *pmp)
{
	struct puffs_msgpark *park, *park_next;

	/*
	 * Mark filesystem status as dying so that operations don't
	 * attempt to march to userspace any longer.
	 */
	pmp->pmp_status = PUFFSTAT_DYING;

	/* signal waiters on REQUEST TO file server queue */
	for (park = TAILQ_FIRST(&pmp->pmp_msg_touser); park; park = park_next) {
		uint8_t opclass;

		mutex_enter(&park->park_mtx);
		puffs_msgpark_reference(park);
		park_next = TAILQ_NEXT(park, park_entries);

		KASSERT(park->park_flags & PARKFLAG_ONQUEUE1);
		TAILQ_REMOVE(&pmp->pmp_msg_touser, park, park_entries);
		park->park_flags &= ~PARKFLAG_ONQUEUE1;
		pmp->pmp_msg_touser_count--;

		/*
		 * Even though waiters on QUEUE1 are removed in touser()
		 * in case of WAITERGONE, it is still possible for us to
		 * get raced here due to having to retake locks in said
		 * touser().  In the race case simply "ignore" the item
		 * on the queue and move on to the next one.
		 */
		if (park->park_flags & PARKFLAG_WAITERGONE) {
			KASSERT((park->park_flags & PARKFLAG_CALL) == 0);
			KASSERT(park->park_flags & PARKFLAG_WANTREPLY);
			puffs_msgpark_release(park);

		} else {
			opclass = park->park_preq->preq_opclass;
			park->park_preq->preq_rv = ENXIO;

			if (park->park_flags & PARKFLAG_CALL) {
				park->park_done(pmp, park->park_preq,
				    park->park_donearg);
				puffs_msgpark_release1(park, 2);
			} else if ((park->park_flags & PARKFLAG_WANTREPLY)==0) {
				puffs_msgpark_release1(park, 2);
			} else {
				park->park_preq->preq_rv = ENXIO;
				cv_signal(&park->park_cv);
				puffs_msgpark_release(park);
			}
		}
	}

	/* signal waiters on RESPONSE FROM file server queue */
	for (park=TAILQ_FIRST(&pmp->pmp_msg_replywait); park; park=park_next) {
		mutex_enter(&park->park_mtx);
		puffs_msgpark_reference(park);
		park_next = TAILQ_NEXT(park, park_entries);

		KASSERT(park->park_flags & PARKFLAG_ONQUEUE2);
		KASSERT(park->park_flags & PARKFLAG_WANTREPLY);

		TAILQ_REMOVE(&pmp->pmp_msg_replywait, park, park_entries);
		park->park_flags &= ~PARKFLAG_ONQUEUE2;

		if (park->park_flags & PARKFLAG_WAITERGONE) {
			KASSERT((park->park_flags & PARKFLAG_CALL) == 0);
			puffs_msgpark_release(park);
		} else {
			park->park_preq->preq_rv = ENXIO;
			if (park->park_flags & PARKFLAG_CALL) {
				park->park_done(pmp, park->park_preq,
				    park->park_donearg);
				puffs_msgpark_release1(park, 2);
			} else {
				cv_signal(&park->park_cv);
				puffs_msgpark_release(park);
			}
		}
	}

	cv_broadcast(&pmp->pmp_msg_waiter_cv);
}
