/*	$NetBSD: i2sreg.h,v 1.1 2025/09/08 07:28:03 macallan Exp $	*/

/*-
 * Copyright (c) 2002, 2003 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* I2S registers */
#define I2S_INT		0x00
#define I2S_FORMAT	0x10
#define I2S_FRAMECOUNT	0x40
#define I2S_FRAMEMATCH	0x50
#define I2S_WORDSIZE	0x60

/* I2S_INT register definitions */
#define I2S_INT_CLKSTOPPEND 0x01000000  /* clock-stop interrupt pending */

/* I2S_WORDSIZE register definitions */
#define INPUT_STEREO            (2 << 24)
#define INPUT_MONO              (1 << 24)
#define INPUT_16BIT             (0 << 16)
#define INPUT_24BIT             (3 << 16)
#define OUTPUT_STEREO           (2 << 8)
#define OUTPUT_MONO             (1 << 8)
#define OUTPUT_16BIT            (0 << 0)
#define OUTPUT_24BIT            (3 << 0)
