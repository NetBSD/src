/*	$NetBSD: int_fmtio.h,v 1.6 2014/01/29 01:40:35 matt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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

#ifndef _ARM_INT_FMTIO_H_
#define _ARM_INT_FMTIO_H_

/*
 * 7.8.1 Macros for format specifiers
 */

/* fprintf macros for signed integers */

#define	PRId8		"d"	/* int8_t		*/
#define	PRId16		"d"	/* int16_t		*/
#define	PRId32		"d"	/* int32_t		*/
#define	PRId64		"lld"	/* int64_t		*/
#define	PRIdLEAST8	"d"	/* int_least8_t		*/
#define	PRIdLEAST16	"d"	/* int_least16_t	*/
#define	PRIdLEAST32	"d"	/* int_least32_t	*/
#define	PRIdLEAST64	"lld"	/* int_least64_t	*/
#ifdef _LP64
#define	PRIdFAST8	"ld"	/* int_fast8_t		*/
#define	PRIdFAST16	"ld"	/* int_fast16_t		*/
#define	PRIdFAST32	"ld"	/* int_fast32_t		*/
#else
#define	PRIdFAST8	"d"	/* int_fast8_t		*/
#define	PRIdFAST16	"d"	/* int_fast16_t		*/
#define	PRIdFAST32	"d"	/* int_fast32_t		*/
#endif
#define	PRIdFAST64	"lld"	/* int_fast64_t		*/
#define	PRIdMAX		"lld"	/* intmax_t		*/
#define	PRIdPTR		"ld"	/* intptr_t		*/

#define	PRIi8		"i"	/* int8_t		*/
#define	PRIi16		"i"	/* int16_t		*/
#define	PRIi32		"i"	/* int32_t		*/
#define	PRIi64		"lli"	/* int64_t		*/
#define	PRIiLEAST8	"i"	/* int_least8_t		*/
#define	PRIiLEAST16	"i"	/* int_least16_t	*/
#define	PRIiLEAST32	"i"	/* int_least32_t	*/
#define	PRIiLEAST64	"lli"	/* int_least64_t	*/
#ifdef _LP64
#define	PRIiFAST8	"li"	/* int_fast8_t		*/
#define	PRIiFAST16	"li"	/* int_fast16_t		*/
#define	PRIiFAST32	"li"	/* int_fast32_t		*/
#else
#define	PRIiFAST8	"i"	/* int_fast8_t		*/
#define	PRIiFAST16	"i"	/* int_fast16_t		*/
#define	PRIiFAST32	"i"	/* int_fast32_t		*/
#endif
#define	PRIiFAST64	"lli"	/* int_fast64_t		*/
#define	PRIiMAX		"lli"	/* intmax_t		*/
#define	PRIiPTR		"li"	/* intptr_t		*/

/* fprintf macros for unsigned integers */

#define	PRIo8		"o"	/* uint8_t		*/
#define	PRIo16		"o"	/* uint16_t		*/
#define	PRIo32		"o"	/* uint32_t		*/
#define	PRIo64		"llo"	/* uint64_t		*/
#define	PRIoLEAST8	"o"	/* uint_least8_t	*/
#define	PRIoLEAST16	"o"	/* uint_least16_t	*/
#define	PRIoLEAST32	"o"	/* uint_least32_t	*/
#define	PRIoLEAST64	"llo"	/* uint_least64_t	*/
#ifdef _LP64
#define	PRIoFAST8	"lo"	/* uint_fast8_t		*/
#define	PRIoFAST16	"lo"	/* uint_fast16_t	*/
#define	PRIoFAST32	"lo"	/* uint_fast32_t	*/
#else
#define	PRIoFAST8	"o"	/* uint_fast8_t		*/
#define	PRIoFAST16	"o"	/* uint_fast16_t	*/
#define	PRIoFAST32	"o"	/* uint_fast32_t	*/
#endif
#define	PRIoFAST64	"llo"	/* uint_fast64_t	*/
#define	PRIoMAX		"llo"	/* uintmax_t		*/
#define	PRIoPTR		"lo"	/* uintptr_t		*/

#define	PRIu8		"u"	/* uint8_t		*/
#define	PRIu16		"u"	/* uint16_t		*/
#define	PRIu32		"u"	/* uint32_t		*/
#define	PRIu64		"llu"	/* uint64_t		*/
#define	PRIuLEAST8	"u"	/* uint_least8_t	*/
#define	PRIuLEAST16	"u"	/* uint_least16_t	*/
#define	PRIuLEAST32	"u"	/* uint_least32_t	*/
#define	PRIuLEAST64	"llu"	/* uint_least64_t	*/
#ifdef _LP64
#define	PRIuFAST8	"lu"	/* uint_fast8_t		*/
#define	PRIuFAST16	"lu"	/* uint_fast16_t	*/
#define	PRIuFAST32	"lu"	/* uint_fast32_t	*/
#else
#define	PRIuFAST8	"u"	/* uint_fast8_t		*/
#define	PRIuFAST16	"u"	/* uint_fast16_t	*/
#define	PRIuFAST32	"u"	/* uint_fast32_t	*/
#endif
#define	PRIuFAST64	"llu"	/* uint_fast64_t	*/
#define	PRIuMAX		"llu"	/* uintmax_t		*/
#define	PRIuPTR		"lu"	/* uintptr_t		*/

