/*	$NetBSD: drm_agp_netbsd.h,v 1.3.4.3 2017/12/03 11:37:59 jdolecek Exp $	*/

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

#ifndef _DRM_DRM_AGP_NETBSD_H_
#define _DRM_DRM_AGP_NETBSD_H_

/*
 * XXX XXX XXX BEWARE!  This file is full of abstraction violations
 * that are ruthlessly exploited for their value as happy accidents!
 * You have been warned!
 */

#include <sys/agpio.h>

#include <dev/pci/pcivar.h>	/* XXX include order botch */
#include <dev/pci/agpreg.h>
#include <dev/pci/agpvar.h>

#include <linux/kernel.h>
#include <linux/pci.h>

#define	PCI_AGP_COMMAND_FW	AGPCMD_FWEN

#if defined(__i386__) || defined(__x86_64__)
#if defined(_KERNEL_OPT)
#include "agp.h"
#else
#define NAGP 1
#endif
#if NAGP > 0
#define	__OS_HAS_AGP	1
#endif
__CTASSERT(PAGE_SIZE == AGP_PAGE_SIZE);
__CTASSERT(PAGE_SHIFT == AGP_PAGE_SHIFT);
#endif

struct agp_kern_info {
	struct agp_info aki_info;
};

struct agp_bridge_data {
	struct agp_softc abd_sc; /* XXX Abstraction violation! */
};

/*
 * We already have a struct agp_memory, but fortunately it looks like
 * it may accidentally work out.
 */

#if 0
struct agp_memory {
	void *am_cookie;
};
#endif

static inline struct agp_bridge_data *
agp_find_bridge(struct pci_dev *pdev __unused)
{
	/*
	 * XXX How do we find the agp bridge attached to this
	 * particular PCI device?
	 */
	return container_of((struct agp_softc *)agp_find_device(0),
	    struct agp_bridge_data, abd_sc);
}

static inline struct agp_bridge_data *
agp_backend_acquire(struct pci_dev *pdev)
{
	struct agp_bridge_data *const bridge = agp_find_bridge(pdev);

	if (bridge == NULL)
		return NULL;

	/* XXX We lose the error code here.  */
	if (agp_acquire(&bridge->abd_sc) != 0)
		return NULL;

	return bridge;
}

static inline void
agp_backend_release(struct agp_bridge_data *bridge)
{

	/* XXX We lose the error code here.  */
	(void)agp_release(&bridge->abd_sc);
}

/*
 * Happily, agp_enable will accidentally DTRT as is in NetBSD in spite
 * of the name collision, thanks to a curious `void *' argument in its
 * declaration...
 */

#if 0
static inline void
agp_enable(struct agp_bridge_data *bridge)
{
	...
}
#endif

static inline struct agp_memory *
agp_allocate_memory(struct agp_bridge_data *bridge, size_t npages,
    uint32_t type)
{
	return agp_alloc_memory(&bridge->abd_sc, (npages << AGP_PAGE_SHIFT),
	    type);
}

/*
 * Once again, a happy accident makes agp_free_memory work out.
 */

#if 0
static inline void
agp_free_memory(struct agp_bridge_data *bridge, struct agp_memory *mem)
{
	...
}
#endif

/*
 * Unfortunately, Linux's agp_bind_memory doesn't require the agp
 * device as an argument.  So we'll have to kludge that up as we go.
 */
#if 0
static inline void
agp_bind_memory(struct agp_memory *mem, size_t npages)
{
	agp_bind_memory(???, mem, (npages << AGP_PAGE_SHIFT));
}
#endif

static inline void
agp_copy_info(struct agp_bridge_data *bridge, struct agp_kern_info *info)
{
	agp_get_info(bridge, &info->aki_info);
}

static inline int
drm_bind_agp(struct agp_bridge_data *bridge, struct agp_memory *mem,
    unsigned npages)
{
	return agp_bind_memory(&bridge->abd_sc, mem,
	    ((size_t)npages << AGP_PAGE_SHIFT));
}

static inline int
drm_unbind_agp(struct agp_bridge_data *bridge, struct agp_memory *mem)
{
	return agp_unbind_memory(&bridge->abd_sc, mem);
}

static inline void
drm_free_agp(struct agp_bridge_data *bridge, struct agp_memory *mem,
    int npages __unused)
{
	agp_free_memory(&bridge->abd_sc, mem);
}

#endif  /* _DRM_DRM_AGP_NETBSD_H_ */
