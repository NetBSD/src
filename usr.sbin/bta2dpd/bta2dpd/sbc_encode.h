/* $NetBSD: sbc_encode.h,v 1.1 2017/01/28 16:55:54 nat Exp $ */

/*-
 * Copyright (c) 2015 - 2016 Nathanial Sloss <nathanialsloss@yahoo.com.au>
 * All rights reserved.
 *
 *		This software is dedicated to the memory of -
 *	   Baron James Anlezark (Barry) - 1 Jan 1949 - 13 May 2012.
 *
 *		Barry was a man who loved his music.
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

#define SRC_SEP 1
#define SNK_SEP 2

#define SBC_CODEC_ID	0x0
#define mediaTypeAudio	0x0

#define CONFIG_ANY_FREQ	0xf7
#define CONFIG_ANY_ALLOC 0xff
#define MIN_BITPOOL	2
#define DEFAULT_MAXBPOOL 253

#define MODE_STEREO	(1 << 1)
#define MODE_JOINT	(1 << 0)
#define MODE_DUAL	(1 << 2)
#define MODE_MONO	(1 << 3)
#define MODE_ANY	0xf

#define ALLOC_LOUDNESS	(1 << 0)
#define ALLOC_SNR 	(1 << 1)
#define ALLOC_ANY 	3

#define FREQ_16K	(1 << 3)
#define FREQ_32K	(1 << 2)
#define FREQ_44_1K	(1 << 1)
#define FREQ_48K	(1 << 0)
#define FREQ_ANY	0xf


#define BANDS_4		(1 << 1)
#define BANDS_8		(1 << 0)
#define BANDS_ANY	3

#define BLOCKS_4	(1 << 3)
#define BLOCKS_8	(1 << 2)
#define BLOCKS_12	(1 << 1)
#define BLOCKS_16	(1 << 0)
#define BLOCKS_ANY	0xf

u_int FLS(uint8_t);
ssize_t stream(int, int, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
    size_t, int);
ssize_t recvstream(int, int);
