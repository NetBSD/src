/*	$NetBSD: puffs_msgif.c,v 1.12 2006/12/10 23:53:51 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: puffs_msgif.c,v 1.12 2006/12/10 23:53:51 pooka Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/lock.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>


/*
 * kernel-user-kernel waitqueues
 */

static int touser(struct puffs_mount *, struct puffs_park *, uint64_t,
		  struct vnode *, struct vnode *);

uint64_t
puffs_getreqid(struct puffs_mount *pmp)
{
	unsigned int rv;

	simple_lock(&pmp->pmp_lock);
	rv = pmp->pmp_nextreq++;
	simple_unlock(&pmp->pmp_lock);

	return rv;
}

/* vfs request */
int
puffs_vfstouser(struct puffs_mount *pmp, int optype, void *kbuf, size_t buflen)
{
	struct puffs_park park;

	park.park_preq = kbuf;

	park.park_preq->preq_opclass = PUFFSOP_VFS; 
	park.park_preq->preq_optype = optype;

	park.park_maxlen = park.park_copylen = buflen;

	return touser(pmp, &park, puffs_getreqid(pmp), NULL, NULL);
}

/*
 * vnode level request
 */
int
puffs_vntouser(struct puffs_mount *pmp, int optype,
	void *kbuf, size_t buflen, void *cookie,
	struct vnode *vp1, struct vnode *vp2)
{
	struct puffs_park park;

	park.park_preq = kbuf;

	park.park_preq->preq_opclass = PUFFSOP_VN; 
	park.park_preq->preq_optype = optype;
	park.park_preq->preq_cookie = cookie;

	park.park_maxlen = park.park_copylen = buflen;

	return touser(pmp, &park, puffs_getreqid(pmp), vp1, vp2);
}

/*
 * vnode level request, caller-controller req id
 */
int
puffs_vntouser_req(struct puffs_mount *pmp, int optype,
	void *kbuf, size_t buflen, void *cookie, uint64_t reqid,
	struct vnode *vp1, struct vnode *vp2)
{
	struct puffs_park park;

	park.park_preq = kbuf;

	park.park_preq->preq_opclass = PUFFSOP_VN; 
	park.park_preq->preq_optype = optype;
	park.park_preq->preq_cookie = cookie;

	park.park_maxlen = park.park_copylen = buflen;

	return touser(pmp, &park, reqid, vp1, vp2);
}

/*
 * vnode level request, copy routines can adjust "kernbuf".
 * We overload park_copylen != park_maxlen to signal that the park
 * in question is of adjusting type.
 */
int
puffs_vntouser_adjbuf(struct puffs_mount *pmp, int optype,
	void **kbuf, size_t *buflen, size_t maxdelta,
	void *cookie, struct vnode *vp1, struct vnode *vp2)
{
	struct puffs_park park;
	int error;

	park.park_preq = *kbuf;

	park.park_preq->preq_opclass = PUFFSOP_VN; 
	park.park_preq->preq_optype = optype;
	park.park_preq->preq_cookie = cookie;

	park.park_copylen = *buflen;
	park.park_maxlen = maxdelta + *buflen;

	error = touser(pmp, &park, puffs_getreqid(pmp), vp1, vp2);

	*kbuf = park.park_preq;
	*buflen = park.park_copylen;

	return error;
}

/*
 * Notice: kbuf will be free'd later.  I must be allocated from the
 * kernel heap and it's ownership is shifted to this function from
 * now on, i.e. the caller is not allowed to use it anymore!
 */
void
puffs_vntouser_faf(struct puffs_mount *pmp, int optype,
	void *kbuf, size_t buflen, void *cookie)
{
	struct puffs_park *ppark;

	/* XXX: is it allowable to sleep here? */
	ppark = malloc(sizeof(struct puffs_park), M_PUFFS, M_NOWAIT | M_ZERO);
	if (ppark == NULL)
		return; /* 2bad */

	ppark->park_preq = kbuf;

	ppark->park_preq->preq_opclass = PUFFSOP_VN | PUFFSOPFLAG_FAF;
	ppark->park_preq->preq_optype = optype;
	ppark->park_preq->preq_cookie = cookie;

	ppark->park_maxlen = ppark->park_copylen = buflen;

	(void)touser(pmp, ppark, 0, NULL, NULL);
}

