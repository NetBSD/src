/* $NetBSD: sunxi_mixer.c,v 1.3 2019/02/04 12:10:13 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_mixer.c,v 1.3 2019/02/04 12:10:13 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

#include <arm/sunxi/sunxi_drm.h>

#define	SUNXI_MIXER_FREQ	432000000

#define	GLB_BASE		0x00000
#define	BLD_BASE		0x01000
#define	OVL_BASE(n)		(0x02000 + (n) * 0x1000)
#define	OVL_V_BASE		OVL_BASE(0)
#define	OVL_UI_BASE		OVL_BASE(1)

/* GLB registers */
#define	GLB_CTL			0x000
#define	 GLB_CTL_EN				__BIT(0)
#define	GLB_STS			0x004	
#define	GLB_DBUFFER		0x008
#define	 GLB_DBUFFER_DOUBLE_BUFFER_RDY		__BIT(0)
#define	GLB_SIZE		0x00c

/* BLD registers */
#define	BLD_FILL_COLOR_CTL	0x000
#define	 BLD_FILL_COLOR_CTL_P1_EN		__BIT(9)
#define	 BLD_FILL_COLOR_CTL_P0_EN		__BIT(8)
#define	BLD_CH_ISIZE(n)		(0x008 + (n) * 0x10)
#define	BLD_CH_OFFSET(n)	(0x00c + (n) * 0x10)
#define	BLD_CH_RTCTL		0x080
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
#define	  OVL_V_ATTCTL_LAY_FBFMT_XRGB_8888	0x04
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
#define	  OVL_UI_ATTR_CTL_LAY_FBFMT_XRGB_8888	0x04
#define	 OVL_UI_ATTR_CTL_LAY_EN			__BIT(0)
#define	OVL_UI_MBSIZE(n)	(0x004 + (n) * 0x20)
#define	OVL_UI_COOR(n)		(0x008 + (n) * 0x20)
#define	OVL_UI_PITCH(n)		(0x00c + (n) * 0x20)
#define	OVL_UI_TOP_LADD(n)	(0x010 + (n) * 0x20)
#define	OVL_UI_TOP_HADD		0x080
#define	 OVL_UI_TOP_HADD_LAYER0	__BITS(7,0)
#define	OVL_UI_SIZE		0x088

enum {
	MIXER_PORT_OUTPUT = 1,
};

static const char * const compatible[] = {
	"allwinner,sun8i-h3-de2-mixer-0",
	"allwinner,sun50i-a64-de2-mixer-0",
	"allwinner,sun50i-a64-de2-mixer-1",
	NULL
};

struct sunxi_mixer_softc;

struct sunxi_mixer_crtc {
	struct drm_crtc		base;
	struct sunxi_mixer_softc *sc;
};

struct sunxi_mixer_overlay {
	struct drm_plane	base;
	struct sunxi_mixer_softc *sc;
};

struct sunxi_mixer_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	struct sunxi_mixer_crtc	sc_crtc;
	struct sunxi_mixer_overlay sc_overlay;

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
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, OVL_V_BASE + (reg))
#define	OVL_V_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, OVL_V_BASE + (reg), (val))

#define	OVL_UI_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, OVL_UI_BASE + (reg))
#define	OVL_UI_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, OVL_UI_BASE + (reg), (val))

#define	to_sunxi_mixer_crtc(x)		container_of(x, struct sunxi_mixer_crtc, base)
#define	to_sunxi_mixer_overlay(x)	container_of(x, struct sunxi_mixer_overlay, base)

static void
sunxi_mixer_destroy(struct drm_crtc *crtc)
{
	drm_crtc_cleanup(crtc);
}

static const struct drm_crtc_funcs sunxi_mixer_crtc_funcs = {
	.set_config = drm_crtc_helper_set_config,
	.destroy = sunxi_mixer_destroy,
};

static void
sunxi_mixer_dpms(struct drm_crtc *crtc, int mode)
{
}

