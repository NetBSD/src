/*	$NetBSD: rdata_test.c,v 1.7 2018/04/07 22:23:22 christos Exp $	*/

/*
 * Copyright (C) 2012, 2013, 2015-2017  Internet Systems Consortium, Inc. ("ISC")
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

/*! \file */

#include <config.h>

#include <atf-c.h>

#include <unistd.h>

#include <isc/hex.h>
#include <isc/lex.h>
#include <isc/types.h>

#include <dns/callbacks.h>
#include <dns/rdata.h>

#include "dnstest.h"

/*****
 ***** Commonly used structures
 *****/

/*
 * An array of these structures is passed to check_text_ok().
 */
struct text_ok {
	const char *text_in;		/* text passed to fromtext_*() */
	const char *text_out;		/* text expected from totext_*();
					   NULL indicates text_in is invalid */
	int lineno;			/* source line defining this RDATA */
};
typedef struct text_ok text_ok_t;

/*
 * An array of these structures is passed to check_wire_ok().
 */
struct wire_ok {
	unsigned char data[512];	/* RDATA in wire format */
	size_t len;			/* octets of data to parse */
	isc_boolean_t ok;		/* is this RDATA valid? */
	int lineno;			/* source line defining this RDATA */
};
typedef struct wire_ok wire_ok_t;

/*****
 ***** Convenience macros for creating the above structures
 *****/

#define TEXT_VALID_CHANGED(data_in, data_out) \
				{ data_in, data_out, __LINE__ }
#define TEXT_VALID(data)	{ data, data, __LINE__ }
#define TEXT_INVALID(data)	{ data, NULL, __LINE__ }
#define TEXT_SENTINEL()		TEXT_INVALID(NULL)

#define VARGC(...)		(sizeof((unsigned char[]){ __VA_ARGS__ }))
#define WIRE_TEST(ok, ...)	{					      \
					{ __VA_ARGS__ }, VARGC(__VA_ARGS__),  \
					ok, __LINE__			      \
				}
#define WIRE_VALID(...)		WIRE_TEST(ISC_TRUE, __VA_ARGS__)
#define WIRE_INVALID(...)	WIRE_TEST(ISC_FALSE, __VA_ARGS__)
#define WIRE_SENTINEL()		WIRE_TEST(ISC_FALSE)

/*****
 ***** Checking functions used by test cases
 *****/

/*
 * Test whether converting rdata to a type-specific struct and then back to
 * rdata results in the same uncompressed wire form.  This checks whether
 * tostruct_*() and fromstruct_*() routines for given RR class and type behave
 * consistently.
 *
 * This function is called for every correctly processed input RDATA, from both
 * check_text_ok_single() and check_wire_ok_single().
 */
static void
check_struct_conversions(dns_rdata_t *rdata, size_t structsize, int lineno) {
	dns_rdataclass_t rdclass = rdata->rdclass;
	dns_rdatatype_t type = rdata->type;
	isc_result_t result;
	isc_buffer_t target;
	void *rdata_struct;
	char buf[1024], hex[BUFSIZ];

	rdata_struct = isc_mem_allocate(mctx, structsize);
	ATF_REQUIRE(rdata_struct != NULL);

	/*
	 * Convert from uncompressed wire form into type-specific struct.
	 */
	result = dns_rdata_tostruct(rdata, rdata_struct, NULL);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS,
			   "%s (%u): dns_rdata_tostruct() failed",
			   dns_test_tohex(rdata->data, rdata->length,
					  hex, sizeof(hex)),
			   rdata->length);
	/*
	 * Convert from type-specific struct into uncompressed wire form.
	 */
	isc_buffer_init(&target, buf, sizeof(buf));
	result = dns_rdata_fromstruct(NULL, rdclass, type, rdata_struct,
				      &target);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS,
			   "line %d: %s (%u): dns_rdata_fromstruct() failed",
			   lineno, dns_test_tohex(rdata->data, rdata->length,
						  hex, sizeof(hex)),
			   rdata->length);
	/*
	 * Ensure results are consistent.
	 */
	ATF_REQUIRE_EQ_MSG(isc_buffer_usedlength(&target), rdata->length,
			   "line %d: %s (%u): wire form data length changed "
			   "after converting to type-specific struct and back",
			   lineno, dns_test_tohex(rdata->data, rdata->length,
						  hex, sizeof(hex)),
			   rdata->length);
	ATF_REQUIRE_EQ_MSG(memcmp(buf, rdata->data, rdata->length), 0,
			   "line %d: %s (%u): wire form data different after "
			   "converting to type-specific struct and back",
			   lineno, dns_test_tohex(rdata->data, rdata->length,
						  hex, sizeof(hex)),
			   rdata->length);

	isc_mem_free(mctx, rdata_struct);
}

/*
 * Check whether converting supplied text form RDATA into uncompressed wire
 * form succeeds (tests fromtext_*()).  If so, try converting it back into text
 * form and see if it results in the original text (tests totext_*()).
 */
