/*	$NetBSD: ebusvar.h,v 1.1 2002/02/18 04:44:41 uwe Exp $ */

/*
 * Copyright (c) 1999, 2000 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _DEV_EBUS_EBUSVAR_H_
#define _DEV_EBUS_EBUSVAR_H_

/*
 * ebus arguments; ebus attaches to a pci, and devices attach
 * to the ebus.
 */

struct ebus_attach_args {
	char			*ea_name;	/* PROM name */
	int			ea_node;	/* PROM node */

	bus_space_tag_t		ea_bustag;
	bus_dma_tag_t		ea_dmatag;

	struct ebus_regs	*ea_reg;	/* registers */
	u_int32_t		*ea_vaddr;	/* virtual addrs */
	u_int32_t		*ea_intr;	/* interrupts */

	int			ea_nreg;	/* number of them */
	int			ea_nvaddr;
	int			ea_nintr;
};

#define ebus_bus_map(t, a, s, f, v, hp) \
	bus_space_map2(t, 0, a, s, f, v, hp)

#endif /* _DEV_EBUS_EBUSVAR_H_ */
