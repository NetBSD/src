/* 	$NetBSD: xcvbusvar.h,v 1.2 2011/07/01 19:03:50 dyoung Exp $ */

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
#include <sys/types.h>

#include <sys/bus.h>

#ifndef _VIRTEX_DEV_VIRTEXVAR_H_
#define _VIRTEX_DEV_VIRTEXVAR_H_

struct plb_attach_args;

struct ll_dmac {
	bus_space_tag_t 	dmac_iot;
	bus_addr_t 		dmac_ctrl_addr; /* Control registers */
	bus_addr_t 		dmac_stat_addr; /* Status register */
	int 			dmac_chan; 	/* Channel number */
};

struct xcvbus_attach_args {
	/* LocalLink ("llbus") DMA channels. */
	struct ll_dmac 		*vaa_rx_dmac; 	/* Receive channel, if any */
	struct ll_dmac 		*vaa_tx_dmac; 	/* Transmit channel, if any */

	/* Generic portion, enough for DCR/PLB. */
	const char 		*vaa_name; 	/* Device name */
	bus_space_tag_t 	vaa_iot; 	/* Registers */
	bus_addr_t 		vaa_addr;
	bus_dma_tag_t 		vaa_dmat;
	int 			vaa_intr; 	/* Device interrupt line */
	int 			_vaa_is_dcr; 	/* XXX bst flag  */
};

void 	*ll_dmac_intr_establish(int, void(*)(void *), void *);
void 	ll_dmac_intr_disestablish(int, void *);

void 	virtex_autoconf(device_t, struct plb_attach_args *);

int 	xcvbus_print(void *, const char *);
int 	xcvbus_child_match(device_t, cfdata_t, void *);

#endif /* _VIRTEX_DEV_VIRTEXVAR_H_ */
