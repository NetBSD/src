/*	$NetBSD: mdig.c,v 1.1.1.1 2018/08/12 12:07:15 christos Exp $	*/

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

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <isc/app.h>
#include <isc/base64.h>
#include <isc/entropy.h>
#include <isc/hash.h>
#include <isc/hex.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/net.h>
#include <isc/parseint.h>
#include <isc/print.h>
#include <isc/sockaddr.h>
#include <isc/socket.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/byaddr.h>
#include <dns/dispatch.h>
#include <dns/fixedname.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatatype.h>
#include <dns/rdataclass.h>
#include <dns/request.h>
#include <dns/result.h>
#include <dns/view.h>

#include <dns/events.h>
#include <dns/rdataset.h>
#include <dns/resolver.h>
#include <dns/types.h>

#include <dst/result.h>

#include <bind9/getaddresses.h>

#define CHECK(str, x) { \
	if ((x) != ISC_R_SUCCESS) { \
		fprintf(stderr, "mdig: %s failed with %s\n",	\
			(str), isc_result_totext(x));	\
		exit(-1); \
	} \
}

#define RUNCHECK(x) RUNTIME_CHECK((x) == ISC_R_SUCCESS)

#define ADD_STRING(b, s) { 				\
	if (strlen(s) >= isc_buffer_availablelength(b)) \
		return (ISC_R_NOSPACE); 		\
	else 						\
		isc_buffer_putstr(b, s); 		\
}

#define MXNAME (DNS_NAME_MAXTEXT + 1)
#define COMMSIZE 0xffff
#define OUTPUTBUF 32767
#define MAXPORT 0xffff
#define PORT 53
#define MAXTIMEOUT 0xffff
#define TCPTIMEOUT 10
#define UDPTIMEOUT 5
#define MAXTRIES 0xffffffff

static isc_mem_t *mctx;
static dns_requestmgr_t *requestmgr;
static const char *batchname;
static FILE *batchfp;
static isc_boolean_t have_ipv4 = ISC_FALSE;
static isc_boolean_t have_ipv6 = ISC_FALSE;
static isc_boolean_t have_src = ISC_FALSE;
static isc_boolean_t tcp_mode = ISC_FALSE;
static isc_boolean_t besteffort = ISC_TRUE;
static isc_boolean_t display_short_form = ISC_FALSE;
static isc_boolean_t display_headers = ISC_TRUE;
static isc_boolean_t display_comments = ISC_TRUE;
static isc_boolean_t display_rrcomments = ISC_TRUE;
static isc_boolean_t display_ttlunits = ISC_TRUE;
static isc_boolean_t display_ttl = ISC_TRUE;
static isc_boolean_t display_class = ISC_TRUE;
static isc_boolean_t display_crypto = ISC_TRUE;
static isc_boolean_t display_multiline = ISC_FALSE;
static isc_boolean_t display_question = ISC_TRUE;
static isc_boolean_t display_answer  = ISC_TRUE;
static isc_boolean_t display_authority = ISC_TRUE;
static isc_boolean_t display_additional = ISC_TRUE;
static isc_boolean_t display_unknown_format = ISC_FALSE;
static isc_boolean_t continue_on_error = ISC_FALSE;
static isc_uint32_t display_splitwidth = 0xffffffff;
static isc_sockaddr_t srcaddr;
static char *server;
static isc_sockaddr_t dstaddr;
static in_port_t port = 53;
static isc_dscp_t dscp = -1;
static unsigned char cookie_secret[33];
static int onfly = 0;
static char hexcookie[81];

struct query {
	char textname[MXNAME]; /*% Name we're going to be looking up */
	isc_boolean_t ip6_int;
	isc_boolean_t recurse;
	isc_boolean_t have_aaonly;
	isc_boolean_t have_adflag;
	isc_boolean_t have_cdflag;
	isc_boolean_t have_zflag;
	isc_boolean_t dnssec;
	isc_boolean_t expire;
	isc_boolean_t send_cookie;
	char *cookie;
	isc_boolean_t nsid;
	dns_rdatatype_t rdtype;
	dns_rdataclass_t rdclass;
	isc_uint16_t udpsize;
	isc_int16_t edns;
	dns_ednsopt_t *ednsopts;
	unsigned int ednsoptscnt;
	unsigned int ednsflags;
	isc_sockaddr_t *ecs_addr;
	unsigned int timeout;
	unsigned int udptimeout;
	unsigned int udpretries;
	ISC_LINK(struct query) link;
};
static struct query default_query;
static ISC_LIST(struct query) queries;

#define EDNSOPTS 100U
/*% opcode text */
static const char * const opcodetext[] = {
	"QUERY",
	"IQUERY",
	"STATUS",
	"RESERVED3",
	"NOTIFY",
	"UPDATE",
	"RESERVED6",
	"RESERVED7",
	"RESERVED8",
	"RESERVED9",
	"RESERVED10",
	"RESERVED11",
	"RESERVED12",
	"RESERVED13",
	"RESERVED14",
	"RESERVED15"
};

/*% return code text */
static const char * const rcodetext[] = {
	"NOERROR",
	"FORMERR",
	"SERVFAIL",
	"NXDOMAIN",
	"NOTIMP",
	"REFUSED",
	"YXDOMAIN",
	"YXRRSET",
	"NXRRSET",
	"NOTAUTH",
	"NOTZONE",
	"RESERVED11",
	"RESERVED12",
	"RESERVED13",
	"RESERVED14",
	"RESERVED15",
	"BADVERS"
};

/*% safe rcodetext[] */
static char *
rcode_totext(dns_rcode_t rcode)
{
	static char buf[sizeof("?65535")];
	union {
		const char *consttext;
		char *deconsttext;
	} totext;

	if (rcode >= (sizeof(rcodetext)/sizeof(rcodetext[0]))) {
		snprintf(buf, sizeof(buf), "?%u", rcode);
		totext.deconsttext = buf;
	} else
		totext.consttext = rcodetext[rcode];
	return totext.deconsttext;
}

