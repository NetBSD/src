/*	$NetBSD: kern_hook.c,v 1.10 2021/09/30 07:14:09 skrll Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999, 2002, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Luke Mewburn.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_hook.c,v 1.10 2021/09/30 07:14:09 skrll Exp $");

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/hook.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/rwlock.h>
#include <sys/systm.h>

/*
 * A generic linear hook.
 */
struct hook_desc {
	LIST_ENTRY(hook_desc) hk_list;
	void	(*hk_fn)(void *);
	void	*hk_arg;
};
typedef LIST_HEAD(, hook_desc) hook_list_t;

enum hook_list_st {
	HKLIST_IDLE,
	HKLIST_INUSE,
};

struct khook_list {
	hook_list_t	 hl_list;
	kmutex_t	 hl_lock;
	kmutex_t	*hl_cvlock;
	struct lwp	*hl_lwp;
	kcondvar_t	 hl_cv;
	enum hook_list_st
			 hl_state;
	khook_t		*hl_active_hk;
	char		 hl_namebuf[HOOKNAMSIZ];
};

int	powerhook_debug = 0;

static void *
hook_establish(hook_list_t *list, void (*fn)(void *), void *arg)
{
	struct hook_desc *hd;

	hd = malloc(sizeof(*hd), M_DEVBUF, M_NOWAIT);
	if (hd == NULL)
		return (NULL);

	hd->hk_fn = fn;
	hd->hk_arg = arg;
	LIST_INSERT_HEAD(list, hd, hk_list);

	return (hd);
}

static void
hook_disestablish(hook_list_t *list, void *vhook)
{
#ifdef DIAGNOSTIC
	struct hook_desc *hd;

	LIST_FOREACH(hd, list, hk_list) {
                if (hd == vhook)
			break;
	}

	if (hd == NULL)
		panic("hook_disestablish: hook %p not established", vhook);
#endif
	LIST_REMOVE((struct hook_desc *)vhook, hk_list);
	free(vhook, M_DEVBUF);
}

static void
hook_destroy(hook_list_t *list)
{
	struct hook_desc *hd;

	while ((hd = LIST_FIRST(list)) != NULL) {
		LIST_REMOVE(hd, hk_list);
		free(hd, M_DEVBUF);
	}
}

static void
hook_proc_run(hook_list_t *list, struct proc *p)
{
	struct hook_desc *hd;

	LIST_FOREACH(hd, list, hk_list) {
		__FPTRCAST(void (*)(struct proc *, void *), *hd->hk_fn)(p,
		    hd->hk_arg);
	}
}

/*
 * "Shutdown hook" types, functions, and variables.
 *
 * Should be invoked immediately before the
 * system is halted or rebooted, i.e. after file systems unmounted,
 * after crash dump done, etc.
 *
 * Each shutdown hook is removed from the list before it's run, so that
 * it won't be run again.
 */

static hook_list_t shutdownhook_list = LIST_HEAD_INITIALIZER(shutdownhook_list);

void *
shutdownhook_establish(void (*fn)(void *), void *arg)
{
	return hook_establish(&shutdownhook_list, fn, arg);
}

void
shutdownhook_disestablish(void *vhook)
{
	hook_disestablish(&shutdownhook_list, vhook);
}

/*
 * Run shutdown hooks.  Should be invoked immediately before the
 * system is halted or rebooted, i.e. after file systems unmounted,
 * after crash dump done, etc.
 *
 * Each shutdown hook is removed from the list before it's run, so that
 * it won't be run again.
 */
void
doshutdownhooks(void)
{
	struct hook_desc *dp;

	while ((dp = LIST_FIRST(&shutdownhook_list)) != NULL) {
		LIST_REMOVE(dp, hk_list);
		(*dp->hk_fn)(dp->hk_arg);
#if 0
		/*
		 * Don't bother freeing the hook structure,, since we may
		 * be rebooting because of a memory corruption problem,
		 * and this might only make things worse.  It doesn't
		 * matter, anyway, since the system is just about to
		 * reboot.
		 */
		free(dp, M_DEVBUF);
#endif
	}
}

