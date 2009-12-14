/*	$NetBSD: rmixl_usbivar.h,v 1.1.2.1 2009/12/14 07:21:41 cliff Exp $	*/

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

extern void *rmixl_usbi_intr_establish(void *, u_int, int (*)(void *), void *);
extern void  rmixl_usbi_intr_disestablish(void *, void *);

#endif /* _MIPS_RMI_RMIXL_USBIVAR_H_ */
