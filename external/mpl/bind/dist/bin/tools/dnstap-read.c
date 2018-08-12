/*	$NetBSD: dnstap-read.c,v 1.1.1.1 2018/08/12 12:07:15 christos Exp $	*/

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
 * Portions of this code were adapted from dnstap-ldns:
 *
 * Copyright (c) 2014 by Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <config.h>

#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/hex.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/dnstap.h>
#include <dns/fixedname.h>
#include <dns/masterdump.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/result.h>

isc_mem_t *mctx = NULL;
isc_boolean_t memrecord = ISC_FALSE;
isc_boolean_t printmessage = ISC_FALSE;
isc_boolean_t hexmessage = ISC_FALSE;
isc_boolean_t yaml = ISC_FALSE;

const char *program = "dnstap-read";

#define CHECKM(op, msg) \
	do { result = (op);					  \
	       if (result != ISC_R_SUCCESS) {			  \
			fprintf(stderr, 			  \
				"%s: %s: %s\n", program, msg,	  \
				      isc_result_totext(result)); \
			goto cleanup;				  \
		}						  \
	} while (0)

ISC_PLATFORM_NORETURN_PRE static void
fatal(const char *format, ...) ISC_PLATFORM_NORETURN_POST;

static void
fatal(const char *format, ...) {
	va_list args;

	fprintf(stderr, "%s: fatal: ", program);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(1);
}

static void
usage(void) {
	fprintf(stderr, "dnstap-read [-mpxy] [filename]\n");
	fprintf(stderr, "\t-m\ttrace memory allocations\n");
	fprintf(stderr, "\t-p\tprint the full DNS message\n");
	fprintf(stderr, "\t-x\tuse hex format to print DNS message\n");
	fprintf(stderr, "\t-y\tprint YAML format (implies -p)\n");
}

static void
print_dtdata(dns_dtdata_t *dt) {
	isc_result_t result;
	isc_buffer_t *b = NULL;

	isc_buffer_allocate(mctx, &b, 2048);
	if (b == NULL)
		fatal("out of memory");

	CHECKM(dns_dt_datatotext(dt, &b), "dns_dt_datatotext");
	printf("%.*s\n", (int) isc_buffer_usedlength(b),
	       (char *) isc_buffer_base(b));

 cleanup:
	if (b != NULL)
		isc_buffer_free(&b);
}

static void
print_hex(dns_dtdata_t *dt) {
	isc_buffer_t *b = NULL;
	isc_result_t result;
	size_t textlen;

	if (dt->msg == NULL) {
		return;
	}

	textlen = (dt->msgdata.length * 2) + 1;
	isc_buffer_allocate(mctx, &b, textlen);
	if (b == NULL) {
		fatal("out of memory");
	}

	result = isc_hex_totext(&dt->msgdata, 0, "", b);
	CHECKM(result, "isc_hex_totext");

	printf("%.*s\n", (int) isc_buffer_usedlength(b),
	       (char *) isc_buffer_base(b));

 cleanup:
	if (b != NULL)
		isc_buffer_free(&b);
}

static void
print_packet(dns_dtdata_t *dt, const dns_master_style_t *style) {
	isc_buffer_t *b = NULL;
	isc_result_t result;

	if (dt->msg != NULL) {
		size_t textlen = 2048;

		isc_buffer_allocate(mctx, &b, textlen);
		if (b == NULL)
			fatal("out of memory");

		for (;;) {
			isc_buffer_reserve(&b, textlen);
			if (b == NULL)
				fatal("out of memory");

			result = dns_message_totext(dt->msg, style, 0, b);
			if (result == ISC_R_NOSPACE) {
				textlen *= 2;
				continue;
			} else if (result == ISC_R_SUCCESS) {
				printf("%.*s",
				       (int) isc_buffer_usedlength(b),
				       (char *) isc_buffer_base(b));
				isc_buffer_free(&b);
			} else {
				isc_buffer_free(&b);
				CHECKM(result, "dns_message_totext");
			}
			break;
		}
	}

 cleanup:
	if (b != NULL)
		isc_buffer_free(&b);
}

