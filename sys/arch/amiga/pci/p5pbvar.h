/*	$NetBSD: p5pbvar.h,v 1.1.2.1 2012/04/17 00:06:02 yamt Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#ifndef _AMIGA_P5PBVAR_H_

#include <sys/types.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <machine/pci_machdep.h>

struct p5pb_autoconf_entry {
	volatile char	*base;
	uint32_t	size;
	TAILQ_ENTRY(p5pb_autoconf_entry) entries;
};

struct p5pb_softc {
	device_t				sc_dev;

	struct p5bus_attach_args		*p5baa;

	struct bus_space_tag			pci_conf_area;
	struct bus_space_tag			pci_mem_area;
	struct bus_space_tag			pci_io_area;
	struct amiga_pci_chipset		apc;

	uint8_t					bridge_type;
#define P5PB_BRIDGE_CVPPC			1
#define P5PB_BRIDGE_GREX1200			2
#define P5PB_BRIDGE_GREX4000			3

	uint32_t				pci_mem_lowest;
	uint32_t				pci_mem_highest;

	/* list of preconfigured BARs */
	TAILQ_HEAD(, p5pb_autoconf_entry)	auto_bars;
};

#endif /* _AMIGA_P5PBVAR_H_ */