#define	PRIx8		"x"	/* uint8_t		*/
#define	PRIx16		"x"	/* uint16_t		*/
#define	PRIx32		"x"	/* uint32_t		*/
#define	PRIx64		"llx"	/* uint64_t		*/
#define	PRIxLEAST8	"x"	/* uint_least8_t	*/
#define	PRIxLEAST16	"x"	/* uint_least16_t	*/
#define	PRIxLEAST32	"x"	/* uint_least32_t	*/
#define	PRIxLEAST64	"llx"	/* uint_least64_t	*/
#ifdef _LP64
#define	PRIxFAST8	"lx"	/* uint_fast8_t		*/
#define	PRIxFAST16	"lx"	/* uint_fast16_t	*/
#define	PRIxFAST32	"lx"	/* uint_fast32_t	*/
#else
#define	PRIxFAST8	"x"	/* uint_fast8_t		*/
#define	PRIxFAST16	"x"	/* uint_fast16_t	*/
#define	PRIxFAST32	"x"	/* uint_fast32_t	*/
#endif
#define	PRIxFAST64	"llx"	/* uint_fast64_t	*/
#define	PRIxMAX		"llx"	/* uintmax_t		*/
#define	PRIxPTR		"lx"	/* uintptr_t		*/

#define	PRIX8		"X"	/* uint8_t		*/
#define	PRIX16		"X"	/* uint16_t		*/
#define	PRIX32		"X"	/* uint32_t		*/
#define	PRIX64		"llX"	/* uint64_t		*/
#define	PRIXLEAST8	"X"	/* uint_least8_t	*/
#define	PRIXLEAST16	"X"	/* uint_least16_t	*/
#define	PRIXLEAST32	"X"	/* uint_least32_t	*/
#define	PRIXLEAST64	"llX"	/* uint_least64_t	*/
#ifdef _LP64
#define	PRIXFAST8	"lX"	/* uint_fast8_t		*/
#define	PRIXFAST16	"lX"	/* uint_fast16_t	*/
#define	PRIXFAST32	"lX"	/* uint_fast32_t	*/
#else
#define	PRIXFAST8	"X"	/* uint_fast8_t		*/
#define	PRIXFAST16	"X"	/* uint_fast16_t	*/
#define	PRIXFAST32	"X"	/* uint_fast32_t	*/
#endif
#define	PRIXFAST64	"llX"	/* uint_fast64_t	*/
#define	PRIXMAX		"llX"	/* uintmax_t		*/
#define	PRIXPTR		"lX"	/* uintptr_t		*/

/* fscanf macros for signed integers */

#define	SCNd8		"hhd"	/* int8_t		*/
#define	SCNd16		"hd"	/* int16_t		*/
#define	SCNd32		"d"	/* int32_t		*/
#define	SCNd64		"lld"	/* int64_t		*/
#define	SCNdLEAST8	"hhd"	/* int_least8_t		*/
#define	SCNdLEAST16	"hd"	/* int_least16_t	*/
#define	SCNdLEAST32	"d"	/* int_least32_t	*/
#define	SCNdLEAST64	"lld"	/* int_least64_t	*/
#ifdef _LP64
#define	SCNdFAST8	"ld"	/* int_fast8_t		*/
#define	SCNdFAST16	"ld"	/* int_fast16_t		*/
#define	SCNdFAST32	"ld"	/* int_fast32_t		*/
#else
#define	SCNdFAST8	"d"	/* int_fast8_t		*/
#define	SCNdFAST16	"d"	/* int_fast16_t		*/
#define	SCNdFAST32	"d"	/* int_fast32_t		*/
#endif
#define	SCNdFAST64	"lld"	/* int_fast64_t		*/
#define	SCNdMAX		"lld"	/* intmax_t		*/
#define	SCNdPTR		"ld"	/* intptr_t		*/

