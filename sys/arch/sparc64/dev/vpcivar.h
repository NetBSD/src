/*	$NetBSD: vpcivar.h,v 1.1.18.2 2017/12/03 11:36:44 jdolecek Exp $	*/

/*
 * Copyright (c) 2015 Palle Lyckegaard
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

struct vpci_range {
	u_int32_t	cspace;
	u_int32_t	child_hi;
	u_int32_t	child_lo;
	u_int32_t	phys_hi;
	u_int32_t	phys_lo;
	u_int32_t	size_hi;
	u_int32_t	size_lo;
};

struct vpci_pbm {
	struct vpci_softc *vp_sc;
	uint64_t vp_devhandle;

	struct vpci_range *vp_range;
	pci_chipset_tag_t vp_pc;
	int vp_nrange;

	bus_space_tag_t		vp_memt;
	bus_space_tag_t		vp_iot;
	bus_space_tag_t		vp_cfgt;
	bus_space_handle_t	vp_cfgh;
	bus_dma_tag_t		vp_dmat;
	int			vp_flags;
	int			vp_bus_a;
	struct iommu_state	vp_is;
	struct strbuf_ctl	vp_sb;
};

struct vpci_softc {
	device_t sc_dev;
	int sc_node;
	int sc_ign;
	bus_dma_tag_t sc_dmat;
	bus_space_tag_t sc_bustag;
#if 0
FIXME	
	bus_addr_t sc_csr, sc_xbc;
	bus_space_handle_t sc_csrh, sc_xbch;
#else	
	bus_addr_t sc_csr;
	bus_space_handle_t sc_csrh;
#endif	
#if 0
FiXME	
	int sc_oberon;
#endif	
};