/*
 * Wait for the userspace ping-pong game in calling process context.
 *
 * This unlocks vnodes if they are supplied.  vp1 is the vnode
 * before in the locking order, i.e. the one which must be locked
 * before accessing vp2.  This is done here so that operations are
 * already ordered in the queue when vnodes are unlocked (I'm not
 * sure if that's really necessary, but it can't hurt).  Okok, maybe
 * there's a slight ugly-factor also, but let's not worry about that.
 */
static int
touser(struct puffs_mount *pmp, struct puffs_park *ppark, uint64_t reqid,
	struct vnode *vp1, struct vnode *vp2)
{
	struct puffs_req *preq;

	simple_lock(&pmp->pmp_lock);
	if (pmp->pmp_status != PUFFSTAT_RUNNING
	    && pmp->pmp_status != PUFFSTAT_MOUNTING) {
		simple_unlock(&pmp->pmp_lock);
		return ENXIO;
	}

	preq = ppark->park_preq;
	preq->preq_id = reqid;
	preq->preq_buflen = ALIGN(ppark->park_maxlen);

	TAILQ_INSERT_TAIL(&pmp->pmp_req_touser, ppark, park_entries);
	pmp->pmp_req_touser_waiters++;

	/*
	 * Don't do unlock-relock dance yet.  There are a couple of
	 * unsolved issues with it.  If we don't unlock, we can have
	 * processes wanting vn_lock in case userspace hangs.  But
	 * that can be "solved" by killing the userspace process.  It
	 * would of course be nicer to have antilocking in the userspace
	 * interface protocol itself.. your patience will be rewarded.
	 */
#if 0
	/* unlock */
	if (vp2)
		VOP_UNLOCK(vp2, 0);
	if (vp1)
		VOP_UNLOCK(vp1, 0);
#endif

	/*
	 * XXX: does releasing the lock here cause trouble?  Can't hold
	 * it, because otherwise the below would cause locking against
	 * oneself-problems in the kqueue stuff.  yes, it is a
	 * theoretical race, so it must be solved
	 */
	simple_unlock(&pmp->pmp_lock);

	wakeup(&pmp->pmp_req_touser);
	selnotify(pmp->pmp_sel, 0);

	if (PUFFSOP_WANTREPLY(ppark->park_preq->preq_opclass))
		ltsleep(ppark, PUSER, "puffs1", 0, NULL);

#if 0
	/* relock */
	if (vp1)
		KASSERT(vn_lock(vp1, LK_EXCLUSIVE | LK_RETRY) == 0);
	if (vp2)
		KASSERT(vn_lock(vp2, LK_EXCLUSIVE | LK_RETRY) == 0);
#endif

	return ppark->park_preq->preq_rv;
}


/*
 * getop: scan through queued requests until:
 *  1) max number of requests satisfied
 *     OR
 *  2) buffer runs out of space
 *     OR
 *  3) nonblocking is set AND there are no operations available
 *     OR
 *  4) at least one operation was transferred AND there are no more waiting
 */
