/*	$NetBSD: puffs_msgif.c,v 1.13.2.9 2008/02/04 09:23:58 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: puffs_msgif.c,v 1.13.2.9 2008/02/04 09:23:58 yamt Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/fstrans.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/vnode.h>

#include <dev/putter/putter_sys.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

#include <miscfs/syncfs/syncfs.h> /* XXX: for syncer_mutex reference */

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
#define	PARKFLAG_HASERROR	0x40

static pool_cache_t parkpc;
#ifdef PUFFSDEBUG
static int totalpark;
#endif

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

	parkpc = pool_cache_init(sizeof(struct puffs_msgpark), 0, 0, 0,
	    "puffprkl", NULL, IPL_NONE, makepark, nukepark, NULL);
}

void
puffs_msgif_destroy()
{

	pool_cache_destroy(parkpc);
}

static int alloced;

static struct puffs_msgpark *
puffs_msgpark_alloc(int waitok)
{
	struct puffs_msgpark *park;

	park = pool_cache_get(parkpc, waitok ? PR_WAITOK : PR_NOWAIT);
	if (park == NULL)
		return park;

	park->park_refcount = 1;
	park->park_preq = NULL;
	park->park_flags = PARKFLAG_WANTREPLY;

#ifdef PUFFSDEBUG
	totalpark++;
#endif

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
		pool_cache_put(parkpc, park);

#ifdef PUFFSDEBUG
		totalpark--;
#endif
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
	park->park_maxlen = park->park_copylen = len;

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

	KASSERT((park->park_flags & PARKFLAG_CALL) == 0);
	park->park_flags &= ~PARKFLAG_WANTREPLY;
}

void
puffs_msg_setdelta(struct puffs_msgpark *park, size_t delta)
{

	KASSERT(delta < park->park_maxlen); /* "<=" wouldn't make sense */
	park->park_copylen = park->park_maxlen - delta;
}

void
puffs_msg_setinfo(struct puffs_msgpark *park, int class, int type,
	puffs_cookie_t ck)
{

	park->park_preq->preq_opclass = PUFFSOP_OPCLASS(class);
	park->park_preq->preq_optype = type;
	park->park_preq->preq_cookie = ck;
}

void
puffs_msg_setcall(struct puffs_msgpark *park, parkdone_fn donefn, void *donearg)
{

	KASSERT(park->park_flags & PARKFLAG_WANTREPLY);
	park->park_done = donefn;
	park->park_donearg = donearg;
	park->park_flags |= PARKFLAG_CALL;
}

/*
 * kernel-user-kernel waitqueues
 */

static uint64_t
puffs_getmsgid(struct puffs_mount *pmp)
{
	uint64_t rv;

	mutex_enter(&pmp->pmp_lock);
	rv = pmp->pmp_nextmsgid++;
	mutex_exit(&pmp->pmp_lock);

	return rv;
}

/*
 * A word about reference counting of parks.  A reference must be taken
 * when accessing a park and additionally when it is on a queue.  So
 * when taking it off a queue and releasing the access reference, the
 * reference count is generally decremented by 2.
 */

void
puffs_msg_enqueue(struct puffs_mount *pmp, struct puffs_msgpark *park)
{
	struct lwp *l = curlwp;
	struct mount *mp;
	struct puffs_req *preq;

	mp = PMPTOMP(pmp);
	preq = park->park_preq;
	preq->preq_buflen = park->park_maxlen;
	KASSERT(preq->preq_id == 0
	    || (preq->preq_opclass & PUFFSOPFLAG_ISRESPONSE));

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
	 * directly UNLESS we are issueing INACTIVE/RECLAIM.  In that case,
	 * convert it to a FAF, fire off to the file server and return
	 * an error.  Yes, this is bordering disgusting.  Barfbags are on me.
	 */
	if (__predict_false((park->park_flags & PARKFLAG_WANTREPLY)
	   && (park->park_flags & PARKFLAG_CALL) == 0
	   && (l->l_flag & LW_PENDSIG) != 0 && sigispending(l, 0))) {
		park->park_flags |= PARKFLAG_HASERROR;
		preq->preq_rv = EINTR;
		if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_VN
		    && (preq->preq_optype == PUFFS_VN_INACTIVE
		     || preq->preq_optype == PUFFS_VN_RECLAIM)) {
			park->park_preq->preq_opclass |= PUFFSOPFLAG_FAF;
			park->park_flags &= ~PARKFLAG_WANTREPLY;
			DPRINTF(("puffs_msg_enqueue: converted to FAF %p\n",
			    park));
		} else {
			return;
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
		park->park_flags |= PARKFLAG_HASERROR;
		preq->preq_rv = ENXIO;
		return;
	}

#ifdef PUFFSDEBUG
	parkqdump(&pmp->pmp_msg_touser, puffsdebug > 1);
	parkqdump(&pmp->pmp_msg_replywait, puffsdebug > 1);
#endif

	/*
	 * Note: we don't need to lock park since we have the only
	 * reference to it at this point.
	 */
	TAILQ_INSERT_TAIL(&pmp->pmp_msg_touser, park, park_entries);
	park->park_flags |= PARKFLAG_ONQUEUE1;
	pmp->pmp_msg_touser_count++;
	park->park_refcount++;
	mutex_exit(&pmp->pmp_lock);

	cv_broadcast(&pmp->pmp_msg_waiter_cv);
	putter_notify(pmp->pmp_pi);

	DPRINTF(("touser: req %" PRIu64 ", preq: %p, park: %p, "
	    "c/t: 0x%x/0x%x, f: 0x%x\n", preq->preq_id, preq, park,
	    preq->preq_opclass, preq->preq_optype, park->park_flags));
}

