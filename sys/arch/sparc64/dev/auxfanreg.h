/*	$NetBSD: auxfanreg.h,v 1.1 2026/07/26 12:53:13 jdc Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

/*
 * Register definitions for "SUNW,auxfan1" on SB2500S
 * From observation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auxfanreg.h,v 1.1 2026/07/26 12:53:13 jdc Exp $");

#define AUXFAN_REG_0		0x00	/* RW */
#define AUXFAN_REG_1		0x01	/* RO */
#define AUXFAN_DIMM_FAN_SPD_LO	0x02	/* Fan speed low byte */
#define AUXFAN_DIMM_FAN_SPD_HI	0x03	/* Fan speed high byte */
#define AUXFAN_REG_4		0x04	/* RW */
#define AUXFAN_REG_5		0x05	/* RW */
#define AUXFAN_REG_6		0x06	/* RW */
#define AUXFAN_REG_7		0x07	/* RW */
#define AUXFAN_REG_8		0x08	/* RW */
#define AUXFAN_REG_9		0x09	/* RW */
#define AUXFAN_REG_A		0x0a	/* RW */
#define AUXFAN_REG_B		0x0b	/* RO */
#define AUXFAN_REG_C		0x0c	/* RO */
#define AUXFAN_REG_D		0x0d	/* RO */
