/*	$NetBSD: pfil.c,v 1.27.40.2 2017/12/03 11:39:02 jdolecek Exp $	*/

/*
 * Copyright (c) 2013 Mindaugas Rasiukevicius <rmind at NetBSD org>
 * Copyright (c) 1996 Matthew R. Green
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pfil.c,v 1.27.40.2 2017/12/03 11:39:02 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/kmem.h>
#include <sys/psref.h>

#include <net/if.h>
#include <net/pfil.h>

#define	MAX_HOOKS	8

/* Func is either pfil_func_t or pfil_ifunc_t. */
typedef void		(*pfil_polyfunc_t)(void);

typedef struct {
	pfil_polyfunc_t pfil_func;
	void *		pfil_arg;
} pfil_hook_t;

typedef struct {
	pfil_hook_t	hooks[MAX_HOOKS];
	u_int		nhooks;
	struct psref_target psref;
} pfil_list_t;

typedef struct {
	pfil_list_t	*active;	/* lists[0] or lists[1] */
	pfil_list_t	lists[2];
} pfil_listset_t;

CTASSERT(PFIL_IN == 1);
CTASSERT(PFIL_OUT == 2);

struct pfil_head {
	pfil_listset_t	ph_in;
	pfil_listset_t	ph_out;
	pfil_listset_t	ph_ifaddr;
	pfil_listset_t	ph_ifevent;
	int		ph_type;
	void *		ph_key;
	LIST_ENTRY(pfil_head) ph_list;
};

static const int pfil_flag_cases[] = {
	PFIL_IN, PFIL_OUT
};

static LIST_HEAD(, pfil_head) pfil_head_list __read_mostly =
    LIST_HEAD_INITIALIZER(&pfil_head_list);

static kmutex_t pfil_mtx __cacheline_aligned;
static struct psref_class *pfil_psref_class __read_mostly;
static pserialize_t pfil_psz;

void
pfil_init(void)
{
	mutex_init(&pfil_mtx, MUTEX_DEFAULT, IPL_NONE);
	pfil_psz = pserialize_create();
	pfil_psref_class = psref_class_create("pfil", IPL_SOFTNET);
}

static inline void
pfil_listset_init(pfil_listset_t *pflistset)
{
	pflistset->active = &pflistset->lists[0];
	psref_target_init(&pflistset->active->psref, pfil_psref_class);
}

/*
 * pfil_head_create: create and register a packet filter head.
 */
pfil_head_t *
pfil_head_create(int type, void *key)
{
	pfil_head_t *ph;

	if (pfil_head_get(type, key)) {
		return NULL;
	}
	ph = kmem_zalloc(sizeof(pfil_head_t), KM_SLEEP);
	ph->ph_type = type;
	ph->ph_key = key;

	pfil_listset_init(&ph->ph_in);
	pfil_listset_init(&ph->ph_out);
	pfil_listset_init(&ph->ph_ifaddr);
	pfil_listset_init(&ph->ph_ifevent);

	LIST_INSERT_HEAD(&pfil_head_list, ph, ph_list);
	return ph;
}

/*
 * pfil_head_destroy: remove and destroy a packet filter head.
 */
void
pfil_head_destroy(pfil_head_t *pfh)
{
	LIST_REMOVE(pfh, ph_list);

	psref_target_destroy(&pfh->ph_in.active->psref, pfil_psref_class);
	psref_target_destroy(&pfh->ph_out.active->psref, pfil_psref_class);
	psref_target_destroy(&pfh->ph_ifaddr.active->psref, pfil_psref_class);
	psref_target_destroy(&pfh->ph_ifevent.active->psref, pfil_psref_class);

	kmem_free(pfh, sizeof(pfil_head_t));
}

/*
 * pfil_head_get: returns the packer filter head for a given key.
 */
pfil_head_t *
pfil_head_get(int type, void *key)
{
	pfil_head_t *ph;

	LIST_FOREACH(ph, &pfil_head_list, ph_list) {
		if (ph->ph_type == type && ph->ph_key == key)
			break;
	}
	return ph;
}

static pfil_listset_t *
pfil_hook_get(int dir, pfil_head_t *ph)
{
	switch (dir) {
	case PFIL_IN:
		return &ph->ph_in;
	case PFIL_OUT:
		return &ph->ph_out;
	case PFIL_IFADDR:
		return &ph->ph_ifaddr;
	case PFIL_IFNET:
		return &ph->ph_ifevent;
	}
	return NULL;
}

