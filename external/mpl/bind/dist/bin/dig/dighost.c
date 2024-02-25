/*	$NetBSD: dighost.c,v 1.13.2.2 2024/02/25 15:43:02 martin Exp $	*/

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

/*! \file
 *  \note
 * Notice to programmers:  Do not use this code as an example of how to
 * use the ISC library to perform DNS lookups.  Dig and Host both operate
 * on the request level, since they allow fine-tuning of output and are
 * intended as debugging tools.  As a result, they perform many of the
 * functions which could be better handled using the dns_resolver
 * functions in most applications.
 */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_LIBIDN2
#include <idn2.h>
#endif /* HAVE_LIBIDN2 */

#include <isc/app.h>
#include <isc/base64.h>
#include <isc/file.h>
#include <isc/hex.h>
#include <isc/lang.h>
#include <isc/log.h>
#include <isc/managers.h>
#include <isc/netaddr.h>
#include <isc/netdb.h>
#include <isc/nonce.h>
#include <isc/parseint.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/safe.h>
#include <isc/serial.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/byaddr.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/opcode.h>
#include <dns/rcode.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/tsig.h>

#include <dst/dst.h>

#include <isccfg/namedconf.h>

#include <irs/resconf.h>

#include <bind9/getaddresses.h>

#include "dighost.h"

#define systemlocale(l) (void)setlocale(l, "")
#define resetlocale(l)	(void)setlocale(l, "C")

dig_lookuplist_t lookup_list;
dig_serverlist_t server_list;
dig_searchlistlist_t search_list;

static atomic_bool cancel_now = false;

bool check_ra = false, have_ipv4 = false, have_ipv6 = false,
     specified_source = false, free_now = false, usesearch = false,
     showsearch = false, is_dst_up = false, keep_open = false, verbose = false,
     yaml = false;
in_port_t port = 53;
bool port_set = false;
unsigned int timeout = 0;
unsigned int extrabytes;
isc_mem_t *mctx = NULL;
isc_log_t *lctx = NULL;
isc_nm_t *netmgr = NULL;
isc_taskmgr_t *taskmgr = NULL;
isc_task_t *global_task = NULL;
isc_sockaddr_t localaddr;
isc_refcount_t sendcount = 0;
isc_refcount_t recvcount = 0;
int ndots = -1;
int tries = -1;
int lookup_counter = 0;

static char servercookie[256];

#ifdef HAVE_LIBIDN2
static void
idn_locale_to_ace(const char *src, char *dst, size_t dstlen);
static void
idn_ace_to_locale(const char *src, char **dst);
static isc_result_t
idn_output_filter(isc_buffer_t *buffer, unsigned int used_org);
#endif /* HAVE_LIBIDN2 */

isc_nmhandle_t *keep = NULL;
isc_sockaddr_t keepaddr;

/*%
 * Exit Codes:
 *
 *\li	0   Everything went well, including things like NXDOMAIN
 *\li	1   Usage error
 *\li	7   Got too many RR's or Names
 *\li	8   Couldn't open batch file
 *\li	9   No reply from server
 *\li	10  Internal error
 */
int exitcode = 0;
int fatalexit = 0;
char keynametext[MXNAME];
char keyfile[MXNAME] = "";
char keysecret[MXNAME] = "";
unsigned char cookie_secret[33];
unsigned char cookie[8];
const dns_name_t *hmacname = NULL;
unsigned int digestbits = 0;
isc_buffer_t *namebuf = NULL;
dns_tsigkey_t *tsigkey = NULL;
dst_key_t *sig0key = NULL;
bool validated = true;
bool debugging = false;
bool debugtiming = false;
bool memdebugging = false;
const char *progname = NULL;
isc_mutex_t lookup_lock;
dig_lookup_t *current_lookup = NULL;

#define DIG_MAX_ADDRESSES 20

/*%
 * Apply and clear locks at the event level in global task.
 * Can I get rid of these using shutdown events?  XXX
 */
#define LOCK_LOOKUP                                                       \
	{                                                                 \
		debug("lock_lookup %s:%d", __FILE__, __LINE__);           \
		check_result(isc_mutex_lock((&lookup_lock)), "isc_mutex_" \
							     "lock");     \
		debug("success");                                         \
	}
#define UNLOCK_LOOKUP                                                       \
	{                                                                   \
		debug("unlock_lookup %s:%d", __FILE__, __LINE__);           \
		check_result(isc_mutex_unlock((&lookup_lock)), "isc_mutex_" \
							       "unlock");   \
	}