/*
 * "Mountroot hook" types, functions, and variables.
 */

static hook_list_t mountroothook_list=LIST_HEAD_INITIALIZER(mountroothook_list);

void *
mountroothook_establish(void (*fn)(device_t), device_t dev)
{
	return hook_establish(&mountroothook_list, __FPTRCAST(void (*), fn),
	    dev);
}

void
mountroothook_disestablish(void *vhook)
{
	hook_disestablish(&mountroothook_list, vhook);
}

void
mountroothook_destroy(void)
{
	hook_destroy(&mountroothook_list);
}

void
domountroothook(device_t therootdev)
{
	struct hook_desc *hd;

	LIST_FOREACH(hd, &mountroothook_list, hk_list) {
		if (hd->hk_arg == therootdev) {
			(*hd->hk_fn)(hd->hk_arg);
			return;
		}
	}
}

static hook_list_t exechook_list = LIST_HEAD_INITIALIZER(exechook_list);

void *
exechook_establish(void (*fn)(struct proc *, void *), void *arg)
{
	return hook_establish(&exechook_list, __FPTRCAST(void (*)(void *), fn),
	    arg);
}

void
exechook_disestablish(void *vhook)
{
	hook_disestablish(&exechook_list, vhook);
}

/*
 * Run exec hooks.
 */
void
doexechooks(struct proc *p)
{
	hook_proc_run(&exechook_list, p);
}

static hook_list_t exithook_list = LIST_HEAD_INITIALIZER(exithook_list);
extern krwlock_t exec_lock;

void *
exithook_establish(void (*fn)(struct proc *, void *), void *arg)
{
	void *rv;

	rw_enter(&exec_lock, RW_WRITER);
	rv = hook_establish(&exithook_list, __FPTRCAST(void (*)(void *), fn),
	    arg);
	rw_exit(&exec_lock);
	return rv;
}

void
exithook_disestablish(void *vhook)
{

	rw_enter(&exec_lock, RW_WRITER);
	hook_disestablish(&exithook_list, vhook);
	rw_exit(&exec_lock);
}

/*
 * Run exit hooks.
 */
void
doexithooks(struct proc *p)
{
	hook_proc_run(&exithook_list, p);
}

static hook_list_t forkhook_list = LIST_HEAD_INITIALIZER(forkhook_list);

void *
forkhook_establish(void (*fn)(struct proc *, struct proc *))
{
	return hook_establish(&forkhook_list, __FPTRCAST(void (*)(void *), fn),
	    NULL);
}

void
forkhook_disestablish(void *vhook)
{
	hook_disestablish(&forkhook_list, vhook);
}

/*
 * Run fork hooks.
 */
void
doforkhooks(struct proc *p2, struct proc *p1)
{
	struct hook_desc *hd;

	LIST_FOREACH(hd, &forkhook_list, hk_list) {
		__FPTRCAST(void (*)(struct proc *, struct proc *), *hd->hk_fn)
		    (p2, p1);
	}
}

static hook_list_t critpollhook_list = LIST_HEAD_INITIALIZER(critpollhook_list);

void *
critpollhook_establish(void (*fn)(void *), void *arg)
{
	return hook_establish(&critpollhook_list, fn, arg);
}

void
critpollhook_disestablish(void *vhook)
{
	hook_disestablish(&critpollhook_list, vhook);
}

/*
 * Run critical polling hooks.
 */
void
docritpollhooks(void)
{
	struct hook_desc *hd;

	LIST_FOREACH(hd, &critpollhook_list, hk_list) {
		(*hd->hk_fn)(hd->hk_arg);
	}
}

/*
 * "Power hook" types, functions, and variables.
 * The list of power hooks is kept ordered with the last registered hook
 * first.
 * When running the hooks on power down the hooks are called in reverse
 * registration order, when powering up in registration order.
 */
