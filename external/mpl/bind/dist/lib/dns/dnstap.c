/*	$NetBSD: dnstap.c,v 1.1.1.1 2018/08/12 12:08:09 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright (c) 2013-2014, Farsight Security, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*! \file */

#include <config.h>

#ifndef HAVE_DNSTAP
#error DNSTAP not configured.
#endif /* HAVE_DNSTAP */

#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/print.h>
#include <isc/sockaddr.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/dnstap.h>
#include <dns/events.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdataset.h>
#include <dns/result.h>
#include <dns/stats.h>
#include <dns/types.h>
#include <dns/view.h>

#include <dns/dnstap.pb-c.h>
#include <protobuf-c/protobuf-c.h>

#define DTENV_MAGIC			ISC_MAGIC('D', 't', 'n', 'v')
#define VALID_DTENV(env)		ISC_MAGIC_VALID(env, DTENV_MAGIC)

#define DNSTAP_CONTENT_TYPE	"protobuf:dnstap.Dnstap"
#define DNSTAP_INITIAL_BUF_SIZE 256

struct dns_dtmsg {
	void *buf;
	size_t len;
	Dnstap__Dnstap d;
	Dnstap__Message m;
};

struct dns_dthandle {
	dns_dtmode_t mode;
	struct fstrm_reader *reader;
	isc_mem_t *mctx;
};

struct dns_dtenv {
	unsigned int magic;
	isc_refcount_t refcount;

	isc_mem_t *mctx;

	struct fstrm_iothr *iothr;
	struct fstrm_iothr_options *fopt;

	isc_task_t *reopen_task;
	isc_mutex_t reopen_lock;	/* locks 'reopen_queued' */
	isc_boolean_t reopen_queued;

	isc_region_t identity;
	isc_region_t version;
	char *path;
	dns_dtmode_t mode;
	isc_offset_t max_size;
	int rolls;
	isc_log_rollsuffix_t suffix;
	isc_stats_t *stats;
};

#define CHECK(x) do { \
	result = (x); \
	if (result != ISC_R_SUCCESS) \
		goto cleanup; \
	} while (0)

static isc_mutex_t dt_mutex;
static isc_boolean_t dt_initialized = ISC_FALSE;
static isc_thread_key_t dt_key;
static isc_once_t mutex_once = ISC_ONCE_INIT;
static isc_mem_t *dt_mctx = NULL;

/*
 * Change under task exclusive.
 */
static unsigned int generation;

static void
mutex_init(void) {
	RUNTIME_CHECK(isc_mutex_init(&dt_mutex) == ISC_R_SUCCESS);
}

static void
dtfree(void *arg) {
	free(arg);
	isc_thread_key_setspecific(dt_key, NULL);
}

static isc_result_t
dt_init(void) {
	isc_result_t result;

	result = isc_once_do(&mutex_once, mutex_init);
	if (result != ISC_R_SUCCESS)
		return (result);

	if (dt_initialized)
		return (ISC_R_SUCCESS);

	LOCK(&dt_mutex);
	if (!dt_initialized) {
		int ret;

		if (dt_mctx == NULL)
			result = isc_mem_create2(0, 0, &dt_mctx, 0);
		if (result != ISC_R_SUCCESS)
			goto unlock;
		isc_mem_setname(dt_mctx, "dt", NULL);
		isc_mem_setdestroycheck(dt_mctx, ISC_FALSE);

		ret = isc_thread_key_create(&dt_key, dtfree);
		if (ret == 0)
			dt_initialized = ISC_TRUE;
		else
			result = ISC_R_FAILURE;
	}
unlock:
	UNLOCK(&dt_mutex);

	return (result);
}

isc_result_t
dns_dt_create(isc_mem_t *mctx, dns_dtmode_t mode, const char *path,
	      struct fstrm_iothr_options **foptp, dns_dtenv_t **envp)
{
	return (dns_dt_create2(mctx, mode, path, foptp, NULL, envp));
}