/* receive response event handler */
static void
recvresponse(isc_task_t *task, isc_event_t *event) {
	dns_requestevent_t *reqev = (dns_requestevent_t *)event;
	isc_result_t result;
	dns_message_t *query = NULL, *response = NULL;
	unsigned int parseflags = 0;
	isc_buffer_t *buf = NULL;
	unsigned int len = OUTPUTBUF;
	dns_master_style_t *style = NULL;
	unsigned int styleflags = 0;
	dns_messagetextflag_t flags;

	UNUSED(task);

	REQUIRE(reqev != NULL);
	query = reqev->ev_arg;

	if (reqev->result != ISC_R_SUCCESS) {
		fprintf(stderr, "response failed with %s\n",
			isc_result_totext(reqev->result));
		if (continue_on_error)
			goto cleanup;
		else
			exit(-1);
	}

	result = dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &response);
	CHECK("dns_message_create", result);

	parseflags |= DNS_MESSAGEPARSE_PRESERVEORDER;
	if (besteffort) {
		parseflags |= DNS_MESSAGEPARSE_BESTEFFORT;
		parseflags |= DNS_MESSAGEPARSE_IGNORETRUNCATION;
	}
	result = dns_request_getresponse(reqev->request, response, parseflags);
	CHECK("dns_request_getresponse", result);

	styleflags |= DNS_STYLEFLAG_REL_OWNER;
	if (display_comments)
		styleflags |= DNS_STYLEFLAG_COMMENT;
	if (display_unknown_format)
		styleflags |= DNS_STYLEFLAG_UNKNOWNFORMAT;
	if (display_rrcomments)
		styleflags |= DNS_STYLEFLAG_RRCOMMENT;
	if (display_ttlunits)
		styleflags |= DNS_STYLEFLAG_TTL_UNITS;
	if (!display_ttl)
		styleflags |= DNS_STYLEFLAG_NO_TTL;
	if (!display_class)
		styleflags |= DNS_STYLEFLAG_NO_CLASS;
	if (!display_crypto)
		styleflags |= DNS_STYLEFLAG_NOCRYPTO;
	if (display_multiline) {
		styleflags |= DNS_STYLEFLAG_OMIT_OWNER;
		styleflags |= DNS_STYLEFLAG_OMIT_CLASS;
		styleflags |= DNS_STYLEFLAG_REL_DATA;
		styleflags |= DNS_STYLEFLAG_OMIT_TTL;
		styleflags |= DNS_STYLEFLAG_TTL;
		styleflags |= DNS_STYLEFLAG_MULTILINE;
		styleflags |= DNS_STYLEFLAG_COMMENT;
		styleflags |= DNS_STYLEFLAG_RRCOMMENT;
	}
	if (display_multiline || (!display_ttl && !display_class))
		result = dns_master_stylecreate2(&style, styleflags,
						 24, 24, 24, 32, 80, 8,
						 display_splitwidth, mctx);
	else if (!display_ttl || !display_class)
		result = dns_master_stylecreate2(&style, styleflags,
						 24, 24, 32, 40, 80, 8,
						 display_splitwidth, mctx);
	else
		result = dns_master_stylecreate2(&style, styleflags,
						 24, 32, 40, 48, 80, 8,
						 display_splitwidth, mctx);
	CHECK("dns_master_stylecreate2", result);

	flags = 0;
	if (!display_headers) {
		flags |= DNS_MESSAGETEXTFLAG_NOHEADERS;
		flags |= DNS_MESSAGETEXTFLAG_NOCOMMENTS;
	}
	if (!display_comments)
		flags |= DNS_MESSAGETEXTFLAG_NOCOMMENTS;

	result = isc_buffer_allocate(mctx, &buf, len);
	CHECK("isc_buffer_allocate", result);

	if (display_comments && !display_short_form) {
		printf(";; Got answer:\n");

		if (display_headers) {
			printf(";; ->>HEADER<<- opcode: %s, status: %s, "
			       "id: %u\n",
			       opcodetext[response->opcode],
			       rcode_totext(response->rcode),
			       response->id);
			printf(";; flags:");
			if ((response->flags & DNS_MESSAGEFLAG_QR) != 0)
				printf(" qr");
			if ((response->flags & DNS_MESSAGEFLAG_AA) != 0)
				printf(" aa");
			if ((response->flags & DNS_MESSAGEFLAG_TC) != 0)
				printf(" tc");
			if ((response->flags & DNS_MESSAGEFLAG_RD) != 0)
				printf(" rd");
			if ((response->flags & DNS_MESSAGEFLAG_RA) != 0)
				printf(" ra");
			if ((response->flags & DNS_MESSAGEFLAG_AD) != 0)
				printf(" ad");
			if ((response->flags & DNS_MESSAGEFLAG_CD) != 0)
				printf(" cd");
			if ((response->flags & 0x0040U) != 0)
				printf("; MBZ: 0x4");

			printf("; QUERY: %u, ANSWER: %u, "
			       "AUTHORITY: %u, ADDITIONAL: %u\n",
			       response->counts[DNS_SECTION_QUESTION],
			       response->counts[DNS_SECTION_ANSWER],
			       response->counts[DNS_SECTION_AUTHORITY],
			       response->counts[DNS_SECTION_ADDITIONAL]);

			if ((response->flags & DNS_MESSAGEFLAG_RD) != 0 &&
			    (response->flags & DNS_MESSAGEFLAG_RA) == 0)
				printf(";; WARNING: recursion requested "
				       "but not available\n");
		}
	}

repopulate_buffer:

	if (display_comments && display_headers && !display_short_form) {
		result = dns_message_pseudosectiontotext(response,
							 DNS_PSEUDOSECTION_OPT,
							 style, flags, buf);
		if (result == ISC_R_NOSPACE) {
buftoosmall:
			len += OUTPUTBUF;
			isc_buffer_free(&buf);
			result = isc_buffer_allocate(mctx, &buf, len);
			if (result == ISC_R_SUCCESS)
				goto repopulate_buffer;
			else
				goto cleanup;
		}
		CHECK("dns_message_pseudosectiontotext", result);
	}

	if (display_question && display_headers && !display_short_form) {
		result = dns_message_sectiontotext(response,
						   DNS_SECTION_QUESTION,
						   style, flags, buf);
		if (result == ISC_R_NOSPACE)
			goto buftoosmall;
		CHECK("dns_message_sectiontotext", result);
	}

	if (display_answer && !display_short_form) {
		result = dns_message_sectiontotext(response,
						   DNS_SECTION_ANSWER,
						   style, flags, buf);
		if (result == ISC_R_NOSPACE)
			goto buftoosmall;
		CHECK("dns_message_sectiontotext", result);
	} else if (display_answer) {
		dns_name_t *name;
		dns_rdataset_t *rdataset;
		isc_result_t loopresult;
		dns_name_t empty_name;
		dns_rdata_t rdata = DNS_RDATA_INIT;
		unsigned int answerstyleflags = 0;

		if (!display_crypto)
			answerstyleflags |= DNS_STYLEFLAG_NOCRYPTO;
		if (display_unknown_format)
			answerstyleflags |= DNS_STYLEFLAG_UNKNOWNFORMAT;

		dns_name_init(&empty_name, NULL);
		result = dns_message_firstname(response, DNS_SECTION_ANSWER);
		if (result != ISC_R_NOMORE)
			CHECK("dns_message_firstname", result);

		for (;;) {
			if (result == ISC_R_NOMORE)
				break;
			CHECK("dns_message_nextname", result);
			name = NULL;
			dns_message_currentname(response,
						DNS_SECTION_ANSWER,
						&name);

			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				loopresult = dns_rdataset_first(rdataset);
				while (loopresult == ISC_R_SUCCESS) {
					dns_rdataset_current(rdataset, &rdata);
					result = dns_rdata_tofmttext(
							&rdata,
							NULL,
							answerstyleflags,
							0, 60, " ", buf);
					if (result == ISC_R_NOSPACE)
						goto buftoosmall;
					CHECK("dns_rdata_tofmttext", result);
					loopresult =
					    dns_rdataset_next(rdataset);
					dns_rdata_reset(&rdata);
					if (strlen("\n") >=
					    isc_buffer_availablelength(buf))
						goto buftoosmall;
					isc_buffer_putstr(buf, "\n");
				}
			}
			result = dns_message_nextname(response,
						      DNS_SECTION_ANSWER);
		}
	}

	if (display_authority && !display_short_form) {
		result = dns_message_sectiontotext(response,
						   DNS_SECTION_AUTHORITY,
						   style, flags, buf);
		if (result == ISC_R_NOSPACE)
			goto buftoosmall;
		CHECK("dns_message_sectiontotext", result);
	}

	if (display_additional && !display_short_form) {
		result = dns_message_sectiontotext(response,
						   DNS_SECTION_ADDITIONAL,
						   style, flags, buf);
		if (result == ISC_R_NOSPACE)
			goto buftoosmall;
		CHECK("dns_message_sectiontotext", result);
	}


	if (display_additional && !display_short_form && display_headers) {
		/*
		 * Only print the signature on the first record.
		 */
		result =
		    dns_message_pseudosectiontotext(response,
						    DNS_PSEUDOSECTION_TSIG,
						    style, flags, buf);
		if (result == ISC_R_NOSPACE)
			goto buftoosmall;
		CHECK("dns_message_pseudosectiontotext", result);
		result =
		    dns_message_pseudosectiontotext(response,
						    DNS_PSEUDOSECTION_SIG0,
						    style, flags, buf);
		if (result == ISC_R_NOSPACE)
			goto buftoosmall;
		CHECK("dns_message_pseudosectiontotext", result);
	}

	if (display_headers && display_comments && !display_short_form)
		printf("\n");

	printf("%.*s", (int)isc_buffer_usedlength(buf),
	       (char *)isc_buffer_base(buf));
	isc_buffer_free(&buf);

