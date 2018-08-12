/*	$NetBSD: named-rrchecker.c,v 1.1.1.1 2018/08/12 12:07:15 christos Exp $	*/

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

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>
#include <dns/result.h>

static isc_mem_t *mctx;
static isc_lex_t *lex;

static isc_lexspecials_t specials;

ISC_PLATFORM_NORETURN_PRE static void
usage(void) ISC_PLATFORM_NORETURN_POST;

static void
usage(void) {
	fprintf(stderr, "usage: named-rrchecker [-o origin] [-hpCPTu]\n");
	fprintf(stderr, "\t-h: print this help message\n");
	fprintf(stderr, "\t-o origin: set origin to be used when "
			"interpeting the record\n");
	fprintf(stderr, "\t-p: print the record in canonical format\n");
	fprintf(stderr, "\t-C: list the supported class names\n");
	fprintf(stderr, "\t-P: list the supported private type names\n");
	fprintf(stderr, "\t-T: list the supported standard type names\n");
	fprintf(stderr, "\t-u: print the record in unknown record format\n");
	exit(0);
}

ISC_PLATFORM_NORETURN_PRE static void
fatal(const char *format, ...) ISC_PLATFORM_NORETURN_POST;

static void
fatal(const char *format, ...) {
	va_list args;

	fprintf(stderr, "named-rrchecker: ");
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputc('\n', stderr);
	exit(1);
}

