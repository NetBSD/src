/*	$NetBSD: sysbeep_vidc.c,v 1.2 2002/02/18 16:23:54 bjh21 Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mark Brinicombe of Causality Limited.
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

#include <sys/param.h>

__RCSID("$NetBSD: sysbeep_vidc.c,v 1.2 2002/02/18 16:23:54 bjh21 Exp $");

#include <sys/systm.h>
#include <sys/device.h>

#include <arch/arm/iomd/beepvar.h>
#include <arch/arm/iomd/vidcaudiovar.h>

#include "beep.h"
#include "vidcaudio.h"
#include "lmcaudio.h"

/* Prototypes */
int sysbeep_vidc_match __P((struct device *parent, struct cfdata *cf, void *aux));
void sysbeep_vidc_attach __P((struct device *parent, struct device *self, void *aux));
void sysbeep __P((int pitch, int period));

void lmcaudio_beep_generate __P((void));

/* device attach structure */
struct cfattach sysbeep_vidc_ca = {
	sizeof(struct device), sysbeep_vidc_match, sysbeep_vidc_attach
};

int
sysbeep_vidc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);	/* XXX */
}

void
sysbeep_vidc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	printf("\n");
}

void
sysbeep(pitch, period)
	int pitch, period;
{
#if NVIDCAUDIO > 0
	vidcaudio_beep_generate();
#elif NLMCAUDIO > 0
	lmcaudio_beep_generate();
#elif NBEEP > 0
	beep_generate();
#endif /* NVIDCAUDIO */
}