static void
check_text_ok_single(const text_ok_t *text_ok, dns_rdataclass_t rdclass,
		     dns_rdatatype_t type, size_t structsize)
{
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned char buf_fromtext[1024];
	char buf_totext[1024] = { 0 };
	isc_buffer_t target;
	isc_result_t result;

	/*
	 * Try converting text form RDATA into uncompressed wire form.
	 */
	result = dns_test_rdata_fromstring(&rdata, rdclass, type, buf_fromtext,
					   sizeof(buf_fromtext),
					   text_ok->text_in);
	/*
	 * Check whether result is as expected.
	 */
	if (text_ok->text_out != NULL) {
		ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS,
				   "line %d: '%s': "
				   "expected success, got failure",
				   text_ok->lineno, text_ok->text_in);
	} else {
		ATF_REQUIRE_MSG(result != ISC_R_SUCCESS,
				"line %d: '%s': "
				"expected failure, got success",
				text_ok->lineno, text_ok->text_in);
	}
	/*
	 * If text form RDATA was not parsed correctly, performing any
	 * additional checks is pointless.
	 */
	if (result != ISC_R_SUCCESS) {
		return;
	}
	/*
	 * Try converting uncompressed wire form RDATA back into text form and
	 * check whether the resulting text is the same as the original one.
	 */
	isc_buffer_init(&target, buf_totext, sizeof(buf_totext));
	result = dns_rdata_totext(&rdata, NULL, &target);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS,
			   "line %d: '%s': "
			   "failed to convert rdata back to text form",
			   text_ok->lineno, text_ok->text_in);
	ATF_REQUIRE_EQ_MSG(strcmp(buf_totext, text_ok->text_out), 0,
			   "line %d: '%s': "
			   "converts back to '%s', expected '%s'",
			   text_ok->lineno, text_ok->text_in, buf_totext,
			   text_ok->text_out);
	/*
	 * Perform two-way conversion checks between uncompressed wire form and
	 * type-specific struct.
	 */
	check_struct_conversions(&rdata, structsize, text_ok->lineno);
}

/*
 * Test whether supplied wire form RDATA is properly handled as being either
 * valid or invalid for an RR of given rdclass and type.
 */
static void
check_wire_ok_single(const wire_ok_t *wire_ok, dns_rdataclass_t rdclass,
		     dns_rdatatype_t type, size_t structsize)
{
	isc_buffer_t source, target;
	unsigned char buf[1024];
	dns_decompress_t dctx;
	isc_result_t result;
	dns_rdata_t rdata;
	char hex[BUFSIZ];

	/*
	 * Set up len-octet buffer pointing at data.
	 */
	isc_buffer_constinit(&source, wire_ok->data, wire_ok->len);
	isc_buffer_add(&source, wire_ok->len);
	isc_buffer_setactive(&source, wire_ok->len);
	/*
	 * Initialize target structures.
	 */
	isc_buffer_init(&target, buf, sizeof(buf));
	dns_rdata_init(&rdata);
	/*
	 * Try converting wire data into uncompressed wire form.
	 */
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_ANY);
	result = dns_rdata_fromwire(&rdata, rdclass, type, &source, &dctx, 0,
				    &target);
	dns_decompress_invalidate(&dctx);
	/*
	 * Check whether result is as expected.
	 */
	if (wire_ok->ok) {
		ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS,
				   "line %d: %s (%lu): "
				   "expected success, got failure",
				   wire_ok->lineno,
				   dns_test_tohex(wire_ok->data, wire_ok->len,
						  hex, sizeof(hex)),
				   (unsigned long)wire_ok->len);
	} else {
		ATF_REQUIRE_MSG(result != ISC_R_SUCCESS,
				"line %d: %s (%lu): "
				"expected failure, got success",
				wire_ok->lineno,
				dns_test_tohex(wire_ok->data, wire_ok->len,
					       hex, sizeof(hex)),
				(unsigned long)wire_ok->len);
	}
	/*
	 * If data was parsed correctly, perform two-way conversion checks
	 * between uncompressed wire form and type-specific struct.
	 */
	if (result == ISC_R_SUCCESS) {
		check_struct_conversions(&rdata, structsize, wire_ok->lineno);
	}
}

/*
 * Test fromtext_*() and totext_*() routines for given RR class and type for
 * each text form RDATA in the supplied array.  See the comment for
 * check_text_ok_single() for an explanation of how exactly these routines are
 * tested.
 */
static void
check_text_ok(const text_ok_t *text_ok, dns_rdataclass_t rdclass,
	      dns_rdatatype_t type, size_t structsize)
{
	size_t i;

	/*
	 * Check all entries in the supplied array.
	 */
	for (i = 0; text_ok[i].text_in != NULL; i++) {
		check_text_ok_single(&text_ok[i], rdclass, type, structsize);
	}
}

/*
 * For each wire form RDATA in the supplied array, check whether it is properly
 * handled as being either valid or invalid for an RR of given rdclass and
 * type, then check whether trying to process a zero-length wire data buffer
 * yields the expected result.  This checks whether the fromwire_*() routine
 * for given RR class and type behaves as expected.
 */
static void
check_wire_ok(const wire_ok_t *wire_ok, isc_boolean_t empty_ok,
	      dns_rdataclass_t rdclass, dns_rdatatype_t type,
	      size_t structsize)
{
	wire_ok_t empty_wire = WIRE_TEST(empty_ok);
	size_t i;

	/*
	 * Check all entries in the supplied array.
	 */
	for (i = 0; wire_ok[i].len != 0; i++) {
		check_wire_ok_single(&wire_ok[i], rdclass, type, structsize);
	}

	/*
	 * Check empty wire data.
	 */
	check_wire_ok_single(&empty_wire, rdclass, type, structsize);
}

/*
 * Test whether supplied sets of text form and/or wire form RDATA are handled
 * as expected.  This is just a helper function which should be the only
 * function called for a test case using it, due to the use of dns_test_begin()
 * and dns_test_end().
 *
 * The empty_ok argument denotes whether an attempt to parse a zero-length wire
 * data buffer should succeed or not (it is valid for some RR types).  There is
 * no point in performing a similar check for empty text form RDATA, because
 * dns_rdata_fromtext() returns ISC_R_UNEXPECTEDEND before calling fromtext_*()
 * for the given RR class and type.
 */