static void
default_warnerr(const char *format, ...) {
	va_list args;

	printf(";; ");
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

static void
default_comments(dig_lookup_t *lookup, const char *format, ...) {
	va_list args;

	if (lookup->comments) {
		printf(";; ");
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		printf("\n");
	}
}

/* dynamic callbacks */

isc_result_t (*dighost_printmessage)(dig_query_t *query,
				     const isc_buffer_t *msgbuf,
				     dns_message_t *msg, bool headers);

void (*dighost_error)(const char *format, ...) = default_warnerr;

void (*dighost_warning)(const char *format, ...) = default_warnerr;

void (*dighost_comments)(dig_lookup_t *lookup, const char *format,
			 ...) = default_comments;

void (*dighost_received)(unsigned int bytes, isc_sockaddr_t *from,
			 dig_query_t *query);

void (*dighost_trying)(char *frm, dig_lookup_t *lookup);

void (*dighost_shutdown)(void);

/* forward declarations */

#define cancel_lookup(l) _cancel_lookup(l, __FILE__, __LINE__)
static void
_cancel_lookup(dig_lookup_t *lookup, const char *file, unsigned int line);

static void
recv_done(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	  void *arg);

static void
start_udp(dig_query_t *query);

static void
start_tcp(dig_query_t *query);

static void
force_next(dig_query_t *query);

static void
launch_next_query(dig_query_t *query);

static void
clear_current_lookup(void);

static bool
next_origin(dig_lookup_t *oldlookup);

static int
count_dots(char *string) {
	char *s;
	int i = 0;

	s = string;
	while (*s != '\0') {
		if (*s == '.') {
			i++;
		}
		s++;
	}
	return (i);
}

static void
hex_dump(isc_buffer_t *b) {
	unsigned int len, i;
	isc_region_t r;

	isc_buffer_usedregion(b, &r);

	printf("%u bytes\n", r.length);
	for (len = 0; len < r.length; len++) {
		printf("%02x ", r.base[len]);
		if (len % 16 == 15) {
			printf("         ");
			for (i = len - 15; i <= len; i++) {
				if (r.base[i] >= '!' && r.base[i] <= '}') {
					putchar(r.base[i]);
				} else {
					putchar('.');
				}
			}
			printf("\n");
		}
	}
	if (len % 16 != 0) {
		for (i = len; (i % 16) != 0; i++) {
			printf("   ");
		}
		printf("         ");
		for (i = ((len >> 4) << 4); i < len; i++) {
			if (r.base[i] >= '!' && r.base[i] <= '}') {
				putchar(r.base[i]);
			} else {
				putchar('.');
			}
		}
		printf("\n");
	}
}

/*%
 * Append 'len' bytes of 'text' at '*p', failing with
 * ISC_R_NOSPACE if that would advance p past 'end'.
 */
static isc_result_t
append(const char *text, size_t len, char **p, char *end) {
	if (*p + len > end) {
		return (ISC_R_NOSPACE);
	}
	memmove(*p, text, len);
	*p += len;
	return (ISC_R_SUCCESS);
}

static isc_result_t
reverse_octets(const char *in, char **p, char *end) {
	const char *dot = strchr(in, '.');
	size_t len;
	if (dot != NULL) {
		isc_result_t result;
		result = reverse_octets(dot + 1, p, end);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		result = append(".", 1, p, end);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		len = (int)(dot - in);
	} else {
		len = (int)strlen(in);
	}
	return (append(in, len, p, end));
}

isc_result_t
get_reverse(char *reverse, size_t len, char *value, bool strict) {
	int r;
	isc_result_t result;
	isc_netaddr_t addr;

	addr.family = AF_INET6;
	r = inet_pton(AF_INET6, value, &addr.type.in6);
	if (r > 0) {
		/* This is a valid IPv6 address. */
		dns_fixedname_t fname;
		dns_name_t *name;
		unsigned int options = 0;

		name = dns_fixedname_initname(&fname);
		result = dns_byaddr_createptrname(&addr, options, name);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		dns_name_format(name, reverse, (unsigned int)len);
		return (ISC_R_SUCCESS);
	} else {
		/*
		 * Not a valid IPv6 address.  Assume IPv4.
		 * If 'strict' is not set, construct the
		 * in-addr.arpa name by blindly reversing
		 * octets whether or not they look like integers,
		 * so that this can be used for RFC2317 names
		 * and such.
		 */
		char *p = reverse;
		char *end = reverse + len;
		if (strict && inet_pton(AF_INET, value, &addr.type.in) != 1) {
			return (DNS_R_BADDOTTEDQUAD);
		}
		result = reverse_octets(value, &p, end);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		/* Append .in-addr.arpa. and a terminating NUL. */
		result = append(".in-addr.arpa.", 15, &p, end);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		return (ISC_R_SUCCESS);
	}
}

void (*dighost_pre_exit_hook)(void) = NULL;

#if TARGET_OS_IPHONE
void
warn(const char *format, ...) {
	va_list args;

	fflush(stdout);
	fprintf(stderr, ";; Warning: ");
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}
#else  /* if TARGET_OS_IPHONE */
void
warn(const char *format, ...) {
	va_list args;

	fflush(stdout);
	fprintf(stderr, "%s: ", progname);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}
#endif /* if TARGET_OS_IPHONE */

void
digexit(void) {
	if (exitcode < 10) {
		exitcode = 10;
	}
	if (fatalexit != 0) {
		exitcode = fatalexit;
	}
	if (dighost_pre_exit_hook != NULL) {
		dighost_pre_exit_hook();
	}
	exit(exitcode);
}

void
fatal(const char *format, ...) {
	va_list args;

	fflush(stdout);
	fprintf(stderr, "%s: ", progname);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	digexit();
}

void
debug(const char *format, ...) {
	va_list args;
	isc_time_t t;

	if (debugging) {
		fflush(stdout);
		if (debugtiming) {
			TIME_NOW(&t);
			fprintf(stderr, "%u.%06u: ", isc_time_seconds(&t),
				isc_time_nanoseconds(&t) / 1000);
		}
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		fprintf(stderr, "\n");
	}
}

void
check_result(isc_result_t result, const char *msg) {
	if (result != ISC_R_SUCCESS) {
		fatal("%s: %s", msg, isc_result_totext(result));
	}
}

/*%
 * Create a server structure, which is part of the lookup structure.
 * This is little more than a linked list of servers to query in hopes
 * of finding the answer the user is looking for
 */
dig_server_t *
make_server(const char *servname, const char *userarg) {
	dig_server_t *srv;

	REQUIRE(servname != NULL);

	debug("make_server(%s)", servname);
	srv = isc_mem_allocate(mctx, sizeof(struct dig_server));
	strlcpy(srv->servername, servname, MXNAME);
	strlcpy(srv->userarg, userarg, MXNAME);
	ISC_LINK_INIT(srv, link);
	return (srv);
}

/*%
 * Create a copy of the server list from the resolver configuration structure.
 * The dest list must have already had ISC_LIST_INIT applied.
 */
static void
get_server_list(irs_resconf_t *resconf) {
	isc_sockaddrlist_t *servers;
	isc_sockaddr_t *sa;
	dig_server_t *newsrv;
	char tmp[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255") +
		 sizeof("%4000000000")];
	debug("get_server_list()");
	servers = irs_resconf_getnameservers(resconf);
	for (sa = ISC_LIST_HEAD(*servers); sa != NULL;
	     sa = ISC_LIST_NEXT(sa, link))
	{
		int pf = isc_sockaddr_pf(sa);
		isc_netaddr_t na;
		isc_result_t result;
		isc_buffer_t b;

		if (pf == AF_INET && !have_ipv4) {
			continue;
		}
		if (pf == AF_INET6 && !have_ipv6) {
			continue;
		}

		isc_buffer_init(&b, tmp, sizeof(tmp));
		isc_netaddr_fromsockaddr(&na, sa);
		result = isc_netaddr_totext(&na, &b);
		if (result != ISC_R_SUCCESS) {
			continue;
		}
		isc_buffer_putuint8(&b, 0);
		if (pf == AF_INET6 && na.zone != 0) {
			char buf[sizeof("%4000000000")];
			snprintf(buf, sizeof(buf), "%%%u", na.zone);
			strlcat(tmp, buf, sizeof(tmp));
		}
		newsrv = make_server(tmp, tmp);
		ISC_LINK_INIT(newsrv, link);
		ISC_LIST_APPEND(server_list, newsrv, link);
	}
}

void
flush_server_list(void) {
	dig_server_t *s, *ps;

	debug("flush_server_list()");
	s = ISC_LIST_HEAD(server_list);
	while (s != NULL) {
		ps = s;
		s = ISC_LIST_NEXT(s, link);
		ISC_LIST_DEQUEUE(server_list, ps, link);
		isc_mem_free(mctx, ps);
	}
}

void
set_nameserver(char *opt) {
	isc_result_t result;
	isc_sockaddr_t sockaddrs[DIG_MAX_ADDRESSES];
	isc_netaddr_t netaddr;
	int count, i;
	dig_server_t *srv;
	char tmp[ISC_NETADDR_FORMATSIZE];

	if (opt == NULL) {
		return;
	}

	result = bind9_getaddresses(opt, 0, sockaddrs, DIG_MAX_ADDRESSES,
				    &count);
	if (result != ISC_R_SUCCESS) {
		fatal("couldn't get address for '%s': %s", opt,
		      isc_result_totext(result));
	}

	flush_server_list();

	for (i = 0; i < count; i++) {
		isc_netaddr_fromsockaddr(&netaddr, &sockaddrs[i]);
		isc_netaddr_format(&netaddr, tmp, sizeof(tmp));
		srv = make_server(tmp, opt);
		if (srv == NULL) {
			fatal("memory allocation failure");
		}
		ISC_LIST_APPEND(server_list, srv, link);
	}
}

/*%
 * Produce a cloned server list.  The dest list must have already had
 * ISC_LIST_INIT applied.
 */
void
clone_server_list(dig_serverlist_t src, dig_serverlist_t *dest) {
	dig_server_t *srv, *newsrv;

	debug("clone_server_list()");
	srv = ISC_LIST_HEAD(src);
	while (srv != NULL) {
		newsrv = make_server(srv->servername, srv->userarg);
		ISC_LINK_INIT(newsrv, link);
		ISC_LIST_ENQUEUE(*dest, newsrv, link);
		srv = ISC_LIST_NEXT(srv, link);
	}
}

/*%
 * Create an empty lookup structure, which holds all the information needed
 * to get an answer to a user's question.  This structure contains two
 * linked lists: the server list (servers to query) and the query list
 * (outstanding queries which have been made to the listed servers).
 */
dig_lookup_t *
make_empty_lookup(void) {
	dig_lookup_t *looknew;
#ifdef HAVE_LIBIDN2
	bool idn_allowed = isatty(1) ? (getenv("IDN_DISABLE") == NULL) : false;
#endif /* HAVE_LIBIDN2 */

	debug("make_empty_lookup()");

	INSIST(!free_now);

	looknew = isc_mem_allocate(mctx, sizeof(*looknew));
	*looknew = (dig_lookup_t){
		.pending = true,
		.rdtype = dns_rdatatype_a,
		.qrdtype = dns_rdatatype_a,
		.rdclass = dns_rdataclass_in,
		.servfail_stops = true,
		.besteffort = true,
		.opcode = dns_opcode_query,
		.badcookie = true,
#ifdef HAVE_LIBIDN2
		.idnin = idn_allowed,
		.idnout = idn_allowed,
#endif /* HAVE_LIBIDN2 */
		.udpsize = -1,
		.edns = -1,
		.recurse = true,
		.retries = tries,
		.comments = true,
		.stats = true,
		.section_question = true,
		.section_answer = true,
		.section_authority = true,
		.section_additional = true,
		.ednsneg = true,
	};

	dns_fixedname_init(&looknew->fdomain);
	ISC_LINK_INIT(looknew, link);
	ISC_LIST_INIT(looknew->q);
	ISC_LIST_INIT(looknew->my_server_list);

	isc_tlsctx_cache_create(mctx, &looknew->tls_ctx_cache);

	isc_refcount_init(&looknew->references, 1);

	looknew->magic = DIG_LOOKUP_MAGIC;

	debug("make_empty_lookup() = %p->references = %" PRIuFAST32, looknew,
	      isc_refcount_current(&looknew->references));

	return (looknew);
}

#define EDNSOPT_OPTIONS 100U

static void
cloneopts(dig_lookup_t *looknew, dig_lookup_t *lookold) {
	size_t len = sizeof(looknew->ednsopts[0]) * EDNSOPT_OPTIONS;
	size_t i;
	looknew->ednsopts = isc_mem_allocate(mctx, len);
	for (i = 0; i < EDNSOPT_OPTIONS; i++) {
		looknew->ednsopts[i].code = 0;
		looknew->ednsopts[i].length = 0;
		looknew->ednsopts[i].value = NULL;
	}
	looknew->ednsoptscnt = 0;
	if (lookold == NULL || lookold->ednsopts == NULL) {
		return;
	}

	for (i = 0; i < lookold->ednsoptscnt; i++) {
		len = lookold->ednsopts[i].length;
		if (len != 0) {
			INSIST(lookold->ednsopts[i].value != NULL);
			looknew->ednsopts[i].value = isc_mem_allocate(mctx,
								      len);
			memmove(looknew->ednsopts[i].value,
				lookold->ednsopts[i].value, len);
		}
		looknew->ednsopts[i].code = lookold->ednsopts[i].code;
		looknew->ednsopts[i].length = len;
	}
	looknew->ednsoptscnt = lookold->ednsoptscnt;
}

/*%
 * Clone a lookup, perhaps copying the server list.  This does not clone
 * the query list, since it will be regenerated by the setup_lookup()
 * function, nor does it queue up the new lookup for processing.
 * Caution: If you don't clone the servers, you MUST clone the server
 * list separately from somewhere else, or construct it by hand.
 */
dig_lookup_t *
clone_lookup(dig_lookup_t *lookold, bool servers) {
	dig_lookup_t *looknew;

	debug("clone_lookup()");

	INSIST(!free_now);

	looknew = make_empty_lookup();
	strlcpy(looknew->textname, lookold->textname, MXNAME);
	strlcpy(looknew->cmdline, lookold->cmdline, MXNAME);
	looknew->textname[MXNAME - 1] = 0;
	looknew->rdtype = lookold->rdtype;
	looknew->qrdtype = lookold->qrdtype;
	looknew->rdclass = lookold->rdclass;
	looknew->rdtypeset = lookold->rdtypeset;
	looknew->rdclassset = lookold->rdclassset;
	looknew->doing_xfr = lookold->doing_xfr;
	looknew->ixfr_serial = lookold->ixfr_serial;
	looknew->trace = lookold->trace;
	looknew->trace_root = lookold->trace_root;
	looknew->identify = lookold->identify;
	looknew->identify_previous_line = lookold->identify_previous_line;
	looknew->ignore = lookold->ignore;
	looknew->servfail_stops = lookold->servfail_stops;
	looknew->besteffort = lookold->besteffort;
	looknew->dns64prefix = lookold->dns64prefix;
	looknew->dnssec = lookold->dnssec;
	looknew->ednsflags = lookold->ednsflags;
	looknew->opcode = lookold->opcode;
	looknew->expire = lookold->expire;
	looknew->nsid = lookold->nsid;
	looknew->tcp_keepalive = lookold->tcp_keepalive;
	looknew->header_only = lookold->header_only;
	looknew->https_mode = lookold->https_mode;
	if (lookold->https_path != NULL) {
		looknew->https_path = isc_mem_strdup(mctx, lookold->https_path);
	}
	looknew->https_get = lookold->https_get;
	looknew->http_plain = lookold->http_plain;

	looknew->tls_ca_set = lookold->tls_ca_set;
	if (lookold->tls_ca_file != NULL) {
		looknew->tls_ca_file = isc_mem_strdup(mctx,
						      lookold->tls_ca_file);
	};

	looknew->tls_hostname_set = lookold->tls_hostname_set;
	if (lookold->tls_hostname != NULL) {
		looknew->tls_hostname = isc_mem_strdup(mctx,
						       lookold->tls_hostname);
	}

	looknew->tls_key_file_set = lookold->tls_key_file_set;
	if (lookold->tls_key_file != NULL) {
		looknew->tls_key_file = isc_mem_strdup(mctx,
						       lookold->tls_key_file);
	}

	looknew->tls_cert_file_set = lookold->tls_cert_file_set;
	if (lookold->tls_cert_file != NULL) {
		looknew->tls_cert_file = isc_mem_strdup(mctx,
							lookold->tls_cert_file);
	}

	looknew->showbadcookie = lookold->showbadcookie;
	looknew->sendcookie = lookold->sendcookie;
	looknew->seenbadcookie = lookold->seenbadcookie;
	looknew->badcookie = lookold->badcookie;
	looknew->cookie = lookold->cookie;
	if (lookold->ednsopts != NULL) {
		cloneopts(looknew, lookold);
	} else {
		looknew->ednsopts = NULL;
		looknew->ednsoptscnt = 0;
	}
	looknew->ednsneg = lookold->ednsneg;
	looknew->padding = lookold->padding;
	looknew->multiline = lookold->multiline;
	looknew->nottl = lookold->nottl;
	looknew->noclass = lookold->noclass;
	looknew->onesoa = lookold->onesoa;
	looknew->use_usec = lookold->use_usec;
	looknew->nocrypto = lookold->nocrypto;
	looknew->ttlunits = lookold->ttlunits;
	looknew->expandaaaa = lookold->expandaaaa;
	looknew->qr = lookold->qr;
	looknew->idnin = lookold->idnin;
	looknew->idnout = lookold->idnout;
	looknew->udpsize = lookold->udpsize;
	looknew->edns = lookold->edns;
	looknew->recurse = lookold->recurse;
	looknew->aaonly = lookold->aaonly;
	looknew->adflag = lookold->adflag;
	looknew->cdflag = lookold->cdflag;
	looknew->raflag = lookold->raflag;
	looknew->tcflag = lookold->tcflag;
	looknew->print_unknown_format = lookold->print_unknown_format;
	looknew->zflag = lookold->zflag;
	looknew->setqid = lookold->setqid;
	looknew->qid = lookold->qid;
	looknew->ns_search_only = lookold->ns_search_only;
	looknew->tcp_mode = lookold->tcp_mode;
	looknew->tcp_mode_set = lookold->tcp_mode_set;
	looknew->tls_mode = lookold->tls_mode;
	looknew->comments = lookold->comments;
	looknew->stats = lookold->stats;
	looknew->section_question = lookold->section_question;
	looknew->section_answer = lookold->section_answer;
	looknew->section_authority = lookold->section_authority;
	looknew->section_additional = lookold->section_additional;
	looknew->origin = lookold->origin;
	looknew->retries = lookold->retries;
	looknew->tsigctx = NULL;
	looknew->need_search = lookold->need_search;
	looknew->done_as_is = lookold->done_as_is;
	looknew->rrcomments = lookold->rrcomments;
	looknew->fuzzing = lookold->fuzzing;
	looknew->fuzztime = lookold->fuzztime;

	if (lookold->ecs_addr != NULL) {
		size_t len = sizeof(isc_sockaddr_t);
		looknew->ecs_addr = isc_mem_allocate(mctx, len);
		memmove(looknew->ecs_addr, lookold->ecs_addr, len);
	}

	dns_name_copy(dns_fixedname_name(&lookold->fdomain),
		      dns_fixedname_name(&looknew->fdomain));

	if (servers) {
		if (lookold->tls_ctx_cache != NULL) {
			isc_tlsctx_cache_detach(&looknew->tls_ctx_cache);
			isc_tlsctx_cache_attach(lookold->tls_ctx_cache,
						&looknew->tls_ctx_cache);
		}
		clone_server_list(lookold->my_server_list,
				  &looknew->my_server_list);
	}

	isc_refcount_init(&looknew->references, 1);

	looknew->magic = DIG_LOOKUP_MAGIC;

	return (looknew);
}

/*%
 * Requeue a lookup for further processing, perhaps copying the server
 * list.  The new lookup structure is returned to the caller, and is
 * queued for processing.  If servers are not cloned in the requeue, they
 * must be added before allowing the current event to complete, since the
 * completion of the event may result in the next entry on the lookup
 * queue getting run.
 */
dig_lookup_t *
requeue_lookup(dig_lookup_t *lookold, bool servers) {
	dig_lookup_t *looknew = NULL;

	debug("requeue_lookup()");

	lookup_counter++;
	if (lookup_counter > LOOKUP_LIMIT) {
		fatal("too many lookups");
	}

	looknew = clone_lookup(lookold, servers);
	INSIST(looknew != NULL);

	debug("before insertion, init@%p -> %p, new@%p -> %p", lookold,
	      lookold->link.next, looknew, looknew->link.next);
	ISC_LIST_PREPEND(lookup_list, looknew, link);
	debug("after insertion, init -> %p, new = %p, new -> %p", lookold,
	      looknew, looknew->link.next);
	return (looknew);
}

void
setup_text_key(void) {
	isc_result_t result;
	dns_name_t keyname;
	isc_buffer_t secretbuf;
	unsigned int secretsize;
	unsigned char *secretstore;

	debug("setup_text_key()");
	isc_buffer_allocate(mctx, &namebuf, MXNAME);
	dns_name_init(&keyname, NULL);
	isc_buffer_putstr(namebuf, keynametext);
	secretsize = (unsigned int)strlen(keysecret) * 3 / 4;
	secretstore = isc_mem_allocate(mctx, secretsize);
	isc_buffer_init(&secretbuf, secretstore, secretsize);
	result = isc_base64_decodestring(keysecret, &secretbuf);
	if (result != ISC_R_SUCCESS) {
		goto failure;
	}

	secretsize = isc_buffer_usedlength(&secretbuf);

	if (hmacname == NULL) {
		result = DST_R_UNSUPPORTEDALG;
		goto failure;
	}

	result = dns_name_fromtext(&keyname, namebuf, dns_rootname, 0, namebuf);
	if (result != ISC_R_SUCCESS) {
		goto failure;
	}

	result = dns_tsigkey_create(&keyname, hmacname, secretstore,
				    (int)secretsize, false, NULL, 0, 0, mctx,
				    NULL, &tsigkey);
failure:
	if (result != ISC_R_SUCCESS) {
		printf(";; Couldn't create key %s: %s\n", keynametext,
		       isc_result_totext(result));
	} else {
		dst_key_setbits(tsigkey->key, digestbits);
	}

	isc_mem_free(mctx, secretstore);
	dns_name_invalidate(&keyname);
	isc_buffer_free(&namebuf);
}

static isc_result_t
parse_uint_helper(uint32_t *uip, const char *value, uint32_t max,
		  const char *desc, int base) {
	uint32_t n;
	isc_result_t result = isc_parse_uint32(&n, value, base);
	if (result == ISC_R_SUCCESS && n > max) {
		result = ISC_R_RANGE;
	}
	if (result != ISC_R_SUCCESS) {
		printf("invalid %s '%s': %s\n", desc, value,
		       isc_result_totext(result));
		return (result);
	}
	*uip = n;
	return (ISC_R_SUCCESS);
}

isc_result_t
parse_uint(uint32_t *uip, const char *value, uint32_t max, const char *desc) {
	return (parse_uint_helper(uip, value, max, desc, 10));
}

isc_result_t
parse_xint(uint32_t *uip, const char *value, uint32_t max, const char *desc) {
	return (parse_uint_helper(uip, value, max, desc, 0));
}

static uint32_t
parse_bits(char *arg, const char *desc, uint32_t max) {
	isc_result_t result;
	uint32_t tmp;

	result = parse_uint(&tmp, arg, max, desc);
	if (result != ISC_R_SUCCESS) {
		fatal("couldn't parse digest bits");
	}
	tmp = (tmp + 7) & ~0x7U;
	return (tmp);
}

isc_result_t
parse_netprefix(isc_sockaddr_t **sap, const char *value) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_sockaddr_t *sa = NULL;
	struct in_addr in4;
	struct in6_addr in6;
	uint32_t prefix_length = 0xffffffff;
	char *slash = NULL;
	bool parsed = false;
	bool prefix_parsed = false;
	char buf[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:XXX.XXX.XXX.XXX/128")];

	REQUIRE(sap != NULL && *sap == NULL);

	if (strlcpy(buf, value, sizeof(buf)) >= sizeof(buf)) {
		fatal("invalid prefix '%s'\n", value);
	}

	sa = isc_mem_allocate(mctx, sizeof(*sa));
	memset(sa, 0, sizeof(*sa));

	if (strcmp(buf, "0") == 0) {
		sa->type.sa.sa_family = AF_UNSPEC;
		prefix_length = 0;
		goto done;
	}

	slash = strchr(buf, '/');
	if (slash != NULL) {
		*slash = '\0';
		result = isc_parse_uint32(&prefix_length, slash + 1, 10);
		if (result != ISC_R_SUCCESS) {
			fatal("invalid prefix length in '%s': %s\n", value,
			      isc_result_totext(result));
		}
		prefix_parsed = true;
	}

	if (inet_pton(AF_INET6, buf, &in6) == 1) {
		parsed = true;
		isc_sockaddr_fromin6(sa, &in6, 0);
		if (prefix_length > 128) {
			prefix_length = 128;
		}
	} else if (inet_pton(AF_INET, buf, &in4) == 1) {
		parsed = true;
		isc_sockaddr_fromin(sa, &in4, 0);
		if (prefix_length > 32) {
			prefix_length = 32;
		}
	} else if (prefix_parsed) {
		int i;

		for (i = 0; i < 3 && strlen(buf) < sizeof(buf) - 2; i++) {
			strlcat(buf, ".0", sizeof(buf));
			if (inet_pton(AF_INET, buf, &in4) == 1) {
				parsed = true;
				isc_sockaddr_fromin(sa, &in4, 0);
				break;
			}
		}

		if (prefix_length > 32) {
			prefix_length = 32;
		}
	}

	if (!parsed) {
		fatal("invalid address '%s'", value);
	}

done:
	sa->length = prefix_length;
	*sap = sa;

	return (ISC_R_SUCCESS);
}

/*
 * Parse HMAC algorithm specification
 */
void
parse_hmac(const char *hmac) {
	char buf[20];
	size_t len;

	REQUIRE(hmac != NULL);

	len = strlen(hmac);
	if (len >= sizeof(buf)) {
		fatal("unknown key type '%.*s'", (int)len, hmac);
	}
	strlcpy(buf, hmac, sizeof(buf));

	digestbits = 0;

	if (strcasecmp(buf, "hmac-md5") == 0) {
		hmacname = DNS_TSIG_HMACMD5_NAME;
	} else if (strncasecmp(buf, "hmac-md5-", 9) == 0) {
		hmacname = DNS_TSIG_HMACMD5_NAME;
		digestbits = parse_bits(&buf[9], "digest-bits [0..128]", 128);
	} else if (strcasecmp(buf, "hmac-sha1") == 0) {
		hmacname = DNS_TSIG_HMACSHA1_NAME;
		digestbits = 0;
	} else if (strncasecmp(buf, "hmac-sha1-", 10) == 0) {
		hmacname = DNS_TSIG_HMACSHA1_NAME;
		digestbits = parse_bits(&buf[10], "digest-bits [0..160]", 160);
	} else if (strcasecmp(buf, "hmac-sha224") == 0) {
		hmacname = DNS_TSIG_HMACSHA224_NAME;
	} else if (strncasecmp(buf, "hmac-sha224-", 12) == 0) {
		hmacname = DNS_TSIG_HMACSHA224_NAME;
		digestbits = parse_bits(&buf[12], "digest-bits [0..224]", 224);
	} else if (strcasecmp(buf, "hmac-sha256") == 0) {
		hmacname = DNS_TSIG_HMACSHA256_NAME;
	} else if (strncasecmp(buf, "hmac-sha256-", 12) == 0) {
		hmacname = DNS_TSIG_HMACSHA256_NAME;
		digestbits = parse_bits(&buf[12], "digest-bits [0..256]", 256);
	} else if (strcasecmp(buf, "hmac-sha384") == 0) {
		hmacname = DNS_TSIG_HMACSHA384_NAME;
	} else if (strncasecmp(buf, "hmac-sha384-", 12) == 0) {
		hmacname = DNS_TSIG_HMACSHA384_NAME;
		digestbits = parse_bits(&buf[12], "digest-bits [0..384]", 384);
	} else if (strcasecmp(buf, "hmac-sha512") == 0) {
		hmacname = DNS_TSIG_HMACSHA512_NAME;
	} else if (strncasecmp(buf, "hmac-sha512-", 12) == 0) {
		hmacname = DNS_TSIG_HMACSHA512_NAME;
		digestbits = parse_bits(&buf[12], "digest-bits [0..512]", 512);
	} else {
		fprintf(stderr,
			";; Warning, ignoring "
			"invalid TSIG algorithm %s\n",
			buf);
	}
}

/*
 * Get a key from a named.conf format keyfile
 */
static isc_result_t
read_confkey(void) {
	cfg_parser_t *pctx = NULL;
	cfg_obj_t *file = NULL;
	const cfg_obj_t *keyobj = NULL;
	const cfg_obj_t *secretobj = NULL;
	const cfg_obj_t *algorithmobj = NULL;
	const char *keyname;
	const char *secretstr;
	const char *algorithm;
	isc_result_t result;

	if (!isc_file_exists(keyfile)) {
		return (ISC_R_FILENOTFOUND);
	}

	result = cfg_parser_create(mctx, NULL, &pctx);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = cfg_parse_file(pctx, keyfile, &cfg_type_sessionkey, &file);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = cfg_map_get(file, "key", &keyobj);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	(void)cfg_map_get(keyobj, "secret", &secretobj);
	(void)cfg_map_get(keyobj, "algorithm", &algorithmobj);
	if (secretobj == NULL || algorithmobj == NULL) {
		fatal("key must have algorithm and secret");
	}

	keyname = cfg_obj_asstring(cfg_map_getname(keyobj));
	secretstr = cfg_obj_asstring(secretobj);
	algorithm = cfg_obj_asstring(algorithmobj);

	strlcpy(keynametext, keyname, sizeof(keynametext));
	strlcpy(keysecret, secretstr, sizeof(keysecret));
	parse_hmac(algorithm);
	setup_text_key();

cleanup:
	if (pctx != NULL) {
		if (file != NULL) {
			cfg_obj_destroy(pctx, &file);
		}
		cfg_parser_destroy(&pctx);
	}

	return (result);
}

void
setup_file_key(void) {
	isc_result_t result;
	dst_key_t *dstkey = NULL;

	debug("setup_file_key()");

	if (sig0key != NULL) {
		dst_key_free(&sig0key);
	}

	/* Try reading the key from a K* pair */
	result = dst_key_fromnamedfile(
		keyfile, NULL, DST_TYPE_PRIVATE | DST_TYPE_KEY, mctx, &dstkey);

	/* If that didn't work, try reading it as a session.key keyfile */
	if (result != ISC_R_SUCCESS) {
		result = read_confkey();
		if (result == ISC_R_SUCCESS) {
			return;
		}
	}

	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "Couldn't read key from %s: %s\n", keyfile,
			isc_result_totext(result));
		goto failure;
	}

	switch (dst_key_alg(dstkey)) {
	case DST_ALG_HMACMD5:
		hmacname = DNS_TSIG_HMACMD5_NAME;
		break;
	case DST_ALG_HMACSHA1:
		hmacname = DNS_TSIG_HMACSHA1_NAME;
		break;
	case DST_ALG_HMACSHA224:
		hmacname = DNS_TSIG_HMACSHA224_NAME;
		break;
	case DST_ALG_HMACSHA256:
		hmacname = DNS_TSIG_HMACSHA256_NAME;
		break;
	case DST_ALG_HMACSHA384:
		hmacname = DNS_TSIG_HMACSHA384_NAME;
		break;
	case DST_ALG_HMACSHA512:
		hmacname = DNS_TSIG_HMACSHA512_NAME;
		break;
	}

	if (hmacname != NULL) {
		result = dns_tsigkey_createfromkey(
			dst_key_name(dstkey), hmacname, dstkey, false, NULL, 0,
			0, mctx, NULL, &tsigkey);
		if (result != ISC_R_SUCCESS) {
			printf(";; Couldn't create key %s: %s\n", keynametext,
			       isc_result_totext(result));
			goto failure;
		}
	} else {
		dst_key_attach(dstkey, &sig0key);
		dst_key_free(&dstkey);
	}
failure:
	if (dstkey != NULL) {
		dst_key_free(&dstkey);
	}
}

static dig_searchlist_t *
make_searchlist_entry(char *domain) {
	dig_searchlist_t *search;
	search = isc_mem_allocate(mctx, sizeof(*search));
	strlcpy(search->origin, domain, MXNAME);
	search->origin[MXNAME - 1] = 0;
	ISC_LINK_INIT(search, link);
	return (search);
}

static void
clear_searchlist(void) {
	dig_searchlist_t *search;
	while ((search = ISC_LIST_HEAD(search_list)) != NULL) {
		ISC_LIST_UNLINK(search_list, search, link);
		isc_mem_free(mctx, search);
	}
}

