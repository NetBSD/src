/*	$NetBSD: imxusbvar.h,v 1.7 2023/05/04 17:09:44 bouyer Exp $	*/
/*
 * Copyright (c) 2019  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMXUSBVAR_H
#define _ARM_IMX_IMXUSBVAR_H

struct imxehci_softc;

enum imx_usb_role {
	IMXUSB_HOST,
	IMXUSB_DEVICE
};

struct imxusbc_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_ioh_usbnc;

	struct clk *sc_clk;

	/* filled in by platform dependent param & routine */
	bus_addr_t sc_ehci_offset;
	bus_size_t sc_ehci_size;

	void (* sc_init_md_hook)(struct imxehci_softc *, uintptr_t);
	void *(* sc_intr_establish_md_hook)(struct imxehci_softc *, uintptr_t);
	void (* sc_setup_md_hook)(struct imxehci_softc *, enum imx_usb_role,
				  uintptr_t);
	uintptr_t sc_md_hook_data;
};

struct imxusbc_attach_args {
	bus_space_tag_t aa_iot;
	bus_space_handle_t aa_ioh;
	bus_dma_tag_t aa_dmat;
	int aa_unit;	/* 0: OTG, 1: HOST1, 2: HOST2 ... */
	int aa_irq;
};

enum imx_usb_if {
	IMXUSBC_IF_UTMI,
	IMXUSBC_IF_PHILIPS,
	IMXUSBC_IF_ULPI,
	IMXUSBC_IF_SERIAL,
	IMXUSBC_IF_UTMI_WIDE,
	IMXUSBC_IF_HSIC
};

struct imxehci_softc {
	ehci_softc_t sc_hsc;

	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	void *sc_ih;

	struct imxusbc_softc *sc_usbc;

	uint sc_unit;
	enum imx_usb_if sc_iftype;
};

int imxusbc_attach_common(device_t, device_t, bus_space_tag_t, bus_addr_t, bus_size_t);
void imxehci_reset(struct imxehci_softc *);

#endif	/* _ARM_IMX_IMXUSBVAR_H */
