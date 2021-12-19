/*	$NetBSD: drm_client.h,v 1.1 2021/12/19 10:36:32 riastradh Exp $	*/

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

#ifndef _DRM_DRM_CLIENT_H_
#define	_DRM_DRM_CLIENT_H_

#include <linux/mutex.h>

struct drm_client_buffer;
struct drm_client_dev;
struct drm_client_funcs;
struct drm_device;
struct drm_mode_set;

struct drm_client_dev {
	struct mutex			modeset_mutex;
	struct drm_mode_set		*modesets;
	struct drm_framebuffer		*fb;
	struct drm_device		*dev;
	const struct drm_client_funcs	*funcs;
};

struct drm_client_funcs {
	struct module *owner;
	void	(*unregister)(struct drm_client_dev *);
	int	(*restore)(struct drm_client_dev *);
	int	(*hotplug)(struct drm_client_dev *);
};

struct drm_client_buffer {
	struct drm_framebuffer	*fb;
};

int	drm_client_init(struct drm_device *, struct drm_client_dev *,
	    const char *, const struct drm_client_funcs *);
void	drm_client_register(struct drm_client_dev *);
void	drm_client_release(struct drm_client_dev *);

void	drm_client_dev_hotplug(struct drm_device *);
void	drm_client_dev_restore(struct drm_device *);
void	drm_client_dev_unregister(struct drm_device *);

struct drm_client_buffer *
	drm_client_framebuffer_create(struct drm_client_dev *, u32, u32, u32);
void	*drm_client_buffer_vmap(struct drm_client_buffer *);
void	drm_client_buffer_vunmap(struct drm_client_buffer *);
void	drm_client_framebuffer_delete(struct drm_client_buffer *);

int	drm_client_modeset_create(struct drm_client_dev *);
int	drm_client_modeset_probe(struct drm_client_dev *, unsigned, unsigned);
bool	drm_client_rotation(struct drm_mode_set *, unsigned *);
int	drm_client_modeset_commit_force(struct drm_client_dev *);
int	drm_client_modeset_commit(struct drm_client_dev *);
int	drm_client_modeset_dpms(struct drm_client_dev *, int);
void	drm_client_modeset_free(struct drm_client_dev *);

#define drm_client_for_each_modeset(MODESET, CLIENT)		   \
	KASSERT(mutex_is_locked(&(CLIENT)->modeset_mutex));	   \
	for ((MODESET) = (CLIENT)->modesets; (MODESET)->crtc; (MODESET)++)

#define drm_client_for_each_connector_iter(CONNECTOR, ITER)		      \
	drm_for_each_connector_iter(CONNECTOR, ITER)			      \
		if ((CONNECTOR)->connector_type !=			      \
		    DRM_MODE_CONNECTOR_WRITEBACK)

#endif	/* _DRM_DRM_CLIENT_H_ */