static void
create_search_list(irs_resconf_t *resconf) {
	irs_resconf_searchlist_t *list;
	irs_resconf_search_t *entry;
	dig_searchlist_t *search;

	debug("create_search_list()");
	clear_searchlist();

	list = irs_resconf_getsearchlist(resconf);
	for (entry = ISC_LIST_HEAD(*list); entry != NULL;
	     entry = ISC_LIST_NEXT(entry, link))
	{
		search = make_searchlist_entry(entry->domain);
		ISC_LIST_APPEND(search_list, search, link);
	}
}

/*%
 * Append 'addr' to the list of servers to be queried.  This function is only
 * called when no server addresses are explicitly specified and either libirs
 * returns an empty list of servers to use or none of the addresses returned by
 * libirs are usable due to the specified address family restrictions.
 */
static void
add_fallback_nameserver(const char *addr) {
	dig_server_t *server = make_server(addr, addr);
	ISC_LINK_INIT(server, link);
	ISC_LIST_APPEND(server_list, server, link);
}

/*%
 * Setup the system as a whole, reading key information and resolv.conf
 * settings.
 */
void
setup_system(bool ipv4only, bool ipv6only) {
	irs_resconf_t *resconf = NULL;
	isc_result_t result;

	debug("setup_system()");

	if (ipv4only) {
		if (have_ipv4) {
			isc_net_disableipv6();
			have_ipv6 = false;
		} else {
			fatal("can't find IPv4 networking");
		}
	}

	if (ipv6only) {
		if (have_ipv6) {
			isc_net_disableipv4();
			have_ipv4 = false;
		} else {
			fatal("can't find IPv6 networking");
		}
	}

	result = irs_resconf_load(mctx, RESOLV_CONF, &resconf);
	if (result != ISC_R_SUCCESS && result != ISC_R_FILENOTFOUND) {
		fatal("parse of %s failed", RESOLV_CONF);
	}

	create_search_list(resconf);
	if (ndots == -1) {
		ndots = irs_resconf_getndots(resconf);
		debug("ndots is %d.", ndots);
	}
	if (timeout == 0) {
		timeout = irs_resconf_gettimeout(resconf);
		debug("timeout is %d.", timeout);
	}
	if (tries == -1) {
		tries = irs_resconf_getattempts(resconf);
		if (tries == 0) {
			tries = 3;
		}
		debug("retries is %d.", tries);
	}

	/* If user doesn't specify server use nameservers from resolv.conf. */
	if (ISC_LIST_EMPTY(server_list)) {
		get_server_list(resconf);
	}

	/* If we don't find a nameserver fall back to localhost */
	if (ISC_LIST_EMPTY(server_list)) {
		if (have_ipv6) {
			add_fallback_nameserver("::1");
		}
		if (have_ipv4) {
			add_fallback_nameserver("127.0.0.1");
		}
	}

	irs_resconf_destroy(&resconf);

	if (keyfile[0] != 0) {
		setup_file_key();
	} else if (keysecret[0] != 0) {
		setup_text_key();
	}

	isc_nonce_buf(cookie_secret, sizeof(cookie_secret));
}

/*%
 * Override the search list derived from resolv.conf by 'domain'.
 */
void
set_search_domain(char *domain) {
	dig_searchlist_t *search;

	clear_searchlist();
	search = make_searchlist_entry(domain);
	ISC_LIST_APPEND(search_list, search, link);
}

/*%
 * Setup the ISC and DNS libraries for use by the system.
 */
void
setup_libs(void) {
	isc_result_t result;
	isc_logconfig_t *logconfig = NULL;

	debug("setup_libs()");

	result = isc_net_probeipv4();
	if (result == ISC_R_SUCCESS) {
		have_ipv4 = true;
	}

	result = isc_net_probeipv6();
	if (result == ISC_R_SUCCESS) {
		have_ipv6 = true;
	}
	if (!have_ipv6 && !have_ipv4) {
		fatal("can't find either v4 or v6 networking");
	}

	isc_mem_create(&mctx);
	isc_mem_setname(mctx, "dig");

	isc_log_create(mctx, &lctx, &logconfig);
	isc_log_setcontext(lctx);
	dns_log_init(lctx);
	dns_log_setcontext(lctx);

	result = isc_log_usechannel(logconfig, "default_debug", NULL, NULL);
	check_result(result, "isc_log_usechannel");

	isc_log_setdebuglevel(lctx, 0);

	isc_managers_create(mctx, 1, 0, &netmgr, &taskmgr, NULL);

	result = isc_task_create(taskmgr, 0, &global_task);
	check_result(result, "isc_task_create");
	isc_task_setname(global_task, "dig", NULL);

	result = dst_lib_init(mctx, NULL);
	check_result(result, "dst_lib_init");
	is_dst_up = true;

	isc_mutex_init(&lookup_lock);
}

typedef struct dig_ednsoptname {
	uint32_t code;
	const char *name;
} dig_ednsoptname_t;

dig_ednsoptname_t optnames[] = {
	{ 1, "LLQ" },	       /* draft-sekar-dns-llq */
	{ 3, "NSID" },	       /* RFC 5001 */
	{ 5, "DAU" },	       /* RFC 6975 */
	{ 6, "DHU" },	       /* RFC 6975 */
	{ 7, "N3U" },	       /* RFC 6975 */
	{ 8, "ECS" },	       /* RFC 7871 */
	{ 9, "EXPIRE" },       /* RFC 7314 */
	{ 10, "COOKIE" },      /* RFC 7873 */
	{ 11, "KEEPALIVE" },   /* RFC 7828 */
	{ 12, "PADDING" },     /* RFC 7830 */
	{ 12, "PAD" },	       /* shorthand */
	{ 13, "CHAIN" },       /* RFC 7901 */
	{ 14, "KEY-TAG" },     /* RFC 8145 */
	{ 15, "EDE" },	       /* ietf-dnsop-extended-error-16 */
	{ 16, "CLIENT-TAG" },  /* draft-bellis-dnsop-edns-tags */
	{ 17, "SERVER-TAG" },  /* draft-bellis-dnsop-edns-tags */
	{ 26946, "DEVICEID" }, /* Brian Hartvigsen */
};

#define N_EDNS_OPTNAMES (sizeof(optnames) / sizeof(optnames[0]))

void
save_opt(dig_lookup_t *lookup, char *code, char *value) {
	isc_result_t result;
	uint32_t num = 0;
	isc_buffer_t b;
	bool found = false;
	unsigned int i;

	if (lookup->ednsoptscnt >= EDNSOPT_OPTIONS) {
		fatal("too many ednsopts");
	}

	for (i = 0; i < N_EDNS_OPTNAMES; i++) {
		if (strcasecmp(code, optnames[i].name) == 0) {
			num = optnames[i].code;
			found = true;
			break;
		}
	}

	if (!found) {
		result = parse_uint(&num, code, 65535, "ednsopt");
		if (result != ISC_R_SUCCESS) {
			fatal("bad edns code point: %s", code);
		}
	}

	if (lookup->ednsopts == NULL) {
		cloneopts(lookup, NULL);
	}
	INSIST(lookup->ednsopts != NULL);

	if (lookup->ednsopts[lookup->ednsoptscnt].value != NULL) {
		isc_mem_free(mctx, lookup->ednsopts[lookup->ednsoptscnt].value);
	}

	lookup->ednsopts[lookup->ednsoptscnt].code = num;
	lookup->ednsopts[lookup->ednsoptscnt].length = 0;
	lookup->ednsopts[lookup->ednsoptscnt].value = NULL;

	if (value != NULL) {
		char *buf;
		buf = isc_mem_allocate(mctx, strlen(value) / 2 + 1);
		isc_buffer_init(&b, buf, (unsigned int)strlen(value) / 2 + 1);
		result = isc_hex_decodestring(value, &b);
		check_result(result, "isc_hex_decodestring");
		lookup->ednsopts[lookup->ednsoptscnt].value =
			isc_buffer_base(&b);
		lookup->ednsopts[lookup->ednsoptscnt].length =
			isc_buffer_usedlength(&b);
	}

	lookup->ednsoptscnt++;
}

/*%
 * Add EDNS0 option record to a message.  Currently, the only supported
 * options are UDP buffer size, the DO bit, and EDNS options
 * (e.g., NSID, COOKIE, client-subnet)
 */
static void
add_opt(dns_message_t *msg, uint16_t udpsize, uint16_t edns, unsigned int flags,
	dns_ednsopt_t *opts, size_t count) {
	dns_rdataset_t *rdataset = NULL;
	isc_result_t result;

	debug("add_opt()");
	result = dns_message_buildopt(msg, &rdataset, edns, udpsize, flags,
				      opts, count);
	check_result(result, "dns_message_buildopt");
	result = dns_message_setopt(msg, rdataset);
	check_result(result, "dns_message_setopt");
}

/*%
 * Add a question section to a message, asking for the specified name,
 * type, and class.
 */
static void
add_question(dns_message_t *message, dns_name_t *name, dns_rdataclass_t rdclass,
	     dns_rdatatype_t rdtype) {
	dns_rdataset_t *rdataset;
	isc_result_t result;

	debug("add_question()");
	rdataset = NULL;
	result = dns_message_gettemprdataset(message, &rdataset);
	check_result(result, "dns_message_gettemprdataset()");
	dns_rdataset_makequestion(rdataset, rdclass, rdtype);
	ISC_LIST_APPEND(name->list, rdataset, link);
}

/*%
 * Check if we're done with all the queued lookups, which is true iff
 * all sockets, sends, and recvs are accounted for (counters == 0),
 * and the lookup list is empty.
 * If we are done, pass control back out to dighost_shutdown() (which is
 * part of dig.c, host.c, or nslookup.c) to either shutdown the system as
 * a whole or reseed the lookup list.
 */
static void
check_if_done(void) {
	dig_lookup_t *lookup = NULL;

	debug("check_if_done()");
	debug("list %s", ISC_LIST_EMPTY(lookup_list) ? "empty" : "full");

	lookup = ISC_LIST_HEAD(lookup_list);
	while (lookup != NULL) {
		dig_lookup_t *next = NULL;
		debug("pending lookup %p", lookup);
		next = ISC_LIST_NEXT(lookup, link);
		lookup = next;
	}

	if (ISC_LIST_EMPTY(lookup_list) && current_lookup == NULL &&
	    isc_refcount_current(&sendcount) == 0)
	{
		INSIST(isc_refcount_current(&recvcount) == 0);
		debug("shutting down");
		dighost_shutdown();
	}
}

/*%
 * Check if we're done with all the queries in the lookup, except for
 * the `except_q` query (can be NULL if no exception is required).
 * Expects `l` to be a valid and locked lookup.
 */
static bool
check_if_queries_done(dig_lookup_t *l, dig_query_t *except_q) {
	dig_query_t *q = ISC_LIST_HEAD(l->q);

	debug("check_if_queries_done(%p)", l);

	while (q != NULL) {
		if (!q->started || isc_refcount_current(&q->references) > 1) {
			if (!q->canceled && q != except_q) {
				debug("there is a pending query %p", q);
				return (false);
			}
		}
		q = ISC_LIST_NEXT(q, link);
	}

	return (true);
}

static void
_destroy_lookup(dig_lookup_t *lookup) {
	dig_server_t *s;
	void *ptr;

	REQUIRE(lookup != NULL);
	REQUIRE(ISC_LIST_EMPTY(lookup->q));

	debug("destroy_lookup");

	isc_refcount_destroy(&lookup->references);

	s = ISC_LIST_HEAD(lookup->my_server_list);
	while (s != NULL) {
		debug("freeing server %p belonging to %p", s, lookup);
		ptr = s;
		s = ISC_LIST_NEXT(s, link);
		ISC_LIST_DEQUEUE(lookup->my_server_list, (dig_server_t *)ptr,
				 link);
		isc_mem_free(mctx, ptr);
	}
	if (lookup->sendmsg != NULL) {
		dns_message_detach(&lookup->sendmsg);
	}
	if (lookup->querysig != NULL) {
		debug("freeing buffer %p", lookup->querysig);
		isc_buffer_free(&lookup->querysig);
	}
	if (lookup->sendspace != NULL) {
		isc_mem_put(mctx, lookup->sendspace, COMMSIZE);
	}

	if (lookup->tsigctx != NULL) {
		dst_context_destroy(&lookup->tsigctx);
	}

	if (lookup->ecs_addr != NULL) {
		isc_mem_free(mctx, lookup->ecs_addr);
	}

	if (lookup->ednsopts != NULL) {
		size_t i;
		for (i = 0; i < EDNSOPT_OPTIONS; i++) {
			if (lookup->ednsopts[i].value != NULL) {
				isc_mem_free(mctx, lookup->ednsopts[i].value);
			}
		}
		isc_mem_free(mctx, lookup->ednsopts);
	}

	if (lookup->https_path) {
		isc_mem_free(mctx, lookup->https_path);
	}

	if (lookup->tls_ctx_cache != NULL) {
		isc_tlsctx_cache_detach(&lookup->tls_ctx_cache);
	}

	if (lookup->tls_ca_file != NULL) {
		isc_mem_free(mctx, lookup->tls_ca_file);
	}

	if (lookup->tls_hostname != NULL) {
		isc_mem_free(mctx, lookup->tls_hostname);
	}

	if (lookup->tls_key_file != NULL) {
		isc_mem_free(mctx, lookup->tls_key_file);
	}

	if (lookup->tls_cert_file != NULL) {
		isc_mem_free(mctx, lookup->tls_cert_file);
	}

	isc_mem_free(mctx, lookup);
}

#define lookup_attach(s, t) _lookup_attach(s, t, __FILE__, __LINE__)
static void
_lookup_attach(dig_lookup_t *lookup, dig_lookup_t **lookupp, const char *file,
	       unsigned int line) {
	REQUIRE(DIG_VALID_LOOKUP(lookup));
	REQUIRE(lookupp != NULL && *lookupp == NULL);

	debug("%s:%u:lookup_attach(%p) = %" PRIuFAST32, file, line, lookup,
	      isc_refcount_current(&lookup->references) + 1);

	(void)isc_refcount_increment(&lookup->references);

	*lookupp = lookup;
}

#define lookup_detach(l) _lookup_detach(l, __FILE__, __LINE__)
static void
_lookup_detach(dig_lookup_t **lookupp, const char *file, unsigned int line) {
	REQUIRE(DIG_VALID_LOOKUP(*lookupp));

	dig_lookup_t *lookup = *lookupp;
	*lookupp = NULL;

	debug("%s:%u:lookup_detach(%p) = %" PRIuFAST32, file, line, lookup,
	      isc_refcount_current(&lookup->references) - 1);

	if (isc_refcount_decrement(&lookup->references) == 1) {
		_destroy_lookup(lookup);
		if (lookup == current_lookup) {
			current_lookup = NULL;
			start_lookup();
		}
	}
}

void
destroy_lookup(dig_lookup_t *lookup) {
	REQUIRE(DIG_VALID_LOOKUP(lookup));

	REQUIRE(isc_refcount_decrement(&lookup->references) == 1);
	_destroy_lookup(lookup);
}

/*%
 * Destroy a query when we're done with it.  WARNING: This routine
 * WILL invalidate the query pointer.
 */
static void
destroy_query(dig_query_t *query, const char *file, unsigned int line) {
	debug("%s:%u:destroy_query(%p) = %" PRIuFAST32, file, line, query,
	      isc_refcount_current(&query->references));

	isc_refcount_destroy(&query->references);

	lookup_detach(&query->lookup);

	INSIST(query->recvspace != NULL);

	isc_mem_put(mctx, query->recvspace, COMMSIZE);
	isc_mem_put(mctx, query->tmpsendspace, COMMSIZE);

	query->magic = 0;
	isc_mem_free(mctx, query);
}

#define query_attach(s, t) _query_attach(s, t, __FILE__, __LINE__)

