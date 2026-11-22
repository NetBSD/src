/* $NetBSD: sun60i_a733_rtc_ccu.h,v 1.1 2026/11/22 14:29:22 jmcneill Exp $ */

/*-
 * Copyright (c) 2026 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SUN60I_A733_RTC_CCU_H
#define _SUN60I_A733_RTC_CCU_H

#define	A733_CLK_IOSC			0
#define	A733_CLK_EXT32K_GATE		1
#define	A733_CLK_IOSC_DIV32K		2
#define	A733_CLK_OSC32K			3
#define	A733_CLK_DCXO24M_DIV32K		4
#define	A733_CLK_RTC32K			5
#define	A733_CLK_RTC_1K			6
#define	A733_CLK_RTC_32K_FANOUT		7
#define	A733_CLK_RTC_DCXO_WAKEUP	8
#define	A733_CLK_RTC_DCXO_SERDES1	9
#define	A733_CLK_RTC_DCXO_SERDES0	10
#define	A733_CLK_RTC_SPI		11
#define	A733_CLK_DCXO			12

#endif /* !_SUN60I_A733_RTC_CCU_H */
