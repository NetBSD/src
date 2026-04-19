/* $NetBSD: sunxi_mixer.c,v 1.20 2026/04/19 10:55:21 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_mixer.c,v 1.20 2026/04/19 10:55:21 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <dev/fdt/fdt_port.h>
#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_drm.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_vblank.h>

#define	MIXER_CURSOR_MAXWIDTH	256
#define	MIXER_CURSOR_MAXHEIGHT	256

#define	SUNXI_MIXER_FREQ	432000000

#define	GLB_BASE		0x00000
#define	BLD_BASE		0x01000
#define	OVL_BASE(n)		(0x02000 + (n) * 0x1000)
#define	VSU_BASE		0x20000
#define	CSC_BASE(n)		((n) == 0 ? 0xaa050 : 0xa0000)

/* GLB registers */
#define	GLB_CTL			0x000
#define	 GLB_CTL_EN				__BIT(0)
#define	GLB_STS			0x004	
#define	GLB_DBUFFER		0x008
#define	 GLB_DBUFFER_DOUBLE_BUFFER_RDY		__BIT(0)
#define	GLB_SIZE		0x00c

/* BLD registers */
#define	BLD_FILL_COLOR_CTL	0x000
#define	 BLD_FILL_COLOR_CTL_P3_EN		__BIT(11)
#define	 BLD_FILL_COLOR_CTL_P2_EN		__BIT(10)
#define	 BLD_FILL_COLOR_CTL_P1_EN		__BIT(9)
#define	 BLD_FILL_COLOR_CTL_P0_EN		__BIT(8)
#define	 BLD_FILL_COLOR_CTL_P3_FCEN		__BIT(3)
#define	 BLD_FILL_COLOR_CTL_P2_FCEN		__BIT(2)
#define	 BLD_FILL_COLOR_CTL_P1_FCEN		__BIT(1)
#define	 BLD_FILL_COLOR_CTL_P0_FCEN		__BIT(0)
#define	BLD_FILL_COLOR(n)	(0x004 + (n) * 0x10)
#define	BLD_CH_ISIZE(n)		(0x008 + (n) * 0x10)
#define	BLD_CH_OFFSET(n)	(0x00c + (n) * 0x10)
#define	BLD_CH_RTCTL		0x080
#define	 BLD_CH_RTCTL_P3			__BITS(15,12)
#define	 BLD_CH_RTCTL_P2			__BITS(11,8)
#define	 BLD_CH_RTCTL_P1			__BITS(7,4)
#define	 BLD_CH_RTCTL_P0			__BITS(3,0)
#define	BLD_SIZE		0x08c
#define	BLD_CTL(n)		(0x090 + (n) * 0x04)

/* OVL_V registers */
#define	OVL_V_ATTCTL(n)		(0x000 + (n) * 0x30)
#define	 OVL_V_ATTCTL_VIDEO_UI_SEL		__BIT(15)
#define	 OVL_V_ATTCTL_LAY_FBFMT			__BITS(12,8)
#define	  OVL_V_ATTCTL_LAY_FBFMT_VYUY		0x00
#define	  OVL_V_ATTCTL_LAY_FBFMT_YVYU		0x01
#define	  OVL_V_ATTCTL_LAY_FBFMT_UYVY		0x02
#define	  OVL_V_ATTCTL_LAY_FBFMT_YUYV		0x03
#define	  OVL_V_ATTCTL_LAY_FBFMT_YUV422		0x06
#define	  OVL_V_ATTCTL_LAY_FBFMT_YUV420		0x0a
#define	  OVL_V_ATTCTL_LAY_FBFMT_YUV411		0x0e
#if BYTE_ORDER == BIG_ENDIAN
#define	  OVL_V_ATTCTL_LAY_FBFMT_ARGB_8888	0x03
#define	  OVL_V_ATTCTL_LAY_FBFMT_XRGB_8888	0x07
#else
#define	  OVL_V_ATTCTL_LAY_FBFMT_ARGB_8888	0x00
#define	  OVL_V_ATTCTL_LAY_FBFMT_XRGB_8888	0x04
#endif
#define	 OVL_V_ATTCTL_LAY0_EN			__BIT(0)
#define	OVL_V_MBSIZE(n)		(0x004 + (n) * 0x30)
#define	OVL_V_COOR(n)		(0x008 + (n) * 0x30)
#define	OVL_V_PITCH0(n)		(0x00c + (n) * 0x30)
#define	OVL_V_PITCH1(n)		(0x010 + (n) * 0x30)
#define	OVL_V_PITCH2(n)		(0x014 + (n) * 0x30)
#define	OVL_V_TOP_LADD0(n)	(0x018 + (n) * 0x30)
#define	OVL_V_TOP_LADD1(n)	(0x01c + (n) * 0x30)
#define	OVL_V_TOP_LADD2(n)	(0x020 + (n) * 0x30)
#define	OVL_V_FILL_COLOR(n)	(0x0c0 + (n) * 0x4)
#define	OVL_V_TOP_HADD0		0x0d0
#define	OVL_V_TOP_HADD1		0x0d4
#define	OVL_V_TOP_HADD2		0x0d8
#define	 OVL_V_TOP_HADD_LAYER0	__BITS(7,0)
#define	OVL_V_SIZE		0x0e8
#define	OVL_V_HDS_CTL0		0x0f0
#define	OVL_V_HDS_CTL1		0x0f4
#define	OVL_V_VDS_CTL0		0x0f8
#define	OVL_V_VDS_CTL1		0x0fc

