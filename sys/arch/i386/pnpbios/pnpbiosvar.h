/* $NetBSD: pnpbiosvar.h,v 1.10.92.1 2009/05/13 17:17:51 jym Exp $ */
/*
 * Copyright (c) 1999
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/device.h>		/* for device_t */

struct pnpbios_attach_args {
	isa_chipset_tag_t paa_ic;
};

int pnpbios_probe(void);

struct pnp_compatid {
	char idstr[8];
	struct pnp_compatid *next;
};

struct pnp_mem {
	SIMPLEQ_ENTRY(pnp_mem) next;
	uint32_t minbase, maxbase, align, len;
	int flags;
};
struct pnp_io {
	SIMPLEQ_ENTRY(pnp_io) next;
	uint16_t minbase, maxbase, align, len;
	int flags;
};
struct pnp_irq {
	SIMPLEQ_ENTRY(pnp_irq) next;
	uint16_t mask;
	int flags;
};
struct pnp_dma {
	SIMPLEQ_ENTRY(pnp_dma) next;
	uint8_t mask;
	int flags;
};

#define PNP_MAXMEM 4
#define PNP_MAXIOPORT 8
#define PNP_MAXIRQ 2
#define PNP_MAXDMA 2

struct pnpresources {
	int nummem, numio, numirq, numdma;
	SIMPLEQ_HEAD(, pnp_mem) mem;
	SIMPLEQ_HEAD(, pnp_io) io;
	SIMPLEQ_HEAD(, pnp_irq) irq;
	SIMPLEQ_HEAD(, pnp_dma) dma;
	struct pnpresources *dependant_link;
	struct pnp_compatid *compatids;
	char *longname;
};

typedef void *pnpbios_tag_t; /* driver private */

struct pnpbiosdev_attach_args {
	pnpbios_tag_t pbt;
	isa_chipset_tag_t ic;
	int idx;
	struct pnpresources *resc;
	char *idstr;
	char *primid;
};

int pnpbios_io_map(pnpbios_tag_t, struct pnpresources *, int,
			bus_space_tag_t *, bus_space_handle_t *);
void pnpbios_io_unmap(pnpbios_tag_t, struct pnpresources *, int,
			bus_space_tag_t, bus_space_handle_t);
void *pnpbios_intr_establish(pnpbios_tag_t, struct pnpresources *, int,
				  int, int (*)(void *), void *);

int pnpbios_getiobase(pnpbios_tag_t, struct pnpresources *, int,
			   bus_space_tag_t *, int *);
int pnpbios_getiosize(pnpbios_tag_t, struct pnpresources *, int, int *);
int pnpbios_getirqnum(pnpbios_tag_t, struct pnpresources *, int, int *, int *);
int pnpbios_getdmachan(pnpbios_tag_t, struct pnpresources *, int, int *);
void pnpbios_print_devres(device_t, struct pnpbiosdev_attach_args *);
