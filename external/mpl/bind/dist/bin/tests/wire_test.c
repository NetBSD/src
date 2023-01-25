/*	$NetBSD: wire_test.c,v 1.8 2023/01/25 21:43:24 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>
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
bool printmemstats = false;
bool dorender = false;

static void
process_message(isc_buffer_t *source);

static isc_result_t
printmessage(dns_message_t *msg);

static void
CHECKRESULT(isc_result_t result, const char *msg) {
	if (result != ISC_R_SUCCESS) {
		printf("%s: %s\n", msg, dns_result_totext(result));

		exit(1);
	}
}

static int
fromhex(char c) {
	if (c >= '0' && c <= '9') {
		return (c - '0');
	} else if (c >= 'a' && c <= 'f') {
		return (c - 'a' + 10);
	} else if (c >= 'A' && c <= 'F') {
		return (c - 'A' + 10);
	}

	fprintf(stderr, "bad input format: %02x\n", c);
	exit(3);
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

		isc_buffer_init(&b, buf, len);
		result = dns_message_totext(msg, &dns_master_style_debug, 0,
					    &b);
		if (result == ISC_R_NOSPACE) {
			isc_mem_put(mctx, buf, len);
			len *= 2;
		} else if (result == ISC_R_SUCCESS) {
			printf("%.*s\n", (int)isc_buffer_usedlength(&b), buf);
		}
	} while (result == ISC_R_NOSPACE);

	if (buf != NULL) {
		isc_mem_put(mctx, buf, len);
	}

	return (result);
}

int
main(int argc, char *argv[]) {
	isc_buffer_t *input = NULL;
	bool need_close = false;
	bool tcp = false;
	bool rawdata = false;
	isc_result_t result;
	uint8_t c;
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
			{
				isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
			}
			if (strcasecmp(isc_commandline_argument, "trace") == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGTRACE;
			}
			if (strcasecmp(isc_commandline_argument, "usage") == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGUSAGE;
			}
			if (strcasecmp(isc_commandline_argument, "size") == 0) {
				isc_mem_debugging |= ISC_MEM_DEBUGSIZE;
			}
			if (strcasecmp(isc_commandline_argument, "mctx") == 0) {
				isc_mem_debugging |= ISC_MEM_DEBUGCTX;
			}
			break;
		default:
			break;
		}
	}
	isc_commandline_reset = true;

	isc_mem_create(&mctx);

	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
		switch (ch) {
		case 'b':
			parseflags |= DNS_MESSAGEPARSE_BESTEFFORT;
			break;
		case 'd':
			rawdata = true;
			break;
		case 'm':
			break;
		case 'p':
			parseflags |= DNS_MESSAGEPARSE_PRESERVEORDER;
			break;
		case 'r':
			dorender = true;
			break;
		case 's':
			printmemstats = true;
			break;
		case 't':
			tcp = true;
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
		need_close = true;
	} else {
		f = stdin;
	}

	isc_buffer_allocate(mctx, &input, 64 * 1024);

	if (rawdata) {
		while (fread(&c, 1, 1, f) != 0) {
			result = isc_buffer_reserve(&input, 1);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			isc_buffer_putuint8(input, (uint8_t)c);
		}
	} else {
		char s[BUFSIZ];

		while (fgets(s, sizeof(s), f) != NULL) {
			char *rp = s, *wp = s;
			size_t i, len = 0;

			while (*rp != '\0') {
				if (*rp == '#') {
					break;
				}
				if (*rp != ' ' && *rp != '\t' && *rp != '\r' &&
				    *rp != '\n')
				{
					*wp++ = *rp;
					len++;
				}
				rp++;
			}
			if (len == 0U) {
				continue;
			}
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
				result = isc_buffer_reserve(&input, 1);
				RUNTIME_CHECK(result == ISC_R_SUCCESS);
				isc_buffer_putuint8(input, (uint8_t)c);
			}
		}
	}

	if (need_close) {
		fclose(f);
	}

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
	} else {
		process_message(input);
	}

	isc_buffer_free(&input);

	if (printmemstats) {
		isc_mem_stats(mctx, stdout);
	}
	isc_mem_destroy(&mctx);

	return (0);
}

static void
process_message(isc_buffer_t *source) {
	dns_message_t *message;
	isc_result_t result;
	int i;

	message = NULL;
	dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &message);

	result = dns_message_parse(message, source, parseflags);
	if (result == DNS_R_RECOVERABLE) {
		result = ISC_R_SUCCESS;
	}
	CHECKRESULT(result, "dns_message_parse failed");

	result = printmessage(message);
	CHECKRESULT(result, "printmessage() failed");

	if (printmemstats) {
		isc_mem_stats(mctx, stdout);
	}

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

		for (i = 0; i < DNS_SECTION_MAX; i++) {
			message->counts[i] = 0; /* Another hack XXX */
		}

		result = dns_compress_init(&cctx, -1, mctx);
		CHECKRESULT(result, "dns_compress_init() failed");

		result = dns_message_renderbegin(message, &cctx, &buffer);
		CHECKRESULT(result, "dns_message_renderbegin() failed");

		result = dns_message_rendersection(message,
						   DNS_SECTION_QUESTION, 0);
		CHECKRESULT(result, "dns_message_rendersection(QUESTION) "
				    "failed");

		result = dns_message_rendersection(message, DNS_SECTION_ANSWER,
						   0);
		CHECKRESULT(result, "dns_message_rendersection(ANSWER) failed");

		result = dns_message_rendersection(message,
						   DNS_SECTION_AUTHORITY, 0);
		CHECKRESULT(result, "dns_message_rendersection(AUTHORITY) "
				    "failed");

		result = dns_message_rendersection(message,
						   DNS_SECTION_ADDITIONAL, 0);
		CHECKRESULT(result, "dns_message_rendersection(ADDITIONAL) "
				    "failed");

		dns_message_renderend(message);

		dns_compress_invalidate(&cctx);

		message->from_to_wire = DNS_MESSAGE_INTENTPARSE;
		dns_message_detach(&message);

		printf("Message rendered.\n");
		if (printmemstats) {
			isc_mem_stats(mctx, stdout);
		}

		dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &message);

		result = dns_message_parse(message, &buffer, parseflags);
		CHECKRESULT(result, "dns_message_parse failed");

		result = printmessage(message);
		CHECKRESULT(result, "printmessage() failed");
	}
	dns_message_detach(&message);
}