static void
_query_attach(dig_query_t *source, dig_query_t **targetp, const char *file,
	      unsigned int line) {
	REQUIRE(DIG_VALID_QUERY(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	debug("%s:%u:query_attach(%p) = %" PRIuFAST32, file, line, source,
	      isc_refcount_current(&source->references) + 1);

	(void)isc_refcount_increment(&source->references);

	*targetp = source;
}

#define query_detach(q) _query_detach(q, __FILE__, __LINE__)

static void
_query_detach(dig_query_t **queryp, const char *file, unsigned int line) {
	dig_query_t *query = NULL;
	dig_lookup_t *lookup = NULL;

	REQUIRE(DIG_VALID_QUERY(*queryp));

	query = *queryp;
	*queryp = NULL;

	lookup = query->lookup;

	if (lookup->current_query == query) {
		query_detach(&lookup->current_query);
	}

	debug("%s:%u:query_detach(%p) = %" PRIuFAST32, file, line, query,
	      isc_refcount_current(&query->references) - 1);

	if (isc_refcount_decrement(&query->references) == 1) {
		INSIST(query->readhandle == NULL);
		INSIST(query->sendhandle == NULL);

		if (ISC_LINK_LINKED(query, link)) {
			ISC_LIST_UNLINK(lookup->q, query, link);
		}
		destroy_query(query, file, line);
	}
}

/*%
 * If we can, start the next lookup in the queue running.
 * This assumes that the lookup on the head of the queue hasn't been
 * started yet.  It also removes the lookup from the head of the queue,
 * setting the current_lookup pointer pointing to it.
 */
void
start_lookup(void) {
	debug("start_lookup()");

	if (atomic_load(&cancel_now)) {
		return;
	}

	/*
	 * If there's a current lookup running, we really shouldn't get
	 * here.
	 */
	INSIST(current_lookup == NULL);

	current_lookup = ISC_LIST_HEAD(lookup_list);

	/*
	 * Put the current lookup somewhere so cancel_all can find it
	 */
	if (current_lookup != NULL) {
		/*
		 * Formally, we should attach the lookup to the current_lookup
		 * and detach it from the lookup_list, but it would be one
		 * attach and one detach.
		 */
		ISC_LIST_DEQUEUE(lookup_list, current_lookup, link);
		if (setup_lookup(current_lookup)) {
			do_lookup(current_lookup);
		} else if (next_origin(current_lookup)) {
			lookup_detach(&current_lookup);
			start_lookup();
		}
	} else {
		check_if_done();
	}
}

/*%
 * If we can, clear the current lookup and start the next one running.
 * (Note that while the reference count of current_lookup may be
 * decremented, current_lookup will not be set to NULL.)
 */
static void
clear_current_lookup(void) {
	dig_lookup_t *lookup = current_lookup;

	INSIST(!free_now);

	debug("clear_current_lookup()");

	if (lookup == NULL) {
		debug("current_lookup is already detached");
		return;
	}

	if (lookup->cleared) {
		debug("current_lookup is already cleared");
		return;
	}

	if (ISC_LIST_HEAD(lookup->q) != NULL) {
		debug("still have a worker");
		return;
	}

	lookup->cleared = true;
	debug("lookup cleared");

	lookup_detach(&lookup);
}

/*%
 * Create and queue a new lookup as a followup to the current lookup,
 * based on the supplied message and section.  This is used in trace and
 * name server search modes to start a new lookup using servers from
 * NS records in a reply. Returns the number of followup lookups made.
 */
static int
followup_lookup(dns_message_t *msg, dig_query_t *query, dns_section_t section) {
	dig_lookup_t *lookup = NULL;
	dig_server_t *srv = NULL;
	dns_rdataset_t *rdataset = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_name_t *name = NULL;
	isc_result_t result;
	bool success = false;
	int numLookups = 0;
	int num;
	isc_result_t lresult, addresses_result;
	char bad_namestr[DNS_NAME_FORMATSIZE];
	dns_name_t *domain;
	bool horizontal = false, bad = false;

	INSIST(!free_now);

	debug("following up %s", query->lookup->textname);

	addresses_result = ISC_R_SUCCESS;
	bad_namestr[0] = '\0';
	for (result = dns_message_firstname(msg, section);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(msg, section))
	{
		name = NULL;
		dns_message_currentname(msg, section, &name);

		if (section == DNS_SECTION_AUTHORITY) {
			rdataset = NULL;
			result = dns_message_findtype(name, dns_rdatatype_soa,
						      0, &rdataset);
			if (result == ISC_R_SUCCESS) {
				return (0);
			}
		}
		rdataset = NULL;
		result = dns_message_findtype(name, dns_rdatatype_ns, 0,
					      &rdataset);
		if (result != ISC_R_SUCCESS) {
			continue;
		}

		debug("found NS set");

		if (query->lookup->trace && !query->lookup->trace_root) {
			dns_namereln_t namereln;
			unsigned int nlabels;
			int order;

			domain = dns_fixedname_name(&query->lookup->fdomain);
			namereln = dns_name_fullcompare(name, domain, &order,
							&nlabels);
			if (namereln == dns_namereln_equal) {
				if (!horizontal) {
					dighost_warning("BAD (HORIZONTAL) "
							"REFERRAL");
				}
				horizontal = true;
			} else if (namereln != dns_namereln_subdomain) {
				if (!bad) {
					dighost_warning("BAD REFERRAL");
				}
				bad = true;
				continue;
			}
		}

		for (result = dns_rdataset_first(rdataset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(rdataset))
		{
			char namestr[DNS_NAME_FORMATSIZE];
			dns_rdata_ns_t ns;

			if (query->lookup->trace_root &&
			    query->lookup->nsfound >= MXSERV)
			{
				break;
			}

			dns_rdataset_current(rdataset, &rdata);

			query->lookup->nsfound++;
			result = dns_rdata_tostruct(&rdata, &ns, NULL);
			check_result(result, "dns_rdata_tostruct");
			dns_name_format(&ns.name, namestr, sizeof(namestr));
			dns_rdata_freestruct(&ns);

			/* Initialize lookup if we've not yet */
			debug("found NS %s", namestr);
			if (!success) {
				success = true;
				lookup_counter++;
				lookup = requeue_lookup(query->lookup, false);
				cancel_lookup(query->lookup);
				lookup->doing_xfr = false;
				if (!lookup->trace_root &&
				    section == DNS_SECTION_ANSWER)
				{
					lookup->trace = false;
				} else {
					lookup->trace = query->lookup->trace;
				}
				lookup->ns_search_only =
					query->lookup->ns_search_only;
				lookup->trace_root = false;
				if (lookup->ns_search_only) {
					lookup->recurse = false;
				}
				domain = dns_fixedname_name(&lookup->fdomain);
				dns_name_copy(name, domain);
			}
			debug("adding server %s", namestr);
			num = getaddresses(lookup, namestr, &lresult);
			if (lresult != ISC_R_SUCCESS) {
				printf("couldn't get address for '%s': %s\n",
				       namestr, isc_result_totext(lresult));
				if (addresses_result == ISC_R_SUCCESS) {
					addresses_result = lresult;
					strlcpy(bad_namestr, namestr,
						sizeof(bad_namestr));
				}
			}
			numLookups += num;
			dns_rdata_reset(&rdata);
		}
	}
	if (numLookups == 0 && addresses_result != ISC_R_SUCCESS) {
		fatal("couldn't get address for '%s': %s", bad_namestr,
		      isc_result_totext(result));
	}

	if (lookup == NULL && section == DNS_SECTION_ANSWER &&
	    (query->lookup->trace || query->lookup->ns_search_only))
	{
		return (followup_lookup(msg, query, DNS_SECTION_AUTHORITY));
	}

	/*
	 * Randomize the order the nameserver will be tried.
	 */
	if (numLookups > 1) {
		uint32_t i, j;
		dig_serverlist_t my_server_list;
		dig_server_t *next;

		ISC_LIST_INIT(my_server_list);

		i = numLookups;
		for (srv = ISC_LIST_HEAD(lookup->my_server_list); srv != NULL;
		     srv = ISC_LIST_HEAD(lookup->my_server_list))
		{
			INSIST(i > 0);
			j = isc_random_uniform(i);
			next = ISC_LIST_NEXT(srv, link);
			while (j-- > 0 && next != NULL) {
				srv = next;
				next = ISC_LIST_NEXT(srv, link);
			}
			ISC_LIST_DEQUEUE(lookup->my_server_list, srv, link);
			ISC_LIST_APPEND(my_server_list, srv, link);
			i--;
		}
		ISC_LIST_APPENDLIST(lookup->my_server_list, my_server_list,
				    link);
	}

	return (numLookups);
}

/*%
 * Create and queue a new lookup using the next origin from the search
 * list, read in setup_system().
 *
 * Return true iff there was another searchlist entry.
 */
static bool
next_origin(dig_lookup_t *oldlookup) {
	dig_lookup_t *newlookup;
	dig_searchlist_t *search;
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_result_t result;

	INSIST(!free_now);

	debug("next_origin()");
	debug("following up %s", oldlookup->textname);

	if (!usesearch) {
		/*
		 * We're not using a search list, so don't even think
		 * about finding the next entry.
		 */
		return (false);
	}

	/*
	 * Check for a absolute name or ndots being met.
	 */
	name = dns_fixedname_initname(&fixed);
	result = dns_name_fromstring2(name, oldlookup->textname, NULL, 0, NULL);
	if (result == ISC_R_SUCCESS &&
	    (dns_name_isabsolute(name) ||
	     (int)dns_name_countlabels(name) > ndots))
	{
		return (false);
	}

	if (oldlookup->origin == NULL && !oldlookup->need_search) {
		/*
		 * Then we just did rootorg; there's nothing left.
		 */
		return (false);
	}
	if (oldlookup->origin == NULL && oldlookup->need_search) {
		newlookup = requeue_lookup(oldlookup, true);
		newlookup->origin = ISC_LIST_HEAD(search_list);
		newlookup->need_search = false;
	} else {
		search = ISC_LIST_NEXT(oldlookup->origin, link);
		if (search == NULL && oldlookup->done_as_is) {
			return (false);
		}
		newlookup = requeue_lookup(oldlookup, true);
		newlookup->origin = search;
	}
	cancel_lookup(oldlookup);
	return (true);
}

/*%
 * Insert an SOA record into the sendmessage in a lookup.  Used for
 * creating IXFR queries.
 */
static void
insert_soa(dig_lookup_t *lookup) {
	isc_result_t result;
	dns_rdata_soa_t soa;
	dns_rdata_t *rdata = NULL;
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdataset_t *rdataset = NULL;
	dns_name_t *soaname = NULL;

	debug("insert_soa()");
	soa.mctx = mctx;
	soa.serial = lookup->ixfr_serial;
	soa.refresh = 0;
	soa.retry = 0;
	soa.expire = 0;
	soa.minimum = 0;
	soa.common.rdclass = lookup->rdclass;
	soa.common.rdtype = dns_rdatatype_soa;

	dns_name_init(&soa.origin, NULL);
	dns_name_init(&soa.contact, NULL);

	dns_name_clone(dns_rootname, &soa.origin);
	dns_name_clone(dns_rootname, &soa.contact);

	isc_buffer_init(&lookup->rdatabuf, lookup->rdatastore,
			sizeof(lookup->rdatastore));

	result = dns_message_gettemprdata(lookup->sendmsg, &rdata);
	check_result(result, "dns_message_gettemprdata");

	result = dns_rdata_fromstruct(rdata, lookup->rdclass, dns_rdatatype_soa,
				      &soa, &lookup->rdatabuf);
	check_result(result, "isc_rdata_fromstruct");

	result = dns_message_gettemprdatalist(lookup->sendmsg, &rdatalist);
	check_result(result, "dns_message_gettemprdatalist");

	result = dns_message_gettemprdataset(lookup->sendmsg, &rdataset);
	check_result(result, "dns_message_gettemprdataset");

	dns_rdatalist_init(rdatalist);
	rdatalist->type = dns_rdatatype_soa;
	rdatalist->rdclass = lookup->rdclass;
	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);

	dns_rdatalist_tordataset(rdatalist, rdataset);

	result = dns_message_gettempname(lookup->sendmsg, &soaname);
	check_result(result, "dns_message_gettempname");
	dns_name_clone(lookup->name, soaname);
	ISC_LIST_INIT(soaname->list);
	ISC_LIST_APPEND(soaname->list, rdataset, link);
	dns_message_addname(lookup->sendmsg, soaname, DNS_SECTION_AUTHORITY);
}

static void
compute_cookie(unsigned char *clientcookie, size_t len) {
	/* XXXMPA need to fix, should be per server. */
	INSIST(len >= 8U);
	memmove(clientcookie, cookie_secret, 8);
}

#define new_query(l, s, u) _new_query(l, s, u, __FILE__, __LINE__)

static dig_query_t *
_new_query(dig_lookup_t *lookup, char *servname, char *userarg,
	   const char *file, unsigned int line) {
	dig_query_t *query = NULL;

	query = isc_mem_allocate(mctx, sizeof(dig_query_t));
	debug("create query %p linked to lookup %p", query, lookup);
	*query = (dig_query_t){ .sendbuf = lookup->renderbuf,
				.servname = servname,
				.userarg = userarg,
				.warn_id = true,
				.recvspace = isc_mem_get(mctx, COMMSIZE),
				.tmpsendspace = isc_mem_get(mctx, COMMSIZE) };

	lookup_attach(lookup, &query->lookup);

	isc_refcount_init(&query->references, 1);

	debug("%s:%u:new_query(%p) = %" PRIuFAST32, file, line, query,
	      isc_refcount_current(&query->references));

	if (query->recvspace == NULL) {
		fatal("memory allocation failure");
	}
	if (query->tmpsendspace == NULL) {
		fatal("memory allocation failure");
	}

	isc_time_settoepoch(&query->time_sent);
	isc_time_settoepoch(&query->time_recv);

	ISC_LINK_INIT(query, clink);
	ISC_LINK_INIT(query, link);

	query->magic = DIG_QUERY_MAGIC;
	return (query);
}

/*%
 * Setup the supplied lookup structure, making it ready to start sending
 * queries to servers.  Create and initialize the message to be sent as
 * well as the query structures and buffer space for the replies.  If the
 * server list is empty, clone it from the system default list.
 */
bool
setup_lookup(dig_lookup_t *lookup) {
	isc_result_t result;
	unsigned int len;
	dig_server_t *serv;
	dig_query_t *query;
	isc_buffer_t b;
	dns_compress_t cctx;
	char store[MXNAME];
	char ecsbuf[20];
	char cookiebuf[256];
	char *origin = NULL;
	char *textname = NULL;

	REQUIRE(lookup != NULL);

#ifdef HAVE_LIBIDN2
	char idn_origin[MXNAME], idn_textname[MXNAME];
#endif /* HAVE_LIBIDN2 */

	INSIST(!free_now);

	debug("setup_lookup(%p)", lookup);

	dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER, &lookup->sendmsg);

	if (lookup->new_search) {
		debug("resetting lookup counter.");
		lookup_counter = 0;
	}

	if (ISC_LIST_EMPTY(lookup->my_server_list)) {
		debug("cloning server list");
		clone_server_list(server_list, &lookup->my_server_list);
	}
	result = dns_message_gettempname(lookup->sendmsg, &lookup->name);
	check_result(result, "dns_message_gettempname");

	isc_buffer_init(&lookup->namebuf, lookup->name_space,
			sizeof(lookup->name_space));
	isc_buffer_init(&lookup->onamebuf, lookup->oname_space,
			sizeof(lookup->oname_space));

	/*
	 * We cannot convert `textname' and `origin' separately.
	 * `textname' doesn't contain TLD, but local mapping needs
	 * TLD.
	 */
	textname = lookup->textname;
#ifdef HAVE_LIBIDN2
	if (lookup->idnin) {
		idn_locale_to_ace(textname, idn_textname, sizeof(idn_textname));
		debug("idn_textname: %s", idn_textname);
		textname = idn_textname;
	}
#endif /* HAVE_LIBIDN2 */

	/*
	 * If the name has too many dots, force the origin to be NULL
	 * (which produces an absolute lookup).  Otherwise, take the origin
	 * we have if there's one in the struct already.  If it's NULL,
	 * take the first entry in the searchlist iff either usesearch
	 * is TRUE or we got a domain line in the resolv.conf file.
	 */
	if (lookup->new_search) {
		if ((count_dots(textname) >= ndots) || !usesearch) {
			lookup->origin = NULL; /* Force abs lookup */
			lookup->done_as_is = true;
			lookup->need_search = usesearch;
		} else if (lookup->origin == NULL && usesearch) {
			lookup->origin = ISC_LIST_HEAD(search_list);
			lookup->need_search = false;
		}
	}

	if (lookup->origin != NULL) {
		debug("trying origin %s", lookup->origin->origin);
		result = dns_message_gettempname(lookup->sendmsg,
						 &lookup->oname);
		check_result(result, "dns_message_gettempname");
		/* XXX Helper funct to conv char* to name? */
		origin = lookup->origin->origin;
#ifdef HAVE_LIBIDN2
		if (lookup->idnin) {
			idn_locale_to_ace(origin, idn_origin,
					  sizeof(idn_origin));
			debug("trying idn origin %s", idn_origin);
			origin = idn_origin;
		}
#endif /* HAVE_LIBIDN2 */
		len = (unsigned int)strlen(origin);
		isc_buffer_init(&b, origin, len);
		isc_buffer_add(&b, len);
		result = dns_name_fromtext(lookup->oname, &b, dns_rootname, 0,
					   &lookup->onamebuf);
		if (result != ISC_R_SUCCESS) {
			dns_message_puttempname(lookup->sendmsg, &lookup->name);
			dns_message_puttempname(lookup->sendmsg,
						&lookup->oname);
			fatal("'%s' is not in legal name syntax (%s)", origin,
			      isc_result_totext(result));
		}
		if (lookup->trace && lookup->trace_root) {
			dns_name_clone(dns_rootname, lookup->name);
		} else {
			dns_fixedname_t fixed;
			dns_name_t *name;

			name = dns_fixedname_initname(&fixed);
			len = (unsigned int)strlen(textname);
			isc_buffer_init(&b, textname, len);
			isc_buffer_add(&b, len);
			result = dns_name_fromtext(name, &b, NULL, 0, NULL);
			if (result == ISC_R_SUCCESS) {
				if (!dns_name_isabsolute(name)) {
					result = dns_name_concatenate(
						name, lookup->oname,
						lookup->name, &lookup->namebuf);
				} else {
					dns_name_copy(name, lookup->name);
				}
			}
			if (result != ISC_R_SUCCESS) {
				dns_message_puttempname(lookup->sendmsg,
							&lookup->name);
				dns_message_puttempname(lookup->sendmsg,
							&lookup->oname);
				if (result == DNS_R_NAMETOOLONG) {
					return (false);
				}
				fatal("'%s' is not in legal name syntax (%s)",
				      lookup->textname,
				      isc_result_totext(result));
			}
		}
		dns_message_puttempname(lookup->sendmsg, &lookup->oname);
	} else {
		debug("using root origin");
		if (lookup->trace && lookup->trace_root) {
			dns_name_clone(dns_rootname, lookup->name);
		} else {
			len = (unsigned int)strlen(textname);
			isc_buffer_init(&b, textname, len);
			isc_buffer_add(&b, len);
			result = dns_name_fromtext(lookup->name, &b,
						   dns_rootname, 0,
						   &lookup->namebuf);
		}
		if (result != ISC_R_SUCCESS) {
			dns_message_puttempname(lookup->sendmsg, &lookup->name);
			warn("'%s' is not a legal name "
			     "(%s)",
			     lookup->textname, isc_result_totext(result));
#if TARGET_OS_IPHONE
			clear_current_lookup();
			return (false);
#else  /* if TARGET_OS_IPHONE */
			cleanup_openssl_refs();
			digexit();
#endif /* if TARGET_OS_IPHONE */
		}
	}
	dns_name_format(lookup->name, store, sizeof(store));
	dighost_trying(store, lookup);
	INSIST(dns_name_isabsolute(lookup->name));

	lookup->sendmsg->id = (dns_messageid_t)isc_random16();
	lookup->sendmsg->opcode = lookup->opcode;
	lookup->msgcounter = 0;

	/*
	 * If this is a trace request, completely disallow recursion after
	 * looking up the root name servers, since it's meaningless for traces.
	 */
	if ((lookup->trace || lookup->ns_search_only) && !lookup->trace_root) {
		lookup->recurse = false;
	}

	if (lookup->recurse && lookup->rdtype != dns_rdatatype_axfr &&
	    lookup->rdtype != dns_rdatatype_ixfr)
	{
		debug("recursive query");
		lookup->sendmsg->flags |= DNS_MESSAGEFLAG_RD;
	}

	/* XXX aaflag */
	if (lookup->aaonly) {
		debug("AA query");
		lookup->sendmsg->flags |= DNS_MESSAGEFLAG_AA;
	}

	if (lookup->adflag) {
		debug("AD query");
		lookup->sendmsg->flags |= DNS_MESSAGEFLAG_AD;
	}

	if (lookup->cdflag) {
		debug("CD query");
		lookup->sendmsg->flags |= DNS_MESSAGEFLAG_CD;
	}

	if (lookup->raflag) {
		debug("RA query");
		lookup->sendmsg->flags |= DNS_MESSAGEFLAG_RA;
	}

	if (lookup->tcflag) {
		debug("TC query");
		lookup->sendmsg->flags |= DNS_MESSAGEFLAG_TC;
	}

	if (lookup->zflag) {
		debug("Z query");
		lookup->sendmsg->flags |= 0x0040U;
	}

	if (lookup->setqid) {
		debug("set QID");
		lookup->sendmsg->id = lookup->qid;
	}

	dns_message_addname(lookup->sendmsg, lookup->name,
			    DNS_SECTION_QUESTION);

	if (lookup->trace && lookup->trace_root) {
		lookup->qrdtype = lookup->rdtype;
		lookup->rdtype = dns_rdatatype_ns;
	}

	if ((lookup->rdtype == dns_rdatatype_axfr) ||
	    (lookup->rdtype == dns_rdatatype_ixfr))
	{
		/*
		 * Force TCP mode if we're doing an axfr.
		 */
		if (lookup->rdtype == dns_rdatatype_axfr) {
			lookup->doing_xfr = true;
			lookup->tcp_mode = true;
		} else if (lookup->tcp_mode) {
			lookup->doing_xfr = true;
		}
	}

	if (!lookup->header_only) {
		add_question(lookup->sendmsg, lookup->name, lookup->rdclass,
			     lookup->rdtype);
	}

	/* add_soa */
	if (lookup->rdtype == dns_rdatatype_ixfr) {
		insert_soa(lookup);
	}

	/* XXX Insist this? */
	lookup->tsigctx = NULL;
	lookup->querysig = NULL;
	if (tsigkey != NULL) {
		debug("initializing keys");
		result = dns_message_settsigkey(lookup->sendmsg, tsigkey);
		check_result(result, "dns_message_settsigkey");
	} else if (sig0key != NULL) {
		debug("initializing keys");
		result = dns_message_setsig0key(lookup->sendmsg, sig0key);
		check_result(result, "dns_message_setsig0key");
	}

	if (lookup->fuzzing) {
		lookup->sendmsg->fuzzing = true;
		lookup->sendmsg->fuzztime = lookup->fuzztime;
	}

	lookup->sendspace = isc_mem_get(mctx, COMMSIZE);

	result = dns_compress_init(&cctx, -1, mctx);
	check_result(result, "dns_compress_init");

	debug("starting to render the message");
	isc_buffer_init(&lookup->renderbuf, lookup->sendspace, COMMSIZE);
	result = dns_message_renderbegin(lookup->sendmsg, &cctx,
					 &lookup->renderbuf);
	check_result(result, "dns_message_renderbegin");
	if (lookup->udpsize > -1 || lookup->dnssec || lookup->edns > -1 ||
	    lookup->ecs_addr != NULL)
	{
#define MAXOPTS (EDNSOPT_OPTIONS + DNS_EDNSOPTIONS)
		dns_ednsopt_t opts[MAXOPTS];
		unsigned int flags;
		unsigned int i = 0;

		/*
		 * There can't be more than MAXOPTS options to send:
		 * a maximum of EDNSOPT_OPTIONS set by +ednsopt
		 * and DNS_EDNSOPTIONS set by other arguments
		 * (+nsid, +cookie, etc).
		 */
		if (lookup->udpsize < 0) {
			lookup->udpsize = DEFAULT_EDNS_BUFSIZE;
		}
		if (lookup->edns < 0) {
			lookup->edns = DEFAULT_EDNS_VERSION;
		}

		if (lookup->nsid) {
			INSIST(i < MAXOPTS);
			opts[i].code = DNS_OPT_NSID;
			opts[i].length = 0;
			opts[i].value = NULL;
			i++;
		}

		if (lookup->ecs_addr != NULL) {
			uint8_t addr[16];
			uint16_t family = 0;
			uint32_t plen;
			struct sockaddr *sa;
			struct sockaddr_in *sin;
			struct sockaddr_in6 *sin6;
			size_t addrl;

			sa = &lookup->ecs_addr->type.sa;
			plen = lookup->ecs_addr->length;

			/* Round up prefix len to a multiple of 8 */
			addrl = (plen + 7) / 8;

			INSIST(i < MAXOPTS);
			opts[i].code = DNS_OPT_CLIENT_SUBNET;
			opts[i].length = (uint16_t)addrl + 4;
			check_result(result, "isc_buffer_allocate");

			/*
			 * XXXMUKS: According to RFC7871, "If there is
			 * no ADDRESS set, i.e., SOURCE PREFIX-LENGTH is
			 * set to 0, then FAMILY SHOULD be set to the
			 * transport over which the query is sent."
			 *
			 * However, at this point we don't know what
			 * transport(s) we'll be using, so we can't
			 * set the value now. For now, we're using
			 * IPv4 as the default the +subnet option
			 * used an IPv4 prefix, or for +subnet=0,
			 * and IPv6 if the +subnet option used an
			 * IPv6 prefix.
			 *
			 * (For future work: preserve the offset into
			 * the buffer where the family field is;
			 * that way we can update it in start_udp()
			 * or start_tcp() once we know
			 * what it outght to be.)
			 */
			switch (sa->sa_family) {
			case AF_UNSPEC:
				INSIST(plen == 0);
				family = 1;
				break;
			case AF_INET:
				INSIST(plen <= 32);
				family = 1;
				sin = (struct sockaddr_in *)sa;
				memmove(addr, &sin->sin_addr, addrl);
				break;
			case AF_INET6:
				INSIST(plen <= 128);
				family = 2;
				sin6 = (struct sockaddr_in6 *)sa;
				memmove(addr, &sin6->sin6_addr, addrl);
				break;
			default:
				UNREACHABLE();
			}

			isc_buffer_init(&b, ecsbuf, sizeof(ecsbuf));
			/* family */
			isc_buffer_putuint16(&b, family);
			/* source prefix-length */
			isc_buffer_putuint8(&b, plen);
			/* scope prefix-length */
			isc_buffer_putuint8(&b, 0);

			/* address */
			if (addrl > 0) {
				/* Mask off last address byte */
				if ((plen % 8) != 0) {
					addr[addrl - 1] &= ~0U
							   << (8 - (plen % 8));
				}
				isc_buffer_putmem(&b, addr, (unsigned)addrl);
			}

			opts[i].value = (uint8_t *)ecsbuf;
			i++;
		}

		if (lookup->sendcookie) {
			INSIST(i < MAXOPTS);
			opts[i].code = DNS_OPT_COOKIE;
			if (lookup->cookie != NULL) {
				isc_buffer_init(&b, cookiebuf,
						sizeof(cookiebuf));
				result = isc_hex_decodestring(lookup->cookie,
							      &b);
				check_result(result, "isc_hex_decodestring");
				opts[i].value = isc_buffer_base(&b);
				opts[i].length = isc_buffer_usedlength(&b);
			} else {
				compute_cookie(cookie, sizeof(cookie));
				opts[i].length = 8;
				opts[i].value = cookie;
			}
			i++;
		}

		if (lookup->expire) {
			INSIST(i < MAXOPTS);
			opts[i].code = DNS_OPT_EXPIRE;
			opts[i].length = 0;
			opts[i].value = NULL;
			i++;
		}

		if (lookup->tcp_keepalive) {
			INSIST(i < MAXOPTS);
			opts[i].code = DNS_OPT_TCP_KEEPALIVE;
			opts[i].length = 0;
			opts[i].value = NULL;
			i++;
		}

		if (lookup->ednsoptscnt != 0) {
			INSIST(i + lookup->ednsoptscnt <= MAXOPTS);
			memmove(&opts[i], lookup->ednsopts,
				sizeof(dns_ednsopt_t) * lookup->ednsoptscnt);
			i += lookup->ednsoptscnt;
		}

		if (lookup->padding != 0 && (i >= MAXOPTS)) {
			debug("turned off padding because of EDNS overflow");
			lookup->padding = 0;
		}

		if (lookup->padding != 0) {
			INSIST(i < MAXOPTS);
			opts[i].code = DNS_OPT_PAD;
			opts[i].length = 0;
			opts[i].value = NULL;
			i++;
			dns_message_setpadding(lookup->sendmsg,
					       lookup->padding);
		}

		flags = lookup->ednsflags;
		flags &= ~DNS_MESSAGEEXTFLAG_DO;
		if (lookup->dnssec) {
			flags |= DNS_MESSAGEEXTFLAG_DO;
		}
		add_opt(lookup->sendmsg, lookup->udpsize, lookup->edns, flags,
			opts, i);
	}

	result = dns_message_rendersection(lookup->sendmsg,
					   DNS_SECTION_QUESTION, 0);
	check_result(result, "dns_message_rendersection");
	result = dns_message_rendersection(lookup->sendmsg,
					   DNS_SECTION_AUTHORITY, 0);
	check_result(result, "dns_message_rendersection");
	result = dns_message_renderend(lookup->sendmsg);
	check_result(result, "dns_message_renderend");
	debug("done rendering");

	dns_compress_invalidate(&cctx);

	/*
	 * Force TCP mode if the request is larger than 512 bytes.
	 */
	if (isc_buffer_usedlength(&lookup->renderbuf) > 512) {
		lookup->tcp_mode = true;
	}

	lookup->pending = false;

	for (serv = ISC_LIST_HEAD(lookup->my_server_list); serv != NULL;
	     serv = ISC_LIST_NEXT(serv, link))
	{
		query = new_query(lookup, serv->servername, serv->userarg);
		ISC_LIST_ENQUEUE(lookup->q, query, link);
	}

	return (true);
}

/*%
 * NSSEARCH mode special mode handling function to start the next query in the
 * list. The lookup lock must be held by the caller. The function will detach
 * both the lookup and the query, and may cancel the lookup and clear the
 * current lookup.
 */
static void
nssearch_next(dig_lookup_t *l, dig_query_t *q) {
	dig_query_t *next = ISC_LIST_NEXT(q, link);
	bool tcp_mode = l->tcp_mode;

	INSIST(l->ns_search_only && !l->trace_root);
	INSIST(l == current_lookup);

	if (next == NULL) {
		/*
		 * If this is the last query, and if there was
		 * not a single successful query in the whole
		 * lookup, then treat the situation as an error,
		 * cancel and clear the lookup.
		 */
		if (check_if_queries_done(l, q) && !l->ns_search_success) {
			dighost_error("NS servers could not be reached");
			if (exitcode < 9) {
				exitcode = 9;
			}

			cancel_lookup(l);
			query_detach(&q);
			lookup_detach(&l);
			clear_current_lookup();
		} else {
			query_detach(&q);
			lookup_detach(&l);
		}
	} else {
		query_detach(&q);
		lookup_detach(&l);

		debug("sending next, since searching");
		if (tcp_mode) {
			start_tcp(next);
		} else {
			start_udp(next);
		}
	}
}

/*%
 * Event handler for send completion.  Track send counter, and clear out
 * the query if the send was canceled.
 */
static void
send_done(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	dig_query_t *query = (dig_query_t *)arg;
	dig_lookup_t *l = NULL;

	REQUIRE(DIG_VALID_QUERY(query));
	INSIST(query->sendhandle != NULL);
	INSIST(handle == query->sendhandle);

	debug("send_done(%p, %s, %p)", handle, isc_result_totext(eresult), arg);

	isc_refcount_decrement0(&sendcount);
	debug("sendcount=%" PRIuFAST32, isc_refcount_current(&sendcount));

	INSIST(!free_now);

	LOCK_LOOKUP;

	isc_nmhandle_detach(&query->sendhandle);

	lookup_attach(query->lookup, &l);

	if (eresult == ISC_R_CANCELED || query->canceled) {
		debug("send_done: cancel");
		if (!query->canceled) {
			cancel_lookup(l);
		}
		query_detach(&query);
		lookup_detach(&l);
		UNLOCK_LOOKUP;
		return;
	} else if (eresult != ISC_R_SUCCESS) {
		debug("send failed: %s", isc_result_totext(eresult));
	}

	if (l->ns_search_only && !l->trace_root) {
		nssearch_next(l, query);
	} else {
		query_detach(&query);
		lookup_detach(&l);
	}

	check_if_done();
	UNLOCK_LOOKUP;
}

/*%
 * Cancel a lookup, sending canceling reads on all existing sockets.
 */

static void
_cancel_lookup(dig_lookup_t *lookup, const char *file, unsigned int line) {
	dig_query_t *query, *next;

	debug("%s:%u:%s()", file, line, __func__);
	query = ISC_LIST_HEAD(lookup->q);
	while (query != NULL) {
		REQUIRE(DIG_VALID_QUERY(query));
		next = ISC_LIST_NEXT(query, link);
		ISC_LIST_DEQUEUE(lookup->q, query, link);
		debug("canceling pending query %p, belonging to %p", query,
		      query->lookup);
		query->canceled = true;
		if (query->readhandle != NULL &&
		    !isc_nm_is_http_handle(query->readhandle))
		{
			isc_nm_cancelread(query->readhandle);
		}
		query_detach(&query);
		query = next;
	}
	lookup->pending = false;
	lookup->retries = 0;
	check_if_done();
}

static isc_tlsctx_t *
get_create_tls_context(dig_query_t *query, const bool is_https,
		       isc_tlsctx_client_session_cache_t **psess_cache) {
	isc_result_t result;
	isc_tlsctx_t *ctx = NULL, *found_ctx = NULL;
	isc_tls_cert_store_t *store = NULL, *found_store = NULL;
	char tlsctxname[ISC_SOCKADDR_FORMATSIZE];
	const uint16_t family = isc_sockaddr_pf(&query->sockaddr) == PF_INET6
					? AF_INET6
					: AF_INET;
	isc_tlsctx_cache_transport_t transport =
		is_https ? isc_tlsctx_cache_https : isc_tlsctx_cache_tls;
	const bool hostname_ignore_subject = !is_https;
	isc_tlsctx_client_session_cache_t *sess_cache = NULL,
					  *found_sess_cache = NULL;

	if (query->lookup->tls_key_file_set != query->lookup->tls_cert_file_set)
	{
		return (NULL);
	}

	isc_sockaddr_format(&query->sockaddr, tlsctxname, sizeof(tlsctxname));

	result = isc_tlsctx_cache_find(query->lookup->tls_ctx_cache, tlsctxname,
				       transport, family, &found_ctx,
				       &found_store, &found_sess_cache);
	if (result != ISC_R_SUCCESS) {
		if (query->lookup->tls_ca_set) {
			if (found_store == NULL) {
				result = isc_tls_cert_store_create(
					query->lookup->tls_ca_file, &store);

				if (result != ISC_R_SUCCESS) {
					goto failure;
				}
			} else {
				store = found_store;
			}
		}

		result = isc_tlsctx_createclient(&ctx);
		if (result != ISC_R_SUCCESS) {
			goto failure;
		}

		if (store != NULL) {
			const char *hostname =
				query->lookup->tls_hostname_set
					? query->lookup->tls_hostname
					: query->userarg;
			/*
			 * According to RFC 8310, Subject field MUST NOT be
			 * inspected when verifying hostname for DoT. Only
			 * SubjectAltName must be checked. That is NOT the case
			 * for HTTPS.
			 */
			result = isc_tlsctx_enable_peer_verification(
				ctx, false, store, hostname,
				hostname_ignore_subject);
			if (result != ISC_R_SUCCESS) {
				goto failure;
			}
		}

		if (query->lookup->tls_key_file_set &&
		    query->lookup->tls_cert_file_set)
		{
			result = isc_tlsctx_load_certificate(
				ctx, query->lookup->tls_key_file,
				query->lookup->tls_cert_file);
			if (result != ISC_R_SUCCESS) {
				goto failure;
			}
		}

		if (!is_https) {
			isc_tlsctx_enable_dot_client_alpn(ctx);
		}

#if HAVE_LIBNGHTTP2
		if (is_https) {
			isc_tlsctx_enable_http2client_alpn(ctx);
		}
#endif /* HAVE_LIBNGHTTP2 */

		isc_tlsctx_client_session_cache_create(
			mctx, ctx, ISC_TLSCTX_CLIENT_SESSION_CACHE_DEFAULT_SIZE,
			&sess_cache);

		result = isc_tlsctx_cache_add(
			query->lookup->tls_ctx_cache, tlsctxname, transport,
			family, ctx, store, sess_cache, NULL, NULL, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if (psess_cache != NULL) {
			INSIST(*psess_cache == NULL);
			*psess_cache = sess_cache;
		}
		return (ctx);
	}

	if (psess_cache != NULL) {
		INSIST(*psess_cache == NULL);
		*psess_cache = found_sess_cache;
	}

	INSIST(!query->lookup->tls_ca_set || found_store != NULL);
	return (found_ctx);
failure:
	if (ctx != NULL) {
		isc_tlsctx_free(&ctx);
	}
	/*
	 * The 'found_store' is being managed by the TLS context
	 * cache. Thus, we should keep it as it is, as it will get
	 * destroyed alongside the cache. As there is one store per
	 * multiple TLS contexts, we need to handle store deletion in a
	 * special way.
	 */
	if (store != NULL && store != found_store) {
		isc_tls_cert_store_free(&store);
	}
	return (NULL);
}

static void
tcp_connected(isc_nmhandle_t *handle, isc_result_t eresult, void *arg);

/*%
 * Unlike start_udp, this can't be called multiple times with the same
 * query.  When we retry TCP, we requeue the whole lookup, which should
 * start anew.
 */
static void
start_tcp(dig_query_t *query) {
	isc_result_t result;
	dig_query_t *next = NULL;
	dig_query_t *connectquery = NULL;
	isc_tlsctx_t *tlsctx = NULL;
	bool tls_mode = false;
	isc_tlsctx_client_session_cache_t *sess_cache = NULL;
	REQUIRE(DIG_VALID_QUERY(query));

	debug("start_tcp(%p)", query);

	query_attach(query, &query->lookup->current_query);

	tls_mode = dig_lookup_is_tls(query->lookup);

	/*
	 * For TLS connections, we want to override the default
	 * port number.
	 */
	if (!port_set) {
		if (tls_mode) {
			port = 853;
		} else if (query->lookup->https_mode &&
			   !query->lookup->http_plain)
		{
			port = 443;
		} else if (query->lookup->https_mode) {
			port = 80;
		} else {
			port = 53;
		}
	}

	debug("query->servname = %s\n", query->servname);

	result = get_address(query->servname, port, &query->sockaddr);
	if (result != ISC_R_SUCCESS) {
		/*
		 * This servname doesn't have an address.  Try the next server
		 * by triggering an immediate 'timeout' (we lie, but the effect
		 * is the same).
		 */
		force_next(query);
		return;
	}

	if (isc_sockaddr_pf(&query->sockaddr) == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&query->sockaddr.type.sin6.sin6_addr))
	{
		isc_netaddr_t netaddr;
		char buf[ISC_NETADDR_FORMATSIZE];

		isc_netaddr_fromsockaddr(&netaddr, &query->sockaddr);
		isc_netaddr_format(&netaddr, buf, sizeof(buf));
		dighost_warning("Skipping mapped address '%s'", buf);

		if (ISC_LINK_LINKED(query, link)) {
			next = ISC_LIST_NEXT(query, link);
		} else {
			next = NULL;
		}
		query_detach(&query);
		if (next == NULL) {
			dighost_warning("No acceptable nameservers");
			clear_current_lookup();
		} else {
			start_tcp(next);
		}
		return;
	}

	INSIST(query->handle == NULL);

	if (keep != NULL && isc_sockaddr_equal(&keepaddr, &query->sockaddr)) {
		query->handle = keep;
		launch_next_query(query);
		query_detach(&query);
		return;
	} else {
		int local_timeout = timeout * 1000;
		if (local_timeout == 0) {
			local_timeout = TCP_TIMEOUT * 1000;
		}

		if (keep != NULL) {
			isc_nmhandle_detach(&keep);
		}

		if (!specified_source) {
			if ((isc_sockaddr_pf(&query->sockaddr) == AF_INET) &&
			    have_ipv4)
			{
				isc_sockaddr_any(&localaddr);
			} else {
				isc_sockaddr_any6(&localaddr);
			}
		}

		REQUIRE(query != NULL);

		query_attach(query, &connectquery);

		if (tls_mode) {
			tlsctx = get_create_tls_context(connectquery, false,
							&sess_cache);
			if (tlsctx == NULL) {
				goto failure_tls;
			}
			isc_nm_tlsdnsconnect(netmgr, &localaddr,
					     &query->sockaddr, tcp_connected,
					     connectquery, local_timeout, 0,
					     tlsctx, sess_cache);
#if HAVE_LIBNGHTTP2
		} else if (query->lookup->https_mode) {
			char uri[4096] = { 0 };
			isc_nm_http_makeuri(!query->lookup->http_plain,
					    &query->sockaddr, query->userarg,
					    port, query->lookup->https_path,
					    uri, sizeof(uri));

			if (!query->lookup->http_plain) {
				tlsctx = get_create_tls_context(
					connectquery, true, &sess_cache);
				if (tlsctx == NULL) {
					goto failure_tls;
				}
			}

			isc_nm_httpconnect(netmgr, &localaddr, &query->sockaddr,
					   uri, !query->lookup->https_get,
					   tcp_connected, connectquery, tlsctx,
					   sess_cache, 0, local_timeout);
#endif
		} else {
			isc_nm_tcpdnsconnect(netmgr, &localaddr,
					     &query->sockaddr, tcp_connected,
					     connectquery, local_timeout, 0);
		}
	}
	return;
failure_tls:
	if (query->lookup->tls_key_file_set != query->lookup->tls_cert_file_set)
	{
		dighost_warning(
			"both TLS client certificate and key file must be "
			"specified a the same time");
	} else {
		dighost_warning("TLS context cannot be created");
	}

	if (ISC_LINK_LINKED(query, link)) {
		next = ISC_LIST_NEXT(query, link);
	} else {
		next = NULL;
	}
	query_detach(&connectquery);
	query_detach(&query);
	if (next == NULL) {
		clear_current_lookup();
	} else {
		start_tcp(next);
	}
}

static void
print_query_size(dig_query_t *query) {
	if (!yaml) {
		printf(";; QUERY SIZE: %u\n\n",
		       isc_buffer_usedlength(&query->lookup->renderbuf));
	}
}

static void
send_udp(dig_query_t *query) {
	dig_query_t *sendquery = NULL;
	isc_region_t r;

	query_attach(query, &sendquery);

	isc_buffer_usedregion(&query->sendbuf, &r);
	debug("sending a request");
	if (query->lookup->use_usec) {
		TIME_NOW_HIRES(&query->time_sent);
	} else {
		TIME_NOW(&query->time_sent);
	}

	isc_nmhandle_attach(query->handle, &query->sendhandle);

	isc_nm_send(query->handle, &r, send_done, sendquery);
	isc_refcount_increment0(&sendcount);
	debug("sendcount=%" PRIuFAST32, isc_refcount_current(&sendcount));

	/* XXX qrflag, print_query, etc... */
	if (query->lookup->qr) {
		extrabytes = 0;
		dighost_printmessage(query, &query->lookup->renderbuf,
				     query->lookup->sendmsg, true);
		if (query->lookup->stats) {
			print_query_size(query);
		}
	}
}

static void
udp_ready(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	dig_query_t *query = (dig_query_t *)arg;
	dig_query_t *next = NULL;
	char sockstr[ISC_SOCKADDR_FORMATSIZE];
	dig_lookup_t *l = NULL;
	dig_query_t *readquery = NULL;
	int local_timeout = timeout * 1000;

	REQUIRE(DIG_VALID_QUERY(query));
	REQUIRE(query->handle == NULL);

	debug("udp_ready()");

	query->started = true;

	if (atomic_load(&cancel_now)) {
		query_detach(&query);
		return;
	}

	INSIST(!free_now);

	debug("udp_ready(%p, %s, %p)", handle, isc_result_totext(eresult),
	      query);

	LOCK_LOOKUP;
	lookup_attach(query->lookup, &l);

	if (eresult == ISC_R_CANCELED || query->canceled) {
		debug("in cancel handler");
		if (!query->canceled) {
			cancel_lookup(l);
		}
		query_detach(&query);
		lookup_detach(&l);
		clear_current_lookup();
		UNLOCK_LOOKUP;
		return;
	}

	if (eresult != ISC_R_SUCCESS) {
		debug("udp setup failed: %s", isc_result_totext(eresult));
		isc_sockaddr_format(&query->sockaddr, sockstr, sizeof(sockstr));
		dighost_warning("UDP setup with %s(%s) for %s failed: %s.",
				sockstr, query->servname, l->textname,
				isc_result_totext(eresult));

		/*
		 * NSSEARCH mode: if the current query failed to start properly,
		 * then send_done() will not be called, and we want to make sure
		 * that the next query gets a chance to start in order to not
		 * break the chain.
		 */
		if (l->ns_search_only && !l->trace_root) {
			nssearch_next(l, query);

			check_if_done();
			UNLOCK_LOOKUP;
			return;
		}

		if (exitcode < 9) {
			exitcode = 9;
		}

		if (l->retries > 1) {
			l->retries--;
			debug("making new UDP request, %d tries left",
			      l->retries);
			requeue_lookup(l, true);
			next = NULL;
		} else if ((l->current_query != NULL) &&
			   (ISC_LINK_LINKED(l->current_query, link)))
		{
			next = ISC_LIST_NEXT(l->current_query, link);
		} else {
			next = NULL;
		}

		query_detach(&query);
		if (next == NULL) {
			cancel_lookup(l);
		}
		lookup_detach(&l);

		if (next != NULL) {
			start_udp(next);
			check_if_done();
		} else {
			dighost_error("no servers could be reached\n");
			clear_current_lookup();
		}

		UNLOCK_LOOKUP;
		return;
	}

	exitcode = 0;

	query_attach(query, &readquery);

	debug("recving with lookup=%p, query=%p, handle=%p", query->lookup,
	      query, handle);

	query->handle = handle;
	isc_nmhandle_attach(handle, &query->readhandle);
	isc_refcount_increment0(&recvcount);
	debug("recvcount=%" PRIuFAST32, isc_refcount_current(&recvcount));

	if (local_timeout == 0) {
		local_timeout = UDP_TIMEOUT * 1000;
	}

	debug("have local timeout of %d", local_timeout);
	isc_nmhandle_settimeout(handle, local_timeout);

	isc_nm_read(handle, recv_done, readquery);
	send_udp(readquery);

	query_detach(&query);
	lookup_detach(&l);
	UNLOCK_LOOKUP;
}

/*%
 * Send a UDP packet to the remote nameserver, possible starting the
 * recv action as well.  Also make sure that the timer is running and
 * is properly reset.
 */
static void
start_udp(dig_query_t *query) {
	isc_result_t result;
	dig_query_t *next = NULL;
	dig_query_t *connectquery = NULL;

	REQUIRE(DIG_VALID_QUERY(query));

	debug("start_udp(%p)", query);

	query_attach(query, &query->lookup->current_query);
	debug("working on lookup %p, query %p", query->lookup, query);

	if (query->handle != NULL) {
		launch_next_query(query);
		query_detach(&query);
		return;
	}

	result = get_address(query->servname, port, &query->sockaddr);
	if (result != ISC_R_SUCCESS) {
		/* This servname doesn't have an address. */
		force_next(query);
		return;
	}

	if (isc_sockaddr_pf(&query->sockaddr) == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&query->sockaddr.type.sin6.sin6_addr))
	{
		isc_netaddr_t netaddr;
		char buf[ISC_NETADDR_FORMATSIZE];

		isc_netaddr_fromsockaddr(&netaddr, &query->sockaddr);
		isc_netaddr_format(&netaddr, buf, sizeof(buf));
		dighost_warning("Skipping mapped address '%s'", buf);
		next = ISC_LIST_NEXT(query, link);
		query_detach(&query);
		if (next == NULL) {
			dighost_warning("No acceptable nameservers");
			clear_current_lookup();
		} else {
			start_udp(next);
		}
		return;
	}

	if (!specified_source) {
		if ((isc_sockaddr_pf(&query->sockaddr) == AF_INET) && have_ipv4)
		{
			isc_sockaddr_any(&localaddr);
		} else {
			isc_sockaddr_any6(&localaddr);
		}
	}

	query_attach(query, &connectquery);
	isc_nm_udpconnect(netmgr, &localaddr, &query->sockaddr, udp_ready,
			  connectquery,
			  (timeout ? timeout : UDP_TIMEOUT) * 1000, 0);
}