static bool
sunxi_mixer_mode_fixup(struct drm_crtc *crtc,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static int
sunxi_mixer_mode_do_set_base(struct drm_crtc *crtc, struct drm_framebuffer *fb,
    int x, int y, int atomic)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;
	struct sunxi_drm_framebuffer *sfb = atomic?
	    to_sunxi_drm_framebuffer(fb) :
	    to_sunxi_drm_framebuffer(crtc->primary->fb);

	uint64_t paddr = (uint64_t)sfb->obj->dmamap->dm_segs[0].ds_addr;

	uint32_t haddr = (paddr >> 32) & OVL_UI_TOP_HADD_LAYER0;
	uint32_t laddr = paddr & 0xffffffff;

	/* Framebuffer start address */
	OVL_UI_WRITE(sc, OVL_UI_TOP_HADD, haddr);
	OVL_UI_WRITE(sc, OVL_UI_TOP_LADD(0), laddr);

	return 0;
}

static int
sunxi_mixer_mode_set(struct drm_crtc *crtc, struct drm_display_mode *mode,
    struct drm_display_mode *adjusted_mode, int x, int y,
    struct drm_framebuffer *old_fb)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;
	uint32_t val;

	const uint32_t size = ((adjusted_mode->vdisplay - 1) << 16) |
			      (adjusted_mode->hdisplay - 1);
	const uint32_t offset = (y << 16) | x;

	/* Set global size */
	GLB_WRITE(sc, GLB_SIZE, size);

	/* Enable pipe 0 */
	val = BLD_READ(sc, BLD_FILL_COLOR_CTL);
	val |= BLD_FILL_COLOR_CTL_P0_EN;
	BLD_WRITE(sc, BLD_FILL_COLOR_CTL, val);

	/* Set blender 0 input size */
	BLD_WRITE(sc, BLD_CH_ISIZE(0), size);
	/* Set blender 0 offset */
	BLD_WRITE(sc, BLD_CH_OFFSET(0), offset);
	/* Route channel 1 to pipe 0 */
	val = BLD_READ(sc, BLD_CH_RTCTL);
	val &= ~BLD_CH_RTCTL_P0;
	val |= __SHIFTIN(1, BLD_CH_RTCTL_P0);
	BLD_WRITE(sc, BLD_CH_RTCTL, val);
	/* Set blender output size */
	BLD_WRITE(sc, BLD_SIZE, size);

	/* Enable UI overlay in XRGB8888 mode */
	val = OVL_UI_ATTR_CTL_LAY_EN |
	      __SHIFTIN(OVL_UI_ATTR_CTL_LAY_FBFMT_XRGB_8888, OVL_UI_ATTR_CTL_LAY_FBFMT);
	OVL_UI_WRITE(sc, OVL_UI_ATTR_CTL(0), val);
	/* Set UI overlay layer size */
	OVL_UI_WRITE(sc, OVL_UI_MBSIZE(0), size);
	/* Set UI overlay offset */
	OVL_UI_WRITE(sc, OVL_UI_COOR(0), offset);
	/* Set UI overlay line size */
	OVL_UI_WRITE(sc, OVL_UI_PITCH(0), adjusted_mode->hdisplay * 4);
	/* Set UI overlay window size */
	OVL_UI_WRITE(sc, OVL_UI_SIZE, size);

	sunxi_mixer_mode_do_set_base(crtc, old_fb, x, y, 0);

	return 0;
}

static int
sunxi_mixer_mode_set_base(struct drm_crtc *crtc, int x, int y,
    struct drm_framebuffer *old_fb)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;

	sunxi_mixer_mode_do_set_base(crtc, old_fb, x, y, 0);

	/* Commit settings */
	GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);

	return 0;
}

static int
sunxi_mixer_mode_set_base_atomic(struct drm_crtc *crtc, struct drm_framebuffer *fb,
    int x, int y, enum mode_set_atomic state)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;

	sunxi_mixer_mode_do_set_base(crtc, fb, x, y, 1);

	/* Commit settings */
	GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);

	return 0;
}

static void
sunxi_mixer_disable(struct drm_crtc *crtc)
{
}

static void
sunxi_mixer_prepare(struct drm_crtc *crtc)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;

	/* RT enable */
	GLB_WRITE(sc, GLB_CTL, GLB_CTL_EN);
}

static void
sunxi_mixer_commit(struct drm_crtc *crtc)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;

	/* Commit settings */
	GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);
}

static const struct drm_crtc_helper_funcs sunxi_mixer_crtc_helper_funcs = {
	.dpms = sunxi_mixer_dpms,
	.mode_fixup = sunxi_mixer_mode_fixup,
	.mode_set = sunxi_mixer_mode_set,
	.mode_set_base = sunxi_mixer_mode_set_base,
	.mode_set_base_atomic = sunxi_mixer_mode_set_base_atomic,
	.disable = sunxi_mixer_disable,
	.prepare = sunxi_mixer_prepare,
	.commit = sunxi_mixer_commit,
};