cleanup:
	fflush(stdout);
	if (style != NULL)
		dns_master_styledestroy(&style, mctx);
	if (query != NULL)
		dns_message_destroy(&query);
	if (response != NULL)
		dns_message_destroy(&response);
	dns_request_destroy(&reqev->request);
	isc_event_free(&event);

	if (--onfly == 0)
		isc_app_shutdown();
	return;
}

/*%
 * Add EDNS0 option record to a message.  Currently, the only supported
 * options are UDP buffer size, the DO bit, and EDNS options
 * (e.g., NSID, COOKIE, client-subnet)
 */
static void
add_opt(dns_message_t *msg, isc_uint16_t udpsize, isc_uint16_t edns,
	unsigned int flags, dns_ednsopt_t *opts, size_t count)
{
	dns_rdataset_t *rdataset = NULL;
	isc_result_t result;

	result = dns_message_buildopt(msg, &rdataset, edns, udpsize, flags,
				      opts, count);
	CHECK("dns_message_buildopt", result);
	result = dns_message_setopt(msg, rdataset);
	CHECK("dns_message_setopt", result);
}

static void
compute_cookie(unsigned char *cookie, size_t len) {
	/* XXXMPA need to fix, should be per server. */
	INSIST(len >= 8U);
	memmove(cookie, cookie_secret, 8);
}

static isc_result_t
sendquery(struct query *query, isc_task_t *task)
{
	dns_request_t *request;
	dns_message_t *message;
	dns_name_t *qname;
	dns_rdataset_t *qrdataset;
	isc_result_t result;
	dns_fixedname_t queryname;
	isc_buffer_t buf;
	unsigned int options;

	onfly++;

	dns_fixedname_init(&queryname);
	isc_buffer_init(&buf, query->textname, strlen(query->textname));
	isc_buffer_add(&buf, strlen(query->textname));
	result = dns_name_fromtext(dns_fixedname_name(&queryname), &buf,
				   dns_rootname, 0, NULL);
	CHECK("dns_name_fromtext", result);

	message = NULL;
	result = dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER, &message);
	CHECK("dns_message_create", result);

	message->opcode = dns_opcode_query;
	if (query->recurse)
		message->flags |= DNS_MESSAGEFLAG_RD;
	if (query->have_aaonly)
		message->flags |= DNS_MESSAGEFLAG_AA;
	if (query->have_adflag)
		message->flags |= DNS_MESSAGEFLAG_AD;
	if (query->have_cdflag)
		message->flags |= DNS_MESSAGEFLAG_CD;
	if (query->have_zflag)
		message->flags |= 0x0040U;
	message->rdclass = query->rdclass;
	message->id = (unsigned short)(random() & 0xFFFF);

	qname = NULL;
	result = dns_message_gettempname(message, &qname);
	CHECK("dns_message_gettempname", result);

	qrdataset = NULL;
	result = dns_message_gettemprdataset(message, &qrdataset);
	CHECK("dns_message_gettemprdataset", result);

	dns_name_init(qname, NULL);
	dns_name_clone(dns_fixedname_name(&queryname), qname);
	dns_rdataset_makequestion(qrdataset, query->rdclass,
				  query->rdtype);
	ISC_LIST_APPEND(qname->list, qrdataset, link);
	dns_message_addname(message, qname, DNS_SECTION_QUESTION);

	if (query->udpsize > 0 || query->dnssec ||
	    query->edns > -1 || query->ecs_addr != NULL)
	{
		dns_ednsopt_t opts[EDNSOPTS + DNS_EDNSOPTIONS];
		unsigned int flags;
		int i = 0;
		char ecsbuf[20];
		unsigned char cookie[40];

		if (query->udpsize == 0)
			query->udpsize = 4096;
		if (query->edns < 0)
			query->edns = 0;

		if (query->nsid) {
			INSIST(i < DNS_EDNSOPTIONS);
			opts[i].code = DNS_OPT_NSID;
			opts[i].length = 0;
			opts[i].value = NULL;
			i++;
		}

		if (query->ecs_addr != NULL) {
			isc_uint8_t addr[16], family;
			isc_uint32_t plen;
			struct sockaddr *sa;
			struct sockaddr_in *sin;
			struct sockaddr_in6 *sin6;
			size_t addrl;
			isc_buffer_t b;

			sa = &query->ecs_addr->type.sa;
			plen = query->ecs_addr->length;

			/* Round up prefix len to a multiple of 8 */
			addrl = (plen + 7) / 8;

			INSIST(i < DNS_EDNSOPTIONS);
			opts[i].code = DNS_OPT_CLIENT_SUBNET;
			opts[i].length = (isc_uint16_t) addrl + 4;
			CHECK("isc_buffer_allocate", result);
			isc_buffer_init(&b, ecsbuf, sizeof(ecsbuf));
			if (sa->sa_family == AF_INET) {
				family = 1;
				sin = (struct sockaddr_in *) sa;
				memmove(addr, &sin->sin_addr, 4);
				if ((plen % 8) != 0)
					addr[addrl-1] &=
						~0U << (8 - (plen % 8));
			} else {
				family = 2;
				sin6 = (struct sockaddr_in6 *) sa;
				memmove(addr, &sin6->sin6_addr, 16);
			}

			/* Mask off last address byte */
			if (addrl > 0 && (plen % 8) != 0)
				addr[addrl - 1] &= ~0U << (8 - (plen % 8));

			/* family */
			isc_buffer_putuint16(&b, family);
			/* source prefix-length */
			isc_buffer_putuint8(&b, plen);
			/* scope prefix-length */
			isc_buffer_putuint8(&b, 0);
			/* address */
			if (addrl > 0)
				isc_buffer_putmem(&b, addr,
						  (unsigned)addrl);

			opts[i].value = (isc_uint8_t *) ecsbuf;
			i++;
		}

		if (query->send_cookie) {
			INSIST(i < DNS_EDNSOPTIONS);
			opts[i].code = DNS_OPT_COOKIE;
			if (query->cookie != NULL) {
				isc_buffer_t b;

				isc_buffer_init(&b, cookie, sizeof(cookie));
				result = isc_hex_decodestring(query->cookie,
							      &b);
				CHECK("isc_hex_decodestring", result);
				opts[i].value = isc_buffer_base(&b);
				opts[i].length = isc_buffer_usedlength(&b);
			} else {
				compute_cookie(cookie, 8);
				opts[i].length = 8;
				opts[i].value = cookie;
			}
			i++;
		}

		if (query->expire) {
			INSIST(i < DNS_EDNSOPTIONS);
			opts[i].code = DNS_OPT_EXPIRE;
			opts[i].length = 0;
			opts[i].value = NULL;
			i++;
		}

		if (query->ednsoptscnt != 0) {
			memmove(&opts[i], query->ednsopts,
				sizeof(dns_ednsopt_t) * query->ednsoptscnt);
			i += query->ednsoptscnt;
		}

		flags = query->ednsflags;
		flags &= ~DNS_MESSAGEEXTFLAG_DO;
		if (query->dnssec)
			flags |= DNS_MESSAGEEXTFLAG_DO;
		add_opt(message, query->udpsize, query->edns, flags, opts, i);
	}

	options = 0;
	if (tcp_mode)
		options |= DNS_REQUESTOPT_TCP | DNS_REQUESTOPT_SHARE;
	request = NULL;
	result = dns_request_createvia4(requestmgr, message,
					have_src ? &srcaddr : NULL, &dstaddr,
					dscp, options, NULL,
					query->timeout, query->udptimeout,
					query->udpretries, task,
					recvresponse, message, &request);
	CHECK("dns_request_createvia4", result);

	return ISC_R_SUCCESS;
}

static void
sendqueries(isc_task_t *task, isc_event_t *event)
{
	struct query *query = (struct query *)event->ev_arg;

	isc_event_free(&event);

	while (query != NULL) {
		struct query *next = ISC_LIST_NEXT(query, link);

		sendquery(query, task);
		query = next;
	}

	if (onfly == 0)
		isc_app_shutdown();
	return;
}

