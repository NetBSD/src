/*	$NetBSD: threadpool_tester.c,v 1.3 2018/12/26 22:21:10 thorpej Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: threadpool_tester.c,v 1.3 2018/12/26 22:21:10 thorpej Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/threadpool.h>

MODULE(MODULE_CLASS_MISC, threadpool_tester, NULL);

#ifdef THREADPOOL_VERBOSE
#define	TP_LOG(x)		printf x
#else
#define	TP_LOG(x)		/* nothing */
#endif /* THREADPOOL_VERBOSE */

static struct tester_context {
	kmutex_t ctx_mutex;
	struct sysctllog *ctx_sysctllog;
	struct threadpool *ctx_unbound[PRI_COUNT + 1];
	struct threadpool_percpu *ctx_percpu[PRI_COUNT + 1];
	unsigned int ctx_value;
	struct threadpool_job ctx_job;
} tester_ctx;

#define	pri_to_idx(pri)		((pri) == PRI_NONE ? PRI_COUNT : (pri))

static bool
pri_is_valid(pri_t pri)
{
	return (pri == PRI_NONE || (pri >= PRI_USER && pri < PRI_COUNT));
}

static int
threadpool_tester_get_unbound(SYSCTLFN_ARGS)
{
	struct tester_context *ctx;
	struct threadpool *pool, *opool = NULL;
	struct sysctlnode node;
	int error, val;

	node = *rnode;
	ctx = node.sysctl_data;

	val = -1;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (! pri_is_valid(val))
		return EINVAL;

	error = threadpool_get(&pool, val);
	if (error) {
		TP_LOG(("%s: threadpool_get(..., %d) failed -> %d\n",
		    __func__, val, error));
		return error;
	}

	mutex_enter(&ctx->ctx_mutex);
	if (ctx->ctx_unbound[pri_to_idx(val)] == NULL)
		ctx->ctx_unbound[pri_to_idx(val)] = pool;
	else
		opool = ctx->ctx_unbound[pri_to_idx(val)];
	mutex_exit(&ctx->ctx_mutex);

	if (opool != NULL) {
		/* Should have gotten reference to existing pool. */
		TP_LOG(("%s: found existing unbound pool for pri %d (%s)\n",
		    __func__, val, opool == pool ? "match" : "NO MATCH"));
		KASSERT(opool == pool);
		threadpool_put(pool, val);
		error = EEXIST;
	} else {
		TP_LOG(("%s: created unbound pool for pri %d\n",
		    __func__, val));
	}

	return error;
}

static int
threadpool_tester_put_unbound(SYSCTLFN_ARGS)
{
	struct tester_context *ctx;
	struct threadpool *pool;
	struct sysctlnode node;
	int error, val;

	node = *rnode;
	ctx = node.sysctl_data;

	val = -1;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (! pri_is_valid(val))
		return EINVAL;

	mutex_enter(&ctx->ctx_mutex);
	/* We only ever maintain a single reference. */
	pool = ctx->ctx_unbound[pri_to_idx(val)];
	ctx->ctx_unbound[pri_to_idx(val)] = NULL;
	mutex_exit(&ctx->ctx_mutex);

	if (pool == NULL) {
		TP_LOG(("%s: no unbound pool for pri %d\n",
		    __func__, val));
		return ENODEV;
	}

	threadpool_put(pool, val);
	TP_LOG(("%s: released unbound pool for pri %d\n",
	    __func__, val));

	return 0;
}

static int
threadpool_tester_run_unbound(SYSCTLFN_ARGS)
{
	struct tester_context *ctx;
	struct threadpool *pool;
	struct sysctlnode node;
	int error, val;

	node = *rnode;
	ctx = node.sysctl_data;

	val = -1;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (! pri_is_valid(val))
		return EINVAL;

	mutex_enter(&ctx->ctx_mutex);
	pool = ctx->ctx_unbound[pri_to_idx(val)];
	if (pool == NULL) {
		TP_LOG(("%s: no unbound pool for pri %d\n",
		    __func__, val));
		mutex_exit(&ctx->ctx_mutex);
		return ENODEV;
	}

	threadpool_schedule_job(pool, &ctx->ctx_job);
	TP_LOG(("%s: scheduled job on unbound pool for pri %d\n",
	    __func__, val));
	mutex_exit(&ctx->ctx_mutex);

	return 0;
}