static void
sunxi_mixer_overlay_destroy(struct drm_plane *plane)
{
}

static bool
sunxi_mixer_overlay_ui(uint32_t drm_format)
{
	switch (drm_format) {
	case DRM_FORMAT_XRGB8888:
		return true;
	default:
		return false;
	}
}

static u_int
sunxi_mixer_overlay_format(uint32_t drm_format)
{
	switch (drm_format) {
	case DRM_FORMAT_XRGB8888:	return OVL_V_ATTCTL_LAY_FBFMT_XRGB_8888;
	case DRM_FORMAT_VYUY:		return OVL_V_ATTCTL_LAY_FBFMT_VYUY;
	case DRM_FORMAT_YVYU:		return OVL_V_ATTCTL_LAY_FBFMT_YVYU;
	case DRM_FORMAT_UYVY:		return OVL_V_ATTCTL_LAY_FBFMT_UYVY;
	case DRM_FORMAT_YUYV:		return OVL_V_ATTCTL_LAY_FBFMT_YUYV;
	case DRM_FORMAT_YUV422:		return OVL_V_ATTCTL_LAY_FBFMT_YUV422;
	case DRM_FORMAT_YUV420:		return OVL_V_ATTCTL_LAY_FBFMT_YUV420;
	case DRM_FORMAT_YUV411:		return OVL_V_ATTCTL_LAY_FBFMT_YUV411;
	default:			return 0;	/* shouldn't happen */
	}
}

static int
sunxi_mixer_overlay_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
    struct drm_framebuffer *fb, int crtc_x, int crtc_y, u_int crtc_w, u_int crtc_h,
    uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	struct sunxi_mixer_overlay *overlay = to_sunxi_mixer_overlay(plane);
	struct sunxi_mixer_softc * const sc = overlay->sc;
	struct sunxi_drm_framebuffer *sfb = to_sunxi_drm_framebuffer(fb);
	uint32_t val;

	const u_int fbfmt = sunxi_mixer_overlay_format(fb->pixel_format);
	const uint64_t paddr = (uint64_t)sfb->obj->dmamap->dm_segs[0].ds_addr;

	const uint32_t input_size = (((src_h >> 16) - 1) << 16) | ((src_w >> 16) - 1);
	const uint32_t input_pos = ((src_y >> 16) << 16) | (src_x >> 16);

	OVL_V_WRITE(sc, OVL_V_MBSIZE(0), input_size);
	OVL_V_WRITE(sc, OVL_V_COOR(0), input_pos);

	OVL_V_WRITE(sc, OVL_V_PITCH0(0), fb->pitches[0]);
	OVL_V_WRITE(sc, OVL_V_PITCH1(0), fb->pitches[1]);
	OVL_V_WRITE(sc, OVL_V_PITCH2(0), fb->pitches[2]);

	const uint64_t paddr0 = paddr + fb->offsets[0] +
	    (src_x >> 16) * drm_format_plane_cpp(fb->pixel_format, 0) +
	    (src_y >> 16) * fb->pitches[0];
	const uint64_t paddr1 = paddr + fb->offsets[1] +
	    (src_x >> 16) * drm_format_plane_cpp(fb->pixel_format, 1) +
	    (src_y >> 16) * fb->pitches[1];
	const uint64_t paddr2 = paddr + fb->offsets[2] +
	    (src_x >> 16) * drm_format_plane_cpp(fb->pixel_format, 2) +
	    (src_y >> 16) * fb->pitches[2];

	OVL_V_WRITE(sc, OVL_V_TOP_HADD0, (paddr0 >> 32) & OVL_V_TOP_HADD_LAYER0);
	OVL_V_WRITE(sc, OVL_V_TOP_HADD1, (paddr1 >> 32) & OVL_V_TOP_HADD_LAYER0);
	OVL_V_WRITE(sc, OVL_V_TOP_HADD2, (paddr2 >> 32) & OVL_V_TOP_HADD_LAYER0);

	OVL_V_WRITE(sc, OVL_V_TOP_LADD0(0), paddr0 & 0xffffffff);
	OVL_V_WRITE(sc, OVL_V_TOP_LADD1(0), paddr1 & 0xffffffff);
	OVL_V_WRITE(sc, OVL_V_TOP_LADD2(0), paddr2 & 0xffffffff);

	OVL_V_WRITE(sc, OVL_V_SIZE, input_size);

	val = OVL_V_ATTCTL_LAY0_EN;
	val |= __SHIFTIN(fbfmt, OVL_V_ATTCTL_LAY_FBFMT);
	if (sunxi_mixer_overlay_ui(fb->pixel_format) == true)
		val |= OVL_V_ATTCTL_VIDEO_UI_SEL;
	OVL_V_WRITE(sc, OVL_V_ATTCTL(0), val);

	/* Set blender 1 input size */
	BLD_WRITE(sc, BLD_CH_ISIZE(1), input_size);
	/* Set blender 1 offset */
	BLD_WRITE(sc, BLD_CH_OFFSET(1), (crtc_y << 16) | crtc_x);
	/* Route channel 0 to pipe 1 */
	val = BLD_READ(sc, BLD_CH_RTCTL);
	val &= ~BLD_CH_RTCTL_P1;
	val |= __SHIFTIN(0, BLD_CH_RTCTL_P1);
	BLD_WRITE(sc, BLD_CH_RTCTL, val);

        /* Enable pipe 1 */
	val = BLD_READ(sc, BLD_FILL_COLOR_CTL);
	val |= BLD_FILL_COLOR_CTL_P1_EN;
	BLD_WRITE(sc, BLD_FILL_COLOR_CTL, val);

	/* Commit settings */
	GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);

	return 0;
}

