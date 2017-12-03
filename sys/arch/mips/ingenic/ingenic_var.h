/*	$NetBSD: ingenic_var.h,v 1.6.8.2 2017/12/03 11:36:28 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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

#ifndef INGENIC_VAR_H
#define INGENIC_VAR_H

#include <sys/bus.h>

struct apbus_attach_args {
	const char	*aa_name;
	bus_space_tag_t	aa_bst;
	bus_dma_tag_t	aa_dmat;
	bus_addr_t	aa_addr;
	uint32_t	aa_irq;
	uint32_t	aa_pclk;	/* PCLK in kHz */
	uint32_t	aa_mclk;	/* MCLK in kHz */
	uint32_t	aa_clockreg;
};

extern bus_space_tag_t ingenic_memt;
void apbus_init(void);

uint32_t mips_cp0_corectrl_read(void);
uint32_t mips_cp0_corestatus_read(void);
uint32_t mips_cp0_corereim_read(void);
uint32_t mips_cp0_corembox_read(u_int);

void mips_cp0_corectrl_write(uint32_t);
void mips_cp0_corestatus_write(uint32_t);
void mips_cp0_corereim_write(uint32_t);
void mips_cp0_corembox_write(u_int, uint32_t);

#endif /* INGENIC_VAR_H */
