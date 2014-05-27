/*	$NetBSD: agp_i810var.h,v 1.4 2014/05/27 03:17:33 riastradh Exp $	*/

/*-
 * Copyright (c) 2000 Doug Rabson
 * Copyright (c) 2000 Ruslan Ermilov
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

#include <sys/types.h>
#include <sys/bus.h>

#include <dev/pci/pcivar.h>

#include <dev/pci/agpvar.h>

struct agp_i810_softc {
	struct pci_attach_args vga_pa;	/* integrated graphics device args */
	int chiptype;			/* chipset family: i810, i830, &c. */

	/* Memory-mapped I/O for device registers.  */
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t size;

	/* Graphics translation table.  */
	bus_space_tag_t gtt_bst;
	bus_space_handle_t gtt_bsh;
	bus_size_t gtt_size;

	/* Chipset flush page.  */
	bus_space_tag_t flush_bst;
	bus_space_handle_t flush_bsh;
	bus_addr_t flush_addr;

	uint32_t initial_aperture;	/* aperture size at startup */
	struct agp_gatt *gatt;		/* AGP graphics addr. trans. tbl. */
	uint32_t dcache_size;		/* i810-only on-chip memory size */
	uint32_t stolen;		/* num. GTT entries for stolen mem. */

	uint32_t pgtblctl;		/* saved PGTBL_CTL?  XXX unused */
};

extern struct agp_softc	*agp_i810_sc;

int	agp_i810_write_gtt_entry(struct agp_i810_softc *, off_t, bus_addr_t);
void	agp_i810_post_gtt_entry(struct agp_i810_softc *, off_t);
void	agp_i810_chipset_flush(struct agp_i810_softc *);