/*%
 * If there are more servers available for querying within 'lookup', initiate a
 * TCP or UDP query to the next available server and return true; otherwise,
 * return false.
 */
static bool
try_next_server(dig_lookup_t *lookup) {
	dig_query_t *current_query, *next_query;

	current_query = lookup->current_query;
	if (current_query == NULL || !ISC_LINK_LINKED(current_query, link)) {
		return (false);
	}

	next_query = ISC_LIST_NEXT(current_query, link);
	if (next_query == NULL) {
		return (false);
	}

	debug("trying next server...");

	if (lookup->tcp_mode) {
		start_tcp(next_query);
	} else {
		start_udp(next_query);
	}

	return (true);
}

static void
force_next(dig_query_t *query) {
	dig_lookup_t *l = NULL;

	REQUIRE(DIG_VALID_QUERY(query));

	debug("force_next()");

	LOCK_LOOKUP;
	INSIST(!free_now);

	if (atomic_load(&cancel_now)) {
		UNLOCK_LOOKUP;
		return;
	}

	lookup_attach(query->lookup, &l);

	if (try_next_server(l)) {
		lookup_detach(&l);
		UNLOCK_LOOKUP;
		return;
	}

	if (l->retries > 1) {
		l->retries--;
		debug("making new %s request, %d tries left",
		      l->tcp_mode ? "TCP" : "UDP", l->retries);
		requeue_lookup(l, true);
		lookup_detach(&l);
		isc_refcount_decrement0(&recvcount);
		debug("recvcount=%" PRIuFAST32,
		      isc_refcount_current(&recvcount));
		query_detach(&query);
		clear_current_lookup();
		UNLOCK_LOOKUP;
		return;
	}

	if (query->readhandle != NULL) {
		isc_refcount_decrement0(&recvcount);
		debug("recvcount=%" PRIuFAST32,
		      isc_refcount_current(&recvcount));
	}

	if (l->ns_search_only) {
		isc_netaddr_t netaddr;
		char buf[ISC_NETADDR_FORMATSIZE];

		isc_netaddr_fromsockaddr(&netaddr, &query->sockaddr);
		isc_netaddr_format(&netaddr, buf, sizeof(buf));

		dighost_error("no response from %s\n", buf);
	} else {
		printf("%s", l->cmdline);
		dighost_error("no servers could be reached\n");
	}

	if (exitcode < 9) {
		exitcode = 9;
	}

	query_detach(&query);
	cancel_lookup(l);
	lookup_detach(&l);
	clear_current_lookup();
	UNLOCK_LOOKUP;
}

