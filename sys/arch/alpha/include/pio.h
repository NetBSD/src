/*	$NetBSD: pio.h,v 1.5 2000/06/19 12:19:38 simonb Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _ALPHA_PIO_H_
#define	_ALPHA_PIO_H_

#include <machine/bus.h>

#ifdef _KERNEL
#error This file is for userspace only.
#else
struct alpha_pci_io_ops {
	u_int8_t	(*apio_inb)(bus_addr_t);
	u_int16_t	(*apio_inw)(bus_addr_t);
	u_int32_t	(*apio_inl)(bus_addr_t);
	void		(*apio_outb)(bus_addr_t, u_int8_t);
	void		(*apio_outw)(bus_addr_t, u_int16_t);
	void		(*apio_outl)(bus_addr_t, u_int32_t);
};

extern const struct alpha_pci_io_ops *alpha_pci_io_switch;

#define	inb(addr)	(*alpha_pci_io_switch->apio_inb)((addr))
#define	inw(addr)	(*alpha_pci_io_switch->apio_inw)((addr))
#define	inl(addr)	(*alpha_pci_io_switch->apio_inl)((addr))

#define	outb(addr, val)	(*alpha_pci_io_switch->apio_outb)((addr), (val))
#define	outw(addr, val)	(*alpha_pci_io_switch->apio_outw)((addr), (val))
#define	outl(addr, val)	(*alpha_pci_io_switch->apio_outl)((addr), (val))

int	alpha_pci_io_enable(int);
#endif /* _KERNEL */

#endif /* _ALPHA_PIO_H_ */
