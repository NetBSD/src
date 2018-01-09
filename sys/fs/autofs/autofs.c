/*	$NetBSD: autofs.c,v 1.2 2018/01/09 13:56:00 martin Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * Copyright (c) 2016 The DragonFly Project
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tomohiro Kusumi <kusumi.tomohiro@gmail.com>.
 *
 * This software was developed by Edward Tomasz Napierala under sponsorship
 * from the FreeBSD Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*-
 * Copyright (c) 1989, 1991, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autofs.c,v 1.2 2018/01/09 13:56:00 martin Exp $");

#include "autofs.h"

#include <sys/atomic.h>
#include <sys/queue.h>
#include <sys/signalvar.h>

static int	autofs_open(dev_t dev, int flags, int mode, struct lwp *l);
static int	autofs_close(dev_t dev, int flags, int mode, struct lwp *l);
static int	autofs_ioctl(dev_t dev, const u_long cmd, void *data, int flag,
    struct lwp *l);

struct cdevsw autofs_ops = {
	.d_open		= autofs_open,
	.d_close	= autofs_close,
	.d_ioctl	= autofs_ioctl,
};

/*
 * List of signals that can interrupt an autofs trigger.
 */
static int autofs_sig_set[] = {
	SIGINT,
	SIGTERM,
	SIGHUP,
	SIGKILL,
	SIGQUIT,
};

struct pool	autofs_request_pool;
struct pool	autofs_node_pool;
struct autofs_softc	*autofs_softc = NULL;
struct workqueue	*autofs_tmo_wq = NULL;

int autofs_debug = 1;
int autofs_mount_on_stat = 0;
int autofs_timeout = 30;
int autofs_cache = 600;
int autofs_retry_attempts = 3;
int autofs_retry_delay = 1;
int autofs_interruptible = 1;

static int
autofs_node_cmp(const struct autofs_node *a, const struct autofs_node *b)
{

	return strcmp(a->an_name, b->an_name);
}

RB_GENERATE(autofs_node_tree, autofs_node, an_entry, autofs_node_cmp);

bool
autofs_ignore_thread(void)
{

	if (autofs_softc->sc_dev_opened == false)
		return false;

	mutex_enter(proc_lock);
	if (autofs_softc->sc_dev_sid == curproc->p_pgrp->pg_id) {
		mutex_exit(proc_lock);
		return true;
	}
	mutex_exit(proc_lock);

	return false;
}

static char *
autofs_path(struct autofs_node *anp)
{
	struct autofs_mount *amp = anp->an_mount;
	size_t len;
	char *path, *tmp;

	path = kmem_strdup("", KM_SLEEP);
	for (; anp->an_parent; anp = anp->an_parent) {
		len = strlen(anp->an_name) + strlen(path) + 2;
		tmp = kmem_alloc(len, KM_SLEEP);
		snprintf(tmp, len, "%s/%s", anp->an_name, path);
		kmem_strfree(path);
		path = tmp;
	}

	len = strlen(amp->am_on) + strlen(path) + 2;
	tmp = kmem_alloc(len, KM_SLEEP);
	snprintf(tmp, len, "%s/%s", amp->am_on, path);
	kmem_strfree(path);

	return tmp;
}

void
autofs_timeout_wq(struct work *wk, void *arg)
{
	struct autofs_request *ar = (void *)wk;

	mutex_enter(&autofs_softc->sc_lock);
	AUTOFS_WARN("request %d for %s timed out after %d seconds",
	    ar->ar_id, ar->ar_path, autofs_timeout);

	ar->ar_error = ETIMEDOUT;
	ar->ar_wildcards = true;
	ar->ar_done = true;
	ar->ar_in_progress = false;
	cv_broadcast(&autofs_softc->sc_cv);
	mutex_exit(&autofs_softc->sc_lock);
}

static void
autofs_timeout_callout(void *context)
{
	struct autofs_request *ar = context;

	workqueue_enqueue(autofs_tmo_wq, &ar->ar_wk, NULL);
}

