/*	$NetBSD: txioman.c,v 1.3.2.1 2002/02/11 20:08:11 jdolecek Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <hpcmips/tx/tx39var.h>

#include <dev/hpc/hpciovar.h>

int txioman_match(struct device *, struct cfdata *, void *);
void txioman_attach(struct device *, struct device *, void *);
void txioman_callback(struct device *);
int txioman_print(void *, const char *);
hpcio_chip_t tx_conf_reference_ioman(void *, int);

struct cfattach txioman_ca = {
	sizeof(struct device), txioman_match, txioman_attach
};

int
txioman_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (1);
}

void
txioman_attach(struct device *parent, struct device *self, void *aux)
{
	printf("\n");

	config_defer(self, txioman_callback);
}

void
txioman_callback(struct device *self)
{
	struct hpcio_attach_args haa;

	haa.haa_busname = HPCIO_BUSNAME;
	haa.haa_sc = tx_conf_get_tag();
	haa.haa_getchip = tx_conf_reference_ioman;
	haa.haa_iot = 0; /* not needed for TX */

	config_found(self, &haa, txioman_print);
}

int
txioman_print(void *aux, const char *pnp)
{

	return (QUIET);
}

void
tx_conf_register_ioman(tx_chipset_tag_t t, struct hpcio_chip *hc)
{
	KASSERT(t == tx_conf_get_tag());
	KASSERT(hc);

	t->tc_iochip[hc->hc_chipid] = hc;
}

hpcio_chip_t
tx_conf_reference_ioman(void *ctx, int iochip)
{
	struct tx_chipset_tag *t = tx_conf_get_tag();
	KASSERT(iochip >= 0 && iochip < NTXIO_GROUP);

	return (t->tc_iochip[iochip]);
}
