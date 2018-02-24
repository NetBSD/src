/*	$NetBSD: wqinput.c,v 1.4 2018/02/24 07:37:09 ozaki-r Exp $	*/

/*-
 * Copyright (c) 2017 Internet Initiative Japan Inc.
 * All rights reserved.
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

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/syslog.h>
#include <sys/workqueue.h>
#include <sys/atomic.h>
#include <sys/queue.h>
#include <sys/percpu.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <netinet/wqinput.h>

#define WQINPUT_LIST_MAXLEN	IFQ_MAXLEN

struct wqinput_work {
	struct mbuf	*ww_mbuf;
	int		ww_off;
	int		ww_proto;
	struct wqinput_work *ww_next;
};

struct wqinput_worklist {
	/*
	 * XXX: TAILQ cannot be used because TAILQ_INIT memories the address
	 * of percpu data while percpu(9) may move percpu data during bootup.
	 */
	struct wqinput_work *wwl_head;
	struct wqinput_work *wwl_tail;
	unsigned int	wwl_len;
	unsigned long	wwl_dropped;
	struct work	wwl_work;
	bool		wwl_wq_is_active;
};

struct wqinput {
	struct workqueue *wqi_wq;
	struct pool	wqi_work_pool;
	struct percpu	*wqi_worklists; /* struct wqinput_worklist */
	void    	(*wqi_input)(struct mbuf *, int, int);
};

static void wqinput_work(struct work *, void *);
static void wqinput_sysctl_setup(const char *, struct wqinput *);

static void
wqinput_drops(void *p, void *arg, struct cpu_info *ci __unused)
{
	struct wqinput_worklist *const wwl = p;
	int *sum = arg;

	*sum += wwl->wwl_dropped;
}

static int
wqinput_sysctl_drops_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct wqinput *wqi;
	int sum = 0;
	int error;

	node = *rnode;
	wqi = node.sysctl_data;

	percpu_foreach(wqi->wqi_worklists, wqinput_drops, &sum);

	node.sysctl_data = &sum;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	return 0;
}

static void
wqinput_sysctl_setup(const char *name, struct wqinput *wqi)
{
	const struct sysctlnode *cnode, *rnode;
	int error;

	error = sysctl_createv(NULL, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "wqinput",
	    SYSCTL_DESCR("workqueue-based pr_input controls"),
	    NULL, 0, NULL, 0, CTL_NET, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto bad;

	error = sysctl_createv(NULL, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, name,
	    SYSCTL_DESCR("Protocol controls for workqueue-based pr_input"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto bad;

	error = sysctl_createv(NULL, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "inputq",
	    SYSCTL_DESCR("wqinput input queue controls"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto bad;

	error = sysctl_createv(NULL, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_INT, "drops",
	    SYSCTL_DESCR("Total packets dropped due to full input queue"),
	    wqinput_sysctl_drops_handler, 0, (void *)wqi, 0, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto bad;

	return;
bad:
	log(LOG_ERR, "%s: could not create a sysctl node for %s\n",
	    __func__, name);
	return;
}

struct wqinput *
wqinput_create(const char *name, void (*func)(struct mbuf *, int, int))
{
	struct wqinput *wqi;
	int error;
	char namebuf[32];

	snprintf(namebuf, sizeof(namebuf), "%s_wqinput", name);

	wqi = kmem_alloc(sizeof(*wqi), KM_SLEEP);

	error = workqueue_create(&wqi->wqi_wq, namebuf, wqinput_work, wqi,
	    PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE|WQ_PERCPU);
	if (error != 0)
		panic("%s: workqueue_create failed (%d)\n", __func__, error);
	pool_init(&wqi->wqi_work_pool, sizeof(struct wqinput_work), 0, 0, 0,
	    name, NULL, IPL_SOFTNET);
	wqi->wqi_worklists = percpu_alloc(sizeof(struct wqinput_worklist));
	wqi->wqi_input = func;

	wqinput_sysctl_setup(name, wqi);

	return wqi;
}

static struct wqinput_work *
wqinput_work_get(struct wqinput_worklist *wwl)
{
	struct wqinput_work *work;

	/* Must be called at IPL_SOFTNET */

	work = wwl->wwl_head;
	if (work != NULL) {
		KASSERTMSG(wwl->wwl_len > 0, "wwl->wwl_len=%d", wwl->wwl_len);
		wwl->wwl_len--;
		wwl->wwl_head = work->ww_next;
		work->ww_next = NULL;

		if (wwl->wwl_head == NULL)
			wwl->wwl_tail = NULL;
	} else {
		KASSERT(wwl->wwl_len == 0);
	}

	return work;
}

static void
wqinput_work(struct work *wk, void *arg)
{
	struct wqinput *wqi = arg;
	struct wqinput_work *work;
	struct wqinput_worklist *wwl;
	int s;

	/* Users expect to run at IPL_SOFTNET */
	s = splsoftnet();
	/* This also prevents LWP migrations between CPUs */
	wwl = percpu_getref(wqi->wqi_worklists);

	/* We can allow enqueuing another work at this point */
	wwl->wwl_wq_is_active = false;

	while ((work = wqinput_work_get(wwl)) != NULL) {
		mutex_enter(softnet_lock);
		KERNEL_LOCK_UNLESS_NET_MPSAFE();
		wqi->wqi_input(work->ww_mbuf, work->ww_off, work->ww_proto);
		KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
		mutex_exit(softnet_lock);

		pool_put(&wqi->wqi_work_pool, work);
	}

	percpu_putref(wqi->wqi_worklists);
	splx(s);
}

static void
wqinput_work_put(struct wqinput_worklist *wwl, struct wqinput_work *work)
{

	if (wwl->wwl_tail != NULL) {
		wwl->wwl_tail->ww_next = work;
	} else {
		wwl->wwl_head = work;
	}
	wwl->wwl_tail = work;
	wwl->wwl_len++;
}

void
wqinput_input(struct wqinput *wqi, struct mbuf *m, int off, int proto)
{
	struct wqinput_work *work;
	struct wqinput_worklist *wwl;

	wwl = percpu_getref(wqi->wqi_worklists);

	/* Prevent too much work and mbuf from being queued */
	if (wwl->wwl_len >= WQINPUT_LIST_MAXLEN) {
		wwl->wwl_dropped++;
		m_freem(m);
		goto out;
	}

	work = pool_get(&wqi->wqi_work_pool, PR_NOWAIT);
	if (work == NULL) {
		wwl->wwl_dropped++;
		m_freem(m);
		goto out;
	}
	work->ww_mbuf = m;
	work->ww_off = off;
	work->ww_proto = proto;
	work->ww_next = NULL;

	wqinput_work_put(wwl, work);

	/* Avoid enqueuing another work when one is already enqueued */
	if (wwl->wwl_wq_is_active)
		goto out;
	wwl->wwl_wq_is_active = true;

	workqueue_enqueue(wqi->wqi_wq, &wwl->wwl_work, NULL);
out:
	percpu_putref(wqi->wqi_worklists);
}
