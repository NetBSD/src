/*	$NetBSD: ucbio.c,v 1.1 2000/02/27 16:34:13 uch Exp $	*/

/*
 * Copyright (c) 2000, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Device driver for PHILIPS UCB1200 Advanced modem/audio analog front-end
 *	General Purpose I/O part.
 */
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39sibvar.h>
#include <hpcmips/tx/tx39sibreg.h>

#include <hpcmips/dev/ucb1200var.h>
#include <hpcmips/dev/ucb1200reg.h>

#include "locators.h"

extern char*	btnmgr_name __P((long));

int	ucbio_match	__P((struct device*, struct cfdata*, void*));
void	ucbio_attach	__P((struct device*, struct device*, void*));

struct ucbio_softc {
	struct device	sc_dev;
};

struct cfattach ucbio_ca = {
	sizeof(struct ucbio_softc), ucbio_match, ucbio_attach
};

int	ucbio_print	__P((void*, const char*));
int	ucbio_submatch	__P((struct device*, struct cfdata*, void*));

int
ucbio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (1);
}

void
ucbio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ucb1200_attach_args *ucba = aux;
	struct ucbio_attach_args uia;
	int port;

	printf("\n");
	
	uia.uia_tc = ucba->ucba_tc;
	for (port = 0; port < UCB1200_IOPORT_MAX; port++) {
		uia.uia_port = port;
		config_found_sm(self, &uia, ucbio_print, ucbio_submatch);
	}
}

int
ucbio_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ucbio_attach_args *uia = aux;
	platid_mask_t mask;
	
	if (cf->cf_loc[NEWGPBUSIFCF_PORT] != uia->uia_port)
		return (0);

	mask = PLATID_DEREF(cf->cf_loc[NEWGPBUSIFCF_PLATFORM]);
	if (!platid_match(&platid, &mask))
		return (0);

	uia->uia_id = cf->cf_loc[NEWGPBUSIFCF_ID];
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
ucbio_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct ucbio_attach_args *uia = aux;
	
	if (!pnp) 
		printf(" port %d \"%s\"", uia->uia_port,
		       btnmgr_name(uia->uia_id));
	
	return (QUIET);
}