static int
pfil_list_add(pfil_listset_t *phlistset, pfil_polyfunc_t func, void *arg,
              int flags)
{
	u_int nhooks;
	pfil_list_t *newlist, *oldlist;
	pfil_hook_t *pfh;

	mutex_enter(&pfil_mtx);

	/* Check if we have a free slot. */
	nhooks = phlistset->active->nhooks;
	if (nhooks == MAX_HOOKS) {
		mutex_exit(&pfil_mtx);
		return ENOSPC;
	}
	KASSERT(nhooks < MAX_HOOKS);

	if (phlistset->active == &phlistset->lists[0]) {
		oldlist = &phlistset->lists[0];
		newlist = &phlistset->lists[1];
	} else{
		oldlist = &phlistset->lists[1];
		newlist = &phlistset->lists[0];
	}

	/* Make sure the hook is not already added. */
	for (u_int i = 0; i < nhooks; i++) {
		pfh = &oldlist->hooks[i];
		if (pfh->pfil_func == func && pfh->pfil_arg == arg) {
			mutex_exit(&pfil_mtx);
			return EEXIST;
		}
	}

	/* create new pfil_list_t copied from old */
	memcpy(newlist, oldlist, sizeof(pfil_list_t));
	psref_target_init(&newlist->psref, pfil_psref_class);

	/*
	 * Finally, add the hook.  Note: for PFIL_IN we insert the hooks in
	 * reverse order of the PFIL_OUT so that the same path is followed
	 * in or out of the kernel.
	 */
	if (flags & PFIL_IN) {
		/* XXX: May want to revisit this later; */
		size_t len = sizeof(pfil_hook_t) * nhooks;
		pfh = &newlist->hooks[0];
		memmove(&newlist->hooks[1], pfh, len);
	} else {
		pfh = &newlist->hooks[nhooks];
	}
	newlist->nhooks++;

	pfh->pfil_func = func;
	pfh->pfil_arg  = arg;

	/* switch from oldlist to newlist */
	membar_producer();
	phlistset->active = newlist;
#ifdef NET_MPSAFE
	pserialize_perform(pfil_psz);
#endif
	mutex_exit(&pfil_mtx);

	/* Wait for all readers */
#ifdef NET_MPSAFE
	psref_target_destroy(&oldlist->psref, pfil_psref_class);
#endif

	return 0;
}

/*
 * pfil_add_hook: add a function (hook) to the packet filter head.
 * The possible flags are:
 *
 *	PFIL_IN		call on incoming packets
 *	PFIL_OUT	call on outgoing packets
 *	PFIL_ALL	call on all of the above
 */
int
pfil_add_hook(pfil_func_t func, void *arg, int flags, pfil_head_t *ph)
{
	int error = 0;

	KASSERT(func != NULL);
	KASSERT((flags & ~PFIL_ALL) == 0);

	for (u_int i = 0; i < __arraycount(pfil_flag_cases); i++) {
		const int fcase = pfil_flag_cases[i];
		pfil_listset_t *phlistset;

		if ((flags & fcase) == 0) {
			continue;
		}
		phlistset = pfil_hook_get(fcase, ph);
		error = pfil_list_add(phlistset, (pfil_polyfunc_t)func, arg,
		    flags);
		if (error && (error != EEXIST))
			break;
	}
	if (error && (error != EEXIST)) {
		pfil_remove_hook(func, arg, flags, ph);
	}
	return error;
}

/*
 * pfil_add_ihook: add an interface-event function (hook) to the packet
 * filter head.  The possible flags are:
 *
 *	PFIL_IFADDR	call on interface reconfig (cmd is ioctl #)
 *	PFIL_IFNET	call on interface attach/detach (cmd is PFIL_IFNET_*)
 */
int
pfil_add_ihook(pfil_ifunc_t func, void *arg, int flags, pfil_head_t *ph)
{
	pfil_listset_t *phlistset;

	KASSERT(func != NULL);
	KASSERT(flags == PFIL_IFADDR || flags == PFIL_IFNET);

	phlistset = pfil_hook_get(flags, ph);
	return pfil_list_add(phlistset, (pfil_polyfunc_t)func, arg, flags);
}

/*
 * pfil_list_remove: remove the hook from a specified list.
 */