bool
autofs_cached(struct autofs_node *anp, const char *component, int componentlen)
{
	struct autofs_mount *amp = anp->an_mount;

	KASSERT(!mutex_owned(&amp->am_lock));

	/*
	 * For root node we need to request automountd(8) assistance even
	 * if the node is marked as cached, but the requested top-level
	 * directory does not exist.  This is necessary for wildcard indirect
	 * map keys to work.  We don't do this if we know that there are
	 * no wildcards.
	 */
	if (!anp->an_parent && componentlen && anp->an_wildcards) {
		int error;
		KASSERT(amp->am_root == anp);
		mutex_enter(&amp->am_lock);
		error = autofs_node_find(anp, component, componentlen, NULL);
		mutex_exit(&amp->am_lock);
		if (error)
			return false;
	}

	return anp->an_cached;
}

static void
autofs_cache_callout(void *context)
{
	struct autofs_node *anp = context;

	autofs_node_uncache(anp);
}

void
autofs_flush(struct autofs_mount *amp)
{
	struct autofs_node *anp = amp->am_root;
	struct autofs_node *child;

	mutex_enter(&amp->am_lock);
	RB_FOREACH(child, autofs_node_tree, &anp->an_children) {
		autofs_node_uncache(child);
	}
	autofs_node_uncache(amp->am_root);
	mutex_exit(&amp->am_lock);

	AUTOFS_DEBUG("%s flushed", amp->am_on);
}

/*
 * The set/restore sigmask functions are used to (temporarily) overwrite
 * the thread sigmask during triggering.
 */
static void
autofs_set_sigmask(sigset_t *oldset)
{
	sigset_t newset;
	int i;

	sigfillset(&newset);
	/* Remove the autofs set of signals from newset */
	mutex_enter(proc_lock);
	mutex_enter(curproc->p_lock);

	for (i = 0; i < __arraycount(autofs_sig_set); i++) {
		/*
		 * But make sure we leave the ones already masked
		 * by the process, i.e. remove the signal from the
		 * temporary signalmask only if it wasn't already
		 * in sigmask.
		 */
		if (!sigismasked(curlwp, autofs_sig_set[i]))
			sigdelset(&newset, autofs_sig_set[i]);
	}
	sigprocmask1(curlwp, SIG_SETMASK, &newset, oldset);

	mutex_exit(curproc->p_lock);
	mutex_exit(proc_lock);
}

static void
autofs_restore_sigmask(sigset_t *set)
{

	mutex_enter(proc_lock);
	mutex_enter(curproc->p_lock);

	sigprocmask1(curlwp, SIG_SETMASK, set, NULL);

	mutex_exit(curproc->p_lock);
	mutex_exit(proc_lock);
}

