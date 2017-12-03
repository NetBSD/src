/* $NetBSD: omap2_obiovar.h,v 1.2.2.1 2017/12/03 11:35:55 jdolecek Exp $ */

/*
 * Copyright (c) 2007 Microsoft
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _ARM_OMAP_OMAP2_OBIOVAR_H_
#define _ARM_OMAP_OMAP2_OBIOVAR_H_

struct obio_attach_args {
	const char *obio_name;
	bus_space_tag_t	obio_iot;
	bus_addr_t	obio_addr;
	bus_size_t	obio_size;
	int		obio_intr;
	bus_dma_tag_t	obio_dmat;
	unsigned int	obio_mult;
	unsigned int	obio_intrbase;
	int		obio_edmabase;
};

struct obio_softc {
	device_t		sc_dev;
	bus_dma_tag_t		sc_dmat;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_base;
	bus_size_t		sc_size;
	device_t		sc_obio_dev;
};

uint32_t omap_chipid(void);
uint32_t omap_devid(void);

#endif /* _ARM_OMAP_OMAP2_OBIOVAR_H_ */