struct powerhook_desc {
	TAILQ_ENTRY(powerhook_desc) sfd_list;
	void	(*sfd_fn)(int, void *);
	void	*sfd_arg;
	char	sfd_name[16];
};

static TAILQ_HEAD(powerhook_head, powerhook_desc) powerhook_list =
    TAILQ_HEAD_INITIALIZER(powerhook_list);

void *
powerhook_establish(const char *name, void (*fn)(int, void *), void *arg)
{
	struct powerhook_desc *ndp;

	ndp = (struct powerhook_desc *)
	    malloc(sizeof(*ndp), M_DEVBUF, M_NOWAIT);
	if (ndp == NULL)
		return (NULL);

	ndp->sfd_fn = fn;
	ndp->sfd_arg = arg;
	strlcpy(ndp->sfd_name, name, sizeof(ndp->sfd_name));
	TAILQ_INSERT_HEAD(&powerhook_list, ndp, sfd_list);

	aprint_error("%s: WARNING: powerhook_establish is deprecated\n", name);
	return (ndp);
}

void
powerhook_disestablish(void *vhook)
{
#ifdef DIAGNOSTIC
	struct powerhook_desc *dp;

	TAILQ_FOREACH(dp, &powerhook_list, sfd_list)
                if (dp == vhook)
			goto found;
	panic("powerhook_disestablish: hook %p not established", vhook);
 found:
#endif

	TAILQ_REMOVE(&powerhook_list, (struct powerhook_desc *)vhook,
	    sfd_list);
	free(vhook, M_DEVBUF);
}

/*
 * Run power hooks.
 */
void
dopowerhooks(int why)
{
	struct powerhook_desc *dp;
	const char *why_name;
	static const char * pwr_names[] = {PWR_NAMES};
	why_name = why < __arraycount(pwr_names) ? pwr_names[why] : "???";

	if (why == PWR_RESUME || why == PWR_SOFTRESUME) {
		TAILQ_FOREACH_REVERSE(dp, &powerhook_list, powerhook_head,
		    sfd_list)
		{
			if (powerhook_debug)
				printf("dopowerhooks %s: %s (%p)\n",
				    why_name, dp->sfd_name, dp);
			(*dp->sfd_fn)(why, dp->sfd_arg);
		}
	} else {
		TAILQ_FOREACH(dp, &powerhook_list, sfd_list) {
			if (powerhook_debug)
				printf("dopowerhooks %s: %s (%p)\n",
				    why_name, dp->sfd_name, dp);
			(*dp->sfd_fn)(why, dp->sfd_arg);
		}
	}

	if (powerhook_debug)
		printf("dopowerhooks: %s done\n", why_name);
}

/*
 * A simple linear hook.
 */

khook_list_t *
simplehook_create(int ipl, const char *wmsg)
{
	khook_list_t *l;

	l = kmem_zalloc(sizeof(*l), KM_SLEEP);

	mutex_init(&l->hl_lock, MUTEX_DEFAULT, ipl);
	strlcpy(l->hl_namebuf, wmsg, sizeof(l->hl_namebuf));
	cv_init(&l->hl_cv, l->hl_namebuf);
	LIST_INIT(&l->hl_list);
	l->hl_state = HKLIST_IDLE;

	return l;
}

void
simplehook_destroy(khook_list_t *l)
{
	struct hook_desc *hd;

	KASSERT(l->hl_state == HKLIST_IDLE);

	while ((hd = LIST_FIRST(&l->hl_list)) != NULL) {
		LIST_REMOVE(hd, hk_list);
		kmem_free(hd, sizeof(*hd));
	}

	cv_destroy(&l->hl_cv);
	mutex_destroy(&l->hl_lock);
	kmem_free(l, sizeof(*l));
}

