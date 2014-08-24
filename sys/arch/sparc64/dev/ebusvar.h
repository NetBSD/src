/*	$NetBSD: ebusvar.h,v 1.12 2014/08/24 19:06:14 palle Exp $	*/

/*
 * Copyright (c) 1999, 2000, 2001 Matthew R. Green
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SPARC64_DEV_EBUSVAR_H_
#define _SPARC64_DEV_EBUSVAR_H_

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/time.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

struct ebus_softc {
	device_t			sc_dev;

	int				sc_node;

	bus_space_tag_t			sc_memtag;	/* from pci */
	bus_space_tag_t			sc_iotag;	/* from pci */
	bus_space_tag_t			sc_childbustag;	/* pass to children */
	bus_dma_tag_t			sc_dmatag;

	struct ebus_ranges		*sc_range;
	struct ebus_interrupt_map	*sc_intmap;
	struct ebus_interrupt_map_mask	sc_intmapmask;

	int				sc_nrange;	/* counters */
	int				sc_nintmap;
};

int	ebus_setup_attach_args(struct ebus_softc *, int,
	    struct ebus_attach_args *);
void	ebus_destroy_attach_args(struct ebus_attach_args *);
int	ebus_print(void *, const char *);
void	ebus_find_ino(struct ebus_softc *, struct ebus_attach_args *);

paddr_t ebus_bus_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);

bus_space_tag_t ebus_alloc_bus_tag(struct ebus_softc *, int);

#endif