static int
autofs_trigger_one(struct autofs_node *anp, const char *component,
    int componentlen)
{
	struct autofs_mount *amp = anp->an_mount;
	struct autofs_request *ar;
	char *key, *path;
	int error = 0, request_error;
	bool wildcards;

	KASSERT(mutex_owned(&autofs_softc->sc_lock));

	if (!anp->an_parent) {
		key = autofs_strndup(component, componentlen, KM_SLEEP);
	} else {
		struct autofs_node *firstanp;
		for (firstanp = anp; firstanp->an_parent->an_parent;
		    firstanp = firstanp->an_parent)
			continue;
		key = kmem_strdup(firstanp->an_name, KM_SLEEP);
	}

	path = autofs_path(anp);

	TAILQ_FOREACH(ar, &autofs_softc->sc_requests, ar_next) {
		if (strcmp(ar->ar_path, path))
			continue;
		if (strcmp(ar->ar_key, key))
			continue;

		KASSERT(!strcmp(ar->ar_from, amp->am_from));
		KASSERT(!strcmp(ar->ar_prefix, amp->am_prefix));
		KASSERT(!strcmp(ar->ar_options, amp->am_options));
		break;
	}

	if (ar) {
		atomic_add_int(&ar->ar_refcount, 1);
	} else {
		ar = pool_get(&autofs_request_pool, PR_WAITOK);
		ar->ar_mount = amp;
		ar->ar_id = autofs_softc->sc_last_request_id++;
		ar->ar_done = false;
		ar->ar_error = 0;
		ar->ar_wildcards = false;
		ar->ar_in_progress = false;
		strlcpy(ar->ar_from, amp->am_from, sizeof(ar->ar_from));
		strlcpy(ar->ar_path, path, sizeof(ar->ar_path));
		strlcpy(ar->ar_prefix, amp->am_prefix, sizeof(ar->ar_prefix));
		strlcpy(ar->ar_key, key, sizeof(ar->ar_key));
		strlcpy(ar->ar_options,
		    amp->am_options, sizeof(ar->ar_options));

		callout_init(&ar->ar_callout, 0);
		callout_reset(&ar->ar_callout, autofs_timeout * hz,
		    autofs_timeout_callout, ar);
		ar->ar_refcount = 1;
		TAILQ_INSERT_TAIL(&autofs_softc->sc_requests, ar, ar_next);
	}

	cv_broadcast(&autofs_softc->sc_cv);
	while (ar->ar_done == false) {
		if (autofs_interruptible) {
			sigset_t oldset;
			autofs_set_sigmask(&oldset);
			error = cv_wait_sig(&autofs_softc->sc_cv,
			    &autofs_softc->sc_lock);
			autofs_restore_sigmask(&oldset);
			if (error) {
				AUTOFS_WARN("cv_wait_sig for %s failed "
				    "with error %d", ar->ar_path, error);
				break;
			}
		} else {
			cv_wait(&autofs_softc->sc_cv, &autofs_softc->sc_lock);
		}
	}

	request_error = ar->ar_error;
	if (request_error)
		AUTOFS_WARN("request for %s completed with error %d",
		    ar->ar_path, request_error);

	wildcards = ar->ar_wildcards;

	/*
	 * Check if this is the last reference.
	 */
	if (!atomic_add_int_nv(&ar->ar_refcount, -1)) {
		TAILQ_REMOVE(&autofs_softc->sc_requests, ar, ar_next);
		mutex_exit(&autofs_softc->sc_lock);
		callout_halt(&ar->ar_callout, NULL);
		pool_put(&autofs_request_pool, ar);
		mutex_enter(&autofs_softc->sc_lock);
	}

	/*
	 * Note that we do not do negative caching on purpose.  This
	 * way the user can retry access at any time, e.g. after fixing
	 * the failure reason, without waiting for cache timer to expire.
	 */
	if (!error && !request_error && autofs_cache > 0) {
		autofs_node_cache(anp);
		anp->an_wildcards = wildcards;
		callout_reset(&anp->an_callout, autofs_cache * hz,
		    autofs_cache_callout, anp);
	}

	kmem_strfree(key);
	kmem_strfree(path);

	if (error)
		return error;
	return request_error;
}

int
autofs_trigger(struct autofs_node *anp, const char *component,
    int componentlen)
{
	for (;;) {
		int error, dummy;

		error = autofs_trigger_one(anp, component, componentlen);
		if (!error) {
			anp->an_retries = 0;
			return 0;
		}
		if (error == EINTR || error == ERESTART) {
			AUTOFS_DEBUG("trigger interrupted by signal, "
			    "not retrying");
			anp->an_retries = 0;
			return error;
		}
		anp->an_retries++;
		if (anp->an_retries >= autofs_retry_attempts) {
			AUTOFS_DEBUG("trigger failed %d times; returning "
			    "error %d", anp->an_retries, error);
			anp->an_retries = 0;
			return error;

		}
		AUTOFS_DEBUG("trigger failed with error %d; will retry in "
		    "%d seconds, %d attempts left", error, autofs_retry_delay,
		    autofs_retry_attempts - anp->an_retries);
		mutex_exit(&autofs_softc->sc_lock);
		tsleep(&dummy, 0, "autofs_retry", autofs_retry_delay * hz);
		mutex_enter(&autofs_softc->sc_lock);
	}
}

