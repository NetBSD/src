/*	$NetBSD: pnpbusvar.h,v 1.1.10.1 2006/06/19 03:45:06 chap Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#ifndef _PREP_PNPBUSVAR_H_
#define _PREP_PNPBUSVAR_H_

#include <sys/queue.h>
#include <machine/bus.h>
#include <machine/residual.h>

struct pnpbus_mem {
	SIMPLEQ_ENTRY(pnpbus_mem) next;
	uint32_t minbase, maxbase, align, len;
	int flags;
};
struct pnpbus_io {
	SIMPLEQ_ENTRY(pnpbus_io) next;
	uint16_t minbase, maxbase, align, len;
	int flags;
};
struct pnpbus_irq {
	SIMPLEQ_ENTRY(pnpbus_irq) next;
	uint16_t mask;
	int flags;
};
struct pnpbus_dma {
	SIMPLEQ_ENTRY(pnpbus_dma) next;
	uint8_t mask;
	int flags;
};
struct pnpbus_compatid {
	char idstr[8];
	struct pnpbus_compatid *next;
};

#define PNPBUS_MAXMEM 4
#define PNPBUS_MAXIOPORT 8
#define PNPBUS_MAXIRQ 2
#define PNPBUS_MAXDMA 2

struct pnpresources {
	int nummem, numio, numirq, numdma;
	SIMPLEQ_HEAD(, pnpbus_mem) mem;
	SIMPLEQ_HEAD(, pnpbus_io) io;
	SIMPLEQ_HEAD(, pnpbus_irq) irq;
	SIMPLEQ_HEAD(, pnpbus_dma) dma;
	struct pnpbus_compatid *compatids;
};

/*
 * Bus attach arguments
 */
struct pnpbus_attach_args {
	bus_space_tag_t paa_iot;		/* i/o space tag */
	bus_space_tag_t paa_memt;		/* mem space tag */
	isa_chipset_tag_t paa_ic;		/* ISA chipset tag */
};

/*
 * driver attach arguments
 */
struct pnpbus_dev_attach_args {
	bus_space_tag_t pna_iot;	/* i/o space tag */
	bus_space_tag_t pna_memt;	/* mem space tag */
	isa_chipset_tag_t pna_ic;	/* ISA chipset tag */

	struct pnpresources pna_res;	/* resources gathered from PNP */
	char	pna_devid[8];		/* PNP device id string */
	PPC_DEVICE *pna_ppc_dev;	/* PNP device entry */
	void	*pna_aux;		/* driver specific */
	uint8_t	basetype;		/* PNP base type */
	uint8_t subtype;		/* PNP subtype */
	uint8_t interface;		/* PNP interface */
	uint8_t spare;			/* struct packing */
};

/*
 * master bus
 */
struct pnpbus_softc {
	struct	device sc_dev;		/* base device */
	isa_chipset_tag_t sc_ic;

	bus_space_tag_t sc_iot;		/* io space tag */
	bus_space_tag_t sc_memt;	/* mem space tag */
};

int	pnpbus_scan(struct pnpbus_dev_attach_args *pna, PPC_DEVICE *dev);
void	pnpbus_print_devres(struct pnpbus_dev_attach_args *pna);
void	*pnpbus_intr_establish(int idx, int level, int (*ih_fun)(void *),
	    void *ih_arg, struct pnpresources *r);
void	pnpbus_intr_disestablish(void *arg);
int	pnpbus_getirqnum(struct pnpresources *r, int idx, int *irqp, int *istp);
int	pnpbus_getdmachan(struct pnpresources *r, int idx, int *chanp);
int	pnpbus_getioport(struct pnpresources *r, int idx, int *basep,
	    int *sizep);
int	pnpbus_io_map(struct pnpresources *r, int idx, bus_space_tag_t *tagp,
	    bus_space_handle_t *hdlp);
void	pnpbus_io_unmap(struct pnpresources *r, int idx, bus_space_tag_t tag,
	    bus_space_handle_t hdl);

#endif /* _PREP_PNPBUSVAR_H_ */
