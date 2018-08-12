/*	$NetBSD: dns.h,v 1.2 2018/08/12 13:02:32 christos Exp $	*/

/*
 * Copyright (C) 2000, 2001  Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (C) 2004 - 2015 Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND NOMINUM DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL NOMINUM BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <isc/types.h>

#ifndef PERF_DNS_H
#define PERF_DNS_H 1

#define MAX_UDP_PACKET 512
#define MAX_EDNS_PACKET 4096

typedef struct perf_dnstsigkey perf_dnstsigkey_t;
typedef struct perf_dnsctx perf_dnsctx_t;

extern const char *perf_dns_rcode_strings[];

perf_dnstsigkey_t *
perf_dns_parsetsigkey(const char *arg, isc_mem_t *mctx);

void
perf_dns_destroytsigkey(perf_dnstsigkey_t **tsigkeyp);

perf_dnsctx_t *
perf_dns_createctx(isc_boolean_t updates);

void
perf_dns_destroyctx(perf_dnsctx_t **ctxp);

isc_result_t
perf_dns_buildrequest(perf_dnsctx_t *ctx, const isc_textregion_t *record,
		      isc_uint16_t qid,
		      isc_boolean_t edns, isc_boolean_t dnssec,
		      perf_dnstsigkey_t *tsigkey, isc_buffer_t *msg);

#endif
