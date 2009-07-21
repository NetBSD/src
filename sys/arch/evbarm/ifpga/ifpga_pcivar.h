/*	$NetBSD: ifpga_pcivar.h,v 1.2 2009/07/21 16:04:16 dyoung Exp $ */

/*
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

struct ifpga_pci_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_io_ioh;
	bus_space_handle_t	sc_conf_ioh;

	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_app0_ioh;
	bus_space_handle_t	sc_app1_ioh;
	bus_space_handle_t	sc_reg_ioh;
};

/* Apperture 0, 256MB normal cycles.  */
#define IFPGA_PCI_APP0_256MB_BASE	0x40000081
#define IFPGA_PCI_APP0_512MB_BASE	0x40000091
#define IFPGA_PCI_APP0_256MB_MAP	0x4006

/* Apperture 1, 256MB normal cycles, prefetchable.  */
#define IFPGA_PCI_APP1_256MB_BASE	0x50000081
#define IFPGA_PCI_APP1_256MB_MAP	0x5006

/* Apperture 1, 16MB configuration cycles.  */
#define IFPGA_PCI_APP1_CONF_BASE	0x61000041
#define IFPGA_PCI_APP1_CONF_T0_MAP	0x000a		/* Type 0 cycle */
#define IFPGA_PCI_APP1_CONF_T1_MAP	0x000b		/* Type 1 cycle */