ISC_PLATFORM_NORETURN_PRE static void
usage(void) ISC_PLATFORM_NORETURN_POST;

static void
usage(void) {
	fputs("Usage: mdig @server {global-opt} host\n"
	      "           {local-opt} [ host {local-opt} [...]]\n",
	      stderr);
	fputs("\nUse \"mdig -h\" (or \"mdig -h | more\") "
	      "for complete list of options\n",
	      stderr);
	exit(1);
}

/*% help */
static void
help(void) {
	fputs("Usage: mdig @server {global-opt} host\n"
	      "           {local-opt} [ host {local-opt} [...]]\n",
	      stdout);
	fputs(
"Where:\n"
" anywhere opt    is one of:\n"
"                 -f filename         (batch mode)\n"
"                 -h                  (print help and exit)\n"
"                 -v                  (print version and exit)\n"
" global opt      is one of:\n"
"                 -4                  (use IPv4 query transport only)\n"
"                 -6                  (use IPv6 query transport only)\n"
"                 -b address[#port]   (bind to source address/port)\n"
"                 -p port             (specify port number)\n"
"                 -m                  (enable memory usage debugging)\n"
"                 +[no]dscp[=###]     (Set the DSCP value to ### [0..63])\n"
"                 +[no]vc             (TCP mode)\n"
"                 +[no]tcp            (TCP mode, alternate syntax)\n"
"                 +[no]besteffort     (Try to parse even illegal messages)\n"
"                 +[no]cl             (Control display of class in records)\n"
"                 +[no]comments       (Control display of comment lines)\n"
"                 +[no]rrcomments     (Control display of per-record "
				       "comments)\n"
"                 +[no]crypto         (Control display of cryptographic "
				       "fields in records)\n"
"                 +[no]question       (Control display of question)\n"
"                 +[no]answer         (Control display of answer)\n"
"                 +[no]authority      (Control display of authority)\n"
"                 +[no]additional     (Control display of additional)\n"
"                 +[no]short          (Disable everything except short\n"
"                                      form of answer)\n"
"                 +[no]ttlid          (Control display of ttls in records)\n"
"                 +[no]ttlunits       (Display TTLs in human-readable units)\n"
"                 +[no]unknownformat  (Print RDATA in RFC 3597 \"unknown\" format)\n"
"                 +[no]all            (Set or clear all display flags)\n"
"                 +[no]multiline      (Print records in an expanded format)\n"
"                 +[no]split=##       (Split hex/base64 fields into chunks)\n"
" local opt       is one of:\n"
"                 -c class            (specify query class)\n"
"                 -t type             (specify query type)\n"
"                 -i                  (use IP6.INT for IPv6 reverse lookups)\n"
"                 -x dot-notation     (shortcut for reverse lookups)\n"
"                 +timeout=###        (Set query timeout) [UDP=5,TCP=10]\n"
"                 +udptimeout=###     (Set timeout before UDP retry)\n"
"                 +tries=###          (Set number of UDP attempts) [3]\n"
"                 +retry=###          (Set number of UDP retries) [2]\n"
"                 +bufsize=###        (Set EDNS0 Max UDP packet size)\n"
"                 +subnet=addr        (Set edns-client-subnet option)\n"
"                 +[no]edns[=###]     (Set EDNS version) [0]\n"
"                 +ednsflags=###      (Set EDNS flag bits)\n"
"                 +ednsopt=###[:value] (Send specified EDNS option)\n"
"                 +noednsopt          (Clear list of +ednsopt options)\n"
"                 +[no]recurse        (Recursive mode)\n"
"                 +[no]aaonly         (Set AA flag in query (+[no]aaflag))\n"
"                 +[no]adflag         (Set AD flag in query)\n"
"                 +[no]cdflag         (Set CD flag in query)\n"
"                 +[no]zflag          (Set Z flag in query)\n"
"                 +[no]dnssec         (Request DNSSEC records)\n"
"                 +[no]expire         (Request time to expire)\n"
"                 +[no]cookie[=###]   (Send a COOKIE option)\n"
"                 +[no]nsid           (Request Name Server ID)\n",
	stdout);
}

static char *
next_token(char **stringp, const char *delim) {
	char *res;

	do {
		res = strsep(stringp, delim);
		if (res == NULL)
			break;
	} while (*res == '\0');
	return (res);
}

ISC_PLATFORM_NORETURN_PRE static void
fatal(const char *format, ...)
ISC_FORMAT_PRINTF(1, 2) ISC_PLATFORM_NORETURN_POST;

static void
fatal(const char *format, ...) {
	va_list args;

	fflush(stdout);
	fprintf(stderr, "mdig: ");
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(-2);
}

static isc_result_t
parse_uint_helper(isc_uint32_t *uip, const char *value, isc_uint32_t max,
		  const char *desc, int base) {
	isc_uint32_t n;
	isc_result_t result = isc_parse_uint32(&n, value, base);
	if (result == ISC_R_SUCCESS && n > max)
		result = ISC_R_RANGE;
	if (result != ISC_R_SUCCESS) {
		printf("invalid %s '%s': %s\n", desc,
		       value, isc_result_totext(result));
		return (result);
	}
	*uip = n;
	return (ISC_R_SUCCESS);
}

static isc_result_t
parse_uint(isc_uint32_t *uip, const char *value, isc_uint32_t max,
	   const char *desc) {
	return (parse_uint_helper(uip, value, max, desc, 10));
}

static isc_result_t
parse_xint(isc_uint32_t *uip, const char *value, isc_uint32_t max,
	   const char *desc) {
	return (parse_uint_helper(uip, value, max, desc, 0));
}

static dns_ednsopt_t ednsopts[EDNSOPTS];
static unsigned char ednsoptscnt = 0;

static void
save_opt(struct query *query, char *code, char *value) {
	isc_uint32_t num;
	isc_buffer_t b;
	isc_result_t result;

	if (ednsoptscnt == EDNSOPTS)
		fatal("too many ednsopts");

	result = parse_uint(&num, code, 65535, "ednsopt");
	if (result != ISC_R_SUCCESS)
		fatal("bad edns code point: %s", code);

	ednsopts[ednsoptscnt].code = num;
	ednsopts[ednsoptscnt].length = 0;
	ednsopts[ednsoptscnt].value = NULL;

	if (value != NULL) {
		char *buf;
		buf = isc_mem_allocate(mctx, strlen(value)/2 + 1);
		if (buf == NULL)
			fatal("out of memory");
		isc_buffer_init(&b, buf, strlen(value)/2 + 1);
		result = isc_hex_decodestring(value, &b);
		CHECK("isc_hex_decodestring", result);
		ednsopts[ednsoptscnt].value = isc_buffer_base(&b);
		ednsopts[ednsoptscnt].length = isc_buffer_usedlength(&b);
	}

	if (query->ednsoptscnt == 0)
		query->ednsopts = &ednsopts[ednsoptscnt];
	query->ednsoptscnt++;
	ednsoptscnt++;
}

