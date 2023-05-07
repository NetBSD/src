/* $NetBSD: plicvar.h,v 1.1 2023/05/07 12:41:48 skrll Exp $ */

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Portions of this code is derived from software contributed to The NetBSD
 * Foundation by Simon Burge.
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

#ifndef _RISCV_PLICVAR_H
#define	_RISCV_PLICVAR_H

struct plic_intrhand {
	int	(*ih_func)(void *);
	void	*ih_arg;
	bool	ih_mpsafe;
	u_int	ih_irq;
	u_int	ih_cidx;
};

struct plic_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	uint32_t		sc_ndev;

	uint32_t		sc_context[MAXCPUS];
	struct plic_intrhand	*sc_intr;
	struct evcnt		*sc_intrevs;
};


int	plic_intr(void *);
void	plic_enable(struct plic_softc *, u_int, u_int);
void	plic_disable(struct plic_softc *, u_int, u_int);
void	plic_set_priority(struct plic_softc *, u_int, uint32_t);
void	plic_set_threshold(struct plic_softc *, cpuid_t, uint32_t);
int	plic_attach_common(struct plic_softc *, bus_addr_t, bus_size_t);

void *	plic_intr_establish_xname(u_int, int, int, int (*)(void *), void *,
	    const char *);
void	plic_intr_disestablish(void *);


#endif /* _RISCV_PLICVAR_H */
