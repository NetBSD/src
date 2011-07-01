/*	$NetBSD: consinit.c,v 1.8 2011/07/01 20:26:36 dyoung Exp $ */

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Console init functions for machines with VIDC
 *
 * Created      : 17/09/94
 * Updated	: 18/04/01 updated for new wscons
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: consinit.c,v 1.8 2011/07/01 20:26:36 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/cons.h>

#include <arm/iomd/vidc.h>
#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>
#include <arm/iomd/iomdkbcvar.h>
#include <arm/iomd/vidcvideo.h>

#include "vidcvideo.h"
#include "iomdkbc.h"

#if ((NVIDCVIDEO > 0) && (NIOMDKBC > 0))

extern videomemory_t videomemory;
extern struct bus_space iomd_bs_tag;

#endif


#ifdef COMCONSOLE
extern void comcninit(struct consdev *cp);
#endif


void
consinit(void)
{

	static int consinit_called = 0;

	if (consinit_called != 0)
		return;
	consinit_called = 1;

#ifdef COMCONSOLE
	comcninit(NULL);
	return;
#endif


#if ((NVIDCVIDEO > 0) && (NIOMDKBC > 0))
	vidcvideo_cnattach(videomemory.vidm_vbase);
#if NIOMDKBC > 0
	iomdkbc_cnattach(&iomd_bs_tag, IOMD_ADDRESS(IOMD_KBDDAT), 0);
#endif
	return;
#else
	/* XXX For old VIDC console. */
	cninit();
	return;
#endif
	panic("No console");	/* Will we ever see this?  */
}