static isc_result_t
parse_netprefix(isc_sockaddr_t **sap, const char *value) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_sockaddr_t *sa = NULL;
	struct in_addr in4;
	struct in6_addr in6;
	isc_uint32_t netmask = 0xffffffff;
	char *slash = NULL;
	isc_boolean_t parsed = ISC_FALSE;
	char buf[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:XXX.XXX.XXX.XXX/128")];

	if (strlcpy(buf, value, sizeof(buf)) >= sizeof(buf))
		fatal("invalid prefix '%s'\n", value);

	slash = strchr(buf, '/');
	if (slash != NULL) {
		*slash = '\0';
		result = isc_parse_uint32(&netmask, slash + 1, 10);
		if (result != ISC_R_SUCCESS) {
			fatal("invalid prefix length in '%s': %s\n",
			      value, isc_result_totext(result));
		}
	} else if (strcmp(value, "0") == 0) {
		netmask = 0;
	}

	sa = isc_mem_allocate(mctx, sizeof(*sa));
	if (sa == NULL)
		fatal("out of memory");
	if (inet_pton(AF_INET6, buf, &in6) == 1) {
		parsed = ISC_TRUE;
		isc_sockaddr_fromin6(sa, &in6, 0);
		if (netmask > 128)
			netmask = 128;
	} else if (inet_pton(AF_INET, buf, &in4) == 1) {
		parsed = ISC_TRUE;
		isc_sockaddr_fromin(sa, &in4, 0);
		if (netmask > 32)
			netmask = 32;
	} else if (netmask != 0xffffffff) {
		int i;

		for (i = 0; i < 3 && strlen(buf) < sizeof(buf) - 2; i++) {
			strlcat(buf, ".0", sizeof(buf));
			if (inet_pton(AF_INET, buf, &in4) == 1) {
				parsed = ISC_TRUE;
				isc_sockaddr_fromin(sa, &in4, 0);
				break;
			}
		}

		if (netmask > 32)
			netmask = 32;
	}

	if (!parsed)
		fatal("invalid address '%s'", value);

	sa->length = netmask;
	*sap = sa;

	return (ISC_R_SUCCESS);
}

/*%
 * Append 'len' bytes of 'text' at '*p', failing with
 * ISC_R_NOSPACE if that would advance p past 'end'.
 */
static isc_result_t
append(const char *text, int len, char **p, char *end) {
	if (len > end - *p)
		return (ISC_R_NOSPACE);
	memmove(*p, text, len);
	*p += len;
	return (ISC_R_SUCCESS);
}

static isc_result_t
reverse_octets(const char *in, char **p, char *end) {
	const char *dot = strchr(in, '.');
	int len;
	if (dot != NULL) {
		isc_result_t result;
		result = reverse_octets(dot + 1, p, end);
		CHECK("reverse_octets", result);
		result = append(".", 1, p, end);
		CHECK("append", result);
		len = (int)(dot - in);
	} else {
		len = strlen(in);
	}
	return (append(in, len, p, end));
}

static void
get_reverse(char *reverse, size_t len, const char *value,
	    isc_boolean_t ip6_int)
{
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

		if (ip6_int)
			options |= DNS_BYADDROPT_IPV6INT;
		name = dns_fixedname_initname(&fname);
		result = dns_byaddr_createptrname2(&addr, options, name);
		CHECK("dns_byaddr_createptrname2", result);
		dns_name_format(name, reverse, (unsigned int)len);
		return;
	} else {
		/*
		 * Not a valid IPv6 address.  Assume IPv4.
		 * Construct the in-addr.arpa name by blindly
		 * reversing octets whether or not they look like
		 * integers, so that this can be used for RFC2317
		 * names and such.
		 */
		char *p = reverse;
		char *end = reverse + len;
		result = reverse_octets(value, &p, end);
		CHECK("reverse_octets", result);
		/* Append .in-addr.arpa. and a terminating NUL. */
		result = append(".in-addr.arpa.", 15, &p, end);
		CHECK("append", result);
		return;
	}
}

/*%
 * We're not using isc_commandline_parse() here since the command line
 * syntax of mdig is quite a bit different from that which can be described
 * by that routine.
 * XXX doc options
 */

