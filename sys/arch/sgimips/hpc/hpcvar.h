/*	$NetBSD: hpcvar.h,v 1.4 2002/03/13 13:12:27 simonb Exp $	*/

/*
 * Copyright (c) 2001 Rafal K. Boni
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARCH_SGIMIPS_HPC_HPCVAR_H_
#define	_ARCH_SGIMIPS_HPC_HPCVAR_H_

struct hpc_attach_args {
	const char		*ha_name;	/* name of device */
	bus_addr_t		ha_devoff;	/* offset of device */
	bus_addr_t		ha_dmaoff;	/* offset of DMA regs */
	int			ha_irq;		/* interrupt line */

	bus_space_tag_t		ha_st;		/* HPC space tag */
	bus_space_handle_t	ha_sh;		/* HPC space handle XXX */
	bus_dma_tag_t		ha_dmat;	/* HPC DMA tag */
};

#endif	/* _ARCH_SGIMIPS_HPC_HPCVAR_H_ */