/* OVL_UI registers */
#define	OVL_UI_ATTR_CTL(n)	(0x000 + (n) * 0x20)
#define	 OVL_UI_ATTR_CTL_LAY_FBFMT		__BITS(12,8)
#if BYTE_ORDER == BIG_ENDIAN
#define	  OVL_UI_ATTR_CTL_LAY_FBFMT_ARGB_8888	0x03
#define	  OVL_UI_ATTR_CTL_LAY_FBFMT_XRGB_8888	0x07
#else
#define	  OVL_UI_ATTR_CTL_LAY_FBFMT_ARGB_8888	0x00
#define	  OVL_UI_ATTR_CTL_LAY_FBFMT_XRGB_8888	0x04
#endif
#define	 OVL_UI_ATTR_CTL_LAY_EN			__BIT(0)
#define	OVL_UI_MBSIZE(n)	(0x004 + (n) * 0x20)
#define	OVL_UI_COOR(n)		(0x008 + (n) * 0x20)
#define	OVL_UI_PITCH(n)		(0x00c + (n) * 0x20)
#define	OVL_UI_TOP_LADD(n)	(0x010 + (n) * 0x20)
#define	OVL_UI_FILL_COLOR(n)	(0x018 + (n) * 0x20)
#define	OVL_UI_TOP_HADD		0x080
#define	 OVL_UI_TOP_HADD_LAYER1	__BITS(15,8)
#define	 OVL_UI_TOP_HADD_LAYER0	__BITS(7,0)
#define	OVL_UI_SIZE		0x088

/* VSU registers */
#define	VS_CTRL_REG		0x000
#define	 VS_CTRL_COEF_SWITCH_EN			__BIT(4)
#define	 VS_CTRL_EN				__BIT(0)
#define	VS_STATUS_REG		0x008
#define	VS_FIELD_CTRL_REG	0x00c
#define	VS_OUT_SIZE_REG		0x040
#define	VS_Y_SIZE_REG		0x080
#define	VS_Y_HSTEP_REG		0x088
#define	VS_Y_VSTEP_REG		0x08c
#define	VS_Y_HPHASE_REG		0x090
#define	VS_Y_VPHASE0_REG	0x098
#define	VS_Y_VPHASE1_REG	0x09c
#define	VS_C_SIZE_REG		0x0c0
#define	VS_C_HSTEP_REG		0x0c8
#define	VS_C_VSTEP_REG		0x0cc
#define	VS_C_HPHASE_REG		0x0d0
#define	VS_C_VPHASE0_REG	0x0d8
#define	VS_C_VPHASE1_REG	0x0dc
#define	VS_Y_HCOEF0_REG(n)	(0x200 + (n) * 0x4)
#define	VS_Y_HCOEF1_REG(n)	(0x300 + (n) * 0x4)
#define	VS_Y_VCOEF_REG(n)	(0x400 + (n) * 0x4)
#define	VS_C_HCOEF0_REG(n)	(0x600 + (n) * 0x4)
#define	VS_C_HCOEF1_REG(n)	(0x700 + (n) * 0x4)
#define	VS_C_VCOEF_REG(n)	(0x800 + (n) * 0x4)