isc_result_t
dns_dt_create2(isc_mem_t *mctx, dns_dtmode_t mode, const char *path,
	       struct fstrm_iothr_options **foptp, isc_task_t *reopen_task,
	       dns_dtenv_t **envp)
{
	isc_result_t result = ISC_R_SUCCESS;
	fstrm_res res;
	struct fstrm_unix_writer_options *fuwopt = NULL;
	struct fstrm_file_options *ffwopt = NULL;
	struct fstrm_writer_options *fwopt = NULL;
	struct fstrm_writer *fw = NULL;
	dns_dtenv_t *env = NULL;

	REQUIRE(path != NULL);
	REQUIRE(envp != NULL && *envp == NULL);
	REQUIRE(foptp != NULL && *foptp != NULL);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSTAP,
		      DNS_LOGMODULE_DNSTAP, ISC_LOG_INFO,
		      "opening dnstap destination '%s'", path);

	generation++;

	env = isc_mem_get(mctx, sizeof(dns_dtenv_t));
	if (env == NULL)
		CHECK(ISC_R_NOMEMORY);

	memset(env, 0, sizeof(dns_dtenv_t));

	CHECK(isc_refcount_init(&env->refcount, 1));
	CHECK(isc_stats_create(mctx, &env->stats, dns_dnstapcounter_max));
	env->path = isc_mem_strdup(mctx, path);
	if (env->path == NULL)
		CHECK(ISC_R_NOMEMORY);

	fwopt = fstrm_writer_options_init();
	if (fwopt == NULL)
		CHECK(ISC_R_NOMEMORY);

	res = fstrm_writer_options_add_content_type(fwopt,
					    DNSTAP_CONTENT_TYPE,
					    sizeof(DNSTAP_CONTENT_TYPE) - 1);
	if (res != fstrm_res_success)
		CHECK(ISC_R_FAILURE);

	if (mode == dns_dtmode_file) {
		ffwopt = fstrm_file_options_init();
		if (ffwopt != NULL) {
			fstrm_file_options_set_file_path(ffwopt, env->path);
			fw = fstrm_file_writer_init(ffwopt, fwopt);
		}
	} else if (mode == dns_dtmode_unix) {
		fuwopt = fstrm_unix_writer_options_init();
		if (fuwopt != NULL) {
			fstrm_unix_writer_options_set_socket_path(fuwopt,
								  env->path);
			fw = fstrm_unix_writer_init(fuwopt, fwopt);
		}
	} else
		CHECK(ISC_R_FAILURE);

	if (fw == NULL)
		CHECK(ISC_R_FAILURE);

	env->iothr = fstrm_iothr_init(*foptp, &fw);
	if (env->iothr == NULL) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSTAP,
			      DNS_LOGMODULE_DNSTAP, ISC_LOG_WARNING,
			      "unable to initialize dnstap I/O thread");
		fstrm_writer_destroy(&fw);
		CHECK(ISC_R_FAILURE);
	}
	env->mode = mode;
	env->max_size = 0;
	env->rolls = ISC_LOG_ROLLINFINITE;
	env->fopt = *foptp;
	*foptp = NULL;

	env->reopen_task = reopen_task;
	isc_mutex_init(&env->reopen_lock);
	env->reopen_queued = ISC_FALSE;

	isc_mem_attach(mctx, &env->mctx);

	env->magic = DTENV_MAGIC;
	*envp = env;

 cleanup:
	if (ffwopt != NULL)
		fstrm_file_options_destroy(&ffwopt);

	if (fuwopt != NULL)
		fstrm_unix_writer_options_destroy(&fuwopt);

	if (fwopt != NULL)
		fstrm_writer_options_destroy(&fwopt);

	if (result != ISC_R_SUCCESS) {
		if (env != NULL) {
			isc_mutex_destroy(&env->reopen_lock);
			if (env->mctx != NULL)
				isc_mem_detach(&env->mctx);
			if (env->path != NULL)
				isc_mem_free(mctx, env->path);
			if (env->stats != NULL)
				isc_stats_detach(&env->stats);
			isc_mem_put(mctx, env, sizeof(dns_dtenv_t));
		}
	}

	return (result);
}

isc_result_t
dns_dt_setupfile(dns_dtenv_t *env, isc_uint64_t max_size, int rolls,
		 isc_log_rollsuffix_t suffix)
{
	REQUIRE(VALID_DTENV(env));

	/*
	 * If we're using unix domain socket mode, then any
	 * change from the default values is invalid.
	 */
	if (env->mode == dns_dtmode_unix) {
		if (max_size == 0 && rolls == ISC_LOG_ROLLINFINITE &&
		    suffix == isc_log_rollsuffix_increment)
		{
			return (ISC_R_SUCCESS);
		} else {
			return (ISC_R_INVALIDFILE);
		}
	}

	env->max_size = max_size;
	env->rolls = rolls;
	env->suffix = suffix;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_dt_reopen(dns_dtenv_t *env, int roll) {
	isc_result_t result = ISC_R_SUCCESS;
	fstrm_res res;
	isc_logfile_t file;
	struct fstrm_unix_writer_options *fuwopt = NULL;
	struct fstrm_file_options *ffwopt = NULL;
	struct fstrm_writer_options *fwopt = NULL;
	struct fstrm_writer *fw = NULL;

	REQUIRE(VALID_DTENV(env));

	/*
	 * Run in task-exclusive mode.
	 */
	result = isc_task_beginexclusive(env->reopen_task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	/*
	 * Check that we can create a new fw object.
	 */
	fwopt = fstrm_writer_options_init();
	if (fwopt == NULL) {
		isc_task_endexclusive(env->reopen_task);
		return (ISC_R_NOMEMORY);
	}

	res = fstrm_writer_options_add_content_type(fwopt,
					    DNSTAP_CONTENT_TYPE,
					    sizeof(DNSTAP_CONTENT_TYPE) - 1);
	if (res != fstrm_res_success) {
		CHECK(ISC_R_FAILURE);
	}

	if (env->mode == dns_dtmode_file) {
		ffwopt = fstrm_file_options_init();
		if (ffwopt != NULL) {
			fstrm_file_options_set_file_path(ffwopt, env->path);
			fw = fstrm_file_writer_init(ffwopt, fwopt);
		}
	} else if (env->mode == dns_dtmode_unix) {
		fuwopt = fstrm_unix_writer_options_init();
		if (fuwopt != NULL) {
			fstrm_unix_writer_options_set_socket_path(fuwopt,
								  env->path);
			fw = fstrm_unix_writer_init(fuwopt, fwopt);
		}
	} else {
		CHECK(ISC_R_NOTIMPLEMENTED);
	}

	if (fw == NULL) {
		CHECK(ISC_R_FAILURE);
	}

	/*
	 * We are committed here.
	 */
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSTAP,
		      DNS_LOGMODULE_DNSTAP, ISC_LOG_INFO,
		      "%s dnstap destination '%s'",
		      (roll < 0) ? "reopening" : "rolling",
		      env->path);

	generation++;

	if (env->iothr != NULL) {
		fstrm_iothr_destroy(&env->iothr);
	}

	if (roll != 0) {
		roll = env->rolls;
	}

	if (env->mode == dns_dtmode_file && roll != 0) {
		/*
		 * Create a temporary isc_logfile_t structure so we can
		 * take advantage of the logfile rolling facility.
		 */
		char *filename = isc_mem_strdup(env->mctx, env->path);
		file.name = filename;
		file.stream = NULL;
		file.versions = roll != 0 ? roll : env->rolls;
		file.maximum_size = 0;
		file.maximum_reached = ISC_FALSE;
		file.suffix = env->suffix;
		result = isc_logfile_roll(&file);
		isc_mem_free(env->mctx, filename);
		CHECK(result);
	}

	env->iothr = fstrm_iothr_init(env->fopt, &fw);
	if (env->iothr == NULL) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSTAP,
			      DNS_LOGMODULE_DNSTAP, ISC_LOG_WARNING,
			      "unable to initialize dnstap I/O thread");
		CHECK(ISC_R_FAILURE);
	}

 cleanup:
	if (ffwopt != NULL)
		fstrm_file_options_destroy(&ffwopt);

	if (fw != NULL)
		fstrm_writer_destroy(&fw);

	if (fwopt != NULL)
		fstrm_writer_options_destroy(&fwopt);

	if (fuwopt != NULL)
		fstrm_unix_writer_options_destroy(&fuwopt);

	isc_task_endexclusive(env->reopen_task);

	return (result);
}

