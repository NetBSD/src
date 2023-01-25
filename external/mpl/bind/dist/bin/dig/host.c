/*	$NetBSD: host.c,v 1.10 2023/01/25 21:43:23 christos Exp $	*/

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

#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>

#include <isc/app.h>
#include <isc/commandline.h>
#include <isc/netaddr.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/byaddr.h>
#include <dns/fixedname.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>

#include <dig/dig.h>

static bool short_form = true, listed_server = false;
static bool default_lookups = true;
static int seen_error = -1;
static bool list_addresses = true;
static bool list_almost_all = false;
static dns_rdatatype_t list_type = dns_rdatatype_a;
static bool printed_server = false;
static bool ipv4only = false, ipv6only = false;

static const char *opcodetext[] = { "QUERY",	  "IQUERY",	"STATUS",
				    "RESERVED3",  "NOTIFY",	"UPDATE",
				    "RESERVED6",  "RESERVED7",	"RESERVED8",
				    "RESERVED9",  "RESERVED10", "RESERVED11",
				    "RESERVED12", "RESERVED13", "RESERVED14",
				    "RESERVED15" };

static const char *rcodetext[] = { "NOERROR",	 "FORMERR",    "SERVFAIL",
				   "NXDOMAIN",	 "NOTIMP",     "REFUSED",
				   "YXDOMAIN",	 "YXRRSET",    "NXRRSET",
				   "NOTAUTH",	 "NOTZONE",    "RESERVED11",
				   "RESERVED12", "RESERVED13", "RESERVED14",
				   "RESERVED15", "BADVERS" };

struct rtype {
	unsigned int type;
	const char *text;
};

struct rtype rtypes[] = { { 1, "has address" },
			  { 2, "name server" },
			  { 5, "is an alias for" },
			  { 11, "has well known services" },
			  { 12, "domain name pointer" },
			  { 13, "host information" },
			  { 15, "mail is handled by" },
			  { 16, "descriptive text" },
			  { 19, "x25 address" },
			  { 20, "ISDN address" },
			  { 24, "has signature" },
			  { 25, "has key" },
			  { 28, "has IPv6 address" },
			  { 29, "location" },
			  { 0, NULL } };

static char *
rcode_totext(dns_rcode_t rcode) {
	static char buf[sizeof("?65535")];
	union {
		const char *consttext;
		char *deconsttext;
	} totext;

	if (rcode >= (sizeof(rcodetext) / sizeof(rcodetext[0]))) {
		snprintf(buf, sizeof(buf), "?%u", rcode);
		totext.deconsttext = buf;
	} else {
		totext.consttext = rcodetext[rcode];
	}
	return (totext.deconsttext);
}

ISC_PLATFORM_NORETURN_PRE static void
show_usage(void) ISC_PLATFORM_NORETURN_POST;

static void
show_usage(void) {
	fputs("Usage: host [-aCdilrTvVw] [-c class] [-N ndots] [-t type] [-W "
	      "time]\n"
	      "            [-R number] [-m flag] [-p port] hostname [server]\n"
	      "       -a is equivalent to -v -t ANY\n"
	      "       -A is like -a but omits RRSIG, NSEC, NSEC3\n"
	      "       -c specifies query class for non-IN data\n"
	      "       -C compares SOA records on authoritative nameservers\n"
	      "       -d is equivalent to -v\n"
	      "       -l lists all hosts in a domain, using AXFR\n"
	      "       -m set memory debugging flag (trace|record|usage)\n"
	      "       -N changes the number of dots allowed before root lookup "
	      "is done\n"
	      "       -p specifies the port on the server to query\n"
	      "       -r disables recursive processing\n"
	      "       -R specifies number of retries for UDP packets\n"
	      "       -s a SERVFAIL response should stop query\n"
	      "       -t specifies the query type\n"
	      "       -T enables TCP/IP mode\n"
	      "       -U enables UDP mode\n"
	      "       -v enables verbose output\n"
	      "       -V print version number and exit\n"
	      "       -w specifies to wait forever for a reply\n"
	      "       -W specifies how long to wait for a reply\n"
	      "       -4 use IPv4 query transport only\n"
	      "       -6 use IPv6 query transport only\n",
	      stderr);
	exit(1);
}