/* CSC registers */
#define	CSC_BYPASS_REG		0x000
#define	 CSC_BYPASS_DISABLE			__BIT(0)
#define	CSC_COEFF0_REG(n)	(0x10 + 0x10 * (n))
#define	GLB_ALPHA_REG		0x040

enum {
	MIXER_PORT_OUTPUT = 1,
};

struct sunxi_mixer_compat_data {
	uint8_t ovl_ui_count;
	uint8_t mixer_index;
};

struct sunxi_mixer_compat_data mixer0_data = {
	.ovl_ui_count = 3,
	.mixer_index = 0,
};

struct sunxi_mixer_compat_data mixer1_data = {
	.ovl_ui_count = 1,
	.mixer_index = 1,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun8i-h3-de2-mixer-0",
	  .data = &mixer0_data },
	{ .compat = "allwinner,sun8i-v3s-de2-mixer",
	  .data = &mixer0_data },
	{ .compat = "allwinner,sun50i-a64-de2-mixer-0",
	  .data = &mixer0_data },
	{ .compat = "allwinner,sun50i-a64-de2-mixer-1",
	  .data = &mixer1_data },

	DEVICE_COMPAT_EOL
};

struct sunxi_mixer_softc;

struct sunxi_mixer_crtc {
	struct drm_crtc		base;
	struct sunxi_mixer_softc *sc;
};

struct sunxi_mixer_plane {
	struct drm_plane	base;
	struct sunxi_mixer_softc *sc;
};

struct sunxi_mixer_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	u_int			sc_ovl_ui_count;

	struct sunxi_mixer_crtc	sc_crtc;
	struct sunxi_mixer_plane sc_plane;

	struct fdt_device_ports	sc_ports;
};

#define	GLB_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, GLB_BASE + (reg))
#define	GLB_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, GLB_BASE + (reg), (val))

#define	BLD_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, BLD_BASE + (reg))
#define	BLD_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, BLD_BASE + (reg), (val))

#define	OVL_V_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, OVL_BASE(0) + (reg))
#define	OVL_V_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, OVL_BASE(0) + (reg), (val))

#define	OVL_UI_READ(sc, n, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, OVL_BASE((n) + 1) + (reg))
#define	OVL_UI_WRITE(sc, n, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, OVL_BASE((n) + 1) + (reg), (val))

#define	VSU_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, VSU_BASE + (reg))
#define	VSU_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, VSU_BASE + (reg), (val))

#define	CSC_READ(sc, n, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, CSC_BASE(n) + (reg))
#define	CSC_WRITE(sc, n, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, CSC_BASE(n) + (reg), (val))

#define	to_sunxi_mixer_crtc(x)		container_of(x, struct sunxi_mixer_crtc, base)
#define	to_sunxi_mixer_plane(x)	container_of(x, struct sunxi_mixer_plane, base)

static int
sunxi_mixer_plane_atomic_check(struct drm_plane *plane,
    struct drm_plane_state *state)
{
	struct drm_crtc_state *crtc_state;

	if (state->crtc == NULL) {
		return 0;
	}

	crtc_state = drm_atomic_get_new_crtc_state(state->state, state->crtc);
	if (IS_ERR(crtc_state)) {
		return PTR_ERR(crtc_state);
	}

	return drm_atomic_helper_check_plane_state(state, crtc_state,
	    DRM_PLANE_HELPER_NO_SCALING, DRM_PLANE_HELPER_NO_SCALING,
	    false, true);
}

static void
sunxi_mixer_plane_atomic_update(struct drm_plane *plane,
    struct drm_plane_state *old_state)
{
	struct sunxi_mixer_plane *mixer_plane = to_sunxi_mixer_plane(plane);
	struct sunxi_mixer_softc * const sc = mixer_plane->sc;
	struct sunxi_drm_framebuffer *sfb =
	    to_sunxi_drm_framebuffer(plane->state->fb);
	uint32_t block_h, block_w, x, y, block_start_y, num_hblocks;
	uint32_t val;

	block_h = drm_format_info_block_height(sfb->base.format, 0);
	block_w = drm_format_info_block_width(sfb->base.format, 0);
	x = plane->state->src_x >> 16;
	y = plane->state->src_y >> 16;
	block_start_y = (y / block_h) * block_h;
	num_hblocks = x / block_w;