/*%
 * For transfers that involve multiple recvs (XFR's in particular),
 * launch the next recv.
 */
static void
launch_next_query(dig_query_t *query) {
	dig_query_t *readquery = NULL;
	int local_timeout = timeout * 1000;
	dig_lookup_t *l = NULL;
	isc_region_t r;
	bool xfr;

	REQUIRE(DIG_VALID_QUERY(query));
	INSIST(!free_now);

	debug("launch_next_query()");

	lookup_attach(query->lookup, &l);

	if (!l->pending) {
		debug("ignoring launch_next_query because !pending");
		query_detach(&query);
		lookup_detach(&l);
		clear_current_lookup();
		return;
	}

	isc_nmhandle_attach(query->handle, &query->readhandle);
	isc_refcount_increment0(&recvcount);
	debug("recvcount=%" PRIuFAST32, isc_refcount_current(&recvcount));

	if (local_timeout == 0) {
		local_timeout = TCP_TIMEOUT * 1000;
	}

	debug("have local timeout of %d", local_timeout);
	isc_nmhandle_settimeout(query->handle, local_timeout);

	xfr = query->lookup->rdtype == dns_rdatatype_ixfr ||
	      query->lookup->rdtype == dns_rdatatype_axfr;
	if (xfr && isc_nm_socket_type(query->handle) == isc_nm_tlsdnssocket) {
		isc_result_t result = isc_nm_xfr_checkperm(query->handle);
		if (result != ISC_R_SUCCESS) {
			dighost_error("zone transfers over the established TLS "
				      "connection are not allowed: %s",
				      isc_result_totext(result));
			isc_refcount_decrement0(&recvcount);
			isc_nmhandle_detach(&query->readhandle);
			cancel_lookup(l);
			lookup_detach(&l);
			clear_current_lookup();
			return;
		}
	}

	query_attach(query, &readquery);

	isc_nm_read(query->handle, recv_done, readquery);

	if (!query->first_soa_rcvd) {
		dig_query_t *sendquery = NULL;
		debug("sending a request in launch_next_query");
		if (query->lookup->use_usec) {
			TIME_NOW_HIRES(&query->time_sent);
		} else {
			TIME_NOW(&query->time_sent);
		}

		query_attach(query, &sendquery);
		isc_buffer_usedregion(&query->sendbuf, &r);
		if (keep != NULL) {
			query->handle = keep;
		}

		isc_nmhandle_attach(query->handle, &query->sendhandle);
		isc_nm_send(query->handle, &r, send_done, sendquery);
		isc_refcount_increment0(&sendcount);
		debug("sendcount=%" PRIuFAST32,
		      isc_refcount_current(&sendcount));

		/* XXX qrflag, print_query, etc... */
		if (l->qr) {
			extrabytes = 0;
			dighost_printmessage(query, &l->renderbuf, l->sendmsg,
					     true);
			if (l->stats) {
				print_query_size(query);
			}
		}
	}

	lookup_detach(&l);
	return;
}

/*%
 * Event handler for TCP connect complete.  Make sure the connection was
 * successful, then pass into launch_next_query to actually send the
 * question.
 */
static void
tcp_connected(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	dig_query_t *query = (dig_query_t *)arg;
	dig_query_t *next = NULL;
	char sockstr[ISC_SOCKADDR_FORMATSIZE];
	dig_lookup_t *l = NULL;

	REQUIRE(DIG_VALID_QUERY(query));
	REQUIRE(query->handle == NULL);

	debug("tcp_connected()");

	query->started = true;

	if (atomic_load(&cancel_now)) {
		query_detach(&query);
		return;
	}

	INSIST(!free_now);

	debug("tcp_connected(%p, %s, %p)", handle, isc_result_totext(eresult),
	      query);

	LOCK_LOOKUP;
	lookup_attach(query->lookup, &l);

	if (eresult == ISC_R_CANCELED || eresult == ISC_R_TLSBADPEERCERT ||
	    query->canceled)
	{
		debug("in cancel handler");
		isc_sockaddr_format(&query->sockaddr, sockstr, sizeof(sockstr));
		if (eresult == ISC_R_TLSBADPEERCERT) {
			dighost_warning(
				"TLS peer certificate verification for "
				"%s failed: %s",
				sockstr,
				isc_nm_verify_tls_peer_result_string(handle));
		} else if (query->lookup->rdtype == dns_rdatatype_ixfr ||
			   query->lookup->rdtype == dns_rdatatype_axfr)
		{
			puts("; Transfer failed.");
		}

		if (!query->canceled) {
			cancel_lookup(l);
		}

		query_detach(&query);
		lookup_detach(&l);
		clear_current_lookup();
		UNLOCK_LOOKUP;
		return;
	}

	if (eresult != ISC_R_SUCCESS) {
		debug("unsuccessful connection: %s",
		      isc_result_totext(eresult));
		isc_sockaddr_format(&query->sockaddr, sockstr, sizeof(sockstr));
		dighost_warning("Connection to %s(%s) for %s failed: %s.",
				sockstr, query->servname, l->textname,
				isc_result_totext(eresult));

		/*
		 * NSSEARCH mode: if the current query failed to start properly,
		 * then send_done() will not be called, and we want to make sure
		 * that the next query gets a chance to start in order to not
		 * break the chain.
		 */
		if (l->ns_search_only && !l->trace_root) {
			nssearch_next(l, query);

			check_if_done();
			UNLOCK_LOOKUP;
			return;
		}

		/* XXX Clean up exitcodes */
		if (exitcode < 9) {
			exitcode = 9;
		}

		if (l->retries > 1) {
			l->retries--;
			debug("making new TCP request, %d tries left",
			      l->retries);
			requeue_lookup(l, true);
			next = NULL;
		} else if ((l->current_query != NULL) &&
			   (ISC_LINK_LINKED(l->current_query, link)))
		{
			next = ISC_LIST_NEXT(l->current_query, link);
		} else {
			next = NULL;
		}

		query_detach(&query);
		if (next == NULL) {
			cancel_lookup(l);
		}
		lookup_detach(&l);

		if (next != NULL) {
			start_tcp(next);
			check_if_done();
		} else {
			dighost_error("no servers could be reached\n");
			clear_current_lookup();
		}

		UNLOCK_LOOKUP;
		return;
	}

	exitcode = 0;

	query->handle = handle;
	if (keep_open) {
		keepaddr = query->sockaddr;
		if (keep != NULL) {
			isc_nmhandle_detach(&keep);
		}

		isc_nmhandle_attach(handle, &keep);
	}

	launch_next_query(query);
	query_detach(&query);
	lookup_detach(&l);
	UNLOCK_LOOKUP;
}

/*%
 * Check if the ongoing XFR needs more data before it's complete, using
 * the semantics of IXFR and AXFR protocols.  Much of the complexity of
 * this routine comes from determining when an IXFR is complete.
 * false means more data is on the way, and the recv has been issued.
 */
