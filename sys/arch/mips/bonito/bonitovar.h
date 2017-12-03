/*	$NetBSD: bonitovar.h,v 1.5.12.1 2017/12/03 11:36:26 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#ifndef _MIPS_BONITO_BONITOVAR_H_
#define	_MIPS_BONITO_BONITOVAR_H_

#include <dev/pci/pcivar.h>

#include <mips/cpuregs.h>

struct bonito_config {
	int		bc_adbase;	/* AD line base for config access */

	/* Prototype interrupt and GPIO registers. */
	uint32_t	bc_gpioIE;
	uint32_t	bc_intEdge;
	uint32_t	bc_intSteer;
	uint32_t	bc_intPol;

	/* PCI Attach hook , if needed */
	void		(*bc_attach_hook)(device_t, device_t,
			    struct pcibus_attach_args *);
		 
};

#ifdef _KERNEL
void	bonito_pci_init(pci_chipset_tag_t, const struct bonito_config *);

void	bonito_iobc_wbinv_range(paddr_t, psize_t);
void	bonito_iobc_inv_range(paddr_t, psize_t);
#endif /* _KERNEL */

#endif /* _MIPS_BONITO_BONITOVAR_H_ */
