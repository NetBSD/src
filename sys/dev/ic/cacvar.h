/*	$NetBSD: cacvar.h,v 1.4.2.1 2000/06/22 17:06:38 minoura Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef _IC_CACVAR_H_
#define _IC_CACVAR_H_

#include "locators.h"

#define	CAC_MAX_CCBS	20
#define	CAC_MAX_XFER	(0xffff * 512)
#define CAC_SG_SIZE	32

struct cac_softc;
struct cac_ccb;

struct cac_context {
	void		(*cc_handler) __P((struct cac_ccb *, int));
	struct device	*cc_dv;
	void 		*cc_context;
};

struct cac_ccb {
	/* Data the controller will touch - 276 bytes */
	struct cac_hdr	ccb_hdr;
	struct cac_req	ccb_req;
	struct cac_sgb	ccb_seg[CAC_SG_SIZE];

	/* Data the controller won't touch */
	int		ccb_flags;
	int		ccb_datasize;
	paddr_t		ccb_paddr;
	bus_dmamap_t	ccb_dmamap_xfer;
	SIMPLEQ_ENTRY(cac_ccb) ccb_chain;
	struct cac_context ccb_context;
};

#define	CAC_CCB_DATA_IN		0x0001
#define	CAC_CCB_DATA_OUT	0x0002

struct cac_linkage {
	void	(*cl_submit) __P((struct cac_softc *, paddr_t));
	paddr_t	(*cl_completed) __P((struct cac_softc *));
	int	(*cl_intr_pending) __P((struct cac_softc *));
	void	(*cl_intr_enable) __P((struct cac_softc *, int));
	int	(*cl_fifo_full) __P((struct cac_softc *));
};

struct cac_softc {
	struct device		sc_dv;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dmamap;
	void			*sc_ih;
	struct cac_linkage	*sc_cl;
	char			*sc_typestr;
	caddr_t			sc_ccbs;
	paddr_t			sc_ccbs_paddr;
	SIMPLEQ_HEAD(, cac_ccb)	sc_ccb_free;	
	SIMPLEQ_HEAD(, cac_ccb)	sc_ccb_queue;
};

struct cac_attach_args {
	int		caca_unit;
};

#define CACACF_UNIT	0

#define	cacacf_unit	cf_loc[CACACF_UNIT]

#define	CACACF_UNIT_UNKNOWN 	-1

int	cac_cmd __P((struct cac_softc *, int, void *, int, int, int, int, 
		struct cac_context *));
void	cac_minphys __P((struct buf *));
int	cac_intr __P((void *));
int	cac_init __P((struct cac_softc *, const char *));
void	cac_ccb_free __P((struct cac_softc *, struct cac_ccb *));
int	cac_ccb_start __P((struct cac_softc *, struct cac_ccb *));
struct	cac_ccb *cac_ccb_alloc __P((struct cac_softc *, int));

#endif	/* !_IC_CACVAR_H_ */
