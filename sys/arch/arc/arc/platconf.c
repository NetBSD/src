/*	$NetBSD: platconf.c,v 1.3 2003/07/15 00:04:44 lukem Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by SODA Noriyuki.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: platconf.c,v 1.3 2003/07/15 00:04:44 lukem Exp $");

#include "opt_platform.h"

#include <sys/param.h>

#include <machine/platform.h>

#define ARRAY_LENGTH(array)	(sizeof(array) / sizeof(array[0]))

struct platform *const plattab[] = {
#ifdef PLATFORM_ACER_PICA_61
	&platform_acer_pica_61,
#endif
#ifdef PLATFORM_DESKTECH_TYNE
	&platform_desktech_tyne,
#endif
#ifdef PLATFORM_DESKTECH_ARCSTATION_I
	&platform_desktech_arcstation_i,
#endif
#ifdef PLATFORM_MICROSOFT_JAZZ
	&platform_microsoft_jazz,
#endif
#ifdef PLATFORM_NEC_R94
	&platform_nec_r94,
#endif
#ifdef PLATFORM_NEC_R96
	&platform_nec_r96,
	&platform_nec_riscserver_2200,
#endif
#ifdef PLATFORM_NEC_RAX94
	&platform_nec_rax94,
#endif
#ifdef PLATFORM_NEC_RD94
	&platform_nec_rd94,
#endif
#ifdef PLATFORM_NEC_JC94
	&platform_nec_jc94,
#endif
#ifdef PLATFORM_NEC_J96A
	&platform_nec_j96a,
#endif
#ifdef PLATFORM_SNI_RM200PCI
	&platform_sni_rm200pci,
#endif
};

const int nplattab = ARRAY_LENGTH(plattab);
