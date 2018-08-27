/*	$NetBSD: intel-gtt.h,v 1.6 2018/08/27 07:17:01 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _DRM_INTEL_GTT_H_
#define _DRM_INTEL_GTT_H_

#include <sys/types.h>
#include <sys/bus.h>

struct pci_dev;
struct agp_bridge_data;

void	intel_gtt_get(size_t * /* GPU VA size in bytes */,
	    size_t * /* graphics stolen memory in bytes */,
	    bus_addr_t * /* aperture base */,
	    unsigned long * /* aperture size in bytes */);
int	intel_gmch_probe(struct pci_dev *, struct pci_dev *,
	    struct agp_bridge_data *);
void	intel_gmch_remove(void);
bool	intel_enable_gtt(void);
void	intel_gtt_chipset_flush(void);
void	intel_gtt_insert_sg_entries(bus_dmamap_t, unsigned, unsigned);
void	intel_gtt_clear_range(unsigned, unsigned);

#define	AGP_USER_MEMORY		1
#define	AGP_USER_CACHED_MEMORY	2

extern int	intel_iommu_gfx_mapped;

#endif  /* _DRM_INTEL_GTT_H_ */
