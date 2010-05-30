/* $NetBSD: opbvar.h,v 1.4.94.1 2010/05/30 05:17:02 rmind Exp $ */

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

#include <machine/bus.h>

struct opb_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_zmiih;
	bus_space_handle_t sc_rgmiih;
};

struct opb_attach_args {
	const char *opb_name;
	int opb_instance;
	u_long opb_addr;
	int opb_irq;
	bus_space_tag_t opb_bt;		/* Bus space tag */
	bus_dma_tag_t opb_dmat;		/* DMA tag */

	int opb_flags;
#define	OPB_FLAGS_EMAC_GBE		(1 << 0)  /* emac Giga bit Ethernet */
#define	OPB_FLAGS_EMAC_STACV2		(1 << 1)  /* emac Other version STAC */
#define	OPB_FLAGS_EMAC_HT256		(1 << 2)  /* emac 256bit Hash Table */
#define	OPB_FLAGS_EMAC_RMII_ZMII	(1 << 3)  /* emac RMII uses ZMII */
#define	OPB_FLAGS_EMAC_RMII_RGMII	(1 << 4)  /* emac RMII uses RGMII */
};

/* For use before opb_attach() is called */
extern bus_space_tag_t opb_get_bus_space_tag(void);
extern int (*opb_get_frequency)(void);