static void
plus_option(char *option, struct query *query, isc_boolean_t global)
{
	isc_result_t result;
	char option_store[256];
	char *cmd, *value, *ptr, *code;
	isc_uint32_t num;
	isc_boolean_t state = ISC_TRUE;
	size_t n;

	strlcpy(option_store, option, sizeof(option_store));
	ptr = option_store;
	cmd = next_token(&ptr, "=");
	if (cmd == NULL) {
		printf(";; Invalid option %s\n", option_store);
		return;
	}
	value = ptr;
	if (strncasecmp(cmd, "no", 2) == 0) {
		cmd += 2;
		state = ISC_FALSE;
	}

#define FULLCHECK(A) \
	do { \
		size_t _l = strlen(cmd); \
		if (_l >= sizeof(A) || strncasecmp(cmd, A, _l) != 0) \
			goto invalid_option; \
	} while (0)
#define FULLCHECK2(A, B) \
	do { \
		size_t _l = strlen(cmd); \
		if ((_l >= sizeof(A) || strncasecmp(cmd, A, _l) != 0) && \
		    (_l >= sizeof(B) || strncasecmp(cmd, B, _l) != 0)) \
			goto invalid_option; \
	} while (0)
#define GLOBAL() \
	do { \
		if (!global) \
			goto global_option; \
	} while (0)

	switch (cmd[0]) {
	case 'a':
		switch (cmd[1]) {
		case 'a': /* aaonly / aaflag */
			FULLCHECK2("aaonly", "aaflag");
			query->have_aaonly = state;
			break;
		case 'd':
			switch (cmd[2]) {
			case 'd': /* additional */
				FULLCHECK("additional");
				display_additional = state;
				break;
			case 'f': /* adflag */
			case '\0': /* +ad is a synonym for +adflag */
				FULLCHECK("adflag");
				query->have_adflag = state;
				break;
			default:
				goto invalid_option;
			}
			break;
		case 'l': /* all */
			FULLCHECK("all");
			GLOBAL();
			display_question = state;
			display_answer = state;
			display_authority = state;
			display_additional = state;
			display_comments = state;
			display_rrcomments = state;
			break;
		case 'n': /* answer */
			FULLCHECK("answer");
			GLOBAL();
			display_answer = state;
			break;
		case 'u': /* authority */
			FULLCHECK("authority");
			GLOBAL();
			display_authority = state;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'b':
		switch (cmd[1]) {
		case 'e':/* besteffort */
			FULLCHECK("besteffort");
			GLOBAL();
			besteffort = state;
			break;
		case 'u':/* bufsize */
			FULLCHECK("bufsize");
			if (value == NULL)
				goto need_value;
			if (!state)
				goto invalid_option;
			result = parse_uint(&num, value, COMMSIZE,
					    "buffer size");
			CHECK("parse_uint(buffer size)", result);
			query->udpsize = num;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'c':
		switch (cmd[1]) {
		case 'd':/* cdflag */
			switch (cmd[2]) {
			case 'f': /* cdflag */
			case '\0': /* +cd is a synonym for +cdflag */
				FULLCHECK("cdflag");
				query->have_cdflag = state;
				break;
			default:
				goto invalid_option;
			}
			break;
		case 'l': /* cl */
			FULLCHECK("cl");
			GLOBAL();
			display_class = state;
			break;
		case 'o': /* comments */
			switch (cmd[2]) {
			case 'm':
				FULLCHECK("comments");
				GLOBAL();
				display_comments = state;
				break;
			case 'n':
				FULLCHECK("continue");
				GLOBAL();
				continue_on_error = state;
				break;
			case 'o':
				FULLCHECK("cookie");
				if (state && query->edns == -1)
					query->edns = 0;
				query->send_cookie = state;
				if (value != NULL) {
					n = strlcpy(hexcookie, value,
						    sizeof(hexcookie));
					if (n >= sizeof(hexcookie))
						fatal("COOKIE data too large");
					query->cookie = hexcookie;
				} else
					query->cookie = NULL;
				break;
			default:
				goto invalid_option;
			}
			break;
		case 'r':
			FULLCHECK("crypto");
			GLOBAL();
			display_crypto = state;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'd':
		switch (cmd[1]) {
		case 'n': /* dnssec */
			FULLCHECK("dnssec");
			if (state && query->edns == -1)
				query->edns = 0;
			query->dnssec = state;
			break;
		case 's': /* dscp */
			FULLCHECK("dscp");
			GLOBAL();
			if (!state) {
				dscp = -1;
				break;
			}
			if (value == NULL)
				goto need_value;
			result = parse_uint(&num, value, 0x3f, "DSCP");
			CHECK("parse_uint(DSCP)", result);
			dscp = num;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'e':
		switch (cmd[1]) {
		case 'd':
			switch(cmd[2]) {
			case 'n':
				switch (cmd[3]) {
				case 's':
					switch (cmd[4]) {
					case 0:
						FULLCHECK("edns");
						if (!state) {
							query->edns = -1;
							break;
						}
						if (value == NULL) {
							query->edns = 0;
							break;
						}
						result = parse_uint(&num,
								    value,
								    255,
								    "edns");
						CHECK("parse_uint(edns)",
						      result);
						query->edns = num;
						break;
					case 'f':
						FULLCHECK("ednsflags");
						if (!state) {
							query->ednsflags = 0;
							break;
						}
						if (value == NULL) {
							query->ednsflags = 0;
							break;
						}
						result = parse_xint(&num,
								    value,
								    0xffff,
								  "ednsflags");
						CHECK("parse_xint(ednsflags)",
						      result);
						query->ednsflags = num;
						break;
					case 'o':
						FULLCHECK("ednsopt");
						if (!state) {
							query->ednsoptscnt = 0;
							break;
						}
						if (value == NULL)
							fatal("ednsopt no "
							      "code point "
							      "specified");
						code = next_token(&value, ":");
						save_opt(query, code, value);
						break;
					default:
						goto invalid_option;
					}
					break;
				default:
					goto invalid_option;
				}
				break;
			default:
				goto invalid_option;
			}
			break;
		case 'x':
			FULLCHECK("expire");
			query->expire = state;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'm': /* multiline */
		FULLCHECK("multiline");
		GLOBAL();
		display_multiline = state;
		break;
	case 'n':
		FULLCHECK("nsid");
		if (state && query->edns == -1)
			query->edns = 0;
		query->nsid = state;
		break;
	case 'q':
		FULLCHECK("question");
		GLOBAL();
		display_question = state;
		break;
	case 'r':
		switch (cmd[1]) {
		case 'e':
			switch (cmd[2]) {
			case 'c': /* recurse */
				FULLCHECK("recurse");
				query->recurse = state;
				break;
			case 't': /* retry / retries */
				FULLCHECK2("retry", "retries");
				if (value == NULL)
					goto need_value;
				if (!state)
					goto invalid_option;
				result = parse_uint(&query->udpretries,
						    value,
						    MAXTRIES - 1,
						    "udpretries");
				CHECK("parse_uint(udpretries)", result);
				query->udpretries++;
				break;
			default:
				goto invalid_option;
			}
			break;
		case 'r':
			FULLCHECK("rrcomments");
			GLOBAL();
			display_rrcomments = state;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 's':
		switch (cmd[1]) {
		case 'h':
			FULLCHECK("short");
			GLOBAL();
			display_short_form = state;
			if (state) {
				display_question = ISC_FALSE;
				display_answer = ISC_TRUE;
				display_authority = ISC_FALSE;
				display_additional = ISC_FALSE;
				display_comments = ISC_FALSE;
				display_rrcomments = ISC_FALSE;
			}
			break;
		case 'p': /* split */
			FULLCHECK("split");
			GLOBAL();
			if (value != NULL && !state)
				goto invalid_option;
			if (!state) {
				display_splitwidth = 0;
				break;
			} else if (value == NULL)
				break;

			result = parse_uint(&display_splitwidth, value,
					    1023, "split");
			if ((display_splitwidth % 4) != 0) {
				display_splitwidth =
				    ((display_splitwidth + 3) / 4) * 4;
				fprintf(stderr, ";; Warning, split must be "
						"a multiple of 4; adjusting "
						"to %u\n",
					display_splitwidth);
			}
			/*
			 * There is an adjustment done in the
			 * totext_<rrtype>() functions which causes
			 * splitwidth to shrink.  This is okay when we're
			 * using the default width but incorrect in this
			 * case, so we correct for it
			 */
			if (display_splitwidth)
				display_splitwidth += 3;
			CHECK("parse_uint(split)", result);
			break;
		case 'u': /* subnet */
			FULLCHECK("subnet");
			if (state && value == NULL)
				goto need_value;
			if (!state) {
				if (query->ecs_addr != NULL) {
					isc_mem_free(mctx, query->ecs_addr);
					query->ecs_addr = NULL;
				}
				break;
			}
			if (query->edns == -1)
				query->edns = 0;
			result = parse_netprefix(&query->ecs_addr, value);
			CHECK("parse_netprefix", result);
			break;
		default:
			goto invalid_option;
		}
		break;
	case 't':
		switch (cmd[1]) {
		case 'c': /* tcp */
			FULLCHECK("tcp");
			GLOBAL();
			tcp_mode = state;
			break;
		case 'i': /* timeout */
			FULLCHECK("timeout");
			if (value == NULL)
				goto need_value;
			if (!state)
				goto invalid_option;
			result = parse_uint(&query->timeout, value,
					    MAXTIMEOUT, "timeout");
			CHECK("parse_uint(timeout)", result);
			if (query->timeout == 0)
				query->timeout = 1;
			break;
		case 'r':
			FULLCHECK("tries");
			if (value == NULL)
				goto need_value;
			if (!state)
				goto invalid_option;
			result = parse_uint(&query->udpretries, value,
					    MAXTRIES, "udpretries");
			CHECK("parse_uint(udpretries)", result);
			if (query->udpretries == 0)
				query->udpretries = 1;
			break;
		case 't':
			switch (cmd[2]) {
			case 'l':
				switch (cmd[3]) {
				case 0:
				case 'i': /* ttlid */
					FULLCHECK2("ttl", "ttlid");
					GLOBAL();
					display_ttl = state;
					break;
				case 'u': /* ttlunits */
					FULLCHECK("ttlunits");
					GLOBAL();
					display_ttl = ISC_TRUE;
					display_ttlunits = state;
					break;
				default:
					goto invalid_option;
				}
				break;
			default:
				goto invalid_option;
			}
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'u':
		switch (cmd[1]) {
		case 'd':
			FULLCHECK("udptimeout");
			if (value == NULL)
				goto need_value;
			if (!state)
				goto invalid_option;
			result = parse_uint(&query->udptimeout, value,
					    MAXTIMEOUT, "udptimeout");
			CHECK("parse_uint(udptimeout)", result);
			break;
		case 'n':
			FULLCHECK("unknownformat");
			display_unknown_format = state;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'v':
		FULLCHECK("vc");
		GLOBAL();
		tcp_mode = state;
		break;
	case 'z': /* zflag */
		FULLCHECK("zflag");
		query->have_zflag = state;
		break;
	global_option:
		fprintf(stderr, "Ignored late global option: +%s\n", option);
		break;
	default:
	invalid_option:
	need_value:
		fprintf(stderr, "Invalid option: +%s\n", option);
		usage();
	}
	return;
}

/*%
 * #ISC_TRUE returned if value was used
 */
static const char *single_dash_opts = "46himv";
/*static const char *dash_opts = "46bcfhiptvx";*/
static isc_boolean_t
dash_option(const char *option, char *next, struct query *query,
	    isc_boolean_t global, isc_boolean_t *setname)
{
	char opt;
	const char *value;
	isc_result_t result;
	isc_boolean_t value_from_next;
	isc_consttextregion_t tr;
	dns_rdatatype_t rdtype;
	dns_rdataclass_t rdclass;
	char textname[MXNAME];
	struct in_addr in4;
	struct in6_addr in6;
	in_port_t srcport;
	char *hash;
	isc_uint32_t num;

	while (strpbrk(option, single_dash_opts) == &option[0]) {
		/*
		 * Since the -[46hiv] options do not take an argument,
		 * account for them (in any number and/or combination)
		 * if they appear as the first character(s) of an opt.
		 */
		opt = option[0];
		switch (opt) {
		case '4':
			GLOBAL();
			if (have_ipv4) {
				isc_net_disableipv6();
				have_ipv6 = ISC_FALSE;
			} else {
				fatal("can't find IPv4 networking");
				/* NOTREACHED */
				return (ISC_FALSE);
			}
			break;
		case '6':
			GLOBAL();
			if (have_ipv6) {
				isc_net_disableipv4();
				have_ipv4 = ISC_FALSE;
			} else {
				fatal("can't find IPv6 networking");
				/* NOTREACHED */
				return (ISC_FALSE);
			}
			break;
		case 'h':
			help();
			exit(0);
			break;
		case 'i':
			query->ip6_int = ISC_TRUE;
			break;
		case 'm':
			/*
			 * handled by preparse_args()
			 */
			break;
		case 'v':
			fputs("mDiG " VERSION "\n", stderr);
			exit(0);
			break;
		}
		if (strlen(option) > 1U)
			option = &option[1];
		else
			return (ISC_FALSE);
	}
	opt = option[0];
	if (strlen(option) > 1U) {
		value_from_next = ISC_FALSE;
		value = &option[1];
	} else {
		value_from_next = ISC_TRUE;
		value = next;
	}
	if (value == NULL)
		goto invalid_option;
	switch (opt) {
	case 'b':
		GLOBAL();
		hash = strchr(value, '#');
		if (hash != NULL) {
			result = parse_uint(&num, hash + 1, MAXPORT,
					    "port number");
			CHECK("parse_uint(srcport)", result);
			srcport = num;
			*hash = '\0';
		} else
			srcport = 0;
		if (have_ipv6 && inet_pton(AF_INET6, value, &in6) == 1) {
			isc_sockaddr_fromin6(&srcaddr, &in6, srcport);
			isc_net_disableipv4();
		} else if (have_ipv4 && inet_pton(AF_INET, value, &in4) == 1) {
			isc_sockaddr_fromin(&srcaddr, &in4, srcport);
			isc_net_disableipv6();
		} else {
			if (hash != NULL)
				*hash = '#';
			fatal("invalid address %s", value);
		}
		if (hash != NULL)
			*hash = '#';
		have_src = ISC_TRUE;
		return (value_from_next);
	case 'c':
		tr.base = value;
		tr.length = strlen(value);
		result = dns_rdataclass_fromtext(&rdclass,
						 (isc_textregion_t *)&tr);
		CHECK("dns_rdataclass_fromtext", result);
		query->rdclass = rdclass;
		return (value_from_next);
	case 'f':
		batchname = value;
		return (value_from_next);
	case 'p':
		GLOBAL();
		result = parse_uint(&num, value, MAXPORT, "port number");
		CHECK("parse_uint(port)", result);
		port = num;
		return (value_from_next);
	case 't':
		tr.base = value;
		tr.length = strlen(value);
		result = dns_rdatatype_fromtext(&rdtype,
						(isc_textregion_t *)&tr);
		CHECK("dns_rdatatype_fromtext", result);
		query->rdtype = rdtype;
		return (value_from_next);
	case 'x':
		get_reverse(textname, sizeof(textname), value, query->ip6_int);
		strlcpy(query->textname, textname, sizeof(query->textname));
		query->rdtype = dns_rdatatype_ptr;
		query->rdclass = dns_rdataclass_in;
		*setname = ISC_TRUE;
		return (value_from_next);
	global_option:
		fprintf(stderr, "Ignored late global option: -%s\n", option);
		usage();
	default:
	invalid_option:
		fprintf(stderr, "Invalid option: -%s\n", option);
		usage();
	}
	/* NOTREACHED */
	return (ISC_FALSE);
}

static struct query *
clone_default_query() {
	struct query *query;

	query = isc_mem_allocate(mctx, sizeof(struct query));
	if (query == NULL)
		fatal("memory allocation failure in %s:%d",
		      __FILE__, __LINE__);
	memmove(query, &default_query, sizeof(struct query));
	if (default_query.ecs_addr != NULL) {
		size_t len = sizeof(isc_sockaddr_t);

		query->ecs_addr = isc_mem_allocate(mctx, len);
		if (query->ecs_addr == NULL)
			fatal("memory allocation failure in %s:%d",
			      __FILE__, __LINE__);
		memmove(query->ecs_addr, default_query.ecs_addr, len);
	}

	if (query->timeout == 0)
		query->timeout = tcp_mode ? TCPTIMEOUT : UDPTIMEOUT;

	return query;
}

/*%
 * Because we may be trying to do memory allocation recording, we're going
 * to need to parse the arguments for the -m *before* we start the main
 * argument parsing routine.
 *
 * I'd prefer not to have to do this, but I am not quite sure how else to
 * fix the problem.  Argument parsing in mdig involves memory allocation
 * by its nature, so it can't be done in the main argument parser.
 */
static void
preparse_args(int argc, char **argv) {
	int rc;
	char **rv;
	char *option;
	isc_boolean_t ipv4only = ISC_FALSE, ipv6only = ISC_FALSE;

	rc = argc;
	rv = argv;
	for (rc--, rv++; rc > 0; rc--, rv++) {
		if (rv[0][0] != '-')
			continue;
		option = &rv[0][1];
		while (strpbrk(option, single_dash_opts) == &option[0]) {
			switch (option[0]) {
			case 'm':
				isc_mem_debugging = ISC_MEM_DEBUGTRACE |
					ISC_MEM_DEBUGRECORD;
				break;
			case '4':
				if (ipv6only) {
					fatal("only one of -4 and -6 allowed");
				}
				ipv4only = ISC_TRUE;
				break;
			case '6':
				if (ipv4only) {
					fatal("only one of -4 and -6 allowed");
				}
				ipv6only = ISC_TRUE;
				break;
			}
			option = &option[1];
		}
	}
}

static void
parse_args(isc_boolean_t is_batchfile, int argc, char **argv)
{
	struct query *query = NULL;
	char batchline[MXNAME];
	int bargc;
	char *bargv[64];
	int rc;
	char **rv;
	char *input;
	isc_boolean_t global = ISC_TRUE;

	/*
	 * The semantics for parsing the args is a bit complex; if
	 * we don't have a host yet, make the arg apply globally,
	 * otherwise make it apply to the latest host.  This is
	 * a bit different than the previous versions, but should
	 * form a consistent user interface.
	 *
	 * First, create a "default query" which won't actually be used
	 * anywhere, except for cloning into new queries
	 */

	if (!is_batchfile) {
		default_query.textname[0] = 0;
		default_query.ip6_int = ISC_FALSE;
		default_query.recurse = ISC_TRUE;
		default_query.have_aaonly = ISC_FALSE;
		default_query.have_adflag = ISC_TRUE; /*XXX*/
		default_query.have_cdflag = ISC_FALSE;
		default_query.have_zflag = ISC_FALSE;
		default_query.dnssec = ISC_FALSE;
		default_query.expire = ISC_FALSE;
		default_query.send_cookie = ISC_FALSE;
		default_query.cookie = NULL;
		default_query.nsid = ISC_FALSE;
		default_query.rdtype = dns_rdatatype_a;
		default_query.rdclass = dns_rdataclass_in;
		default_query.udpsize = 0;
		default_query.edns = 0; /*XXX*/
		default_query.ednsopts = NULL;
		default_query.ednsoptscnt = 0;
		default_query.ednsflags = 0;
		default_query.ecs_addr = NULL;
		default_query.timeout = 0;
		default_query.udptimeout = 0;
		default_query.udpretries = 3;
		ISC_LINK_INIT(&default_query, link);
	}

	if (is_batchfile) {
		/* Processing '-f batchfile'. */
		query = clone_default_query();
		global = ISC_FALSE;
	} else
		query = &default_query;

	rc = argc;
	rv = argv;
	for (rc--, rv++; rc > 0; rc--, rv++) {
		if (strncmp(rv[0], "%", 1) == 0)
			break;
		if (rv[0][0] == '@') {
			if (server != NULL)
				fatal("server already set to @%s", server);
			server = &rv[0][1];
		} else if (rv[0][0] == '+') {
			plus_option(&rv[0][1], query, global);
		} else if (rv[0][0] == '-') {
			isc_boolean_t setname = ISC_FALSE;

			if (rc <= 1) {
				if (dash_option(&rv[0][1], NULL, query,
						global, &setname)) {
					rc--;
					rv++;
				}
			} else {
				if (dash_option(&rv[0][1], rv[1], query,
						global, &setname)) {
					rc--;
					rv++;
				}
			}
			if (setname) {
				if (query == &default_query)
					query = clone_default_query();
				ISC_LIST_APPEND(queries, query, link);

				default_query.textname[0] = 0;
				query = clone_default_query();
				global = ISC_FALSE;
			}
		} else {
			/*
			 * Anything which isn't an option
			 */
			if (query == &default_query)
				query = clone_default_query();
			strlcpy(query->textname, rv[0],
				sizeof(query->textname));
			ISC_LIST_APPEND(queries, query, link);

			query = clone_default_query();
			global = ISC_FALSE;
			/* XXX Error message */
		}
	}

	/*
	 * If we have a batchfile, read the query list from it.
	 */
	if ((batchname != NULL) && !is_batchfile) {
		if (strcmp(batchname, "-") == 0)
			batchfp = stdin;
		else
			batchfp = fopen(batchname, "r");
		if (batchfp == NULL) {
			perror(batchname);
			fatal("couldn't open batch file '%s'", batchname);
		}
		while (fgets(batchline, sizeof(batchline), batchfp) != 0) {
			bargc = 1;
			if (batchline[0] == '\r' || batchline[0] == '\n'
			    || batchline[0] == '#' || batchline[0] == ';')
				continue;
			input = batchline;
			bargv[bargc] = next_token(&input, " \t\r\n");
			while ((bargc < 14) && (bargv[bargc] != NULL)) {
				bargc++;
				bargv[bargc] = next_token(&input, " \t\r\n");
			}

			bargv[0] = argv[0];
			parse_args(ISC_TRUE, bargc, (char **)bargv);
		}
		if (batchfp != stdin)
			fclose(batchfp);
	}
	if (query != &default_query) {
		if (query->ecs_addr != NULL)
			isc_mem_free(mctx, query->ecs_addr);
		isc_mem_free(mctx, query);
	}
}

/*% Main processing routine for mdig */
int
main(int argc, char *argv[]) {
	struct query *query;
	isc_result_t result;
	isc_sockaddr_t bind_any;
	isc_log_t *lctx;
	isc_logconfig_t *lcfg;
	isc_entropy_t *ectx;
	isc_taskmgr_t *taskmgr;
	isc_task_t *task;
	isc_timermgr_t *timermgr;
	isc_socketmgr_t *socketmgr;
	dns_dispatchmgr_t *dispatchmgr;
	unsigned int attrs, attrmask;
	dns_dispatch_t *dispatchvx;
	dns_view_t *view;
	int ns;

	RUNCHECK(isc_app_start());

	dns_result_register();

	if (isc_net_probeipv4() == ISC_R_SUCCESS)
		have_ipv4 = ISC_TRUE;
	if (isc_net_probeipv6() == ISC_R_SUCCESS)
		have_ipv6 = ISC_TRUE;
	if (!have_ipv4 && !have_ipv6)
		fatal("could not find either IPv4 or IPv6");

	preparse_args(argc, argv);

	mctx = NULL;
	RUNCHECK(isc_mem_create(0, 0, &mctx));

	lctx = NULL;
	lcfg = NULL;
	RUNCHECK(isc_log_create(mctx, &lctx, &lcfg));

	ectx = NULL;
	RUNCHECK(isc_entropy_create(mctx, &ectx));
	RUNCHECK(dst_lib_init(mctx, ectx, ISC_ENTROPY_GOODONLY));
	RUNCHECK(isc_hash_create(mctx, ectx, DNS_NAME_MAXWIRE));
	RUNCHECK(isc_entropy_getdata(ectx, cookie_secret,
				     sizeof(cookie_secret), NULL, 0));

	ISC_LIST_INIT(queries);
	parse_args(ISC_FALSE, argc, argv);
	if (server == NULL)
		fatal("a server '@xxx' is required");

	ns = 0;
	result = bind9_getaddresses(server, port, &dstaddr, 1, &ns);
	if (result != ISC_R_SUCCESS) {
		fatal("couldn't get address for '%s': %s",
		      server, isc_result_totext(result));
	}

	if (isc_sockaddr_pf(&dstaddr) == PF_INET && have_ipv6) {
		isc_net_disableipv6();
		have_ipv6 = ISC_FALSE;
	} else if (isc_sockaddr_pf(&dstaddr) == PF_INET6 && have_ipv4) {
		isc_net_disableipv4();
		have_ipv4 = ISC_FALSE;
	}
	if (have_ipv4 && have_ipv6)
		fatal("can't choose between IPv4 and IPv6");

	taskmgr = NULL;
	RUNCHECK(isc_taskmgr_create(mctx, 1, 0, &taskmgr));
	task = NULL;
	RUNCHECK(isc_task_create(taskmgr, 0, &task));
	timermgr = NULL;

	RUNCHECK(isc_timermgr_create(mctx, &timermgr));
	socketmgr = NULL;
	RUNCHECK(isc_socketmgr_create(mctx, &socketmgr));
	dispatchmgr = NULL;
	RUNCHECK(dns_dispatchmgr_create(mctx, ectx, &dispatchmgr));

	attrs = DNS_DISPATCHATTR_UDP |
		DNS_DISPATCHATTR_MAKEQUERY;
	if (have_ipv4) {
		isc_sockaddr_any(&bind_any);
		attrs |= DNS_DISPATCHATTR_IPV4;
	} else {
		isc_sockaddr_any6(&bind_any);
		attrs |= DNS_DISPATCHATTR_IPV6;
	}
	attrmask = DNS_DISPATCHATTR_UDP |
		   DNS_DISPATCHATTR_TCP |
		   DNS_DISPATCHATTR_IPV4 |
		   DNS_DISPATCHATTR_IPV6;
	dispatchvx = NULL;
	RUNCHECK(dns_dispatch_getudp(dispatchmgr, socketmgr, taskmgr,
				     have_src ? &srcaddr : &bind_any,
				     4096, 100, 100, 17, 19,
				     attrs, attrmask, &dispatchvx));
	requestmgr = NULL;
	RUNCHECK(dns_requestmgr_create(mctx, timermgr, socketmgr,
				       taskmgr, dispatchmgr,
				       have_ipv4 ? dispatchvx : NULL,
				       have_ipv6 ? dispatchvx : NULL,
				       &requestmgr));

	view = NULL;
	RUNCHECK(dns_view_create(mctx, 0, "_test", &view));

	query = ISC_LIST_HEAD(queries);
	RUNCHECK(isc_app_onrun(mctx, task, sendqueries, query));

	(void)isc_app_run();

	query = ISC_LIST_HEAD(queries);
	while (query != NULL) {
		struct query *next = ISC_LIST_NEXT(query, link);

		if (query->ecs_addr != NULL) {
			isc_mem_free(mctx, query->ecs_addr);
			query->ecs_addr = NULL;
		}
		isc_mem_free(mctx, query);
		query = next;
	}

	if (default_query.ecs_addr != NULL)
		isc_mem_free(mctx, default_query.ecs_addr);

	dns_view_detach(&view);

	dns_requestmgr_shutdown(requestmgr);
	dns_requestmgr_detach(&requestmgr);

	dns_dispatch_detach(&dispatchvx);
	dns_dispatchmgr_destroy(&dispatchmgr);

	isc_socketmgr_destroy(&socketmgr);
	isc_timermgr_destroy(&timermgr);

	isc_task_shutdown(task);
	isc_task_detach(&task);
	isc_taskmgr_destroy(&taskmgr);

	dst_lib_destroy();
	isc_hash_destroy();
	isc_entropy_detach(&ectx);

	isc_log_destroy(&lctx);

	isc_mem_destroy(&mctx);

	isc_app_finish();

	return (0);
}
