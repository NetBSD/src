/*	$NetBSD: phantomas.c,v 1.4.18.1 2009/05/13 17:17:43 jym Exp $	*/
/*	$OpenBSD: phantomas.c,v 1.1 2002/12/18 23:52:45 mickey Exp $	*/

/*
 * Copyright (c) 2002 Michael Shalayeff
 * All rights reserved.
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
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hp700/dev/cpudevs.h>

struct phantomas_softc {
	device_t sc_dev;
};

int	phantomasmatch(device_t, cfdata_t, void *);
void	phantomasattach(device_t, device_t, void *);
static void phantomas_callback(device_t self, struct confargs *ca);

CFATTACH_DECL_NEW(phantomas, sizeof(struct phantomas_softc),
    phantomasmatch, phantomasattach, NULL, NULL);

int
phantomasmatch(device_t parent, cfdata_t cfdata, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_type.iodc_type != HPPA_TYPE_BCPORT ||
	    ca->ca_type.iodc_sv_model != HPPA_BCPORT_PHANTOM) {
		return (0);
	}
	return (1);
}

void
phantomasattach(device_t parent, device_t self, void *aux)
{
	struct phantomas_softc *sc = device_private(self);
	struct confargs *ca = aux, nca;

	sc->sc_dev = self;
	nca = *ca;
	nca.ca_hpabase = 0;
	nca.ca_nmodules = MAXMODBUS;

	aprint_normal("\n");
	pdc_scanbus(self, &nca, phantomas_callback);
}

static void
phantomas_callback(device_t self, struct confargs *ca)
{

	config_found_sm_loc(self, "gedoens", NULL, ca, mbprint, mbsubmatch);
}