static int
pfil_list_remove(pfil_listset_t *phlistset, pfil_polyfunc_t func, void *arg)
{
	u_int nhooks;
	pfil_list_t *oldlist, *newlist;

	mutex_enter(&pfil_mtx);

	/* create new pfil_list_t copied from old */
	if (phlistset->active == &phlistset->lists[0]) {
		oldlist = &phlistset->lists[0];
		newlist = &phlistset->lists[1];
	} else{
		oldlist = &phlistset->lists[1];
		newlist = &phlistset->lists[0];
	}
	memcpy(newlist, oldlist, sizeof(*newlist));
	psref_target_init(&newlist->psref, pfil_psref_class);

	nhooks = newlist->nhooks;
	for (u_int i = 0; i < nhooks; i++) {
		pfil_hook_t *last, *pfh = &newlist->hooks[i];

		if (pfh->pfil_func != func || pfh->pfil_arg != arg) {
			continue;
		}
		if ((last = &newlist->hooks[nhooks - 1]) != pfh) {
			memcpy(pfh, last, sizeof(pfil_hook_t));
		}
		newlist->nhooks--;

		/* switch from oldlist to newlist */
		phlistset->active = newlist;
		membar_producer();
#ifdef NET_MPSAFE
		pserialize_perform(pfil_psz);
#endif
		mutex_exit(&pfil_mtx);

		/* Wait for all readers */
#ifdef NET_MPSAFE
		psref_target_destroy(&oldlist->psref, pfil_psref_class);
#endif

		return 0;
	}
	mutex_exit(&pfil_mtx);
	return ENOENT;
}

/*
 * pfil_remove_hook: remove the hook from the packet filter head.
 */
int
pfil_remove_hook(pfil_func_t func, void *arg, int flags, pfil_head_t *ph)
{
	KASSERT((flags & ~PFIL_ALL) == 0);

	for (u_int i = 0; i < __arraycount(pfil_flag_cases); i++) {
		const int fcase = pfil_flag_cases[i];
		pfil_listset_t *pflistset;

		if ((flags & fcase) == 0) {
			continue;
		}
		pflistset = pfil_hook_get(fcase, ph);
		(void)pfil_list_remove(pflistset, (pfil_polyfunc_t)func, arg);
	}
	return 0;
}

int
pfil_remove_ihook(pfil_ifunc_t func, void *arg, int flags, pfil_head_t *ph)
{
	pfil_listset_t *pflistset;

	KASSERT(flags == PFIL_IFADDR || flags == PFIL_IFNET);
	pflistset = pfil_hook_get(flags, ph);
	(void)pfil_list_remove(pflistset, (pfil_polyfunc_t)func, arg);
	return 0;
}

/*
 * pfil_run_hooks: run the specified packet filter hooks.
 */
int
pfil_run_hooks(pfil_head_t *ph, struct mbuf **mp, ifnet_t *ifp, int dir)
{
	struct mbuf *m = mp ? *mp : NULL;
	pfil_listset_t *phlistset;
	pfil_list_t *phlist;
	struct psref psref;
	int s, bound;
	int ret = 0;

	KASSERT(dir == PFIL_IN || dir == PFIL_OUT);
	if (__predict_false((phlistset = pfil_hook_get(dir, ph)) == NULL)) {
		return ret;
	}

	bound = curlwp_bind();
	s = pserialize_read_enter();
	phlist = phlistset->active;
	membar_datadep_consumer();
	psref_acquire(&psref, &phlist->psref, pfil_psref_class);
	pserialize_read_exit(s);
	for (u_int i = 0; i < phlist->nhooks; i++) {
		pfil_hook_t *pfh = &phlist->hooks[i];
		pfil_func_t func = (pfil_func_t)pfh->pfil_func;

		ret = (*func)(pfh->pfil_arg, &m, ifp, dir);
		if (m == NULL || ret)
			break;
	}
	psref_release(&psref, &phlist->psref, pfil_psref_class);
	curlwp_bindx(bound);

	if (mp) {
		*mp = m;
	}
	return ret;
}

static void
pfil_run_arg(pfil_listset_t *phlistset, u_long cmd, void *arg)
{
	pfil_list_t *phlist;
	struct psref psref;
	int s, bound;

	bound = curlwp_bind();
	s = pserialize_read_enter();
	phlist = phlistset->active;
	membar_datadep_consumer();
	psref_acquire(&psref, &phlist->psref, pfil_psref_class);
	pserialize_read_exit(s);
	for (u_int i = 0; i < phlist->nhooks; i++) {
		pfil_hook_t *pfh = &phlist->hooks[i];
		pfil_ifunc_t func = (pfil_ifunc_t)pfh->pfil_func;
		(*func)(pfh->pfil_arg, cmd, arg);
	}
	psref_release(&psref, &phlist->psref, pfil_psref_class);
	curlwp_bindx(bound);
}

void
pfil_run_addrhooks(pfil_head_t *ph, u_long cmd, struct ifaddr *ifa)
{
	pfil_run_arg(&ph->ph_ifaddr, cmd, ifa);
}

void
pfil_run_ifhooks(pfil_head_t *ph, u_long cmd, struct ifnet *ifp)
{
	pfil_run_arg(&ph->ph_ifevent, cmd, ifp);
}
