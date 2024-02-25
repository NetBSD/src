/*	$NetBSD: test-async.c,v 1.2.2.2 2024/02/25 15:44:21 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

/* aliases for the exported symbols */

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/ht.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

#include <ns/client.h>
#include <ns/events.h>
#include <ns/hooks.h>
#include <ns/log.h>
#include <ns/query.h>
#include <ns/types.h>

#define CHECK(op)                              \
	do {                                   \
		result = (op);                 \
		if (result != ISC_R_SUCCESS) { \
			goto cleanup;          \
		}                              \
	} while (0)

/*
 * Persistent data for use by this module. This will be associated
 * with client object address in the hash table, and will remain
 * accessible until the client object is detached.
 */
typedef struct async_instance {
	ns_plugin_t *module;
	isc_mem_t *mctx;
	isc_ht_t *ht;
	isc_mutex_t hlock;
	isc_log_t *lctx;
} async_instance_t;

typedef struct state {
	bool async;
	ns_hook_resevent_t *rev;
	ns_hookpoint_t hookpoint;
	isc_result_t origresult;
} state_t;

/*
 * Forward declarations of functions referenced in install_hooks().
 */
static ns_hookresult_t
async_qctx_initialize(void *arg, void *cbdata, isc_result_t *resp);
static ns_hookresult_t
async_query_done_begin(void *arg, void *cbdata, isc_result_t *resp);
static ns_hookresult_t
async_qctx_destroy(void *arg, void *cbdata, isc_result_t *resp);

/*%
 * Register the functions to be called at each hook point in 'hooktable', using
 * memory context 'mctx' for allocating copies of stack-allocated structures
 * passed to ns_hook_add().  Make sure 'inst' will be passed as the 'cbdata'
 * argument to every callback.
 */
static void
install_hooks(ns_hooktable_t *hooktable, isc_mem_t *mctx,
	      async_instance_t *inst) {
	const ns_hook_t async_init = {
		.action = async_qctx_initialize,
		.action_data = inst,
	};
	const ns_hook_t async_donebegin = {
		.action = async_query_done_begin,
		.action_data = inst,
	};
	const ns_hook_t async_destroy = {
		.action = async_qctx_destroy,
		.action_data = inst,
	};

	ns_hook_add(hooktable, mctx, NS_QUERY_QCTX_INITIALIZED, &async_init);
	ns_hook_add(hooktable, mctx, NS_QUERY_DONE_BEGIN, &async_donebegin);
	ns_hook_add(hooktable, mctx, NS_QUERY_QCTX_DESTROYED, &async_destroy);
}

static void
logmsg(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	isc_log_write(ns_lctx, NS_LOGCATEGORY_GENERAL, NS_LOGMODULE_HOOKS,
		      ISC_LOG_INFO, fmt, ap);
	va_end(ap);
}

/**
** Mandatory plugin API functions:
**
** - plugin_destroy
** - plugin_register
** - plugin_version
** - plugin_check
**/

/*
 * Called by ns_plugin_register() to initialize the plugin and
 * register hook functions into the view hook table.
 */
isc_result_t
plugin_register(const char *parameters, const void *cfg, const char *cfg_file,
		unsigned long cfg_line, isc_mem_t *mctx, isc_log_t *lctx,
		void *actx, ns_hooktable_t *hooktable, void **instp) {
	async_instance_t *inst = NULL;

	UNUSED(parameters);
	UNUSED(cfg);
	UNUSED(actx);

	isc_log_write(lctx, NS_LOGCATEGORY_GENERAL, NS_LOGMODULE_HOOKS,
		      ISC_LOG_INFO,
		      "registering 'test-async' module from %s:%lu", cfg_file,
		      cfg_line);

	inst = isc_mem_get(mctx, sizeof(*inst));
	*inst = (async_instance_t){ .mctx = NULL };
	isc_mem_attach(mctx, &inst->mctx);

	isc_ht_init(&inst->ht, mctx, 16, ISC_HT_CASE_SENSITIVE);
	isc_mutex_init(&inst->hlock);

	/*
	 * Set hook points in the view's hooktable.
	 */
	install_hooks(hooktable, mctx, inst);

	*instp = inst;

	return (ISC_R_SUCCESS);
}

isc_result_t
plugin_check(const char *parameters, const void *cfg, const char *cfg_file,
	     unsigned long cfg_line, isc_mem_t *mctx, isc_log_t *lctx,
	     void *actx) {
	UNUSED(parameters);
	UNUSED(cfg);
	UNUSED(cfg_file);
	UNUSED(cfg_line);
	UNUSED(mctx);
	UNUSED(lctx);
	UNUSED(actx);

	return (ISC_R_SUCCESS);
}

/*
 * Called by ns_plugins_free(); frees memory allocated by
 * the module when it was registered.
 */
void
plugin_destroy(void **instp) {
	async_instance_t *inst = (async_instance_t *)*instp;

	if (inst->ht != NULL) {
		isc_ht_destroy(&inst->ht);
		isc_mutex_destroy(&inst->hlock);
	}

	isc_mem_putanddetach(&inst->mctx, inst, sizeof(*inst));
	*instp = NULL;

	return;
}