static isc_result_t
toregion(dns_dtenv_t *env, isc_region_t *r, const char *str) {
	unsigned char *p = NULL;

	REQUIRE(r != NULL);

	if (str != NULL) {
		p = (unsigned char *) isc_mem_strdup(env->mctx, str);
		if (p == NULL)
			return (ISC_R_NOMEMORY);
	}

	if (r->base != NULL) {
		isc_mem_free(env->mctx, r->base);
		r->length = 0;
	}

	if (p != NULL) {
		r->base = p;
		r->length = strlen((char *) p);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_dt_setidentity(dns_dtenv_t *env, const char *identity) {
	REQUIRE(VALID_DTENV(env));

	return (toregion(env, &env->identity, identity));
}

isc_result_t
dns_dt_setversion(dns_dtenv_t *env, const char *version) {
	REQUIRE(VALID_DTENV(env));

	return (toregion(env, &env->version, version));
}

static struct fstrm_iothr_queue *
dt_queue(dns_dtenv_t *env) {
	isc_result_t result;
	struct ioq {
		unsigned int generation;
		struct fstrm_iothr_queue *ioq;
	} *ioq;

	REQUIRE(VALID_DTENV(env));

	if (env->iothr == NULL)
		return (NULL);

	result = dt_init();
	if (result != ISC_R_SUCCESS)
		return (NULL);

	ioq = (struct ioq *)isc_thread_key_getspecific(dt_key);
	if (ioq != NULL && ioq->generation != generation) {
		result = isc_thread_key_setspecific(dt_key, NULL);
		if (result != ISC_R_SUCCESS)
			return (NULL);
		free(ioq);
		ioq = NULL;
	}
	if (ioq == NULL) {
		ioq = malloc(sizeof(*ioq));
		if (ioq == NULL)
			return (NULL);
		ioq->generation = generation;
		ioq->ioq = fstrm_iothr_get_input_queue(env->iothr);
		if (ioq->ioq == NULL) {
			free(ioq);
			return (NULL);
		}
		result = isc_thread_key_setspecific(dt_key, ioq);
		if (result != ISC_R_SUCCESS) {
			free(ioq);
			return (NULL);
		}
	}

	return (ioq->ioq);
}

void
dns_dt_attach(dns_dtenv_t *source, dns_dtenv_t **destp) {
	REQUIRE(VALID_DTENV(source));
	REQUIRE(destp != NULL && *destp == NULL);

	isc_refcount_increment(&source->refcount, NULL);
	*destp = source;
}

isc_result_t
dns_dt_getstats(dns_dtenv_t *env, isc_stats_t **statsp) {
	REQUIRE(VALID_DTENV(env));
	REQUIRE(statsp != NULL && *statsp == NULL);

	if (env->stats == NULL)
		return (ISC_R_NOTFOUND);
	isc_stats_attach(env->stats, statsp);
	return (ISC_R_SUCCESS);
}

static void
destroy(dns_dtenv_t *env) {

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSTAP,
		      DNS_LOGMODULE_DNSTAP, ISC_LOG_INFO,
		      "closing dnstap");
	env->magic = 0;

	generation++;

	if (env->iothr != NULL)
		fstrm_iothr_destroy(&env->iothr);
	if (env->fopt != NULL)
		fstrm_iothr_options_destroy(&env->fopt);

	if (env->identity.base != NULL) {
		isc_mem_free(env->mctx, env->identity.base);
		env->identity.length = 0;
	}
	if (env->version.base != NULL) {
		isc_mem_free(env->mctx, env->version.base);
		env->version.length = 0;
	}
	if (env->path != NULL)
		isc_mem_free(env->mctx, env->path);
	if (env->stats != NULL)
		isc_stats_detach(&env->stats);

	isc_mem_putanddetach(&env->mctx, env, sizeof(*env));
}

void
dns_dt_detach(dns_dtenv_t **envp) {
	unsigned int refs;
	dns_dtenv_t *env;

	REQUIRE(envp != NULL && VALID_DTENV(*envp));

	env = *envp;
	*envp = NULL;

	isc_refcount_decrement(&env->refcount, &refs);
	if (refs == 0)
		destroy(env);
}

static isc_result_t
pack_dt(const Dnstap__Dnstap *d, void **buf, size_t *sz) {
	ProtobufCBufferSimple sbuf;

	REQUIRE(d != NULL);
	REQUIRE(sz != NULL);

	memset(&sbuf, 0, sizeof(sbuf));
	sbuf.base.append = protobuf_c_buffer_simple_append;
	sbuf.len = 0;
	sbuf.alloced = DNSTAP_INITIAL_BUF_SIZE;

	/* Need to use malloc() here because protobuf uses free() */
	sbuf.data = malloc(sbuf.alloced);
	if (sbuf.data == NULL)
		return (ISC_R_NOMEMORY);
	sbuf.must_free_data = 1;

	*sz = dnstap__dnstap__pack_to_buffer(d, (ProtobufCBuffer *) &sbuf);
	if (sbuf.data == NULL)
		return (ISC_R_FAILURE);
	*buf = sbuf.data;

	return (ISC_R_SUCCESS);
}

static void
send_dt(dns_dtenv_t *env, void *buf, size_t len) {
	struct fstrm_iothr_queue *ioq;
	fstrm_res res;

	REQUIRE(env != NULL);

	if (buf == NULL)
		return;

	ioq = dt_queue(env);
	if (ioq == NULL) {
		free(buf);
		return;
	}

	res = fstrm_iothr_submit(env->iothr, ioq, buf, len,
				 fstrm_free_wrapper, NULL);
	if (res != fstrm_res_success) {
		if (env->stats != NULL)
			isc_stats_increment(env->stats,
					    dns_dnstapcounter_drop);
		free(buf);
	} else {
		if (env->stats != NULL)
			isc_stats_increment(env->stats,
					    dns_dnstapcounter_success);
	}
}

static void
init_msg(dns_dtenv_t *env, dns_dtmsg_t *dm, Dnstap__Message__Type mtype) {
	memset(dm, 0, sizeof(*dm));
	dm->d.base.descriptor = &dnstap__dnstap__descriptor;
	dm->m.base.descriptor = &dnstap__message__descriptor;
	dm->d.type = DNSTAP__DNSTAP__TYPE__MESSAGE;
	dm->d.message = &dm->m;
	dm->m.type = mtype;

	if (env->identity.length != 0) {
		dm->d.identity.data = env->identity.base;
		dm->d.identity.len = env->identity.length;
		dm->d.has_identity = ISC_TRUE;
	}

	if (env->version.length != 0) {
		dm->d.version.data = env->version.base;
		dm->d.version.len = env->version.length;
		dm->d.has_version = ISC_TRUE;
	}
}

static Dnstap__Message__Type
dnstap_type(dns_dtmsgtype_t msgtype) {
	switch (msgtype) {
	case DNS_DTTYPE_SQ:
		return (DNSTAP__MESSAGE__TYPE__STUB_QUERY);
	case DNS_DTTYPE_SR:
		return (DNSTAP__MESSAGE__TYPE__STUB_RESPONSE);
	case DNS_DTTYPE_CQ:
		return (DNSTAP__MESSAGE__TYPE__CLIENT_QUERY);
	case DNS_DTTYPE_CR:
		return (DNSTAP__MESSAGE__TYPE__CLIENT_RESPONSE);
	case DNS_DTTYPE_AQ:
		return (DNSTAP__MESSAGE__TYPE__AUTH_QUERY);
	case DNS_DTTYPE_AR:
		return (DNSTAP__MESSAGE__TYPE__AUTH_RESPONSE);
	case DNS_DTTYPE_RQ:
		return (DNSTAP__MESSAGE__TYPE__RESOLVER_QUERY);
	case DNS_DTTYPE_RR:
		return (DNSTAP__MESSAGE__TYPE__RESOLVER_RESPONSE);
	case DNS_DTTYPE_FQ:
		return (DNSTAP__MESSAGE__TYPE__FORWARDER_QUERY);
	case DNS_DTTYPE_FR:
		return (DNSTAP__MESSAGE__TYPE__FORWARDER_RESPONSE);
	case DNS_DTTYPE_TQ:
		return (DNSTAP__MESSAGE__TYPE__TOOL_QUERY);
	case DNS_DTTYPE_TR:
		return (DNSTAP__MESSAGE__TYPE__TOOL_RESPONSE);
	default:
		INSIST(0);
	}
}

static void
cpbuf(isc_buffer_t *buf, ProtobufCBinaryData *p, protobuf_c_boolean *has) {
	p->data = isc_buffer_base(buf);
	p->len = isc_buffer_usedlength(buf);
	*has = 1;
}

static void
setaddr(dns_dtmsg_t *dm, isc_sockaddr_t *sa, isc_boolean_t tcp,
	ProtobufCBinaryData *addr, protobuf_c_boolean *has_addr,
	isc_uint32_t *port, protobuf_c_boolean *has_port)
{
	int family = isc_sockaddr_pf(sa);

	if (family != AF_INET6 && family != AF_INET)
		return;

	if (family == AF_INET6) {
		dm->m.socket_family = DNSTAP__SOCKET_FAMILY__INET6;
		addr->data = sa->type.sin6.sin6_addr.s6_addr;
		addr->len = 16;
		*port = ntohs(sa->type.sin6.sin6_port);
	} else {
		dm->m.socket_family = DNSTAP__SOCKET_FAMILY__INET;
		addr->data = (isc_uint8_t *) &sa->type.sin.sin_addr.s_addr;
		addr->len = 4;
		*port = ntohs(sa->type.sin.sin_port);
	}

	if (tcp)
		dm->m.socket_protocol = DNSTAP__SOCKET_PROTOCOL__TCP;
	else
		dm->m.socket_protocol = DNSTAP__SOCKET_PROTOCOL__UDP;

	dm->m.has_socket_protocol = 1;
	dm->m.has_socket_family = 1;
	*has_addr = 1;
	*has_port = 1;
}

/*%
 * Invoke dns_dt_reopen() and re-allow dnstap output file rolling.  This
 * function is run in the context of the task stored in the 'reopen_task' field
 * of the dnstap environment structure.
 */
static void
perform_reopen(isc_task_t *task, isc_event_t *event) {
	dns_dtenv_t *env;

	REQUIRE(event != NULL);
	REQUIRE(event->ev_type == DNS_EVENT_FREESTORAGE);

	env = (dns_dtenv_t *)event->ev_arg;

	REQUIRE(VALID_DTENV(env));
	REQUIRE(task == env->reopen_task);

	/*
	 * Roll output file in the context of env->reopen_task.
	 */
	dns_dt_reopen(env, env->rolls);

	/*
	 * Clean up.
	 */
	isc_event_free(&event);
	isc_task_detach(&task);

	/*
	 * Re-allow output file rolling.
	 */
	LOCK(&env->reopen_lock);
	env->reopen_queued = ISC_FALSE;
	UNLOCK(&env->reopen_lock);
}

/*%
 * Check whether a dnstap output file roll is due and if so, initiate it (the
 * actual roll happens asynchronously).
 */
static void
check_file_size_and_maybe_reopen(dns_dtenv_t *env) {
	isc_task_t *reopen_task = NULL;
	isc_event_t *event;
	struct stat statbuf;

	/*
	 * If the task from which the output file should be reopened was not
	 * specified, abort.
	 */
	if (env->reopen_task == NULL) {
		return;
	}

	/*
	 * If an output file roll is not currently queued, check the current
	 * size of the output file to see whether a roll is needed.  Return if
	 * it is not.
	 */
	LOCK(&env->reopen_lock);
	if (env->reopen_queued || stat(env->path, &statbuf) < 0 ||
	    statbuf.st_size <= env->max_size)
	{
		goto unlock_and_return;
	}

	/*
	 * We need to roll the output file, but it needs to be done in the
	 * context of env->reopen_task.  Allocate and send an event to achieve
	 * that, then disallow output file rolling until the roll we queue is
	 * completed.
	 */
	event = isc_event_allocate(env->mctx, NULL, DNS_EVENT_FREESTORAGE,
				   perform_reopen, env, sizeof(*event));
	if (event == NULL) {
		goto unlock_and_return;
	}
	isc_task_attach(env->reopen_task, &reopen_task);
	isc_task_send(reopen_task, &event);
	env->reopen_queued = ISC_TRUE;

 unlock_and_return:
	UNLOCK(&env->reopen_lock);
}

void
dns_dt_send(dns_view_t *view, dns_dtmsgtype_t msgtype,
	    isc_sockaddr_t *qaddr, isc_sockaddr_t *raddr,
	    isc_boolean_t tcp, isc_region_t *zone, isc_time_t *qtime,
	    isc_time_t *rtime, isc_buffer_t *buf)
{
	isc_time_t now, *t;
	dns_dtmsg_t dm;

	REQUIRE(DNS_VIEW_VALID(view));

	if ((msgtype & view->dttypes) == 0)
		return;

	if (view->dtenv == NULL)
		return;

	REQUIRE(VALID_DTENV(view->dtenv));

	if (view->dtenv->max_size != 0) {
		check_file_size_and_maybe_reopen(view->dtenv);
	}

	TIME_NOW(&now);
	t = &now;

	init_msg(view->dtenv, &dm, dnstap_type(msgtype));

	/* Query/response times */
	switch (msgtype) {
	case DNS_DTTYPE_AR:
	case DNS_DTTYPE_CR:
	case DNS_DTTYPE_RR:
	case DNS_DTTYPE_FR:
	case DNS_DTTYPE_SR:
	case DNS_DTTYPE_TR:
		if (rtime != NULL)
			t = rtime;

		dm.m.response_time_sec = isc_time_seconds(t);
		dm.m.has_response_time_sec = 1;
		dm.m.response_time_nsec = isc_time_nanoseconds(t);
		dm.m.has_response_time_nsec = 1;

		cpbuf(buf, &dm.m.response_message, &dm.m.has_response_message);

		/* Types RR and FR get both query and response times */
		if (msgtype == DNS_DTTYPE_CR || msgtype == DNS_DTTYPE_AR)
			break;

		/* FALLTHROUGH */
	case DNS_DTTYPE_AQ:
	case DNS_DTTYPE_CQ:
	case DNS_DTTYPE_FQ:
	case DNS_DTTYPE_RQ:
	case DNS_DTTYPE_SQ:
	case DNS_DTTYPE_TQ:
		if (qtime != NULL)
			t = qtime;

		dm.m.query_time_sec = isc_time_seconds(t);
		dm.m.has_query_time_sec = 1;
		dm.m.query_time_nsec = isc_time_nanoseconds(t);
		dm.m.has_query_time_nsec = 1;

		cpbuf(buf, &dm.m.query_message, &dm.m.has_query_message);
		break;
	default:
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSTAP,
			      DNS_LOGMODULE_DNSTAP, ISC_LOG_ERROR,
			      "invalid dnstap message type %d", msgtype);
		return;
	}

	/* Zone/bailiwick */
	switch (msgtype) {
	case DNS_DTTYPE_AR:
	case DNS_DTTYPE_RQ:
	case DNS_DTTYPE_RR:
	case DNS_DTTYPE_FQ:
	case DNS_DTTYPE_FR:
		if (zone != NULL && zone->base != NULL && zone->length != 0) {
			dm.m.query_zone.data = zone->base;
			dm.m.query_zone.len = zone->length;
			dm.m.has_query_zone = 1;
		}
		break;
	default:
		break;
	}

	if (qaddr != NULL) {
		setaddr(&dm, qaddr, tcp,
			&dm.m.query_address, &dm.m.has_query_address,
			&dm.m.query_port, &dm.m.has_query_port);
	}
	if (raddr != NULL) {
		setaddr(&dm, raddr, tcp,
			&dm.m.response_address, &dm.m.has_response_address,
			&dm.m.response_port, &dm.m.has_response_port);
	}

	if (pack_dt(&dm.d, &dm.buf, &dm.len) == ISC_R_SUCCESS)
		send_dt(view->dtenv, dm.buf, dm.len);
}

