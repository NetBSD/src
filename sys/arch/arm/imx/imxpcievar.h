/*	$NetBSD: imxpcievar.h,v 1.3 2019/10/16 11:16:30 hkenken Exp $	*/

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

#ifndef	_ARM_IMX_IMXPCIEVAR_H_
#define	_ARM_IMX_IMXPCIEVAR_H_

struct imxpcie_ih;

struct imxpcie_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_root_ioh;
	bus_space_handle_t sc_gpr_ioh;
	bus_dma_tag_t sc_dmat;

	paddr_t sc_root_addr;
	size_t sc_root_size;

	struct arm32_pci_chipset sc_pc;

	TAILQ_HEAD(, imxpcie_ih) sc_intrs;

	void *sc_ih;
	kmutex_t sc_lock;
	u_int sc_intrgen;

	struct clk *sc_clk_pcie;
	struct clk *sc_clk_pcie_bus;
	struct clk *sc_clk_pcie_phy;
	struct clk *sc_clk_pcie_ext;
	struct clk *sc_clk_pcie_ext_src;
	bool sc_ext_osc;

	void *sc_cookie;
	void (* sc_pci_netbsd_configure)(void *);
	uint32_t (* sc_gpr_read)(void *, uint32_t);
	void (* sc_gpr_write)(void *, uint32_t, uint32_t);
	void (* sc_reset)(void *);

	bool sc_have_sw_reset;
};

struct imxpcie_ih {
	int (*ih_handler)(void *);
	void *ih_arg;
	int ih_ipl;
	TAILQ_ENTRY(imxpcie_ih) ih_entry;
};

int imxpcie_intr(void *);
void imxpcie_attach_common(struct imxpcie_softc *);

#endif	/* _ARM_IMX_IMXPCIEVAR_H_ */
