/*	$NetBSD: int_fmtio.h,v 1.1 2001/04/15 17:13:18 kleink Exp $	*/

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

#ifndef _VAX_INT_FMTIO_H_
#define _VAX_INT_FMTIO_H_

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
#define	PRIdMAX		"lld"	/* intmax_t		*/
#ifdef __ELF__
#define	PRIdPTR		"ld"	/* intptr_t		*/
#else
#define	PRIdPTR		"d"	/* intptr_t		*/
#endif

#define	PRIi8		"i"	/* int8_t		*/
#define	PRIi16		"i"	/* int16_t		*/
#define	PRIi32		"i"	/* int32_t		*/
#define	PRIi64		"lli"	/* int64_t		*/
#define	PRIiLEAST8	"i"	/* int_least8_t		*/
#define	PRIiLEAST16	"i"	/* int_least16_t	*/
#define	PRIiLEAST32	"i"	/* int_least32_t	*/
#define	PRIiLEAST64	"lli"	/* int_least64_t	*/
#define	PRIiMAX		"lli"	/* intmax_t		*/
#ifdef __ELF__
#define	PRIiPTR		"li"	/* intptr_t		*/
#else
#define	PRIiPTR		"i"	/* intptr_t		*/
#endif

/* fprintf macros for unsigned integers */

#define	PRIo8		"o"	/* uint8_t		*/
#define	PRIo16		"o"	/* uint16_t		*/
#define	PRIo32		"o"	/* uint32_t		*/
#define	PRIo64		"llo"	/* uint64_t		*/
#define	PRIoLEAST8	"o"	/* uint_least8_t	*/
#define	PRIoLEAST16	"o"	/* uint_least16_t	*/
#define	PRIoLEAST32	"o"	/* uint_least32_t	*/
#define	PRIoLEAST64	"llo"	/* uint_least64_t	*/
#define	PRIoMAX		"llo"	/* uintmax_t		*/
#ifdef __ELF__
#define	PRIoPTR		"lo"	/* uintptr_t		*/
#else
#define	PRIoPTR		"o"	/* uintptr_t		*/
#endif

#define	PRIu8		"u"	/* uint8_t		*/
#define	PRIu16		"u"	/* uint16_t		*/
#define	PRIu32		"u"	/* uint32_t		*/
#define	PRIu64		"llu"	/* uint64_t		*/
#define	PRIuLEAST8	"u"	/* uint_least8_t	*/
#define	PRIuLEAST16	"u"	/* uint_least16_t	*/
#define	PRIuLEAST32	"u"	/* uint_least32_t	*/
#define	PRIuLEAST64	"llu"	/* uint_least64_t	*/
#define	PRIuMAX		"llu"	/* uintmax_t		*/
#ifdef __ELF__
#define	PRIuPTR		"lu"	/* uintptr_t		*/
#else
#define	PRIuPTR		"u"	/* uintptr_t		*/
#endif

#define	PRIx8		"x"	/* uint8_t		*/
#define	PRIx16		"x"	/* uint16_t		*/
#define	PRIx32		"x"	/* uint32_t		*/
#define	PRIx64		"llx"	/* uint64_t		*/
#define	PRIxLEAST8	"x"	/* uint_least8_t	*/
#define	PRIxLEAST16	"x"	/* uint_least16_t	*/
#define	PRIxLEAST32	"x"	/* uint_least32_t	*/
#define	PRIxLEAST64	"llx"	/* uint_least64_t	*/
#define	PRIxMAX		"llx"	/* uintmax_t		*/
#ifdef __ELF__
#define	PRIxPTR		"lx"	/* uintptr_t		*/
#else
#define	PRIxPTR		"x"	/* uintptr_t		*/
#endif

#define	PRIX8		"X"	/* uint8_t		*/
#define	PRIX16		"X"	/* uint16_t		*/
#define	PRIX32		"X"	/* uint32_t		*/
#define	PRIX64		"llX"	/* uint64_t		*/
#define	PRIXLEAST8	"X"	/* uint_least8_t	*/
#define	PRIXLEAST16	"X"	/* uint_least16_t	*/
#define	PRIXLEAST32	"X"	/* uint_least32_t	*/
#define	PRIXLEAST64	"llX"	/* uint_least64_t	*/
#define	PRIXMAX		"llX"	/* uintmax_t		*/
#ifdef __ELF__
#define	PRIXPTR		"lX"	/* uintptr_t		*/
#else
#define	PRIXPTR		"X"	/* uintptr_t		*/
#endif

