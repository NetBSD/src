/*	$NetBSD: wire_test.c,v 1.7 2015/12/17 04:00:42 christos Exp $	*/

/*
 * Copyright (C) 2004, 2005, 2007, 2015  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <config.h>

#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/file.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/message.h>
#include <dns/result.h>

int parseflags = 0;
isc_mem_t *mctx = NULL;
isc_boolean_t printmemstats = ISC_FALSE;
isc_boolean_t dorender = ISC_FALSE;

static void
process_message(isc_buffer_t *source);

static isc_result_t
printmessage(dns_message_t *msg);

static inline void
CHECKRESULT(isc_result_t result, const char *msg) {
	if (result != ISC_R_SUCCESS) {
		printf("%s: %s\n", msg, dns_result_totext(result));

		exit(1);
	}
}

static int
fromhex(char c) {
	if (c >= '0' && c <= '9')
		return (c - '0');
	else if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	else if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);

	fprintf(stderr, "bad input format: %02x\n", c);
	exit(3);
	/* NOTREACHED */
}

static void
usage(void) {
	fprintf(stderr, "wire_test [-b] [-d] [-p] [-r] [-s]\n");
	fprintf(stderr, "          [-m {usage|trace|record|size|mctx}]\n");
	fprintf(stderr, "          [filename]\n\n");
	fprintf(stderr, "\t-b\tBest-effort parsing (ignore some errors)\n");
	fprintf(stderr, "\t-d\tRead input as raw binary data\n");
	fprintf(stderr, "\t-p\tPreserve order of the records in messages\n");
	fprintf(stderr, "\t-r\tAfter parsing, re-render the message\n");
	fprintf(stderr, "\t-s\tPrint memory statistics\n");
	fprintf(stderr, "\t-t\tTCP mode - ignore the first 2 bytes\n");
}

static isc_result_t
printmessage(dns_message_t *msg) {
	isc_buffer_t b;
	char *buf = NULL;
	int len = 1024;
	isc_result_t result = ISC_R_SUCCESS;

	do {
		buf = isc_mem_get(mctx, len);
		if (buf == NULL) {
			result = ISC_R_NOMEMORY;
			break;
		}

		isc_buffer_init(&b, buf, len);
		result = dns_message_totext(msg, &dns_master_style_debug,
					    0, &b);
		if (result == ISC_R_NOSPACE) {
			isc_mem_put(mctx, buf, len);
			len *= 2;
		} else if (result == ISC_R_SUCCESS)
			printf("%.*s\n", (int) isc_buffer_usedlength(&b), buf);
	} while (result == ISC_R_NOSPACE);

	if (buf != NULL)
		isc_mem_put(mctx, buf, len);

	return (result);
}

int
main(int argc, char *argv[]) {
	isc_buffer_t *input = NULL;
	isc_boolean_t need_close = ISC_FALSE;
	isc_boolean_t tcp = ISC_FALSE;
	isc_boolean_t rawdata = ISC_FALSE;
	isc_result_t result;
	isc_uint8_t c;
	FILE *f;
	int ch;

#define CMDLINE_FLAGS "bdm:prst"
	/*
	 * Process memory debugging argument first.
	 */
	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
		switch (ch) {
		case 'm':
			if (strcasecmp(isc_commandline_argument, "record") == 0)
				isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
			if (strcasecmp(isc_commandline_argument, "trace") == 0)
				isc_mem_debugging |= ISC_MEM_DEBUGTRACE;
			if (strcasecmp(isc_commandline_argument, "usage") == 0)
				isc_mem_debugging |= ISC_MEM_DEBUGUSAGE;
			if (strcasecmp(isc_commandline_argument, "size") == 0)
				isc_mem_debugging |= ISC_MEM_DEBUGSIZE;
			if (strcasecmp(isc_commandline_argument, "mctx") == 0)
				isc_mem_debugging |= ISC_MEM_DEBUGCTX;
			break;
		default:
			break;
		}
	}
	isc_commandline_reset = ISC_TRUE;

	RUNTIME_CHECK(isc_mem_create(0, 0, &mctx) == ISC_R_SUCCESS);

	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
		switch (ch) {
			case 'b':
				parseflags |= DNS_MESSAGEPARSE_BESTEFFORT;
				break;
			case 'd':
				rawdata = ISC_TRUE;
				break;
			case 'm':
				break;
			case 'p':
				parseflags |= DNS_MESSAGEPARSE_PRESERVEORDER;
				break;
			case 'r':
				dorender = ISC_TRUE;
				break;
			case 's':
				printmemstats = ISC_TRUE;
				break;
			case 't':
				tcp = ISC_TRUE;
				break;
			default:
				usage();
				exit(1);
		}
	}

	argc -= isc_commandline_index;
	argv += isc_commandline_index;

	if (argc >= 1) {
		f = fopen(argv[0], "r");
		if (f == NULL) {
			fprintf(stderr, "%s: fopen failed\n", argv[0]);
			exit(1);
		}
		need_close = ISC_TRUE;
	} else
		f = stdin;

	result = isc_buffer_allocate(mctx, &input, 64 * 1024);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	if (rawdata) {
		while (fread(&c, 1, 1, f) != 0) {
			RUNTIME_CHECK(isc_buffer_availablelength(input) > 0);
			isc_buffer_putuint8(input, (isc_uint8_t) c);
		}
	} else {
		char s[BUFSIZ];

		while (fgets(s, sizeof(s), f) != NULL) {
			char *rp = s, *wp = s;
			size_t i, len = 0;

			while (*rp != '\0') {
				if (*rp == '#')
					break;
				if (*rp != ' ' && *rp != '\t' &&
				    *rp != '\r' && *rp != '\n') {
					*wp++ = *rp;
					len++;
				}
				rp++;
			}
			if (len == 0U)
				break;
			if (len % 2 != 0U) {
				fprintf(stderr, "bad input format: %lu\n",
				       (unsigned long)len);
				exit(1);
			}

			rp = s;
			for (i = 0; i < len; i += 2) {
				c = fromhex(*rp++);
				c *= 16;
				c += fromhex(*rp++);
				RUNTIME_CHECK(isc_buffer_availablelength(input) > 0);
				isc_buffer_putuint8(input, (isc_uint8_t) c);
			}
		}
	}

	if (need_close)
		fclose(f);

	if (tcp) {
		while (isc_buffer_remaininglength(input) != 0) {
			unsigned int tcplen;

			if (isc_buffer_remaininglength(input) < 2) {
				fprintf(stderr, "premature end of packet\n");
				exit(1);
			}
			tcplen = isc_buffer_getuint16(input);

			if (isc_buffer_remaininglength(input) < tcplen) {
				fprintf(stderr, "premature end of packet\n");
				exit(1);
			}
			process_message(input);
		}
	} else
		process_message(input);

	if (input != NULL)
		isc_buffer_free(&input);

	if (printmemstats)
		isc_mem_stats(mctx, stdout);
	isc_mem_destroy(&mctx);

	return (0);
}

