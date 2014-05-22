/*	$NetBSD: msiiepvar.h,v 1.8.2.1 2014/05/22 11:40:09 yamt Exp $ */

/*
 * Copyright (c) 2001 Valeriy E. Ushakov
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
 *    derived from this software without specific prior written permission
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

#ifndef _SPARC_MSIIEP_VAR_H_
#define _SPARC_MSIIEP_VAR_H_

struct msiiep_attach_args {
	const char *msa_name;
	struct mainbus_attach_args *msa_ma;
};

struct mspcic_softc {
	/* PROM node */
	int			sc_node;

	/* parent (mainbus) tags */
	bus_space_tag_t		sc_bustag;
	bus_dma_tag_t		sc_dmatag;

	/* our tags */
	bus_space_tag_t		sc_memt;
	bus_space_tag_t		sc_iot;
	bus_dma_tag_t		sc_dmat;
};

extern int	mspcic_assigned_interrupt(int);

/*
 * Bus space tags for memory and i/o.
 */

struct mspcic_pci_map {
	uint32_t sysbase;
	uint32_t pcibase;
	uint32_t size;
};

#define IOMAP_SIZE 2
#define MEMMAP_SIZE 3

#endif /* _SPARC_MSIIEP_VAR_H_ */
