/*	$NetBSD: intel_ringbuffer.c,v 1.1.2.2 2013/07/24 04:02:12 riastradh Exp $	*/

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

/* intel_ringbuffer.c stubs */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intel_ringbuffer.c,v 1.1.2.2 2013/07/24 04:02:12 riastradh Exp $");

#include "i915_drv.h"

void
intel_cleanup_ring_buffer(struct intel_ring_buffer *ring __unused)
{
}

int
intel_render_ring_init_dri(struct drm_device *dev __unused, u64 start __unused,
    u32 size __unused)
{
	return 0;
}

void
intel_ring_advance(struct intel_ring_buffer *ring __unused)
{
}

int
intel_ring_begin(struct intel_ring_buffer *ring __unused, int n __unused)
{
	return ENOTTY;
}

/*
 * XXX Kludge!  Copypasta from intel_ringbuffer.c until we can use the
 * whole thing.
 */

u32
intel_ring_get_active_head(struct intel_ring_buffer *ring)
{
	drm_i915_private_t *dev_priv = ring->dev->dev_private;
	u32 acthd_reg = INTEL_INFO(ring->dev)->gen >= 4 ?
			RING_ACTHD(ring->mmio_base) : ACTHD;

	return I915_READ(acthd_reg);
}

int
intel_ring_idle(struct intel_ring_buffer *ring __unused)
{
	return 0;
}

void
intel_ring_setup_status_page(struct intel_ring_buffer *ring __unused)
{
}
