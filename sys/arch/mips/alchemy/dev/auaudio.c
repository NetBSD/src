/* $NetBSD: auaudio.c,v 1.8 2011/06/06 17:13:06 matt Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auaudio.c,v 1.8 2011/06/06 17:13:06 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/aubusvar.h>

static int	auaudio_match(device_t, cfdata_t, void *);
static void	auaudio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(auaudio, sizeof (struct device),
    auaudio_match, auaudio_attach, NULL, NULL);

int
auaudio_match(device_t parent, cfdata_t match, void *aux)
{
	struct aubus_attach_args *aa = aux;

	return (0);	/* XXX unimplemented! */
	if (strcmp(aa->aa_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
auaudio_attach(device_t parent, device_t self, void *aux)
{

	printf(": Au1X00 audio\n");	/* \n in clockattach */
}