static int
threadpool_tester_get_percpu(SYSCTLFN_ARGS)
{
	struct tester_context *ctx;
	struct threadpool_percpu *pcpu, *opcpu = NULL;
	struct sysctlnode node;
	int error, val;

	node = *rnode;
	ctx = node.sysctl_data;

	val = -1;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (! pri_is_valid(val))
		return EINVAL;

	error = threadpool_percpu_get(&pcpu, val);
	if (error) {
		TP_LOG(("%s: threadpool_percpu_get(..., %d) failed -> %d\n",
		    __func__, val, error));
		return error;
	}

	mutex_enter(&ctx->ctx_mutex);
	if (ctx->ctx_percpu[pri_to_idx(val)] == NULL)
		ctx->ctx_percpu[pri_to_idx(val)] = pcpu;
	else
		opcpu = ctx->ctx_percpu[pri_to_idx(val)];
	mutex_exit(&ctx->ctx_mutex);

	if (opcpu != NULL) {
		/* Should have gotten reference to existing pool. */
		TP_LOG(("%s: found existing unbound pool for pri %d (%s)\n",
		    __func__, val, opcpu == pcpu ? "match" : "NO MATCH"));
		KASSERT(opcpu == pcpu);
		threadpool_percpu_put(pcpu, val);
		error = EEXIST;
	} else {
		TP_LOG(("%s: created percpu pool for pri %d\n",
		    __func__, val));
	}

	return error;
}

static int
threadpool_tester_put_percpu(SYSCTLFN_ARGS)
{
	struct tester_context *ctx;
	struct threadpool_percpu *pcpu;
	struct sysctlnode node;
	int error, val;

	node = *rnode;
	ctx = node.sysctl_data;

	val = -1;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (! pri_is_valid(val))
		return EINVAL;

	mutex_enter(&ctx->ctx_mutex);
	/* We only ever maintain a single reference. */
	pcpu = ctx->ctx_percpu[pri_to_idx(val)];
	ctx->ctx_percpu[pri_to_idx(val)] = NULL;
	mutex_exit(&ctx->ctx_mutex);

	if (pcpu == NULL) {
		TP_LOG(("%s: no percpu pool for pri %d\n",
		    __func__, val));
		return ENODEV;
	}

	threadpool_percpu_put(pcpu, val);
	TP_LOG(("%s: released percpu pool for pri %d\n",
	    __func__, val));

	return 0;
}

static int
threadpool_tester_run_percpu(SYSCTLFN_ARGS)
{
	struct tester_context *ctx;
	struct threadpool_percpu *pcpu;
	struct threadpool *pool;
	struct sysctlnode node;
	int error, val;

	node = *rnode;
	ctx = node.sysctl_data;

	val = -1;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (! pri_is_valid(val))
		return EINVAL;

	mutex_enter(&ctx->ctx_mutex);
	pcpu = ctx->ctx_percpu[pri_to_idx(val)];
	if (pcpu == NULL) {
		TP_LOG(("%s: no percpu pool for pri %d\n",
		    __func__, val));
		mutex_exit(&ctx->ctx_mutex);
		return ENODEV;
	}

	pool = threadpool_percpu_ref(pcpu);
	KASSERT(pool != NULL);

	threadpool_schedule_job(pool, &ctx->ctx_job);
	TP_LOG(("%s: scheduled job on percpu pool for pri %d\n",
	    __func__, val));
	mutex_exit(&ctx->ctx_mutex);

	return 0;
}

static int
threadpool_tester_test_value(SYSCTLFN_ARGS)
{
	struct tester_context *ctx;
	struct sysctlnode node;
	unsigned int val;
	int error;

	node = *rnode;
	ctx = node.sysctl_data;

	mutex_enter(&ctx->ctx_mutex);
	val = ctx->ctx_value;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		mutex_exit(&ctx->ctx_mutex);
		return error;
	}
	ctx->ctx_value = val;
	mutex_exit(&ctx->ctx_mutex);

	return 0;
}

static void
threadpool_tester_job(struct threadpool_job *job)
{
	struct tester_context *ctx =
	    container_of(job, struct tester_context, ctx_job);
	unsigned int oval, nval;

	TP_LOG(("%s: job = %p, ctx = %p\n", __func__, job, ctx));

	mutex_enter(&ctx->ctx_mutex);
	oval = ctx->ctx_value;
	nval = oval + 1;	/* always reference oval and nval */
	ctx->ctx_value = nval;
	mutex_exit(&ctx->ctx_mutex);

	TP_LOG(("%s: %u -> %u\n", __func__, oval, nval));
	(void) kpause("tptestjob", false, hz, NULL);

	mutex_enter(&ctx->ctx_mutex);
	threadpool_job_done(job);
	mutex_exit(&ctx->ctx_mutex);
}