/*
 * Returns plugin API version for compatibility checks.
 */
int
plugin_version(void) {
	return (NS_PLUGIN_VERSION);
}

static state_t *
client_state_get(const query_ctx_t *qctx, async_instance_t *inst) {
	state_t *state = NULL;
	isc_result_t result;

	LOCK(&inst->hlock);
	result = isc_ht_find(inst->ht, (const unsigned char *)&qctx->client,
			     sizeof(qctx->client), (void **)&state);
	UNLOCK(&inst->hlock);

	return (result == ISC_R_SUCCESS ? state : NULL);
}

static void
client_state_create(const query_ctx_t *qctx, async_instance_t *inst) {
	state_t *state = NULL;
	isc_result_t result;

	state = isc_mem_get(inst->mctx, sizeof(*state));
	*state = (state_t){ .async = false };

	LOCK(&inst->hlock);
	result = isc_ht_add(inst->ht, (const unsigned char *)&qctx->client,
			    sizeof(qctx->client), state);
	UNLOCK(&inst->hlock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

static void
client_state_destroy(const query_ctx_t *qctx, async_instance_t *inst) {
	state_t *state = client_state_get(qctx, inst);
	isc_result_t result;

	if (state == NULL) {
		return;
	}

	LOCK(&inst->hlock);
	result = isc_ht_delete(inst->ht, (const unsigned char *)&qctx->client,
			       sizeof(qctx->client));
	UNLOCK(&inst->hlock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	isc_mem_put(inst->mctx, state, sizeof(*state));
}

static ns_hookresult_t
async_qctx_initialize(void *arg, void *cbdata, isc_result_t *resp) {
	query_ctx_t *qctx = (query_ctx_t *)arg;
	async_instance_t *inst = (async_instance_t *)cbdata;
	state_t *state = NULL;

	logmsg("qctx init hook");
	*resp = ISC_R_UNSET;

	state = client_state_get(qctx, inst);
	if (state == NULL) {
		client_state_create(qctx, inst);
	}

	return (NS_HOOK_CONTINUE);
}

static void
cancelasync(ns_hookasync_t *hctx) {
	UNUSED(hctx);
	logmsg("cancelasync");
}

static void
destroyasync(ns_hookasync_t **ctxp) {
	ns_hookasync_t *ctx = *ctxp;

	logmsg("destroyasync");
	*ctxp = NULL;
	isc_mem_putanddetach(&ctx->mctx, ctx, sizeof(*ctx));
}

static isc_result_t
doasync(query_ctx_t *qctx, isc_mem_t *mctx, void *arg, isc_task_t *task,
	isc_taskaction_t action, void *evarg, ns_hookasync_t **ctxp) {
	ns_hook_resevent_t *rev = (ns_hook_resevent_t *)isc_event_allocate(
		mctx, task, NS_EVENT_HOOKASYNCDONE, action, evarg,
		sizeof(*rev));
	ns_hookasync_t *ctx = isc_mem_get(mctx, sizeof(*ctx));
	state_t *state = (state_t *)arg;

	logmsg("doasync");
	*ctx = (ns_hookasync_t){ .mctx = NULL };
	isc_mem_attach(mctx, &ctx->mctx);
	ctx->cancel = cancelasync;
	ctx->destroy = destroyasync;

	rev->hookpoint = state->hookpoint;
	rev->origresult = state->origresult;
	qctx->result = DNS_R_NOTIMP;
	rev->saved_qctx = qctx;
	rev->ctx = ctx;

	state->rev = rev;

	isc_task_send(task, (isc_event_t **)&rev);

	*ctxp = ctx;
	return (ISC_R_SUCCESS);
}

static ns_hookresult_t
async_query_done_begin(void *arg, void *cbdata, isc_result_t *resp) {
	query_ctx_t *qctx = (query_ctx_t *)arg;
	async_instance_t *inst = (async_instance_t *)cbdata;
	state_t *state = client_state_get(qctx, inst);

	UNUSED(qctx);
	UNUSED(cbdata);
	UNUSED(state);

	logmsg("done begin hook");
	if (state->async) {
		/* resuming */
		state->async = false;
		return (NS_HOOK_CONTINUE);
	}

	/* initial call */
	state->async = true;
	state->hookpoint = NS_QUERY_DONE_BEGIN;
	state->origresult = *resp;
	ns_query_hookasync(qctx, doasync, state);
	return (NS_HOOK_RETURN);
}

static ns_hookresult_t
async_qctx_destroy(void *arg, void *cbdata, isc_result_t *resp) {
	query_ctx_t *qctx = (query_ctx_t *)arg;
	async_instance_t *inst = (async_instance_t *)cbdata;

	logmsg("qctx destroy hook");
	*resp = ISC_R_UNSET;

	if (!qctx->detach_client) {
		return (NS_HOOK_CONTINUE);
	}

	client_state_destroy(qctx, inst);

	return (NS_HOOK_CONTINUE);
}
