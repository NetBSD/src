/*	$NetBSD: writeHex.c,v 1.4 2002/02/21 07:38:16 itojun Exp $	*/

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

This C source file is part of TestFloat, Release 2a, a package of programs
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
#include "milieu.h"
#include "softfloat.h"
#include "writeHex.h"

void writeHex_flag( flag a, FILE *stream )
{

    fputc( a ? '1' : '0', stream );

}

static void writeHex_bits8( bits8 a, FILE *stream )
{
    int digit;

    digit = ( a>>4 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    fputc( '0' + digit, stream );
    digit = a & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    fputc( '0' + digit, stream );

}

static void writeHex_bits12( int16 a, FILE *stream )
{
    int digit;

    digit = ( a>>8 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    fputc( '0' + digit, stream );
    digit = ( a>>4 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    fputc( '0' + digit, stream );
    digit = a & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    fputc( '0' + digit, stream );

}

static void writeHex_bits16( bits16 a, FILE *stream )
{
    int digit;

    digit = ( a>>12 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    fputc( '0' + digit, stream );
    digit = ( a>>8 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    fputc( '0' + digit, stream );
    digit = ( a>>4 ) & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    fputc( '0' + digit, stream );
    digit = a & 0xF;
    if ( 9 < digit ) digit += 'A' - ( '0' + 10 );
    fputc( '0' + digit, stream );

}

void writeHex_bits32( bits32 a, FILE *stream )
{

    writeHex_bits16( a>>16, stream );
    writeHex_bits16( a, stream );

}

#ifdef BITS64

void writeHex_bits64( bits64 a, FILE *stream )
{

    writeHex_bits32( a>>32, stream );
    writeHex_bits32( a, stream );

}

#endif

void writeHex_float32( float32 a, FILE *stream )
{

    fputc( ( ( (sbits32) a ) < 0 ) ? '8' : '0', stream );
    writeHex_bits8( a>>23, stream );
    fputc( '.', stream );
    writeHex_bits8( ( a>>16 ) & 0x7F, stream );
    writeHex_bits16( a, stream );

}

#ifdef BITS64

void writeHex_float64( float64 a, FILE *stream )
{

    writeHex_bits12( a>>52, stream );
    fputc( '.', stream );
    writeHex_bits12( a>>40, stream );
    writeHex_bits8( a>>32, stream );
    writeHex_bits32( a, stream );

}

#else

void writeHex_float64( float64 a, FILE *stream )
{

    writeHex_bits12( a.high>>20, stream );
    fputc( '.', stream );
    writeHex_bits12( a.high>>8, stream );
    writeHex_bits8( a.high, stream );
    writeHex_bits32( a.low, stream );

}

#endif

#ifdef FLOATX80

void writeHex_floatx80( floatx80 a, FILE *stream )
{

    writeHex_bits16( a.high, stream );
    fputc( '.', stream );
    writeHex_bits64( a.low, stream );

}

#endif

#ifdef FLOAT128

void writeHex_float128( float128 a, FILE *stream )
{

    writeHex_bits16( a.high>>48, stream );
    fputc( '.', stream );
    writeHex_bits16( a.high>>32, stream );
    writeHex_bits32( a.high, stream );
    writeHex_bits64( a.low, stream );

}

#endif

void writeHex_float_flags( uint8 flags, FILE *stream )
{

    fputc( flags & float_flag_invalid   ? 'v' : '.', stream );
    fputc( flags & float_flag_divbyzero ? 'z' : '.', stream );
    fputc( flags & float_flag_overflow  ? 'o' : '.', stream );
    fputc( flags & float_flag_underflow ? 'u' : '.', stream );
    fputc( flags & float_flag_inexact   ? 'x' : '.', stream );

}