static bool
check_for_more_data(dig_lookup_t *lookup, dig_query_t *query,
		    dns_message_t *msg, isc_sockaddr_t *peer, int len) {
	dns_rdataset_t *rdataset = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_soa_t soa;
	uint32_t ixfr_serial = lookup->ixfr_serial, serial;
	isc_result_t result;
	bool ixfr = lookup->rdtype == dns_rdatatype_ixfr;
	bool axfr = lookup->rdtype == dns_rdatatype_axfr;

	if (ixfr) {
		axfr = query->ixfr_axfr;
	}

	debug("check_for_more_data()");

	/*
	 * By the time we're in this routine, we know we're doing
	 * either an AXFR or IXFR.  If there's no second_rr_type,
	 * then we don't yet know which kind of answer we got back
	 * from the server.  Here, we're going to walk through the
	 * rr's in the message, acting as necessary whenever we hit
	 * an SOA rr.
	 */

	query->msg_count++;
	query->byte_count += len;
	result = dns_message_firstname(msg, DNS_SECTION_ANSWER);
	if (result != ISC_R_SUCCESS) {
		puts("; Transfer failed.");
		return (true);
	}
	do {
		dns_name_t *name;
		name = NULL;
		dns_message_currentname(msg, DNS_SECTION_ANSWER, &name);
		for (rdataset = ISC_LIST_HEAD(name->list); rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link))
		{
			result = dns_rdataset_first(rdataset);
			if (result != ISC_R_SUCCESS) {
				continue;
			}
			do {
				query->rr_count++;
				dns_rdata_reset(&rdata);
				dns_rdataset_current(rdataset, &rdata);
				/*
				 * If this is the first rr, make sure
				 * it's an SOA
				 */
				if ((!query->first_soa_rcvd) &&
				    (rdata.type != dns_rdatatype_soa))
				{
					puts("; Transfer failed.  "
					     "Didn't start with SOA answer.");
					return (true);
				}
				if ((!query->second_rr_rcvd) &&
				    (rdata.type != dns_rdatatype_soa))
				{
					query->second_rr_rcvd = true;
					query->second_rr_serial = 0;
					debug("got the second rr as nonsoa");
					axfr = query->ixfr_axfr = true;
					goto next_rdata;
				}

				/*
				 * If the record is anything except an SOA
				 * now, just continue on...
				 */
				if (rdata.type != dns_rdatatype_soa) {
					goto next_rdata;
				}

				/* Now we have an SOA.  Work with it. */
				debug("got an SOA");
				result = dns_rdata_tostruct(&rdata, &soa, NULL);
				check_result(result, "dns_rdata_tostruct");
				serial = soa.serial;
				dns_rdata_freestruct(&soa);
				if (!query->first_soa_rcvd) {
					query->first_soa_rcvd = true;
					query->first_rr_serial = serial;
					debug("this is the first serial %u",
					      serial);
					if (ixfr &&
					    isc_serial_ge(ixfr_serial, serial))
					{
						debug("got up to date "
						      "response");
						goto doexit;
					}
					goto next_rdata;
				}
				if (axfr) {
					debug("doing axfr, got second SOA");
					goto doexit;
				}
				if (!query->second_rr_rcvd) {
					if (query->first_rr_serial == serial) {
						debug("doing ixfr, got "
						      "empty zone");
						goto doexit;
					}
					debug("this is the second serial %u",
					      serial);
					query->second_rr_rcvd = true;
					query->second_rr_serial = serial;
					goto next_rdata;
				}
				/*
				 * If we get to this point, we're doing an
				 * IXFR and have to start really looking
				 * at serial numbers.
				 */
				if (query->first_rr_serial == serial) {
					debug("got a match for ixfr");
					if (!query->first_repeat_rcvd) {
						query->first_repeat_rcvd = true;
						goto next_rdata;
					}
					debug("done with ixfr");
					goto doexit;
				}
				debug("meaningless soa %u", serial);
			next_rdata:
				result = dns_rdataset_next(rdataset);
			} while (result == ISC_R_SUCCESS);
		}
		result = dns_message_nextname(msg, DNS_SECTION_ANSWER);
	} while (result == ISC_R_SUCCESS);
	isc_nmhandle_detach(&query->readhandle);
	launch_next_query(query);
	query_detach(&query);
	return (false);
doexit:
	dighost_received(len, peer, query);
	return (true);
}

static void
process_cookie(dig_lookup_t *l, dns_message_t *msg, isc_buffer_t *optbuf,
	       size_t optlen) {
	char bb[256];
	isc_buffer_t hexbuf;
	size_t len;
	const unsigned char *sent;
	bool copy = true;
	isc_result_t result;

	if (l->cookie != NULL) {
		isc_buffer_init(&hexbuf, bb, sizeof(bb));
		result = isc_hex_decodestring(l->cookie, &hexbuf);
		check_result(result, "isc_hex_decodestring");
		sent = isc_buffer_base(&hexbuf);
		len = isc_buffer_usedlength(&hexbuf);
	} else {
		sent = cookie;
		len = sizeof(cookie);
	}

	INSIST(msg->cc_ok == 0 && msg->cc_bad == 0);
	if (len >= 8 && optlen >= 8U) {
		if (isc_safe_memequal(isc_buffer_current(optbuf), sent, 8)) {
			msg->cc_ok = 1;
		} else {
			dighost_warning("Warning: Client COOKIE mismatch");
			msg->cc_bad = 1;
			copy = false;
		}
	} else {
		dighost_warning("Warning: COOKIE bad token (too short)");
		msg->cc_bad = 1;
		copy = false;
	}
	if (copy) {
		isc_region_t r;

		r.base = isc_buffer_current(optbuf);
		r.length = (unsigned int)optlen;
		isc_buffer_init(&hexbuf, servercookie, sizeof(servercookie));
		result = isc_hex_totext(&r, 2, "", &hexbuf);
		check_result(result, "isc_hex_totext");
		if (isc_buffer_availablelength(&hexbuf) > 0) {
			isc_buffer_putuint8(&hexbuf, 0);
			l->cookie = servercookie;
		}
	}
	isc_buffer_forward(optbuf, (unsigned int)optlen);
}

static void
process_opt(dig_lookup_t *l, dns_message_t *msg) {
	dns_rdata_t rdata;
	isc_result_t result;
	isc_buffer_t optbuf;
	uint16_t optcode, optlen;
	dns_rdataset_t *opt = msg->opt;
	bool seen_cookie = false;

	result = dns_rdataset_first(opt);
	if (result == ISC_R_SUCCESS) {
		dns_rdata_init(&rdata);
		dns_rdataset_current(opt, &rdata);
		isc_buffer_init(&optbuf, rdata.data, rdata.length);
		isc_buffer_add(&optbuf, rdata.length);
		while (isc_buffer_remaininglength(&optbuf) >= 4) {
			optcode = isc_buffer_getuint16(&optbuf);
			optlen = isc_buffer_getuint16(&optbuf);
			switch (optcode) {
			case DNS_OPT_COOKIE:
				/*
				 * Only process the first cookie option.
				 */
				if (seen_cookie) {
					isc_buffer_forward(&optbuf, optlen);
					break;
				}
				process_cookie(l, msg, &optbuf, optlen);
				seen_cookie = true;
				break;
			default:
				isc_buffer_forward(&optbuf, optlen);
				break;
			}
		}
	}
}

static int
ednsvers(dns_rdataset_t *opt) {
	return ((opt->ttl >> 16) & 0xff);
}

/*%
 * Event handler for recv complete.  Perform whatever actions are necessary,
 * based on the specifics of the user's request.
 */
static void
recv_done(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	  void *arg) {
	dig_query_t *query = (dig_query_t *)arg;
	isc_buffer_t b;
	dns_message_t *msg = NULL;
	isc_result_t result;
	dig_lookup_t *n = NULL;
	dig_lookup_t *l = NULL;
	bool docancel = false;
	bool donext = false;
	bool match = true;
	bool done_process_opt = false;
	unsigned int parseflags;
	dns_messageid_t id;
	unsigned int msgflags;
	int newedns;
	isc_sockaddr_t peer;

	REQUIRE(DIG_VALID_QUERY(query));
	REQUIRE(query->readhandle != NULL);
	INSIST(!free_now);

	debug("recv_done(%p, %s, %p, %p)", handle, isc_result_totext(eresult),
	      region, arg);

	LOCK_LOOKUP;

	isc_refcount_decrement0(&recvcount);
	debug("recvcount=%" PRIuFAST32, isc_refcount_current(&recvcount));

	lookup_attach(query->lookup, &l);

	if (eresult == ISC_R_CANCELED || query->canceled) {
		debug("recv_done: cancel");
		isc_nmhandle_detach(&query->readhandle);
		if (!query->canceled) {
			cancel_lookup(l);
		}
		query_detach(&query);
		lookup_detach(&l);
		clear_current_lookup();
		UNLOCK_LOOKUP;
		return;
	}

	if (query->lookup->use_usec) {
		TIME_NOW_HIRES(&query->time_recv);
	} else {
		TIME_NOW(&query->time_recv);
	}

	if ((!l->pending && !l->ns_search_only) || atomic_load(&cancel_now)) {
		debug("no longer pending.  Got %s", isc_result_totext(eresult));

		goto next_lookup;
	}

	/*
	 * NSSEARCH mode is special, because the queries in the followup lookup
	 * are independent and they are being started in parallel, so if one of
	 * them fails there is no need to start the next query in the lookup,
	 * and this failure can be treated as a soft error (with a warning
	 * message), because there are usually more than one NS servers in the
	 * lookup's queries list. However, if there was not a single successful
	 * query in the followup lookup, then print an error message and exit
	 * with a non-zero exit code.
	 */
	if (l->ns_search_only && !l->trace_root) {
		if (eresult == ISC_R_SUCCESS) {
			l->ns_search_success = true;
		} else {
			char sockstr[ISC_SOCKADDR_FORMATSIZE];
			isc_sockaddr_format(&query->sockaddr, sockstr,
					    sizeof(sockstr));

			dighost_warning("communications error to %s: %s",
					sockstr, isc_result_totext(eresult));

			/*
			 * If this is not the last query, then we detach the
			 * query, but keep the lookup running.
			 */
			if (!check_if_queries_done(l, query)) {
				goto detach_query;
			}

			/*
			 * This is the last query, and if there was not a
			 * single successful query in the whole lookup, then
			 * treat the situation as an error.
			 */
			if (!l->ns_search_success) {
				dighost_error(
					"NS servers could not be reached");
				if (exitcode < 9) {
					exitcode = 9;
				}
			}

			goto cancel_lookup;
		}
	}

	if (eresult != ISC_R_SUCCESS) {
		char sockstr[ISC_SOCKADDR_FORMATSIZE];

		isc_sockaddr_format(&query->sockaddr, sockstr, sizeof(sockstr));
		dighost_warning("communications error to %s: %s", sockstr,
				isc_result_totext(eresult));

		if (l->retries > 1 && !l->tcp_mode) {
			dig_query_t *newq = NULL;

			/*
			 * For UDP, insert a copy of the current query just
			 * after itself in the list, and start it to retry the
			 * request.
			 */
			newq = new_query(l, query->servname, query->userarg);
			ISC_LIST_INSERTAFTER(l->q, query, newq, link);
			if (l->current_query == query) {
				query_detach(&l->current_query);
			}
			if (l->current_query == NULL) {
				l->retries--;
				debug("making new UDP request, %d tries left",
				      l->retries);
				start_udp(newq);
			}
			if (check_if_queries_done(l, query)) {
				goto cancel_lookup;
			}

			goto detach_query;
		} else if (l->retries > 1 && l->tcp_mode) {
			/*
			 * For TCP, we have to requeue the whole lookup, see
			 * the comments above the start_tcp() function.
			 */
			l->retries--;
			debug("making new TCP request, %d tries left",
			      l->retries);
			requeue_lookup(l, true);

			if (keep != NULL) {
				isc_nmhandle_detach(&keep);
			}

			goto cancel_lookup;
		} else {
			dig_query_t *next = ISC_LIST_NEXT(query, link);

			/*
			 * No retries left, go to the next query, if there is
			 * one.
			 */
			if (next != NULL) {
				if (l->current_query == query) {
					query_detach(&l->current_query);
				}
				if (l->current_query == NULL) {
					debug("starting next query %p", next);
					if (l->tcp_mode) {
						start_tcp(next);
					} else {
						start_udp(next);
					}
				}
				if (check_if_queries_done(l, query)) {
					goto cancel_lookup;
				}

				goto detach_query;
			}

			/*
			 * Otherwise, print the cmdline and an error message,
			 * and cancel the lookup.
			 */
			printf("%s", l->cmdline);
			dighost_error("no servers could be reached\n");

			if (exitcode < 9) {
				exitcode = 9;
			}

			if (keep != NULL) {
				isc_nmhandle_detach(&keep);
			}

			goto cancel_lookup;
		}
	}

	isc_buffer_init(&b, region->base, region->length);
	isc_buffer_add(&b, region->length);

	peer = isc_nmhandle_peeraddr(handle);

	result = dns_message_peekheader(&b, &id, &msgflags);
	if (result != ISC_R_SUCCESS || l->sendmsg->id != id) {
		match = false;
		if (l->tcp_mode) {
			bool fail = true;
			if (result == ISC_R_SUCCESS) {
				if ((!query->first_soa_rcvd || query->warn_id))
				{
					dighost_warning("%s: ID mismatch: "
							"expected ID %u, got "
							"%u",
							query->first_soa_rcvd
								? "WARNING"
								: "ERROR",
							l->sendmsg->id, id);
				}
				if (query->first_soa_rcvd) {
					fail = false;
				}
				query->warn_id = false;
			} else {
				dighost_warning("ERROR: short (< header size) "
						"message");
			}
			if (fail) {
				goto cancel_lookup;
			}
			match = true;
		} else if (result == ISC_R_SUCCESS) {
			dighost_warning("Warning: ID mismatch: expected ID %u,"
					" got %u",
					l->sendmsg->id, id);
		} else {
			dighost_warning("Warning: short (< header size) "
					"message received");
		}
	}

	if (result == ISC_R_SUCCESS && (msgflags & DNS_MESSAGEFLAG_QR) == 0) {
		dighost_warning("Warning: query response not set");
	}

	if (!match) {
		/*
		 * We are still attached to query and the query->readhandle is
		 * also attached
		 */
		isc_refcount_increment0(&recvcount);
		debug("recvcount=%" PRIuFAST32,
		      isc_refcount_current(&recvcount));
		isc_nm_read(handle, recv_done, query);
		goto keep_query;
	}

	dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &msg);

	if (tsigkey != NULL) {
		if (l->querysig == NULL) {
			debug("getting initial querysig");
			result = dns_message_getquerytsig(l->sendmsg, mctx,
							  &l->querysig);
			check_result(result, "dns_message_getquerytsig");
		}
		result = dns_message_setquerytsig(msg, l->querysig);
		check_result(result, "dns_message_setquerytsig");
		result = dns_message_settsigkey(msg, tsigkey);
		check_result(result, "dns_message_settsigkey");
		msg->tsigctx = l->tsigctx;
		l->tsigctx = NULL;
		if (l->msgcounter != 0) {
			msg->tcp_continuation = 1;
		}
		l->msgcounter++;
	}

	debug("before parse starts");
	parseflags = l->dns64prefix ? 0 : DNS_MESSAGEPARSE_PRESERVEORDER;
	if (l->besteffort) {
		parseflags |= DNS_MESSAGEPARSE_BESTEFFORT;
		parseflags |= DNS_MESSAGEPARSE_IGNORETRUNCATION;
	}

	result = dns_message_parse(msg, &b, parseflags);
	if (result == DNS_R_RECOVERABLE) {
		dighost_warning("Warning: Message parser reports malformed "
				"message packet.");
	} else if (result != ISC_R_SUCCESS) {
		if (!yaml) {
			printf(";; Got bad packet: %s\n",
			       isc_result_totext(result));
			hex_dump(&b);
		}
		goto cancel_lookup;
	}

	if (msg->opcode != l->opcode) {
		char expect[20] = { 0 }, got[20] = { 0 };

		isc_buffer_init(&b, &expect, sizeof(expect));
		result = dns_opcode_totext(l->opcode, &b);
		check_result(result, "dns_opcode_totext");

		isc_buffer_init(&b, &got, sizeof(got));
		result = dns_opcode_totext(msg->opcode, &b);
		check_result(result, "dns_opcode_totext");

		dighost_warning("Warning: Opcode mismatch: expected %s, got %s",
				expect, got);

		isc_refcount_increment0(&recvcount);
		debug("recvcount=%" PRIuFAST32,
		      isc_refcount_current(&recvcount));
		isc_nm_read(handle, recv_done, query);
		goto keep_query;
	}

	if (msg->counts[DNS_SECTION_QUESTION] != 0) {
		match = true;
		for (result = dns_message_firstname(msg, DNS_SECTION_QUESTION);
		     result == ISC_R_SUCCESS && match;
		     result = dns_message_nextname(msg, DNS_SECTION_QUESTION))
		{
			dns_name_t *name = NULL;
			dns_rdataset_t *rdataset;

			dns_message_currentname(msg, DNS_SECTION_QUESTION,
						&name);
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link))
			{
				if (l->rdtype != rdataset->type ||
				    l->rdclass != rdataset->rdclass ||
				    !dns_name_equal(l->name, name))
				{
					char namestr[DNS_NAME_FORMATSIZE];
					char typebuf[DNS_RDATATYPE_FORMATSIZE];
					char classbuf[DNS_RDATACLASS_FORMATSIZE];
					dns_name_format(name, namestr,
							sizeof(namestr));
					dns_rdatatype_format(rdataset->type,
							     typebuf,
							     sizeof(typebuf));
					dns_rdataclass_format(rdataset->rdclass,
							      classbuf,
							      sizeof(classbuf));
					dighost_warning(";; Question section "
							"mismatch: got "
							"%s/%s/%s",
							namestr, typebuf,
							classbuf);
					match = false;
				}
			}
		}

		if (!match) {
			if (l->tcp_mode) {
				goto cancel_lookup;
			}

			/*
			 * We are still attached to query and the
			 * query->readhandle is also attached
			 */
			isc_refcount_increment0(&recvcount);
			debug("recvcount=%" PRIuFAST32,
			      isc_refcount_current(&recvcount));
			isc_nm_read(handle, recv_done, query);
			goto keep_query;
		}
	}

	if (msg->rcode == dns_rcode_badvers && msg->opt != NULL &&
	    (newedns = ednsvers(msg->opt)) < l->edns && l->ednsneg)
	{
		/*
		 * Add minimum EDNS version required checks here if needed.
		 */
		dighost_comments(l, "BADVERS, retrying with EDNS version %u.",
				 (unsigned int)newedns);
		l->edns = newedns;
		n = requeue_lookup(l, true);
		if (l->trace && l->trace_root) {
			n->rdtype = l->qrdtype;
		}
		goto cancel_lookup;
	}

	if ((msg->flags & DNS_MESSAGEFLAG_TC) != 0 && !l->ignore &&
	    !l->tcp_mode)
	{
		if (l->cookie == NULL && l->sendcookie && msg->opt != NULL) {
			process_opt(l, msg);
		}
		dighost_comments(l, "Truncated, retrying in TCP mode.");
		n = requeue_lookup(l, true);
		n->tcp_mode = true;
		if (l->trace && l->trace_root) {
			n->rdtype = l->qrdtype;
		}
		goto cancel_lookup;
	}

	if (msg->rcode == dns_rcode_badcookie && !l->tcp_mode &&
	    l->sendcookie && l->badcookie)
	{
		process_opt(l, msg);
		if (msg->cc_ok) {
			if (l->showbadcookie) {
				dighost_printmessage(query, &b, msg, true);
				dighost_received(isc_buffer_usedlength(&b),
						 &peer, query);
			}
			dighost_comments(l, "BADCOOKIE, retrying%s.",
					 l->seenbadcookie ? " in TCP mode"
							  : "");
			n = requeue_lookup(l, true);
			if (l->seenbadcookie) {
				n->tcp_mode = true;
			}
			n->seenbadcookie = true;
			if (l->trace && l->trace_root) {
				n->rdtype = l->qrdtype;
			}
			goto cancel_lookup;
		}
		done_process_opt = true;
	}

	if ((msg->rcode == dns_rcode_servfail && !l->servfail_stops) ||
	    (check_ra && (msg->flags & DNS_MESSAGEFLAG_RA) == 0 && l->recurse))
	{
		dig_query_t *next = ISC_LIST_NEXT(query, link);
		if (l->current_query == query) {
			query_detach(&l->current_query);
		}
		if (next != NULL) {
			debug("sending query %p", next);
			if (l->tcp_mode) {
				start_tcp(next);
			} else {
				start_udp(next);
			}
			dighost_comments(l,
					 "Got %s from %s, trying next "
					 "server",
					 msg->rcode == dns_rcode_servfail
						 ? "SERVFAIL reply"
						 : "recursion not available",
					 query->servname);
			if (check_if_queries_done(l, query)) {
				goto cancel_lookup;
			}

			goto detach_query;
		}
	}

	if (tsigkey != NULL) {
		result = dns_tsig_verify(&b, msg, NULL, NULL);
		if (result != ISC_R_SUCCESS) {
			dighost_warning("Couldn't verify signature: %s",
					isc_result_totext(result));
			validated = false;
		}
		l->tsigctx = msg->tsigctx;
		msg->tsigctx = NULL;
		if (l->querysig != NULL) {
			debug("freeing querysig buffer %p", l->querysig);
			isc_buffer_free(&l->querysig);
		}
		result = dns_message_getquerytsig(msg, mctx, &l->querysig);
		check_result(result, "dns_message_getquerytsig");
	}

	extrabytes = isc_buffer_remaininglength(&b);

	debug("after parse");
	if (l->doing_xfr && l->xfr_q == NULL) {
		l->xfr_q = query;
		/*
		 * Once we are in the XFR message, increase
		 * the timeout to much longer, so brief network
		 * outages won't cause the XFR to abort
		 */
		if (timeout != INT_MAX && query->timer != NULL) {
			unsigned int local_timeout;

			if (timeout == 0) {
				if (l->tcp_mode) {
					local_timeout = TCP_TIMEOUT * 4000;
				} else {
					local_timeout = UDP_TIMEOUT * 4000;
				}
			} else {
				if (timeout < (INT_MAX / 4)) {
					local_timeout = timeout * 4000;
				} else {
					local_timeout = INT_MAX;
				}
			}

			debug("have local timeout of %d", local_timeout);
			isc_nmhandle_settimeout(query->handle, local_timeout);
		}
	}

	if (!done_process_opt) {
		if (l->cookie != NULL) {
			if (msg->opt == NULL) {
				dighost_warning("expected opt record in "
						"response");
			} else {
				process_opt(l, msg);
			}
		} else if (l->sendcookie && msg->opt != NULL) {
			process_opt(l, msg);
		}
	}

	if (!l->doing_xfr || l->xfr_q == query) {
		if (msg->rcode == dns_rcode_nxdomain &&
		    (l->origin != NULL || l->need_search))
		{
			if (!next_origin(l) || showsearch) {
				dighost_printmessage(query, &b, msg, true);
				dighost_received(isc_buffer_usedlength(&b),
						 &peer, query);
			}
		} else if (!l->trace && !l->ns_search_only) {
			dighost_printmessage(query, &b, msg, true);
		} else if (l->trace) {
			int nl = 0;
			int count = msg->counts[DNS_SECTION_ANSWER];

			debug("in TRACE code");
			if (!l->ns_search_only) {
				dighost_printmessage(query, &b, msg, true);
			}

			l->rdtype = l->qrdtype;
			if (l->trace_root || (l->ns_search_only && count > 0)) {
				if (!l->trace_root) {
					l->rdtype = dns_rdatatype_soa;
				}
				nl = followup_lookup(msg, query,
						     DNS_SECTION_ANSWER);
				l->trace_root = false;
			} else if (count == 0) {
				nl = followup_lookup(msg, query,
						     DNS_SECTION_AUTHORITY);
			}
			if (nl == 0) {
				docancel = true;
			}
		} else {
			debug("in NSSEARCH code");

			if (l->trace_root) {
				/*
				 * This is the initial NS query.
				 */
				int nl;

				l->rdtype = dns_rdatatype_soa;
				nl = followup_lookup(msg, query,
						     DNS_SECTION_ANSWER);
				if (nl == 0) {
					docancel = true;
				}
				l->trace_root = false;
				usesearch = false;
			} else {
				/*
				 * This is a query in the followup lookup
				 */
				dighost_printmessage(query, &b, msg, true);

				docancel = check_if_queries_done(l, query);
			}
		}
	}

	if (l->pending) {
		debug("still pending.");
	}

	if (l->doing_xfr) {
		if (query != l->xfr_q) {
			goto detach_query;
		}
		if (!docancel) {
			docancel = check_for_more_data(l, query, msg, &peer,
						       region->length);
		}
		if (docancel) {
			goto cancel_lookup;
		}
		/*
		 * check_for_more_data() will detach from query->readhandle
		 * and query on its own, as it needs to reuse the query and
		 * reattach to the readhandle in launch_next_query().
		 */
		goto keep_query;
	} else {
		if (msg->rcode == dns_rcode_noerror || l->origin == NULL) {
			dighost_received(isc_buffer_usedlength(&b), &peer,
					 query);
		}

		if (!l->ns_search_only) {
			l->pending = false;
		}
		if (!l->ns_search_only || l->trace_root || docancel) {
			goto cancel_lookup;
		}
		goto next_lookup;
	}
