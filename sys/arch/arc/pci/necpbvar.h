/*	$NetBSD: necpbvar.h,v 1.6.74.1 2008/09/28 10:39:47 mjf Exp $	*/

/*-
 * Copyright (C) 2000 Shuichiro URATA.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

struct necpb_context {
	int	nc_initialized;
	struct arc_bus_space nc_memt;
	struct arc_bus_space nc_iot;
	struct arc_bus_dma_tag nc_dmat;
	struct arc_pci_chipset nc_pc;
};

struct necpb_softc {
	device_t sc_dev;
	struct necpb_context *sc_ncp;
};

struct necpb_intrhand {
	int	(*ih_func)(void *);		/* interrupt handler */
	void	*ih_arg;			/* arg for handler */
	struct	necpb_intrhand *ih_next;	/* next intrhand chain */
	int	ih_intn;			/* interrupt channel */
	struct evcnt ih_evcnt;			/* interrupt counter */
	char ih_evname[32];			/* event counter name */
};

void	necpb_init(struct necpb_context *ncp);

/* for console initialization */
extern struct necpb_context necpb_main_context;
