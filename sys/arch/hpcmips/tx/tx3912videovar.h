/*	$NetBSD: tx3912videovar.h,v 1.6 2001/06/14 11:09:55 uch Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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

int	tx3912video_init(paddr_t, paddr_t *);

/*
 * 8bpp CLUT
 */
#define TX3912VIDEO_RGB24TOINDEX(rgb)					\
	((((((rgb) >> 16) & 0xff) >> 5) << 5) |				\
	 (((((rgb) >> 8) & 0xff) >> 5) << 2) |				\
	 ((((rgb) & 0xff) >> 6)))

/* system color */
#define TX3912VIDEO_BLACK		TX3912VIDEO_RGB24TOINDEX(0x000000)
#define TX3912VIDEO_RED			TX3912VIDEO_RGB24TOINDEX(0xff0000)
#define TX3912VIDEO_GREEN		TX3912VIDEO_RGB24TOINDEX(0x00ff00)
#define TX3912VIDEO_YELLOW		TX3912VIDEO_RGB24TOINDEX(0xffff00)
#define TX3912VIDEO_BLUE		TX3912VIDEO_RGB24TOINDEX(0x0000ff)
#define TX3912VIDEO_MAGENTA		TX3912VIDEO_RGB24TOINDEX(0xff00ff)
#define TX3912VIDEO_CYAN		TX3912VIDEO_RGB24TOINDEX(0x00ffff)
#define TX3912VIDEO_WHITE		TX3912VIDEO_RGB24TOINDEX(0xffffff)
#define TX3912VIDEO_DARK_BLACK		TX3912VIDEO_RGB24TOINDEX(0x000000)
#define TX3912VIDEO_DARK_RED		TX3912VIDEO_RGB24TOINDEX(0x800000)
#define TX3912VIDEO_DARK_GREEN		TX3912VIDEO_RGB24TOINDEX(0x008000)
#define TX3912VIDEO_DARK_YELLOW		TX3912VIDEO_RGB24TOINDEX(0x808000)
#define TX3912VIDEO_DARK_BLUE		TX3912VIDEO_RGB24TOINDEX(0x000080)
#define TX3912VIDEO_DARK_MAGENTA	TX3912VIDEO_RGB24TOINDEX(0x800080)
#define TX3912VIDEO_DARK_CYAN		TX3912VIDEO_RGB24TOINDEX(0x008080)
#define TX3912VIDEO_DARK_WHITE		TX3912VIDEO_RGB24TOINDEX(0x808080)

/* 
 * Y =  0.2990 * R + 0.5870 * G + 0.1140 * B
 * U = -0.1690 * R - 0.3316 * G + 0.5000 * B
 * V =  0.5000 * R - 0.4186 * G - 0.0813 * B
 */

/*
 * debug functions.
 */
void	tx3912video_calibration_pattern(void);
void	tx3912video_line(int, int, int, int);
void	tx3912video_dot(int, int);
