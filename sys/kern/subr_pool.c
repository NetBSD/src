/*	$NetBSD: subr_pool.c,v 1.2 1998/02/19 23:52:14 pk Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/pool.h>

/*
 * Pool resource management utility.
 */

struct pool_item {
	struct pool_item	*pi_next;
};


struct pool *
pool_create(size, nitems, wchan, mtype, storage)
	size_t	size;
	int	nitems;
	char	*wchan;
	int	mtype;
	caddr_t storage;
{
	struct pool *pp;
	caddr_t cp;

	if (size < sizeof(struct pool_item)) {
		printf("pool_create: size %lu too small\n", (u_long)size);
		return (NULL);
	}

	if (storage) {
		pp = (struct pool *)storage;
		cp = (caddr_t)ALIGN(pp + 1);
	} else {
		pp = (struct pool *)malloc(sizeof(*pp), mtype, M_NOWAIT);
		if (pp == NULL)
			return (NULL);
		cp = NULL;
	}

	pp->pr_freelist = NULL;
	pp->pr_freecount = 0;
	pp->pr_hiwat = 0;
	pp->pr_flags = (storage ? PR_STATIC : 0);
	pp->pr_size = size;
	pp->pr_wchan = wchan;
	pp->pr_mtype = mtype;
	simple_lock_init(&pp->pr_lock);

	if (nitems != 0) {
		if (pool_prime(pp, nitems, cp) != 0) {
			pool_destroy(pp);
			return (NULL);
		}
	}

	return (pp);
}

/*
 * De-commision a pool resource.
 */
void
pool_destroy(pp)
	struct pool *pp;
{
	struct pool_item *pi;

	if (pp->pr_flags & PR_STATIC)
		return;

	while ((pi = pp->pr_freelist) != NULL) {
		pp->pr_freelist = pi->pi_next;
		free(pi, pp->pr_mtype);
	}
	free(pp, pp->pr_mtype);
}


/*
 * Grab an item from the pool; must be called at splbio
 */
void *
pool_get(pp, flags)
	struct pool *pp;
	int flags;
{
	void *v;
	struct pool_item *pi;

#ifdef DIAGNOSTIC
	if ((pp->pr_flags & PR_STATIC) && (flags & PR_MALLOCOK))
		panic("pool_get: static");
#endif

again:
	simple_lock(&pp->pr_lock);
	if ((v = pp->pr_freelist) == NULL) {
		if (flags & PR_MALLOCOK)
			v = (void *)malloc(pp->pr_size, pp->pr_mtype, M_NOWAIT);

		if (v == NULL) {
			if ((flags & PR_WAITOK) == 0)
				return (NULL);
			pp->pr_flags |= PR_WANTED;
			simple_unlock(&pp->pr_lock);
			tsleep((caddr_t)pp, PSWP, pp->pr_wchan, 0);
			goto again;
		}
	} else {
		pi = v;
		pp->pr_freelist = pi->pi_next;
		pp->pr_freecount--;
	}
	simple_unlock(&pp->pr_lock);
	return (v);
}

/*
 * Return resource to the pool; must be called at splbio
 */
void
pool_put(pp, v)
	struct pool *pp;
	void *v;
{
	struct pool_item *pi = v;

	simple_lock(&pp->pr_lock);
	if ((pp->pr_flags & PR_WANTED) || pp->pr_freecount < pp->pr_hiwat) {
		/* Return to pool */
		pi->pi_next = pp->pr_freelist;
		pp->pr_freelist = pi;
		pp->pr_freecount++;
		if (pp->pr_flags & PR_WANTED) {
			pp->pr_flags &= ~PR_WANTED;
			wakeup((caddr_t)pp);
		}
	} else {
#ifdef DIAGNOSTIC
		if (pp->pr_flags & PR_STATIC) {
			/* can't happen because hiwat > freecount */
			panic("pool_put: static");
		}
#endif
		/* Return to system */
		free(v, M_DEVBUF);

		/*
		 * Return any excess items allocated during periods of
		 * contention.
		 */
		while (pp->pr_freecount > pp->pr_hiwat) {
			pi = pp->pr_freelist;
			pp->pr_freelist = pi->pi_next;
			pp->pr_freecount--;
			free(pi, M_DEVBUF);
		}
	}
	simple_unlock(&pp->pr_lock);
}

/*
 * Add N items to the pool
 */
int
pool_prime(pp, n, storage)
	struct pool *pp;
	int n;
	caddr_t storage;
{
	struct pool_item *pi;
	caddr_t cp = storage;

#ifdef DIAGNOSTIC
	if (storage && !(pp->pr_flags & PR_STATIC))
		panic("pool_prime: static");
	/* !storage && static caught below */
#endif

	simple_lock(&pp->pr_lock);
	pp->pr_hiwat += n;
	while (n--) {
		if (pp->pr_flags & PR_STATIC) {
			pi = (struct pool_item *)cp;
			cp = (caddr_t)ALIGN(cp + pp->pr_size);
		} else
			pi = malloc(pp->pr_size, pp->pr_mtype, M_NOWAIT);

		if (pi == NULL) {
			simple_unlock(&pp->pr_lock);
			return (ENOMEM);
		}

		pi->pi_next = pp->pr_freelist;
		pp->pr_freelist = pi;
		pp->pr_freecount++;
	}
	simple_unlock(&pp->pr_lock);
	return (0);
}