int
puffs_getop(struct puffs_mount *pmp, struct puffs_reqh_get *phg, int nonblock)
{
	struct puffs_park *park;
	struct puffs_req *preq;
	uint8_t *bufpos;
	int error, donesome;

	donesome = error = 0;
	bufpos = phg->phg_buf;

	simple_lock(&pmp->pmp_lock);
	while (phg->phg_nops == 0 || donesome != phg->phg_nops) {
 again:
		if (pmp->pmp_status != PUFFSTAT_RUNNING) {
			/* if we got some, they don't really matter anymore */
			error = ENXIO;
			goto out;
		}
		if (TAILQ_EMPTY(&pmp->pmp_req_touser)) {
			if (donesome)
				goto out;

			if (nonblock) {
				error = EWOULDBLOCK;
				goto out;
			}

			error = ltsleep(&pmp->pmp_req_touser, PUSER | PCATCH,
			    "puffs2", 0, &pmp->pmp_lock);
			if (error)
				goto out;
			else
				goto again;
		}

		park = TAILQ_FIRST(&pmp->pmp_req_touser);
		preq = park->park_preq;

		if (phg->phg_buflen < preq->preq_buflen) {
			if (!donesome)
				error = E2BIG;
			goto out;
		}
		TAILQ_REMOVE(&pmp->pmp_req_touser, park, park_entries);

		simple_unlock(&pmp->pmp_lock);
		DPRINTF(("puffsgetop: get op %" PRIu64 " (%d.), from %p "
		    "len %zu (buflen %zu), target %p\n", preq->preq_id,
		    donesome, preq, park->park_copylen, preq->preq_buflen,
		    bufpos));

		if ((error = copyout(preq, bufpos, park->park_copylen)) != 0) {
			DPRINTF(("    FAILED %d\n", error));
			/*
			 * ok, user server is probably trying to cheat.
			 * stuff op back & return error to user
			 */
			 simple_lock(&pmp->pmp_lock);
			 TAILQ_INSERT_HEAD(&pmp->pmp_req_touser, park,
			     park_entries);

			 if (donesome)
				error = 0;
			 goto out;
		}
		bufpos += preq->preq_buflen;
		phg->phg_buflen -= preq->preq_buflen;
		donesome++;

		simple_lock(&pmp->pmp_lock);
		pmp->pmp_req_touser_waiters--;

		if (PUFFSOP_WANTREPLY(preq->preq_opclass)) {
			TAILQ_INSERT_TAIL(&pmp->pmp_req_replywait, park,
			    park_entries);
		} else {
			simple_unlock(&pmp->pmp_lock);
			free(preq, M_PUFFS);
			free(park, M_PUFFS);
			simple_lock(&pmp->pmp_lock);
		}
	}

 out:
	phg->phg_more = pmp->pmp_req_touser_waiters;
	simple_unlock(&pmp->pmp_lock);

	phg->phg_nops = donesome;

	return error;
}

int
puffs_putop(struct puffs_mount *pmp, struct puffs_reqh_put *php)
{
	struct puffs_park *park;
	void *userbuf;
	uint64_t id;
	size_t reqlen;
	int error;
	int donesome;

	donesome = error = 0;

	id = php->php_id;
	userbuf = php->php_buf;
	reqlen = php->php_buflen;

	simple_lock(&pmp->pmp_lock);
	while (donesome != php->php_nops) {
#ifdef DEBUG
		simple_unlock(&pmp->pmp_lock);
		DPRINTF(("puffsputop: searching for %" PRIu64 ", ubuf: %p, "
		    "len %zu\n", id, userbuf, reqlen));
		simple_lock(&pmp->pmp_lock);
#endif
		TAILQ_FOREACH(park, &pmp->pmp_req_replywait, park_entries) {
			if (park->park_preq->preq_id == id)
				break;
		}

		if (park == NULL) {
			error = EINVAL;
			break;
		}
		TAILQ_REMOVE(&pmp->pmp_req_replywait, park, park_entries);
		simple_unlock(&pmp->pmp_lock);

		if (park->park_maxlen != park->park_copylen) {
			/* sanitycheck size of incoming transmission. */
			if (reqlen > pmp->pmp_req_maxsize) {
				DPRINTF(("puffsputop: outrageous user buf "
				    "size: %zu\n", reqlen));
				error = EINVAL;
				goto loopout;
			}

			if (reqlen > park->park_copylen) {
				if (reqlen > park->park_maxlen) {
					DPRINTF(("puffsputop: adj copysize "
					    "> max size, %zu vs %zu\n",
					    reqlen, park->park_maxlen));
					error = EINVAL;
					goto loopout;
				}
				free(park->park_preq, M_PUFFS);
				park->park_preq = malloc(reqlen,
				    M_PUFFS, M_WAITOK);

				park->park_copylen = reqlen;
				DPRINTF(("puffsputop: adjbuf, new addr %p, "
				    "len %zu\n", park->park_preq, reqlen));
			}
		} else {
			if (reqlen == 0 || reqlen > park->park_copylen) {
				reqlen = park->park_copylen;
				DPRINTF(("puffsputop: kernel bufsize override: "
				    "%zu\n", reqlen));
			}
		}

		DPRINTF(("puffsputpop: copyin from %p to %p, len %zu\n",
		    userbuf, park->park_preq, reqlen));
		error = copyin(userbuf, park->park_preq, reqlen);
		if (error)
			goto loopout;

		/* all's well, prepare for next op */
		id = park->park_preq->preq_id;
		reqlen = park->park_preq->preq_buflen;
		userbuf = park->park_preq->preq_nextbuf;
		donesome++;

 loopout:
		if (error)
			park->park_preq->preq_rv = error;
		wakeup(park);

		simple_lock(&pmp->pmp_lock);
		if (error)
			break;
	}

	simple_unlock(&pmp->pmp_lock);
	php->php_nops -= donesome;

	return error;
}

