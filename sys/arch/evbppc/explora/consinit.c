/*	$NetBSD: consinit.c,v 1.4 2003/07/25 11:44:19 scw Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: consinit.c,v 1.4 2003/07/25 11:44:19 scw Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/explora.h>
#include <machine/bus.h>

#include "com.h"
#if NCOM > 0
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif
#include "pckbd.h"

#include <evbppc/explora/dev/elbvar.h>

#include "opt_explora.h"

#ifndef COM_CONSOLE_SPEED
#define COM_CONSOLE_SPEED	9600
#endif

void
consinit(void)
{
	bus_space_tag_t tag;
	static int done = 0;
#ifndef COM_IS_CONSOLE
	extern void fb_cnattach(bus_space_tag_t, bus_addr_t, void *);
#endif

	if (done)
		return;

	done = 1;

#ifdef COM_IS_CONSOLE
	tag = elb_get_bus_space_tag(BASE_COM);
	comcnattach(tag, BASE_COM, COM_CONSOLE_SPEED,
	    COM_FREQ, COM_TYPE_NORMAL,
	    (TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8);
#else
	/* Clear VRam */
	memset((void *)BASE_FB, 0, SIZE_FB);

	tag = elb_get_bus_space_tag(BASE_FB);
	fb_cnattach(tag, BASE_FB2, (void *)BASE_FB);
	tag = elb_get_bus_space_tag(BASE_PCKBC);
	pckbc_cnattach(tag, BASE_PCKBC, BASE_PCKBC2-BASE_PCKBC, PCKBC_KBD_SLOT);
#endif
}
