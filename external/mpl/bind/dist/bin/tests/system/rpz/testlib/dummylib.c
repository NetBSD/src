/*	$NetBSD: dummylib.c,v 1.2 2025/01/26 16:25:00 christos Exp $	*/

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

/*
 * Limited implementation of the DNSRPS API for testing purposes.
 *
 * Copyright (c) 2016-2017 Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <isc/endian.h>
#include <isc/util.h>

#include <dns/librpz.h>

#include "test-data.h"
#include "trpz.h"

librpz_log_fnc_t *g_log_fnc = NULL;
const char *g_prog_nm = NULL;
bool g_scan_data_file_for_errors = true;

typedef struct {
	void *mutex_ctx;
	void *log_ctx;
	librpz_mutex_t *mutex_lock_fn;
	librpz_mutex_t *mutex_unlock_fn;
	librpz_mutex_t *mutex_destroy_fn;
} trpz_clist_t;

typedef struct {
	char *cstr;
	bool uses_expired;
	trpz_clist_t *pclist;
} trpz_client_t;

typedef struct {
	size_t idx; /* value only used for node iteration */
	trpz_client_t *client;
	bool have_rd;
	char zone[256];
	char domain[256];
	size_t zidx;
	trpz_result_t rstack[LIBRPZ_RSP_STACK_DEPTH];
	size_t stack_idx;
	trpz_zone_t *all_zones;
	trpz_result_t *all_nodes;
	size_t num_zones, num_nodes;
	ssize_t last_zone;
	ssize_t *base_zones;
	size_t nbase_zones;
} trpz_rsp_t;

librpz_log_level_t g_log_level = LIBRPZ_LOG_TRACE2;
FILE *g_log_outf = NULL;

static int
apply_all_updates(trpz_rsp_t *trsp);
static void
clear_all_updates(trpz_rsp_t *trsp);

static bool
domain_ntop(const u_char *src, char *dst, size_t dstsiz);
static bool
domain_pton2(const char *src, u_char *dst, size_t dstsiz, size_t *dstlen,
	     bool lower);

void
trpz_set_log(librpz_log_fnc_t *new_log, const char *prog_nm);
void
trpz_vlog(librpz_log_level_t level, void *ctx, const char *p, va_list args)
	LIBRPZ_PF(3, 0);
void
trpz_log(librpz_log_level_t level, void *ctx, const char *p, ...)
	LIBRPZ_PF(3, 4);
librpz_log_level_t
trpz_log_level_val(librpz_log_level_t level);
void
trpz_vpemsg(librpz_emsg_t *emsg, const char *p, va_list args) LIBRPZ_PF(2, 0);
void
trpz_pemsg(librpz_emsg_t *emsg, const char *fmt, ...) LIBRPZ_PF(2, 3);
librpz_clist_t *
trpz_clist_create(librpz_emsg_t *emsg, librpz_mutex_t *lock,
		  librpz_mutex_t *unlock, librpz_mutex_t *mutex_destroy,
		  void *mutex_ctx, void *log_ctx);
void
trpz_clist_detach(librpz_clist_t **clistp);
bool
trpz_connect(librpz_emsg_t *emsg, librpz_client_t *client, bool optional);
librpz_client_t *
trpz_client_create(librpz_emsg_t *emsg, librpz_clist_t *clist, const char *cstr,
		   bool use_expired);
void
trpz_client_detach(librpz_client_t **clientp);
bool
trpz_rsp_create(librpz_emsg_t *emsg, librpz_rsp_t **rspp, int *min_ns_dotsp,
		librpz_client_t *client, bool have_rd, bool have_do);
void
trpz_rsp_detach(librpz_rsp_t **rspp);
bool
trpz_rsp_push(librpz_emsg_t *emsg, librpz_rsp_t *rsp);
bool
trpz_rsp_pop(librpz_emsg_t *emsg, librpz_result_t *result, librpz_rsp_t *rsp);
bool
trpz_rsp_pop_discard(librpz_emsg_t *emsg, librpz_rsp_t *rsp);
bool
trpz_rsp_domain(librpz_emsg_t *emsg, librpz_domain_buf_t *owner,
		librpz_rsp_t *rsp);
bool
trpz_rsp_result(librpz_emsg_t *emsg, librpz_result_t *result, bool recursed,
		const librpz_rsp_t *rsp);
bool
trpz_rsp_soa(librpz_emsg_t *emsg, uint32_t *ttlp, librpz_rr_t **rrp,
	     librpz_domain_buf_t *origin, librpz_result_t *result,
	     librpz_rsp_t *rsp);
bool
trpz_rsp_rr(librpz_emsg_t *emsg, uint16_t *typep, uint16_t *classp,
	    uint32_t *ttlp, librpz_rr_t **rrp, librpz_result_t *result,
	    const uint8_t *qname, size_t qname_size, librpz_rsp_t *rsp);
bool
trpz_ck_domain(librpz_emsg_t *emsg, const uint8_t *domain, size_t domain_size,
	       librpz_trig_t trig, librpz_result_id_t hit_id, bool recursed,
	       librpz_rsp_t *rsp);
bool
trpz_ck_ip(librpz_emsg_t *emsg, const void *addr, uint family,
	   librpz_trig_t trig, librpz_result_id_t hit_id, bool recursed,
	   librpz_rsp_t *rsp);
bool
trpz_rsp_clientip_prefix(librpz_emsg_t *emsg, librpz_prefix_t *prefix,
			 librpz_rsp_t *rsp);
bool
trpz_have_trig(librpz_trig_t trig, bool ipv6, const librpz_rsp_t *rsp);
bool
trpz_rsp_forget_zone(librpz_emsg_t *emsg, librpz_cznum_t znum,
		     librpz_rsp_t *rsp);
char *
trpz_vers_stats(librpz_emsg_t *emsg, librpz_rsp_t *rsp);
bool
trpz_soa_serial(librpz_emsg_t *emsg, uint32_t *serialp, const char *domain_nm,
		librpz_rsp_t *rsp);
const char *
trpz_policy2str(librpz_policy_t policy, char *buf, size_t buf_size);

#define BASE_ZONE_ANY	  -1
#define BASE_ZONE_INVALID -2

librpz_0_t LIBRPZ_DEF = {
	.dnsrpzd_path = "test-only",
	.version = "0.0",
	.log_level_val = trpz_log_level_val,
	.set_log = trpz_set_log,
	.vpemsg = trpz_vpemsg,
	.pemsg = trpz_pemsg,
	.vlog = trpz_vlog,
	.log = trpz_log,
	.clist_create = trpz_clist_create,
	.clist_detach = trpz_clist_detach,
	.client_create = trpz_client_create,
	.connect = trpz_connect,
	.client_detach = trpz_client_detach,
	.rsp_create = trpz_rsp_create,
	.rsp_detach = trpz_rsp_detach,
	.rsp_result = trpz_rsp_result,
	.have_trig = trpz_have_trig,
	.rsp_domain = trpz_rsp_domain,
	.rsp_rr = trpz_rsp_rr,
	.rsp_soa = trpz_rsp_soa,
	.soa_serial = trpz_soa_serial,
	.rsp_push = trpz_rsp_push,
	.rsp_pop = trpz_rsp_pop,
	.rsp_pop_discard = trpz_rsp_pop_discard,
	.rsp_forget_zone = trpz_rsp_forget_zone,
	.ck_ip = trpz_ck_ip,
	.ck_domain = trpz_ck_domain,
	.policy2str = trpz_policy2str,
};

/*
 * Returns whether or not searching in the specified zone, by index, is
 * permitted. A client/RSP state can support a variable number of configured
 * zones.
 */
static bool
has_base_zone(trpz_rsp_t *trsp, ssize_t zone) {
	size_t n;

	if (trsp == NULL || trsp->base_zones == NULL || trsp->nbase_zones == 0)
	{
		return false;
	}

	for (n = 0; n < trsp->nbase_zones; n++) {
		if (trsp->base_zones[n] == BASE_ZONE_ANY ||
		    trsp->base_zones[n] == zone)
		{
			return true;
		}
	}

	return false;
}