/* this is probably going to die away at some point? */
/*
 * XXX: currently bitrotted
 */
#if 0
static int
puffssizeop(struct puffs_mount *pmp, struct puffs_sizeop *psop_user)
{
	struct puffs_sizepark *pspark;
	void *kernbuf;
	size_t copylen;
	int error;

	/* locate correct op */
	simple_lock(&pmp->pmp_lock);
	TAILQ_FOREACH(pspark, &pmp->pmp_req_sizepark, pkso_entries) {
		if (pspark->pkso_reqid == psop_user->pso_reqid) {
			TAILQ_REMOVE(&pmp->pmp_req_sizepark, pspark,
			    pkso_entries);
			break;
		}
	}
	simple_unlock(&pmp->pmp_lock);

	if (pspark == NULL)
		return EINVAL;

	error = 0;
	copylen = MIN(pspark->pkso_bufsize, psop_user->pso_bufsize);

	/*
	 * XXX: uvm stuff to avoid bouncy-bouncy copying?
	 */
	if (PUFFS_SIZEOP_UIO(pspark->pkso_reqtype)) {
		kernbuf = malloc(copylen, M_PUFFS, M_WAITOK | M_ZERO);
		if (pspark->pkso_reqtype == PUFFS_SIZEOPREQ_UIO_IN) {
			error = copyin(psop_user->pso_userbuf,
			    kernbuf, copylen);
			if (error) {
				printf("psop ERROR1 %d\n", error);
				goto escape;
			}
		}
		error = uiomove(kernbuf, copylen, pspark->pkso_uio);
		if (error) {
			printf("uiomove from kernel %p, len %d failed: %d\n",
			    kernbuf, (int)copylen, error);
			goto escape;
		}
			
		if (pspark->pkso_reqtype == PUFFS_SIZEOPREQ_UIO_OUT) {
			error = copyout(kernbuf,
			    psop_user->pso_userbuf, copylen);
			if (error) {
				printf("psop ERROR2 %d\n", error);
				goto escape;
			}
		}
 escape:
		free(kernbuf, M_PUFFS);
	} else if (PUFFS_SIZEOP_BUF(pspark->pkso_reqtype)) {
		copylen = MAX(pspark->pkso_bufsize, psop_user->pso_bufsize);
		if (pspark->pkso_reqtype == PUFFS_SIZEOPREQ_BUF_IN) {
			error = copyin(psop_user->pso_userbuf,
			pspark->pkso_copybuf, copylen);
		} else {
			error = copyout(pspark->pkso_copybuf,
			    psop_user->pso_userbuf, copylen);
		}
	}
#ifdef DIAGNOSTIC
	else
		panic("puffssizeop: invalid reqtype %d\n",
		    pspark->pkso_reqtype);
#endif /* DIAGNOSTIC */

	return error;
}
#endif