static void
check_rdata(const text_ok_t *text_ok, const wire_ok_t *wire_ok,
	    isc_boolean_t empty_ok, dns_rdataclass_t rdclass,
	    dns_rdatatype_t type, size_t structsize)
{
	isc_result_t result;

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	if (text_ok != NULL) {
		check_text_ok(text_ok, rdclass, type, structsize);
	}
	if (wire_ok != NULL) {
		check_wire_ok(wire_ok, empty_ok, rdclass, type, structsize);
	}

	dns_test_end();
}

/*****
 ***** Individual unit tests
 *****/

/*
 * CSYNC tests.
 *
 * RFC 7477:
 *
 * 2.1.  The CSYNC Resource Record Format
 *
 * 2.1.1.  The CSYNC Resource Record Wire Format
 *
 *    The CSYNC RDATA consists of the following fields:
 *
 *                           1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
 *       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                          SOA Serial                           |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |       Flags                   |            Type Bit Map       /
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      /                     Type Bit Map (continued)                  /
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * 2.1.1.1.  The SOA Serial Field
 *
 *    The SOA Serial field contains a copy of the 32-bit SOA serial number
 *    from the child zone.  If the soaminimum flag is set, parental agents
 *    querying children's authoritative servers MUST NOT act on data from
 *    zones advertising an SOA serial number less than this value.  See
 *    [RFC1982] for properly implementing "less than" logic.  If the
 *    soaminimum flag is not set, parental agents MUST ignore the value in
 *    the SOA Serial field.  Clients can set the field to any value if the
 *    soaminimum flag is unset, such as the number zero.
 *
 * (...)
 *
 * 2.1.1.2.  The Flags Field
 *
 *    The Flags field contains 16 bits of boolean flags that define
 *    operations that affect the processing of the CSYNC record.  The flags
 *    defined in this document are as follows:
 *
 *       0x00 0x01: "immediate"
 *
 *       0x00 0x02: "soaminimum"
 *
 *    The definitions for how the flags are to be used can be found in
 *    Section 3.
 *
 *    The remaining flags are reserved for use by future specifications.
 *    Undefined flags MUST be set to 0 by CSYNC publishers.  Parental
 *    agents MUST NOT process a CSYNC record if it contains a 1 value for a
 *    flag that is unknown to or unsupported by the parental agent.
 *
 * 2.1.1.2.1.  The Type Bit Map Field
 *
 *    The Type Bit Map field indicates the record types to be processed by
 *    the parental agent, according to the procedures in Section 3.  The
 *    Type Bit Map field is encoded in the same way as the Type Bit Map
 *    field of the NSEC record, described in [RFC4034], Section 4.1.2.  If
 *    a bit has been set that a parental agent implementation does not
 *    understand, the parental agent MUST NOT act upon the record.
 *    Specifically, a parental agent must not simply copy the data, and it
 *    must understand the semantics associated with a bit in the Type Bit
 *    Map field that has been set to 1.
 */
