/* $NetBSD: anx_dp.h,v 1.3 2021/12/19 11:00:47 riastradh Exp $ */

/*-
 * Copyright (c) 2019 Jonathan A. Kollasch <jakllsch@kollasch.net>
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

#ifndef _DEV_IC_ANXDP_H
#define _DEV_IC_ANXDP_H

#if ANXDP_AUDIO
#include <dev/audio/audio_dai.h>
#endif

#include <drm/drm_drv.h>
#include <drm/drm_bridge.h>
#include <drm/drm_connector.h>
#include <drm/drm_dp_helper.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct anxdp_softc;

struct anxdp_connector {
	struct drm_connector	base;
	struct anxdp_softc	*sc;
#if ANXDP_AUDIO

	bool			monitor_audio;
#endif
};

struct anxdp_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	u_int			sc_flags;
#define	ANXDP_FLAG_ROCKCHIP	__BIT(0)

#if ANXDP_AUDIO
	struct audio_dai_device	sc_dai;
	uint8_t			sc_swvol;

#endif
	struct anxdp_connector	sc_connector;
	struct drm_bridge	sc_bridge;
	struct drm_dp_aux       sc_dpaux;
	struct drm_panel *	sc_panel;
	uint8_t			sc_dpcd[DP_RECEIVER_CAP_SIZE];

	struct drm_display_mode	sc_curmode;
};

#define	to_anxdp_connector(x)	container_of(x, struct anxdp_connector, base)

int		anxdp_attach(struct anxdp_softc *);
int		anxdp_bind(struct anxdp_softc *, struct drm_encoder *);

void		anxdp_dpms(struct anxdp_softc *, int);

#endif /* !_DEV_IC_ANXDP_H */