static bool
pack_soa_record(unsigned char *rdatap, size_t rbufsz, size_t *rdlenp,
		const rpz_soa_t *psoa) {
	size_t needed = (sizeof(uint32_t) * 5) + strlen(psoa->mname) + 2 +
			strlen(psoa->rname) + 2;
	size_t mlen = 0, rlen = 0, used = 0;

	if (needed > rbufsz) {
		return false;
	}

	if (!domain_pton2(psoa->mname, rdatap, rbufsz, &rlen, true)) {
		return false;
	}

	if (!domain_pton2(psoa->rname, rdatap + rlen, rbufsz - rlen, &mlen,
			  true))
	{
		return false;
	}

	used = rlen + mlen;

	rdatap += rlen + mlen;
	ISC_U32TO8_BE(rdatap, psoa->serial);
	rdatap += 4;
	ISC_U32TO8_BE(rdatap, psoa->refresh);
	rdatap += 4;
	ISC_U32TO8_BE(rdatap, psoa->retry);
	rdatap += 4;
	ISC_U32TO8_BE(rdatap, psoa->expire);
	rdatap += 4;
	ISC_U32TO8_BE(rdatap, psoa->minimum);
	used += (4 * 5);

	SET_IF_NOT_NULL(rdlenp, used);

	return true;
}

static void
do_log(librpz_log_level_t level, void *ctx, const char *fmt, va_list args)
	LIBRPZ_PF(3, 0);
static void
do_log(librpz_log_level_t level, void *ctx, const char *fmt, va_list args) {
	if (level > g_log_level) {
		return;
	}

	if (g_log_fnc != NULL) {
		char lbuf[8192] = { 0 };

		vsnprintf(lbuf, sizeof(lbuf) - 1, fmt, args);
		g_log_fnc(level, ctx, lbuf);
		return;
	}

	if (g_log_outf == NULL) {
		return;
	}

	vfprintf(g_log_outf, fmt, args);
	fprintf(g_log_outf, "\n");
	return;
}

void
trpz_vlog(librpz_log_level_t level, void *ctx, const char *p, va_list args) {
	do_log(level, ctx, p, args);
	return;
}

void
trpz_log(librpz_log_level_t level, void *ctx, const char *p, ...) {
	va_list ap;

	va_start(ap, p);
	trpz_vlog(level, ctx, p, ap);
	va_end(ap);

	return;
}

void
trpz_set_log(librpz_log_fnc_t *new_log, const char *prog_nm) {
	assert(new_log != NULL || prog_nm != NULL);

	if (new_log != NULL) {
		g_log_fnc = new_log;
	}

	if (prog_nm != NULL) {
		g_prog_nm = prog_nm;
	}
}

librpz_log_level_t
trpz_log_level_val(librpz_log_level_t level) {
	if (level >= LIBRPZ_LOG_INVALID) {
		return g_log_level;
	}

	g_log_level = (level < LIBRPZ_LOG_FATAL) ? LIBRPZ_LOG_FATAL : level;

	return g_log_level;
}

void
trpz_vpemsg(librpz_emsg_t *emsg, const char *p, va_list args) {
	if (emsg == NULL) {
		return;
	}

	vsnprintf(emsg->c, sizeof(emsg->c), p, args);
	emsg->c[sizeof(emsg->c) - 1] = 0;
	return;
}

void
trpz_pemsg(librpz_emsg_t *emsg, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	trpz_vpemsg(emsg, fmt, ap);
	va_end(ap);

	return;
}

/*
 * Scan the data file for errors and log anything found that's critically
 * wrong.
 */
static void
scan_data_file_for_errors(void *lctx) {
	char *updfile = NULL, *fname = NULL, *last = NULL;
	char *tmp = NULL;

	updfile = getenv("DNSRPS_TEST_UPDATE_FILE");
	if (updfile == NULL) {
		return;
	}

	tmp = strdup(updfile);
	if (tmp == NULL) {
		return;
	}

	fname = strtok_r(tmp, ":", &last);

	while (fname) {
		char *errp = NULL;
		int ret;

		ret = sanity_check_data_file(fname, &errp);

		if ((ret < 0) && errp) {
			trpz_log(LIBRPZ_LOG_ERROR, lctx, "%s", errp);
			free(errp);
		}

		fname = strtok_r(NULL, ":", &last);
	}
	free(tmp);

	return;
}

librpz_clist_t *
trpz_clist_create(librpz_emsg_t *emsg, librpz_mutex_t *lock,
		  librpz_mutex_t *unlock, librpz_mutex_t *mutex_destroy,
		  void *mutex_ctx, void *log_ctx) {
	trpz_clist_t *result = NULL;

	result = calloc(1, sizeof(*result));
	if (result == NULL) {
		trpz_pemsg(emsg, "calloc: %s", strerror(errno));
		return NULL;
	}

	result->mutex_ctx = mutex_ctx;
	result->log_ctx = log_ctx;
	result->mutex_lock_fn = lock;
	result->mutex_unlock_fn = unlock;
	result->mutex_destroy_fn = mutex_destroy;

	if (g_scan_data_file_for_errors) {
		scan_data_file_for_errors(log_ctx);
	}

	return (librpz_clist_t *)result;
}

void
trpz_clist_detach(librpz_clist_t **clistp) {
	if (clistp != NULL && *clistp != NULL) {
		librpz_clist_t *clist = *clistp;
		*clistp = NULL;
		free(clist);
	}

	return;
}

bool
trpz_connect(librpz_emsg_t *emsg, librpz_client_t *client, bool optional) {
	UNUSED(optional);

	if (client == NULL) {
		trpz_pemsg(emsg, "Can't connect to null client");
		return false;
	}

	return true;
}

const char *
trpz_policy2str(librpz_policy_t policy, char *buf, size_t buf_size) {
	const char *pname = NULL;

	if (buf == NULL || buf_size == 0) {
		return NULL;
	}

	switch (policy) {
	case LIBRPZ_POLICY_UNDEFINED:
		pname = "UNDEFINED";
		break;
	case LIBRPZ_POLICY_DELETED:
		pname = "DELETED";
		break;
	case LIBRPZ_POLICY_PASSTHRU:
		pname = "PASSTHRU";
		break;
	case LIBRPZ_POLICY_DROP:
		pname = "DROP";
		break;
	case LIBRPZ_POLICY_TCP_ONLY:
		pname = "TCP-ONLY";
		break;
	case LIBRPZ_POLICY_NXDOMAIN:
		pname = "NXDOMAIN";
		break;
	case LIBRPZ_POLICY_NODATA:
		pname = "NODATA";
		break;
	case LIBRPZ_POLICY_RECORD:
		pname = "RECORD";
		break;
	case LIBRPZ_POLICY_GIVEN:
		pname = "GIVEN";
		break;
	case LIBRPZ_POLICY_DISABLED:
		pname = "DISABLED";
		break;
	case LIBRPZ_POLICY_CNAME:
		pname = "CNAME";
		break;
	default:
		pname = "UNKNOWN";
		break;
	}

	strncpy(buf, pname, buf_size);
	buf[buf_size - 1] = 0;
	return buf;
}

/*
 * Get the entire set of zones configured by the config string,
 * and bind then to the specified RSP state.
 *
 * The array of active zone indices is returned by the function on success,
 * or NULL on failure. The total number of zones is stored in pnzones.
 */