void
dns_dt_shutdown() {
	if (dt_mctx != NULL)
		isc_mem_detach(&dt_mctx);
}

static isc_result_t
putstr(isc_buffer_t **b, const char *str) {
	isc_result_t result;

	result = isc_buffer_reserve(b, strlen(str));
	if (result != ISC_R_SUCCESS)
		return (ISC_R_NOSPACE);

	isc_buffer_putstr(*b, str);
	return (ISC_R_SUCCESS);
}

static isc_result_t
putaddr(isc_buffer_t **b, isc_region_t *ip) {
	char buf[64];

	if (ip->length == 4) {
		if (!inet_ntop(AF_INET, ip->base, buf, sizeof(buf)))
			return (ISC_R_FAILURE);
	} else if (ip->length == 16) {
		if (!inet_ntop(AF_INET6, ip->base, buf, sizeof(buf)))
			return (ISC_R_FAILURE);
	} else
		return (ISC_R_BADADDRESSFORM);

	return (putstr(b, buf));
}

static isc_boolean_t
dnstap_file(struct fstrm_reader *r) {
	fstrm_res res;
	const struct fstrm_control *control = NULL;
	const isc_uint8_t *rtype = NULL;
	size_t dlen = strlen(DNSTAP_CONTENT_TYPE), rlen = 0;
	size_t n = 0;

	res = fstrm_reader_get_control(r, FSTRM_CONTROL_START, &control);
	if (res != fstrm_res_success)
		return (ISC_FALSE);

	res = fstrm_control_get_num_field_content_type(control, &n);
	if (res != fstrm_res_success)
		return (ISC_FALSE);
	if (n > 0) {
		res = fstrm_control_get_field_content_type(control, 0,
							   &rtype, &rlen);
		if (res != fstrm_res_success)
			return (ISC_FALSE);

		if (rlen != dlen)
			return (ISC_FALSE);

		if (memcmp(DNSTAP_CONTENT_TYPE, rtype, dlen) == 0)
			return (ISC_TRUE);
	}

	return (ISC_FALSE);
}

