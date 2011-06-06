/*	$NetBSD: rmixl_usbivar.h,v 1.1.8.1 2011/06/06 09:06:10 jruoho Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors
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

#ifndef _MIPS_RMI_RMIXL_USBIVAR_H_
#define _MIPS_RMI_RMIXL_USBIVAR_H_

#include <sys/bus.h>
#include <mips/bus_dma.h>

struct rmixl_usbi_attach_args {
	bus_space_tag_t	usbi_eb_bst;
	bus_space_tag_t	usbi_el_bst;
	bus_addr_t	usbi_addr;
	bus_size_t	usbi_size;
	int		usbi_intr;
	bus_dma_tag_t	usbi_dmat;
};

typedef struct rmixl_usbi_dispatch {
	int (*func)(void *);
	void *arg; 
	struct evcnt count;
} rmixl_usbi_dispatch_t;

typedef struct rmixl_usbi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_eb_bst;
	bus_space_tag_t		sc_el_bst;
	bus_addr_t		sc_addr;
	bus_size_t		sc_size;
	bus_dma_tag_t		sc_dmat;
	device_t		sc_ohci_devs[2];
	rmixl_usbi_dispatch_t	sc_dispatch[RMIXL_UB_INTERRUPT_MAX + 1];
} rmixl_usbi_softc_t;


#ifdef _KERNEL
void *rmixl_usbi_intr_establish(void *, u_int, int (*)(void *), void *);
void  rmixl_usbi_intr_disestablish(void *, void *);
#endif

#endif /* _MIPS_RMI_RMIXL_USBIVAR_H_ */
