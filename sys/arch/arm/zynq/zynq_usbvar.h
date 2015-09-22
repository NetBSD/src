/*	$NetBSD: zynq_usbvar.h,v 1.1.2.3 2015/09/22 12:05:38 skrll Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
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

#ifndef _ARM_ZYNQ_ZYNQ_USBVAR_H
#define _ARM_ZYNQ_ZYNQ_USBVAR_H

struct zynqehci_softc;

enum zynq_usb_role {
	ZYNQUSB_HOST,
	ZYNQUSB_DEVICE,
	ZYNQUSB_OTG
};

enum zynq_usb_if {
	ZYNQUSBC_IF_UTMI,
	ZYNQUSBC_IF_PHILIPS,
	ZYNQUSBC_IF_ULPI,
	ZYNQUSBC_IF_SERIAL,
	ZYNQUSBC_IF_UTMI_WIDE
};

struct zynqehci_softc {
	ehci_softc_t sc_hsc;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct zynqusbc_softc *sc_usbc;
	enum zynq_usb_if sc_iftype;
	enum zynq_usb_role sc_role;
};

void zynqusb_attach_common(device_t parent, device_t self,
    bus_space_tag_t, bus_dma_tag_t, paddr_t, size_t, int, int,
    enum zynq_usb_if, enum zynq_usb_role);

/*
 * defined in zynq7000_usb.c
 */
int zynqusb_match(device_t, cfdata_t, void *);
void zynqusb_attach(device_t, device_t, void *);
void zynqusb_reset(struct zynqehci_softc *);

#endif	/* _ARM_ZYNQ_ZYNQ_USBVAR_H */