static int
sunxi_mixer_overlay_disable_plane(struct drm_plane *plane)
{
	struct sunxi_mixer_overlay *overlay = to_sunxi_mixer_overlay(plane);
	struct sunxi_mixer_softc * const sc = overlay->sc;
	uint32_t val;

	val = BLD_READ(sc, BLD_FILL_COLOR_CTL);
	val &= ~BLD_FILL_COLOR_CTL_P1_EN;
	BLD_WRITE(sc, BLD_FILL_COLOR_CTL, val);

	/* Commit settings */
	GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);

	return 0;
}

static const struct drm_plane_funcs sunxi_mixer_overlay_funcs = {
	.update_plane = sunxi_mixer_overlay_update_plane,
	.disable_plane = sunxi_mixer_overlay_disable_plane,
	.destroy = sunxi_mixer_overlay_destroy,
};

static uint32_t sunxi_mixer_overlay_formats[] = {
	DRM_FORMAT_XRGB8888,
#if notyet
	DRM_FORMAT_VYUY,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YUV422,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_YUV411,
#endif
};

static int
sunxi_mixer_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct sunxi_mixer_softc * const sc = device_private(dev);
	struct drm_device *ddev;

	if (!activate)
		return EINVAL;

	ddev = sunxi_drm_endpoint_device(ep);
	if (ddev == NULL) {
		DRM_ERROR("couldn't find DRM device\n");
		return ENXIO;
	}

	sc->sc_crtc.sc = sc;
	sc->sc_overlay.sc = sc;

	drm_crtc_init(ddev, &sc->sc_crtc.base, &sunxi_mixer_crtc_funcs);
	drm_crtc_helper_add(&sc->sc_crtc.base, &sunxi_mixer_crtc_helper_funcs);

	drm_universal_plane_init(ddev, &sc->sc_overlay.base,
	    1 << drm_crtc_index(&sc->sc_crtc.base), &sunxi_mixer_overlay_funcs,
	    sunxi_mixer_overlay_formats, __arraycount(sunxi_mixer_overlay_formats),
	    DRM_PLANE_TYPE_OVERLAY);

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

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sunxi_mixer_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_mixer_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct fdt_endpoint *out_ep;
	const int phandle = faa->faa_phandle;
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

	aprint_naive("\n");
	aprint_normal(": Display Engine Mixer\n");

	sc->sc_ports.dp_ep_activate = sunxi_mixer_ep_activate;
	sc->sc_ports.dp_ep_get_data = sunxi_mixer_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_CRTC);

	out_ep = fdt_endpoint_get_from_index(&sc->sc_ports, MIXER_PORT_OUTPUT, 0);
	if (out_ep != NULL)
		sunxi_drm_register_endpoint(phandle, out_ep);
}

CFATTACH_DECL_NEW(sunxi_mixer, sizeof(struct sunxi_mixer_softc),
	sunxi_mixer_match, sunxi_mixer_attach, NULL, NULL);
