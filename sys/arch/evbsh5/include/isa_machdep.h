/*	$NetBSD: isa_machdep.h,v 1.1 2002/07/05 13:31:46 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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

#ifndef _EVBSH5_ISA_MACHDEP_H
#define _EVBSH5_ISA_MACHDEP_H

/*
 * Types provided to machine-independent ISA code
 */
typedef void *isa_chipset_tag_t;

/*
 * Functions provided to machine-independent ISA code.
 */
struct device;
struct isabus_attach_args;

extern void isa_attach_hook(struct device *, struct device *,
	    struct isabus_attach_args *);
extern int isa_intr_alloc(isa_chipset_tag_t, int, int, int *);
extern const struct evcnt *isa_intr_evcnt(isa_chipset_tag_t, int);
extern void *isa_intr_establish(isa_chipset_tag_t, int, int,
	    int, int (*)(void *), void *);
extern void isa_intr_disestablish(isa_chipset_tag_t, void *);

/*
 * We don't support ISA dma, so these are just dummy stubs.
 */
struct isa_dma_state;
extern void isa_dmainit(struct isa_dma_state *, bus_space_tag_t,
	    bus_dma_tag_t, struct device *);
extern bus_size_t isa_dmamaxsize(struct isa_dma_state *, int);
extern int isa_dmamap_create(struct isa_dma_state *, int, bus_size_t, int);
extern int isa_dmastart(struct isa_dma_state *, int, void *,
	    bus_size_t, struct proc *, int, int);
extern int isa_dmadone(struct isa_dma_state *, int);

#endif /* _EVBSH5_ISA_MACHDEP_H */
