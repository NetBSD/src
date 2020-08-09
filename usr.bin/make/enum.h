/*	$NetBSD: enum.h,v 1.3 2020/08/09 02:53:21 christos Exp $	*/

/*
 Copyright (c) 2020 Roland Illig <rillig@NetBSD.org>
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MAKE_ENUM_H
#define MAKE_ENUM_H

/*
 * Generate string representation for bitmasks.
 */

typedef struct {
	int es_value;
	const char *es_name;
} EnumToStringSpec;

const char *Enum_ToString(char *, size_t, int, const EnumToStringSpec *);

#define ENUM__SEP "|"

#define ENUM__JOIN_1(v1) \
	#v1
#define ENUM__JOIN_2(v1, v2) \
	#v1 ENUM__SEP ENUM__JOIN_1(v2)
#define ENUM__JOIN_3(v1, v2, v3) \
	#v1 ENUM__SEP ENUM__JOIN_2(v2, v3)
#define ENUM__JOIN_4(v1, v2, v3, v4) \
	#v1 ENUM__SEP ENUM__JOIN_3(v2, v3, v4)
#define ENUM__JOIN_5(v1, v2, v3, v4, v5) \
	#v1 ENUM__SEP ENUM__JOIN_4(v2, v3, v4, v5)
#define ENUM__JOIN_6(v1, v2, v3, v4, v5, v6) \
	#v1 ENUM__SEP ENUM__JOIN_5(v2, v3, v4, v5, v6)
#define ENUM__JOIN_7(v1, v2, v3, v4, v5, v6, v7) \
	#v1 ENUM__SEP ENUM__JOIN_6(v2, v3, v4, v5, v6, v7)

#define ENUM__RTTI(typnam, specs, joined) \
	static const EnumToStringSpec typnam ## _ ## ToStringSpecs[] = specs; 
#if 0
	static const size_t typnam ## _ ## ToStringSize = sizeof joined
#endif

#define ENUM__SPEC(v) { v, #v }

#define ENUM__SPEC_3(v1, v2, v3) { \
	ENUM__SPEC(v1), \
	ENUM__SPEC(v2), \
	ENUM__SPEC(v3), \
	{ 0, "" } }

#define ENUM__SPEC_7(v1, v2, v3, v4, v5, v6, v7) { \
	ENUM__SPEC(v1), \
	ENUM__SPEC(v2), \
	ENUM__SPEC(v3), \
	ENUM__SPEC(v4), \
	ENUM__SPEC(v5), \
	ENUM__SPEC(v6), \
	ENUM__SPEC(v7), \
	{ 0, "" } }

#define ENUM_RTTI_3(typnam, v1, v2, v3) \
	ENUM__RTTI(typnam, \
		  ENUM__SPEC_3(v1, v2, v3), \
		  ENUM__JOIN_3(v1, v2, v3))

#define ENUM_RTTI_7(typnam, v1, v2, v3, v4, v5, v6, v7) \
	ENUM__RTTI(typnam, \
		  ENUM__SPEC_7(v1, v2, v3, v4, v5, v6, v7), \
		  ENUM__JOIN_7(v1, v2, v3, v4, v5, v6, v7))

#endif
