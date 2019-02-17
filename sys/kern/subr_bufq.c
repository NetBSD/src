/*	$NetBSD: subr_bufq.c,v 1.27 2019/02/17 23:17:41 bad Exp $	*/
/*	NetBSD: subr_disk.c,v 1.70 2005/08/20 12:00:01 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_disksubr.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_bufq.c,v 1.27 2019/02/17 23:17:41 bad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/bufq_impl.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>
#include <sys/module.h>

#define	STRAT_MATCH(id, bs)	(strcmp((id), (bs)->bs_name) == 0)

static void sysctl_kern_bufq_strategies_setup(struct sysctllog **);
static SLIST_HEAD(, bufq_strat) bufq_strat_list =
    SLIST_HEAD_INITIALIZER(bufq_strat_list);

static kmutex_t bufq_mutex;

static struct sysctllog *sysctllog;

void
bufq_init(void)
{

	mutex_init(&bufq_mutex, MUTEX_DEFAULT, IPL_NONE);
	sysctl_kern_bufq_strategies_setup(&sysctllog);
}

int
bufq_register(struct bufq_strat *bs)
{

	mutex_enter(&bufq_mutex);
	SLIST_INSERT_HEAD(&bufq_strat_list, bs, bs_next);
	bs->bs_refcnt = 0;
	mutex_exit(&bufq_mutex);

	return 0;
}

int
bufq_unregister(struct bufq_strat *bs)
{

	mutex_enter(&bufq_mutex);
	if (bs->bs_refcnt != 0) {
		mutex_exit(&bufq_mutex);
		return EBUSY;
	}
	SLIST_REMOVE(&bufq_strat_list, bs, bufq_strat, bs_next);
	mutex_exit(&bufq_mutex);

	return 0;
}

/*
 * Create a device buffer queue.
 */
int
bufq_alloc(struct bufq_state **bufqp, const char *strategy, int flags)
{
	struct bufq_strat *bsp, *it;
	struct bufq_state *bufq;
	int error = 0;
	u_int gen;
	bool found_exact;
	char strategy_module_name[MAXPATHLEN];

	KASSERT((flags & BUFQ_EXACT) == 0 || strategy != BUFQ_STRAT_ANY);

	switch (flags & BUFQ_SORT_MASK) {
	case BUFQ_SORT_RAWBLOCK:
	case BUFQ_SORT_CYLINDER:
		break;
	case 0:
		/*
		 * for strategies which don't care about block numbers.
		 * eg. fcfs
		 */
		flags |= BUFQ_SORT_RAWBLOCK;
		break;
	default:
		panic("bufq_alloc: sort out of range");
	}

	/*
	 * select strategy.
	 * if a strategy specified by flags is found, use it.
	 * otherwise, select one with the largest bs_prio.
	 */
	mutex_enter(&bufq_mutex);
	do {
		gen = module_gen;
		bsp = NULL;
		found_exact = false;

		SLIST_FOREACH(it, &bufq_strat_list, bs_next) {
			if (strategy != BUFQ_STRAT_ANY &&
			    STRAT_MATCH(strategy, (it))) {
				bsp = it;
				found_exact = true;
				break;
			}
			if (bsp == NULL || (it)->bs_prio > bsp->bs_prio)
				bsp = it;
		}
		if (strategy == BUFQ_STRAT_ANY || found_exact)
			break;

		/* Try to autoload the bufq strategy module */
		strlcpy(strategy_module_name, "bufq_",
			sizeof(strategy_module_name));
		strlcat(strategy_module_name, strategy,
			sizeof(strategy_module_name));
		mutex_exit(&bufq_mutex);
		(void) module_autoload(strategy_module_name, MODULE_CLASS_BUFQ);
		mutex_enter(&bufq_mutex);
	} while (gen != module_gen);

	if (bsp == NULL) {
		panic("bufq_alloc: no strategy");
	}
	if (strategy != BUFQ_STRAT_ANY && !found_exact) {
		if ((flags & BUFQ_EXACT)) {
			error = ENOENT;
			mutex_exit(&bufq_mutex);
			goto out;
		}
#if defined(DEBUG)
		printf("bufq_alloc: '%s' is not available. using '%s'.\n",
		    strategy, bsp->bs_name);
#endif
	}
#if defined(BUFQ_DEBUG)
	/* XXX aprint? */
	printf("bufq_alloc: using '%s'\n", bsp->bs_name);
#endif

	bsp->bs_refcnt++;
	mutex_exit(&bufq_mutex);
	*bufqp = bufq = kmem_zalloc(sizeof(*bufq), KM_SLEEP);
	bufq->bq_flags = flags;
	bufq->bq_strat = bsp;
	(*bsp->bs_initfn)(bufq);

out:
	return error;
}

