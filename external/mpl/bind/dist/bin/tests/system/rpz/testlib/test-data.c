/*	$NetBSD: test-data.c,v 1.2 2025/01/26 16:25:00 christos Exp $	*/

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

#define _GNU_SOURCE 1
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <isc/atomic.h>
#include <isc/util.h>

#include "test-data.h"

const rpz_soa_t g_soa_record = { "a.rpz-ns.dns-nod.net",
				 "nod-admin.fsi.io",
				 12345,
				 3600,
				 1200,
				 604800,
				 60 };

int
wdns_str_to_name(const char *str, uint8_t **pbuf, bool downcase);

static char *
str_printf(const char *fmt, ...) {
	va_list ap;
	char tbuf[8192], *result = NULL;

	va_start(ap, fmt);
	vsnprintf(tbuf, sizeof(tbuf) - 1, fmt, ap);
	tbuf[sizeof(tbuf) - 1] = 0;
	va_end(ap);

	result = strdup(tbuf);
	if (result == NULL) {
		perror("strdup");
	}

	return result;
}

/*
 * Given a config-ready RPZ IP address, determine its family and normal
 * canonical representation.
 */
int
get_address_info(const char *astr, int *pfamily, char *pbuf,
		 const char *optname, char **errp) {
	char tmpc[512] = { 0 };
	char *tok = NULL, *tptr = tmpc, *last_tok = NULL;
	size_t lcount = 0, bcount = 0;
	unsigned int prefix = 0, values[16] = { 0 }, hex_values[16] = { 0 };
	bool is_ipv6 = false;

	if (astr == NULL || pfamily == NULL || pbuf == NULL) {
		return -1;
	}

	strncpy(tmpc, astr, sizeof(tmpc) - 1);

	while ((tok = strsep(&tptr, "."))) {
		char *eptr = NULL;
		unsigned long val;

		lcount++;
		last_tok = tok;

		errno = 0;
		val = strtoul(tok, &eptr, 10);

		if (errno != 0 || *eptr != '\0') {
			bool bad = false;

			bcount++;
			errno = 0;
			eptr = NULL;
			val = strtoul(tok, &eptr, 16);

			if (errno || *eptr != '\0') {
				if (strcmp(tok, "zz") == 0) {
					val = ~0;
					is_ipv6 = true;
				} else {
					bad = true;
				}
			}

			if (!bad && (lcount > 1)) {
				hex_values[lcount - 2] = val;
			}
		} else {
			if (val > 255) {
				bcount++;
			}

			if (lcount == 1) {
				prefix = val;
			} else if (lcount > 1) {
				unsigned int hexval;
				values[lcount - 2] = val;

				/*
				 * All integer strings are valid hex
				 * strings, but decimal values are longer,
				 * so we have to check for overflow when
				 * reading as hex.
				 */
				errno = 0;
				hexval = strtoul(tok, &eptr, 16);
				if (errno != 0) {
					return -1;
				}
				hex_values[lcount - 2] = hexval;
			}
		}
	}

	if (last_tok && (strncmp(last_tok, "rpz-", 4) == 0)) {
		lcount--;
		bcount--;
	}

	/* Not acceptable for either address family. */
	if (lcount > 9) {
		return -1;
	}

	*pfamily = (!is_ipv6 && (lcount == 5)) ? AF_INET : AF_INET6;

	/*
	 * For AF_INET we expect exactly 4 "good" (0<->255) octets and the
	 * subnet mask.
	 */
	if (*pfamily == AF_INET) {
		if (prefix > 32) {
			if (errp != NULL) {
				*errp = str_printf(
					"invalid rpz IP address \"%s\"; "
					"invalid prefix length of %u",
					(optname ? optname : astr), prefix);
			}

			return -1;
		} else if (bcount > 0) {
			return -1;
		}

		sprintf(pbuf, "%u.%u.%u.%u", values[3], values[2], values[1],
			values[0]);
	} else {
		size_t n;

		if (prefix > 128) {
			if (errp != NULL) {
				*errp = str_printf(
					"invalid rpz IP address \"%s\"; "
					"invalid prefix length of %u",
					(optname ? optname : astr), prefix);
			}

			return -1;
		}

		*pbuf = 0;

		/*
		 * Walk the values backward. Account for :: and discard
		 * chunks > 2 octets.
		 */
		for (n = lcount - 1; n > 0; n--) {
			if (hex_values[n - 1] == ~0U) {
				strcat(pbuf, ":");
			} else {
				if (hex_values[n - 1] > 0xffff) {
					return -1;
				} else if (n > 1) {
					sprintf(&pbuf[strlen(pbuf)],
						"%x:", hex_values[n - 1]);
				} else {
					sprintf(&pbuf[strlen(pbuf)], "%x",
						hex_values[n - 1]);
				}
			}
		}
	}

	return 0;
}