#define	SCNi8		"hhi"	/* int8_t		*/
#define	SCNi16		"hi"	/* int16_t		*/
#define	SCNi32		"i"	/* int32_t		*/
#define	SCNi64		"lli"	/* int64_t		*/
#define	SCNiLEAST8	"hhi"	/* int_least8_t		*/
#define	SCNiLEAST16	"hi"	/* int_least16_t	*/
#define	SCNiLEAST32	"i"	/* int_least32_t	*/
#define	SCNiLEAST64	"lli"	/* int_least64_t	*/
#ifdef _LP64
#define	SCNiFAST8	"li"	/* int_fast8_t		*/
#define	SCNiFAST16	"li"	/* int_fast16_t		*/
#define	SCNiFAST32	"li"	/* int_fast32_t		*/
#else
#define	SCNiFAST8	"i"	/* int_fast8_t		*/
#define	SCNiFAST16	"i"	/* int_fast16_t		*/
#define	SCNiFAST32	"i"	/* int_fast32_t		*/
#endif
#define	SCNiFAST64	"lli"	/* int_fast64_t		*/
#define	SCNiMAX		"lli"	/* intmax_t		*/
#define	SCNiPTR		"li"	/* intptr_t		*/

/* fscanf macros for unsigned integers */

#define	SCNo8		"hho"	/* uint8_t		*/
#define	SCNo16		"ho"	/* uint16_t		*/
#define	SCNo32		"o"	/* uint32_t		*/
#define	SCNo64		"llo"	/* uint64_t		*/
#define	SCNoLEAST8	"hho"	/* uint_least8_t	*/
#define	SCNoLEAST16	"ho"	/* uint_least16_t	*/
#define	SCNoLEAST32	"o"	/* uint_least32_t	*/
#define	SCNoLEAST64	"llo"	/* uint_least64_t	*/
#ifdef _LP64
#define	SCNoFAST8	"lo"	/* uint_fast8_t		*/
#define	SCNoFAST16	"lo"	/* uint_fast16_t	*/
#define	SCNoFAST32	"lo"	/* uint_fast32_t	*/
#else
#define	SCNoFAST8	"o"	/* uint_fast8_t		*/
#define	SCNoFAST16	"o"	/* uint_fast16_t	*/
#define	SCNoFAST32	"o"	/* uint_fast32_t	*/
#endif
#define	SCNoFAST64	"llo"	/* uint_fast64_t	*/
#define	SCNoMAX		"llo"	/* uintmax_t		*/
#define	SCNoPTR		"lo"	/* uintptr_t		*/

#define	SCNu8		"hhu"	/* uint8_t		*/
#define	SCNu16		"hu"	/* uint16_t		*/
#define	SCNu32		"u"	/* uint32_t		*/
#define	SCNu64		"llu"	/* uint64_t		*/
#define	SCNuLEAST8	"hhu"	/* uint_least8_t	*/
#define	SCNuLEAST16	"hu"	/* uint_least16_t	*/
#define	SCNuLEAST32	"u"	/* uint_least32_t	*/
#define	SCNuLEAST64	"llu"	/* uint_least64_t	*/
#ifdef _LP64
#define	SCNuFAST8	"lu"	/* uint_fast8_t		*/
#define	SCNuFAST16	"lu"	/* uint_fast16_t	*/
#define	SCNuFAST32	"lu"	/* uint_fast32_t	*/
#else
#define	SCNuFAST8	"u"	/* uint_fast8_t		*/
#define	SCNuFAST16	"u"	/* uint_fast16_t	*/
#define	SCNuFAST32	"u"	/* uint_fast32_t	*/
#endif
#define	SCNuFAST64	"llu"	/* uint_fast64_t	*/
#define	SCNuMAX		"llu"	/* uintmax_t		*/
#define	SCNuPTR		"lu"	/* uintptr_t		*/

#define	SCNx8		"hhx"	/* uint8_t		*/
#define	SCNx16		"hx"	/* uint16_t		*/
#define	SCNx32		"x"	/* uint32_t		*/
#define	SCNx64		"llx"	/* uint64_t		*/
#define	SCNxLEAST8	"hhx"	/* uint_least8_t	*/
#define	SCNxLEAST16	"hx"	/* uint_least16_t	*/
#define	SCNxLEAST32	"x"	/* uint_least32_t	*/
#define	SCNxLEAST64	"llx"	/* uint_least64_t	*/
#ifdef _LP64
#define	SCNxFAST8	"lx"	/* uint_fast8_t		*/
#define	SCNxFAST16	"lx"	/* uint_fast16_t	*/
#define	SCNxFAST32	"lx"	/* uint_fast32_t	*/
#else
#define	SCNxFAST8	"x"	/* uint_fast8_t		*/
#define	SCNxFAST16	"x"	/* uint_fast16_t	*/
#define	SCNxFAST32	"x"	/* uint_fast32_t	*/
#endif
#define	SCNxFAST64	"llx"	/* uint_fast64_t	*/
#define	SCNxMAX		"llx"	/* uintmax_t		*/
#define	SCNxPTR		"lx"	/* uintptr_t		*/

#endif /* !_ARM_INT_FMTIO_H_ */