int
main(int argc, char *argv[]) {
	isc_token_t token;
	isc_result_t result;
	int c;
	unsigned int options = 0;
	dns_rdatatype_t rdtype;
	dns_rdataclass_t rdclass;
	char text[256*1024];
	char data[64*1024];
	isc_buffer_t tbuf;
	isc_buffer_t dbuf;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_boolean_t doexit = ISC_FALSE;
	isc_boolean_t once = ISC_FALSE;
	isc_boolean_t print = ISC_FALSE;
	isc_boolean_t unknown = ISC_FALSE;
	unsigned int t;
	char *origin = NULL;
	dns_fixedname_t fixed;
	dns_name_t *name = NULL;

	while ((c = isc_commandline_parse(argc, argv, "ho:puCPT")) != -1) {
		switch (c) {
		case 'o':
			origin = isc_commandline_argument;
			break;

		case 'p':
			print = ISC_TRUE;
			break;

		case 'u':
			unknown = ISC_TRUE;
			break;

		case 'C':
			for (t = 1; t <= 0xfeffu; t++) {
				if (dns_rdataclass_ismeta(t))
					continue;
				dns_rdataclass_format(t, text, sizeof(text));
				if (strncmp(text, "CLASS", 4) != 0)
					fprintf(stdout, "%s\n", text);
			}
			exit(0);

		case 'P':
			for (t = 0xff00; t <= 0xfffeu; t++) {
				if (dns_rdatatype_ismeta(t))
					continue;
				dns_rdatatype_format(t, text, sizeof(text));
				if (strncmp(text, "TYPE", 4) != 0)
					fprintf(stdout, "%s\n", text);
			}
			doexit = ISC_TRUE;
			break;

		case 'T':
			for (t = 1; t <= 0xfeffu; t++) {
				if (dns_rdatatype_ismeta(t))
					continue;
				dns_rdatatype_format(t, text, sizeof(text));
				if (strncmp(text, "TYPE", 4) != 0)
					fprintf(stdout, "%s\n", text);
			}
			doexit = ISC_TRUE;
			break;

		case '?':
		case 'h':
			/* Does not return. */
			usage();

		default:
			fprintf(stderr, "%s: unhandled option -%c\n",
				argv[0], isc_commandline_option);
			exit(1);
		}
	}
	if (doexit)
		exit(0);

	RUNTIME_CHECK(isc_mem_create(0, 0, &mctx) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_lex_create(mctx, 256, &lex) == ISC_R_SUCCESS);

	/*
	 * Set up to lex DNS master file.
	 */

	specials['('] = 1;
	specials[')'] = 1;
	specials['"'] = 1;
	isc_lex_setspecials(lex, specials);
	options = ISC_LEXOPT_EOL;
	isc_lex_setcomments(lex, ISC_LEXCOMMENT_DNSMASTERFILE);

	RUNTIME_CHECK(isc_lex_openstream(lex, stdin) == ISC_R_SUCCESS);

	if (origin != NULL) {
		name = dns_fixedname_initname(&fixed);
		result = dns_name_fromstring(name, origin, 0, NULL);
		if (result != ISC_R_SUCCESS) {
			fatal("dns_name_fromstring: %s",
			      dns_result_totext(result));
		}
	}

	while ((result = isc_lex_gettoken(lex, options | ISC_LEXOPT_NUMBER,
					  &token)) == ISC_R_SUCCESS) {
		if (token.type == isc_tokentype_eof)
			break;
		if (token.type == isc_tokentype_eol)
			continue;
		if (once) {
			fatal("extra data");
		}
		/*
		 * Get class.
		 */
		if (token.type == isc_tokentype_number) {
			rdclass = (dns_rdataclass_t) token.value.as_ulong;
			if (token.value.as_ulong > 0xffffu) {
				fatal("class value too big %lu",
				      token.value.as_ulong);
			}
			if (dns_rdataclass_ismeta(rdclass)) {
				fatal("class %lu is a meta value",
				      token.value.as_ulong);
			}
		} else if (token.type == isc_tokentype_string) {
			result = dns_rdataclass_fromtext(&rdclass,
					&token.value.as_textregion);
			if (result != ISC_R_SUCCESS) {
				fatal("dns_rdataclass_fromtext: %s",
				      dns_result_totext(result));
			}
			if (dns_rdataclass_ismeta(rdclass)) {
				fatal("class %.*s(%d) is a meta value",
				      (int)token.value.as_textregion.length,
				      token.value.as_textregion.base, rdclass);
			}
		} else {
			fatal("unexpected token %u", token.type);
		}

		result = isc_lex_gettoken(lex, options | ISC_LEXOPT_NUMBER,
					  &token);
		if (result != ISC_R_SUCCESS)
			break;
		if (token.type == isc_tokentype_eol)
			continue;
		if (token.type == isc_tokentype_eof)
			break;

		/*
		 * Get type.
		 */
		if (token.type == isc_tokentype_number) {
			rdtype = (dns_rdatatype_t) token.value.as_ulong;
			if (token.value.as_ulong > 0xffffu) {
				fatal("type value too big %lu",
				      token.value.as_ulong);
			}
			if (dns_rdatatype_ismeta(rdtype)) {
				fatal("type %lu is a meta value",
				      token.value.as_ulong);
			}
		} else if (token.type == isc_tokentype_string) {
			result = dns_rdatatype_fromtext(&rdtype,
					&token.value.as_textregion);
			if (result != ISC_R_SUCCESS) {
				fatal("dns_rdatatype_fromtext: %s",
				      dns_result_totext(result));
			}
			if (dns_rdatatype_ismeta(rdtype)) {
				fatal("type %.*s(%d) is a meta value",
				      (int)token.value.as_textregion.length,
				      token.value.as_textregion.base, rdtype);
			}
		} else {
			fatal("unexpected token %u", token.type);
		}

		isc_buffer_init(&dbuf, data, sizeof(data));
		result = dns_rdata_fromtext(&rdata, rdclass, rdtype, lex,
					    name, 0, mctx, &dbuf, NULL);
		if (result != ISC_R_SUCCESS) {
			fatal("dns_rdata_fromtext: %s",
			      dns_result_totext(result));
		}
		once = ISC_TRUE;
	}
	if (result != ISC_R_EOF) {
		fatal("eof not found");
	}
	if (!once) {
		fatal("no records found");
	}

	if (print) {
		isc_buffer_init(&tbuf, text, sizeof(text));
		result = dns_rdataclass_totext(rdclass, &tbuf);
		if (result != ISC_R_SUCCESS) {
			fatal("dns_rdataclass_totext: %s",
			      dns_result_totext(result));
		}
		isc_buffer_putstr(&tbuf, "\t");
		result = dns_rdatatype_totext(rdtype, &tbuf);
		if (result != ISC_R_SUCCESS) {
			fatal("dns_rdatatype_totext: %s",
			      dns_result_totext(result));
		}
		isc_buffer_putstr(&tbuf, "\t");
		result = dns_rdata_totext(&rdata, NULL, &tbuf);
		if (result != ISC_R_SUCCESS) {
			fatal("dns_rdata_totext: %s",
			      dns_result_totext(result));
		}

		printf("%.*s\n", (int)tbuf.used, (char*)tbuf.base);
		fflush(stdout);
	}

	if (unknown) {
		isc_buffer_init(&tbuf, text, sizeof(text));
		result = dns_rdataclass_tounknowntext(rdclass, &tbuf);
		if (result != ISC_R_SUCCESS) {
			fatal("dns_rdataclass_tounknowntext: %s",
			      dns_result_totext(result));
		}
		isc_buffer_putstr(&tbuf, "\t");
		result = dns_rdatatype_tounknowntext(rdtype, &tbuf);
		if (result != ISC_R_SUCCESS) {
			fatal("dns_rdatatype_tounknowntext: %s",
			      dns_result_totext(result));
		}
		isc_buffer_putstr(&tbuf, "\t");
		result = dns_rdata_tofmttext(&rdata, NULL,
					     DNS_STYLEFLAG_UNKNOWNFORMAT,
					     0, 0, "", &tbuf);
		if (result != ISC_R_SUCCESS) {
			fatal("dns_rdata_tofmttext: %sn",
			      dns_result_totext(result));
		}

		printf("%.*s\n", (int)tbuf.used, (char*)tbuf.base);
		fflush(stdout);
	}

	isc_lex_close(lex);
	isc_lex_destroy(&lex);
	isc_mem_destroy(&mctx);
	return (0);
}