rpz_soa_t *
parse_serial(unsigned char *rdata, size_t rdlen) {
	rpz_soa_t *result = NULL;
	char dname[WDNS_PRESLEN_NAME];
	size_t mlen, rlen;
	uint32_t *uptr = NULL;

	result = calloc(1, sizeof(*result));
	if (result == NULL) {
		perror("calloc");
		return NULL;
	}

	mlen = wdns_domain_to_str(rdata, rdlen, dname);
	result->mname = strdup(dname);
	rlen = wdns_domain_to_str(rdata + mlen, rdlen - mlen, dname);
	result->rname = strdup(dname);
	uptr = (uint32_t *)(rdata + mlen + rlen);
	result->serial = ntohl(*uptr);
	uptr++;
	result->refresh = ntohl(*uptr);
	uptr++;
	result->retry = ntohl(*uptr);
	uptr++;
	result->expire = ntohl(*uptr);
	uptr++;
	result->minimum = ntohl(*uptr);

	return result;
}

size_t
wdns_domain_to_str(const uint8_t *src, size_t src_len, char *dst) {
	size_t bytes_read = 0;
	size_t bytes_remaining = src_len;
	uint8_t oclen;

	if (src == NULL) {
		return 0;
	}

	oclen = *src;
	while (bytes_remaining > 0 && oclen != 0) {
		src++;
		bytes_remaining--;

		bytes_read += oclen + 1 /* length octet */;

		while (oclen-- && bytes_remaining > 0) {
			uint8_t c = *src++;
			bytes_remaining--;

			if (c == '.' || c == '\\') {
				*dst++ = '\\';
				*dst++ = c;
			} else if (c >= '!' && c <= '~') {
				*dst++ = c;
			} else {
				snprintf(dst, 5, "\\%.3d", c);
				dst += 4;
			}
		}
		*dst++ = '.';
		oclen = *src;
	}
	if (bytes_read == 0) {
		*dst++ = '.';
	}
	bytes_read++;

	*dst = '\0';
	return bytes_read;
}

/* Add parsed update specification to maintained list of nodes. */
static trpz_result_t *
apply_update_to_set(trpz_result_t **results, size_t *pnresults,
		    trpz_zone_t **pzones, const char *node, size_t zidx,
		    uint32_t ttl, librpz_trig_t trigger, librpz_policy_t policy,
		    int *modified, unsigned long flags, char **errp) {
	size_t n;
	int family = 0;

	UNUSED(flags);

	*modified = 0;

	switch (trigger) {
	case LIBRPZ_TRIG_QNAME:
	case LIBRPZ_TRIG_NSDNAME:
		(*pzones)[zidx].has_triggers[0][trigger] = 1;
		break;
	case LIBRPZ_TRIG_CLIENT_IP:
	case LIBRPZ_TRIG_IP:
	case LIBRPZ_TRIG_NSIP: {
		char abuf[128];

		if (get_address_info(node, &family, abuf, NULL, errp) < 0) {
			fprintf(stderr,
				"Error in determining IP address type: %s\n",
				node);
			return NULL;
		} else if (family == AF_INET) {
			(*pzones)[zidx].has_triggers[0][trigger] = 1;
		} else {
			(*pzones)[zidx].has_triggers[1][trigger] = 1;
		}

	} break;
	default:
		break;
	}

	for (n = 0; n < *pnresults; n++) {
		trpz_result_t *rptr = &((*results)[n]);

		if (rptr->result.cznum != zidx) {
			continue;
		}

		if (!strcmp(rptr->dname, node)) {
			if (rptr->result.trig == trigger &&
			    rptr->result.policy == policy && rptr->ttl == ttl)
			{
				return rptr;
			}

			rptr->result.trig = trigger;
			rptr->result.policy = policy;
			rptr->result.zpolicy = policy;
			rptr->ttl = ttl;
			*modified = 1;
			return rptr;
		}
	}

	/* No match. Instead, append. */
	(*pnresults)++;

	*results = realloc(*results, (*pnresults * sizeof(**results)));
	if (*results == NULL) {
		perror("realloc");
		return NULL;
	}

	memset(&((*results)[*pnresults - 1]), 0, sizeof(**results));
	(*results)[*pnresults - 1].dname = strdup(node);
	(*results)[*pnresults - 1].ttl = ttl;
	(*results)[*pnresults - 1].result.trig = trigger;
	(*results)[*pnresults - 1].result.policy = policy;
	(*results)[*pnresults - 1].result.zpolicy = policy;
	(*results)[*pnresults - 1].result.cznum = zidx;
	(*results)[*pnresults - 1].result.dznum = zidx;
	(*results)[*pnresults - 1].result.log = 1;
	(*results)[*pnresults - 1].poverride = policy;
	(*results)[*pnresults - 1].match_trig = trigger;

	if (family == AF_INET6) {
		(*results)[*pnresults - 1].flags |= NODE_FLAG_IPV6_ADDRESS;
	}

	*modified = 1;
	return &((*results)[*pnresults - 1]);
}

/*
 * Add a parsed RR value that is maintained in conjunction with record policy
 * items.
 */
