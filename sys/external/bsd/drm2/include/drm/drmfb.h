/*	$NetBSD: drmfb.h,v 1.2.14.2 2017/12/03 11:37:59 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#ifndef	_DRM_DRMFB_H_
#define	_DRM_DRMFB_H_

#include <sys/device_if.h>

#include <dev/wsfb/genfbvar.h>

struct drm_fb_helper;
struct drm_fb_helper_surface_sizes;

struct drmfb_attach_args;
struct drmfb_params;
struct drmfb_softc;

struct drmfb_params {
	/*
	 * Framebuffer mmap.
	 */
	paddr_t		(*dp_mmapfb)(struct drmfb_softc *, off_t, int);

	/*
	 * Other mmap, e.g. for arbitrary PCI BARs.  Available only to
	 * users with privileges for access to unmanaged memory.
	 */
	paddr_t		(*dp_mmap)(struct drmfb_softc *, off_t, int);

	/*
	 * Ioctl handler.  Must handle:
	 *
	 * - WSDISPLAYIO_GET_BUSID
	 * - WSDISPLAYIO_GTYPE
	 *
	 * May add or override anything else.  Return EPASSTHROUGH to
	 * defer.
	 */
	int		(*dp_ioctl)(struct drmfb_softc *, unsigned long,
			    void *, int, struct lwp *);
	/* XXX Kludge!  */
	bool		(*dp_is_vga_console)(struct drm_device *);
	void		(*dp_disable_vga)(struct drm_device *);
};

struct drmfb_attach_args {
	device_t			da_dev;
	struct drm_fb_helper		*da_fb_helper;
	const struct drm_fb_helper_surface_size	*da_fb_sizes;
	void				*da_fb_vaddr;
	uint32_t			da_fb_linebytes;
	const struct drmfb_params	*da_params;
};

/* Treat as opaque.  Must be first member of device softc.  */
struct drmfb_softc {
	struct genfb_softc		sc_genfb; /* XXX Must be first. */
	struct drmfb_attach_args	sc_da;
};

int	drmfb_attach(struct drmfb_softc *, const struct drmfb_attach_args *);
int	drmfb_detach(struct drmfb_softc *, int);
bool	drmfb_shutdown(struct drmfb_softc *, int);

#endif	/* _DRM_DRMFB_H_ */
