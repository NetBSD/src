/*	$NetBSD: debug.h,v 1.4 2002/05/03 07:31:24 takemura Exp $	*/

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
#ifdef __DPRINTF_EXT
/*
 * debug printf with Function name 
 */
#define	PRINTF(fmt, args...)	printf("%s: " fmt, __FUNCTION__ , ##args) 
#ifdef DPRINTF_ENABLE
#ifndef DPRINTF_DEBUG
#error "specify unique debug symbol"
#endif
#ifndef DPRINTF_LEVEL
#define DPRINTF_LEVEL	1
#endif
int	DPRINTF_DEBUG = DPRINTF_LEVEL;
#define	DPRINTF(fmt, args...)	if (DPRINTF_DEBUG) PRINTF(fmt, ##args)
#define	_DPRINTF(fmt, args...)	if (DPRINTF_DEBUG) printf(fmt, ##args)
#define DPRINTFN(n, fmt, args...)					\
			   	if (DPRINTF_DEBUG > (n)) PRINTF(fmt, ##args)
#else /* DPRINTF_ENABLE */
#define	DPRINTF(args...)	((void)0)
#define	_DPRINTF(args...)	((void)0)
#define DPRINTFN(n, args...)	((void)0)
#endif /* DPRINTF_ENABLE */

#else	/* __DPRINTF_EXT */
/*
 * normal debug printf
 */
#ifdef DPRINTF_ENABLE
#ifndef DPRINTF_DEBUG
#error "specify unique debug symbol"
#endif
#ifndef DPRINTF_LEVEL
#define DPRINTF_LEVEL	1
#endif
int	DPRINTF_DEBUG = DPRINTF_LEVEL;
#define	DPRINTF(arg)		if (DPRINTF_DEBUG) printf arg
#define DPRINTFN(n, arg)	if (DPRINTF_DEBUG > (n)) printf arg
#else /* DPRINTF_ENABLE */
#define	DPRINTF(arg)		((void)0)
#define DPRINTFN(n, arg)	((void)0)
#endif /* DPRINTF_ENABLE */

#endif /* __DPRINT_EXT */
#endif /* USE_HPC_DPRINTF */

/*
 * debug print utility
 */
#define DBG_BIT_PRINT_COUNT	(1 << 0)
#define DBG_BIT_PRINT_QUIET	(1 << 1)
#define dbg_bit_print(a)						\
	__dbg_bit_print((a), sizeof(typeof(a)), 0, 0, 0, DBG_BIT_PRINT_COUNT)
#define dbg_bit_print_msg(a, m)						\
	__dbg_bit_print((a), sizeof(typeof(a)), 0, 0, (m), DBG_BIT_PRINT_COUNT)
#define dbg_bit_display(a)						\
	__dbg_bit_print((a), sizeof(typeof(a)), 0, 0, 0, DBG_BIT_PRINT_QUIET)
void __dbg_bit_print(u_int32_t, int, int, int, char *, int);
void dbg_bitmask_print(u_int32_t, u_int32_t, const char *);
void dbg_draw_line(int);
void dbg_banner_title(const char *, size_t);
void dbg_banner_line(void);
#define dbg_banner_function()						\
{									\
	const char funcname[] = __FUNCTION__;				\
	dbg_banner_title(funcname, sizeof funcname);			\
}

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
