/*	$OpenBSD: pyrovar.h,v 1.3 2007/04/03 19:59:01 kettenis Exp $	*/

/*
 * Copyright (c) 2007 Mark Kettenis
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

struct pyro_range {
	u_int32_t	cspace;
	u_int32_t	child_hi;
	u_int32_t	child_lo;
	u_int32_t	phys_hi;
	u_int32_t	phys_lo;
	u_int32_t	size_hi;
	u_int32_t	size_lo;
};

struct pyro_pbm {
	struct pyro_softc *pp_sc;

	struct pyro_range *pp_range;
	pci_chipset_tag_t pp_pc;
	int pp_nrange;

	bus_space_tag_t		pp_memt;
	bus_space_tag_t		pp_iot;
	bus_space_tag_t		pp_cfgt;
	bus_space_handle_t	pp_cfgh;
	bus_dma_tag_t		pp_dmat;
	int			pp_flags;
	int			pp_bus_a;
	struct iommu_state	pp_is;
	struct strbuf_ctl	pp_sb;
};

struct pyro_softc {
	device_t sc_dev;
	int sc_node;
	int sc_ign;
	bus_dma_tag_t sc_dmat;
	bus_space_tag_t sc_bustag;
	bus_addr_t sc_csr, sc_xbc;
	bus_space_handle_t sc_csrh, sc_xbch;

	int sc_oberon;
};