isc_result_t
dns_dt_open(const char *filename, dns_dtmode_t mode, isc_mem_t *mctx,
	    dns_dthandle_t **handlep)
{
	isc_result_t result;
	struct fstrm_file_options *fopt = NULL;
	fstrm_res res;
	dns_dthandle_t *handle;

	REQUIRE(handlep != NULL && *handlep == NULL);

	handle = isc_mem_get(mctx, sizeof(*handle));
	if (handle == NULL)
		CHECK(ISC_R_NOMEMORY);

	handle->mode = mode;
	handle->mctx = NULL;

	switch(mode) {
	case dns_dtmode_file:
		fopt = fstrm_file_options_init();
		if (fopt == NULL)
			CHECK(ISC_R_NOMEMORY);

		fstrm_file_options_set_file_path(fopt, filename);

		handle->reader = fstrm_file_reader_init(fopt, NULL);
		if (handle->reader == NULL)
			CHECK(ISC_R_NOMEMORY);

		res = fstrm_reader_open(handle->reader);
		if (res != fstrm_res_success)
			CHECK(ISC_R_FAILURE);

		if (!dnstap_file(handle->reader))
			CHECK(DNS_R_BADDNSTAP);
		break;
	case dns_dtmode_unix:
		return (ISC_R_NOTIMPLEMENTED);
	default:
		INSIST(0);
	}

	isc_mem_attach(mctx, &handle->mctx);
	result = ISC_R_SUCCESS;
	*handlep = handle;
	handle = NULL;

 cleanup:
	if (result != ISC_R_SUCCESS && handle->reader != NULL) {
		fstrm_reader_destroy(&handle->reader);
		handle->reader = NULL;
	}
	if (fopt != NULL)
		fstrm_file_options_destroy(&fopt);
	if (handle != NULL)
		isc_mem_put(mctx, handle, sizeof(*handle));
	return (result);
}

