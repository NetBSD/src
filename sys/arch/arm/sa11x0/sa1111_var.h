/*	$NetBSD: sa1111_var.h,v 1.10.64.2 2009/06/20 07:20:01 yamt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
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

#ifndef _SA1111_VAR_H
#define _SA1111_VAR_H

struct sacc_intrhand {
	void *ih_soft;
	int ih_irq;
	struct sacc_intrhand *ih_next;
};

struct sacc_intrvec {
	uint32_t lo;	/* bits 0..31 */
	uint32_t hi;	/* bits 32..54 */
};

struct sacc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_tag_t sc_piot;	/* parent(SA-1110)'s iot */
	bus_space_handle_t sc_gpioh;

	uint32_t sc_gpiomask;	/* SA-1110 GPIO mask */

	struct sacc_intrvec sc_imask;
	struct sacc_intrhand *sc_intrhand[SACCIC_LEN];
	int sc_intrtype[SACCIC_LEN];
};

typedef void *sacc_chipset_tag_t;

struct sa1111_attach_args {
	bus_addr_t		sa_addr;	/* i/o address  */
	bus_size_t		sa_size;
	int			sa_intr;
};

#define IST_EDGE_RAISE	6
#define IST_EDGE_FALL	7

void	*sacc_intr_establish(sacc_chipset_tag_t *, int, int, int,
			  int (*)(void *), void *);
void	sacc_intr_disestablish(sacc_chipset_tag_t *, void *);
int	sacc_probe(device_t, cfdata_t, void *);
int	sa1111_search(device_t, cfdata_t, const int *, void *);

#endif /* _SA1111_VAR_H */
