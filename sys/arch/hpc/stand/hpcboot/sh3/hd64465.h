/*	$NetBSD: hd64465.h,v 1.1 2002/02/04 17:38:27 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#include "../../../../hpcsh/dev/hd64465/hd64465uartreg.h"

#define	LSR_TXRDY	0x20	/* Transmitter buffer empty */

#define HD64465COM_TX_BUSY()						\
	while ((VOLATILE_REF8(HD64465_ULSR_REG8) & LSR_TXRDY) == 0)

#define HD64465COM_PUTC(c)						\
__BEGIN_MACRO								\
	HD64465COM_TX_BUSY();						\
	VOLATILE_REF8(HD64465_UTBR_REG8) = (c);				\
	HD64465COM_TX_BUSY();						\
__END_MACRO

#define HD64465COM_PRINT(s)						\
__BEGIN_MACRO								\
	char *__s =(char *)(s);						\
	int __i;							\
	for (__i = 0; __s[__i] != '\0'; __i++) {			\
		char __c = __s[__i];					\
		if (__c == '\n')					\
			HD64465COM_PUTC('\r');				\
		HD64465COM_PUTC(__c);					\
	}								\
__END_MACRO
