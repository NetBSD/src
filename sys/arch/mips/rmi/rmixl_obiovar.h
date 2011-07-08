/*	$NetBSD: rmixl_obiovar.h,v 1.4 2011/07/08 19:10:14 dyoung Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors.
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

#ifndef _MIPS_RMI_RMIXL_OBIOVAR_H_
#define _MIPS_RMI_RMIXL_OBIOVAR_H_

#include <sys/bus.h>
#include <dev/pci/pcivar.h>
#include <mips/pci_machdep.h>

struct obio_attach_args {
	bus_space_tag_t	obio_eb_bst;
	bus_space_tag_t	obio_el_bst;
	bus_addr_t	obio_addr;
	bus_size_t	obio_size;
	int		obio_intr;
	int		obio_tmsk;
	unsigned int	obio_mult;
	bus_dma_tag_t	obio_29bit_dmat;
	bus_dma_tag_t	obio_32bit_dmat;
	bus_dma_tag_t	obio_64bit_dmat;
};

typedef struct obio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_eb_bst;
	bus_space_tag_t		sc_el_bst;
	bus_dma_tag_t		sc_29bit_dmat;
	bus_dma_tag_t		sc_32bit_dmat;
	bus_dma_tag_t		sc_64bit_dmat;
	bus_addr_t		sc_base;
	bus_size_t		sc_size;
} obio_softc_t;

#endif /* _MIPS_RMI_RMIXL_OBIOVAR_H_ */