static void
host_shutdown(void) {
	(void)isc_app_shutdown();
}

static void
received(unsigned int bytes, isc_sockaddr_t *from, dig_query_t *query) {
	isc_time_t now;
	int diff;

	if (!short_form) {
		char fromtext[ISC_SOCKADDR_FORMATSIZE];
		isc_sockaddr_format(from, fromtext, sizeof(fromtext));
		if (query->lookup->use_usec) {
			TIME_NOW_HIRES(&now);
		} else {
			TIME_NOW(&now);
		}
		diff = (int)isc_time_microdiff(&now, &query->time_sent);
		printf("Received %u bytes from %s in %d ms\n", bytes, fromtext,
		       diff / 1000);
	}
}

static void
trying(char *frm, dig_lookup_t *lookup) {
	UNUSED(lookup);

	if (!short_form) {
		printf("Trying \"%s\"\n", frm);
	}
}

static void
say_message(dns_name_t *name, const char *msg, dns_rdata_t *rdata,
	    dig_query_t *query) {
	isc_buffer_t *b = NULL;
	char namestr[DNS_NAME_FORMATSIZE];
	isc_region_t r;
	isc_result_t result;
	unsigned int bufsize = BUFSIZ;

	dns_name_format(name, namestr, sizeof(namestr));
retry:
	isc_buffer_allocate(mctx, &b, bufsize);
	result = dns_rdata_totext(rdata, NULL, b);
	if (result == ISC_R_NOSPACE) {
		isc_buffer_free(&b);
		bufsize *= 2;
		goto retry;
	}
	check_result(result, "dns_rdata_totext");
	isc_buffer_usedregion(b, &r);
	if (query->lookup->identify_previous_line) {
		printf("Nameserver %s:\n\t", query->servname);
	}
	printf("%s %s %.*s", namestr, msg, (int)r.length, (char *)r.base);
	if (query->lookup->identify) {
		printf(" on server %s", query->servname);
	}
	printf("\n");
	isc_buffer_free(&b);
}