	uint64_t paddr = (uint64_t)sfb->obj->dmamap->dm_segs[0].ds_addr;

	paddr += block_start_y * sfb->base.pitches[0];
	paddr += sfb->base.format->char_per_block[0] * num_hblocks;

	uint32_t haddr = (paddr >> 32) & OVL_UI_TOP_HADD_LAYER0;
	uint32_t laddr = paddr & 0xffffffff;

	/* Enable UI overlay */
	val = OVL_UI_ATTR_CTL_LAY_EN |
	      __SHIFTIN(OVL_UI_ATTR_CTL_LAY_FBFMT_XRGB_8888,
			OVL_UI_ATTR_CTL_LAY_FBFMT);
	OVL_UI_WRITE(sc, 0, OVL_UI_ATTR_CTL(0), val);

	/* Set UI overlay line size */
	OVL_UI_WRITE(sc, 0, OVL_UI_PITCH(0), sfb->base.pitches[0]);

	/* Framebuffer start address */
	val = OVL_UI_READ(sc, 0, OVL_UI_TOP_HADD);
	val &= ~OVL_UI_TOP_HADD_LAYER0;
	val |= __SHIFTIN(haddr, OVL_UI_TOP_HADD_LAYER0);
	OVL_UI_WRITE(sc, 0, OVL_UI_TOP_HADD, val);
	OVL_UI_WRITE(sc, 0, OVL_UI_TOP_LADD(0), laddr);

	/* Commit settings */
	GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);
}

static void
sunxi_mixer_plane_atomic_disable(struct drm_plane *plane,
    struct drm_plane_state *state)
{
	struct sunxi_mixer_plane *mixer_plane = to_sunxi_mixer_plane(plane);
	struct sunxi_mixer_softc * const sc = mixer_plane->sc;

	/* Disable UI overlay */
	OVL_UI_WRITE(sc, 0, OVL_UI_ATTR_CTL(0), 0);
}

static const struct drm_plane_helper_funcs sunxi_mixer_plane_helper_funcs = {
	.atomic_check = sunxi_mixer_plane_atomic_check,
	.atomic_update = sunxi_mixer_plane_atomic_update,
	.atomic_disable = sunxi_mixer_plane_atomic_disable,
};

static bool
sunxi_mixer_format_mod_supported(struct drm_plane *plane, uint32_t format,
    uint64_t modifier)
{
	return modifier == DRM_FORMAT_MOD_LINEAR;
}

static const struct drm_plane_funcs sunxi_mixer_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
	.format_mod_supported = sunxi_mixer_format_mod_supported,
};

static void
sunxi_mixer_crtc_dpms(struct drm_crtc *crtc, int mode)
{
}

static int
sunxi_mixer_crtc_atomic_check(struct drm_crtc *crtc,
    struct drm_crtc_state *state)
{
	bool enabled = state->plane_mask & drm_plane_mask(crtc->primary);

	if (enabled != state->enable) {
		return -EINVAL;
	}

	return drm_atomic_add_affected_planes(state->state, crtc);
}

static void
sunxi_mixer_crtc_atomic_enable(struct drm_crtc *crtc,
    struct drm_crtc_state *state)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;
	struct drm_display_mode *adjusted_mode = &crtc->state->adjusted_mode;
	uint32_t val;

	const uint32_t size = ((adjusted_mode->vdisplay - 1) << 16) |
			      (adjusted_mode->hdisplay - 1);

	/* Set global size */
	GLB_WRITE(sc, GLB_SIZE, size);

	/* Enable pipe 0 */
	val = BLD_READ(sc, BLD_FILL_COLOR_CTL);
	val |= BLD_FILL_COLOR_CTL_P0_EN;
	BLD_WRITE(sc, BLD_FILL_COLOR_CTL, val);

	/* Set blender 0 input size */
	BLD_WRITE(sc, BLD_CH_ISIZE(0), size);
	/* Set blender 0 offset */
	BLD_WRITE(sc, BLD_CH_OFFSET(0), 0);
	/* Route channel 1 to pipe 0 */
	val = BLD_READ(sc, BLD_CH_RTCTL);
	val &= ~BLD_CH_RTCTL_P0;
	val |= __SHIFTIN(1, BLD_CH_RTCTL_P0);
	BLD_WRITE(sc, BLD_CH_RTCTL, val);
	/* Set blender output size */
	BLD_WRITE(sc, BLD_SIZE, size);

	/* Set UI overlay layer size */
	OVL_UI_WRITE(sc, 0, OVL_UI_MBSIZE(0), size);
	/* Set UI overlay offset */
	OVL_UI_WRITE(sc, 0, OVL_UI_COOR(0), 0);
	/* Set UI overlay window size */
	OVL_UI_WRITE(sc, 0, OVL_UI_SIZE, size);

	/* Commit settings */
	GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);

	drm_crtc_vblank_on(crtc);
}