static void
process_message(isc_buffer_t *source) {
	dns_message_t *message;
	isc_result_t result;
	int i;

	message = NULL;
	result = dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &message);
	CHECKRESULT(result, "dns_message_create failed");

	result = dns_message_parse(message, source, parseflags);
	if (result == DNS_R_RECOVERABLE)
		result = ISC_R_SUCCESS;
	CHECKRESULT(result, "dns_message_parse failed");

	result = printmessage(message);
	CHECKRESULT(result, "printmessage() failed");

	if (printmemstats)
		isc_mem_stats(mctx, stdout);

	if (dorender) {
		unsigned char b2[64 * 1024];
		isc_buffer_t buffer;
		dns_compress_t cctx;

		isc_buffer_init(&buffer, b2, sizeof(b2));

		/*
		 * XXXMLG
		 * Changing this here is a hack, and should not be done in
		 * reasonable application code, ever.
		*/
		message->from_to_wire = DNS_MESSAGE_INTENTRENDER;

		for (i = 0; i < DNS_SECTION_MAX; i++)
			message->counts[i] = 0;  /* Another hack XXX */

		result = dns_compress_init(&cctx, -1, mctx);
		CHECKRESULT(result, "dns_compress_init() failed");

		result = dns_message_renderbegin(message, &cctx, &buffer);
		CHECKRESULT(result, "dns_message_renderbegin() failed");

		result = dns_message_rendersection(message,
						   DNS_SECTION_QUESTION, 0);
		CHECKRESULT(result,
			    "dns_message_rendersection(QUESTION) failed");

		result = dns_message_rendersection(message,
						   DNS_SECTION_ANSWER, 0);
		CHECKRESULT(result,
			    "dns_message_rendersection(ANSWER) failed");

		result = dns_message_rendersection(message,
						   DNS_SECTION_AUTHORITY, 0);
		CHECKRESULT(result,
			    "dns_message_rendersection(AUTHORITY) failed");

		result = dns_message_rendersection(message,
						   DNS_SECTION_ADDITIONAL, 0);
		CHECKRESULT(result,
			    "dns_message_rendersection(ADDITIONAL) failed");

		dns_message_renderend(message);

		dns_compress_invalidate(&cctx);

		message->from_to_wire = DNS_MESSAGE_INTENTPARSE;
		dns_message_destroy(&message);

		printf("Message rendered.\n");
		if (printmemstats)
			isc_mem_stats(mctx, stdout);

		result = dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE,
					    &message);
		CHECKRESULT(result, "dns_message_create failed");

		result = dns_message_parse(message, &buffer, parseflags);
		CHECKRESULT(result, "dns_message_parse failed");

		result = printmessage(message);
		CHECKRESULT(result, "printmessage() failed");
	}
	dns_message_destroy(&message);
}
