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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: joybus.h,v 1.0 2026/05/18 22:54:30 gummybuns Exp $");

#include <sys/param.h>

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

#define SI_NONE		0x0000
#define	SI_N64		0x0500
#define	SI_N64MIC	0x0001
#define SI_N64KB	0x0002
#define	SI_N64MS	0x0200
#define	SI_GBA		0x0004
#define SI_GBABIOS	0x0800
#define SI_GC		0x0900
#define SI_WAVEBRD_RECV	0xe960
#define SI_WAVEBRD	JB_WIRELESS & JB_RUMBLE & JB_CONTROLLER
#define SI_GCKB		0x0802
#define SI_GCSTEER	0x0800 /* risk: steering wheel + gbabios identical. */

#define IS_DOLPHIN(n)	ISSET(n, JB_CONTROLLER)
#define IS_N64(n)	!ISSET(n, JB_CONTROLLER)
#define IS_GCPAD(n)	(((n) & (JB_CONTROLLER | JB_DOLPHIN)) == SI_GC) || ISSET(n, JB_WIRELESS)
#define IS_GBA(n)	(n == SI_GBA || n == SI_GBABIOS)