cancel_lookup:
	docancel = true;
next_lookup:
	donext = true;
detach_query:
	isc_nmhandle_detach(&query->readhandle);
	query_detach(&query);
	if (docancel) {
		cancel_lookup(l);
	}
keep_query:
	if (msg != NULL) {
		dns_message_detach(&msg);
	}
	lookup_detach(&l);
	if (donext) {
		clear_current_lookup();
	}
	UNLOCK_LOOKUP;
}

/*%
 * Turn a name into an address, using system-supplied routines.  This is
 * used in looking up server names, etc... and needs to use system-supplied
 * routines, since they may be using a non-DNS system for these lookups.
 */
isc_result_t
get_address(char *host, in_port_t myport, isc_sockaddr_t *sockaddr) {
	int count;
	isc_result_t result;
	bool is_running;

	is_running = isc_app_isrunning();
	if (is_running) {
		isc_app_block();
	}
	result = bind9_getaddresses(host, myport, sockaddr, 1, &count);
	if (is_running) {
		isc_app_unblock();
	}
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	INSIST(count == 1);

	return (ISC_R_SUCCESS);
}

int
getaddresses(dig_lookup_t *lookup, const char *host, isc_result_t *resultp) {
	isc_result_t result;
	isc_sockaddr_t sockaddrs[DIG_MAX_ADDRESSES];
	isc_netaddr_t netaddr;
	int count, i;
	dig_server_t *srv;
	char tmp[ISC_NETADDR_FORMATSIZE];

	result = bind9_getaddresses(host, 0, sockaddrs, DIG_MAX_ADDRESSES,
				    &count);
	if (resultp != NULL) {
		*resultp = result;
	}
	if (result != ISC_R_SUCCESS) {
		if (resultp == NULL) {
			fatal("couldn't get address for '%s': %s", host,
			      isc_result_totext(result));
		}
		return (0);
	}

	for (i = 0; i < count; i++) {
		isc_netaddr_fromsockaddr(&netaddr, &sockaddrs[i]);
		isc_netaddr_format(&netaddr, tmp, sizeof(tmp));
		srv = make_server(tmp, host);
		ISC_LIST_APPEND(lookup->my_server_list, srv, link);
	}

	return (count);
}

/*%
 * Initiate either a TCP or UDP lookup
 */
void
do_lookup(dig_lookup_t *lookup) {
	dig_query_t *query;

	REQUIRE(lookup != NULL);

	debug("do_lookup()");
	lookup->pending = true;
	query = ISC_LIST_HEAD(lookup->q);
	if (query != NULL) {
		REQUIRE(DIG_VALID_QUERY(query));
		if (lookup->tcp_mode) {
			start_tcp(query);
		} else {
			start_udp(query);
		}
	}
}

/*%
 * Start everything in action upon task startup.
 */
void
onrun_callback(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	isc_event_free(&event);
	LOCK_LOOKUP;
	start_lookup();
	UNLOCK_LOOKUP;
}

/*%
 * Make everything on the lookup queue go away.  Mainly used by the
 * SIGINT handler.
 */
void
cancel_all(void) {
	dig_lookup_t *l, *n;
	dig_query_t *q, *nq;

	debug("cancel_all()");

	LOCK_LOOKUP;
	if (free_now) {
		UNLOCK_LOOKUP;
		return;
	}
	atomic_store(&cancel_now, true);
	while (current_lookup != NULL) {
		for (q = ISC_LIST_HEAD(current_lookup->q); q != NULL; q = nq) {
			nq = ISC_LIST_NEXT(q, link);
			debug("canceling pending query %p, belonging to %p", q,
			      current_lookup);
			q->canceled = true;
			if (q->readhandle != NULL &&
			    !isc_nm_is_http_handle(q->readhandle))
			{
				isc_nm_cancelread(q->readhandle);
			}
			query_detach(&q);
		}

		/*
		 * current_lookup could have been detached via query_detach().
		 */
		if (current_lookup != NULL) {
			lookup_detach(&current_lookup);
		}
	}
	l = ISC_LIST_HEAD(lookup_list);
	while (l != NULL) {
		n = ISC_LIST_NEXT(l, link);
		ISC_LIST_DEQUEUE(lookup_list, l, link);
		lookup_detach(&l);
		l = n;
	}
	UNLOCK_LOOKUP;
}

void
cleanup_openssl_refs(void) {
	if (tsigkey != NULL) {
		debug("freeing TSIG key %p", tsigkey);
		dns_tsigkey_detach(&tsigkey);
	}

	if (sig0key != NULL) {
		debug("freeing SIG(0) key %p", sig0key);
		dst_key_free(&sig0key);
	}

	if (is_dst_up) {
		debug("destroy DST lib");
		dst_lib_destroy();
		is_dst_up = false;
	}
}

/*%
 * Destroy all of the libs we are using, and get everything ready for a
 * clean shutdown.
 */
void
destroy_libs(void) {
	if (keep != NULL) {
		isc_nmhandle_detach(&keep);
	}
	debug("destroy_libs()");
	if (global_task != NULL) {
		debug("freeing task");
		isc_task_detach(&global_task);
	}

	isc_managers_destroy(&netmgr, &taskmgr, NULL);

	LOCK_LOOKUP;
	isc_refcount_destroy(&recvcount);
	isc_refcount_destroy(&sendcount);

	INSIST(ISC_LIST_HEAD(lookup_list) == NULL);
	INSIST(current_lookup == NULL);
	INSIST(!free_now);

	free_now = true;

	flush_server_list();

	clear_searchlist();

	cleanup_openssl_refs();

	if (namebuf != NULL) {
		debug("freeing key %p", tsigkey);
		isc_buffer_free(&namebuf);
	}

	UNLOCK_LOOKUP;
	isc_mutex_destroy(&lookup_lock);

	debug("Removing log context");
	isc_log_destroy(&lctx);

	debug("Destroy memory");
	if (memdebugging != 0) {
		isc_mem_stats(mctx, stderr);
	}
	if (mctx != NULL) {
		isc_mem_destroy(&mctx);
	}
}

#ifdef HAVE_LIBIDN2
static isc_result_t
idn_output_filter(isc_buffer_t *buffer, unsigned int used_org) {
	char src[MXNAME], *dst = NULL;
	size_t srclen, dstlen;
	isc_result_t result = ISC_R_SUCCESS;

	/*
	 * Copy name from 'buffer' to 'src' and terminate it with NULL.
	 */
	srclen = isc_buffer_usedlength(buffer) - used_org;
	if (srclen >= sizeof(src)) {
		warn("Input name too long to perform IDN conversion");
		goto cleanup;
	}
	memmove(src, (char *)isc_buffer_base(buffer) + used_org, srclen);
	src[srclen] = '\0';

	systemlocale(LC_ALL);

	/*
	 * Convert 'src' to the current locale's character encoding.
	 */
	idn_ace_to_locale(src, &dst);

	resetlocale(LC_ALL);

	/*
	 * Check whether the converted name will fit back into 'buffer'.
	 */
	dstlen = strlen(dst);
	if (isc_buffer_length(buffer) < used_org + dstlen) {
		result = ISC_R_NOSPACE;
		goto cleanup;
	}

	/*
	 * Put the converted name back into 'buffer'.
	 */
	isc_buffer_subtract(buffer, srclen);
	memmove(isc_buffer_used(buffer), dst, dstlen);
	isc_buffer_add(buffer, dstlen);

	/*
	 * Clean up.
	 */
cleanup:
	if (dst != NULL) {
		idn2_free(dst);
	}

	return (result);
}

/*%
 * Convert 'src', which is a string using the current locale's character
 * encoding, into an ACE string suitable for use in the DNS, storing the
 * conversion result in 'dst', which is 'dstlen' bytes large.
 *
 * 'dst' MUST be large enough to hold any valid domain name.
 */
static void
idn_locale_to_ace(const char *src, char *dst, size_t dstlen) {
	const char *final_src;
	char *ascii_src;
	int res;

	systemlocale(LC_ALL);

	/*
	 * We trust libidn2 to return an error if 'src' is too large to be a
	 * valid domain name.
	 */
	res = idn2_to_ascii_lz(src, &ascii_src, IDN2_NONTRANSITIONAL);
	if (res == IDN2_DISALLOWED) {
		res = idn2_to_ascii_lz(src, &ascii_src, IDN2_TRANSITIONAL);
	}
	if (res != IDN2_OK) {
		fatal("'%s' is not a legal IDNA2008 name (%s), use +noidnin",
		      src, idn2_strerror(res));
	}

	/*
	 * idn2_to_ascii_lz() normalizes all strings to lower case, but we
	 * generally don't want to lowercase all input strings; make sure to
	 * return the original case if the two strings differ only in case.
	 */
	final_src = (strcasecmp(src, ascii_src) == 0 ? src : ascii_src);

	(void)strlcpy(dst, final_src, dstlen);

	idn2_free(ascii_src);

	resetlocale(LC_ALL);
}

/*%
 * Convert 'src', which is an ACE string suitable for use in the DNS, into a
 * string using the current locale's character encoding, storing the conversion
 * result in 'dst'.
 *
 * The caller MUST subsequently release 'dst' using idn2_free().
 */
static void
idn_ace_to_locale(const char *src, char **dst) {
	char *local_src, *utf8_src;
	int res;

	systemlocale(LC_ALL);

	/*
	 * We need to:
	 *
	 *  1) check whether 'src' is a valid IDNA2008 name,
	 *  2) if it is, output it in the current locale's character encoding.
	 *
	 * Unlike idn2_to_ascii_*(), idn2_to_unicode_*() functions are unable
	 * to perform IDNA2008 validity checks.  Thus, we need to decode any
	 * Punycode in 'src', check if the resulting name is a valid IDNA2008
	 * name, and only once we ensure it is, output that name in the current
	 * locale's character encoding.
	 *
	 * We could just use idn2_to_unicode_8zlz() + idn2_to_ascii_lz(), but
	 * then we would not be able to universally tell invalid names and
	 * character encoding errors apart (if the current locale uses ASCII
	 * for character encoding, the former function would fail even for a
	 * valid IDNA2008 name, as long as it contained any non-ASCII
	 * character).  Thus, we need to take a longer route.
	 *
	 * First, convert 'src' to UTF-8, ignoring the current locale.
	 */
	res = idn2_to_unicode_8z8z(src, &utf8_src, 0);
	if (res != IDN2_OK) {
		fatal("Bad ACE string '%s' (%s), use +noidnout", src,
		      idn2_strerror(res));
	}

	/*
	 * Then, check whether decoded 'src' is a valid IDNA2008 name
	 * and if disallowed character is found, fallback to IDNA2003.
	 */
	res = idn2_to_ascii_8z(utf8_src, NULL, IDN2_NONTRANSITIONAL);
	if (res == IDN2_DISALLOWED) {
		res = idn2_to_ascii_8z(utf8_src, NULL, IDN2_TRANSITIONAL);
	}
	if (res != IDN2_OK) {
		fatal("'%s' is not a legal IDNA2008 name (%s), use +noidnout",
		      src, idn2_strerror(res));
	}

	/*
	 * Finally, try converting the decoded 'src' into the current locale's
	 * character encoding.
	 */
	res = idn2_to_unicode_8zlz(utf8_src, &local_src, 0);
	if (res != IDN2_OK) {
		static bool warned = false;

		res = idn2_to_ascii_8z(utf8_src, &local_src, 0);
		if (res != IDN2_OK) {
			fatal("Cannot represent '%s' "
			      "in the current locale nor ascii (%s), "
			      "use +noidnout or a different locale",
			      src, idn2_strerror(res));
		} else if (!warned) {
			fprintf(stderr,
				";; Warning: cannot represent '%s' "
				"in the current locale",
				local_src);
			warned = true;
		}
	}

	/*
	 * Free the interim conversion result.
	 */
	idn2_free(utf8_src);

	*dst = local_src;

	resetlocale(LC_ALL);
}
#endif /* HAVE_LIBIDN2 */

void
dig_idnsetup(dig_lookup_t *lookup, bool active) {
#ifdef HAVE_LIBIDN2
	isc_result_t result;
	result = dns_name_settotextfilter(
		(active && lookup->idnout) ? idn_output_filter : NULL);
	check_result(result, "dns_name_settotextfilter");
#else
	UNUSED(lookup);
	UNUSED(active);
	return;
#endif /* HAVE_LIBIDN2 */
}

bool
dig_lookup_is_tls(const dig_lookup_t *lookup) {
	if (lookup->tls_mode || (lookup->tls_ca_set && !lookup->https_mode)) {
		return (true);
	}

	return (false);
}
