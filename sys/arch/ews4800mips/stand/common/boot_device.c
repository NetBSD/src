/*	$NetBSD: boot_device.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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

#include <sys/types.h>
#include <machine/sbd.h>
#include "common.h"

/*
 * Get boot device and unit number. IPL sets it.
 */
void __nvsram_type_a(int *, int *);
void __nvsram_type_b(int *, int *);
void __nvsram_type_c(int *, int *);
void __nvsram_type_d(int *, int *);
void __nvsram_type_e(int *, int *);

void (*tab[])(int *, int *) = {
	__nvsram_type_c, /* ER */
	__nvsram_type_a, /* TR */
	__nvsram_type_a, /* SR */
	__nvsram_type_b, /* TR+2D */
	__nvsram_type_b, /* TR+3D */
	__nvsram_type_c, /* LR */
	__nvsram_type_c, /* ER2 */
	__nvsram_type_d, /* TR2 */
	__nvsram_type_c, /* ER1V */
	__nvsram_type_e, /* LRC */
	__nvsram_type_c, /* SR2 */
	__nvsram_type_d, /* ER+ */
	__nvsram_type_c, /* TR2+ */
	__nvsram_type_d, /* ER2A */
};

void
boot_device(int *type, int *unit, int *fd_format)
{
	int i;

	*fd_format = SBD_INFO->fdd == 1 ? FD_FORMAT_2HD : FD_FORMAT_2D;
	i  = SBD_INFO->machine - 0x1010;
	if (i < sizeof tab / sizeof tab[0])
		return tab[i](type, unit);

	return __nvsram_type_e(type, unit);
}

void
__nvsram_type_a(int *type, int *unit)
{
	/* TR, SR */
	*type = NVSRAM_BOOTDEV_HARDDISK;
	*unit = 0;
}

void
__nvsram_type_b(int *type, int *unit)
{
	/* TR+2D, TR+3D */
	*type = NVSRAM_BOOTDEV_HARDDISK;
	*unit = *(uint8_t *)0xba021808;
}

void
__nvsram_type_c(int *type, int *unit)
{
	/* ER, LR, ER2, ER1V, SR2, TR2+ */
	*type = NVSRAM_BOOTDEV_HARDDISK;
	*unit = *(uint8_t *)0xbb012088;
}

void
__nvsram_type_d(int *type, int *unit)
{
	/* TR2, ER+, ER2A */
	*type = *(uint8_t *)0xbb023030;
	*unit = *(uint8_t *)0xbb023034;
}

void
__nvsram_type_e(int *type, int *unit)
{
	/* LRC, TR2A, LER, LER_L, LER_H, MUS, LT, LT_L ... */
	*type = *(uint8_t *)0xbe493030;
	*unit = *(uint8_t *)0xbe493034;
}
