/* $NetBSD: dcms.c,v 1.1.2.2 1999/04/17 13:45:53 nisimura Exp $ */

/*
 * Copyright (c) 1998, 1999 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Tohru Nishimura
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dcms.c,v 1.1.2.2 1999/04/17 13:45:53 nisimura Exp $");

/*
 * WSCONS attachments for VSXXX and DC7085 combo
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsmousevar.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/dec/vsxxxvar.h>
#include <pmax/ibus/dc7085reg.h>	/* XXX dev/ic/dc7085reg.h XXX */
#include <pmax/ibus/dc7085var.h>	/* XXX machine/dc7085var.h XXX */

#include "locators.h"

static int  dcms_match __P((struct device *, struct cfdata *, void *));
static void dcms_attach __P((struct device *, struct device *, void *));

const struct cfattach dcms_ca = {
	sizeof(struct vsxxx_softc), dcms_match, dcms_attach,
};

static int
dcms_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if (strcmp(CFNAME(parent), "dc") != 0)
		return 0;
	if (cf->cf_loc[DCCF_LINE] != DCCF_LINE_DEFAULT) /* XXX correct? XXX */
		return 0;
#ifdef later
	initialize line #1 with 4800N8
	send a query and wait for the reply for a while
	if a valid reply was received,
		return 1
	return 0;
#else
	return 1;
#endif
}

static void
dcms_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dc_softc *dc = (void *)parent;
	struct vsxxx_softc *dcms = (void *)self;
	struct wsmousedev_attach_args a;

	printf("\n");

	a.accessops = &vsxxx_accessops;
	a.accesscookie = (void *)dcms;

	dcms->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	dc->sc_line[VSMSE].f = vsxxx_input;
	dc->sc_line[VSMSE].a = (void *)dcms;
}
