/*	$NetBSD: simplehook_tester.c,v 1.2 2022/04/10 09:50:46 andvar Exp $	*/
/*
 * Copyright (c) 2021 Internet Initiative Japan Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: simplehook_tester.c,v 1.2 2022/04/10 09:50:46 andvar Exp $");

#include <sys/param.h>

#include <sys/condvar.h>
#include <sys/hook.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/sysctl.h>
#include <sys/workqueue.h>

#ifdef SIMPLEHOOK_TESTER_DEBUG
#define HK_DPRINTF(a)	printf a
#else
#define HK_DPRINTF(a)	__nothing
#endif

MODULE(MODULE_CLASS_MISC, simplehook_tester, NULL);
extern int simplehook_tester_init(void);
struct tester_context;

struct tester_hook {
	struct tester_context	*th_ctx;
	khook_t			*th_hook;
	size_t			 th_idx;
	int			 th_count;
	bool			 th_stopping;
	bool			 th_stopped;
	bool			 th_disestablish;
};

static struct tester_context {
	kmutex_t		 ctx_mutex;
	kcondvar_t		 ctx_cv;
	struct sysctllog	*ctx_sysctllog;
	struct workqueue	*ctx_wq;
	struct work		 ctx_wk;
	bool			 ctx_wk_enqueued;
	bool			 ctx_wk_waiting;

	khook_list_t		*ctx_hooks;
	struct tester_hook	 ctx_hook[2];

	khook_t			*ctx_nbhook;
} tester_ctx;

static int
simplehook_tester_created(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct tester_context *ctx;
	int error, val;
	size_t i;

	node = *rnode;
	ctx = node.sysctl_data;

	mutex_enter(&ctx->ctx_mutex);
	val = ctx->ctx_hooks != NULL ? 1 : 0;
	mutex_exit(&ctx->ctx_mutex);

	node.sysctl_data = &val;
	node.sysctl_size = sizeof(val);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (val != 0 && val != 1)
		return EINVAL;

	error = 0;
	mutex_enter(&ctx->ctx_mutex);
	if (val == 1) {
		if (ctx->ctx_hooks != NULL) {
			error = EEXIST;
		} else {
			HK_DPRINTF(("[%s, %d]: create hook list\n",
			    __func__, __LINE__));
			ctx->ctx_hooks = simplehook_create(IPL_NONE,
			    "tester hooks");
			KASSERT(ctx->ctx_hooks != NULL);
		}
	} else {
		if (ctx->ctx_hooks == NULL) {
			error = ENXIO;
		} else if (ctx->ctx_wk_waiting) {
			error = EBUSY;
		} else {
			ctx->ctx_wk_waiting = true;
			mutex_exit(&ctx->ctx_mutex);

			workqueue_wait(ctx->ctx_wq, &ctx->ctx_wk);

			mutex_enter(&ctx->ctx_mutex);
			ctx->ctx_wk_waiting = false;

			HK_DPRINTF(("[%s, %d]: destroy hook list\n",
			    __func__, __LINE__));
			simplehook_destroy(ctx->ctx_hooks);
			ctx->ctx_hooks = NULL;
			ctx->ctx_nbhook = NULL;
			for (i = 0; i < __arraycount(ctx->ctx_hook); i++) {
				ctx->ctx_hook[i].th_hook = NULL;
			}
		}
	}
	mutex_exit(&ctx->ctx_mutex);

	return error;
}

static void
simplehook_tester_work(struct work *wk, void *xctx)
{
	struct tester_context *ctx;

	ctx = xctx;

	mutex_enter(&ctx->ctx_mutex);
	ctx->ctx_wk_enqueued = false;
	mutex_exit(&ctx->ctx_mutex);

	simplehook_dohooks(ctx->ctx_hooks);
}

static int
simplehook_tester_dohooks(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct tester_context *ctx;
	int error, val;

	node = *rnode;
	ctx = node.sysctl_data;

	mutex_enter(&ctx->ctx_mutex);
	val = ctx->ctx_wk_enqueued ? 1 : 0;
	mutex_exit(&ctx->ctx_mutex);

	node.sysctl_data = &val;
	node.sysctl_size = sizeof(val);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (val != 0 && val != 1)
		return EINVAL;

	mutex_enter(&ctx->ctx_mutex);
	if (val == 1) {
		if (ctx->ctx_wk_enqueued) {
			error = EEXIST;
		} else if (ctx->ctx_wk_waiting) {
			error = EBUSY;
		} else if (ctx->ctx_hooks == NULL) {
			error = ENXIO;
		} else {
			HK_DPRINTF(("[%s, %d]: dohook\n", __func__, __LINE__));
			ctx->ctx_wk_enqueued = true;
			workqueue_enqueue(ctx->ctx_wq,
			    &ctx->ctx_wk, NULL);
		}
	} else {
		if (ctx->ctx_wk_waiting) {
			error = EBUSY;
		} else {
			ctx->ctx_wk_waiting = true;
			mutex_exit(&ctx->ctx_mutex);

			workqueue_wait(ctx->ctx_wq, &ctx->ctx_wk);

			mutex_enter(&ctx->ctx_mutex);
			ctx->ctx_wk_waiting = false;
		}
	}
	mutex_exit(&ctx->ctx_mutex);

	return error;
}

static void
simplehook_tester_hook(void *xth)
{
	struct tester_context *ctx;
	struct tester_hook *th;

	th = xth;
	ctx = th->th_ctx;
	mutex_enter(&ctx->ctx_mutex);

	HK_DPRINTF(("[%s, %d]: hook%zu called\n",
	    __func__, __LINE__, th->th_idx));

	th->th_stopped = false;

	while (th->th_stopping) {
		HK_DPRINTF(("[%s, %d]: hook%zu stopping\n",
		    __func__, __LINE__, th->th_idx));
		th->th_stopped = true;
		cv_wait(&ctx->ctx_cv, &ctx->ctx_mutex);
	}

	if (th->th_stopped) {
		HK_DPRINTF(("[%s, %d]: hook%zu restart\n",
		    __func__, __LINE__, th->th_idx));
		th->th_stopped = false;
	}

	th->th_count++;

	if (th->th_disestablish && th->th_hook != NULL) {
		HK_DPRINTF(("[%s, %d]: disestablish running hook%zu\n",
		    __func__, __LINE__, th->th_idx));
		simplehook_disestablish(ctx->ctx_hooks,
		    th->th_hook, &ctx->ctx_mutex);
		th->th_hook = NULL;
	}

	HK_DPRINTF(("[%s, %d]: hook%zu exit\n",
	    __func__, __LINE__, th->th_idx));

	mutex_exit(&ctx->ctx_mutex);
}

static void
simplehook_tester_hook_nb(void *xctx)
{

	HK_DPRINTF(("[%s, %d]: non-block hook called\n",
	    __func__, __LINE__));

	HK_DPRINTF(("[%s, %d]: sleep 1 sec\n",
	    __func__, __LINE__));
	kpause("smplhk_nb", true, 1 * hz, NULL);

	HK_DPRINTF(("[%s, %d]: non-block hook exit\n",
	    __func__, __LINE__));
}

static int
simplehook_tester_established(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct tester_context *ctx;
	struct tester_hook *th;
	int val, error;

	node = *rnode;
	th = node.sysctl_data;
	ctx = th->th_ctx;

	mutex_enter(&ctx->ctx_mutex);
	val = th->th_hook == NULL ? 0 : 1;
	mutex_exit(&ctx->ctx_mutex);

	node.sysctl_data = &val;
	node.sysctl_size = sizeof(val);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	error = 0;
	mutex_enter(&ctx->ctx_mutex);

	if (val == 1) {
		if (th->th_hook != NULL) {
			error = EEXIST;
		} else {
			th->th_hook = simplehook_establish(ctx->ctx_hooks,
			    simplehook_tester_hook, th);
			KASSERT(th->th_hook != NULL);
			HK_DPRINTF(("[%s, %d]: established hook%zu (%p)\n",
			    __func__, __LINE__, th->th_idx, th->th_hook));
		}
	} else {
		if (th->th_hook == NULL) {
			error = ENXIO;
		} else {
			bool stopped = false;
			if (th->th_stopping) {
				HK_DPRINTF(("[%s, %d]: stopping = false\n",
				    __func__, __LINE__));
				th->th_stopping = false;
				cv_broadcast(&ctx->ctx_cv);
				stopped = true;
			}
			HK_DPRINTF(("[%s, %d]: disestablish hook%zu (%p)\n",
			    __func__, __LINE__, th->th_idx, th->th_hook));
			simplehook_disestablish(ctx->ctx_hooks,
			    th->th_hook, &ctx->ctx_mutex);
			th->th_hook = NULL;
			if (stopped) {
				HK_DPRINTF(("[%s, %d]: disestablished hook%zu\n",
				    __func__, __LINE__, th->th_idx));
			}
		}
	}

	mutex_exit(&ctx->ctx_mutex);

	return error;
}

static int
simplehook_tester_established_nb(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct tester_context *ctx;
	int val, error;

	node = *rnode;
	ctx = node.sysctl_data;

	mutex_enter(&ctx->ctx_mutex);
	val = ctx->ctx_nbhook == NULL ? 0 : 1;
	mutex_exit(&ctx->ctx_mutex);

	node.sysctl_data = &val;
	node.sysctl_size = sizeof(val);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	error = 0;
	mutex_enter(&ctx->ctx_mutex);

	if (val == 1) {
		if (ctx->ctx_nbhook != NULL) {
			error = EEXIST;
		} else {
			ctx->ctx_nbhook = simplehook_establish(ctx->ctx_hooks,
			    simplehook_tester_hook_nb, ctx);
			KASSERT(ctx->ctx_nbhook != NULL);
			HK_DPRINTF(("[%s, %d]: established nbhook (%p)\n",
			    __func__, __LINE__, ctx->ctx_nbhook));
		}
	} else {
		if (ctx->ctx_nbhook == NULL) {
			error = ENXIO;
		} else {
			HK_DPRINTF(("[%s, %d]: disestablish nbhook (%p)\n",
			    __func__, __LINE__, ctx->ctx_nbhook));
			simplehook_disestablish(ctx->ctx_hooks,
			    ctx->ctx_nbhook, NULL);
			ctx->ctx_nbhook = NULL;
			HK_DPRINTF(("[%s, %d]: disestablished\n",
			    __func__, __LINE__));
		}
	}

	mutex_exit(&ctx->ctx_mutex);

	return error;
}

static int
simplehook_tester_stopping(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct tester_context *ctx;
	struct tester_hook *th;
	int error;
	bool val;

	node = *rnode;
	th = node.sysctl_data;
	ctx = th->th_ctx;

	mutex_enter(&ctx->ctx_mutex);
	val = th->th_stopping;
	mutex_exit(&ctx->ctx_mutex);

	node.sysctl_data = &val;
	node.sysctl_size = sizeof(val);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	error = 0;
	mutex_enter(&ctx->ctx_mutex);
	if (val == true && !th->th_stopping) {
		th->th_stopping = true;
	} else if (val == false && th->th_stopping) {
		th->th_stopping = false;
		cv_broadcast(&ctx->ctx_cv);
	}
	mutex_exit(&ctx->ctx_mutex);

	return error;
}

static int
simplehook_tester_stopped(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct tester_context *ctx;
	struct tester_hook *th;
	bool val;

	node = *rnode;
	th = node.sysctl_data;
	ctx = th->th_ctx;

	mutex_enter(&ctx->ctx_mutex);
	val = th->th_stopped;
	mutex_exit(&ctx->ctx_mutex);

	node.sysctl_data = &val;
	node.sysctl_size = sizeof(val);

	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
simplehook_tester_disestablish(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct tester_context *ctx;
	struct tester_hook *th;
	int error;
	bool val;

	node = *rnode;
	th = node.sysctl_data;
	ctx = th->th_ctx;

	mutex_enter(&ctx->ctx_mutex);
	val = th->th_disestablish;
	mutex_exit(&ctx->ctx_mutex);

	node.sysctl_data = &val;
	node.sysctl_size = sizeof(val);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (val != 0 && val != 1)
		return EINVAL;

	error = 0;
	mutex_enter(&ctx->ctx_mutex);
	if (val == true && !th->th_disestablish) {
		th->th_disestablish = true;
	} else if (val == false && th->th_disestablish) {
		th->th_disestablish = false;
	}
	mutex_exit(&ctx->ctx_mutex);

	return 0;
}

static int
simplehook_tester_count(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct tester_context *ctx;
	struct tester_hook *th;
	int error, val;

	node = *rnode;
	th = node.sysctl_data;
	ctx = th->th_ctx;

	mutex_enter(&ctx->ctx_mutex);
	val = th->th_count;
	mutex_exit(&ctx->ctx_mutex);

	node.sysctl_data = &val;
	node.sysctl_size = sizeof(val);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	mutex_enter(&ctx->ctx_mutex);
	th->th_count = val;
	mutex_exit(&ctx->ctx_mutex);

	return 0;
}

static int
simplehook_tester_create_sysctl(struct tester_context *ctx)
{
	struct sysctllog **log;
	const struct sysctlnode *rnode, *cnode;
	void *ptr;
	char buf[32];
	int error;
	size_t i;

	log = &ctx->ctx_sysctllog;
	ptr = (void *)ctx;

	error = sysctl_createv(log, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "simplehook_tester",
	    SYSCTL_DESCR("simplehook testing interface"),
	    NULL, 0, NULL, 0, CTL_KERN, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto bad;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hook_list",
	    SYSCTL_DESCR("hook list"), NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	error = sysctl_createv(log, 0, &cnode, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "created", SYSCTL_DESCR("create and destroy hook list"),
	    simplehook_tester_created, 0, ptr, 0,
	    CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto bad;

	error = sysctl_createv(log, 0, &cnode, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "dohooks", SYSCTL_DESCR("do hooks"),
	    simplehook_tester_dohooks, 0, ptr, 0,
	    CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto bad;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "nbhook",
	    SYSCTL_DESCR("non-blocking hook"), NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto bad;

	error = sysctl_createv(log, 0, &cnode, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "established",
	    SYSCTL_DESCR("establish and disestablish hook"),
	    simplehook_tester_established_nb,
	    0, ptr, 0, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto bad;

	for (i = 0; i < __arraycount(ctx->ctx_hook); i++) {
		snprintf(buf, sizeof(buf), "hook%zu", i);
		ptr = (void *)&ctx->ctx_hook[i];

		error = sysctl_createv(log, 0, &rnode, &cnode,
		    CTLFLAG_PERMANENT, CTLTYPE_NODE, buf,
		    SYSCTL_DESCR("hook information"), NULL, 0, NULL, 0,
		    CTL_CREATE, CTL_EOL);
		if (error != 0)
			goto bad;

		error = sysctl_createv(log, 0, &cnode, NULL,
		    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		    CTLTYPE_INT, "established",
		    SYSCTL_DESCR("establish and disestablish hook"),
		    simplehook_tester_established,
		    0, ptr, 0, CTL_CREATE, CTL_EOL);
		if (error != 0)
			goto bad;

		error = sysctl_createv(log, 0, &cnode, NULL,
		    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		    CTLTYPE_BOOL, "stopping",
		    SYSCTL_DESCR("stopping at beginning of the hook"),
		    simplehook_tester_stopping, 0, ptr, 0,
		    CTL_CREATE, CTL_EOL);
		if (error != 0)
			goto bad;

		error = sysctl_createv(log, 0, &cnode, NULL,
		    CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		    CTLTYPE_BOOL, "stopped",
		    SYSCTL_DESCR("the hook is stopped"),
		    simplehook_tester_stopped, 0, ptr, 0,
		    CTL_CREATE, CTL_EOL);
		if (error != 0)
			goto bad;

		error = sysctl_createv(log, 0, &cnode, NULL,
		    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_BOOL,
		    "disestablish_in_hook",
		    SYSCTL_DESCR("disestablish this hook in it"),
		    simplehook_tester_disestablish, 0, ptr, 0,
		    CTL_CREATE, CTL_EOL);
		if (error != 0)
			goto bad;

		error = sysctl_createv(log, 0, &cnode, NULL,
		    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		    CTLTYPE_INT, "count",
		    SYSCTL_DESCR("the number of calls of the hook"),
		    simplehook_tester_count, 0, ptr, 0,
		    CTL_CREATE, CTL_EOL);
		if (error != 0)
			goto bad;
	}

	HK_DPRINTF(("[%s, %d]: created sysctls\n", __func__, __LINE__));
	return 0;

bad:
	sysctl_teardown(log);
	return error;
}

static void
simplehook_tester_init_ctx(struct tester_context *ctx)
{
	size_t i;

	memset(ctx, 0, sizeof(*ctx));
	mutex_init(&ctx->ctx_mutex, MUTEX_DEFAULT, IPL_NONE);
	workqueue_create(&ctx->ctx_wq, "shook_tester_wq",
	    simplehook_tester_work, ctx, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	cv_init(&ctx->ctx_cv, "simplehook_tester_cv");

	for (i = 0; i < __arraycount(ctx->ctx_hook); i++) {
		ctx->ctx_hook[i].th_ctx = ctx;
		ctx->ctx_hook[i].th_idx = i;
	}
}


int
simplehook_tester_init(void)
{
	int error;

	simplehook_tester_init_ctx(&tester_ctx);
	error = simplehook_tester_create_sysctl(&tester_ctx);

	return error;
}

static int
simplehook_tester_fini(void)
{
	struct tester_context *ctx;
	struct tester_hook *th;
	khook_list_t *hooks;
	size_t i;

	ctx = &tester_ctx;

	sysctl_teardown(&ctx->ctx_sysctllog);

	mutex_enter(&ctx->ctx_mutex);

	hooks = ctx->ctx_hooks;
	ctx->ctx_hooks = NULL;
	ctx->ctx_wk_waiting = true;

	for (i = 0; i < __arraycount(ctx->ctx_hook); i++) {
		th = &ctx->ctx_hook[i];
		th->th_stopping = false;
	}
	cv_broadcast(&ctx->ctx_cv);

	mutex_exit(&ctx->ctx_mutex);

	workqueue_wait(ctx->ctx_wq, &ctx->ctx_wk);

	workqueue_destroy(ctx->ctx_wq);
	if (hooks != NULL)
		simplehook_destroy(hooks);
	cv_destroy(&ctx->ctx_cv);
	mutex_destroy(&ctx->ctx_mutex);

	return 0;
}

static int
simplehook_tester_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = simplehook_tester_init();
		break;

	case MODULE_CMD_FINI:
		error = simplehook_tester_fini();
		break;

	case MODULE_CMD_STAT:
	default:
		error = ENOTTY;
	}

	return error;
}
