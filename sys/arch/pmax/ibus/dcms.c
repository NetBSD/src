/* $NetBSD: dcms.c,v 1.1.2.1 1999/03/31 01:48:13 nisimura Exp $ */

/*
 * Copyright (c) 1998 Tohru Nishimura.  All rights reserved.
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

__KERNEL_RCSID(0, "$NetBSD: dcms.c,v 1.1.2.1 1999/03/31 01:48:13 nisimura Exp $");

/*
 * WSCONS attachments for VSXXX and DC7085 combo
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsmousevar.h>

#include <dev/dec/vsxxxvar.h>
#include <pmax/ibus/dc7085reg.h>	/* XXX dev/dec/dc7085reg.h XXX */
#include <pmax/ibus/dc7085var.h>	/* XXX 'Think different(ly)' XXX */

static int  dcms_match __P((struct device *, struct cfdata *, void *));
static void dcms_attach __P((struct device *, struct device *, void *));

const struct cfattach dcms_ca = {
	sizeof(struct vsxxx_softc), dcms_match, dcms_attach,
};

#define	CFNAME(cf) ((cf)->dv_cfdata->cf_driver->cd_name)

static int
dcms_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if (strcmp(CFNAME(parent), "dc") != 0)
		return 0;
	/* XXX check here whether mice responses by query XXX */
	return 1;
}

static void
dcms_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dc_softc *dc = (void *)parent;
	struct vsxxx_softc *dcms = (void *)self;
	struct dc7085reg *reg = dc->sc_reg;
	struct wsmousedev_attach_args a;
	int s;

	printf("\n");

	s = spltty();
        reg->dccsr = DC_CLR;
	delay(1000);
        reg->dctcr = VSLINE;
        reg->dclpr = DC_RE | DC_B4800 | DC_BITS8 | VSLINE << 3;
        reg->dccsr = DC_MSE | DC_RIE | DC_TIE;
	splx(s);

	a.accessops = &vsxxx_accessops;
	a.accesscookie = (void *)dcms;

	dcms->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	dc->sc_input[VSLINE].f = vsxxx_input;
	dc->sc_input[VSLINE].a = (void *)dcms;
}