static isc_result_t
printsection(dns_message_t *msg, dns_section_t sectionid,
	     const char *section_name, bool headers, dig_query_t *query) {
	dns_name_t *name, *print_name;
	dns_rdataset_t *rdataset;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_buffer_t target;
	isc_result_t result, loopresult;
	isc_region_t r;
	dns_name_t empty_name;
	char tbuf[4096] = { 0 };
	bool first;
	bool no_rdata = (sectionid == DNS_SECTION_QUESTION);

	if (headers) {
		printf(";; %s SECTION:\n", section_name);
	}

	dns_name_init(&empty_name, NULL);

	result = dns_message_firstname(msg, sectionid);
	if (result == ISC_R_NOMORE) {
		return (ISC_R_SUCCESS);
	} else if (result != ISC_R_SUCCESS) {
		return (result);
	}

	for (;;) {
		name = NULL;
		dns_message_currentname(msg, sectionid, &name);

		isc_buffer_init(&target, tbuf, sizeof(tbuf));
		first = true;
		print_name = name;

		for (rdataset = ISC_LIST_HEAD(name->list); rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link))
		{
			if (query->lookup->rdtype == dns_rdatatype_axfr &&
			    !((!list_addresses &&
			       (list_type == dns_rdatatype_any ||
				rdataset->type == list_type)) ||
			      (list_addresses &&
			       (rdataset->type == dns_rdatatype_a ||
				rdataset->type == dns_rdatatype_aaaa ||
				rdataset->type == dns_rdatatype_ns ||
				rdataset->type == dns_rdatatype_ptr))))
			{
				continue;
			}
			if (list_almost_all &&
			    (rdataset->type == dns_rdatatype_rrsig ||
			     rdataset->type == dns_rdatatype_nsec ||
			     rdataset->type == dns_rdatatype_nsec3))
			{
				continue;
			}
			if (!short_form) {
				result = dns_rdataset_totext(rdataset,
							     print_name, false,
							     no_rdata, &target);
				if (result != ISC_R_SUCCESS) {
					return (result);
				}
#ifdef USEINITALWS
				if (first) {
					print_name = &empty_name;
					first = false;
				}
#else  /* ifdef USEINITALWS */
				UNUSED(first); /* Shut up compiler. */
#endif /* ifdef USEINITALWS */
			} else {
				loopresult = dns_rdataset_first(rdataset);
				while (loopresult == ISC_R_SUCCESS) {
					struct rtype *t;
					const char *rtt;
					char typebuf[DNS_RDATATYPE_FORMATSIZE];
					char typebuf2[DNS_RDATATYPE_FORMATSIZE +
						      20];
					dns_rdataset_current(rdataset, &rdata);

					for (t = rtypes; t->text != NULL; t++) {
						if (t->type == rdata.type) {
							rtt = t->text;
							goto found;
						}
					}

					dns_rdatatype_format(rdata.type,
							     typebuf,
							     sizeof(typebuf));
					snprintf(typebuf2, sizeof(typebuf2),
						 "has %s record", typebuf);
					rtt = typebuf2;
				found:
					say_message(print_name, rtt, &rdata,
						    query);
					dns_rdata_reset(&rdata);
					loopresult =
						dns_rdataset_next(rdataset);
				}
			}
		}
		if (!short_form) {
			isc_buffer_usedregion(&target, &r);
			if (no_rdata) {
				printf(";%.*s", (int)r.length, (char *)r.base);
			} else {
				printf("%.*s", (int)r.length, (char *)r.base);
			}
		}

		result = dns_message_nextname(msg, sectionid);
		if (result == ISC_R_NOMORE) {
			break;
		} else if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
printrdata(dns_message_t *msg, dns_rdataset_t *rdataset,
	   const dns_name_t *owner, const char *set_name, bool headers) {
	isc_buffer_t target;
	isc_result_t result;
	isc_region_t r;
	char tbuf[4096];

	UNUSED(msg);
	if (headers) {
		printf(";; %s SECTION:\n", set_name);
	}

	isc_buffer_init(&target, tbuf, sizeof(tbuf));

	result = dns_rdataset_totext(rdataset, owner, false, false, &target);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	isc_buffer_usedregion(&target, &r);
	printf("%.*s", (int)r.length, (char *)r.base);

	return (ISC_R_SUCCESS);
}

static void
chase_cnamechain(dns_message_t *msg, dns_name_t *qname) {
	isc_result_t result;
	dns_rdataset_t *rdataset;
	dns_rdata_cname_t cname;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned int i = msg->counts[DNS_SECTION_ANSWER];

	while (i-- > 0) {
		rdataset = NULL;
		result = dns_message_findname(msg, DNS_SECTION_ANSWER, qname,
					      dns_rdatatype_cname, 0, NULL,
					      &rdataset);
		if (result != ISC_R_SUCCESS) {
			return;
		}
		result = dns_rdataset_first(rdataset);
		check_result(result, "dns_rdataset_first");
		dns_rdata_reset(&rdata);
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &cname, NULL);
		check_result(result, "dns_rdata_tostruct");
		dns_name_copynf(&cname.cname, qname);
		dns_rdata_freestruct(&cname);
	}
}