static void
sunxi_mixer_crtc_atomic_disable(struct drm_crtc *crtc,
    struct drm_crtc_state *state)
{
	drm_crtc_vblank_off(crtc);

	if (crtc->state->event && !crtc->state->active) {
		spin_lock(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		spin_unlock(&crtc->dev->event_lock);

		crtc->state->event = NULL;
	}
}

static void
sunxi_mixer_crtc_atomic_flush(struct drm_crtc *crtc,
    struct drm_crtc_state *state)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;
	int ret;

	/* Commit settings */
	GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);

	/*
	 * If caller wants a vblank event, tell the vblank interrupt
	 * handler to send it on the next interrupt.
	 */
	spin_lock(&crtc->dev->event_lock);
	if (crtc->state->event) {
		struct sunxi_drm_softc *drm_sc = device_private(crtc->dev->dev);
		const int crtc_index = drm_crtc_index(crtc);
		struct sunxi_drm_vblank *vbl = &drm_sc->sc_vbl[crtc_index];

		if ((ret = drm_crtc_vblank_get_locked(crtc)) != 0) {
			aprint_error_dev(sc->sc_dev,
			     "drm_crtc_vblank_get: %d\n", ret);
		}
		if (vbl->event) { /* XXX leaky; KASSERT? */
			aprint_error_dev(sc->sc_dev, "unfinished vblank\n");
		}
		vbl->event = crtc->state->event;
		crtc->state->event = NULL;
	}
	spin_unlock(&crtc->dev->event_lock);
}

static int
sunxi_mixer_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct drm_device *ddev = crtc->dev;
	struct sunxi_drm_softc *drm_sc = device_private(ddev->dev);
	const int crtc_index = drm_crtc_index(crtc);

	if (drm_sc->sc_vbl[crtc_index].enable_vblank != NULL) {
		drm_sc->sc_vbl[crtc_index].enable_vblank(
		    drm_sc->sc_vbl[crtc_index].priv);
	}

	return 0;
}

static void
sunxi_mixer_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct drm_device *ddev = crtc->dev;
	struct sunxi_drm_softc *drm_sc = device_private(ddev->dev);
	const int crtc_index = drm_crtc_index(crtc);

	if (drm_sc->sc_vbl[crtc_index].disable_vblank != NULL) {
		drm_sc->sc_vbl[crtc_index].disable_vblank(
		    drm_sc->sc_vbl[crtc_index].priv);
	}
}

static const struct drm_crtc_helper_funcs sunxi_mixer_crtc_helper_funcs = {
	.dpms = sunxi_mixer_crtc_dpms,
	.atomic_check = sunxi_mixer_crtc_atomic_check,
	.atomic_enable = sunxi_mixer_crtc_atomic_enable,
	.atomic_disable = sunxi_mixer_crtc_atomic_disable,
	.atomic_flush = sunxi_mixer_crtc_atomic_flush,
};

static const struct drm_crtc_funcs sunxi_mixer_crtc_funcs = {
	.set_config = drm_atomic_helper_set_config,
	.destroy = drm_crtc_cleanup,
	.page_flip = drm_atomic_helper_page_flip,
	.reset = drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
	.enable_vblank = sunxi_mixer_crtc_enable_vblank,
	.disable_vblank = sunxi_mixer_crtc_disable_vblank,
};

static uint32_t sunxi_mixer_plane_formats[] = {
	DRM_FORMAT_XRGB8888,
};

static const uint64_t sunxi_mixer_plane_modifiers[] = {
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_INVALID,
};