isc_result_t
dns_dt_getframe(dns_dthandle_t *handle, isc_uint8_t **bufp, size_t *sizep) {
	const isc_uint8_t *data;
	fstrm_res res;

	REQUIRE(handle != NULL);
	REQUIRE(bufp != NULL);
	REQUIRE(sizep != NULL);

	data = (const isc_uint8_t *) *bufp;

	res = fstrm_reader_read(handle->reader, &data, sizep);
	switch (res) {
	case fstrm_res_success:
		if (data == NULL)
			return (ISC_R_FAILURE);
		DE_CONST(data, *bufp);
		return (ISC_R_SUCCESS);
	case fstrm_res_stop:
		return (ISC_R_NOMORE);
	default:
		return (ISC_R_FAILURE);
	}
}

void
dns_dt_close(dns_dthandle_t **handlep) {
	dns_dthandle_t *handle;

	REQUIRE(handlep != NULL && *handlep != NULL);

	handle = *handlep;
	*handlep = NULL;

	if (handle->reader != NULL) {
		fstrm_reader_destroy(&handle->reader);
		handle->reader = NULL;
	}
	isc_mem_putanddetach(&handle->mctx, handle, sizeof(*handle));
}

isc_result_t
dns_dt_parse(isc_mem_t *mctx, isc_region_t *src, dns_dtdata_t **destp) {
	isc_result_t result;
	Dnstap__Message *m;
	dns_dtdata_t *d = NULL;
	isc_buffer_t b;

	REQUIRE(src != NULL);
	REQUIRE(destp != NULL && *destp == NULL);

	d = isc_mem_get(mctx, sizeof(*d));
	if (d == NULL)
		return (ISC_R_NOMEMORY);

	memset(d, 0, sizeof(*d));
	isc_mem_attach(mctx, &d->mctx);

	d->frame = dnstap__dnstap__unpack(NULL, src->length, src->base);
	if (d->frame == NULL)
		CHECK(ISC_R_NOMEMORY);

	if (d->frame->type != DNSTAP__DNSTAP__TYPE__MESSAGE)
		CHECK(DNS_R_BADDNSTAP);

	m = d->frame->message;

	/* Message type */
	switch (m->type) {
	case DNSTAP__MESSAGE__TYPE__AUTH_QUERY:
		d->type = DNS_DTTYPE_AQ;
		break;
	case DNSTAP__MESSAGE__TYPE__AUTH_RESPONSE:
		d->type = DNS_DTTYPE_AR;
		break;
	case DNSTAP__MESSAGE__TYPE__CLIENT_QUERY:
		d->type = DNS_DTTYPE_CQ;
		break;
	case DNSTAP__MESSAGE__TYPE__CLIENT_RESPONSE:
		d->type = DNS_DTTYPE_CR;
		break;
	case DNSTAP__MESSAGE__TYPE__FORWARDER_QUERY:
		d->type = DNS_DTTYPE_FQ;
		break;
	case DNSTAP__MESSAGE__TYPE__FORWARDER_RESPONSE:
		d->type = DNS_DTTYPE_FR;
		break;
	case DNSTAP__MESSAGE__TYPE__RESOLVER_QUERY:
		d->type = DNS_DTTYPE_RQ;
		break;
	case DNSTAP__MESSAGE__TYPE__RESOLVER_RESPONSE:
		d->type = DNS_DTTYPE_RR;
		break;
	case DNSTAP__MESSAGE__TYPE__STUB_QUERY:
		d->type = DNS_DTTYPE_SQ;
		break;
	case DNSTAP__MESSAGE__TYPE__STUB_RESPONSE:
		d->type = DNS_DTTYPE_SR;
		break;
	case DNSTAP__MESSAGE__TYPE__TOOL_QUERY:
		d->type = DNS_DTTYPE_TQ;
		break;
	case DNSTAP__MESSAGE__TYPE__TOOL_RESPONSE:
		d->type = DNS_DTTYPE_TR;
		break;
	default:
		CHECK(DNS_R_BADDNSTAP);
	}

	/* Query? */
	if ((d->type & DNS_DTTYPE_QUERY) != 0)
		d->query = ISC_TRUE;
	else
		d->query = ISC_FALSE;

	/* Parse DNS message */
	if (d->query && m->has_query_message) {
		d->msgdata.base = m->query_message.data;
		d->msgdata.length = m->query_message.len;
	} else if (!d->query && m->has_response_message) {
		d->msgdata.base = m->response_message.data;
		d->msgdata.length = m->response_message.len;
	}

	isc_buffer_init(&b, d->msgdata.base, d->msgdata.length);
	isc_buffer_add(&b, d->msgdata.length);
	CHECK(dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &d->msg));
	result = dns_message_parse(d->msg, &b, 0);
	if (result != ISC_R_SUCCESS) {
		if (result != DNS_R_RECOVERABLE)
			dns_message_destroy(&d->msg);
		result = ISC_R_SUCCESS;
	}

	/* Timestamp */
	if (d->query) {
		if (m->has_query_time_sec && m->has_query_time_nsec)
			isc_time_set(&d->qtime, m->query_time_sec,
				     m->query_time_nsec);
	} else {
		if (m->has_response_time_sec && m->has_response_time_nsec)
			isc_time_set(&d->rtime, m->response_time_sec,
				     m->response_time_nsec);
	}

	/* Peer address */
	if (m->has_query_address) {
		d->qaddr.base = m->query_address.data;
		d->qaddr.length = m->query_address.len;
	}
	if (m->has_query_port) {
		d->qport = m->query_port;
	}

	if (m->has_response_address) {
		d->raddr.base = m->response_address.data;
		d->raddr.length = m->response_address.len;
	}
	if (m->has_response_port) {
		d->rport = m->response_port;
	}

	/* Socket protocol */
	if (m->has_socket_protocol) {
		const ProtobufCEnumValue *type =
			protobuf_c_enum_descriptor_get_value(
				&dnstap__socket_protocol__descriptor,
				m->socket_protocol);
		if (type != NULL &&
		    type->value == DNSTAP__SOCKET_PROTOCOL__TCP)
			d->tcp = ISC_TRUE;
		else
			d->tcp = ISC_FALSE;
	}

	/* Query tuple */
	if (d->msg != NULL) {
		dns_name_t *name = NULL;
		dns_rdataset_t *rdataset;

		CHECK(dns_message_firstname(d->msg, DNS_SECTION_QUESTION));
		dns_message_currentname(d->msg, DNS_SECTION_QUESTION, &name);
		rdataset = ISC_LIST_HEAD(name->list);

		dns_name_format(name, d->namebuf, sizeof(d->namebuf));
		dns_rdatatype_format(rdataset->type, d->typebuf,
				     sizeof(d->typebuf));
		dns_rdataclass_format(rdataset->rdclass, d->classbuf,
				      sizeof(d->classbuf));
	}

	*destp = d;

 cleanup:
	if (result != ISC_R_SUCCESS)
		dns_dtdata_free(&d);

	return (result);
}