static int
add_other_rr(trpz_result_t *node, const char *rrtype, const char *val,
	     uint32_t ttl, int *modified) {
	trpz_rr_t nrec = { 0 };
	size_t n;
	static atomic_uint_fast32_t rrn = 1;

	*modified = 0;

	nrec.class = ns_c_in;
	nrec.ttl = ttl;
	nrec.rrn = atomic_fetch_add_relaxed(&rrn, 1);

	if (!strcasecmp(rrtype, "A")) {
		uint32_t addr;

		if (inet_pton(AF_INET, val, &addr) != 1) {
			fprintf(stderr,
				"Error determining policy record IPv4 address: "
				"%s\n",
				val);
			return -1;
		}

		nrec.type = ns_t_a;
		nrec.rdlength = sizeof(uint32_t);

		nrec.rdata = malloc(nrec.rdlength);
		if (nrec.rdata == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		memmove(nrec.rdata, &addr, nrec.rdlength);
	} else if (!strcasecmp(rrtype, "AAAA")) {
		char addr[16] = { 0 };

		if (inet_pton(AF_INET6, val, addr) != 1) {
			fprintf(stderr,
				"Error determining policy record IPv6 address: "
				"%s\n",
				val);
			return -1;
		}

		nrec.type = ns_t_aaaa;
		nrec.rdlength = sizeof(addr);

		nrec.rdata = malloc(nrec.rdlength);
		if (nrec.rdata == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		memmove(nrec.rdata, addr, nrec.rdlength);
	} else if (!strcasecmp(rrtype, "TXT")) {
		nrec.type = ns_t_txt;
		nrec.rdlength = 1 + strlen(val);

		nrec.rdata = calloc(nrec.rdlength, 1);
		if (nrec.rdata == NULL) {
			perror("calloc");
			exit(EXIT_FAILURE);
		}

		nrec.rdata[0] = nrec.rdlength - 1;
		memmove(&(nrec.rdata[1]), val, nrec.rdlength - 1);
	} else if (!strcasecmp(rrtype, "CNAME")) {
		int ret;

		nrec.type = ns_t_cname;
		ret = wdns_str_to_name(val, &(nrec.rdata), 1);

		if (ret <= 0) {
			fprintf(stderr,
				"Error processing CNAME policy record data "
				"(%d)!\n",
				ret);
			return -1;
		}

		nrec.rdlength = ret;
	} else if (!strcasecmp(rrtype, "DNAME")) {
		int ret;

		nrec.type = ns_t_dname;
		ret = wdns_str_to_name(val, &(nrec.rdata), 1);

		if (ret <= 0) {
			fprintf(stderr,
				"Error processing DNAME policy record data "
				"(%d)!\n",
				ret);
			return -1;
		}

		nrec.rdlength = ret;
	} else {
		fprintf(stderr,
			"Error: unsupported policy record type: \"%s\"\n",
			rrtype);
		return -1;
	}

	for (n = 0; n < node->nrrs; n++) {
		trpz_rr_t *rptr = &(node->rrs[n]);

		/* Same thing. Don't replace. */
		if (rptr->type == nrec.type && rptr->class == nrec.class &&
		    rptr->ttl == nrec.ttl && rptr->rdlength == nrec.rdlength &&
		    !memcmp(rptr->rdata, nrec.rdata, nrec.rdlength))
		{
			free(nrec.rdata);
			return n + 1;
		}
	}

	node->nrrs++;

	node->rrs = realloc(node->rrs, (node->nrrs * sizeof(*(node->rrs))));
	if (node->rrs == NULL) {
		perror("realloc");
		exit(EXIT_FAILURE);
	}

	memset(&(node->rrs[node->nrrs - 1]), 0, sizeof(node->rrs[0]));
	node->rrs[node->nrrs - 1] = nrec;
	*modified = 1;

	return node->nrrs;
}

void
reverse_labels(const char *str, char *pbuf) {
	const char *sptr = str, *end = NULL;

	if (sptr == NULL || *sptr == 0) {
		return;
	}

	sptr += (strlen(sptr) - 1);
	end = sptr + 1;
	*pbuf = 0;

	if (*sptr == '.') {
		sptr--;
		end--;
	}

	while (sptr >= str) {
		if ((*sptr != '.') && (sptr != str)) {
			sptr--;
			continue;
		}

		if (sptr == str) {
			strncat(pbuf, sptr, (end - sptr));
			break;
		}

		strncat(pbuf, sptr + 1, (end - (sptr + 1)));
		strcat(pbuf, ".");
		end = sptr--;
	}

	if (pbuf[strlen(pbuf) - 1] == '.') {
		pbuf[strlen(pbuf) - 1] = 0;
	}

	return;
}

/* Parse trailing zone options as specified in a cstr line */
unsigned long
parse_zone_options(const char *str) {
	char tmpstr[8192] = { 0 };
	char *tok = NULL, *sptr = NULL;
	unsigned long result = 0;

	if (str == NULL || *str == 0) {
		return 0;
	}

	strncpy(tmpstr, str, sizeof(tmpstr) - 1);

	tok = strtok_r(tmpstr, " ", &sptr);

	while (tok) {
		if (!strcasecmp(tok, "policy")) {
			tok = strtok_r(NULL, " ", &sptr);
			if (tok == NULL) {
				break;
			}

			if (!strcasecmp(tok, "passthru")) {
				result |= ZOPT_POLICY_PASSTHRU;
			} else if (!strcasecmp(tok, "drop")) {
				result |= ZOPT_POLICY_DROP;
			} else if (!strcasecmp(tok, "tcp-only")) {
				result |= ZOPT_POLICY_TCP_ONLY;
			} else if (!strcasecmp(tok, "nxdomain")) {
				result |= ZOPT_POLICY_NXDOMAIN;
			} else if (!strcasecmp(tok, "nodata")) {
				result |= ZOPT_POLICY_NODATA;
			} else if (!strcasecmp(tok, "given")) {
				result |= ZOPT_POLICY_GIVEN;
			} else if (!strcasecmp(tok, "disabled")) {
				result |= ZOPT_POLICY_DISABLED;
			} else if (!strcasecmp(tok, "no-op")) {
				;
			}
		} else {
			if (!strcasecmp(tok, "max-policy-ttl")) {
				tok = strtok_r(NULL, " ", &sptr);
				if (tok == NULL) {
					break;
				}
			} else if (!strcasecmp(tok, "recursive-only")) {
				tok = strtok_r(NULL, " ", &sptr);
				if (tok == NULL) {
					break;
				}

				if (!strcasecmp(tok, "yes")) {
					result |= ZOPT_RECURSIVE_ONLY;
				} else if (!strcasecmp(tok, "no")) {
					result |= ZOPT_NOT_RECURSIVE_ONLY;
				}

			} else {
				if (!strcasecmp(tok, "qname-as-ns")) {
					tok = strtok_r(NULL, " ", &sptr);
					if (tok == NULL) {
						break;
					}

					if (!strcasecmp(tok, "yes")) {
						result |= ZOPT_QNAME_AS_NS;
					}

				} else if (!strcasecmp(tok, "ip-as-ns")) {
					tok = strtok_r(NULL, " ", &sptr);
					if (tok == NULL) {
						break;
					}

					if (!strcasecmp(tok, "yes")) {
						result |= ZOPT_IP_AS_NS;
					}

				} else if (!strcasecmp(tok,
						       "qname-wait-recurse"))
				{
					tok = strtok_r(NULL, " ", &sptr);
					if (tok == NULL) {
						break;
					}

					if (!strcasecmp(tok, "no")) {
						result |=
							ZOPT_NO_QNAME_WAIT_RECURSE;
					}

				} else if (!strcasecmp(tok,
						       "nsip-wait-recurse"))
				{
					tok = strtok_r(NULL, " ", &sptr);
					if (tok == NULL) {
						break;
					}

					if (!strcasecmp(tok, "no")) {
						result |=
							ZOPT_NO_NSIP_WAIT_RECURSE;
					}
				}
			}
		}

		tok = strtok_r(NULL, " ", &sptr);
	}

	/*        LIBRPZ_POLICY_CNAME,      */
	return result;
}

/*
 * Parse an update string and attempt to add any relevant data to the node
 * and policy RR tables.
 */
int
apply_update(const char *updstr, trpz_result_t **presults, size_t *pnresults,
	     trpz_zone_t **pzones, size_t *pnzones, int is_static,
	     unsigned long flags, char **errp) {
	trpz_result_t *res = NULL;
	char cmdbuf[64] = { 0 }, nodebuf[256] = { 0 }, rrbuf[32] = { 0 },
	     databuf[256] = { 0 };
	char *nend = NULL;
	librpz_policy_t policy = LIBRPZ_POLICY_UNDEFINED;
	librpz_trig_t trig = LIBRPZ_TRIG_QNAME;
	unsigned int ttl;
	ssize_t n, zidx = -1;
	size_t ndlen = 0, last_matchlen = 0;
	int nfield, zupd = 0;

	nfield = sscanf(updstr, "%63s %255s %u %31s %255s", cmdbuf, nodebuf,
			&ttl, rrbuf, databuf);
	if (nfield < 1) {
		return -1;
	}

	/*
	 * Special case for handling zone additions; here the 'ttl' field
	 * becomes a serial.
	 */
	if (!strcasecmp(cmdbuf, "zone")) {
		trpz_zone_t *zptr = NULL;
		bool qname_as_ns = false, ip_as_ns = false,
		     not_recursive_only = false, do_inc = false;
		bool no_qname_as_ns = false, no_ip_as_ns = false,
		     recursive_only = false, no_nsip_wait_recurse = false;

		if (nfield < 3) {
			return -1;
		}

		if (!strcasecmp(rrbuf, "qname_as_ns")) {
			qname_as_ns = true;
		} else if (!strcasecmp(rrbuf, "ip_as_ns")) {
			ip_as_ns = true;
		} else if (!strcasecmp(rrbuf, "not_recursive_only")) {
			not_recursive_only = true;
		} else if (!strcasecmp(rrbuf, "inc")) {
			do_inc = true;
		} else if (!strcasecmp(rrbuf, "no_qname_as_ns")) {
			no_qname_as_ns = true;
		} else if (!strcasecmp(rrbuf, "no_ip_as_ns")) {
			no_ip_as_ns = true;
		} else if (!strcasecmp(rrbuf, "recursive_only")) {
			recursive_only = true;
		} else if (!strcasecmp(rrbuf, "no_nsip_wait_recurse")) {
			no_nsip_wait_recurse = true;
		}

		if (flags & ZOPT_RECURSIVE_ONLY) {
			recursive_only = true;
			not_recursive_only = false;
		} else if (flags & ZOPT_NOT_RECURSIVE_ONLY) {
			recursive_only = false;
			not_recursive_only = true;
		}

		if (flags & ZOPT_NO_NSIP_WAIT_RECURSE) {
			no_nsip_wait_recurse = true;
		}

		for (n = 0; (size_t)n < *pnzones; n++) {
			if (!strcmp((*pzones)[n].name, nodebuf)) {
				/*
				 * Force override of serial. But only if
				 * serial is non-zero.
				 */
				if (ttl) {
					if (do_inc) {
						(*pzones)[n].serial += ttl;
					} else {
						(*pzones)[n].serial = ttl;
					}
				}

				(*pzones)[n].has_update = 0;

				if (qname_as_ns) {
					(*pzones)[n].qname_as_ns = true;
				}

				if (ip_as_ns) {
					(*pzones)[n].ip_as_ns = true;
				}

				if (no_qname_as_ns) {
					(*pzones)[n].qname_as_ns = false;
				}

				if (no_ip_as_ns) {
					(*pzones)[n].ip_as_ns = false;
				}

				if (not_recursive_only) {
					(*pzones)[n].not_recursive_only = true;
				}

				if (recursive_only) {
					(*pzones)[n].not_recursive_only = false;
				}

				if (flags & ZOPT_NO_QNAME_WAIT_RECURSE) {
					(*pzones)[n].no_qname_wait_recurse =
						true;
				}

				if (no_nsip_wait_recurse) {
					(*pzones)[n].no_nsip_wait_recurse =
						true;
				}

				return 0;
			}
		}

		(*pnzones)++;

		*pzones = realloc(*pzones, (*pnzones * sizeof(**pzones)));
		if (*pzones == NULL) {
			perror("realloc");
			exit(EXIT_FAILURE);
		}

		zptr = &(*pzones)[*pnzones - 1];
		*zptr = (trpz_zone_t){
			.serial = ttl,
			.qname_as_ns = qname_as_ns,
			.ip_as_ns = ip_as_ns,
			.flags = flags,
			.not_recursive_only = not_recursive_only,
		};

		if (qname_as_ns) {
			(*pzones)[n].qname_as_ns = true;
		}

		if (ip_as_ns) {
			(*pzones)[n].ip_as_ns = true;
		}

		if (flags & ZOPT_NO_QNAME_WAIT_RECURSE) {
			(*pzones)[n].no_qname_wait_recurse = true;
		}

		if (no_nsip_wait_recurse) {
			(*pzones)[n].no_nsip_wait_recurse = true;
		}

		strncpy(zptr->name, nodebuf, LIBRPZ_MAXDOMAIN + 1);
		if (zptr->name[strlen(zptr->name) - 1] == '.') {
			zptr->name[strlen(zptr->name) - 1] = 0;
		}

		return 0;
	} else if (nfield != 5) {
		return -1;
	}

	if (strcasecmp(cmdbuf, "add")) {
		fprintf(stderr, "Warning: only update add action is currently "
				"supported!\n");
		return -1;
	}

	if (!strcasecmp(rrbuf, "A")) {
		policy = LIBRPZ_POLICY_RECORD;
	} else if (!strcasecmp(rrbuf, "CNAME")) {
		if (!strcmp(databuf, ".")) {
			policy = LIBRPZ_POLICY_NXDOMAIN;
		} else if (!strcmp(databuf, "*.")) {
			policy = LIBRPZ_POLICY_NODATA;
		} else if (!strcasecmp(databuf, "rpz-passthru.")) {
			policy = LIBRPZ_POLICY_PASSTHRU;
		} else if (!strcasecmp(databuf, "rpz-drop.")) {
			policy = LIBRPZ_POLICY_DROP;
		} else if (!strcasecmp(databuf, "rpz-tcp-only.")) {
			policy = LIBRPZ_POLICY_TCP_ONLY;
		} else {
			policy = LIBRPZ_POLICY_RECORD;
		}

	} else if (!strcasecmp(rrbuf, "TXT")) {
		char *ftext = NULL;

		ftext = strstr(updstr, databuf);
		if (ftext == NULL) {
			fprintf(stderr, "Error parsing TXT record: \"%s\"\n",
				updstr);
			return -1;
		}

		if (*ftext == '"') {
			*ftext++ = 0;

			if (ftext[strlen(ftext) - 1] == '"') {
				ftext[strlen(ftext) - 1] = 0;
			}
		}

		strncpy(databuf, ftext, sizeof(databuf));
		databuf[sizeof(databuf) - 1] = 0;
		policy = LIBRPZ_POLICY_RECORD;
	} else if (!strcasecmp(rrbuf, "DNAME")) {
		policy = LIBRPZ_POLICY_RECORD;
	} else if (!strcasecmp(rrbuf, "AAAA")) {
		policy = LIBRPZ_POLICY_RECORD;
	} else {
		fprintf(stderr,
			"Warning: target \"%s\" is not currently supported!\n",
			rrbuf);
		return -1;
	}

	if (policy == LIBRPZ_POLICY_UNDEFINED) {
		fprintf(stderr, "Error: could not determine appropriate policy "
				"for update!\n");
		return -1;
	}

	for (n = 0; (size_t)n < *pnzones; n++) {
		const char *zptr = nodebuf;
		size_t cmplen;

		zptr += strlen(zptr) - 1;

		if (*zptr == '.') {
			zptr--;
		}

		cmplen = strlen((*pzones)[n].name);

		if ((*pzones)[n].name[cmplen - 1] == '.') {
			cmplen--;
		}

		zptr -= (cmplen - 1);

		if ((zptr <= nodebuf) || (*(zptr - 1) != '.')) {
			continue;
		}

		if (!strncmp((*pzones)[n].name, zptr, cmplen)) {
			/*
			 * We don't break immediately after a match because
			 * there might be a better one yet.
			 */
			if (cmplen > last_matchlen) {
				ndlen = strlen(nodebuf) - cmplen;

				if (nodebuf[strlen(nodebuf) - 1] == '.') {
					ndlen--;
				}

				/*
				 * Account for the period between the node name
				 * and zone name.
				 */
				ndlen--;
				zidx = n;
				last_matchlen = cmplen;
			}
		}
	}

	if (memmem(nodebuf, ndlen, ".rpz-", 5)) {
		char *tptr = nodebuf + ndlen - 1;
		size_t slen;

		while (strncmp(tptr, ".rpz-", 5)) {
			tptr--;
		}

		slen = nodebuf + ndlen - tptr;
		nend = tptr;

		if (slen == 7 && !memcmp(tptr, ".rpz-ip", 7)) {
			trig = LIBRPZ_TRIG_IP;
		} else if (slen == 9 && !memcmp(tptr, ".rpz-nsip", 9)) {
			trig = LIBRPZ_TRIG_NSIP;
		} else if (slen == 14 && !memcmp(tptr, ".rpz-client-ip", 14)) {
			trig = LIBRPZ_TRIG_CLIENT_IP;
		} else if (slen == 12 && !memcmp(tptr, ".rpz-nsdname", 12)) {
			trig = LIBRPZ_TRIG_NSDNAME;
		} else {
			fprintf(stderr, "Warning: unknown suffix \"%s\"\n",
				tptr);
			nend = NULL;
		}

		/* We saved the trigger value, so shave that part off. */
		*tptr = 0;
	}

	if (zidx == -1) {
		return 0;
	}

	nodebuf[ndlen] = 0;

	/*
	 * The original, deprecated PASSTHRU encoding of a CNAME pointing
	 * to the trigger QNAME might still be in use in local, private
	 * policy zones, and  so it is still recognized by RPZ subscriber
	 * implementations as of 2016.
	 */
	if ((policy == LIBRPZ_POLICY_RECORD) && !strcasecmp(rrbuf, "cname")) {
		char tmpname[512] = { 0 };

		strncpy(tmpname, databuf, sizeof(tmpname) - 1);

		if ((nodebuf[strlen(nodebuf) - 1] == '.') &&
		    (tmpname[strlen(tmpname) - 1] != '.'))
		{
			size_t tlen = strlen(tmpname);

			tmpname[tlen] = '.';
			tmpname[tlen + 1] = 0;
		} else if ((nodebuf[strlen(nodebuf) - 1] != '.') &&
			   (tmpname[strlen(tmpname) - 1] == '.'))
		{
			tmpname[strlen(tmpname) - 1] = 0;
		}

		/* A special case of PASSTHRU (with trailing characters) */
		if (nend != NULL &&
		    (strlen(databuf) == (size_t)(nend - nodebuf)) &&
		    !strncmp(databuf, nodebuf, (nend - nodebuf)))
		{
			policy = LIBRPZ_POLICY_PASSTHRU;
		}

		if (!strcmp(nodebuf, tmpname)) {
			policy = LIBRPZ_POLICY_PASSTHRU;
		}
	}

	res = apply_update_to_set(presults, pnresults, pzones, nodebuf, zidx,
				  ttl, trig, policy, &zupd, flags, errp);

	if (res) {
		if (zupd && !is_static) {
			(*pzones)[zidx].has_update = 1;
		} else if (is_static) {
			res->flags |= NODE_FLAG_STATIC_DATA;
		}

		if (policy == LIBRPZ_POLICY_RECORD) {
			/*
			 * Policy/RR change does not seem to prompt zone
			 * serial increment. (has_update)
			 */
			if (add_other_rr(res, rrbuf, databuf, ttl, &zupd) < 0) {
				fprintf(stderr,
					"Error: could not add policy record %s "
					"/ %s\n",
					rrbuf, databuf);
				return -1;
			}
		}
	}

	return 0;
}

/*
 * XXX: memory leak. Also, does not properly preserve "static" node entries,
 * as envisioned.
 */
static void
free_nodes(trpz_result_t **presults, size_t *pnresults) {
	size_t n, tot;

	if (presults == NULL || *presults == NULL) {
		if (pnresults != NULL) {
			*pnresults = 0;
		}
		return;
	}

	tot = *pnresults;

	for (n = tot; n > 0; n--) {
		trpz_result_t *res = &((*presults)[n - 1]);
		size_t m;

		if (res->canonical != NULL) {
			free(res->canonical);
		}

		if (res->dname != NULL) {
			free(res->dname);
		}

		for (m = 0; m < res->nrrs; m++) {
			if (res->rrs[m].rdata != NULL) {
				free(res->rrs[m].rdata);
			}
		}

		if (res->rrs != NULL) {
			free(res->rrs);
		}
	}

	free(*presults);
	*presults = NULL;
	*pnresults = 0;

	return;
}

/*
 * Perform only sanity checking on a data file's contents.
 *
 * Note that this function only really exists to facilitate the logging of error
 * messages that may be expected to occur upon encounter with certain invalid
 * node data in unit tests.
 *
 * fname is the pathname of the data file to be checked.
 * errp is a pointer to an error string that may be set if this function fails.
 * It is the responsibility of the caller to free this pointer if it is returned
 * non-NULL.
 *
 * This function returns 0 on success, or -1 on failure, possibly setting *errp
 * on failure.
 */
int
sanity_check_data_file(const char *fname, char **errp) {
	FILE *f = NULL;
	int result = -1;

	SET_IF_NOT_NULL(errp, NULL);

	f = fopen(fname, "r");
	if (f == NULL) {
		fprintf(stderr, "couldn't sanity check %s\n", fname);
		perror("fopen");
		return -1;
	}

	while (!feof(f)) {
		char line[1024] = { 0 }, cmdbuf[64] = { 0 },
		     nodebuf[256] = { 0 }, rrbuf[32] = { 0 },
		     databuf[256] = { 0 };
		char *lptr = line;
		int nfield;
		unsigned int ttl;

		if (!fgets(line, sizeof(line) - 1, f)) {
			break;
		}

		if (!line[0]) {
			continue;
		}

		if (line[strlen(line) - 1] == '\n') {
			line[strlen(line) - 1] = 0;
		}

		if (!line[0] || line[0] == ';') {
			continue;
		}

		while (*lptr && !isspace((unsigned char)*lptr)) {
			lptr++;
		}

		*lptr++ = 0;

		if (!strcasecmp(line, "server") || !strcasecmp(line, "send") ||
		    !strcasecmp(line, "wipe") ||
		    !strcasecmp(line, "rollback") ||
		    !strcasecmp(line, "restart"))
		{
			continue;
		} else if (strcasecmp(line, "static") &&
			   strcasecmp(line, "update"))
		{
			if (errp != NULL) {
				*errp = str_printf("Found unknown instruction "
						   "directive: \"%s\"\n",
						   line);
			}

			goto out;
		}

		while (isspace((unsigned char)*lptr)) {
			lptr++;
		}

		nfield = sscanf(lptr, "%63s %255s %u %31s %255s", cmdbuf,
				nodebuf, &ttl, rrbuf, databuf);

		/* We don't care about checking zones - only node entries. */
		if (nfield != 5) {
			continue;
		}

		if (strcasecmp(cmdbuf, "add")) {
			continue;
		}

		if (strcasecmp(rrbuf, "A") && strcasecmp(rrbuf, "CNAME") &&
		    strcasecmp(rrbuf, "TXT") && strcasecmp(rrbuf, "DNAME") &&
		    strcasecmp(rrbuf, "AAAA"))
		{
			if (errp != NULL) {
				*errp = str_printf("Target \"%s\" is not "
						   "currently supported!\n",
						   rrbuf);
			}

			goto out;
		}

		if (strstr(nodebuf, ".rpz-")) {
			char abuf[64], tmpname[512];
			char *tptr = nodebuf;
			size_t slen;
			int family;

			while (strncmp(tptr, ".rpz-", 5)) {
				tptr++;
			}

			slen = nodebuf + strlen(nodebuf) - tptr;

			if (!(slen >= 8 && !memcmp(tptr, ".rpz-ip.", 8)) &&
			    !(slen >= 10 && !memcmp(tptr, ".rpz-nsip.", 10)) &&
			    !(slen >= 15 &&
			      !memcmp(tptr, ".rpz-client-ip.", 15)) &&
			    !(slen >= 13 && !memcmp(tptr, ".rpz-nsdname.", 13)))
			{
				continue;
			}

			strncpy(tmpname, nodebuf, sizeof(tmpname) - 1);
			tmpname[sizeof(tmpname) - 1] = 0;

			*tptr = 0;

			if (get_address_info(nodebuf, &family, abuf, tmpname,
					     errp) < 0)
			{
				goto out;
			}
		}
	}

	result = 0;

out:
	fclose(f);

	return result;
}

/* Load a database of nodes from a given filename. */
int
load_all_updates(const char *fname, trpz_result_t **presults, size_t *pnresults,
		 trpz_zone_t **pzones, size_t *pnzones, char **errp) {
	FILE *f = NULL;

	f = fopen(fname, "r");
	if (f == NULL) {
		fprintf(stderr, "couldn't load updates from %s\n", fname);
		perror("fopen");
		return -1;
	}

	while (!feof(f)) {
		char line[1024] = { 0 };
		char *lptr = line;
		int is_static = 0;

		if (!fgets(line, sizeof(line) - 1, f)) {
			break;
		}

		if (!line[0]) {
			continue;
		}

		if (line[strlen(line) - 1] == '\n') {
			line[strlen(line) - 1] = 0;
		}

		if (!line[0]) {
			strcpy(line, "send");
		}

		if (!line[0] || line[0] == ';') {
			continue;
		}

		while (*lptr && !isspace((unsigned char)*lptr)) {
			lptr++;
		}

		*lptr++ = 0;

		if (!strcasecmp(line, "server")) {
			continue;
		} else if (!strcasecmp(line, "send")) {
			size_t n;

			for (n = 0; n < *pnzones; n++) {
				if ((*pzones)[n].has_update) {
					(*pzones)[n].serial += 1;
					(*pzones)[n].rollback += 1;
					(*pzones)[n].has_update = 0;
				}
			}

			continue;
		} else if (!strcasecmp(line, "wipe") ||
			   !strcasecmp(line, "rollback"))
		{
			size_t n;
			int rollback;

			rollback = strcasecmp(line, "rollback") == 0;

			free_nodes(presults, pnresults);

			/* Now push forward the serial by # rollback */
			for (n = 0; n < *pnzones; n++) {
				if (rollback) {
					(*pzones)[n].serial +=
						(*pzones)[n].rollback;
					(*pzones)[n].rollback = 0;
				}

				memset((*pzones)[n].has_triggers, 0,
				       sizeof((*pzones)[n].has_triggers));
			}

			continue;
		} else if (!strcasecmp(line, "static")) {
			is_static = 1;
		} else if (!strcasecmp(line, "restart")) {
			size_t n;

			for (n = 0; n < *pnzones; n++) {
				(*pzones)[n].serial = 1;
				(*pzones)[n].rollback = 0;
			}

			continue;
		} else if (strcasecmp(line, "update")) {
			fprintf(stderr,
				"Warning: skipping unknown instruction "
				"directive: \"%s\"\n",
				line);
			continue;
		}

		/* Everything here is an update */
		while (isspace((unsigned char)*lptr)) {
			lptr++;
		}

		if (apply_update(lptr, presults, pnresults, pzones, pnzones,
				 is_static, 0, errp) == -1)
		{
			fprintf(stderr,
				"Error: could not apply update \"%s\"\n", lptr);
			fclose(f);
			return -1;
		}
	}

	fclose(f);

	return 0;
}

#define WDNS_MAXLEN_NAME 255
int
wdns_str_to_name(const char *str, uint8_t **pbuf, bool downcase) {
	const char *p = NULL;
	size_t label_len;
	ssize_t slen;
	uint8_t c, *oclen = NULL, *data = NULL;
	int res = -1;

	assert(pbuf != NULL);

	p = str;
	slen = strlen(str);

	if (slen == 1 && *p == '.') {
		*pbuf = malloc(1);
		if (*pbuf == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		*pbuf[0] = 0;
		return 1;
	}

	res = 0;
	*pbuf = malloc(WDNS_MAXLEN_NAME);
	if (*pbuf == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	data = *pbuf;
	label_len = 0;
	oclen = data++;
	res++;

	for (;;) {
		c = *p++;
		label_len++;

		/* Will the wire name become too long? */
		if (res >= WDNS_MAXLEN_NAME) {
			goto out;
		}

		if (slen == 0) {
			/* end of input */
			*oclen = --label_len;
			*data++ = '\0';
			res++;
			break;
		}

		if (c >= 'A' && c <= 'Z') {
			/* an upper case letter; downcase it */
			if (downcase) {
				c |= 0x20;
			}
			*data++ = c;
			res++;
		} else if (c == '\\' && !isdigit((unsigned char)*p)) {
			/* an escaped character */
			if (slen <= 0) {
				goto out;
			}
			*data++ = *p;
			res++;
			p++;
			slen--;
		} else if (c == '\\' && slen >= 3) {
			/* an escaped octet */
			char d[4];
			char *endptr = NULL;
			long int val;

			d[0] = *p++;
			d[1] = *p++;
			d[2] = *p++;
			d[3] = '\0';
			slen -= 3;
			if (!isdigit((unsigned char)d[0]) ||
			    !isdigit((unsigned char)d[1]) ||
			    !isdigit((unsigned char)d[2]))
			{
				goto out;
			}
			val = strtol(d, &endptr, 10);
			if (endptr != NULL && *endptr == '\0' && val >= 0 &&
			    val <= 255)
			{
				uint8_t uval;

				uval = (uint8_t)val;
				*data++ = uval;
				res++;
			} else {
				goto out;
			}
		} else if (c == '\\') {
			/* should not occur */
			goto out;
		} else if (c == '.') {
			/* end of label */
			*oclen = --label_len;
			if (label_len == 0) {
				goto out;
			}
			oclen = data++;
			if (slen > 1) {
				res++;
			}
			label_len = 0;
		} else if (c != '\0') {
			*data++ = c;
			res++;
		}

		slen--;
	}

	return res;

out:
	free(*pbuf);
	*pbuf = NULL;
	return -1;
}
