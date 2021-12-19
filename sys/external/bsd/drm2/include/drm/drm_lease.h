/*	$NetBSD: drm_lease.h,v 1.3 2021/12/19 01:08:07 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#ifndef _DRM_DRM_LEASE_H_
#define	_DRM_DRM_LEASE_H_

#include <sys/types.h>

struct drm_device;
struct drm_file;
struct drm_master;

int	drm_mode_create_lease_ioctl(struct drm_device *, void *,
	    struct drm_file *);
int	drm_mode_get_lease_ioctl(struct drm_device *, void *,
	    struct drm_file *);
int	drm_mode_list_lessees_ioctl(struct drm_device *, void *,
	    struct drm_file *);
int	drm_mode_revoke_lease_ioctl(struct drm_device *, void *,
	    struct drm_file *);

struct drm_master *drm_lease_owner(struct drm_master *);
bool	drm_lease_held(struct drm_file *, int);
bool	_drm_lease_held(struct drm_file *, int);
void	drm_lease_revoke(struct drm_master *);
void	drm_lease_destroy(struct drm_master *);
uint32_t drm_lease_filter_crtcs(struct drm_file *, uint32_t);

#endif	/* _DRM_DRM_LEASE_H_ */