/* fscanf macros for signed integers */

#define	SCNd8		"hhd"	/* int8_t		*/
#define	SCNd16		"hd"	/* int16_t		*/
#define	SCNd32		"d"	/* int32_t		*/
#define	SCNd64		"lld"	/* int64_t		*/
#define	SCNdLEAST8	"hhd"	/* int_least8_t		*/
#define	SCNdLEAST16	"hd"	/* int_least16_t	*/
#define	SCNdLEAST32	"d"	/* int_least32_t	*/
#define	SCNdLEAST64	"lld"	/* int_least64_t	*/
#define	SCNdMAX		"lld"	/* intmax_t		*/
#ifdef __ELF__
#define	SCNdPTR		"ld"	/* intptr_t		*/
#else
#define	SCNdPTR		"d"	/* intptr_t		*/
#endif

#define	SCNi8		"hhi"	/* int8_t		*/
#define	SCNi16		"hi"	/* int16_t		*/
#define	SCNi32		"i"	/* int32_t		*/
#define	SCNi64		"lli"	/* int64_t		*/
#define	SCNiLEAST8	"hhi"	/* int_least8_t		*/
#define	SCNiLEAST16	"hi"	/* int_least16_t	*/
#define	SCNiLEAST32	"i"	/* int_least32_t	*/
#define	SCNiLEAST64	"lli"	/* int_least64_t	*/
#define	SCNiMAX		"lli"	/* intmax_t		*/
#ifdef __ELF__
#define	SCNiPTR		"li"	/* intptr_t		*/
#else
#define	SCNiPTR		"i"	/* intptr_t		*/
#endif

/* fscanf macros for unsigned integers */

#define	SCNo8		"hho"	/* uint8_t		*/
#define	SCNo16		"ho"	/* uint16_t		*/
#define	SCNo32		"o"	/* uint32_t		*/
#define	SCNo64		"llo"	/* uint64_t		*/
#define	SCNoLEAST8	"hho"	/* uint_least8_t	*/
#define	SCNoLEAST16	"ho"	/* uint_least16_t	*/
#define	SCNoLEAST32	"o"	/* uint_least32_t	*/
#define	SCNoLEAST64	"llo"	/* uint_least64_t	*/
#define	SCNoMAX		"llo"	/* uintmax_t		*/
#ifdef __ELF__
#define	SCNoPTR		"lo"	/* uintptr_t		*/
#else
#define	SCNoPTR		"o"	/* uintptr_t		*/
#endif

#define	SCNu8		"hhu"	/* uint8_t		*/
#define	SCNu16		"hu"	/* uint16_t		*/
#define	SCNu32		"u"	/* uint32_t		*/
#define	SCNu64		"llu"	/* uint64_t		*/
#define	SCNuLEAST8	"hhu"	/* uint_least8_t	*/
#define	SCNuLEAST16	"hu"	/* uint_least16_t	*/
#define	SCNuLEAST32	"u"	/* uint_least32_t	*/
#define	SCNuLEAST64	"llu"	/* uint_least64_t	*/
#define	SCNuMAX		"llu"	/* uintmax_t		*/
#ifdef __ELF__
#define	SCNuPTR		"lu"	/* uintptr_t		*/
#else
#define	SCNuPTR		"u"	/* uintptr_t		*/
#endif

#define	SCNx8		"hhx"	/* uint8_t		*/
#define	SCNx16		"hx"	/* uint16_t		*/
#define	SCNx32		"x"	/* uint32_t		*/
#define	SCNx64		"llx"	/* uint64_t		*/
#define	SCNxLEAST8	"hhx"	/* uint_least8_t	*/
#define	SCNxLEAST16	"hx"	/* uint_least16_t	*/
#define	SCNxLEAST32	"x"	/* uint_least32_t	*/
#define	SCNxLEAST64	"llx"	/* uint_least64_t	*/
#define	SCNxMAX		"llx"	/* uintmax_t		*/
#ifdef __ELF__
#define	SCNxPTR		"lx"	/* uintptr_t		*/
#else
#define	SCNxPTR		"x"	/* uintptr_t		*/
#endif

#endif /* !_VAX_INT_FMTIO_H_ */
