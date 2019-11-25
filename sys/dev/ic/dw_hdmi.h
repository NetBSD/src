/* $NetBSD: dw_hdmi.h,v 1.1.6.2 2019/11/25 16:18:40 martin Exp $ */

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

#ifndef _DEV_IC_DWHDMI_H
#define _DEV_IC_DWHDMI_H

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcreg.h>

#include <dev/audio/audio_dai.h>

#include <drm/drmP.h>

struct dwhdmi_softc;

struct dwhdmi_connector {
	struct drm_connector	base;
	struct dwhdmi_softc	*sc;

	bool			hdmi_monitor;
	bool			monitor_audio;
};

struct dwhdmi_phy_config {
	u_int			pixel_clock;
	uint32_t		sym;
	uint32_t		term;
	uint32_t		vlev;
};

struct dwhdmi_mpll_config {
	u_int			pixel_clock;
	uint32_t		cpce;
	uint32_t		gmp;
	uint32_t		curr;
};

struct dwhdmi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	u_int			sc_reg_width;
	u_int			sc_flags;
#define	DWHDMI_USE_INTERNAL_PHY	__BIT(0)
	u_int			sc_scl_hcnt;
	u_int			sc_scl_lcnt;

	u_int			sc_phytype;
	u_int			sc_version;

	i2c_tag_t		sc_ic;
	kmutex_t		sc_ic_lock;
	struct i2c_controller	sc_ic_builtin;

	struct audio_dai_device	sc_dai;
	uint8_t			sc_swvol;

	struct dwhdmi_connector	sc_connector;
	struct drm_bridge	sc_bridge;

	struct drm_display_mode	sc_curmode;

	const struct dwhdmi_mpll_config *sc_mpll_config;
	const struct dwhdmi_phy_config *sc_phy_config;

	enum drm_connector_status (*sc_detect)(struct dwhdmi_softc *, bool);
	void			(*sc_enable)(struct dwhdmi_softc *);
	void			(*sc_disable)(struct dwhdmi_softc *);
	void			(*sc_mode_set)(struct dwhdmi_softc *,
					       struct drm_display_mode *,
					       struct drm_display_mode *);
};

#define	to_dwhdmi_connector(x)	container_of(x, struct dwhdmi_connector, base)

int		dwhdmi_attach(struct dwhdmi_softc *);
int		dwhdmi_bind(struct dwhdmi_softc *, struct drm_encoder *);

uint8_t		dwhdmi_read(struct dwhdmi_softc *, bus_size_t);
void		dwhdmi_write(struct dwhdmi_softc *, bus_size_t, uint8_t);

enum drm_connector_status dwhdmi_phy_detect(struct dwhdmi_softc *, bool);
void		dwhdmi_phy_enable(struct dwhdmi_softc *);
void		dwhdmi_phy_disable(struct dwhdmi_softc *);
void		dwhdmi_phy_mode_set(struct dwhdmi_softc *,
				    struct drm_display_mode *,
				    struct drm_display_mode *);

#endif /* !_DEV_IC_DWHDMI_H */