ATF_TC(csync);
ATF_TC_HEAD(csync, tc) {
	atf_tc_set_md_var(tc, "descr", "CSYNC RDATA manipulations");
}
ATF_TC_BODY(csync, tc) {
	text_ok_t text_ok[] = {
		TEXT_INVALID(""),
		TEXT_INVALID("0"),
		TEXT_VALID("0 0"),
		TEXT_VALID("0 0 A"),
		TEXT_VALID("0 0 NS"),
		TEXT_VALID("0 0 AAAA"),
		TEXT_VALID("0 0 A AAAA"),
		TEXT_VALID("0 0 A NS AAAA"),
		TEXT_INVALID("0 0 A NS AAAA BOGUS"),
		TEXT_SENTINEL()
	};
	wire_ok_t wire_ok[] = {
		/*
		 * Short.
		 */
		WIRE_INVALID(0x00),
		/*
		 * Short.
		 */
		WIRE_INVALID(0x00, 0x00),
		/*
		 * Short.
		 */
		WIRE_INVALID(0x00, 0x00, 0x00),
		/*
		 * Short.
		 */
		WIRE_INVALID(0x00, 0x00, 0x00, 0x00),
		/*
		 * Short.
		 */
		WIRE_INVALID(0x00, 0x00, 0x00, 0x00, 0x00),
		/*
		 * Serial + flags only.
		 */
		WIRE_VALID(0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
		/*
		 * Bad type map.
		 */
		WIRE_INVALID(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
		/*
		 * Bad type map.
		 */
		WIRE_INVALID(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
		/*
		 * Good type map.
		 */
		WIRE_VALID(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
			   0x02),
		/*
		 * Sentinel.
		 */
		WIRE_SENTINEL()
	};

	UNUSED(tc);

	check_rdata(text_ok, wire_ok, ISC_FALSE, dns_rdataclass_in,
		    dns_rdatatype_csync, sizeof(dns_rdata_csync_t));
}

/*
 * DOA tests.
 *
 * draft-durand-doa-over-dns-03:
 *
 * 3.2.  DOA RDATA Wire Format
 *
 *        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *     0: |                                                               |
 *        |                        DOA-ENTERPRISE                         |
 *        |                                                               |
 *        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *     4: |                                                               |
 *        |                           DOA-TYPE                            |
 *        |                                                               |
 *        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *     8: |         DOA-LOCATION          |         DOA-MEDIA-TYPE        /
 *        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *    10: /                                                               /
 *        /                  DOA-MEDIA-TYPE (continued)                   /
 *        /                                                               /
 *        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *        /                                                               /
 *        /                           DOA-DATA                            /
 *        /                                                               /
 *        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *
 *    DOA-ENTERPRISE: a 32-bit unsigned integer in network order.
 *
 *    DOA-TYPE: a 32-bit unsigned integer in network order.
 *
 *    DOA-LOCATION: an 8-bit unsigned integer.
 *
 *    DOA-MEDIA-TYPE: A <character-string> (see [RFC1035]).  The first
 *    octet of the <character-string> contains the number of characters to
 *    follow.
 *
 *    DOA-DATA: A variable length blob of binary data.  The length of the
 *    DOA-DATA is not contained within the wire format of the RR and has to
 *    be computed from the RDLENGTH of the entire RR once other fields have
 *    been taken into account.
 *
 * 3.3.  DOA RDATA Presentation Format
 *
 *    The DOA-ENTERPRISE field is presented as an unsigned 32-bit decimal
 *    integer with range 0 - 4,294,967,295.
 *
 *    The DOA-TYPE field is presented as an unsigned 32-bit decimal integer
 *    with range 0 - 4,294,967,295.
 *
 *    The DOA-LOCATION field is presented as an unsigned 8-bit decimal
 *    integer with range 0 - 255.
 *
 *    The DOA-MEDIA-TYPE field is presented as a single <character-string>.
 *
 *    The DOA-DATA is presented as Base64 encoded data [RFC4648] unless the
 *    DOA-DATA is empty in which case it is presented as a single dash
 *    character ("-", ASCII 45).  White space is permitted within Base64
 *    data.
 */
ATF_TC(doa);
ATF_TC_HEAD(doa, tc) {
	atf_tc_set_md_var(tc, "descr", "DOA RDATA manipulations");
}
ATF_TC_BODY(doa, tc) {
	text_ok_t text_ok[] = {
		/*
		 * Valid, non-empty DOA-DATA.
		 */
		TEXT_VALID("0 0 1 \"text/plain\" Zm9v"),
		/*
		 * Valid, non-empty DOA-DATA with whitespace in between.
		 */
		TEXT_VALID_CHANGED("0 0 1 \"text/plain\" Zm 9v",
				   "0 0 1 \"text/plain\" Zm9v"),
		/*
		 * Valid, unquoted DOA-MEDIA-TYPE, non-empty DOA-DATA.
		 */
		TEXT_VALID_CHANGED("0 0 1 text/plain Zm9v",
				   "0 0 1 \"text/plain\" Zm9v"),
		/*
		 * Invalid, quoted non-empty DOA-DATA.
		 */
		TEXT_INVALID("0 0 1 \"text/plain\" \"Zm9v\""),
		/*
		 * Valid, empty DOA-DATA.
		 */
		TEXT_VALID("0 0 1 \"text/plain\" -"),
		/*
		 * Invalid, quoted empty DOA-DATA.
		 */
		TEXT_INVALID("0 0 1 \"text/plain\" \"-\""),
		/*
		 * Invalid, missing "-" in empty DOA-DATA.
		 */
		TEXT_INVALID("0 0 1 \"text/plain\""),
		/*
		 * Valid, undefined DOA-LOCATION.
		 */
		TEXT_VALID("0 0 100 \"text/plain\" Zm9v"),
		/*
		 * Invalid, DOA-LOCATION too big.
		 */
		TEXT_INVALID("0 0 256 \"text/plain\" ZM9v"),
		/*
		 * Valid, empty DOA-MEDIA-TYPE, non-empty DOA-DATA.
		 */
		TEXT_VALID("0 0 2 \"\" aHR0cHM6Ly93d3cuaXNjLm9yZy8="),
		/*
		 * Valid, empty DOA-MEDIA-TYPE, empty DOA-DATA.
		 */
		TEXT_VALID("0 0 1 \"\" -"),
		/*
		 * Valid, DOA-MEDIA-TYPE with a space.
		 */
		TEXT_VALID("0 0 1 \"plain text\" Zm9v"),
		/*
		 * Invalid, missing DOA-MEDIA-TYPE.
		 */
		TEXT_INVALID("1234567890 1234567890 1"),
		/*
		 * Valid, DOA-DATA over 255 octets.
		 */
		TEXT_VALID("1234567890 1234567890 1 \"image/gif\" "
			   "R0lGODlhKAAZAOMCAGZmZgBmmf///zOZzMz//5nM/zNmmWbM"
			   "/5nMzMzMzACZ/////////////////////yH5BAEKAA8ALAAA"
			   "AAAoABkAAATH8IFJK5U2a4337F5ogRkpnoCJrly7PrCKyh8c"
			   "3HgAhzT35MDbbtO7/IJIHbGiOiaTxVTpSVWWLqNq1UVyapNS"
			   "1wd3OAxug0LhnCubcVhsxysQnOt4ATpvvzHlFzl1AwODhWeF"
			   "AgRpen5/UhheAYMFdUB4SFcpGEGGdQeCAqBBLTuSk30EeXd9"
			   "pEsAbKGxjHqDSE0Sp6ixN4N1BJmbc7lIhmsBich1awPAjkY1"
			   "SZR8bJWrz382SGqIBQQFQd4IsUTaX+ceuudPEQA7"),
		/*
		 * Invalid, bad Base64 in DOA-DATA.
		 */
		TEXT_INVALID("1234567890 1234567890 1 \"image/gif\" R0lGODl"),
		/*
		 * Sentinel.
		 */
		TEXT_SENTINEL()
	};
	wire_ok_t wire_ok[] = {
		/*
		 * Valid, empty DOA-MEDIA-TYPE, empty DOA-DATA.
		 */
		WIRE_VALID(0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
			   0x01, 0x00),
		/*
		 * Invalid, missing DOA-MEDIA-TYPE.
		 */
		WIRE_INVALID(0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
			     0x01),
		/*
		 * Invalid, malformed DOA-MEDIA-TYPE length.
		 */
		WIRE_INVALID(0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
			     0x01, 0xff),
		/*
		 * Valid, empty DOA-DATA.
		 */
		WIRE_VALID(0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
			   0x01, 0x03, 0x66, 0x6f, 0x6f),
		/*
		 * Valid, non-empty DOA-DATA.
		 */
		WIRE_VALID(0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
			   0x01, 0x03, 0x66, 0x6f, 0x6f, 0x62, 0x61, 0x72),
		/*
		 * Valid, DOA-DATA over 255 octets.
		 */
		WIRE_VALID(0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
			   0x01, 0x06, 0x62, 0x69, 0x6e, 0x61, 0x72, 0x79,
			   0x00, 0x66, 0x99, 0xff, 0xff, 0xff, 0x33, 0x99,
			   0xcc, 0xcc, 0xff, 0xff, 0x99, 0xcc, 0xff, 0x33,
			   0x66, 0x99, 0x66, 0xcc, 0xff, 0x99, 0xcc, 0xcc,
			   0xcc, 0xcc, 0xcc, 0x00, 0x99, 0xff, 0xff, 0xff,
			   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			   0xff, 0xff, 0xff, 0xff, 0xff, 0x21, 0xf9, 0x04,
			   0x01, 0x0a, 0x00, 0x0f, 0x00, 0x2c, 0x00, 0x00,
			   0x00, 0x00, 0x28, 0x00, 0x19, 0x00, 0x00, 0x04,
			   0xc7, 0xf0, 0x81, 0x49, 0x2b, 0x95, 0x36, 0x6b,
			   0x8d, 0xf7, 0xec, 0x5e, 0x68, 0x81, 0x19, 0x29,
			   0x9e, 0x80, 0x89, 0xae, 0x5c, 0xbb, 0x3e, 0xb0,
			   0x8a, 0xca, 0x1f, 0x1c, 0xdc, 0x78, 0x00, 0x87,
			   0x34, 0xf7, 0xe4, 0xc0, 0xdb, 0x6e, 0xd3, 0xbb,
			   0xfc, 0x82, 0x48, 0x1d, 0xb1, 0xa2, 0x3a, 0x26,
			   0x93, 0xc5, 0x54, 0xe9, 0x49, 0x55, 0x96, 0x2e,
			   0xa3, 0x6a, 0xd5, 0x45, 0x72, 0x6a, 0x93, 0x52,
			   0xd7, 0x07, 0x77, 0x38, 0x0c, 0x6e, 0x83, 0x42,
			   0xe1, 0x9c, 0x2b, 0x9b, 0x71, 0x58, 0x6c, 0xc7,
			   0x2b, 0x10, 0x9c, 0xeb, 0x78, 0x01, 0x3a, 0x6f,
			   0xbf, 0x31, 0xe5, 0x17, 0x39, 0x75, 0x03, 0x03,
			   0x83, 0x85, 0x67, 0x85, 0x02, 0x04, 0x69, 0x7a,
			   0x7e, 0x7f, 0x52, 0x18, 0x5e, 0x01, 0x83, 0x05,
			   0x75, 0x40, 0x78, 0x48, 0x57, 0x29, 0x18, 0x41,
			   0x86, 0x75, 0x07, 0x82, 0x02, 0xa0, 0x41, 0x2d,
			   0x3b, 0x92, 0x93, 0x7d, 0x04, 0x79, 0x77, 0x7d,
			   0xa4, 0x4b, 0x00, 0x6c, 0xa1, 0xb1, 0x8c, 0x7a,
			   0x83, 0x48, 0x4d, 0x12, 0xa7, 0xa8, 0xb1, 0x37,
			   0x83, 0x75, 0x04, 0x99, 0x9b, 0x73, 0xb9, 0x48,
			   0x86, 0x6b, 0x01, 0x89, 0xc8, 0x75, 0x6b, 0x03,
			   0xc0, 0x8e, 0x46, 0x35, 0x49, 0x94, 0x7c, 0x6c,
			   0x95, 0xab, 0xcf, 0x7f, 0x36, 0x48, 0x6a, 0x88,
			   0x05, 0x04, 0x05, 0x41, 0xde, 0x08, 0xb1, 0x44,
			   0xda, 0x5f, 0xe7, 0x1e, 0xba, 0xe7, 0x4f, 0x11,
			   0x00, 0x3b),
		/*
		 * Sentinel.
		 */
		WIRE_SENTINEL()
	};

	UNUSED(tc);

	check_rdata(text_ok, wire_ok, ISC_FALSE, dns_rdataclass_in,
		    dns_rdatatype_doa, sizeof(dns_rdata_doa_t));
}

/*
 * EDNS Client Subnet tests.
 *
 * RFC 7871:
 *
 * 6.  Option Format
 *
 *    This protocol uses an EDNS0 [RFC6891] option to include client
 *    address information in DNS messages.  The option is structured as
 *    follows:
 *
 *                 +0 (MSB)                            +1 (LSB)
 *       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *    0: |                          OPTION-CODE                          |
 *       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *    2: |                         OPTION-LENGTH                         |
 *       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *    4: |                            FAMILY                             |
 *       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *    6: |     SOURCE PREFIX-LENGTH      |     SCOPE PREFIX-LENGTH       |
 *       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *    8: |                           ADDRESS...                          /
 *       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 *
 *    o  (Defined in [RFC6891]) OPTION-CODE, 2 octets, for ECS is 8 (0x00
 *       0x08).
 *
 *    o  (Defined in [RFC6891]) OPTION-LENGTH, 2 octets, contains the
 *       length of the payload (everything after OPTION-LENGTH) in octets.
 *
 *    o  FAMILY, 2 octets, indicates the family of the address contained in
 *       the option, using address family codes as assigned by IANA in
 *       Address Family Numbers [Address_Family_Numbers].
 *
 *    The format of the address part depends on the value of FAMILY.  This
 *    document only defines the format for FAMILY 1 (IPv4) and FAMILY 2
 *    (IPv6), which are as follows:
 *
 *    o  SOURCE PREFIX-LENGTH, an unsigned octet representing the leftmost
 *       number of significant bits of ADDRESS to be used for the lookup.
 *       In responses, it mirrors the same value as in the queries.
 *
 *    o  SCOPE PREFIX-LENGTH, an unsigned octet representing the leftmost
 *       number of significant bits of ADDRESS that the response covers.
 *       In queries, it MUST be set to 0.
 *
 *    o  ADDRESS, variable number of octets, contains either an IPv4 or
 *       IPv6 address, depending on FAMILY, which MUST be truncated to the
 *       number of bits indicated by the SOURCE PREFIX-LENGTH field,
 *       padding with 0 bits to pad to the end of the last octet needed.
 *
 *    o  A server receiving an ECS option that uses either too few or too
 *       many ADDRESS octets, or that has non-zero ADDRESS bits set beyond
 *       SOURCE PREFIX-LENGTH, SHOULD return FORMERR to reject the packet,
 *       as a signal to the software developer making the request to fix
 *       their implementation.
 *
 *    All fields are in network byte order ("big-endian", per [RFC1700],
 *    Data Notation).
 */
ATF_TC(edns_client_subnet);
ATF_TC_HEAD(edns_client_subnet, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "OPT RDATA with EDNS Client Subnet manipulations");
}
ATF_TC_BODY(edns_client_subnet, tc) {
	wire_ok_t wire_ok[] = {
		/*
		 * Option code with no content.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00, 0x00),
		/*
		 * Option code family 0, source 0, scope 0.
		 */
		WIRE_VALID(0x00, 0x08, 0x00, 0x04,
			   0x00, 0x00, 0x00, 0x00),
		/*
		 * Option code family 1 (IPv4), source 0, scope 0.
		 */
		WIRE_VALID(0x00, 0x08, 0x00, 0x04,
			   0x00, 0x01, 0x00, 0x00),
		/*
		 * Option code family 2 (IPv6) , source 0, scope 0.
		 */
		WIRE_VALID(0x00, 0x08, 0x00, 0x04,
			   0x00, 0x02, 0x00, 0x00),
		/*
		 * Extra octet.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00, 0x05,
			     0x00, 0x00, 0x00, 0x00,
			     0x00),
		/*
		 * Source too long for IPv4.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00,    8,
			     0x00, 0x01,   33, 0x00,
			     0x00, 0x00, 0x00, 0x00),
		/*
		 * Source too long for IPv6.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00,   20,
			     0x00, 0x02,  129, 0x00,
			     0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00),
		/*
		 * Scope too long for IPv4.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00,    8,
			     0x00, 0x01, 0x00,   33,
			     0x00, 0x00, 0x00, 0x00),
		/*
		 * Scope too long for IPv6.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00,   20,
			     0x00, 0x02, 0x00,  129,
			     0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00),
		/*
		 * When family=0, source and scope should be 0.
		 */
		WIRE_VALID(0x00, 0x08, 0x00,    4,
			   0x00, 0x00, 0x00, 0x00),
		/*
		 * When family=0, source and scope should be 0.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00,    5,
			     0x00, 0x00, 0x01, 0x00,
			     0x00),
		/*
		 * When family=0, source and scope should be 0.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00,    5,
			     0x00, 0x00, 0x00, 0x01,
			     0x00),
		/*
		 * Length too short for source IPv4.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00,    7,
			     0x00, 0x01,   32, 0x00,
			     0x00, 0x00, 0x00),
		/*
		 * Length too short for source IPv6.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00,   19,
			     0x00, 0x02,  128, 0x00,
			     0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00),
		/*
		 * Sentinel.
		 */
		WIRE_SENTINEL()
	};

	UNUSED(tc);

	check_rdata(NULL, wire_ok, ISC_TRUE, dns_rdataclass_in,
		    dns_rdatatype_opt, sizeof(dns_rdata_opt_t));
}

/*
 * Successful load test.
 */
ATF_TC(hip);
ATF_TC_HEAD(hip, tc) {
	atf_tc_set_md_var(tc, "descr", "that a oversized HIP record will "
				       "be rejected");
}
ATF_TC_BODY(hip, tc) {
	unsigned char hipwire[DNS_RDATA_MAXLENGTH] = {
				    0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
				    0x04, 0x41, 0x42, 0x43, 0x44, 0x00 };
	unsigned char buf[1024*1024];
	isc_buffer_t source, target;
	dns_rdata_t rdata;
	dns_decompress_t dctx;
	isc_result_t result;
	size_t i;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Fill the rest of input buffer with compression pointers.
	 */
	for (i = 12; i < sizeof(hipwire) - 2; i += 2) {
		hipwire[i] = 0xc0;
		hipwire[i+1] = 0x06;
	}

	isc_buffer_init(&source, hipwire, sizeof(hipwire));
	isc_buffer_add(&source, sizeof(hipwire));
	isc_buffer_setactive(&source, i);
	isc_buffer_init(&target, buf, sizeof(buf));
	dns_rdata_init(&rdata);
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_ANY);
	result = dns_rdata_fromwire(&rdata, dns_rdataclass_in,
				    dns_rdatatype_hip, &source, &dctx,
				    0, &target);
	dns_decompress_invalidate(&dctx);
	ATF_REQUIRE_EQ(result, DNS_R_FORMERR);

	dns_test_end();
}

/*
 * ISDN tests.
 *
 * RFC 1183:
 *
 * 3.2. The ISDN RR
 *
 *    The ISDN RR is defined with mnemonic ISDN and type code 20 (decimal).
 *
 *    An ISDN (Integrated Service Digital Network) number is simply a
 *    telephone number.  The intent of the members of the CCITT is to
 *    upgrade all telephone and data network service to a common service.
 *
 *    The numbering plan (E.163/E.164) is the same as the familiar
 *    international plan for POTS (an un-official acronym, meaning Plain
 *    Old Telephone Service).  In E.166, CCITT says "An E.163/E.164
 *    telephony subscriber may become an ISDN subscriber without a number
 *    change."
 *
 *    ISDN has the following format:
 *
 *    <owner> <ttl> <class> ISDN <ISDN-address> <sa>
 *
 *    The <ISDN-address> field is required; <sa> is optional.
 *
 *    <ISDN-address> identifies the ISDN number of <owner> and DDI (Direct
 *    Dial In) if any, as defined by E.164 [8] and E.163 [7], the ISDN and
 *    PSTN (Public Switched Telephone Network) numbering plan.  E.163
 *    defines the country codes, and E.164 the form of the addresses.  Its
 *    format in master files is a <character-string> syntactically
 *    identical to that used in TXT and HINFO.
 *
 *    <sa> specifies the subaddress (SA).  The format of <sa> in master
 *    files is a <character-string> syntactically identical to that used in
 *    TXT and HINFO.
 *
 *    The format of ISDN is class insensitive.  ISDN RRs cause no
 *    additional section processing.
 *
 *    The <ISDN-address> is a string of characters, normally decimal
 *    digits, beginning with the E.163 country code and ending with the DDI
 *    if any.  Note that ISDN, in Q.931, permits any IA5 character in the
 *    general case.
 *
 *    The <sa> is a string of hexadecimal digits.  For digits 0-9, the
 *    concrete encoding in the Q.931 call setup information element is
 *    identical to BCD.
 *
 *    For example:
 *
 *    Relay.Prime.COM.   IN   ISDN      150862028003217
 *    sh.Prime.COM.      IN   ISDN      150862028003217 004
 *
 *    (Note: "1" is the country code for the North American Integrated
 *    Numbering Area, i.e., the system of "area codes" familiar to people
 *    in those countries.)
 *
 *    The RR data is the ASCII representation of the digits.  It is encoded
 *    as one or two <character-string>s, i.e., count followed by
 *    characters.
 */
ATF_TC(isdn);
ATF_TC_HEAD(isdn, tc) {
	atf_tc_set_md_var(tc, "descr", "ISDN RDATA manipulations");
}
ATF_TC_BODY(isdn, tc) {
	wire_ok_t wire_ok[] = {
		/*
		 * "".
		 */
		WIRE_VALID(0x00),
		/*
		 * "\001".
		 */
		WIRE_VALID(0x01, 0x01),
		/*
		 * "\001" "".
		 */
		WIRE_VALID(0x01, 0x01, 0x00),
		/*
		 * "\001" "\001".
		 */
		WIRE_VALID(0x01, 0x01, 0x01, 0x01),
		/*
		 * Sentinel.
		 */
		WIRE_SENTINEL()
	};

	UNUSED(tc);

	check_rdata(NULL, wire_ok, ISC_FALSE, dns_rdataclass_in,
		    dns_rdatatype_isdn, sizeof(dns_rdata_isdn_t));
}

/*
 * NSEC tests.
 *
 * RFC 4034:
 *
 * 4.1.  NSEC RDATA Wire Format
 *
 *   The RDATA of the NSEC RR is as shown below:
 *
 *                         1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    /                      Next Domain Name                         /
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    /                       Type Bit Maps                           /
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * 4.1.1.  The Next Domain Name Field
 *
 *    The Next Domain field contains the next owner name (in the canonical
 *    ordering of the zone) that has authoritative data or contains a
 *    delegation point NS RRset; see Section 6.1 for an explanation of
 *    canonical ordering.  The value of the Next Domain Name field in the
 *    last NSEC record in the zone is the name of the zone apex (the owner
 *    name of the zone's SOA RR).  This indicates that the owner name of
 *    the NSEC RR is the last name in the canonical ordering of the zone.
 *
 *    A sender MUST NOT use DNS name compression on the Next Domain Name
 *    field when transmitting an NSEC RR.
 *
 *    Owner names of RRsets for which the given zone is not authoritative
 *    (such as glue records) MUST NOT be listed in the Next Domain Name
 *    unless at least one authoritative RRset exists at the same owner
 *    name.
 *
 * 4.1.2.  The Type Bit Maps Field
 *
 *    The Type Bit Maps field identifies the RRset types that exist at the
 *    NSEC RR's owner name.
 *
 *    The RR type space is split into 256 window blocks, each representing
 *    the low-order 8 bits of the 16-bit RR type space.  Each block that
 *    has at least one active RR type is encoded using a single octet
 *    window number (from 0 to 255), a single octet bitmap length (from 1
 *    to 32) indicating the number of octets used for the window block's
 *    bitmap, and up to 32 octets (256 bits) of bitmap.
 *
 *    Blocks are present in the NSEC RR RDATA in increasing numerical
 *    order.
 *
 *       Type Bit Maps Field = ( Window Block # | Bitmap Length | Bitmap )+
 *
 *       where "|" denotes concatenation.
 *
 *    Each bitmap encodes the low-order 8 bits of RR types within the
 *    window block, in network bit order.  The first bit is bit 0.  For
 *    window block 0, bit 1 corresponds to RR type 1 (A), bit 2 corresponds
 *    to RR type 2 (NS), and so forth.  For window block 1, bit 1
 *    corresponds to RR type 257, and bit 2 to RR type 258.  If a bit is
 *    set, it indicates that an RRset of that type is present for the NSEC
 *    RR's owner name.  If a bit is clear, it indicates that no RRset of
 *    that type is present for the NSEC RR's owner name.
 *
 *    Bits representing pseudo-types MUST be clear, as they do not appear
 *    in zone data.  If encountered, they MUST be ignored upon being read.
 */
ATF_TC(nsec);
ATF_TC_HEAD(nsec, tc) {
	atf_tc_set_md_var(tc, "descr", "NSEC RDATA manipulations");
}
ATF_TC_BODY(nsec, tc) {
	text_ok_t text_ok[] = {
		TEXT_INVALID(""),
		TEXT_INVALID("."),
		TEXT_VALID(". RRSIG"),
		TEXT_SENTINEL()
	};
	wire_ok_t wire_ok[] = {
		WIRE_INVALID(0x00),
		WIRE_INVALID(0x00, 0x00),
		WIRE_INVALID(0x00, 0x00, 0x00),
		WIRE_VALID(0x00, 0x00, 0x01, 0x02),
		WIRE_INVALID()
	};

	UNUSED(tc);

	check_rdata(text_ok, wire_ok, ISC_FALSE, dns_rdataclass_in,
		    dns_rdatatype_nsec, sizeof(dns_rdata_nsec_t));
}

/*
 * WKS tests.
 *
 * RFC 1035:
 *
 * 3.4.2. WKS RDATA format
 *
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |                    ADDRESS                    |
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *     |       PROTOCOL        |                       |
 *     +--+--+--+--+--+--+--+--+                       |
 *     |                                               |
 *     /                   <BIT MAP>                   /
 *     /                                               /
 *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * where:
 *
 * ADDRESS         An 32 bit Internet address
 *
 * PROTOCOL        An 8 bit IP protocol number
 *
 * <BIT MAP>       A variable length bit map.  The bit map must be a
 *                 multiple of 8 bits long.
 *
 * The WKS record is used to describe the well known services supported by
 * a particular protocol on a particular internet address.  The PROTOCOL
 * field specifies an IP protocol number, and the bit map has one bit per
 * port of the specified protocol.  The first bit corresponds to port 0,
 * the second to port 1, etc.  If the bit map does not include a bit for a
 * protocol of interest, that bit is assumed zero.  The appropriate values
 * and mnemonics for ports and protocols are specified in [RFC-1010].
 *
 * For example, if PROTOCOL=TCP (6), the 26th bit corresponds to TCP port
 * 25 (SMTP).  If this bit is set, a SMTP server should be listening on TCP
 * port 25; if zero, SMTP service is not supported on the specified
 * address.
 */
ATF_TC(wks);
ATF_TC_HEAD(wks, tc) {
	atf_tc_set_md_var(tc, "descr", "WKS RDATA manipulations");
}
ATF_TC_BODY(wks, tc) {
	wire_ok_t wire_ok[] = {
		/*
		 * Too short.
		 */
		WIRE_INVALID(0x00, 0x08, 0x00, 0x00),
		/*
		 * Minimal TCP.
		 */
		WIRE_VALID(0x00, 0x08, 0x00, 0x00, 6),
		/*
		 * Minimal UDP.
		 */
		WIRE_VALID(0x00, 0x08, 0x00, 0x00, 17),
		/*
		 * Minimal other.
		 */
		WIRE_VALID(0x00, 0x08, 0x00, 0x00, 1),
		/*
		 * Sentinel.
		 */
		WIRE_SENTINEL()
	};

	UNUSED(tc);

	check_rdata(NULL, wire_ok, ISC_FALSE, dns_rdataclass_in,
		    dns_rdatatype_wks, sizeof(dns_rdata_in_wks_t));
}

/*****
 ***** Main
 *****/

ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, csync);
	ATF_TP_ADD_TC(tp, doa);
	ATF_TP_ADD_TC(tp, edns_client_subnet);
	ATF_TP_ADD_TC(tp, hip);
	ATF_TP_ADD_TC(tp, isdn);
	ATF_TP_ADD_TC(tp, nsec);
	ATF_TP_ADD_TC(tp, wks);

	return (atf_no_error());
}