static int
autofs_ioctl_request(struct autofs_daemon_request *adr)
{
	struct autofs_request *ar;

	mutex_enter(&autofs_softc->sc_lock);
	for (;;) {
		int error;
		TAILQ_FOREACH(ar, &autofs_softc->sc_requests, ar_next) {
			if (ar->ar_done)
				continue;
			if (ar->ar_in_progress)
				continue;
			break;
		}

		if (ar)
			break;

		error = cv_wait_sig(&autofs_softc->sc_cv,
		    &autofs_softc->sc_lock);
		if (error) {
			mutex_exit(&autofs_softc->sc_lock);
			return error;
		}
	}

	ar->ar_in_progress = true;
	mutex_exit(&autofs_softc->sc_lock);

	adr->adr_id = ar->ar_id;
	strlcpy(adr->adr_from, ar->ar_from, sizeof(adr->adr_from));
	strlcpy(adr->adr_path, ar->ar_path, sizeof(adr->adr_path));
	strlcpy(adr->adr_prefix, ar->ar_prefix, sizeof(adr->adr_prefix));
	strlcpy(adr->adr_key, ar->ar_key, sizeof(adr->adr_key));
	strlcpy(adr->adr_options, ar->ar_options, sizeof(adr->adr_options));

	mutex_enter(proc_lock);
	autofs_softc->sc_dev_sid = curproc->p_pgrp->pg_id;
	mutex_exit(proc_lock);

	return 0;
}

static int
autofs_ioctl_done(struct autofs_daemon_done *add)
{
	struct autofs_request *ar;

	mutex_enter(&autofs_softc->sc_lock);
	TAILQ_FOREACH(ar, &autofs_softc->sc_requests, ar_next) {
		if (ar->ar_id == add->add_id)
			break;
	}

	if (!ar) {
		mutex_exit(&autofs_softc->sc_lock);
		AUTOFS_DEBUG("id %d not found", add->add_id);
		return ESRCH;
	}

	ar->ar_error = add->add_error;
	ar->ar_wildcards = add->add_wildcards;
	ar->ar_done = true;
	ar->ar_in_progress = false;
	cv_broadcast(&autofs_softc->sc_cv);

	mutex_exit(&autofs_softc->sc_lock);

	return 0;
}

static int
autofs_open(dev_t dev, int flags, int mode, struct lwp *l)
{

	mutex_enter(&autofs_softc->sc_lock);
	/*
	 * We must never block automountd(8) and its descendants, and we use
	 * session ID to determine that: we store session id of the process
	 * that opened the device, and then compare it with session ids
	 * of triggering processes.  This means running a second automountd(8)
	 * instance would break the previous one.  The check below prevents
	 * it from happening.
	 */
	if (autofs_softc->sc_dev_opened) {
		mutex_exit(&autofs_softc->sc_lock);
		return EBUSY;
	}

	autofs_softc->sc_dev_opened = true;
	mutex_exit(&autofs_softc->sc_lock);

	return 0;
}

static int
autofs_close(dev_t dev, int flags, int mode, struct lwp *l)
{

	mutex_enter(&autofs_softc->sc_lock);
	KASSERT(autofs_softc->sc_dev_opened);
	autofs_softc->sc_dev_opened = false;
	mutex_exit(&autofs_softc->sc_lock);

	return 0;
}

static int
autofs_ioctl(dev_t dev, const u_long cmd, void *data, int flag, struct lwp *l)
{

	KASSERT(autofs_softc->sc_dev_opened);

	switch (cmd) {
	case AUTOFSREQUEST:
		return autofs_ioctl_request(
		    (struct autofs_daemon_request *)data);
	case AUTOFSDONE:
		return autofs_ioctl_done(
		    (struct autofs_daemon_done *)data);
	default:
		AUTOFS_DEBUG("invalid cmd %lx", cmd);
		return EINVAL;
	}
	return EINVAL;
}