int
puffs_msg_wait(struct puffs_mount *pmp, struct puffs_msgpark *park)
{
	struct puffs_req *preq = park->park_preq; /* XXX: hmmm */
	struct mount *mp = PMPTOMP(pmp);
	int error = 0;
	int rv;

	mutex_enter(&pmp->pmp_lock);
	puffs_mp_reference(pmp);
	mutex_exit(&pmp->pmp_lock);

	mutex_enter(&park->park_mtx);
	if ((park->park_flags & PARKFLAG_WANTREPLY) == 0
	    || (park->park_flags & PARKFLAG_CALL)) {
		mutex_exit(&park->park_mtx);
		rv = 0;
		goto skipwait;
	}

	/* did the response beat us to the wait? */
	if (__predict_false((park->park_flags & PARKFLAG_DONE)
	    || (park->park_flags & PARKFLAG_HASERROR))) {
		rv = park->park_preq->preq_rv;
		mutex_exit(&park->park_mtx);
		goto skipwait;
	}

	error = cv_wait_sig(&park->park_cv, &park->park_mtx);
	DPRINTF(("puffs_touser: waiter for %p woke up with %d\n",
	    park, error));
	if (error) {
		park->park_flags |= PARKFLAG_WAITERGONE;
		if (park->park_flags & PARKFLAG_DONE) {
			rv = preq->preq_rv;
			mutex_exit(&park->park_mtx);
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

			/*
			 * Still on queue1?  We can safely remove it
			 * without any consequences since the file
			 * server hasn't seen it.  "else" we need to
			 * wait for the response and just ignore it
			 * to avoid signalling an incorrect error to
			 * the file server.
			 */
			if (park->park_flags & PARKFLAG_ONQUEUE1) {
				TAILQ_REMOVE(&pmp->pmp_msg_touser,
				    park, park_entries);
				puffs_msgpark_release(park);
				pmp->pmp_msg_touser_count--;
				park->park_flags &= ~PARKFLAG_ONQUEUE1;
			} else {
				mutex_exit(&park->park_mtx);
			}
			mutex_exit(&pmp->pmp_lock);

			rv = EINTR;
		}
	} else {
		rv = preq->preq_rv;
		mutex_exit(&park->park_mtx);
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

 skipwait:
	mutex_enter(&pmp->pmp_lock);
	puffs_mp_release(pmp);
	mutex_exit(&pmp->pmp_lock);

	return rv;
}

/*
 * XXX: this suuuucks.  Hopefully I'll get rid of this lossage once
 * the whole setback-nonsense gets fixed.
 */
int
puffs_msg_wait2(struct puffs_mount *pmp, struct puffs_msgpark *park,
	struct puffs_node *pn1, struct puffs_node *pn2)
{
	struct puffs_req *preq;
	int rv;

	rv = puffs_msg_wait(pmp, park);

	preq = park->park_preq;
	if (pn1 && preq->preq_setbacks & PUFFS_SETBACK_INACT_N1)
		pn1->pn_stat |= PNODE_DOINACT;
	if (pn2 && preq->preq_setbacks & PUFFS_SETBACK_INACT_N2)
		pn2->pn_stat |= PNODE_DOINACT;

	if (pn1 && preq->preq_setbacks & PUFFS_SETBACK_NOREF_N1)
		pn1->pn_stat |= PNODE_NOREFS;
	if (pn2 && preq->preq_setbacks & PUFFS_SETBACK_NOREF_N2)
		pn2->pn_stat |= PNODE_NOREFS;

	return rv;

}

/*
 * XXX: lazy bum.  please, for the love of foie gras, fix me.
 * This should *NOT* depend on setfaf.  Also "memcpy" could
 * be done more nicely.
 */
void
puffs_msg_sendresp(struct puffs_mount *pmp, struct puffs_req *origpreq, int rv)
{
	struct puffs_msgpark *park;
	struct puffs_req *preq;

	puffs_msgmem_alloc(sizeof(struct puffs_req), &park, (void *)&preq, 1);
	puffs_msg_setfaf(park); /* XXXXXX: avoids reqid override */

	memcpy(preq, origpreq, sizeof(struct puffs_req));
	preq->preq_rv = rv;
	preq->preq_opclass |= PUFFSOPFLAG_ISRESPONSE;

	puffs_msg_enqueue(pmp, park);
	puffs_msgmem_release(park);
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
		preq = park->park_preq;

#if 0
		/* check size */
		/*
		 * XXX: this check is not valid for now, we don't know
		 * the size of the caller's input buffer.  i.e. this
		 * will most likely go away
		 */
		if (maxsize < preq->preq_frhdr.pfr_len) {
			DPRINTF(("buffer too small\n"));
			puffs_msgpark_release(park);
			error = E2BIG;
			break;
		}
#endif

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
		preq->preq_pth.pth_framelen = park->park_copylen;
		*dlen = preq->preq_pth.pth_framelen;
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

size_t
puffs_msgif_waitcount(void *this)
{
	struct puffs_mount *pmp = this;
	size_t rv;

	mutex_enter(&pmp->pmp_lock);
	rv = pmp->pmp_msg_touser_count;
	mutex_exit(&pmp->pmp_lock);

	return rv;
}

/*
 * XXX: locking with this one?
 */
static void
puffsop_msg(void *this, struct puffs_req *preq)
{
	struct puffs_mount *pmp = this;
	struct putter_hdr *pth = &preq->preq_pth;
	struct puffs_msgpark *park;
	int wgone;

	mutex_enter(&pmp->pmp_lock);

	/* Locate waiter */
	TAILQ_FOREACH(park, &pmp->pmp_msg_replywait, park_entries) {
		if (park->park_preq->preq_id == preq->preq_id)
			break;
	}
	if (park == NULL) {
		DPRINTF(("puffsop_msg: no request: %" PRIu64 "\n",
		    preq->preq_id));
		mutex_exit(&pmp->pmp_lock);
		return; /* XXX send error */
	}

	mutex_enter(&park->park_mtx);
	puffs_msgpark_reference(park);
	if (pth->pth_framelen > park->park_maxlen) {
		DPRINTF(("puffsop_msg: invalid buffer length: "
		    "%" PRIu64 " (req %" PRIu64 ", \n", pth->pth_framelen,
		    preq->preq_id));
		park->park_preq->preq_rv = EPROTO;
		cv_signal(&park->park_cv);
		puffs_msgpark_release1(park, 2);
		mutex_exit(&pmp->pmp_lock);
		return; /* XXX: error */
	}
	wgone = park->park_flags & PARKFLAG_WAITERGONE;

	KASSERT(park->park_flags & PARKFLAG_ONQUEUE2);
	TAILQ_REMOVE(&pmp->pmp_msg_replywait, park, park_entries);
	park->park_flags &= ~PARKFLAG_ONQUEUE2;
	mutex_exit(&pmp->pmp_lock);

	if (wgone) {
		DPRINTF(("puffsop_msg: bad service - waiter gone for "
		    "park %p\n", park));
	} else {
		if (park->park_flags & PARKFLAG_CALL) {
			DPRINTF(("puffsop_msg: call for %p, arg %p\n",
			    park->park_preq, park->park_donearg));
			park->park_done(pmp, preq, park->park_donearg);
		} else {
			/* XXX: yes, I know */
			memcpy(park->park_preq, preq, pth->pth_framelen);
		}
	}

	if (!wgone) {
		DPRINTF(("puffs_putop: flagging done for "
		    "park %p\n", park));
		cv_signal(&park->park_cv);
	}

	park->park_flags |= PARKFLAG_DONE;
	puffs_msgpark_release1(park, 2);
}

/*
 * helpers
 */
static void
dosuspendresume(void *arg)
{
	struct puffs_mount *pmp = arg;
	struct mount *mp;
	int rv;

	mp = PMPTOMP(pmp);
	/*
	 * XXX?  does this really do any good or is it just
	 * paranoid stupidity?  or stupid paranoia?
	 */
	if (mp->mnt_iflag & IMNT_UNMOUNT) {
		printf("puffs dosuspendresume(): detected suspend on "
		    "unmounting fs\n");
		goto out;
	}

	/* Do the dance.  Allow only one concurrent suspend */
	rv = vfs_suspend(PMPTOMP(pmp), 1);
	if (rv == 0)
		vfs_resume(PMPTOMP(pmp));

 out:
	mutex_enter(&pmp->pmp_lock);
	KASSERT(pmp->pmp_suspend == 1);
	pmp->pmp_suspend = 0;
	puffs_mp_release(pmp);
	mutex_exit(&pmp->pmp_lock);

	kthread_exit(0);
}

static void
puffsop_suspend(struct puffs_mount *pmp)
{
	int rv = 0;

	mutex_enter(&pmp->pmp_lock);
	if (pmp->pmp_suspend || pmp->pmp_status != PUFFSTAT_RUNNING) {
		rv = EBUSY;
	} else {
		puffs_mp_reference(pmp);
		pmp->pmp_suspend = 1;
	}
	mutex_exit(&pmp->pmp_lock);
	if (rv)
		return;
	rv = kthread_create(PRI_NONE, 0, NULL, dosuspendresume,
	    pmp, NULL, "puffsusp");

	/* XXX: "return" rv */
}

static void
puffsop_flush(struct puffs_mount *pmp, struct puffs_flush *pf)
{
	struct vnode *vp;
	voff_t offlo, offhi;
	int rv, flags = 0;

	if (pf->pf_req.preq_pth.pth_framelen != sizeof(struct puffs_flush)) {
		rv = EINVAL;
		goto out;
	}

	/* XXX: slurry */
	if (pf->pf_op == PUFFS_INVAL_NAMECACHE_ALL) {
		cache_purgevfs(PMPTOMP(pmp));
		rv = 0;
		goto out;
	}

	/*
	 * Get vnode, don't lock it.  Namecache is protected by its own lock
	 * and we have a reference to protect against premature harvesting.
	 *
	 * The node we want here might be locked and the op is in
	 * userspace waiting for us to complete ==> deadlock.  Another
	 * reason we need to eventually bump locking to userspace, as we
	 * will need to lock the node if we wish to do flushes.
	 */
	rv = puffs_cookie2vnode(pmp, pf->pf_cookie, 0, 0, &vp);
	if (rv) {
		if (rv == PUFFS_NOSUCHCOOKIE)
			rv = ENOENT;
		goto out;
	}

	switch (pf->pf_op) {
#if 0
	/* not quite ready, yet */
	case PUFFS_INVAL_NAMECACHE_NODE:
	struct componentname *pf_cn;
	char *name;
		/* get comfortab^Wcomponentname */
		pf_cn = kmem_alloc(componentname);
		memset(pf_cn, 0, sizeof(struct componentname));
		break;

#endif
	case PUFFS_INVAL_NAMECACHE_DIR:
		if (vp->v_type != VDIR) {
			rv = EINVAL;
			break;
		}
		cache_purge1(vp, NULL, PURGE_CHILDREN);
		break;

	case PUFFS_INVAL_PAGECACHE_NODE_RANGE:
		flags = PGO_FREE;
		/*FALLTHROUGH*/
	case PUFFS_FLUSH_PAGECACHE_NODE_RANGE:
		if (flags == 0)
			flags = PGO_CLEANIT;

		if (pf->pf_end > vp->v_size || vp->v_type != VREG) {
			rv = EINVAL;
			break;
		}

		offlo = trunc_page(pf->pf_start);
		offhi = round_page(pf->pf_end);
		if (offhi != 0 && offlo >= offhi) {
			rv = EINVAL;
			break;
		}

		mutex_enter(&vp->v_uobj.vmobjlock);
		rv = VOP_PUTPAGES(vp, offlo, offhi, flags);
		break;

	default:
		rv = EINVAL;
	}

	vrele(vp);

 out:
	puffs_msg_sendresp(pmp, &pf->pf_req, rv);
}

int
puffs_msgif_dispatch(void *this, struct putter_hdr *pth)
{
	struct puffs_mount *pmp = this;
	struct puffs_req *preq = (struct puffs_req *)pth;

	/* XXX: need to send error to userspace */
	if (pth->pth_framelen < sizeof(struct puffs_req)) {
		puffs_msg_sendresp(pmp, preq, EINVAL); /* E2SMALL */
		return 0;
	}

	switch (PUFFSOP_OPCLASS(preq->preq_opclass)) {
	case PUFFSOP_VN:
	case PUFFSOP_VFS:
		DPRINTF(("dispatch: vn/vfs message 0x%x\n", preq->preq_optype));
		puffsop_msg(pmp, preq);
		break;
	case PUFFSOP_FLUSH:
		DPRINTF(("dispatch: flush 0x%x\n", preq->preq_optype));
		puffsop_flush(pmp, (struct puffs_flush *)preq);
		break;
	case PUFFSOP_SUSPEND:
		DPRINTF(("dispatch: suspend\n"));
		puffsop_suspend(pmp);
		break;
	default:
		DPRINTF(("dispatch: invalid class 0x%x\n", preq->preq_opclass));
		puffs_msg_sendresp(pmp, preq, EINVAL);
		break;
	}

	return 0;
}

int
puffs_msgif_close(void *this)
{
	struct puffs_mount *pmp = this;
	struct mount *mp = PMPTOMP(pmp);
	int rv;

	mutex_enter(&pmp->pmp_lock);
	puffs_mp_reference(pmp);

	/*
	 * Free the waiting callers before proceeding any further.
	 * The syncer might be jogging around in this file system
	 * currently.  If we allow it to go to the userspace of no
	 * return while trying to get the syncer lock, well ...
	 */
	puffs_userdead(pmp);

	/*
	 * Make sure someone from puffs_unmount() isn't currently in
	 * userspace.  If we don't take this precautionary step,
	 * they might notice that the mountpoint has disappeared
	 * from under them once they return.  Especially note that we
	 * cannot simply test for an unmounter before calling
	 * dounmount(), since it might be possible that that particular
	 * invocation of unmount was called without MNT_FORCE.  Here we
	 * *must* make sure unmount succeeds.  Also, restart is necessary
	 * since pmp isn't locked.  We might end up with PUTTER_DEAD after
	 * restart and exit from there.
	 */
	if (pmp->pmp_unmounting) {
		cv_wait(&pmp->pmp_unmounting_cv, &pmp->pmp_lock);
		puffs_mp_release(pmp);
		mutex_exit(&pmp->pmp_lock);
		DPRINTF(("puffs_fop_close: unmount was in progress for pmp %p, "
		    "restart\n", pmp));
		return ERESTART;
	}

	/* Won't access pmp from here anymore */
	puffs_mp_release(pmp);
	mutex_exit(&pmp->pmp_lock);

	/*
	 * Detach from VFS.  First do necessary XXX-dance (from
	 * sys_unmount() & other callers of dounmount()
	 *
	 * XXX Freeze syncer.  Must do this before locking the
	 * mount point.  See dounmount() for details.
	 *
	 * XXX2: take a reference to the mountpoint before starting to
	 * wait for syncer_mutex.  Otherwise the mointpoint can be
	 * wiped out while we wait.
	 */
	atomic_inc_uint((unsigned int*)&mp->mnt_refcnt);
	mutex_enter(&syncer_mutex);
	if (mp->mnt_iflag & IMNT_GONE) {
		mutex_exit(&syncer_mutex);
		vfs_destroy(mp);
		return 0;
	}

	/*
	 * microscopic race condition here (although not with the current
	 * kernel), but can't really fix it without starting a crusade
	 * against vfs_busy(), so let it be, let it be, let it be
	 */

	/*
	 * The only way vfs_busy() will fail for us is if the filesystem
	 * is already a goner.
	 * XXX: skating on the thin ice of modern calling conventions ...
	 */
	if (vfs_busy(mp, RW_WRITER, NULL)) {
		mutex_exit(&syncer_mutex);
		vfs_destroy(mp);
		return 0;
	}

	/*
	 * Once we have the mount point, unmount() can't interfere..
	 * or at least in theory it shouldn't.  dounmount() reentracy
	 * might require some visiting at some point.
	 */
	rv = dounmount(mp, MNT_FORCE, curlwp);
	KASSERT(rv == 0);
	vfs_destroy(mp);

	return 0;
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