void
bufq_put(struct bufq_state *bufq, struct buf *bp)
{

	(*bufq->bq_put)(bufq, bp);
}

struct buf *
bufq_get(struct bufq_state *bufq)
{

	return (*bufq->bq_get)(bufq, 1);
}

struct buf *
bufq_peek(struct bufq_state *bufq)
{

	return (*bufq->bq_get)(bufq, 0);
}

struct buf *
bufq_cancel(struct bufq_state *bufq, struct buf *bp)
{

	return (*bufq->bq_cancel)(bufq, bp);
}

/*
 * Drain a device buffer queue.
 */
void
bufq_drain(struct bufq_state *bufq)
{
	struct buf *bp;

	while ((bp = bufq_get(bufq)) != NULL) {
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}
}

/*
 * Destroy a device buffer queue.
 */
void
bufq_free(struct bufq_state *bufq)
{

	KASSERT(bufq_peek(bufq) == NULL);

	bufq->bq_fini(bufq);

	mutex_enter(&bufq_mutex);
	bufq->bq_strat->bs_refcnt--;
	mutex_exit(&bufq_mutex);
	
	kmem_free(bufq, sizeof(*bufq));
}

/*
 * get a strategy identifier of a buffer queue.
 */
const char *
bufq_getstrategyname(struct bufq_state *bufq)
{

	return bufq->bq_strat->bs_name;
}

/*
 * move all requests on a buffer queue to another.
 */
void
bufq_move(struct bufq_state *dst, struct bufq_state *src)
{
	struct buf *bp;

	while ((bp = bufq_get(src)) != NULL) {
		bufq_put(dst, bp);
	}
}

static int
docopy(char *buf, size_t *bufoffp, size_t buflen,
    const char *datap, size_t datalen)
{
	int error = 0;

	if (buf != NULL && datalen > 0) {

		if (*bufoffp + datalen > buflen) {
			goto out;
		}
		error = copyout(datap, buf + *bufoffp, datalen);
		if (error) {
			goto out;
		}
	}
out:
	if (error == 0) {
		*bufoffp += datalen;
	}

	return error;
}

static int
docopystr(char *buf, size_t *bufoffp, size_t buflen, const char *datap)
{

	return docopy(buf, bufoffp, buflen, datap, strlen(datap));
}

static int
docopynul(char *buf, size_t *bufoffp, size_t buflen)
{

	return docopy(buf, bufoffp, buflen, "", 1);
}

/*
 * sysctl function that will print all bufq strategies
 * currently available to the kernel.
 */
static int
sysctl_kern_bufq_strategies(SYSCTLFN_ARGS)
{
	const struct bufq_strat *bq_strat;
	const char *delim = "";
	size_t off = 0;
	size_t buflen = *oldlenp;
	int error;

	SLIST_FOREACH(bq_strat, &bufq_strat_list, bs_next) {
		error = docopystr(oldp, &off, buflen, delim);
		if (error) {
			goto out;
		}
		error = docopystr(oldp, &off, buflen, (bq_strat)->bs_name);
		if (error) {
			goto out;
		}
		delim = " ";
	}

	/* In case there are no registered strategies ... */
	if (off == 0) {
		error = docopystr(oldp, &off, buflen, "NULL");
		if (error) {
			goto out;
		}
	}

	/* NUL terminate */
	error = docopynul(oldp, &off, buflen);
out:
	*oldlenp = off;
	return error;
}

static void
sysctl_kern_bufq_strategies_setup(struct sysctllog **clog)
{
	const struct sysctlnode *node;

	node = NULL;
	sysctl_createv(clog, 0, NULL, &node,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "bufq",
			SYSCTL_DESCR("buffer queue subtree"),
			NULL, 0, NULL, 0,
			CTL_KERN, CTL_CREATE, CTL_EOL);
	if (node != NULL) {
		sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_STRING, "strategies",
			SYSCTL_DESCR("List of bufq strategies present"),
			sysctl_kern_bufq_strategies, 0, NULL, 0,
			CTL_KERN, node->sysctl_num, CTL_CREATE, CTL_EOL);
	}
}
