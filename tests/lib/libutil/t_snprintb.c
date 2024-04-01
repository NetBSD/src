/* $NetBSD: t_snprintb.c,v 1.32 2024/04/01 09:15:51 rillig Exp $ */

/*
 * Copyright (c) 2002, 2004, 2008, 2010, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Christos Zoulas and
 * Roland Illig.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2008, 2010, 2024\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_snprintb.c,v 1.32 2024/04/01 09:15:51 rillig Exp $");

#include <stdio.h>
#include <string.h>
#include <util.h>
#include <vis.h>

#include <atf-c.h>

static const char *
vis_arr(char *buf, size_t bufsize, const char *arr, size_t arrsize)
{
	ATF_REQUIRE(bufsize >= 2);
	int rv = strnvisx(buf + 1, bufsize - 2, arr, arrsize,
	    VIS_WHITE | VIS_OCTAL);
	ATF_REQUIRE_MSG(rv >= 0, "buffer too small for size %zu", arrsize);
	buf[0] = '"';
	buf[1 + rv] = '"';
	buf[1 + rv + 1] = '\0';
	return buf;
}

static void
check_snprintb_m(const char *file, size_t line,
    size_t bufsize, const char *bitfmt, size_t bitfmtlen, uint64_t val,
    size_t line_max,
    int want_rv, const char *want_buf, size_t want_bufsize)
{
	char buf[1024], vis_bitfmt[1024], vis_want_buf[1024], vis_buf[1024];

	ATF_REQUIRE(bufsize <= sizeof(buf));
	ATF_REQUIRE(want_bufsize <= sizeof(buf));
	if (bitfmtlen > 2 && bitfmt[0] == '\177')
		ATF_REQUIRE_MSG(
		    bitfmt[bitfmtlen - 1] == '\0',
		    "%s:%zu: missing trailing '\\0' in new-style bitfmt",
		    file, line);
	if (bufsize == 0)
		want_bufsize = 0;
	memset(buf, 0x5a, sizeof(buf));

	int rv = snprintb_m(buf, bufsize, bitfmt, val, line_max);

	size_t have_bufsize = sizeof(buf);
	while (have_bufsize > 0 && buf[have_bufsize - 1] == 0x5a)
		have_bufsize--;
	if (rv > 0 && (unsigned)rv < have_bufsize
	    && buf[rv - 1] == '\0' && buf[rv] == '\0')
		have_bufsize = rv + 1;
	if (rv < 0)
		for (size_t i = have_bufsize; i >= 2; i--)
			if (buf[i - 2] == '\0' && buf[i - 1] == '\0')
				have_bufsize = i;

	ATF_CHECK_MSG(
	    rv == want_rv
	    && memcmp(buf, want_buf, want_bufsize) == 0
	    && (line_max == 0 || have_bufsize < 2
		|| buf[have_bufsize - 2] == '\0')
	    && (have_bufsize < 1 || buf[have_bufsize - 1] == '\0'),
	    "failed:\n"
	    "\ttest case: %s:%zu\n"
	    "\tformat: %s\n"
	    "\tvalue: %#jx\n"
	    "\tline_max: %zu\n"
	    "\twant: %d bytes %s\n"
	    "\thave: %d bytes %s\n",
	    file, line,
	    vis_arr(vis_bitfmt, sizeof(vis_bitfmt), bitfmt, bitfmtlen),
	    (uintmax_t)val,
	    line_max,
	    want_rv, vis_arr(vis_want_buf, sizeof(vis_want_buf),
		want_buf, want_bufsize),
	    rv, vis_arr(vis_buf, sizeof(vis_buf), buf, have_bufsize));
}

#define	h_snprintb_m_len(bufsize, bitfmt, val, line_max,		\
	    want_rv, want_buf)						\
	check_snprintb_m(__FILE__, __LINE__,				\
	    bufsize, bitfmt, sizeof(bitfmt) - 1, val, line_max,		\
	    want_rv, want_buf, sizeof(want_buf))

#define	h_snprintb(bitfmt, val, want_buf)				\
	h_snprintb_m_len(1024, bitfmt, val, 0, sizeof(want_buf) - 1, want_buf)

#define	h_snprintb_len(bufsize, bitfmt, val, want_rv, want_buf)		\
	h_snprintb_m_len(bufsize, bitfmt, val, 0, want_rv, want_buf)

#define	h_snprintb_error(bitfmt, want_buf)				\
	h_snprintb_m_len(1024, bitfmt, 0x00, 0, -1, want_buf)

#define	h_snprintb_m(bitfmt, val, line_max, want_buf)			\
	h_snprintb_m_len(1024, bitfmt, val, line_max,			\
	    sizeof(want_buf) - 1, want_buf)

ATF_TC(snprintb);
ATF_TC_HEAD(snprintb, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks snprintb(3)");
}
ATF_TC_BODY(snprintb, tc)
{

	// style and number base, old style, octal, zero value
	//
	// The value 0 does not get a leading '0'.
	h_snprintb(
	    "\010",
	    0x00,
	    "0");

	// style and number base, old style, octal, nonzero value
	//
	// Nonzero octal values get a leading '0'.
	h_snprintb(
	    "\010",
	    0xff,
	    "0377");

	// style and number base, old style, decimal, zero value
	h_snprintb(
	    "\012",
	    0x00,
	    "0");

	// style and number base, old style, decimal, nonzero value
	h_snprintb(
	    "\012",
	    0xff,
	    "255");

	// style and number base, old style, hexadecimal, zero value
	//
	// The value 0 does not get a leading '0x'.
	h_snprintb(
	    "\020",
	    0x00,
	    "0");

	// style and number base, old style, hexadecimal, nonzero value
	//
	// Nonzero hexadecimal values get a leading '0x'.
	h_snprintb(
	    "\177\020",
	    0xff,
	    "0xff");

	// style and number base, old style, invalid base 0
	h_snprintb_error(
	    "",
	    "#");

	// style and number base, old style, invalid base 2
	h_snprintb_error(
	    "\002",
	    "#");

	// style and number base, old style, invalid base 255 or -1
	h_snprintb_error(
	    "\377",
	    "#");

	// style and number base, new style, octal, zero value
	//
	// The value 0 does not get a leading '0'.
	h_snprintb(
	    "\177\010",
	    0x00,
	    "0");

	// style and number base, new style, octal, nonzero value
	//
	// Nonzero octal values get a leading '0'.
	h_snprintb(
	    "\177\010",
	    0xff,
	    "0377");

	// style and number base, new style, decimal, zero value
	h_snprintb(
	    "\177\012",
	    0x00,
	    "0");

	// style and number base, new style, decimal, nonzero value
	h_snprintb(
	    "\177\012",
	    0xff,
	    "255");

	// style and number base, new style, hexadecimal, zero value
	//
	// The value 0 does not get a leading '0x'.
	h_snprintb(
	    "\177\020",
	    0x00,
	    "0");

	// style and number base, new style, hexadecimal, nonzero value
	//
	// Nonzero hexadecimal values get a leading '0x'.
	h_snprintb(
	    "\177\020",
	    0xff,
	    "0xff");

	// style and number base, new style, invalid number base 0
	h_snprintb_error(
	    "\177",
	    "#");

	// style and number base, new style, invalid number base 2
	h_snprintb_error(
	    "\177\002",
	    "#");

	// style and number base, new style, invalid number base 255 or -1
	h_snprintb_error(
	    "\177\377",
	    "#");

	// old style, from lsb to msb
	h_snprintb(
	    "\020"
	    "\001bit1"
	    "\002bit2"
	    "\037bit31"
	    "\040bit32",
	    0xffffffff80000001,
	    "0xffffffff80000001<bit1,bit32>");

	// old style, invalid bit number, at the beginning
	h_snprintb_error(
	    "\020"
	    "\041invalid",
	    "0#");

	// old style, invalid bit number, in the middle
	//
	// The old-style format supports only 32 bits, interpreting the
	// \041 as part of the text belonging to bit 1.
	h_snprintb(
	    "\020"
	    "\001bit1"
	    "\041bit33",
	    0x01,
	    "0x1<bit1!bit33>");

	// old style, repeated bit numbers
	//
	// When a bit number is mentioned more than once,
	// this is most likely a typo.
	h_snprintb(
	    "\020"
	    "\001once"
	    "\001again",
	    0x01,
	    "0x1<once,again>");

	// old style, non-printable description
	//
	// The characters ' ' and '\t' are interpreted as bit numbers,
	// not as part of the description; the visual arrangement in this
	// example is intentionally misleading.
	h_snprintb(
	    "\020"
	    "\001least significant"
	    "\002horizontal\ttab"
	    "\003\xC3\xA4",
	    0xff,
	    "0xff<least,horizontal,\xC3\xA4>");

	// old style, empty description
	//
	// Empty descriptions result in multiple commas in a row, which is a
	// mistake.
	h_snprintb(
	    "\020"
	    "\001lsb"
	    "\004"
	    "\005"
	    "\010msb",
	    0xff,
	    "0xff<lsb,,,msb>");

	// old style, buffer size 0, null buffer
	//
	// If the buffer size is 0, the buffer is not accessed at all and
	// may be a null pointer.
	int null_rv_old = snprintb(NULL, 0, "\020\001lsb", 0x01);
	ATF_CHECK_MSG(
	    null_rv_old == 8,
	    "want length 8, have %d", null_rv_old);

	// old style, buffer too small for value
	h_snprintb_len(
	    1, "\020", 0x00,
	    1, "");

	// old style, buffer large enough for zero value
	h_snprintb_len(
	    2, "\020", 0x00,
	    1, "0");

	// old style, buffer too small for nonzero value
	h_snprintb_len(
	    3, "\020", 0x07,
	    3, "0#");

	// old style, buffer large enough for nonzero value
	h_snprintb_len(
	    4, "\020", 0x07,
	    3, "0x7");

	// old style, buffer too small for '<'
	h_snprintb_len(
	    4, "\020\001lsb", 0x07,
	    8, "0x#");

	// old style, buffer too small for description
	h_snprintb_len(
	    7, "\020\001lsb", 0x07,
	    8, "0x7<l#");

	// old style, buffer too small for '>'
	h_snprintb_len(
	    8, "\020\001lsb", 0x07,
	    8, "0x7<ls#");

	// old style, buffer large enough for '>'
	h_snprintb_len(
	    9, "\020\001lsb", 0x07,
	    8, "0x7<lsb>");

	// old style, buffer too small for second description
	h_snprintb_len(
	    9, "\020\001one\002two", 0x07,
	    12, "0x7<one#");

	// old style, buffer too small for second description
	h_snprintb_len(
	    10, "\020\001one\002two", 0x07,
	    12, "0x7<one,#");

	// old style, buffer too small for '>' after second description
	h_snprintb_len(
	    12, "\020\001one\002two", 0x07,
	    12, "0x7<one,tw#");

	// old style, buffer large enough for '>' after second description
	h_snprintb_len(
	    13, "\020\001one\002two", 0x07,
	    12, "0x7<one,two>");

	// new style, buffer size 0, null buffer
	//
	// If the buffer size is 0, the buffer may be NULL.
	int null_rv_new = snprintb(NULL, 0, "\177\020b\000lsb\0", 0x01);
	ATF_CHECK_MSG(
	    null_rv_new == 8,
	    "want length 8, have %d", null_rv_new);

	// new style single bits
	h_snprintb(
	    "\177\020"
	    "b\000lsb\0"
	    "b\001above-lsb\0"
	    "b\037bit31\0"
	    "b\040bit32\0"
	    "b\076below-msb\0"
	    "b\077msb\0",
	    0x8000000180000001,
	    "0x8000000180000001<lsb,bit31,bit32,msb>");

	// new style single bits, duplicate bits
	h_snprintb(
	    "\177\020"
	    "b\000lsb\0"
	    "b\000lsb\0"
	    "b\000lsb\0",
	    0xff,
	    "0xff<lsb,lsb,lsb>");

	// new style single bits, empty description
	h_snprintb(
	    "\177\020"
	    "b\000lsb\0"
	    "b\001\0"
	    "b\002\0"
	    "b\007msb\0"
	    ,
	    0xff,
	    "0xff<lsb,,,msb>");

	// new style single bits, bit number too large
	h_snprintb_error(
	    "\177\020"
	    "b\100too-high\0",
	    "0#");
	h_snprintb_error(
	    "\177\020"
	    "b\377too-high\0",
	    "0#");

	// new style single bits, non-printable description
	//
	// Contrary to the old-style format, the new-style format allows
	// arbitrary characters in the description, even control characters
	// and non-ASCII characters.
	h_snprintb(
	    "\177\020"
	    "b\000space \t \xC3\xA4\0",
	    0x1,
	    "0x1<space \t \xC3\xA4>");

	// new style named bit-field, octal
	//
	// The bit-field value gets a leading '0' iff it is nonzero.
	h_snprintb(
	    "\177\010"
	    "f\000\010byte0\0"
	    "f\010\010byte1\0",
	    0x0100,
	    "0400<byte0=0,byte1=01>");

	// new style named bit-field, decimal
	h_snprintb(
	    "\177\012"
	    "f\000\010byte0\0"
	    "f\010\010byte1\0",
	    0x0100,
	    "256<byte0=0,byte1=1>");

	// new style named bit-field, hexadecimal
	//
	// The bit-field value gets a leading '0x' iff it is nonzero.
	h_snprintb(
	    "\177\020"
	    "f\000\010byte0\0"
	    "f\010\010byte1\0",
	    0x0100,
	    "0x100<byte0=0,byte1=0x1>");

	// new style bit-field, from 0 width 0
	h_snprintb(
	    "\177\020"
	    "f\000\000zero-width\0"
		"=\000zero\0",
	    0xffff,
	    "0xffff<zero-width=0=zero>");

	// new style bit-field, from 0 width 1
	h_snprintb(
	    "\177\020"
	    "f\000\001lsb\0"
		"=\000zero\0"
		"=\001one\0",
	    0x0,
	    "0<lsb=0=zero>");
	h_snprintb(
	    "\177\020"
	    "f\000\001lsb\0"
		"=\000zero\0"
		"=\001one\0",
	    0x1,
	    "0x1<lsb=0x1=one>");

	// new style bit-field, from 0 width 63
	h_snprintb(
	    "\177\020"
	    "f\000\077uint63\0"
		"=\125match\0",
	    0xaaaa5555aaaa5555,
	    "0xaaaa5555aaaa5555<uint63=0x2aaa5555aaaa5555>");

	// new style bit-field, from 0 width 64
	h_snprintb(
	    "\177\020"
	    "f\000\100uint64\0"
		"=\125match\0",
	    0xaaaa5555aaaa5555,
	    "0xaaaa5555aaaa5555<uint64=0xaaaa5555aaaa5555>");

	// new style bit-field, from 0 width 65
	h_snprintb_error(
	    "\177\020"
	    "f\000\101uint65\0",
	    "0#");

	// new style bit-field, from 1 width 8
	h_snprintb(
	    "\177\020"
	    "f\001\010uint8\0"
		"=\203match\0",
	    0x0106,
	    "0x106<uint8=0x83=match>");

	// new style bit-field, from 1 width 9
	//
	// The '=' and ':' directives can match a bit-field value between
	// 0 and 255, independent of the bit-field's width.
	h_snprintb(
	    "\177\020"
	    "f\001\011uint9\0"
		"=\203match\0"
		"*=default-f\0"
	    "F\001\011\0"
		":\203match\0"
		"*default-F\0",
	    0x0306,
	    "0x306<uint9=0x183=default-f,default-F>");

	// new style bit-field, from 24 width 32
	h_snprintb(
	    "\177\020"
	    "f\030\040uint32\0",
	    0xaaaa555500000000,
	    "0xaaaa555500000000<uint32=0xaa555500>");

	// new style bit-field, from 60 width 4
	h_snprintb(
	    "\177\020"
	    "f\074\004uint4\0",
	    0xf555555555555555,
	    "0xf555555555555555<uint4=0xf>");

	// new style bit-field, from 60 width 5
	//
	// The end of the bit-field is out of bounds.
	h_snprintb(
	    "\177\020"
	    "f\074\005uint5\0",
	    0xf555555555555555,
	    "0xf555555555555555<uint5=0xf>");

	// new style bit-field, from 64 width 0
	//
	// The beginning of the bit-field is out of bounds, the end is fine.
	h_snprintb_error(
	    "\177\020"
	    "f\100\000uint0\0",
	    "0#");

	// new style bit-field, from 65 width 0
	//
	// The beginning and end of the bit-field are out of bounds.
	h_snprintb_error(
	    "\177\020"
	    "f\101\000uint0\0",
	    "0#");

	// new style bit-field, empty field description
	//
	// An empty field description results in an isolated '=', which is a
	// mistake.
	h_snprintb(
	    "\177\020"
	    "f\000\004\0"
		"=\001one\0",
	    0x1,
	    "0x1<=0x1=one>");

	// new style bit-field, non-printable description
	//
	// Contrary to the old-style format, the new-style format allows
	// arbitrary characters in the description, even control characters
	// and non-ASCII characters.
	h_snprintb(
	    "\177\020"
	    "f\000\010\t \xC3\xA4\0"
		"=\001\t \xC3\xA4\0"
	    "F\000\010\0"
		":\001\t \xC3\xA4\0"
	    "F\000\010\0"
		"*\t \xC3\xA4\0",
	    0x1,
	    "0x1<\t \xC3\xA4=0x1=\t \xC3\xA4,\t \xC3\xA4,\t \xC3\xA4>");

	// new style bit-field, '=' with empty description
	//
	// The description of a '=' directive should not be empty, as the
	// outputs contains several '=' in a row.
	h_snprintb(
	    "\177\020"
	    "f\000\004f\0"
		"=\001one\0"
		"=\001\0"
		"=\001\0",
	    0x1,
	    "0x1<f=0x1=one==>");

	// new style bit-field, 'F' followed by ':' with empty description
	//
	// An empty description of a ':' directive that doesn't match results
	// in empty angle brackets, which is a mistake.
	h_snprintb(
	    "\177\020"
	    "F\000\004\0"
		":\001\0"
		"*default\0",
	    0x1,
	    "0x1<>");

	// new style bit-field, 'F', ':' with empty description, '*'
	//
	// An empty description of a ':' directive that matches results in
	// normal-looking output, but if it didn't match, the output would
	// contain empty angle brackets, which is a mistake.
	h_snprintb(
	    "\177\020"
	    "F\000\004\0"
		":\001\0"
		"*default\0",
	    0x2,
	    "0x2<default>");

	// new style bit-field, 'f' with non-exhaustive '='
	h_snprintb(
	    "\177\020"
	    "f\000\004Field\0"
		"=\1one\0"
		"=\2two\0",
	    0x3,
	    "0x3<Field=0x3>");

	// new style bit-field, 'F' with non-exhaustive ':'
	//
	// An unnamed bit-field that does not match any values generates empty
	// angle brackets, which looks confusing. The ':' directives should
	// either be exhaustive, or there should be a '*' catch-all directive.
	h_snprintb(
	    "\177\020"
	    "F\000\004\0"
		":\1one\0"
		":\2two\0",
	    0x3,
	    "0x3<>");

	// new style bit-field, 'F' with non-exhaustive ':'
	//
	// A bit-field that does not match any values generates multiple commas
	// in a row, which looks confusing. The ':' directives should either be
	// exhaustive, or there should be a '*' catch-all directive.
	h_snprintb(
	    "\177\020"
	    "b\000bit0\0"
	    "F\000\004\0"
		":\1one\0"
		":\2two\0"
	    "b\001bit1\0",
	    0x3,
	    "0x3<bit0,,bit1>");

	// new style bit-field, '=', can never match
	//
	// The extracted value from the bit-field has 7 bits and is thus less
	// than 128, therefore it can neither match 128 nor 255.
	h_snprintb(
	    "\177\020"
	    "f\000\007f\0"
		"=\200never\0"
		"=\377never\0",
	    0xff,
	    "0xff<f=0x7f>");

	// new style, two separate bit-fields
	h_snprintb(
	    "\177\020"
	    "f\000\004f1\0"
		"=\001one\0"
		"=\002two\0"
	    "f\004\004f2\0"
		"=\001one\0"
		"=\002two\0",
	    0x12,
	    "0x12<f1=0x2=two,f2=0x1=one>");

	// new style, mixed named and unnamed bit-fields
	h_snprintb(
	    "\177\020"
	    "f\000\004f1\0"
		"=\001one\0"
		"=\002two\0"
	    "F\010\004\0"
		":\015thirteen\0"
	    "f\004\004f2\0"
		"=\001one\0"
		"=\002two\0",
	    0x0d12,
	    "0xd12<f1=0x2=two,thirteen,f2=0x1=one>");

	// new style bit-field, overlapping
	h_snprintb(
	    "\177\020"
	    "f\000\004lo\0"
	    "f\002\004mid\0"
	    "f\004\004hi\0"
	    "f\000\010all\0",
	    0x18,
	    "0x18<lo=0x8,mid=0x6,hi=0x1,all=0x18>");

	// new style bit-field, difference between '=' and ':'
	//
	// The ':' directive can almost emulate the '=' directive, without the
	// numeric output and with a different separator. It's best to use
	// either 'f' with '=', or 'F' with ':', but not mix them.
	h_snprintb(
	    "\177\020"
	    "f\000\004field\0"
		"=\010f-value\0"
	    "F\000\000\0"		// Use an empty bit-field
		":\000separator\0"	// to generate a separator.
	    "F\000\004\0"
		":\010F-value\0",
	    0x18,
	    "0x18<field=0x8=f-value,separator,F-value>");

	// new style bit-field default, fixed string
	//
	// The 'f' directive pairs up with the '=' directive,
	// the 'F' directive pairs up with the ':' directive,
	// but there's only one 'default' directive for both variants,
	// so its description should include the '=' when used with 'f' but
	// not with 'F'.
	h_snprintb(
	    "\177\020"
	    "f\030\010f1\0"
		"*default\0"
	    "f\020\010f2\0"
		"*=default\0"
	    "F\010\010\0"
		"*default\0"
	    "F\010\010\0"
		"*=default\0",
	    0x11223344,
	    "0x11223344<f1=0x11default,f2=0x22=default,default,=default>");

	// new style bit-field default, numeric conversion specifier
	h_snprintb(
	    "\177\020"
	    "f\010\010f\0"
		"*=f(%ju)\0"
	    "F\000\010F\0"
		"*F(%ju)\0",
	    0x1122,
	    "0x1122<f=0x11=f(17),F(34)>");

	// new style bit-field default, can never match
	//
	// The '=' directive are exhaustive, making the '*' redundant.
	h_snprintb(
	    "\177\020"
	    "f\010\002f\0"
		"=\000zero\0"
		"=\001one\0"
		"=\002two\0"
		"=\003three\0"
		"*default\0",
	    0xff00,
	    "0xff00<f=0x3=three>");

	// new style bit-field default, invalid conversion specifier
	//
	// There is no reliable way to make snprintf return an error, as such
	// errors are defined as undefined behavior in the C standard.
	// Instead, here's a conversion specifier that produces a literal '%'.
	h_snprintb(
	    "\177\020"
	    "f\000\010f\0"
		"*=%030ju%%\0",
	    0xff,
	    "0xff<f=0xff=000000000000000000000000000255%>");

	// new style unknown directive, at the beginning
	h_snprintb_len(
	    128,
	    "\177\020"
	    "unknown\0",
	    0xff,
	    -1,
	    "0xff#");

	// new style unknown directive, after a known directive
	h_snprintb_len(
	    128,
	    "\177\020"
	    "b\007msb\0"
	    "unknown\0",
	    0xff,
	    -1,
	    "0xff<msb#");

	// new style combinations, 'b' '='
	//
	// A '=' directive without a preceding 'f' or 'F' directive generates
	// misleading output outside the angle brackets, which is a mistake.
	h_snprintb(
	    "\177\020"
	    "b\004bit4\0"
		"=\000clear\0"
		"=\001set\0"
		"=\245complete\0"
	    "b\000bit0\0"
		"=\000clear\0"
		"=\001set\0"
		"=\245complete\0",
	    0xa5,
	    "0xa5=complete<bit0=complete>");

	// new style combinations, 'b' ':'
	//
	// A ':' directive without a preceding 'f' or 'F' directive generates
	// misleading output outside or inside the angle brackets, which is a
	// mistake.
	h_snprintb(
	    "\177\020"
	    "b\004bit4\0"
		":\000clear\0"
		":\001set\0"
		":\245complete\0"
	    "b\000bit0\0"
		":\000clear\0"
		":\001set\0"
		":\245complete\0",
	    0xa5,
	    "0xa5complete<bit0complete>");

	// new style combinations, 'b' '*'
	//
	// A '*' directive without a preceding 'f' or 'F' directive is ignored.
	h_snprintb(
	    "\177\020"
	    "b\004bit4\0"
		"*default(%ju)\0"
	    "b\000bit0\0"
		"*default(%ju)\0",
	    0xa5,
	    "0xa5<bit0>");

	// new style combinations, 'f' 'b' '='
	//
	// A 'b' directive that occurs between an 'f' and an '=' directive
	// generates misleading output, which is a mistake.
	h_snprintb(
	    "\177\020"
	    "f\000\010f\0"
	    "b\005bit5\0"
		"=\245match\0",
	    0xa5,
	    "0xa5<f=0xa5,bit5=match>");

	// new style combinations, 'f' 'b' ':'
	//
	// A 'b' directive that occurs between an 'f' and a ':' directive
	// generates misleading output, which is a mistake.
	h_snprintb(
	    "\177\020"
	    "f\000\010f\0"
	    "b\005bit5\0"
		":\245match\0",
	    0xa5,
	    "0xa5<f=0xa5,bit5match>");

	// new style combinations, 'f' ':'
	//
	// Combining the 'f' directive with the ':' directive produces the
	// misleading output '0x1one', which is a mistake.
	h_snprintb(
	    "\177\20"
	    "f\000\004nibble\0"
		":\001one\0",
	    0x01,
	    "0x1<nibble=0x1one>");

	// new style combinations, 'F' '='
	//
	// Combining the 'F' and '=' directives outputs an isolated '=', which
	// is a mistake.
	h_snprintb(
	    "\177\20"
	    "F\000\004\0"
		"=\001one\0",
	    0x01,
	    "0x1<=one>");

	// new style combinations, '='
	//
	// A '=' directive without a preceding 'f' or 'F' directive generates
	// output that doesn't match the standard '0xaa<description>' form,
	// which is a mistake.
	h_snprintb(
	    "\177\020"
		"=\245match\0",
	    0xa5,
	    "0xa5=match");

	// new style combinations, ':'
	//
	// A ':' directive without a preceding 'f' or 'F' directive generates
	// misleading output, which is a mistake.
	h_snprintb(
	    "\177\020"
		":\245match\0",
	    0xa5,
	    "0xa5match");

	// new style combinations, '*'
	//
	// A '*' directive without a preceding 'f' or 'F' is useless, which is
	// a mistake.
	h_snprintb(
	    "\177\020"
		"*match\0",
	    0xa5,
	    "0xa5");

	// new style combinations, 'f' '*' '='
	//
	// After a catch-all '*' directive, any following '=' directive
	// generates misleading output, which is a mistake.
	h_snprintb(
	    "\177\020"
	    "f\000\010f\0"
		"*=default\0"
		"=\245match\0",
	    0xa5,
	    "0xa5<f=0xa5=default=match>");

	// new style combinations, 'F' '*' ':'
	//
	// After a catch-all '*' directive, any following ':' directive
	// generates misleading output, which is a mistake.
	h_snprintb(
	    "\177\020"
	    "F\000\010F\0"
		"*default\0"
		":\245-match\0",
	    0xa5,
	    "0xa5<default-match>");

	// new style combinations, '*' '*'
	//
	// After a catch-all '*' directive, any further '*' directive is
	// ignored and thus redundant, which is a mistake.
	h_snprintb(
	    "\177\020"
	    "f\000\010f\0"
		"*=default-f\0"
		"*ignored\0"
	    "F\000\010\0"
		"*default-F\0"
		"*ignored\0",
	    0xa5,
	    "0xa5<f=0xa5=default-f,default-F>");

	// example from the manual page, old style octal
	h_snprintb(
	    "\10\2BITTWO\1BITONE",
	    0x03,
	    "03<BITTWO,BITONE>");

	// example from the manual page, old style hexadecimal
	//
	// When using a hexadecimal escape sequence to encode a bit number,
	// the description must not start with a hexadecimal digit, or that
	// digit is interpreted as part of the bit number. To prevent this,
	// the bit number and the description need to be written as separate
	// string literals.
	h_snprintb(
	    "\20"
	    "\x10NOTBOOT" "\x0f""FPP" "\x0eSDVMA"
	    "\x0cVIDEO" "\x0bLORES" "\x0a""FPA" "\x09""DIAG"
	    "\x07""CACHE" "\x06IOCACHE" "\x05LOOPBACK"
	    "\x04""DBGCACHE",
	    0xe860,
	    "0xe860<NOTBOOT,FPP,SDVMA,VIDEO,CACHE,IOCACHE>");

	// example from the manual page, new style bits and fields
	h_snprintb(
	    "\177\020"
	    "b\0LSB\0" "b\1BITONE\0"
	    "f\4\4NIBBLE2\0"
	    "f\x10\4BURST\0" "=\4FOUR\0" "=\xf""FIFTEEN\0"
	    "b\x1fMSB\0",
	    0x800f0701,
	    "0x800f0701<LSB,NIBBLE2=0,BURST=0xf=FIFTEEN,MSB>");

	// example from the manual page, new style mmap
#define	MAP_FMT				\
	"\177\020"			\
	"b\0"  "SHARED\0"		\
	"b\1"  "PRIVATE\0"		\
	"b\2"  "COPY\0"			\
	"b\4"  "FIXED\0"		\
	"b\5"  "RENAME\0"		\
	"b\6"  "NORESERVE\0"		\
	"b\7"  "INHERIT\0"		\
	"b\11" "HASSEMAPHORE\0"		\
	"b\12" "TRYFIXED\0"		\
	"b\13" "WIRED\0"		\
	"F\14\1\0"			\
		":\0" "FILE\0"		\
		":\1" "ANONYMOUS\0"	\
	"b\15" "STACK\0"		\
	"F\30\010\0"			\
		":\000" "ALIGN=NONE\0"	\
		":\015" "ALIGN=8KB\0"	\
		"*"     "ALIGN=2^%ju\0"
	h_snprintb(
	    MAP_FMT,
	    0x0d001234,
	    "0xd001234<COPY,FIXED,RENAME,HASSEMAPHORE,ANONYMOUS,ALIGN=8KB>");
	h_snprintb(
	    MAP_FMT,
	    0x2e000000,
	    "0x2e000000<FILE,ALIGN=2^46>");

	// It is possible but cumbersome to implement a reduced variant of
	// rot13 using snprintb, shown here for lowercase letters only.
	for (char ch = 'A'; ch <= '~'; ch++) {
		char rot13 = ch >= 'a' && ch <= 'm' ? ch + 13
		    : ch >= 'n' && ch <= 'z' ? ch - 13
		    : '?';
		char expected[8];
		ATF_REQUIRE_EQ(7,
		    snprintf(expected, sizeof(expected), "%#x<%c>", ch, rot13));
		h_snprintb(
		    "\177\020"
		    "F\000\010\0"
		    ":an\0:bo\0:cp\0:dq\0:er\0:fs\0:gt\0:hu\0"
		    ":iv\0:jw\0:kx\0:ly\0:mz\0"
		    ":na\0:ob\0:pc\0:qd\0:re\0:sf\0:tg\0:uh\0"
		    ":vi\0:wj\0:xk\0:yl\0:zm\0"
		    // If snprintf accepted "%jc", it would be possible to
		    // echo the non-alphabetic characters instead of a
		    // catchall question mark.
		    "*?\0",
		    ch,
		    expected);
	}

	// new style, small buffers
	h_snprintb_len(
	    0, "\177\020", 0x00,
	    1, "");
	h_snprintb_len(
	    1, "\177\020", 0x00,
	    1, "");
	h_snprintb_len(
	    2, "\177\020", 0x00,
	    1, "0");
	h_snprintb_len(
	    3, "\177\020", 0x00,
	    1, "0");
	h_snprintb_len(
	    3, "\177\020", 0x07,
	    3, "0#");
	h_snprintb_len(
	    4, "\177\020", 0x07,
	    3, "0x7");
	h_snprintb_len(
	    7, "\177\020b\000lsb\0", 0x07,
	    8, "0x7<l#");
	h_snprintb_len(
	    8, "\177\020b\000lsb\0", 0x07,
	    8, "0x7<ls#");
	h_snprintb_len(
	    9, "\177\020b\000lsb\0", 0x07,
	    8, "0x7<lsb>");
	h_snprintb_len(
	    9, "\177\020b\000one\0b\001two\0", 0x07,
	    12, "0x7<one#");
	h_snprintb_len(
	    10, "\177\020b\000one\0b\001two\0", 0x07,
	    12, "0x7<one,#");
	h_snprintb_len(
	    12, "\177\020b\000one\0b\001two\0", 0x07,
	    12, "0x7<one,tw#");
	h_snprintb_len(
	    13, "\177\020b\000one\0b\001two\0", 0x07,
	    12, "0x7<one,two>");
}

ATF_TC(snprintb_m);
ATF_TC_HEAD(snprintb_m, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks snprintb_m(3)");
}
ATF_TC_BODY(snprintb_m, tc)
{

	// old style, line_max exceeded by number in line 1
	h_snprintb_m(
	    "\020",
	    0xff,
	    1,
	    "#\0");

	// old style, line_max exceeded by '<' in line 1
	h_snprintb_m(
	    "\020"
	    "\001lsb",
	    0xff,
	    4,
	    "0xf#\0");

	// old style, line_max exceeded by description
	h_snprintb_m(
	    "\020"
	    "\001bit1"
	    "\002bit2",
	    0xff,
	    7,
	    "0xff<b#\0"
	    "0xff<b#\0");

	// old style, line_max exceeded by '>' in line 1
	h_snprintb_m(
	    "\020"
	    "\001bit1"
	    "\0022",
	    0xff,
	    9,
	    "0xff<bit#\0"
	    "0xff<2>\0");

	// old style, line_max exceeded by description in line 2
	h_snprintb_m(
	    "\020"
	    "\0011"
	    "\002bit2",
	    0xff,
	    8,
	    "0xff<1>\0"
	    "0xff<bi#\0");

	// old style, line_max exceeded by '>' in line 2
	h_snprintb_m(
	    "\020"
	    "\0011"
	    "\002bit2",
	    0xff,
	    9,
	    "0xff<1>\0"
	    "0xff<bit#\0");

	// old style, complete
	h_snprintb_m(
	    "\020"
	    "\0011"
	    "\002bit2",
	    0xff,
	    10,
	    "0xff<1>\0"
	    "0xff<bit2>\0");

	// new style, line_max exceeded by value in line 1
	h_snprintb_m(
	    "\177\020",
	    0xff,
	    3,
	    "0x#\0");

	// new style, line_max exceeded by single-bit '<' in line 1
	h_snprintb_m(
	    "\177\020"
	    "b\000bit\0",
	    0xff,
	    4,
	    "0xf#\0");

	// new style, line_max exceeded by single-bit description in line 1
	h_snprintb_m(
	    "\177\020"
	    "b\000bit0\0"
	    "b\001two\0",
	    0xff,
	    8,
	    "0xff<bi#\0"
	    "0xff<tw#\0");

	// new style, line_max exceeded by single-bit '>' in line 1
	h_snprintb_m(
	    "\177\020"
	    "b\000bit0\0"
	    "b\001two\0",
	    0xff,
	    9,
	    "0xff<bit#\0"
	    "0xff<two>\0");

	// new style, line_max exceeded by single-bit description in line 2
	h_snprintb_m(
	    "\177\020"
	    "b\000one\0"
	    "b\001three\0",
	    0xff,
	    9,
	    "0xff<one>\0"
	    "0xff<thr#\0");

	// new style, line_max exceeded by single-bit '>' in line 2
	h_snprintb_m(
	    "\177\020"
	    "b\000one\0"
	    "b\001three\0",
	    0xff,
	    10,
	    "0xff<one>\0"
	    "0xff<thre#\0");

	// new style, single-bit complete
	h_snprintb_m(
	    "\177\020"
	    "b\000one\0"
	    "b\001three\0",
	    0xff,
	    11,
	    "0xff<one>\0"
	    "0xff<three>\0");

	// new style, line_max exceeded by named bit-field number in line 1
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0",
	    0xff,
	    3,
	    "0x#\0");

	// new style, line_max exceeded by named bit-field '<' in line 1
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0",
	    0xff,
	    4,
	    "0xf#\0");

	// new style, line_max exceeded by bit-field description in line 1
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0",
	    0xff,
	    6,
	    "0xff<#\0");

	// new style, line_max exceeded by named bit-field '=' in line 1
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0",
	    0xff,
	    7,
	    "0xff<l#\0");

	// new style, line_max exceeded by named bit-field value in line 1
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0",
	    0xff,
	    10,
	    "0xff<lo=0#\0");

	// new style, line_max exceeded by named bit-field '=' in line 1
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0"
		"=\017match\0",
	    0xff,
	    12,
	    "0xff<lo=0xf#\0");

	// new style, line_max exceeded by named bit-field value description in
	// line 1
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0"
		"=\017match\0",
	    0xff,
	    16,
	    "0xff<lo=0xf=mat#\0");

	// new style, line_max exceeded by named bit-field '>' in line 1
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0"
		"=\017match\0",
	    0xff,
	    17,
	    "0xff<lo=0xf=matc#\0");

	// new style, line_max exceeded by named bit-field description in
	// line 2
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0"
	    "f\000\004low-bits\0"
		"=\017match\0",
	    0xff,
	    12,
	    "0xff<lo=0xf>\0"
	    "0xff<low-bi#\0");

	// new style, line_max exceeded by named bit-field '=' in line 2
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0"
	    "f\000\004low-bits\0"
		"=\017match\0",
	    0xff,
	    13,
	    "0xff<lo=0xf>\0"
	    "0xff<low-bit#\0");

	// new style, line_max exceeded by named bit-field value in line 2
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0"
	    "f\000\004low-bits\0"
		"=\017match\0",
	    0xff,
	    16,
	    "0xff<lo=0xf>\0"
	    "0xff<low-bits=0#\0");

	// new style, line_max exceeded by named bit-field '=' in line 2
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0"
	    "f\000\004low-bits\0"
		"=\017match\0",
	    0xff,
	    18,
	    "0xff<lo=0xf>\0"
	    "0xff<low-bits=0xf#\0");

	// new style, line_max exceeded by named bit-field value description
	// in line 2
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0"
	    "f\000\004low-bits\0"
		"=\017match\0",
	    0xff,
	    22,
	    "0xff<lo=0xf>\0"
	    "0xff<low-bits=0xf=mat#\0");

	// new style, line_max exceeded by named bit-field '>' in line 2
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0"
	    "f\000\004low-bits\0"
		"=\017match\0",
	    0xff,
	    23,
	    "0xff<lo=0xf>\0"
	    "0xff<low-bits=0xf=matc#\0");

	// new style, named bit-field complete
	h_snprintb_m(
	    "\177\020"
	    "f\000\004lo\0"
	    "f\000\004low-bits\0"
		"=\017match\0",
	    0xff,
	    24,
	    "0xff<lo=0xf>\0"
	    "0xff<low-bits=0xf=match>\0");

	// new style, line_max exceeded by unnamed bit-field number in line 1
	h_snprintb_m(
	    "\177\020"
	    "F\000\004\0",
	    0xff,
	    3,
	    "0x#\0");

	// new style, line_max exceeded by unnamed bit-field '<' in line 1
	h_snprintb_m(
	    "\177\020"
	    "F\000\004\0",
	    0xff,
	    4,
	    "0xf#\0");

	// new style, line_max exceeded by unnamed bit-field value description
	// in line 1
	h_snprintb_m(
	    "\177\020"
	    "F\000\004\0"
		":\017match\0",
	    0xff,
	    9,
	    "0xff<mat#\0");

	// new style, line_max exceeded by unnamed bit-field '>' in line 1
	h_snprintb_m(
	    "\177\020"
	    "F\000\004\0"
		":\017match\0",
	    0xff,
	    10,
	    "0xff<matc#\0");

	// new style, line_max exceeded by unnamed bit-field value description
	// in line 2
	h_snprintb_m(
	    "\177\020"
	    "F\000\004\0"
		":\017m1\0"
		":\017match\0",
	    0xff,
	    10,
	    "0xff<m1ma#\0");

	// new style, line_max exceeded by unnamed bit-field '>' in line 2
	h_snprintb_m(
	    "\177\020"
	    "F\000\004\0"
		":\017m1\0"
		":\017match\0",
	    0xff,
	    10,
	    "0xff<m1ma#\0");

	// new style unnamed bit-field complete
	h_snprintb_m(
	    "\177\020"
	    "F\000\004\0"
		":\017m1\0"
		":\017match\0",
	    0xff,
	    13,
	    "0xff<m1match>\0");

	// new style, line_max exceeded by bit-field default
	h_snprintb_m(
	    "\177\020"
	    "f\000\004f\0"
		"*=default\0",
	    0xff,
	    17,
	    "0xff<f=0xf=defau#\0");

	// new style, line_max exceeded by unmatched field value
	h_snprintb_m(
	    "\177\020"
	    "f\000\004bits\0"
		":\000zero\0",
	    0xff,
	    11,
	    "0xff<bits=#\0");

	// example from the manual page, new style bits and fields
	h_snprintb_m(
	    "\177\020"
	    "b\0LSB\0"
	    "b\1BITONE\0"
	    "f\4\4NIBBLE2\0"
	    "f\x10\4BURST\0"
		"=\4FOUR\0"
		"=\xf""FIFTEEN\0"
	    "b\x1fMSB\0",
	    0x800f0701,
	    34,
	    "0x800f0701<LSB,NIBBLE2=0>\0"
	    "0x800f0701<BURST=0xf=FIFTEEN,MSB>\0");

	// new style, missing number base
	h_snprintb_m_len(
	    1024,
	    "\177",
	    0xff,
	    128,
	    -1,
	    "#\0");

	// new style, buffer too small for complete number in line 2
	h_snprintb_m_len(
	    15,
	    "\177\020"
	    "b\000lsb\0"
	    "b\001two\0",
	    0xff,
	    11,
	    20,
	    "0xff<lsb>\0"
	    "0x#\0");

	// new-style format, buffer too small for '<' in line 2
	h_snprintb_m_len(
	    16,
	    "\177\020"
	    "b\000lsb\0"
	    "b\001two\0",
	    0xff,
	    11,
	    20,
	    "0xff<lsb>\0"
	    "0xf#\0");

	// new-style format, buffer too small for textual fallback
	h_snprintb_m_len(
	    24,
	    "\177\020"
	    "f\000\004bits\0"
		"*=fallback\0"
	    "b\0024\0",
	    0xff,
	    64,
	    26,
	    "0xff<bits=0xf=fallbac#\0");

	// new-style format, buffer too small for numeric fallback
	h_snprintb_m_len(
	    20,
	    "\177\020"
	    "F\000\004\0"
		"*fallback(%040jd)\0",
	    0xff,
	    64,
	    57,
	    "0xff<fallback(000#\0");

	// new-style format, buffer too small for numeric fallback past buffer
	h_snprintb_m_len(
	    15,
	    "\177\020"
	    "F\000\004\0"
		"*fallback(%010jd)\0"
	    "F\004\004\0"
		"*fallback(%010jd)\0",
	    0xff,
	    64,
	    48,
	    "0xff<fallbac#\0");

	// new style, bits and fields, line break between fields
	h_snprintb_m(
	    "\177\020"
	    "b\0LSB\0"
	    "b\1_BITONE\0"
	    "f\4\4NIBBLE2\0"
	    "f\x10\4BURST\0"
		"=\04FOUR\0"
		"=\17FIFTEEN\0"
	    "b\x1fMSB\0",
	    0x800f0701,
	    33,
	    "0x800f0701<LSB,NIBBLE2=0>\0"
	    "0x800f0701<BURST=0xf=FIFTEEN,MSB>\0");

	// new style, bits and fields, line break after field description
	h_snprintb_m(
	    "\177\020"
	    "b\0LSB\0"
	    "b\1_BITONE\0"
	    "f\4\4NIBBLE2\0"
	    "f\020\4BURST\0"
		"=\04FOUR\0"
		"=\17FIFTEEN\0"
	    "b\037MSB\0",
	    0x800f0701,
	    32,
	    "0x800f0701<LSB,NIBBLE2=0>\0"
	    "0x800f0701<BURST=0xf=FIFTEEN>\0"
	    "0x800f0701<MSB>\0");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, snprintb);
	ATF_TP_ADD_TC(tp, snprintb_m);

	return atf_no_error();
}