static isc_result_t
printmessage(dig_query_t *query, const isc_buffer_t *msgbuf, dns_message_t *msg,
	     bool headers) {
	bool did_flag = false;
	dns_rdataset_t *opt, *tsig = NULL;
	const dns_name_t *tsigname;
	isc_result_t result = ISC_R_SUCCESS;
	int force_error;

	UNUSED(msgbuf);
	UNUSED(headers);

	/*
	 * We get called multiple times.
	 * Preserve any existing error status.
	 */
	force_error = (seen_error == 1) ? 1 : 0;
	seen_error = 1;
	if (listed_server && !printed_server) {
		char sockstr[ISC_SOCKADDR_FORMATSIZE];

		printf("Using domain server:\n");
		printf("Name: %s\n", query->userarg);
		isc_sockaddr_format(&query->sockaddr, sockstr, sizeof(sockstr));
		printf("Address: %s\n", sockstr);
		printf("Aliases: \n\n");
		printed_server = true;
	}

	if (msg->rcode != 0) {
		char namestr[DNS_NAME_FORMATSIZE];
		dns_name_format(query->lookup->name, namestr, sizeof(namestr));

		if (query->lookup->identify_previous_line) {
			printf("Nameserver %s:\n\t%s not found: %d(%s)\n",
			       query->servname,
			       (msg->rcode != dns_rcode_nxdomain)
				       ? namestr
				       : query->lookup->textname,
			       msg->rcode, rcode_totext(msg->rcode));
		} else {
			printf("Host %s not found: %d(%s)\n",
			       (msg->rcode != dns_rcode_nxdomain)
				       ? namestr
				       : query->lookup->textname,
			       msg->rcode, rcode_totext(msg->rcode));
		}
		return (ISC_R_SUCCESS);
	}

	if (default_lookups && query->lookup->rdtype == dns_rdatatype_a) {
		char namestr[DNS_NAME_FORMATSIZE];
		dig_lookup_t *lookup;
		dns_fixedname_t fixed;
		dns_name_t *name;

		/* Add AAAA and MX lookups. */
		name = dns_fixedname_initname(&fixed);
		dns_name_copynf(query->lookup->name, name);
		chase_cnamechain(msg, name);
		dns_name_format(name, namestr, sizeof(namestr));
		lookup = clone_lookup(query->lookup, false);
		if (lookup != NULL) {
			strlcpy(lookup->textname, namestr,
				sizeof(lookup->textname));
			lookup->rdtype = dns_rdatatype_aaaa;
			lookup->rdtypeset = true;
			lookup->origin = NULL;
			lookup->retries = tries;
			ISC_LIST_APPEND(lookup_list, lookup, link);
		}
		lookup = clone_lookup(query->lookup, false);
		if (lookup != NULL) {
			strlcpy(lookup->textname, namestr,
				sizeof(lookup->textname));
			lookup->rdtype = dns_rdatatype_mx;
			lookup->rdtypeset = true;
			lookup->origin = NULL;
			lookup->retries = tries;
			ISC_LIST_APPEND(lookup_list, lookup, link);
		}
	}

	if (!short_form) {
		printf(";; ->>HEADER<<- opcode: %s, status: %s, id: %u\n",
		       opcodetext[msg->opcode], rcode_totext(msg->rcode),
		       msg->id);
		printf(";; flags: ");
		if ((msg->flags & DNS_MESSAGEFLAG_QR) != 0) {
			printf("qr");
			did_flag = true;
		}
		if ((msg->flags & DNS_MESSAGEFLAG_AA) != 0) {
			printf("%saa", did_flag ? " " : "");
			did_flag = true;
		}
		if ((msg->flags & DNS_MESSAGEFLAG_TC) != 0) {
			printf("%stc", did_flag ? " " : "");
			did_flag = true;
		}
		if ((msg->flags & DNS_MESSAGEFLAG_RD) != 0) {
			printf("%srd", did_flag ? " " : "");
			did_flag = true;
		}
		if ((msg->flags & DNS_MESSAGEFLAG_RA) != 0) {
			printf("%sra", did_flag ? " " : "");
			did_flag = true;
		}
		if ((msg->flags & DNS_MESSAGEFLAG_AD) != 0) {
			printf("%sad", did_flag ? " " : "");
			did_flag = true;
		}
		if ((msg->flags & DNS_MESSAGEFLAG_CD) != 0) {
			printf("%scd", did_flag ? " " : "");
			did_flag = true;
			POST(did_flag);
		}
		printf("; QUERY: %u, ANSWER: %u, "
		       "AUTHORITY: %u, ADDITIONAL: %u\n",
		       msg->counts[DNS_SECTION_QUESTION],
		       msg->counts[DNS_SECTION_ANSWER],
		       msg->counts[DNS_SECTION_AUTHORITY],
		       msg->counts[DNS_SECTION_ADDITIONAL]);
		opt = dns_message_getopt(msg);
		if (opt != NULL) {
			printf(";; EDNS: version: %u, udp=%u\n",
			       (unsigned int)((opt->ttl & 0x00ff0000) >> 16),
			       (unsigned int)opt->rdclass);
		}
		tsigname = NULL;
		tsig = dns_message_gettsig(msg, &tsigname);
		if (tsig != NULL) {
			printf(";; PSEUDOSECTIONS: TSIG\n");
		}
	}
	if (!ISC_LIST_EMPTY(msg->sections[DNS_SECTION_QUESTION]) && !short_form)
	{
		printf("\n");
		result = printsection(msg, DNS_SECTION_QUESTION, "QUESTION",
				      true, query);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}
	if (!ISC_LIST_EMPTY(msg->sections[DNS_SECTION_ANSWER])) {
		if (!short_form) {
			printf("\n");
		}
		result = printsection(msg, DNS_SECTION_ANSWER, "ANSWER",
				      !short_form, query);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	if (!ISC_LIST_EMPTY(msg->sections[DNS_SECTION_AUTHORITY]) &&
	    !short_form)
	{
		printf("\n");
		result = printsection(msg, DNS_SECTION_AUTHORITY, "AUTHORITY",
				      true, query);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}
	if (!ISC_LIST_EMPTY(msg->sections[DNS_SECTION_ADDITIONAL]) &&
	    !short_form)
	{
		printf("\n");
		result = printsection(msg, DNS_SECTION_ADDITIONAL, "ADDITIONAL",
				      true, query);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}
	if ((tsig != NULL) && !short_form) {
		printf("\n");
		result = printrdata(msg, tsig, tsigname, "PSEUDOSECTION TSIG",
				    true);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}
	if (!short_form) {
		printf("\n");
	}

	if (short_form && !default_lookups &&
	    ISC_LIST_EMPTY(msg->sections[DNS_SECTION_ANSWER]))
	{
		char namestr[DNS_NAME_FORMATSIZE];
		char typestr[DNS_RDATATYPE_FORMATSIZE];
		dns_name_format(query->lookup->name, namestr, sizeof(namestr));
		dns_rdatatype_format(query->lookup->rdtype, typestr,
				     sizeof(typestr));
		printf("%s has no %s record\n", namestr, typestr);
	}
	seen_error = force_error;
	return (result);
}

static const char *optstring = "46aAc:dilnm:p:rst:vVwCDN:R:TUW:";

/*% version */
static void
version(void) {
	fputs("host " VERSION "\n", stderr);
}

static void
pre_parse_args(int argc, char **argv) {
	int c;

	while ((c = isc_commandline_parse(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'm':
			memdebugging = true;
			if (strcasecmp("trace", isc_commandline_argument) == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGTRACE;
			} else if (strcasecmp("record",
					      isc_commandline_argument) == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
			} else if (strcasecmp("usage",
					      isc_commandline_argument) == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGUSAGE;
			}
			break;

		case '4':
			if (ipv6only) {
				fatal("only one of -4 and -6 allowed");
			}
			ipv4only = true;
			break;
		case '6':
			if (ipv4only) {
				fatal("only one of -4 and -6 allowed");
			}
			ipv6only = true;
			break;
		case 'a':
			break;
		case 'A':
			break;
		case 'c':
			break;
		case 'C':
			break;
		case 'd':
			break;
		case 'D':
			if (debugging) {
				debugtiming = true;
			}
			debugging = true;
			break;
		case 'i':
			break;
		case 'l':
			break;
		case 'n':
			break;
		case 'N':
			break;
		case 'p':
			break;
		case 'r':
			break;
		case 'R':
			break;
		case 's':
			break;
		case 't':
			break;
		case 'T':
			break;
		case 'U':
			break;
		case 'v':
			break;
		case 'V':
			version();
			exit(0);
			break;
		case 'w':
			break;
		case 'W':
			break;
		default:
			show_usage();
		}
	}
	isc_commandline_reset = true;
	isc_commandline_index = 1;
}

static void
parse_args(bool is_batchfile, int argc, char **argv) {
	char hostname[MXNAME];
	dig_lookup_t *lookup;
	int c;
	char store[MXNAME];
	isc_textregion_t tr;
	isc_result_t result = ISC_R_SUCCESS;
	dns_rdatatype_t rdtype;
	dns_rdataclass_t rdclass;
	uint32_t serial = 0;

	UNUSED(is_batchfile);

	lookup = make_empty_lookup();

	lookup->servfail_stops = false;
	lookup->besteffort = false;
	lookup->comments = false;
	short_form = !verbose;

	while ((c = isc_commandline_parse(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'l':
			lookup->tcp_mode = true;
			lookup->rdtype = dns_rdatatype_axfr;
			lookup->rdtypeset = true;
			fatalexit = 3;
			break;
		case 'v':
		case 'd':
			short_form = false;
			break;
		case 'r':
			lookup->recurse = false;
			break;
		case 't':
			if (strncasecmp(isc_commandline_argument, "ixfr=", 5) ==
			    0)
			{
				rdtype = dns_rdatatype_ixfr;
				/* XXXMPA add error checking */
				serial = strtoul(isc_commandline_argument + 5,
						 NULL, 10);
				result = ISC_R_SUCCESS;
			} else {
				tr.base = isc_commandline_argument;
				tr.length = strlen(isc_commandline_argument);
				result = dns_rdatatype_fromtext(
					&rdtype, (isc_textregion_t *)&tr);
			}

			if (result != ISC_R_SUCCESS) {
				fatalexit = 2;
				fatal("invalid type: %s\n",
				      isc_commandline_argument);
			}
			if (!lookup->rdtypeset ||
			    lookup->rdtype != dns_rdatatype_axfr)
			{
				lookup->rdtype = rdtype;
			}
			lookup->rdtypeset = true;
			if (rdtype == dns_rdatatype_axfr) {
				/* -l -t any -v */
				list_type = dns_rdatatype_any;
				short_form = false;
				lookup->tcp_mode = true;
			} else if (rdtype == dns_rdatatype_ixfr) {
				lookup->ixfr_serial = serial;
				lookup->tcp_mode = true;
				list_type = rdtype;
			} else if (rdtype == dns_rdatatype_any) {
				if (!lookup->tcp_mode_set) {
					lookup->tcp_mode = true;
				}
			} else {
				list_type = rdtype;
			}
			list_addresses = false;
			default_lookups = false;
			break;
		case 'c':
			tr.base = isc_commandline_argument;
			tr.length = strlen(isc_commandline_argument);
			result = dns_rdataclass_fromtext(
				&rdclass, (isc_textregion_t *)&tr);

			if (result != ISC_R_SUCCESS) {
				fatalexit = 2;
				fatal("invalid class: %s\n",
				      isc_commandline_argument);
			} else {
				lookup->rdclass = rdclass;
				lookup->rdclassset = true;
			}
			default_lookups = false;
			break;
		case 'A':
			list_almost_all = true;
			FALLTHROUGH;
		case 'a':
			if (!lookup->rdtypeset ||
			    lookup->rdtype != dns_rdatatype_axfr)
			{
				lookup->rdtype = dns_rdatatype_any;
			}
			list_type = dns_rdatatype_any;
			list_addresses = false;
			lookup->rdtypeset = true;
			short_form = false;
			default_lookups = false;
			break;
		case 'i':
			/* deprecated */
			break;
		case 'n':
			/* deprecated */
			break;
		case 'm':
			/* Handled by pre_parse_args(). */
			break;
		case 'w':
			/*
			 * The timer routines are coded such that
			 * timeout==MAXINT doesn't enable the timer
			 */
			timeout = INT_MAX;
			break;
		case 'W':
			timeout = atoi(isc_commandline_argument);
			if (timeout < 1) {
				timeout = 1;
			}
			break;
		case 'R':
			tries = atoi(isc_commandline_argument) + 1;
			if (tries < 2) {
				tries = 2;
			}
			break;
		case 'T':
			lookup->tcp_mode = true;
			lookup->tcp_mode_set = true;
			break;
		case 'U':
			lookup->tcp_mode = false;
			lookup->tcp_mode_set = true;
			break;
		case 'C':
			debug("showing all SOAs");
			lookup->rdtype = dns_rdatatype_ns;
			lookup->rdtypeset = true;
			lookup->rdclass = dns_rdataclass_in;
			lookup->rdclassset = true;
			lookup->ns_search_only = true;
			lookup->trace_root = true;
			lookup->identify_previous_line = true;
			default_lookups = false;
			break;
		case 'N':
			debug("setting NDOTS to %s", isc_commandline_argument);
			ndots = atoi(isc_commandline_argument);
			break;
		case 'D':
			/* Handled by pre_parse_args(). */
			break;
		case '4':
			/* Handled by pre_parse_args(). */
			break;
		case '6':
			/* Handled by pre_parse_args(). */
			break;
		case 's':
			lookup->servfail_stops = true;
			break;
		case 'p':
			port = atoi(isc_commandline_argument);
			break;
		}
	}

	lookup->retries = tries;

	if (isc_commandline_index >= argc) {
		show_usage();
	}

	strlcpy(hostname, argv[isc_commandline_index], sizeof(hostname));

	if (argc > isc_commandline_index + 1) {
		set_nameserver(argv[isc_commandline_index + 1]);
		debug("server is %s", argv[isc_commandline_index + 1]);
		listed_server = true;
	} else {
		check_ra = true;
	}

	lookup->pending = false;
	if (get_reverse(store, sizeof(store), hostname, true) == ISC_R_SUCCESS)
	{
		strlcpy(lookup->textname, store, sizeof(lookup->textname));
		lookup->rdtype = dns_rdatatype_ptr;
		lookup->rdtypeset = true;
		default_lookups = false;
	} else {
		strlcpy(lookup->textname, hostname, sizeof(lookup->textname));
		usesearch = true;
	}
	lookup->new_search = true;
	ISC_LIST_APPEND(lookup_list, lookup, link);
}

int
main(int argc, char **argv) {
	isc_result_t result;

	tries = 2;

	ISC_LIST_INIT(lookup_list);
	ISC_LIST_INIT(server_list);
	ISC_LIST_INIT(search_list);

	fatalexit = 1;

	/* setup dighost callbacks */
	dighost_printmessage = printmessage;
	dighost_received = received;
	dighost_trying = trying;
	dighost_shutdown = host_shutdown;

	debug("main()");
	progname = argv[0];
	pre_parse_args(argc, argv);
	result = isc_app_start();
	check_result(result, "isc_app_start");
	setup_libs();
	setup_system(ipv4only, ipv6only);
	parse_args(false, argc, argv);
	if (keyfile[0] != 0) {
		setup_file_key();
	} else if (keysecret[0] != 0) {
		setup_text_key();
	}
	result = isc_app_onrun(mctx, global_task, onrun_callback, NULL);
	check_result(result, "isc_app_onrun");
	isc_app_run();
	cancel_all();
	destroy_libs();
	isc_app_finish();
	return ((seen_error == 0) ? 0 : 1);
}