int
simplehook_dohooks(khook_list_t *l)
{
	struct hook_desc *hd, *nexthd;
	kmutex_t *cv_lock;
	void (*fn)(void *);
	void *arg;

	mutex_enter(&l->hl_lock);
	if (l->hl_state != HKLIST_IDLE) {
		mutex_exit(&l->hl_lock);
		return EBUSY;
	}

	/* stop removing hooks */
	l->hl_state = HKLIST_INUSE;
	l->hl_lwp = curlwp;

	LIST_FOREACH(hd, &l->hl_list, hk_list) {
		if (hd->hk_fn == NULL)
			continue;

		fn = hd->hk_fn;
		arg = hd->hk_arg;
		l->hl_active_hk = hd;
		l->hl_cvlock = NULL;

		mutex_exit(&l->hl_lock);

		/* do callback without l->hl_lock */
		(*fn)(arg);

		mutex_enter(&l->hl_lock);
		l->hl_active_hk = NULL;
		cv_lock = l->hl_cvlock;

		if (hd->hk_fn == NULL) {
			if (cv_lock != NULL) {
				mutex_exit(&l->hl_lock);
				mutex_enter(cv_lock);
			}

			cv_broadcast(&l->hl_cv);

			if (cv_lock != NULL) {
				mutex_exit(cv_lock);
				mutex_enter(&l->hl_lock);
			}
		}
	}

	/* remove marked node while running hooks */
	LIST_FOREACH_SAFE(hd, &l->hl_list, hk_list, nexthd) {
		if (hd->hk_fn == NULL) {
			LIST_REMOVE(hd, hk_list);
			kmem_free(hd, sizeof(*hd));
		}
	}

	l->hl_lwp = NULL;
	l->hl_state = HKLIST_IDLE;
	mutex_exit(&l->hl_lock);

	return 0;
}

khook_t *
simplehook_establish(khook_list_t *l,  void (*fn)(void *), void *arg)
{
	struct hook_desc *hd;

	hd = kmem_zalloc(sizeof(*hd), KM_SLEEP);
	hd->hk_fn = fn;
	hd->hk_arg = arg;

	mutex_enter(&l->hl_lock);
	LIST_INSERT_HEAD(&l->hl_list, hd, hk_list);
	mutex_exit(&l->hl_lock);

	return hd;
}

void
simplehook_disestablish(khook_list_t *l, khook_t *hd, kmutex_t *lock)
{
	struct hook_desc *hd0 __diagused;
	kmutex_t *cv_lock;

	KASSERT(lock == NULL || mutex_owned(lock));
	mutex_enter(&l->hl_lock);

#ifdef DIAGNOSTIC
	LIST_FOREACH(hd0, &l->hl_list, hk_list) {
		if (hd == hd0)
			break;
	}

	if (hd0 == NULL)
		panic("hook_disestablish: hook %p not established", hd);
#endif

	/* The hook is not referred, remove immidiately */
	if (l->hl_state == HKLIST_IDLE) {
		LIST_REMOVE(hd, hk_list);
		kmem_free(hd, sizeof(*hd));
		mutex_exit(&l->hl_lock);
		return;
	}

	/* remove callback. hd will be removed in dohooks */
	hd->hk_fn = NULL;
	hd->hk_arg = NULL;

	/* If the hook is running, wait for the completion */
	if (l->hl_active_hk == hd &&
	    l->hl_lwp != curlwp) {
		if (lock != NULL) {
			cv_lock = lock;
			KASSERT(l->hl_cvlock == NULL);
			l->hl_cvlock = lock;
			mutex_exit(&l->hl_lock);
		} else {
			cv_lock = &l->hl_lock;
		}

		cv_wait(&l->hl_cv, cv_lock);

		if (lock == NULL)
			mutex_exit(&l->hl_lock);
	} else {
		mutex_exit(&l->hl_lock);
	}
}

bool
simplehook_has_hooks(khook_list_t *l)
{
	bool empty;

	mutex_enter(&l->hl_lock);
	empty = LIST_EMPTY(&l->hl_list);
	mutex_exit(&l->hl_lock);

	return !empty;
}
