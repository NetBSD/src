/*	$NetBSD: rmixlvar.h,v 1.1.2.4 2009/09/25 22:21:12 cliff Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <dev/pci/pcivar.h>
#include <machine/bus.h>

typedef enum {
	RMIXL_INTR_EDGE=0,
	RMIXL_INTR_LEVEL,
} rmixl_intr_trigger_t;

typedef enum {
	RMIXL_INTR_RISING=0,
	RMIXL_INTR_HIGH,
	RMIXL_INTR_FALLING,
	RMIXL_INTR_LOW,
} rmixl_intr_polarity_t;

struct rmixl_config {
	uint64_t		 rc_io_pbase;	
	struct mips_bus_space	 rc_el_memt; 	/* little endian bus */
	struct mips_bus_space	 rc_eb_memt; 	/* big    endian bus */
	struct mips_bus_dma_tag	 rc_lt4G_dmat; 	/* space < 4GB */
	struct mips_bus_dma_tag	 rc_ge4G_dmat; 	/* space < memsize */
	struct mips_pci_chipset	 rc_pc; 
	struct extent		*rc_io_ex;
	struct extent		*rc_mem_ex; 
	int			 rc_mallocsafe;
};

extern struct rmixl_config rmixl_configuration;

extern void rmixl_el_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_eb_bus_mem_init(bus_space_tag_t, void *);

extern void *rmixl_intr_establish(int, int,
	rmixl_intr_trigger_t, rmixl_intr_polarity_t,
	int (*)(void *), void *);
extern void  rmixl_intr_disestablish(void *);
