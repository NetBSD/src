/*	$NetBSD: machines.c,v 1.28 2005/07/16 10:43:00 hannken Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn of Wasabi Systems.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: machines.c,v 1.28 2005/07/16 10:43:00 hannken Exp $");
#endif	/* !__lint */

#include <sys/types.h>
#include "installboot.h"

struct ib_mach machines[] = {
	{ "alpha",	alpha_setboot,	alpha_clearboot,
		IB_STAGE1START | IB_ALPHASUM | IB_APPEND | IB_SUNSUM },
	{ "amd64",	i386_setboot,	no_clearboot,
		IB_RESETVIDEO | IB_CONSOLE | IB_CONSPEED | IB_CONSADDR |
		IB_KEYMAP | IB_PASSWORD | IB_TIMEOUT },
	{ "amiga",	amiga_setboot,	no_clearboot,
		IB_STAGE1START | IB_STAGE2START | IB_COMMAND },
	{ "hp300",	hp300_setboot,	no_clearboot,
		IB_APPEND },
	{ "hp700",	hp700_setboot,	hp700_clearboot,
		0 },
	{ "i386",	i386_setboot,	no_clearboot,
		IB_RESETVIDEO | IB_CONSOLE | IB_CONSPEED | IB_CONSADDR |
		IB_KEYMAP | IB_PASSWORD | IB_TIMEOUT },
	{ "macppc",	macppc_setboot,	macppc_clearboot,
		IB_STAGE2START },
	{ "news68k",	news68k_setboot, news68k_clearboot,
		IB_STAGE2START },
	{ "newsmips",	newsmips_setboot, newsmips_clearboot,
		IB_STAGE2START },
	{ "next68k",	next68k_setboot, no_clearboot,
		0 },
	{ "pmax",	pmax_setboot,	pmax_clearboot,
		IB_STAGE1START | IB_APPEND | IB_SUNSUM },
	{ "shark",	no_setboot,	no_clearboot, },
	{ "sparc",	sparc_setboot,	sparc_clearboot,
		IB_STAGE2START },
	{ "sparc64",	sparc64_setboot, sparc64_clearboot },
	{ "sun2",	sun68k_setboot,	sun68k_clearboot,
		IB_STAGE2START },
	{ "sun3",	sun68k_setboot,	sun68k_clearboot,
		IB_STAGE2START },
	{ "vax",	vax_setboot,	vax_clearboot,
		IB_STAGE1START | IB_APPEND | IB_SUNSUM },
	{ "x68k",	x68k_setboot,	x68k_clearboot,
		IB_STAGE1START | IB_STAGE2START },
	{ 0, 0, 0, 0 },
};