static ssize_t *
get_cstr_zones(const char *cstr, trpz_rsp_t *trsp, size_t *pnzones) {
	char tmpc[8192] = { 0 };
	char *tptr = tmpc, *tok = NULL;
	size_t nzones = 0, cur_idx = 0;
	ssize_t *result = NULL;
	unsigned long zflags = 0;

	result = calloc(trsp->num_zones + 1, sizeof(*result));
	if (result == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	if (cstr == NULL) {
		result[0] = BASE_ZONE_ANY;
		*pnzones = 1;
		return result;
	}

	strncpy(tmpc, cstr, sizeof(tmpc) - 1);
	*pnzones = 0;

	while (tptr != NULL && *tptr != '\0') {
		tok = strsep(&tptr, ";\n");

		while (isspace((unsigned char)*tok)) {
			tok++;
		}

		if (strncasecmp(tok, "zone ", 5) == 0) {
			char zcmd[1024] = { 0 };
			char *qend = NULL;
			size_t zind = 0, old_zct = trsp->num_zones;
			unsigned long zopts = 0;

			tok += 5;

			while (isspace((unsigned char)*tok)) {
				tok++;
			}

			if (*tok == '"') {
				qend = strchr(++tok, '"');
				if (qend == NULL) {
					fprintf(stderr, "Error parsing cstr "
							"contents!\n");
					free(result);
					return NULL;
				}

				*qend++ = 0;

				if (tok[strlen(tok) - 1] == '.') {
					tok[strlen(tok) - 1] = 0;
				}

			} else {
				qend = tok;
			}

			while (*qend != '\0' && !isspace((unsigned char)*qend))
			{
				qend++;
			}

			if (*qend != '\0') {
				*qend++ = '\0';
				zopts = parse_zone_options(qend);
			}

			snprintf(zcmd, sizeof(zcmd) - 1, "zone %s 1", tok);

			if (apply_update(zcmd, &(trsp->all_nodes),
					 &(trsp->num_nodes), &(trsp->all_zones),
					 &(trsp->num_zones), 0, zopts,
					 NULL) < 0)
			{
				fprintf(stderr, "Internal error {%s}!\n", zcmd);
				free(result);
				return NULL;
			}

			if (trsp->num_zones > old_zct) {
				result = realloc(result,
						 ((trsp->num_zones + 1) *
						  sizeof(*result)));
				if (result == NULL) {
					perror("realloc");
					exit(EXIT_FAILURE);
				}
			}

			for (zind = 0; zind < trsp->num_zones; zind++) {
				if (!strcmp(trsp->all_zones[zind].name, tok)) {
					break;
				}
			}

			if (zind == trsp->num_zones) {
				free(result);
				return NULL;
			}

			result[cur_idx++] = zind;
			*pnzones = cur_idx;
			nzones++;
		} else {
			unsigned long flags;

			flags = parse_zone_options(tok);
			zflags |= (flags & (ZOPT_QNAME_AS_NS | ZOPT_IP_AS_NS |
					    ZOPT_RECURSIVE_ONLY |
					    ZOPT_NOT_RECURSIVE_ONLY |
					    ZOPT_NO_QNAME_WAIT_RECURSE |
					    ZOPT_NO_NSIP_WAIT_RECURSE));
		}

		tok = NULL;
	}

	if (nzones == 0) {
		free(result);
		return NULL;
	}

	if (zflags != 0) {
		size_t n;

		for (n = 0; n < trsp->num_zones; n++) {
			if (zflags & ZOPT_QNAME_AS_NS) {
				trsp->all_zones[n].qname_as_ns = true;
			}

			if (zflags & ZOPT_IP_AS_NS) {
				trsp->all_zones[n].ip_as_ns = true;
			}

			if (zflags & ZOPT_RECURSIVE_ONLY) {
				trsp->all_zones[n].not_recursive_only = false;
			} else if (zflags & ZOPT_NOT_RECURSIVE_ONLY) {
				trsp->all_zones[n].not_recursive_only = true;
			}

			if (zflags & ZOPT_NO_QNAME_WAIT_RECURSE) {
				trsp->all_zones[n].no_qname_wait_recurse = true;
			}

			if (zflags & ZOPT_NO_NSIP_WAIT_RECURSE) {
				trsp->all_zones[n].no_nsip_wait_recurse = true;
			}
		}
	}

	return result;
}

librpz_client_t *
trpz_client_create(librpz_emsg_t *emsg, librpz_clist_t *clist, const char *cstr,
		   bool use_expired) {
	trpz_client_t *result = NULL;

	if (clist == NULL) {
		trpz_pemsg(emsg, "clist was NULL\n");
		return NULL;
	}

	result = calloc(1, sizeof(*result));
	if (result == NULL) {
		trpz_pemsg(emsg, "calloc: %s", strerror(errno));
		return NULL;
	}

	result->cstr = strdup(cstr);
	if (result->cstr == NULL) {
		trpz_pemsg(emsg, "strdup: %s", strerror(errno));
		free(result);
		return NULL;
	}

	result->uses_expired = use_expired;
	result->pclist = (trpz_clist_t *)clist;

	return (librpz_client_t *)result;
}

void
trpz_client_detach(librpz_client_t **clientp) {
	if (clientp != NULL && *clientp != NULL) {
		trpz_client_t *client = (trpz_client_t *)(*clientp);
		if (client->cstr != NULL) {
			free(client->cstr);
		}
		free(client);
	}

	return;
}

/*
 * If the DNSRPS_TEST_UPDATE_FILE env variable is set,
 * load the current list of test nodes from the specified data file.
 *
 * Any existing nodes are first destroyed.
 */
static int
apply_all_updates(trpz_rsp_t *trsp) {
	char *updfile = NULL, *fname = NULL, *last = NULL;
	char *tmp = NULL;

	updfile = getenv("DNSRPS_TEST_UPDATE_FILE");
	if (updfile == NULL) {
		return 0;
	}

	tmp = strdup(updfile);
	if (tmp == NULL) {
		return -1;
	}

	fname = strtok_r(updfile, ":", &last);
	while (fname != NULL) {
		char *errp = NULL;
		int ret;

		ret = load_all_updates(fname, &trsp->all_nodes,
				       &trsp->num_nodes, &trsp->all_zones,
				       &trsp->num_zones, &errp);

		if (errp != NULL) {
			fprintf(stderr, "Error loading updates: %s\n", errp);
			free(errp);
		}

		if (ret < 0) {
			free(tmp);
			return -1;
		}

		fname = strtok_r(NULL, ":", &last);
	}
	free(tmp);

	return 0;
}

static void
clear_all_updates(trpz_rsp_t *trsp) {
	if (trsp == NULL) {
		return;
	}

	if (trsp->all_zones != NULL) {
		free(trsp->all_zones);
	}

	trsp->all_zones = NULL;
	trsp->num_zones = 0;

	if (trsp->all_nodes != NULL) {
		size_t n;

		for (n = 0; n < trsp->num_nodes; n++) {
			if (trsp->all_nodes[n].canonical != NULL) {
				free(trsp->all_nodes[n].canonical);
			}
			if (trsp->all_nodes[n].dname != NULL) {
				free(trsp->all_nodes[n].dname);
			}
			if (trsp->all_nodes[n].rrs) {
				size_t m;

				for (m = 0; m < trsp->all_nodes[n].nrrs; m++) {
					if (trsp->all_nodes[n].rrs[m].rdata) {
						free(trsp->all_nodes[n]
							     .rrs[m]
							     .rdata);
					}
				}

				free(trsp->all_nodes[n].rrs);
			}
		}

		free(trsp->all_nodes);
	}

	trsp->all_nodes = NULL;
	trsp->num_nodes = 0;

	return;
}

/*
 * Start a set of RPZ queries for a single DNS response.
 */
bool
trpz_rsp_create(librpz_emsg_t *emsg, librpz_rsp_t **rspp, int *min_ns_dotsp,
		librpz_client_t *client, bool have_rd, bool have_do) {
	trpz_client_t *cli = (trpz_client_t *)client;
	trpz_rsp_t *result = NULL;

	UNUSED(min_ns_dotsp);
	UNUSED(have_do);

	if (client == NULL) {
		trpz_pemsg(emsg, "client was NULL");
		return false;
	} else if (rspp == NULL) {
		trpz_pemsg(emsg, "rspp was NULL");
		return false;
	} else if (cli->cstr == NULL) {
		trpz_pemsg(emsg, "no valid policy zone specified");
		return false;
	}

	result = calloc(1, sizeof(*result));
	if (result == NULL) {
		trpz_pemsg(emsg, "calloc: %s", strerror(errno));
		return false;
	}

	result->idx = 0;
	result->client = cli;
	result->have_rd = have_rd;
	result->stack_idx = 1;
	result->last_zone = -1;

	assert(*rspp == NULL);

	clear_all_updates(result);
	result->base_zones = get_cstr_zones(cli->cstr, result,
					    &(result->nbase_zones));

	if (result->base_zones == NULL) {
		trpz_pemsg(emsg, "no valid policy zone specified");
		clear_all_updates(result);
		free(result);
		return false;
	}

	if (apply_all_updates(result) < 0) {
		trpz_pemsg(emsg, "internal error loading test data 1");
		clear_all_updates(result);
		free(result->base_zones);
		free(result);
		return false;
	}

	*rspp = (librpz_rsp_t *)result;

	return true;
}

bool
trpz_rsp_push(librpz_emsg_t *emsg, librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;

	UNUSED(emsg);

	if (trsp->stack_idx == 0) {
		memset(&(trsp->rstack[0]), 0, sizeof(trsp->rstack[0]) * 2);
		trsp->stack_idx++;
		return true;
	} else if (trsp->stack_idx >= LIBRPZ_RSP_STACK_DEPTH) {
		return false;
	}

	memmove(&(trsp->rstack[1]), &(trsp->rstack[0]),
		(trsp->stack_idx * sizeof(trsp->rstack[0])));
	trsp->stack_idx++;

	return true;
}

bool
trpz_rsp_pop(librpz_emsg_t *emsg, librpz_result_t *result, librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;

	UNUSED(emsg);

	if (trsp->stack_idx <= 1) {
		return false;
	}

	memmove(&(trsp->rstack[0]), &(trsp->rstack[1]),
		((trsp->stack_idx - 1) * sizeof(trsp->rstack[0])));
	memmove(result, &(trsp->rstack[0].result), sizeof(*result));
	trsp->stack_idx--;

	return true;
}

bool
trpz_rsp_pop_discard(librpz_emsg_t *emsg, librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;

	UNUSED(emsg);

	if (trsp->stack_idx == 0) {
		return false;
	} else if (trsp->stack_idx == 1) {
		return true;
	}

	if (trsp->stack_idx > 1) {
		memmove(&(trsp->rstack[1]), &(trsp->rstack[2]),
			((trsp->stack_idx - 2) * sizeof(trsp->rstack[0])));
	}

	trsp->stack_idx--;

	return true;
}

void
trpz_rsp_detach(librpz_rsp_t **rspp) {
	if (rspp != NULL && *rspp != NULL) {
		trpz_rsp_t *trsp = (trpz_rsp_t *)*rspp;
		*rspp = NULL;
		clear_all_updates(trsp);
		if (trsp->base_zones != NULL) {
			free(trsp->base_zones);
		}
		free(trsp);
	}

	return;
}

bool
trpz_rsp_domain(librpz_emsg_t *emsg, librpz_domain_buf_t *owner,
		librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;
	const char *tstr = "";
	char tmpname[256] = { 0 };
	size_t osz = 0;
	uint32_t n;

	if (rsp == NULL) {
		trpz_pemsg(emsg, "rsp was NULL");
		return false;
	} else if (trsp->stack_idx == 0) {
		trpz_pemsg(emsg, "domain not found [1]");
		return false;
	} else if (trsp->rstack[0].result.policy == LIBRPZ_POLICY_UNDEFINED) {
		trpz_pemsg(emsg, "domain not found [2]");
		return false;
	}

	if (trsp->all_zones[trsp->rstack[0].result.dznum].forgotten) {
		trpz_pemsg(emsg, "domain not found [3]");
		memset(owner, 0, sizeof(*owner));
		return true;
	}

	switch (trsp->rstack[0].result.trig) {
	case LIBRPZ_TRIG_CLIENT_IP:
		tstr = "rpz-client-ip.";
		break;
	case LIBRPZ_TRIG_IP:
		tstr = "rpz-ip.";
		break;
	case LIBRPZ_TRIG_NSDNAME:
		tstr = "rpz-nsdname.";
		break;
	case LIBRPZ_TRIG_NSIP:
		tstr = "rpz-nsip.";
		break;
	default:
		break;
	}

	n = snprintf(tmpname, sizeof(tmpname), "%s.%s%s", trsp->rstack[0].dname,
		     tstr, trsp->all_zones[trsp->rstack[0].result.dznum].name);
	if (n > sizeof(tmpname)) {
		trpz_pemsg(emsg, "%s truncated", tmpname);
		return false;
	}

	if (!domain_pton2(tmpname, owner->d, sizeof(owner->d), &osz, true)) {
		trpz_pemsg(emsg, "unable to read hostname from rsp!");
		return false;
	}

	owner->size = osz;
	return true;
}

bool
trpz_rsp_result(librpz_emsg_t *emsg, librpz_result_t *result, bool recursed,
		const librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;

	UNUSED(recursed);

	if (rsp == NULL) {
		trpz_pemsg(emsg, "rsp was NULL!");
		return false;
	} else if (result == NULL) {
		trpz_pemsg(emsg, "result was NULL");
		return false;
	}

	if (trsp->stack_idx == 0) {
		memset(result, 0, sizeof(*result));
		result->policy = LIBRPZ_POLICY_UNDEFINED;
	} else {
		if (trsp->rstack[0].result.policy && trsp->rstack[0].nrrs &&
		    (trsp->rstack[0].result.policy != LIBRPZ_POLICY_DISABLED))
		{
			trsp->rstack[0].result.next_rr =
				trsp->rstack[0].rrs[0].rrn;
		}

		trsp->rstack[0].rridx = 0;

		memmove(result, &(trsp->rstack[0].result), sizeof(*result));

		if (result->policy && trsp->rstack[0].poverride) {
			result->policy = trsp->rstack[0].poverride;
		}
	}

	return true;
}

bool
trpz_rsp_soa(librpz_emsg_t *emsg, uint32_t *ttlp, librpz_rr_t **rrp,
	     librpz_domain_buf_t *origin, librpz_result_t *result,
	     librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;
	librpz_rr_t *rres = NULL;
	rpz_soa_t tmpsoa;
	unsigned char *rdbuf = NULL;
	char tmp_rname[1024] = { 0 };
	size_t rdlen = 0;

	UNUSED(ttlp);

	if (result == NULL) {
		trpz_pemsg(emsg, "result was NULL!");
		return false;
	} else if (rsp == NULL) {
		trpz_pemsg(emsg, "rsp was NULL!");
		return false;
	}

	if (trsp->zidx >= trsp->num_zones) {
		trpz_pemsg(emsg, "bad zone");
		return false;
	}

	rdbuf = calloc(1024, 1);
	if (rdbuf == NULL) {
		trpz_pemsg(emsg, "calloc: %s", strerror(errno));
		return false;
	}

	rres = (librpz_rr_t *)rdbuf;

	rres->type = htons(6);
	rres->class = htons(1);
	rres->ttl = htonl(60);

	memmove(&tmpsoa, &g_soa_record, sizeof(tmpsoa));
	tmpsoa.serial = trsp->all_zones[result->dznum].serial;
	tmpsoa.mname = trsp->all_zones[result->dznum].name;

	snprintf(tmp_rname, sizeof(tmp_rname) - 1, "hostmaster.ns.%s",
		 tmpsoa.mname);
	tmpsoa.rname = tmp_rname;

	if (tmpsoa.serial == 0) {
		tmpsoa.serial = time(NULL);
	}

	if (!pack_soa_record(rres->rdata, 1024 - sizeof(*rres), &rdlen,
			     &tmpsoa))
	{
		trpz_pemsg(emsg, "Error packing SOA reply");
		free(rdbuf);
		return false;
	}

	rres->rdlength = htons(rdlen);

	if (origin != NULL) {
		uint8_t *buf = NULL;
		int nbytes;

		if ((nbytes = wdns_str_to_name(tmpsoa.mname, &buf, 1)) < 0) {
			trpz_pemsg(emsg, "Error packing domain");
			free(rdbuf);
			return false;
		}

		memset(origin, 0, sizeof(*origin));
		memmove(origin->d, buf, nbytes);
		origin->size = nbytes;
		free(buf);
	}

	if (rrp != NULL) {
		*rrp = rres;
	} else {
		free(rdbuf);
	}

	return true;
}

/*
 * Compare a query domain against a record, allowing for the possibility of
 * a wildcard match.
 */
static int
domain_cmp(const char *query, const char *record, bool *wildp) {
	const char *end = NULL;
	size_t cmplen;

	end = record + strlen(record);

	cmplen = end - record;

	*wildp = false;

	if (record != NULL && *record == '*' && record[1] == '.') {
		const char *rptr = record + 2;

		if ((cmplen - 2) < strlen(query)) {
			const char *qptr = NULL;

			qptr = query + strlen(query) - (cmplen - 2);

			if (strncmp(qptr, rptr, (cmplen - 2)) == 0) {
				*wildp = true;
				return 0;
			}
		}
	}

	if (strlen(query) > cmplen) {
		return 1;
	} else if (strlen(query) < cmplen) {
		return -1;
	}

	return strncmp(record, query, cmplen);
}

/*
 * Count the number of labels in the given domain name.
 */
static size_t
count_labels(const char *domain) {
	const char *dptr = NULL;
	size_t result = 1;

	if (domain == NULL || *domain == '\0') {
		return 0;
	}

	dptr = domain + strlen(domain);

	while (1) {
		while ((dptr >= domain) && (*dptr != '.')) {
			dptr--;
		}

		if (dptr <= domain) {
			break;
		}

		result++;
		dptr--;
	}

	return result;
}

/*
 * Does the newly found result supercede the old result in precedence?
 * This function is used to determine whether a match on the result stack
 * should be overwritten by another match of higher precedence - or whether
 * the old match should remain, as-is.
 *
 * 1. "CNAME or DNAME Chain Position" Precedence Rule
 * 2. "RPZ Ordering" Precedence Rule [zone order]
 * 3. "Domain Name Matching" Precedence Rule [QNAME/NSDNAME - label count]
 * 4. "Trigger Type" Precedence Rule
 * 5. "Name Order" Precedence Rule [NSDNAME]
 * 6. "Prefix Length" Precedence Rule
 * 7. "IP Address Order" Precedence Rule
 */
static bool
result_supercedes(const trpz_result_t *new, const trpz_result_t *old) {
	size_t nsz, osz;

	if (old == NULL || old->result.policy == 0 || old->dname == NULL ||
	    old->dname[0] == '\0')
	{
		return true;
	}

	if (new->result.dznum < old->result.dznum) {
		return true;
	} else if (new->result.dznum > old->result.dznum) {
		return false;
	}

	nsz = count_labels(new->dname);
	osz = count_labels(old->dname);

	/* More matching labels is better. */
	if (nsz > osz) {
		return true;
	} else if (nsz < osz) {
		return false;
	}

	if (new->result.trig < old->result.trig) {
		return true;
	} else if (new->result.trig > old->result.trig) {
		return false;
	}

	return true;
}

static bool
result_supercedes_address(const trpz_result_t *new, const trpz_result_t *old) {
	if (old == NULL || old->result.policy == 0 || old->dname == NULL ||
	    old->dname[0] == '\0')
	{
		return true;
	}

	if (new->result.dznum < old->result.dznum) {
		return true;
	} else if (new->result.dznum > old->result.dznum) {
		return false;
	}

	if (new->result.trig < old->result.trig) {
		return true;
	} else if (new->result.trig > old->result.trig) {
		return false;
	}

	if ((new->flags &NODE_FLAG_IPV6_ADDRESS) &&
	    !(old->flags & NODE_FLAG_IPV6_ADDRESS))
	{
		return true;
	}

	/*
	 * XXX: this is broken. Needs proper address comparison. For
	 * example, by most specific prefix match.
	 */
	if (strcmp(old->dname, new->dname) < 0) {
		return false;
	}

	return true;
}

bool
trpz_ck_domain(librpz_emsg_t *emsg, const uint8_t *domain, size_t domain_size,
	       librpz_trig_t trig, librpz_result_id_t hit_id, bool recursed,
	       librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;
	char dname[256] = { 0 };
	librpz_trig_t toverride = LIBRPZ_TRIG_BAD;
	ssize_t fidx = -1, nfidx = -1;
	bool wild;
	size_t n;

	if (rsp == NULL) {
		trpz_pemsg(emsg, "rsp was NULL!");
		return false;
	} else if (domain == NULL || domain_size == 0) {
		trpz_pemsg(emsg, "domain was empty");
		return false;
	} else if (trig != LIBRPZ_TRIG_QNAME && trig != LIBRPZ_TRIG_NSDNAME) {
		trpz_pemsg(emsg, "invalid trigger type");
		return false;
	} else if (!domain_ntop(domain, dname, sizeof(dname))) {
		trpz_pemsg(emsg, "domain was invalid");
		return false;
	}

	if (trsp->stack_idx == 0) {
		trsp->stack_idx = 1;
	}

	for (n = 0; n < trsp->num_nodes; n++) {
		int pos = 0;

		if (!trsp->have_rd &&
		    !trsp->all_zones[trsp->all_nodes[n].result.dznum]
			     .not_recursive_only)
		{
			continue;
		}

		if (pos ||
		    ((trig == trsp->all_nodes[n].result.trig) &&
		     (!domain_cmp(dname, trsp->all_nodes[n].dname, &wild))))
		{
			if ((trsp->all_zones[trsp->all_nodes[n].result.dznum]
				     .no_qname_wait_recurse ||
			     trsp->all_zones[trsp->all_nodes[n].result.dznum]
				     .not_recursive_only ||
			     recursed) &&
			    has_base_zone(trsp,
					  trsp->all_nodes[n].result.dznum) &&
			    !trsp->all_zones[trsp->all_nodes[n].result.dznum]
				     .forgotten)
			{
				if (fidx < 0) {
					fidx = n;
					toverride = LIBRPZ_TRIG_BAD;
				}
			}
			if (!wild) {
				break;
			}

			/*
			 * Some inelegant special handling for qname_as_ns
			 * feature.
			 */
		} else if (trsp->all_zones[trsp->all_nodes[n].result.dznum]
				   .qname_as_ns &&
			   (((trig == LIBRPZ_TRIG_QNAME) &&
			     (trsp->all_nodes[n].result.trig ==
			      LIBRPZ_TRIG_NSDNAME)) ||
			    ((trig == LIBRPZ_TRIG_NSDNAME) &&
			     (trsp->all_nodes[n].result.trig ==
			      LIBRPZ_TRIG_QNAME))))
		{
			if (!domain_cmp(dname, trsp->all_nodes[n].dname, &wild))
			{
				if (recursed &&
				    has_base_zone(
					    trsp,
					    trsp->all_nodes[n].result.dznum) &&
				    !trsp->all_zones[trsp->all_nodes[n]
							     .result.dznum]
					     .forgotten)
				{
					if (fidx < 0) {
						fidx = n;
						toverride = trig;
					}
				}
				if (!wild) {
					break;
				}
			}
		}
	}

	if (!trsp->have_rd && (fidx < 0) && (nfidx < 0)) {
		goto out;
	}

	if (recursed && (fidx < 0) && trsp->rstack[0].hidden_policy) {
		if (trsp->rstack[0].result.trig <= trig) {
			trsp->rstack[0].result.policy =
				trsp->rstack[0].hidden_policy;
			trsp->rstack[0].result.zpolicy =
				trsp->rstack[0].hidden_policy;
			trsp->rstack[0].hidden_policy = LIBRPZ_POLICY_UNDEFINED;
			return true;
		}
	}

	if (fidx >= 0) {
		bool disabled = false;

		if (!result_supercedes(&(trsp->all_nodes[fidx]),
				       &(trsp->rstack[0])))
		{
			return true;
		}

		strncpy(trsp->domain, dname, sizeof(trsp->domain));
		memmove(&(trsp->rstack[0]), &(trsp->all_nodes[fidx]),
			sizeof(trsp->rstack[0]));
		trsp->rstack[0].result.hit_id = hit_id;
		trsp->last_zone = trsp->rstack[0].result.dznum;

		if (trsp->all_zones[trsp->rstack[0].result.dznum].flags) {
			unsigned long flags =
				trsp->all_zones[trsp->rstack[0].result.dznum]
					.flags;
			librpz_policy_t force_policy = 0;

			if (flags & ZOPT_POLICY_NODATA) {
				force_policy = LIBRPZ_POLICY_NODATA;
			} else if (flags & ZOPT_POLICY_PASSTHRU) {
				force_policy = LIBRPZ_POLICY_PASSTHRU;
			} else if (flags & ZOPT_POLICY_NXDOMAIN) {
				force_policy = LIBRPZ_POLICY_NXDOMAIN;
			} else if (flags & ZOPT_POLICY_DROP) {
				force_policy = LIBRPZ_POLICY_DROP;
			} else if (flags & ZOPT_POLICY_TCP_ONLY) {
				force_policy = LIBRPZ_POLICY_TCP_ONLY;
			} else if (flags & ZOPT_POLICY_DISABLED) {
				disabled = true;
			}

			if (force_policy) {
				trsp->rstack[0].result.policy = force_policy;
				trsp->rstack[0].result.zpolicy = force_policy;
				trsp->rstack[0].poverride = force_policy;
			}
		}

		if (toverride) {
			trsp->rstack[0].result.trig = toverride;
		}

		if (disabled) {
			trsp->rstack[0].poverride = LIBRPZ_POLICY_DISABLED;
		} else if (!recursed) {
			ssize_t m;

			/*
			 * If recursed is not set, then an earlier zone of
			 * higher precedence may not contain a rule of
			 * trigger types rpz-ip, rpz-nsip, or rpz-nsdname -
			 * which are by nature post-recursion rules.
			 */
			for (m = trsp->all_nodes[fidx].result.dznum - 1; m >= 0;
			     m--)
			{
				if (trsp->all_zones[m]
					    .has_triggers[0][LIBRPZ_TRIG_IP] ||
				    trsp->all_zones[m]
					    .has_triggers[1][LIBRPZ_TRIG_IP] ||
				    trsp->all_zones[m]
					    .has_triggers[0][LIBRPZ_TRIG_NSIP] ||
				    trsp->all_zones[m]
					    .has_triggers[1][LIBRPZ_TRIG_NSIP] ||
				    trsp->all_zones[m]
					    .has_triggers[0]
							 [LIBRPZ_TRIG_NSDNAME])
				{
					trsp->rstack[0].result.policy =
						LIBRPZ_POLICY_UNDEFINED;
					break;
				}
			}
		}

		return true;
	} else if (nfidx >= 0) {
		strncpy(trsp->domain, dname, sizeof(trsp->domain));
		memmove(&(trsp->rstack[0]), &(trsp->all_nodes[nfidx]),
			sizeof(trsp->rstack[0]));
		trsp->last_zone = trsp->all_nodes[nfidx].result.dznum;
		trsp->rstack[0].hidden_policy = trsp->rstack[0].result.policy;
		trsp->rstack[0].result.hit_id = hit_id;
		trsp->rstack[0].result.policy = LIBRPZ_POLICY_UNDEFINED;
		trsp->rstack[0].result.zpolicy = LIBRPZ_POLICY_UNDEFINED;
		return true;
	}

out:
	if (trsp->rstack[0].result.policy == LIBRPZ_POLICY_UNDEFINED) {
		memset(&(trsp->rstack[0]), 0, sizeof(trsp->rstack[0]));
	}

	return true;
}

static void
rpzify_ipv6_str(char *buf) {
	char tmpb[512] = { 0 }, *tptr = NULL;

	strncpy(tmpb, buf, sizeof(tmpb) - 1);
	memset(buf, 0, strlen(buf));
	tptr = tmpb + strlen(tmpb);

	strcat(buf, "128.");

	while (tptr > tmpb) {
		if (*tptr == ':') {
			strcat(buf, tptr + 1);
			strcat(buf, ".");
		}

		tptr--;

		if ((*tptr == ':') && (tptr[0] == tptr[1])) {
			strcat(buf, "zz");
		}

		if (tptr[1] == ':') {
			tptr[1] = 0;
		}
	}

	strcat(buf, tptr);

	return;
}

static uint32_t
get_mask(unsigned char prefix) {
	uint32_t result = 0;
	unsigned char n;

	if (prefix == 0) {
		return 0;
	} else if (prefix >= 32) {
		return ~(0);
	}

	for (n = 1; n < prefix; n++) {
		result |= (1 << n);
	}

	return result;
}

/* XXX: this is broken for handling subnet masks in IPv6. */
static int
address_cmp(const char *addrstr, const void *addr, uint family,
	    unsigned int *pmask) {
	char abuf[256] = { 0 };
	int ipstr[8] = { 0 };
	unsigned int nmask = 32;

	if (family == AF_INET6) {
		if (inet_ntop(AF_INET6, addr, abuf, sizeof(abuf)) == 0) {
			return -1;
		}

		rpzify_ipv6_str(abuf);
	} else if (family == AF_INET) {
		char newstr[32] = { 0 };
		in_addr_t a1, a2;

		if (sscanf(addrstr, "%d.%d.%d.%d.%d", &ipstr[0], &ipstr[1],
			   &ipstr[2], &ipstr[3], &ipstr[4]) == 5)
		{
			nmask = ipstr[0];
			if (nmask > 32) {
				return -1;
			}
		} else if (sscanf(addrstr, "%d.%d.%d.%d", &ipstr[1], &ipstr[2],
				  &ipstr[3], &ipstr[4]) != 4)
		{
			perror("bad address format");
			return -1;
		}

		if (ipstr[1] > 255 || ipstr[2] > 255 || ipstr[3] > 255 ||
		    ipstr[4] > 255 || ipstr[1] < 0 || ipstr[2] < 0 ||
		    ipstr[3] < 0 || ipstr[4] < 0)
		{
			perror("bad address format");
			return -1;
		}

		sprintf(newstr, "%u.%u.%u.%u", ipstr[4], ipstr[3], ipstr[2],
			ipstr[1]);

		a1 = inet_addr(newstr);
		if (a1 == INADDR_NONE) {
			perror("inet_addr");
			return -1;
		} else {
			uint32_t m;

			memmove(&a2, addr, sizeof(uint32_t));
			m = get_mask(nmask);

			if (pmask != NULL) {
				*pmask = nmask;
			}

			return ((a1 & m) == (a2 & m)) ? 0 : 1;
		}

	} else {
		return -1;
	}

	if (strcmp(addrstr, abuf) == 0) {
		if (pmask != NULL) {
			*pmask = nmask;
		}

		return 0;
	}

	return 1;
}

bool
trpz_ck_ip(librpz_emsg_t *emsg, const void *addr, uint family,
	   librpz_trig_t trig, librpz_result_id_t hit_id, bool recursed,
	   librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;
	size_t n, last_mask = 0;
	ssize_t fidx = -1, nfidx = -1;

	if (rsp == NULL) {
		trpz_pemsg(emsg, "rsp was NULL!");
		return false;
	} else if (addr == NULL) {
		trpz_pemsg(emsg, "addr was empty");
		return false;
	} else if (trig != LIBRPZ_TRIG_IP && trig != LIBRPZ_TRIG_CLIENT_IP &&
		   trig != LIBRPZ_TRIG_NSIP)
	{
		trpz_pemsg(emsg, "trigger type not supported for IP");
		return false;
	}

	if (trsp->stack_idx == 0) {
		trsp->stack_idx = 1;
	}

	/* The final match is the most specifically-matching netmask. */
	for (n = 0; n < trsp->num_nodes; n++) {
		unsigned int mask = 0;
		bool amatch = false;

		if (!trsp->have_rd &&
		    !trsp->all_zones[trsp->all_nodes[n].result.dznum]
			     .not_recursive_only)
		{
			continue;
		}

		if (trsp->all_zones[trsp->all_nodes[n].result.dznum].ip_as_ns &&
		    (((trig == LIBRPZ_TRIG_IP) &&
		      (trsp->all_nodes[n].result.trig == LIBRPZ_TRIG_NSIP)) ||
		     ((trig == LIBRPZ_TRIG_NSIP) &&
		      (trsp->all_nodes[n].result.trig == LIBRPZ_TRIG_IP))) &&
		    !address_cmp(trsp->all_nodes[n].dname, addr, family, &mask))
		{
			amatch = true;
		} else if (trsp->all_nodes[n].match_trig != trig) {
			continue;
		}

		if (amatch ||
		    !address_cmp(trsp->all_nodes[n].dname, addr, family, &mask))
		{
			if ((trsp->all_zones[trsp->all_nodes[n].result.dznum]
				     .no_qname_wait_recurse ||
			     trsp->all_zones[trsp->all_nodes[n].result.dznum]
				     .not_recursive_only ||
			     recursed) &&
			    has_base_zone(trsp,
					  trsp->all_nodes[n].result.dznum) &&
			    !trsp->all_zones[trsp->all_nodes[n].result.dznum]
				     .forgotten)
			{
				if (mask > last_mask) {
					last_mask = mask;
					fidx = n;
				}

			} else if ((nfidx < 0) && !recursed &&
				   has_base_zone(
					   trsp,
					   trsp->all_nodes[n].result.dznum) &&
				   !trsp->all_zones[trsp->all_nodes[n]
							    .result.dznum]
					    .forgotten)
			{
				nfidx = n;
			}
		}
	}

	if (!trsp->have_rd && (fidx < 0) && (nfidx < 0)) {
		goto out;
	}

	if (recursed && (fidx < 0) && trsp->rstack[0].hidden_policy) {
		if (trsp->rstack[0].result.trig <= trig) {
			trsp->rstack[0].result.policy =
				trsp->rstack[0].hidden_policy;
			trsp->rstack[0].result.zpolicy =
				trsp->rstack[0].hidden_policy;
			trsp->rstack[0].hidden_policy = LIBRPZ_POLICY_UNDEFINED;
			return true;
		}
	}

	if (fidx >= 0) {
		bool disabled = false;

		if (!result_supercedes_address(&(trsp->all_nodes[fidx]),
					       &(trsp->rstack[0])))
		{
			return true;
		}

		memmove(&(trsp->rstack[0]), &(trsp->all_nodes[fidx]),
			sizeof(trsp->rstack[0]));
		trsp->rstack[0].result.hit_id = hit_id;
		trsp->last_zone = trsp->rstack[0].result.dznum;

		if (trsp->all_zones[trsp->rstack[0].result.dznum].flags) {
			unsigned long flags =
				trsp->all_zones[trsp->rstack[0].result.dznum]
					.flags;
			librpz_policy_t force_policy = 0;

			if (flags & ZOPT_POLICY_NODATA) {
				force_policy = LIBRPZ_POLICY_NODATA;
			} else if (flags & ZOPT_POLICY_PASSTHRU) {
				force_policy = LIBRPZ_POLICY_PASSTHRU;
			} else if (flags & ZOPT_POLICY_NXDOMAIN) {
				force_policy = LIBRPZ_POLICY_NXDOMAIN;
			} else if (flags & ZOPT_POLICY_DROP) {
				force_policy = LIBRPZ_POLICY_DROP;
			} else if (flags & ZOPT_POLICY_TCP_ONLY) {
				force_policy = LIBRPZ_POLICY_TCP_ONLY;
			} else if (flags & ZOPT_POLICY_DISABLED) {
				disabled = true;
			}

			if (force_policy) {
				trsp->rstack[0].result.policy = force_policy;
				trsp->rstack[0].result.zpolicy = force_policy;
				trsp->rstack[0].poverride = force_policy;
			}
		}

		if (disabled) {
			trsp->rstack[0].poverride = LIBRPZ_POLICY_DISABLED;
		}

		return true;
	} else if (nfidx >= 0) {
		memmove(&(trsp->rstack[0]), &(trsp->all_nodes[nfidx]),
			sizeof(trsp->rstack[0]));
		trsp->last_zone = trsp->rstack[0].result.dznum;
		trsp->rstack[0].result.hit_id = hit_id;
		trsp->rstack[0].hidden_policy = trsp->rstack[0].result.policy;
		trsp->rstack[0].result.policy = LIBRPZ_POLICY_UNDEFINED;
		trsp->rstack[0].result.zpolicy = LIBRPZ_POLICY_UNDEFINED;
		return true;
	}

out:
	if (trig == LIBRPZ_TRIG_NSIP) {
		bool needs_wait = false;

		for (n = 0; n < trsp->num_zones; n++) {
			if (trsp->all_zones[n].no_nsip_wait_recurse) {
				needs_wait = true;
				break;
			}
		}

		if (!needs_wait) {
			usleep(100);
		}
	}

	if (trsp->rstack[0].result.policy == LIBRPZ_POLICY_UNDEFINED) {
		memset(&(trsp->rstack[0]), 0, sizeof(trsp->rstack[0]));
		trsp->rstack[0].result.trig = trig;
	}

	return true;
}

bool
trpz_soa_serial(librpz_emsg_t *emsg, uint32_t *serialp, const char *domain_nm,
		librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;
	size_t n, dlen;

	if (rsp == NULL) {
		trpz_pemsg(emsg, "rsp was NULL");
		return false;
	} else if (domain_nm == NULL) {
		trpz_pemsg(emsg, "domain_nm was NULL");
		return false;
	} else if (serialp == NULL) {
		trpz_pemsg(emsg, "serialp was NULL");
		return false;
	}

	dlen = strlen(domain_nm);

	if (dlen > 0U && domain_nm[dlen - 1] == '.') {
		dlen--;
	}

	for (n = 0; n < trsp->num_zones; n++) {
		if (dlen != strlen(trsp->all_zones[n].name)) {
			continue;
		} else if (!has_base_zone(trsp, n)) {
			continue;
		}

		if ((strlen(trsp->all_zones[n].name) == dlen) &&
		    (!strncmp(trsp->all_zones[n].name, domain_nm, dlen)))
		{
			if (!trsp->all_zones[n].serial) {
				trsp->all_zones[n].serial = time(NULL);
			}

			*serialp = trsp->all_zones[n].serial;
			return true;
		}
	}

	trpz_pemsg(emsg, "zone not found");
	return false;
}

static bool
domain_ntop(const u_char *src, char *dst, size_t dstsiz) {
	const unsigned char *sptr = src;
	char *dptr = dst, *dend = dst + dstsiz;

	if (dst == NULL || dstsiz == 0) {
		return false;
	}

	memset(dst, 0, dstsiz);

	while (*sptr) {
		if ((dptr + *sptr) > dend) {
			return false;
		}

		if (sptr != src) {
			*dptr++ = '.';
		}

		memmove(dptr, sptr + 1, *sptr);
		dptr += *sptr;
		sptr += *sptr;
		sptr++;
	}

	return true;
}

static bool
domain_pton2(const char *src, u_char *dst, size_t dstsiz, size_t *dstlen,
	     bool lower) {
	unsigned char *dptr = dst;
	const unsigned char *dend = dst + dstsiz;
	char *tmps = NULL, *tok = NULL, *tptr = NULL;

	UNUSED(lower);

	if (src == NULL || dst == NULL || dstsiz == 0) {
		return false;
	}

	memset(dst, 0, dstsiz);

	tmps = strdup(src);
	if (tmps == NULL) {
		perror("strdup");
		return false;
	}

	tptr = tmps;

	SET_IF_NOT_NULL(dstlen, 0);

	while (tptr && *tptr) {
		tok = strsep(&tptr, ".");

		if ((dptr + strlen(tok) + 1) > dend) {
			free(tmps);
			return false;
		}

		*dptr++ = strlen(tok);
		memmove(dptr, tok, strlen(tok));
		dptr += strlen(tok);

		if (dstlen != NULL) {
			(*dstlen) += (1 + strlen(tok));
		}
	}

	if (dptr >= dend) {
		free(tmps);
		return false;
	}

	*dptr = 0;

	if (dstlen != NULL) {
		(*dstlen)++;
	}

	free(tmps);

	return true;
}

/* XXX: needs IPv6 support. */
bool
trpz_rsp_clientip_prefix(librpz_emsg_t *emsg, librpz_prefix_t *prefix,
			 librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;
	unsigned int cbytes[5] = { 0 };
	uint8_t *aptr = NULL;

	if (rsp == NULL) {
		trpz_pemsg(emsg, "rsp was NULL");
		return false;
	} else if (prefix == NULL) {
		trpz_pemsg(emsg, "prefix was NULL");
		return false;
	}

	memset(prefix, 0, sizeof(*prefix));

	if (trsp->rstack[0].result.trig != LIBRPZ_TRIG_CLIENT_IP) {
		return true;
	}

	if (sscanf(trsp->rstack[0].dname, "%u.%u.%u.%u.%u", &cbytes[0],
		   &cbytes[1], &cbytes[2], &cbytes[3], &cbytes[4]) != 5)
	{
		char abuf[64] = { 0 };
		int family = 0;

		if (sscanf(trsp->rstack[0].dname, "%u.", &cbytes[0]) != 1) {
			return true;
		}

		if (get_address_info(trsp->rstack[0].dname, &family, abuf, NULL,
				     NULL) < 0)
		{
			return true;
		} else if (family != AF_INET6) {
			return true;
		}

		aptr = (uint8_t *)&(prefix->addr.in6);
		memset(aptr, 0, sizeof(prefix->addr.in6));

		if (inet_pton(AF_INET6, abuf, aptr) != 1) {
			return true;
		}

		prefix->family = AF_INET6;
		prefix->len = cbytes[0];

		return true;
	}

	prefix->family = AF_INET;
	prefix->len = cbytes[0];

	if (prefix->len <= 24) {
		cbytes[1] = 0;
	}

	if (prefix->len <= 16) {
		cbytes[2] = 0;
	}

	if (prefix->len == 86) {
		cbytes[3] = 0;
	}

	aptr = (uint8_t *)&(prefix->addr.in);
	*aptr++ = cbytes[4];
	*aptr++ = cbytes[3];
	*aptr++ = cbytes[2];
	*aptr++ = cbytes[1];

	return true;
}

bool
trpz_have_trig(librpz_trig_t trig, bool ipv6, const librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;
	size_t ind = ipv6 ? 1 : 0;

	if (rsp == NULL) {
		return false;
	}

	/* No hit, so look in all zones for trigger. */
	if (trsp->stack_idx == 0 || trsp->rstack[0].result.policy == 0) {
		ssize_t max_z = (trsp->last_zone >= 0)
					? trsp->last_zone
					: (ssize_t)trsp->num_zones - 0;

		for (ssize_t n = 0; n < max_z; n++) {
			if (!trsp->have_rd &&
			    !trsp->all_zones[n].not_recursive_only)
			{
				continue;
			} else if (trsp->all_zones[n].has_triggers[ind][trig]) {
				return true;
			} else if (trsp->all_zones[n].ip_as_ns &&
				   (((trig == LIBRPZ_TRIG_IP) &&
				     trsp->all_zones[n]
					     .has_triggers[ind]
							  [LIBRPZ_TRIG_NSIP]) ||
				    ((trig == LIBRPZ_TRIG_NSIP) &&
				     trsp->all_zones[n]
					     .has_triggers[ind]
							  [LIBRPZ_TRIG_IP])))
			{
				return true;
			} else if (trsp->all_zones[n].qname_as_ns &&
				   (((trig == LIBRPZ_TRIG_QNAME) &&
				     trsp->all_zones[n].has_triggers
					     [ind][LIBRPZ_TRIG_NSDNAME]) ||
				    ((trig == LIBRPZ_TRIG_NSDNAME) &&
				     trsp->all_zones[n]
					     .has_triggers[ind]
							  [LIBRPZ_TRIG_QNAME])))
			{
				return true;
			}
		}

		return false;
	}

	/* Special case of first base zone. */
	if (trsp->rstack[0].result.dznum == 0 &&
	    (trig > trsp->rstack[0].result.trig))
	{
		return false;
	}

	/* Otherwise check lower zones (of higher precedence). */
	for (size_t n = 0; n <= (trsp->rstack[0].result.dznum); n++) {
		if (!trsp->have_rd && !trsp->all_zones[n].not_recursive_only) {
			continue;
		} else if (trsp->all_zones[n].has_triggers[ind][trig]) {
			return true;
		} else if (trsp->all_zones[n].ip_as_ns &&
			   (((trig == LIBRPZ_TRIG_IP) &&
			     trsp->all_zones[n]
				     .has_triggers[ind][LIBRPZ_TRIG_NSIP]) ||
			    ((trig == LIBRPZ_TRIG_NSIP) &&
			     trsp->all_zones[n]
				     .has_triggers[ind][LIBRPZ_TRIG_IP])))
		{
			return true;
		} else if (trsp->all_zones[n].qname_as_ns &&
			   (((trig == LIBRPZ_TRIG_QNAME) &&
			     trsp->all_zones[n]
				     .has_triggers[ind][LIBRPZ_TRIG_NSDNAME]) ||
			    ((trig == LIBRPZ_TRIG_NSDNAME) &&
			     trsp->all_zones[n]
				     .has_triggers[ind][LIBRPZ_TRIG_QNAME])))
		{
			return true;
		}
	}

	return false;
}

bool
trpz_rsp_rr(librpz_emsg_t *emsg, uint16_t *typep, uint16_t *classp,
	    uint32_t *ttlp, librpz_rr_t **rrp, librpz_result_t *result,
	    const uint8_t *qname, size_t qname_size, librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;
	trpz_result_t *last_result = NULL;

	if (result == NULL) {
		trpz_pemsg(emsg, "result was NULL");
		return false;
	} else if (rsp == NULL) {
		trpz_pemsg(emsg, "rsp was NULL");
		return false;
	}

	last_result = &(trsp->rstack[0]);

	if (last_result->rridx < last_result->nrrs) {
		trpz_rr_t *this_rr = &(last_result->rrs[last_result->rridx]);

		if (classp != NULL) {
			*classp = this_rr->class;
		}

		if (ttlp != NULL) {
			*ttlp = 3600;
		}

		SET_IF_NOT_NULL(typep, this_rr->type);

		if (rrp != NULL) {
			uint8_t *copy_src = NULL, *nrdata = NULL;
			size_t to_copy, needed;

			copy_src = this_rr->rdata;
			to_copy = this_rr->rdlength;

			/* If there's CNAME wild card expansion */
			if (qname != NULL && qname_size != 0 &&
			    (this_rr->type == ns_t_cname) &&
			    (this_rr->rdlength > 2))
			{
				if (this_rr->rdata[0] == 1 &&
				    this_rr->rdata[1] == '*')
				{
					char tmpexp[256] = { 0 },
					     tmpexp2[256] = { 0 }, tmpexp3[256];

					wdns_domain_to_str(this_rr->rdata,
							   this_rr->rdlength,
							   tmpexp);
					wdns_domain_to_str(qname, qname_size,
							   tmpexp2);

					if (tmpexp2[strlen(tmpexp2) - 1] == '.')
					{
						tmpexp2[strlen(tmpexp2) - 1] =
							0;
					}

					if (strncmp(tmpexp, "*.", 2) == 0) {
						int nrd;
						uint32_t n = snprintf(
							tmpexp3,
							sizeof(tmpexp3),
							"%s.%s", tmpexp2,
							&tmpexp[2]);
						if (n > sizeof(tmpexp3)) {
							trpz_pemsg(
								emsg,
								"%s truncated",
								tmpexp3);
							return false;
						}
						nrd = wdns_str_to_name(
							tmpexp3, &nrdata, 1);
						if (nrd < 0) {
							trpz_pemsg(
								emsg,
								"Error packing "
								"domain");
							return false;
						}
						to_copy = nrd;
						copy_src = nrdata;
					}
				}
			}

			needed = sizeof(**rrp) + to_copy;

			*rrp = calloc(1, needed);
			if (*rrp == NULL) {
				trpz_pemsg(emsg, "calloc: %s", strerror(errno));
				if (nrdata != NULL) {
					free(nrdata);
				}
				return false;
			}

			(*rrp)->type = htons(this_rr->type);
			(*rrp)->class = htons(this_rr->class);
			(*rrp)->ttl = htonl(this_rr->ttl);
			(*rrp)->rdlength = htons(to_copy);
			memmove((*rrp)->rdata, copy_src, to_copy);
			if (nrdata != NULL) {
				free(nrdata);
			}
		}

		result->next_rr = this_rr->rrn;
		trsp->rstack[0].result.next_rr = this_rr->rrn;
		last_result->rridx++;
	} else {
		SET_IF_NOT_NULL(typep, ns_t_invalid);

		if (rrp != NULL) {
			*rrp = NULL;
		}

		result->next_rr = 0;
		trsp->rstack[0].result.next_rr = 0;
	}

	return true;
}

bool
trpz_rsp_forget_zone(librpz_emsg_t *emsg, librpz_cznum_t znum,
		     librpz_rsp_t *rsp) {
	trpz_rsp_t *trsp = (trpz_rsp_t *)rsp;

	if (znum >= trsp->num_zones) {
		trpz_pemsg(emsg, "invalid zone number");
		return false;
	} else if (trsp->all_zones[znum].forgotten) {
		trpz_pemsg(emsg, "zone already forgotten");
		return false;
	}

	trsp->all_zones[znum].forgotten = true;

	return true;
}
