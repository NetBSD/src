/*	$NetBSD: pxa2x0_mci.h,v 1.1.6.2 2009/05/13 17:16:19 jym Exp $	*/

/*-
 * Copyright (c) 2006 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_PXA2X0_MCI_H_
#define	_PXA2X0_MCI_H_

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/xscale/pxa2x0_dmac.h>

struct pxamci_tag {
	void		*cookie;

	uint32_t	(*get_ocr)(void *);
	int		(*set_power)(void *, uint32_t);
	int		(*card_detect)(void *);
	int		(*write_protect)(void *);
};

struct pxamci_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	void			*sc_ih;
	device_t		sc_sdmmc;

	struct dmac_xfer	*sc_rxdx;
	bus_dma_segment_t	sc_rxdr;	/* read descriptor */
	struct dmac_xfer	*sc_txdx;
	bus_dma_segment_t	sc_txdr;	/* write descriptor */

	struct pxamci_tag	sc_tag;

	u_int			sc_caps;
#define	PMC_CAPS_4BIT	(1U << 0)
#define	PMC_CAPS_NO_DMA	(1U << 1)

	u_int			sc_flags;
#define	PMF_CARDINITED	(1U << 0)

	struct sdmmc_command * volatile sc_cmd;	/* command in progress */

	uint32_t sc_imask;

	uint32_t sc_ocr;			/* selected OCR */
	int sc_maxblklen;			/* maximum block length */
	int sc_buswidth;			/* current bus width */

	u_int sc_clkmin;
	u_int sc_clkmax;
	u_int sc_clkbase;
	uint32_t sc_clkrt;
};

int	pxamci_attach_sub(device_t, struct pxaip_attach_args *);
void	pxamci_card_detect_event(struct pxamci_softc *);

#endif	/* _PXA2X0_MCI_H_ */
