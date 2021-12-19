/*	$NetBSD: drm_writeback.c,v 1.1 2021/12/19 10:46:02 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_writeback.c,v 1.1 2021/12/19 10:46:02 riastradh Exp $");

#include <drm/drm_writeback.h>

struct drm_writeback_connector *
drm_connector_to_writeback(struct drm_connector *connector)
{

	return container_of(connector, struct drm_writeback_connector, base);
}

struct dma_fence *
drm_writeback_get_out_fence(struct drm_writeback_connector *wbconn)
{
	panic("writeback connectors not implemented");
	return NULL;
}

int
drm_writeback_prepare_job(struct drm_writeback_job *wbj)
{
	panic("writeback connectors not implemented");
	return -ENXIO;
}

void
drm_writeback_cleanup_job(struct drm_writeback_job *job)
{
	panic("writeback connectors not implemented");
}

int
drm_writeback_set_fb(struct drm_connector_state *state,
    struct drm_framebuffer *fb)
{
	panic("writeback connectors not implemented");
	return -ENXIO;
}