isc_result_t
dns_dt_datatotext(dns_dtdata_t *d, isc_buffer_t **dest) {
	isc_result_t result;
	char buf[100];

	REQUIRE(d != NULL);
	REQUIRE(dest != NULL && *dest != NULL);

	memset(buf, 0, sizeof(buf));

	/* Timestamp */
	if (d->query && !isc_time_isepoch(&d->qtime))
		isc_time_formattimestamp(&d->qtime, buf, sizeof(buf));
	else if (!d->query && !isc_time_isepoch(&d->rtime))
		isc_time_formattimestamp(&d->rtime, buf, sizeof(buf));

	if (buf[0] == '\0')
		CHECK(putstr(dest, "???\?-?\?-?? ??:??:??.??? "));
	else {
		CHECK(putstr(dest, buf));
		CHECK(putstr(dest, " "));
	}

	/* Type mnemonic */
	switch (d->type) {
	case DNS_DTTYPE_AQ:
		CHECK(putstr(dest, "AQ "));
		break;
	case DNS_DTTYPE_AR:
		CHECK(putstr(dest, "AR "));
		break;
	case DNS_DTTYPE_CQ:
		CHECK(putstr(dest, "CQ "));
		break;
	case DNS_DTTYPE_CR:
		CHECK(putstr(dest, "CR "));
		break;
	case DNS_DTTYPE_FQ:
		CHECK(putstr(dest, "FQ "));
		break;
	case DNS_DTTYPE_FR:
		CHECK(putstr(dest, "FR "));
		break;
	case DNS_DTTYPE_RQ:
		CHECK(putstr(dest, "RQ "));
		break;
	case DNS_DTTYPE_RR:
		CHECK(putstr(dest, "RR "));
		break;
	case DNS_DTTYPE_SQ:
		CHECK(putstr(dest, "SQ "));
		break;
	case DNS_DTTYPE_SR:
		CHECK(putstr(dest, "SR "));
		break;
	case DNS_DTTYPE_TQ:
		CHECK(putstr(dest, "TQ "));
		break;
	case DNS_DTTYPE_TR:
		CHECK(putstr(dest, "TR "));
		break;
	default:
		return (DNS_R_BADDNSTAP);
	}

	/* Query and response addresses */
	if (d->qaddr.length != 0) {
		CHECK(putaddr(dest, &d->qaddr));
		snprintf(buf, sizeof(buf), ":%u", d->qport);
		CHECK(putstr(dest, buf));
	} else {
		CHECK(putstr(dest, "?"));
	}
	if ((d->type & DNS_DTTYPE_QUERY) != 0) {
		CHECK(putstr(dest, " -> "));
	} else {
		CHECK(putstr(dest, " <- "));
	}
	if (d->raddr.length != 0) {
		CHECK(putaddr(dest, &d->raddr));
		snprintf(buf, sizeof(buf), ":%u", d->rport);
		CHECK(putstr(dest, buf));
	} else {
		CHECK(putstr(dest, "?"));
	}

	CHECK(putstr(dest, " "));

	/* Protocol */
	if (d->tcp)
		CHECK(putstr(dest, "TCP "));
	else
		CHECK(putstr(dest, "UDP "));

	/* Message size */
	if (d->msgdata.base != NULL) {
		snprintf(buf, sizeof(buf), "%zub ", (size_t) d->msgdata.length);
		CHECK(putstr(dest, buf));
	} else
		CHECK(putstr(dest, "0b "));

	/* Query tuple */
	if (d->namebuf[0] == '\0')
		CHECK(putstr(dest, "?/"));
	else {
		CHECK(putstr(dest, d->namebuf));
		CHECK(putstr(dest, "/"));
	}

	if (d->classbuf[0] == '\0')
		CHECK(putstr(dest, "?/"));
	else {
		CHECK(putstr(dest, d->classbuf));
		CHECK(putstr(dest, "/"));
	}

	if (d->typebuf[0] == '\0')
		CHECK(putstr(dest, "?"));
	else
		CHECK(putstr(dest, d->typebuf));

	CHECK(isc_buffer_reserve(dest, 1));
	isc_buffer_putuint8(*dest, 0);

 cleanup:
	return (result);
}

void
dns_dtdata_free(dns_dtdata_t **dp) {
	dns_dtdata_t *d;

	REQUIRE(dp != NULL && *dp != NULL);

	d = *dp;

	if (d->msg != NULL)
		dns_message_destroy(&d->msg);
	if (d->frame != NULL)
		dnstap__dnstap__free_unpacked(d->frame, NULL);

	isc_mem_putanddetach(&d->mctx, d, sizeof(*d));

	*dp = NULL;
}
