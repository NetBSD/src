/* $NetBSD: testLoops.h,v 1.2 2001/03/13 07:49:34 ross Exp $ */

/* This is a derivative work. */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ross Harvey.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
===============================================================================

This C header file is part of TestFloat, Release 2a, a package of programs
for testing the correctness of floating-point arithmetic complying to the
IEC/IEEE Standard for Floating-Point.

Written by John R. Hauser.  More information is available through the Web
page `http://HTTP.CS.Berkeley.EDU/~jhauser/arithmetic/TestFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort
has been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT
TIMES RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO
PERSONS AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ANY
AND ALL LOSSES, COSTS, OR OTHER PROBLEMS ARISING FROM ITS USE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) they include prominent notice that the work is derivative, and (2) they
include prominent notice akin to these four paragraphs for those parts of
this code that are retained.

===============================================================================
*/

#include <stdio.h>

extern volatile flag stop;

extern const char *trueName, *testName;
extern flag forever, errorStop;
extern uint32 maxErrorCount;
extern flag checkNaNs;
extern int8 *trueFlagsPtr;
extern int8 ( *testFlagsFunctionPtr )( void );
extern const char *functionName;
extern const char *roundingPrecisionName, *roundingModeName, *tininessModeName;
extern flag anyErrors;

void writeFunctionName( FILE * );
void exitWithStatus( void );

void test_a_int32_z_float32( float32 ( int32 ), float32 ( int32 ) );
void test_a_int32_z_float64( float64 ( int32 ), float64 ( int32 ) );
#ifdef FLOATX80
void test_a_int32_z_floatx80( floatx80 ( int32 ), floatx80 ( int32 ) );
#endif
#ifdef FLOAT128
void test_a_int32_z_float128( float128 ( int32 ), float128 ( int32 ) );
#endif
#ifdef BITS64
void test_a_int64_z_float32( float32 ( int64 ), float32 ( int64 ) );
void test_a_int64_z_float64( float64 ( int64 ), float64 ( int64 ) );
#ifdef FLOATX80
void test_a_int64_z_floatx80( floatx80 ( int64 ), floatx80 ( int64 ) );
#endif
#ifdef FLOAT128
void test_a_int64_z_float128( float128 ( int64 ), float128 ( int64 ) );
#endif
#endif

void test_a_float32_z_int32( int32 ( float32 ), int32 ( float32 ) );
#ifdef BITS64
void test_a_float32_z_int64( int64 ( float32 ), int64 ( float32 ) );
#endif
void test_a_float32_z_float64( float64 ( float32 ), float64 ( float32 ) );
#ifdef FLOATX80
void test_a_float32_z_floatx80( floatx80 ( float32 ), floatx80 ( float32 ) );
#endif
#ifdef FLOAT128
void test_a_float32_z_float128( float128 ( float32 ), float128 ( float32 ) );
#endif
void test_az_float32( float32 ( float32 ), float32 ( float32 ) );
void
 test_ab_float32_z_flag(
     flag ( float32, float32 ), flag ( float32, float32 ) );
void
 test_abz_float32(
     float32 ( float32, float32 ), float32 ( float32, float32 ) );

void test_a_float64_z_int32( int32 ( float64 ), int32 ( float64 ) );
#ifdef BITS64
void test_a_float64_z_int64( int64 ( float64 ), int64 ( float64 ) );
#endif
void test_a_float64_z_float32( float32 ( float64 ), float32 ( float64 ) );
#ifdef FLOATX80
void test_a_float64_z_floatx80( floatx80 ( float64 ), floatx80 ( float64 ) );
#endif
#ifdef FLOAT128
void test_a_float64_z_float128( float128 ( float64 ), float128 ( float64 ) );
#endif
void test_az_float64( float64 ( float64 ), float64 ( float64 ) );
void
 test_ab_float64_z_flag(
     flag ( float64, float64 ), flag ( float64, float64 ) );
void
 test_abz_float64(
     float64 ( float64, float64 ), float64 ( float64, float64 ) );

#ifdef FLOATX80

void test_a_floatx80_z_int32( int32 ( floatx80 ), int32 ( floatx80 ) );
#ifdef BITS64
void test_a_floatx80_z_int64( int64 ( floatx80 ), int64 ( floatx80 ) );
#endif
void test_a_floatx80_z_float32( float32 ( floatx80 ), float32 ( floatx80 ) );
void test_a_floatx80_z_float64( float64 ( floatx80 ), float64 ( floatx80 ) );
#ifdef FLOAT128
void
 test_a_floatx80_z_float128( float128 ( floatx80 ), float128 ( floatx80 ) );
#endif
void test_az_floatx80( floatx80 ( floatx80 ), floatx80 ( floatx80 ) );
void
 test_ab_floatx80_z_flag(
     flag ( floatx80, floatx80 ), flag ( floatx80, floatx80 ) );
void
 test_abz_floatx80(
     floatx80 ( floatx80, floatx80 ), floatx80 ( floatx80, floatx80 ) );

#endif

#ifdef FLOAT128

void test_a_float128_z_int32( int32 ( float128 ), int32 ( float128 ) );
#ifdef BITS64
void test_a_float128_z_int64( int64 ( float128 ), int64 ( float128 ) );
#endif
void test_a_float128_z_float32( float32 ( float128 ), float32 ( float128 ) );
void test_a_float128_z_float64( float64 ( float128 ), float64 ( float128 ) );
#ifdef FLOATX80
void
 test_a_float128_z_floatx80( floatx80 ( float128 ), floatx80 ( float128 ) );
#endif
void test_az_float128( float128 ( float128 ), float128 ( float128 ) );
void
 test_ab_float128_z_flag(
     flag ( float128, float128 ), flag ( float128, float128 ) );
void
 test_abz_float128(
     float128 ( float128, float128 ), float128 ( float128, float128 ) );

#endif

