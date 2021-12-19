/* $NetBSD: ti_lcdc.h,v 1.3 2021/12/19 12:44:57 riastradh Exp $ */

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

#ifndef _ARM_TI_TI_LCDC_H
#define _ARM_TI_TI_LCDC_H

#include <sys/workqueue.h>

#include <drm/drm_encoder.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>

#define DRIVER_AUTHOR		"NetBSD"

#define DRIVER_NAME		"tilcdc"
#define DRIVER_DESC		"TI LCDC"
#define DRIVER_DATE		"20191103"

#define DRIVER_MAJOR		1
#define DRIVER_MINOR		0
#define DRIVER_PATCHLEVEL	0

struct tilcdc_softc;
struct tilcdc_framebuffer;

struct tilcdc_vblank {
	void			*priv;
	void			(*enable_vblank)(void *);
	void			(*disable_vblank)(void *);
	uint32_t		(*get_vblank_counter)(void *);
};

struct tilcdc_crtc {
	struct drm_crtc		base;
	struct tilcdc_softc	*sc;
};

struct tilcdc_encoder {
	struct drm_encoder	base;
	struct tilcdc_softc	*sc;
};

struct tilcdc_softc {
	device_t		sc_dev;
	struct drm_device	*sc_ddev;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;

	struct clk		*sc_clk;
	int			sc_phandle;

	struct lwp			*sc_task_thread;
	SIMPLEQ_HEAD(, tilcdc_drm_task)	sc_tasks;
	struct workqueue		*sc_task_wq;

	bool			sc_dev_registered;

	struct tilcdc_crtc	sc_crtc;
	struct tilcdc_encoder	sc_encoder;
	struct tilcdc_vblank	sc_vbl;

	struct fdt_device_ports	sc_ports;
};

struct tilcdc_framebuffer {
	struct drm_framebuffer	base;
	struct drm_gem_cma_object *obj;
};

struct tilcdc_fbdev {
	struct drm_fb_helper	helper;
};

struct tilcdcfb_attach_args {
	struct drm_device	*tfa_drm_dev;
	struct drm_fb_helper	*tfa_fb_helper;
	struct drm_fb_helper_surface_size tfa_fb_sizes;
	bus_space_tag_t		tfa_fb_bst;
	bus_dma_tag_t		tfa_fb_dmat;
	uint32_t		tfa_fb_linebytes;
};

struct tilcdc_drm_task {
	union {
		SIMPLEQ_ENTRY(tilcdc_drm_task)	queue;
		struct work			work;
	}		tdt_u;
	void		(*tdt_fn)(struct tilcdc_drm_task *);
};

#define tilcdc_private(ddev)		(ddev)->dev_private
#define	to_tilcdc_framebuffer(x)	container_of(x, struct tilcdc_framebuffer, base)
#define	to_tilcdc_crtc(x)		container_of(x, struct tilcdc_crtc, base)

#define	RD4(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

void	tilcdc_task_init(struct tilcdc_drm_task *,
	    void (*)(struct tilcdc_drm_task *));
void	tilcdc_task_schedule(device_t, struct tilcdc_drm_task *);

#endif /* _ARM_TI_TI_LCDC_H */
