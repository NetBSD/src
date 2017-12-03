/* $NetBSD: sbbrz.c,v 1.3.14.1 2017/12/03 11:36:29 jdolecek Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 * 
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 * 
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 * 
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
 * 
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* from: $NetBSD: apecs.c,v 1.38 2000/06/29 08:58:45 mrg Exp */

/*-
 * Copyright (c) 2000, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Driver for SB-1250 I/O bridge 0, which provides the PCI and LDT
 * interfaces.
 */

#define	_MIPS_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <mips/locore.h>
#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>
#include <mips/sibyte/include/zbbusvar.h>
#include <mips/sibyte/pci/sbbrzvar.h>

static int	sbbrz_match(device_t, cfdata_t, void *);
static void	sbbrz_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sbbrz, 0, sbbrz_match, sbbrz_attach, NULL, NULL);

static int	sbbrz_print(void *, const char *pnp);

/* There can be only one. */
struct sbbrz_softc sbbrz_softc;

static int
sbbrz_match(device_t parent, cfdata_t match, void *aux)
{
	struct zbbus_attach_args *zap = aux;

	if (zap->za_locs.za_type != ZBBUS_ENTTYPE_BRZ)
		return (0);
  
	if (sbbrz_softc.sc_dev != NULL)
		return (0);

	return 1;
}

/*
 * Set up the chipset's function pointers.
 */
static void
sbbrz_init(struct sbbrz_softc *sc)
{
	int error;

	sbbrz_bus_io_init(&sc->sc_iot, sc);
	sbbrz_bus_mem_init(&sc->sc_memt, sc);

	bus_dma_tag_t t = &sc->sc_dmat64;
	t->_cookie = sc;
	t->_wbase = 0;
	t->_bounce_alloc_lo = 0;
	t->_bounce_alloc_hi = 0;
	t->_dmamap_ops = mips_bus_dmamap_ops;
	t->_dmamem_ops = mips_bus_dmamem_ops;
	t->_dmatag_ops = mips_bus_dmatag_ops;

	error = bus_dmatag_subregion(t, 0, (bus_addr_t)1 << 32, &sc->sc_dmat32, 0);
	if (error)
		panic("%s: failed to create 32bit dma tag: %d",  
		    __func__, error);

	sbbrz_pci_init(&sc->sc_pc, sc);
}

static void
sbbrz_attach(device_t parent, device_t self, void *aux)
{
	struct sbbrz_softc *sc = &sbbrz_softc;
	struct pcibus_attach_args pba;
        uint64_t regval;
        bool host;

        /* Tell the user whether it's host or device mode. */
        regval = mips3_ld((register_t)MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_CFG));
        host = (regval & M_SYS_PCI_HOST) != 0;

        aprint_normal(": %s pci mode\n", host ? "host" : "device");

	/* note that we've attached the bridge; can't have two. */
	sc->sc_dev = self;
	self->dv_private = sc;

	/*
         * set up the bridge's info; done once at console init time
	 * (maybe), but doesn't hurt to do twice.
	 */
        sbbrz_init(sc);

#if _has_pba_busname
	pba.pba_busname = "pci";
#endif
	pba.pba_iot = &sc->sc_iot;
	pba.pba_memt = &sc->sc_memt;
	pba.pba_dmat64 = &sc->sc_dmat64;
	pba.pba_dmat = sc->sc_dmat32;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	config_found(self, &pba, sbbrz_print);
}

static int
sbbrz_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;
                
	/* only PCIs can attach to sbbrz; easy. */
	if (pnp)
#if _has_pba_busname
		aprint_normal("%s at %s\n", pba->pba_busname, pnp);
#else
		aprint_normal("\n* sbbrz_pci at %s", pnp);
#endif
	aprint_normal(" bus %d", pba->pba_bus);
	return (UNCONF);
}       
