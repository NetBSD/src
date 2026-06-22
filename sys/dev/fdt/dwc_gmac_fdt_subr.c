/* $NetBSD: dwc_gmac_fdt_subr.c,v 1.1 2026/06/22 20:26:33 jakllsch Exp $ */

/*
 * Copyright (c) 2026 Jonathan A. Kollasch
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD");

#include <sys/param.h>
#include <sys/rndsource.h>

#include <dev/fdt/fdtvar.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <dev/mii/miivar.h>
#include <dev/fdt/dwc_gmac_fdt_subr.h>
#include <dev/ic/dwc_gmac_var.h>

void
dwc_gmac_preattach_fdt(int phandle, struct dwc_gmac_softc *sc)
{
	uint32_t pbl;

	if (of_getprop_uint32(phandle, "snps,pbl", &pbl) == 0) {
		sc->sc_rxpbl = sc->sc_txpbl = pbl;
	}
	if (of_getprop_uint32(phandle, "snps,txpbl", &pbl) == 0) {
		sc->sc_txpbl = pbl;
	}
	if (of_getprop_uint32(phandle, "snps,rxpbl", &pbl) == 0) {
		sc->sc_rxpbl = pbl;
	}
	if (of_hasprop(phandle, "snps,fixed-burst"))
		sc->sc_flags |= DWC_GMAC_FIXED_BURST;
#if notyet
	if (of_hasprop(phandle, "snps,force_thresh_dma_mode"))
		sc->sc_flags |= DWC_GMAC_FORCE_THRESH_DMA_MODE;
#endif
}
