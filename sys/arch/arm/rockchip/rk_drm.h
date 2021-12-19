/* $NetBSD: rk_drm.h,v 1.2 2021/12/19 12:28:27 riastradh Exp $ */

/*-
 * Copyright (c) 2019 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_RK_DRM_H
#define _ARM_RK_DRM_H

#include <sys/workqueue.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>

#define DRIVER_AUTHOR		"Jared McNeill"

#define DRIVER_NAME		"rk"
#define DRIVER_DESC		"Rockchip Display Subsystem"
#define DRIVER_DATE		"20191109"

#define DRIVER_MAJOR		1
#define DRIVER_MINOR		0
#define DRIVER_PATCHLEVEL	0

struct rk_framebuffer;

#define	RK_DRM_MAX_CRTC	2

struct rk_drm_vblank {
	void			*priv;
	void			(*enable_vblank)(void *);
	void			(*disable_vblank)(void *);
	uint32_t		(*get_vblank_counter)(void *);
};

struct rk_drm_softc {
	device_t		sc_dev;
	struct drm_device	*sc_ddev;

	bus_space_tag_t		sc_bst;
	bus_dma_tag_t		sc_dmat;

	int			sc_phandle;

	struct lwp			*sc_task_thread;
	SIMPLEQ_HEAD(, rk_drm_task)	sc_tasks;
	struct workqueue		*sc_task_wq;

	bool			sc_dev_registered;

	struct rk_drm_vblank	sc_vbl[RK_DRM_MAX_CRTC];
};

struct rk_drm_framebuffer {
	struct drm_framebuffer	base;
	struct drm_gem_cma_object *obj;
};

struct rk_drm_ports {
	int			phandle;
	struct fdt_device_ports	*port;
	struct drm_device	*ddev;
	TAILQ_ENTRY(rk_drm_ports) entries;
};

struct rk_drm_fbdev {
	struct drm_fb_helper	helper;
};

struct rk_drmfb_attach_args {
	struct drm_device	*sfa_drm_dev;
	struct drm_fb_helper	*sfa_fb_helper;
	struct drm_fb_helper_surface_size sfa_fb_sizes;
	bus_space_tag_t		sfa_fb_bst;
	bus_dma_tag_t		sfa_fb_dmat;
	uint32_t		sfa_fb_linebytes;
};

struct rk_drm_task {
	union {
		SIMPLEQ_ENTRY(rk_drm_task)	queue;
		struct work			work;
	}		rdt_u;
	void		(*rdt_fn)(struct rk_drm_task *);
};

#define rk_drm_private(ddev)		(ddev)->dev_private
#define	to_rk_drm_framebuffer(x)	container_of(x, struct rk_drm_framebuffer, base)

int	rk_drm_register_port(int, struct fdt_device_ports *);
struct drm_device *rk_drm_port_device(struct fdt_device_ports *);

void	rk_task_init(struct rk_drm_task *,
	    void (*)(struct rk_drm_task *));
void	rk_task_schedule(device_t, struct rk_drm_task *);

#endif /* _ARM_RK_DRM_H */
