/*	$NetBSD: txioman.c,v 1.9.22.1 2011/06/12 00:23:58 rmind Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: txioman.c,v 1.9.22.1 2011/06/12 00:23:58 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <hpcmips/tx/tx39var.h>

#include <dev/hpc/hpciovar.h>

int txioman_match(device_t, cfdata_t, void *);
void txioman_attach(device_t, device_t, void *);
void txioman_callback(device_t);
int txioman_print(void *, const char *);
hpcio_chip_t tx_conf_reference_ioman(void *, int);

CFATTACH_DECL_NEW(txioman, 0,
    txioman_match, txioman_attach, NULL, NULL);

int
txioman_match(device_t parent, cfdata_t cf, void *aux)
{
	return (1);
}

void
txioman_attach(device_t parent, device_t self, void *aux)
{
	printf("\n");

	config_defer(self, txioman_callback);
}

void
txioman_callback(device_t self)
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
