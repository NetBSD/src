/* 	$NetBSD: xcvbus.c,v 1.3 2011/07/01 19:03:50 dyoung Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xcvbus.c,v 1.3 2011/07/01 19:03:50 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <sys/bus.h>
#include <powerpc/ibm4xx/dev/plbvar.h>

#include <evbppc/virtex/dev/xcvbusvar.h>


static int	xcvbus_match(device_t, cfdata_t, void *);
static void	xcvbus_attach(device_t, device_t, void *);


CFATTACH_DECL_NEW(xcvbus, 0,
    xcvbus_match, xcvbus_attach, NULL, NULL);


static int
xcvbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct plb_attach_args 		*paa = aux;

	return (strcmp(paa->plb_name, cf->cf_name) == 0);
}

static void
xcvbus_attach(device_t parent, device_t self, void *aux)
{
	struct plb_attach_args 		*paa = aux;

	printf(": Xilinx Virtex FPGA\n");

	virtex_autoconf(self, paa);
}

/* 
 * Public interface.
 */

int
xcvbus_print(void *aux, const char *pnp)
{
	struct xcvbus_attach_args 	*vaa = aux;

	if (pnp)
		aprint_normal("%s at %s", vaa->vaa_name, pnp);

	if (vaa->vaa_addr != -1) {
		if (vaa->_vaa_is_dcr)
			aprint_normal(" dcr 0x%x", vaa->vaa_iot->pbs_base);
		else
			aprint_normal(" mem 0x%x", vaa->vaa_addr);
	}

	if (vaa->vaa_intr != -1)
		aprint_normal(" intr %d", vaa->vaa_intr);
	if (vaa->vaa_tx_dmac != NULL)
		aprint_normal(" tx %d", vaa->vaa_tx_dmac->dmac_chan);
	if (vaa->vaa_rx_dmac != NULL)
		aprint_normal(" rx %d", vaa->vaa_rx_dmac->dmac_chan);

	return (UNCONF);
}

int
xcvbus_child_match(device_t parent, cfdata_t cf, void *aux)
{
	struct xcvbus_attach_args *vaa = aux;

	return (strcmp(vaa->vaa_name, cf->cf_name) == 0);
}