static int
sunxi_mixer_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct sunxi_mixer_softc * const sc = device_private(dev);
	struct drm_plane *plane, *cursor;
	struct drm_device *ddev;
	struct sunxi_drm_softc *drm_sc;
	bus_size_t reg;

	if (!activate)
		return EINVAL;

	ddev = sunxi_drm_endpoint_device(ep);
	if (ddev == NULL) {
		DRM_ERROR("couldn't find DRM device\n");
		return ENXIO;
	}
	drm_sc = device_private(ddev->dev);

	sc->sc_crtc.sc = sc;
	sc->sc_plane.sc = sc;

	/* Initialize registers */
	for (reg = 0; reg < 0xc000; reg += 4)
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, 0);
	BLD_WRITE(sc, BLD_CTL(0), 0x03010301);
	BLD_WRITE(sc, BLD_CTL(1), 0x03010301);
	BLD_WRITE(sc, BLD_CTL(2), 0x03010301);
	BLD_WRITE(sc, BLD_CTL(3), 0x03010301);

	/* RT enable */
	GLB_WRITE(sc, GLB_CTL, GLB_CTL_EN);

	drm_universal_plane_init(ddev, &sc->sc_plane.base,
	    0, &sunxi_mixer_plane_funcs,
	    sunxi_mixer_plane_formats, __arraycount(sunxi_mixer_plane_formats),
	    sunxi_mixer_plane_modifiers, DRM_PLANE_TYPE_PRIMARY, NULL);
	drm_plane_helper_add(&sc->sc_plane.base, &sunxi_mixer_plane_helper_funcs);
	plane = &sc->sc_plane.base;

	/* TODO: hardware cursor support */
	cursor = NULL;

	drm_crtc_init_with_planes(ddev, &sc->sc_crtc.base, plane, cursor,
	    &sunxi_mixer_crtc_funcs, NULL);
	drm_crtc_helper_add(&sc->sc_crtc.base, &sunxi_mixer_crtc_helper_funcs);

	sc->sc_plane.base.possible_crtcs = 1 << drm_crtc_index(&sc->sc_crtc.base);

	drm_sc->sc_vbl[drm_crtc_index(&sc->sc_crtc.base)].crtc = &sc->sc_crtc.base;

	return fdt_endpoint_activate(ep, activate);
}

static void *
sunxi_mixer_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct sunxi_mixer_softc * const sc = device_private(dev);

	return &sc->sc_crtc;
}

static int
sunxi_mixer_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_mixer_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_mixer_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct fdt_endpoint *out_ep;
	const int phandle = faa->faa_phandle;
	const struct sunxi_mixer_compat_data * const cd =
	    of_compatible_lookup(phandle, compat_data)->data;
	struct clk *clk_bus, *clk_mod;
	struct fdtbus_reset *rst;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	rst = fdtbus_reset_get_index(phandle, 0);
	if (rst == NULL || fdtbus_reset_deassert(rst) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}

	clk_bus = fdtbus_clock_get(phandle, "bus");
	if (clk_bus == NULL || clk_enable(clk_bus) != 0) {
		aprint_error(": couldn't enable bus clock\n");
		return;
	}

	clk_mod = fdtbus_clock_get(phandle, "mod");
	if (clk_mod == NULL ||
	    clk_set_rate(clk_mod, SUNXI_MIXER_FREQ) != 0 ||
	    clk_enable(clk_mod) != 0) {
		aprint_error(": couldn't enable mod clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_ovl_ui_count = cd->ovl_ui_count;

	aprint_naive("\n");
	aprint_normal(": Display Engine Mixer\n");

	sc->sc_ports.dp_ep_activate = sunxi_mixer_ep_activate;
	sc->sc_ports.dp_ep_get_data = sunxi_mixer_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_CRTC);

	out_ep = fdt_endpoint_get_from_index(&sc->sc_ports,
	    MIXER_PORT_OUTPUT, cd->mixer_index);
	if (out_ep == NULL) {
		/* Couldn't find new-style DE2 endpoint, try old style. */
		out_ep = fdt_endpoint_get_from_index(&sc->sc_ports,
		    MIXER_PORT_OUTPUT, 0);
	}

	if (out_ep != NULL)
		sunxi_drm_register_endpoint(phandle, out_ep);
}

CFATTACH_DECL_NEW(sunxi_mixer, sizeof(struct sunxi_mixer_softc),
	sunxi_mixer_match, sunxi_mixer_attach, NULL, NULL);