#define	RETURN_ERROR	if (error) goto return_error

static int
threadpool_tester_init(void)
{
	struct sysctllog **log = &tester_ctx.ctx_sysctllog;
	const struct sysctlnode *rnode, *cnode;
	int error;

	mutex_init(&tester_ctx.ctx_mutex, MUTEX_DEFAULT, IPL_NONE);
	threadpool_job_init(&tester_ctx.ctx_job, threadpool_tester_job,
	    &tester_ctx.ctx_mutex, "tptest");

	error = sysctl_createv(log, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "threadpool_tester",
	    SYSCTL_DESCR("threadpool testing interface"),
	    NULL, 0, NULL, 0, CTL_KERN, CTL_CREATE, CTL_EOL);
	RETURN_ERROR;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "get_unbound",
	    SYSCTL_DESCR("get unbound pool of specified priority"),
	    threadpool_tester_get_unbound, 0,
	    (void *)&tester_ctx, 0, CTL_CREATE, CTL_EOL);
	RETURN_ERROR;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "put_unbound",
	    SYSCTL_DESCR("put unbound pool of specified priority"),
	    threadpool_tester_put_unbound, 0,
	    (void *)&tester_ctx, 0, CTL_CREATE, CTL_EOL);
	RETURN_ERROR;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "run_unbound",
	    SYSCTL_DESCR("run on unbound pool of specified priority"),
	    threadpool_tester_run_unbound, 0,
	    (void *)&tester_ctx, 0, CTL_CREATE, CTL_EOL);
	RETURN_ERROR;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "get_percpu",
	    SYSCTL_DESCR("get percpu pool of specified priority"),
	    threadpool_tester_get_percpu, 0,
	    (void *)&tester_ctx, 0, CTL_CREATE, CTL_EOL);
	RETURN_ERROR;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "put_percpu",
	    SYSCTL_DESCR("put percpu pool of specified priority"),
	    threadpool_tester_put_percpu, 0,
	    (void *)&tester_ctx, 0, CTL_CREATE, CTL_EOL);
	RETURN_ERROR;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "run_percpu",
	    SYSCTL_DESCR("run on percpu pool of specified priority"),
	    threadpool_tester_run_percpu, 0,
	    (void *)&tester_ctx, 0, CTL_CREATE, CTL_EOL);
	RETURN_ERROR;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "test_value",
	    SYSCTL_DESCR("test value that jobs increment"),
	    threadpool_tester_test_value, 0,
	    (void *)&tester_ctx, 0, CTL_CREATE, CTL_EOL);
	RETURN_ERROR;

	return 0;

 return_error:
 	sysctl_teardown(log);
	return error;
}

static int
threadpool_tester_fini(void)
{
	pri_t pri;

	mutex_enter(&tester_ctx.ctx_mutex);
	for (pri = PRI_NONE/*-1*/; pri < PRI_COUNT; pri++) {
		struct threadpool *pool =
		    tester_ctx.ctx_unbound[pri_to_idx(pri)];
		struct threadpool_percpu *pcpu =
		    tester_ctx.ctx_percpu[pri_to_idx(pri)];

		/*
		 * threadpool_cancel_job() may be called on a pool
		 * other than what the job is scheduled on. This is
		 * safe; see comment in threadpool_cancel_job_async().
		 */

		if (pool != NULL) {
			threadpool_cancel_job(pool, &tester_ctx.ctx_job);
			threadpool_put(pool, pri);
			tester_ctx.ctx_unbound[pri_to_idx(pri)] = NULL;
		}
		if (pcpu != NULL) {
			pool = threadpool_percpu_ref(pcpu);
			threadpool_cancel_job(pool, &tester_ctx.ctx_job);
			threadpool_percpu_put(pcpu, pri);
			tester_ctx.ctx_percpu[pri_to_idx(pri)] = NULL;
		}
	}
	mutex_exit(&tester_ctx.ctx_mutex);
	threadpool_job_destroy(&tester_ctx.ctx_job);
	mutex_destroy(&tester_ctx.ctx_mutex);

	sysctl_teardown(&tester_ctx.ctx_sysctllog);

	return 0;
}

static int
threadpool_tester_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = threadpool_tester_init();
		break;

	case MODULE_CMD_FINI:
		error = threadpool_tester_fini();
		break;

	case MODULE_CMD_STAT:
	default:
		error = ENOTTY;
	}

	return error;
}
