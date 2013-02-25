/*	$NetBSD: empbvar.h,v 1.2.2.1 2013/02/25 00:28:22 tls Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#include <sys/types.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <machine/pci_machdep.h>

/*
 * Structure used to describe PCI devices with memory that can be used as
 * bounce buffers. XXX: not used yet.
 */
struct empb_dmamemdev_entry {
	/* location of the device on bus */
	int	bus;
	int	dev;
	int	function;
	/* how to find memory on device */
	uint8_t	bar;	/* which BAR will be used to access the mem */
	uint32_t off;	/* offset from BAR address */
	uint32_t size;	/* how much memory will we steal */
};

struct empb_softc {
	device_t			sc_dev;

	uint16_t			model;

	struct bus_space_tag		setup_area;
	bus_space_tag_t			setup_area_t;
	bus_space_handle_t		setup_area_h;

	struct bus_space_tag		pci_confio_area;
	bus_space_tag_t			pci_confio_t;
	bus_space_handle_t		pci_confio_h;
	uint8_t				pci_confio_mode;

	struct bus_space_tag		pci_mem_win;
	bus_space_tag_t			pci_mem_win_t;
	uint32_t			pci_mem_win_size;
	bus_addr_t			pci_mem_win_pos;
	uint16_t			pci_mem_win_mask;

	struct amiga_pci_chipset	apc;

};


bus_addr_t	empb_switch_window(struct empb_softc *sc, bus_addr_t address);