static void
print_yaml(dns_dtdata_t *dt) {
	Dnstap__Dnstap *frame = dt->frame;
	Dnstap__Message *m = frame->message;
	const ProtobufCEnumValue *ftype, *mtype;
	static isc_boolean_t first = ISC_TRUE;

	ftype = protobuf_c_enum_descriptor_get_value(
				     &dnstap__dnstap__type__descriptor,
				     frame->type);
	if (ftype == NULL)
		return;

	if (!first)
		printf("---\n");
	else
		first = ISC_FALSE;

	printf("type: %s\n", ftype->name);

	if (frame->has_identity)
		printf("identity: %.*s\n", (int) frame->identity.len,
		       frame->identity.data);

	if (frame->has_version)
		printf("version: %.*s\n", (int) frame->version.len,
		       frame->version.data);

	if (frame->type != DNSTAP__DNSTAP__TYPE__MESSAGE)
		return;

	printf("message:\n");

	mtype = protobuf_c_enum_descriptor_get_value(
				     &dnstap__message__type__descriptor,
				     m->type);
	if (mtype == NULL)
		return;

	printf("  type: %s\n", mtype->name);

	if (!isc_time_isepoch(&dt->qtime)) {
		char buf[100];
		isc_time_formatISO8601(&dt->qtime, buf, sizeof(buf));
		printf("  query_time: !!timestamp %s\n", buf);
	}

	if (!isc_time_isepoch(&dt->rtime)) {
		char buf[100];
		isc_time_formatISO8601(&dt->rtime, buf, sizeof(buf));
		printf("  response_time: !!timestamp %s\n", buf);
	}

	if (dt->msgdata.base != NULL) {
		printf("  message_size: %zub\n", (size_t) dt->msgdata.length);
	} else
		printf("  message_size: 0b\n");

	if (m->has_socket_family) {
		const ProtobufCEnumValue *type =
			protobuf_c_enum_descriptor_get_value(
				&dnstap__socket_family__descriptor,
				m->socket_family);
		if (type != NULL)
			printf("  socket_family: %s\n", type->name);
	}

	printf("  socket_protocol: %s\n", dt->tcp ? "TCP" : "UDP");

	if (m->has_query_address) {
		ProtobufCBinaryData *ip = &m->query_address;
		char buf[100];

		(void)inet_ntop(ip->len == 4 ? AF_INET : AF_INET6,
				ip->data, buf, sizeof(buf));
		printf("  query_address: %s\n", buf);
	}

	if (m->has_response_address) {
		ProtobufCBinaryData *ip = &m->response_address;
		char buf[100];

		(void)inet_ntop(ip->len == 4 ? AF_INET : AF_INET6,
				ip->data, buf, sizeof(buf));
		printf("  response_address: %s\n", buf);
	}

	if (m->has_query_port)
		printf("  query_port: %u\n", m->query_port);

	if (m->has_response_port)
		printf("  response_port: %u\n", m->response_port);

	if (m->has_query_zone) {
		isc_result_t result;
		dns_fixedname_t fn;
		dns_name_t *name;
		isc_buffer_t b;
		dns_decompress_t dctx;

		name = dns_fixedname_initname(&fn);

		isc_buffer_init(&b, m->query_zone.data, m->query_zone.len);
		isc_buffer_add(&b, m->query_zone.len);

		dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_NONE);
		result = dns_name_fromwire(name, &b, &dctx, 0, NULL);
		if (result == ISC_R_SUCCESS) {
			printf("  query_zone: ");
			dns_name_print(name, stdout);
			printf("\n");
		}
	}

	if (dt->msg != NULL) {
		printf("  %s:\n", ((dt->type & DNS_DTTYPE_QUERY) != 0)
				     ? "query_message_data"
				     : "response_message_data");

		print_packet(dt, &dns_master_style_yaml);

		printf("  %s: |\n", ((dt->type & DNS_DTTYPE_QUERY) != 0)
				     ? "query_message"
				     : "response_message");
		print_packet(dt, &dns_master_style_indent);
	}
};

int
main(int argc, char *argv[]) {
	isc_result_t result;
	dns_message_t *message = NULL;
	isc_buffer_t *b = NULL;
	dns_dtdata_t *dt = NULL;
	dns_dthandle_t *handle = NULL;
	int rv = 0, ch;

	while ((ch = isc_commandline_parse(argc, argv, "mpxy")) != -1) {
		switch (ch) {
			case 'm':
				isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
				memrecord = ISC_TRUE;
				break;
			case 'p':
				printmessage = ISC_TRUE;
				break;
			case 'x':
				hexmessage = ISC_TRUE;
				break;
			case 'y':
				yaml = ISC_TRUE;
				dns_master_indentstr = "  ";
				dns_master_indent = 2;
				break;
			default:
				usage();
				exit(1);
		}
	}

	argc -= isc_commandline_index;
	argv += isc_commandline_index;

	if (argc < 1)
		fatal("no file specified");

	RUNTIME_CHECK(isc_mem_create(0, 0, &mctx) == ISC_R_SUCCESS);

	dns_result_register();

	CHECKM(dns_dt_open(argv[0], dns_dtmode_file, mctx, &handle),
	       "dns_dt_openfile");

	for (;;) {
		isc_region_t input;
		isc_uint8_t *data;
		size_t datalen;

		result = dns_dt_getframe(handle, &data, &datalen);
		if (result == ISC_R_NOMORE)
			break;
		else
			CHECKM(result, "dns_dt_getframe");

		input.base = data;
		input.length = datalen;

		if (b != NULL)
			isc_buffer_free(&b);
		isc_buffer_allocate(mctx, &b, 2048);
		if (b == NULL)
			fatal("out of memory");

		result = dns_dt_parse(mctx, &input, &dt);
		if (result != ISC_R_SUCCESS) {
			isc_buffer_free(&b);
			continue;
		}

		if (yaml) {
			print_yaml(dt);
		} else if (hexmessage) {
			print_dtdata(dt);
			print_hex(dt);
		} else if (printmessage) {
			print_dtdata(dt);
			print_packet(dt, &dns_master_style_debug);
		} else {
			print_dtdata(dt);
		}

		dns_dtdata_free(&dt);
	}

 cleanup:
	if (dt != NULL)
		dns_dtdata_free(&dt);
	if (handle != NULL)
		dns_dt_close(&handle);
	if (message != NULL)
		dns_message_destroy(&message);
	if (b != NULL)
		isc_buffer_free(&b);
	isc_mem_destroy(&mctx);

	exit(rv);
}
