/*	$NetBSD: teliosioreg.h,v 1.3 2008/05/31 08:08:54 nakayama Exp $	*/

/*
 * Copyright (c) 2005 Takeshi Nakayama.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Sharp Telios machine dependent I/O definitions
 */

/* (KUCS1) */
#define TELIOSIO_REGBASE		0x01000000
#define TELIOSIO_REGSIZE		0x00000010

#define TELIOSIO_BACKLIGHT_VR		0x00
#define		TELIOSIO_BACKLIGHT_VR_VR0	0x0040
#define TELIOSIO_BACKLIGHT_PERIOD	0x04
#define		TELIOSIO_BACKLIGHT_PERIOD_EN	0x0100
#define TELIOSIO_BACKLIGHT_RESET	0x08
#define		TELIOSIO_BACKLIGHT_RESET_MASK	0x01ff
#define TELIOSIO_BACKLIGHT_DELAY	0x0c
#define		TELIOSIO_BACKLIGHT_DELAY_IO0	0x0100
#define		TELIOSIO_BACKLIGHT_DELAY_IO1	0x0200
#define		TELIOSIO_BACKLIGHT_DELAY_IO2	0x0400

/* (IO) */
#define TELIOSIO_AC_STATE	(1 << (6 + TX392X_IODATAINOUT_DIN_SHIFT))
#define TELIOSIO_BMU_CLOCK	(1 << (2 + TX392X_IODATAINOUT_DOUT_SHIFT))
#define TELIOSIO_BMU_DATAOUT	(1 << (1 + TX392X_IODATAINOUT_DOUT_SHIFT))
#define TELIOSIO_BMU_DATAIN	(1 << (0 + TX392X_IODATAINOUT_DIN_SHIFT))

#define TELIOSIO_BMU_CLOCK_FREQ	3072
