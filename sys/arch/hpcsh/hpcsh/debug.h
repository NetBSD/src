/*	$NetBSD: debug.h,v 1.2 2001/06/28 18:59:06 uch Exp $	*/

/*-
 * Copyright (c) 1999-2001 The NetBSD Foundation, Inc.
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

#ifdef DEBUG
//#define INTERRUPT_MONITOR

#define __bitdisp(a, s, e, m, c)					\
({									\
	u_int32_t __j, __j1;						\
	int __i, __s, __e, __n;						\
	__n = sizeof(typeof(a)) * NBBY - 1;				\
	__j1 = 1 << __n;						\
	__e = e ? e : __n;						\
	__s = s;							\
	for (__j = __j1, __i = __n; __j > 0; __j >>=1, __i--) {		\
		if (__i > __e || __i < __s) {				\
			printf("%c", a & __j ? '+' : '-');		\
		} else {						\
			printf("%c", a & __j ? '|' : '.');		\
		}							\
	}								\
	if (m) {							\
		printf("[%s]", (char*)m);				\
	}								\
	if (c) {							\
		for (__j = __j1, __i = __n; __j > 0; __j >>=1, __i--) {	\
			if (!(__i > __e || __i < __s) && (a & __j)) {	\
				printf(" %d", __i);			\
			}						\
		}							\
	}								\
	printf(" [0x%08x] %d", a, a);					\
	printf("\n");							\
})
#define bitdisp(a) __bitdisp((a), 0, 0, 0, 1)

__BEGIN_DECLS
void	dbg_bit_print(u_int32_t, u_int32_t, const char *);
void	dbg_banner_start(const char *, size_t);
void	dbg_banner_end(void);

#ifdef INTERRUPT_MONITOR
enum heart_beat {
	HEART_BEAT_CYAN = 0,
	HEART_BEAT_MAGENTA,
	HEART_BEAT_BLUE,
	HEART_BEAT_YELLOW,
	HEART_BEAT_GREEN,
	HEART_BEAT_RED,
	HEART_BEAT_WHITE,
	HEART_BEAT_BLACK
};
void	__dbg_heart_beat(enum heart_beat);
#else
#define __dbg_heart_beat(x)	((void)0)
#endif /* INTERRUPT_MONITOR */
__END_DECLS

#else /* DEBUG */

#define bitdisp(...)		((void)0)
#define dbg_bit_print(...)	((void)0)
#define dbg_banner_start(...)	((void)0)
#define dbg_banner_end(...)	((void)0)
#define __dbg_heart_beat(...)	((void)0)
#endif /* DEBUG */
