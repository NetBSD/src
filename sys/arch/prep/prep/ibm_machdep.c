/*      $NetBSD: ibm_machdep.c,v 1.6 2002/05/30 16:10:08 nonaka Exp $        */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#include "opt_platform.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/platform.h>

static struct platform *platform_ibm[] = {
#if defined(PLATFORM_IBM_6050)
	&platform_ibm_6050,
#endif
#if defined(PLATFORM_IBM_7248)
	&platform_ibm_7248,
#endif
#if defined(PLATFORM_IBM_7043_140)
	&platform_ibm_7043_140,
#endif
	NULL
};

struct plattab plattab_ibm = {
	platform_ibm,	sizeof(platform_ibm)/sizeof(platform_ibm[0]) - 1
};

void
cpu_setup_ibm_generic(struct device *dev)
{
	u_char l2ctrl, cpuinf;

	/* system control register */
	l2ctrl = *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x81c);
	/* device status register */
	cpuinf = *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x80c);

	/* Enable L2 cache */
	*(volatile u_char *)(PREP_BUS_SPACE_IO + 0x81c) = l2ctrl | 0xc0;
}
