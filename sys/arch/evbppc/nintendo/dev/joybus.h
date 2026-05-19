/* $NetBSD: joybus.h,v 1.0 2026/05/18 22:54:30 gummybuns Exp $ */

/*-
 * Copyright (c) 2026 Zac Brown <gummybuns@protonmail.com>
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

#ifndef _JOYBUS_H_
#define _JOYBUS_H_

#include <sys/param.h>
#include "si.h"

#define	JB_WIRELESS		__BIT(15)
#define	JB_WIRELESS_RECV	__BIT(14)
#define	JB_RUMBLE		__BIT(13)
#define JB_CONTROLLER		__BIT(11)
#define JB_WIRELESS_TYPE	__BIT(10)
#define	JB_WIRELESS_STATE	__BIT(9)
#define JB_DOLPHIN		__BIT(8)
#define JB_WIRELESS_ORIGIN	__BIT(5)
#define JB_WIRELESS_FIXID	__BIT(4)
#define JB_WIRELESS_NONCTRL	__BIT(3)
#define JB_WIRELESS_LITE	__BIT(2)

#define	JB_IDENTIFY	0x00000000
#define	JB_RESET	0xFF000000
#define	JB_GBA_READ	0x14
#define	JB_GBA_WRITE	0x15

#define JB_NONE		0x0000
#define	JB_N64		0x0500
#define	JB_N64MIC	0x0001
#define	JB_N64KB	0x0002
#define	JB_N64MS	0x0200
#define	JB_GBA		0x0004
#define JB_GC		0x0900
#define JB_WAVEBRD_RECV	0xe960
#define JB_WAVEBRD	JB_WIRELESS & JB_RUMBLE & JB_CONTROLLER
#define JB_GCKB		0x0802
#define JB_GCSTEER	0x0800
#define JB_GBABIOS	0x08	/* GBA BIOS actually sends a 1byte response and
				 * sets SISR to NOREP. The second byte will be
				 * whatever was in SIIOBUF before send */

#define IS_DOLPHIN(n)	ISSET(n, JB_CONTROLLER)
#define IS_N64(n)	!ISSET(n, JB_CONTROLLER)
#define IS_GCPAD(n)	(((n) & (JB_CONTROLLER | JB_DOLPHIN)) == JB_GC) || \
				ISSET(n, JB_WIRELESS)

#define JB_DELAY	50	/* lowest delay with results for multiboot */

static inline uint32_t
jb_reset(struct si_softc *sc, unsigned chan, long us)
{
	struct siio_send data;
	uint32_t out[1];
	uint32_t in[1];
	out[0] = JB_RESET;

	data.chan = chan;
	data.outsize = 1;
	data.insize = 3;
	data.in = in;
	data.out = out;

	delay(us);
	__si_send(sc, &data);
	return in[0];
}

static inline uint32_t
jb_identify(struct si_softc *sc, unsigned chan, long us)
{
	struct siio_send data;
	uint32_t out[1];
	uint32_t in[1];
	out[0] = JB_IDENTIFY;

	data.chan = chan;
	data.outsize = 1;
	data.insize = 3;
	data.in = in;
	data.out = out;

	delay(us);
	__si_send(sc, &data);
	return in[0];
}

#endif
