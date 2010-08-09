/*	$NetBSD: debug.h,v 1.11 2010/08/09 23:07:20 uwe Exp $	*/

/*-
 * Copyright (c) 1999-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/*
 * debug version exports all symbols.
 */
#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

/*
 * printf control
 *	sample:
 * #ifdef FOO_DEBUG
 * #define DPRINTF_ENABLE
 * #define DPRINTF_DEBUG	foo_debug
 * #define DPRINTF_LEVEL	2
 * #endif
 */
#ifdef USE_HPC_DPRINTF

#ifdef DPRINTF_ENABLE

#ifndef DPRINTF_DEBUG
#error "specify unique debug variable"
#endif

#ifndef DPRINTF_LEVEL
#define DPRINTF_LEVEL	1
#endif

int DPRINTF_DEBUG = DPRINTF_LEVEL;
#endif /* DPRINTF_ENABLE */


#ifdef __DPRINTF_EXT
/*
 * printf with function name prepended
 */

#define	PRINTF(fmt, args...)	do {			\
		printf("%s: " fmt, __func__ , ##args);	\
	} while (/* CONSTCOND */0)

#ifdef DPRINTF_ENABLE

#define	DPRINTF(fmt, args...)	do {		\
		if (DPRINTF_DEBUG)		\
			PRINTF(fmt, ##args);	\
	} while (/* CONSTCOND */0)

#define	_DPRINTF(fmt, args...)	do {		\
		if (DPRINTF_DEBUG)		\
			printf(fmt, ##args);	\
	} while (/* CONSTCOND */0)

#define DPRINTFN(n, fmt, args...)	do {	\
		if (DPRINTF_DEBUG > (n))	\
			PRINTF(fmt, ##args);	\
	} while (/* CONSTCOND */0)

#define _DPRINTFN(n, fmt, args...) do {		\
		if (DPRINTF_DEBUG > (n))	\
			printf(fmt, ##args);	\
	} while (/* CONSTCOND */0)

#else  /* !DPRINTF_ENABLE */
#define	DPRINTF(args...)	do {} while (/* CONSTCOND */ 0)
#define	_DPRINTF(args...)	do {} while (/* CONSTCOND */ 0)
#define DPRINTFN(n, args...)	do {} while (/* CONSTCOND */ 0)
#define _DPRINTFN(n, args...)	do {} while (/* CONSTCOND */ 0)
#endif /* !DPRINTF_ENABLE */

#else  /* !__DPRINTF_EXT */
/*
 * normal debug printf
 */

#ifdef DPRINTF_ENABLE

#define	DPRINTF(arg)	do {			\
		if (DPRINTF_DEBUG)		\
			printf arg;		\
	} while (/* CONSTCOND */0)

#define DPRINTFN(n, arg)	do {		\
		if (DPRINTF_DEBUG > (n))	\
			printf arg;		\
	} while (/* CONSTCOND */0)

#else  /* !DPRINTF_ENABLE */
#define	DPRINTF(arg)		do {} while (/* CONSTCOND */ 0)
#define DPRINTFN(n, arg)	do {} while (/* CONSTCOND */ 0)
#endif /* !DPRINTF_ENABLE */

#endif /* !__DPRINT_EXT */
#endif /* USE_HPC_DPRINTF */


/*
 * debug print utility
 */
#define DBG_BIT_PRINT_COUNT	(1 << 0)
#define DBG_BIT_PRINT_QUIET	(1 << 1)

void __dbg_bit_print(uint32_t, int, int, int, const char *, int);

#define dbg_bit_print(a) do {						\
		__dbg_bit_print((a), sizeof(typeof(a)), 0, 0, NULL,	\
			DBG_BIT_PRINT_COUNT);				\
	} while (/* CONSTCOND */0)

#define dbg_bit_print_msg(a, m) do {					\
		__dbg_bit_print((a), sizeof(typeof(a)), 0, 0, (m),	\
			DBG_BIT_PRINT_COUNT);				\
	} while (/* CONSTCOND */0)

#define dbg_bit_display(a) do {						\
		__dbg_bit_print((a), sizeof(typeof(a)), 0, 0, NULL,	\
			DBG_BIT_PRINT_QUIET);				\
	} while (/* CONSTCOND */0)

void dbg_bitmask_print(uint32_t, uint32_t, const char *);
void dbg_draw_line(int);
void dbg_banner_title(const char *, size_t);
void dbg_banner_line(void);

#define dbg_banner_function() do {					\
		dbg_banner_title(__func__, sizeof(__func__) - 1);	\
	} while (/* CONSTCOND */ 0)

/* HPC_DEBUG_LCD */
#define RGB565_BLACK		0x0000
#define RGB565_RED		0xf800
#define RGB565_GREEN		0x07e0
#define RGB565_YELLOW		0xffe0
#define RGB565_BLUE		0x001f
#define RGB565_MAGENTA		0xf81f
#define RGB565_CYAN		0x07ff
#define RGB565_WHITE		0xffff

void dbg_lcd_test(void);
