/* $NetBSD: sunxi_mixer.c,v 1.7 2019/02/16 16:20:50 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_mixer.c,v 1.7 2019/02/16 16:20:50 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/sysctl.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

#include <arm/sunxi/sunxi_drm.h>

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
#define	  OVL_V_ATTCTL_LAY_FBFMT_ARGB_8888	0x00
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
#define	  OVL_UI_ATTR_CTL_LAY_FBFMT_ARGB_8888	0x00
#define	  OVL_UI_ATTR_CTL_LAY_FBFMT_XRGB_8888	0x04
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

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun8i-h3-de2-mixer-0",	3 },
	{ "allwinner,sun50i-a64-de2-mixer-0",	3 },
	{ "allwinner,sun50i-a64-de2-mixer-1",	1 },
	{ NULL }
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
	struct sunxi_mixer_plane sc_overlay;

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
sunxi_mixer_mode_do_set_base(struct drm_crtc *crtc, struct drm_framebuffer *fb,
    int x, int y, int atomic)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;
	struct sunxi_drm_framebuffer *sfb = atomic?
	    to_sunxi_drm_framebuffer(fb) :
	    to_sunxi_drm_framebuffer(crtc->primary->fb);
	uint32_t val;

	uint64_t paddr = (uint64_t)sfb->obj->dmamap->dm_segs[0].ds_addr;

	uint32_t haddr = (paddr >> 32) & OVL_UI_TOP_HADD_LAYER0;
	uint32_t laddr = paddr & 0xffffffff;

	/* Framebuffer start address */
	val = OVL_UI_READ(sc, 0, OVL_UI_TOP_HADD);
	val &= ~OVL_UI_TOP_HADD_LAYER0;
	val |= __SHIFTIN(haddr, OVL_UI_TOP_HADD_LAYER0);
	OVL_UI_WRITE(sc, 0, OVL_UI_TOP_HADD, val);
	OVL_UI_WRITE(sc, 0, OVL_UI_TOP_LADD(0), laddr);

	return 0;
}

static void
sunxi_mixer_destroy(struct drm_crtc *crtc)
{
	drm_crtc_cleanup(crtc);
}

static int
sunxi_mixer_page_flip(struct drm_crtc *crtc, struct drm_framebuffer *fb,
    struct drm_pending_vblank_event *event, uint32_t flags)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;
	unsigned long irqflags;

	drm_crtc_wait_one_vblank(crtc);

	sunxi_mixer_mode_do_set_base(crtc, fb, 0, 0, true);

	/* Commit settings */
	GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);

	if (event) {
		spin_lock_irqsave(&crtc->dev->event_lock, irqflags);
		drm_send_vblank_event(crtc->dev, drm_crtc_index(crtc), event);
		spin_unlock_irqrestore(&crtc->dev->event_lock, irqflags);
	}

	return 0;
}

static int
sunxi_mixer_cursor_set(struct drm_crtc *crtc, struct drm_file *file_priv,
    uint32_t handle, uint32_t width, uint32_t height)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;
	struct drm_gem_object *gem_obj = NULL;
	struct drm_gem_cma_object *obj;
	uint32_t val;
	int error;

	/* Only mixers with more than one UI layer can support hardware cursors */
	if (sc->sc_ovl_ui_count <= 1)
		return -EINVAL;

	if (handle == 0) {
		val = BLD_READ(sc, BLD_FILL_COLOR_CTL);
		val &= ~BLD_FILL_COLOR_CTL_P2_EN;
		val |= BLD_FILL_COLOR_CTL_P2_FCEN;
		BLD_WRITE(sc, BLD_FILL_COLOR_CTL, val);

		error = 0;
		goto done;
	}

	/* Arbitrary limits, the hardware layer can do 8192x8192 */
	if (width > MIXER_CURSOR_MAXWIDTH || height > MIXER_CURSOR_MAXHEIGHT) {
		DRM_ERROR("Cursor dimension %ux%u not supported\n", width, height);
		error = -EINVAL;
		goto done;
	}

	gem_obj = drm_gem_object_lookup(crtc->dev, file_priv, handle);
	if (gem_obj == NULL) {
		DRM_ERROR("Cannot find cursor object %#x for crtc %d\n",
		    handle, drm_crtc_index(crtc));
		error = -ENOENT;
		goto done;
	}
	obj = to_drm_gem_cma_obj(gem_obj);

	if (obj->base.size < width * height * 4) {
		DRM_ERROR("Cursor buffer is too small\n");
		error = -ENOMEM;
		goto done;
	}

	uint64_t paddr = (uint64_t)obj->dmamap->dm_segs[0].ds_addr;
	uint32_t haddr = (paddr >> 32) & OVL_UI_TOP_HADD_LAYER0;
	uint32_t laddr = paddr & 0xffffffff;

	/* Framebuffer start address */
	val = OVL_UI_READ(sc, 1, OVL_UI_TOP_HADD);
	val &= ~OVL_UI_TOP_HADD_LAYER0;
	val |= __SHIFTIN(haddr, OVL_UI_TOP_HADD_LAYER0);
	OVL_UI_WRITE(sc, 1, OVL_UI_TOP_HADD, val);
	OVL_UI_WRITE(sc, 1, OVL_UI_TOP_LADD(0), laddr);

	const uint32_t size = ((height - 1) << 16) | (width - 1);
	const uint32_t offset = (crtc->cursor_y << 16) | crtc->cursor_x;
	const uint32_t crtc_size = ((crtc->primary->fb->height - 1) << 16) |
	    (crtc->primary->fb->width - 1);

	/* Enable cursor in ARGB8888 mode */
	val = OVL_UI_ATTR_CTL_LAY_EN |
	      __SHIFTIN(OVL_UI_ATTR_CTL_LAY_FBFMT_ARGB_8888, OVL_UI_ATTR_CTL_LAY_FBFMT);
	OVL_UI_WRITE(sc, 1, OVL_UI_ATTR_CTL(0), val);
	/* Set UI overlay layer size */
	OVL_UI_WRITE(sc, 1, OVL_UI_MBSIZE(0), size);
	/* Set UI overlay offset */
	OVL_UI_WRITE(sc, 1, OVL_UI_COOR(0), offset);
	/* Set UI overlay line size */
	OVL_UI_WRITE(sc, 1, OVL_UI_PITCH(0), width * 4);
	/* Set UI overlay window size */
	OVL_UI_WRITE(sc, 1, OVL_UI_SIZE, crtc_size);

	/* Set blender 2 input size */
	BLD_WRITE(sc, BLD_CH_ISIZE(2), crtc_size);
	/* Set blender 2 offset */
	BLD_WRITE(sc, BLD_CH_OFFSET(2), 0);
	/* Route channel 2 to pipe 2 */
	val = BLD_READ(sc, BLD_CH_RTCTL);
	val &= ~BLD_CH_RTCTL_P2;
	val |= __SHIFTIN(2, BLD_CH_RTCTL_P2);
	BLD_WRITE(sc, BLD_CH_RTCTL, val);

	/* Enable pipe 2 */
	val = BLD_READ(sc, BLD_FILL_COLOR_CTL);
	val |= BLD_FILL_COLOR_CTL_P2_EN;
	val &= ~BLD_FILL_COLOR_CTL_P2_FCEN;
	BLD_WRITE(sc, BLD_FILL_COLOR_CTL, val);

	error = 0;

done:
	if (error == 0) {
		/* Commit settings */
		GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);
	}

	if (gem_obj != NULL)
		drm_gem_object_unreference_unlocked(gem_obj);

	return error;
}

static int
sunxi_mixer_cursor_move(struct drm_crtc *crtc, int x, int y)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;

	crtc->cursor_x = x & 0xffff;
	crtc->cursor_y = y & 0xffff;

	const uint32_t offset = (crtc->cursor_y << 16) | crtc->cursor_x;

	OVL_UI_WRITE(sc, 1, OVL_UI_COOR(0), offset);

	/* Commit settings */
	GLB_WRITE(sc, GLB_DBUFFER, GLB_DBUFFER_DOUBLE_BUFFER_RDY);

	return 0;
}

static const struct drm_crtc_funcs sunxi_mixer0_crtc_funcs = {
	.set_config = drm_crtc_helper_set_config,
	.destroy = sunxi_mixer_destroy,
	.page_flip = sunxi_mixer_page_flip,
	.cursor_set = sunxi_mixer_cursor_set,
	.cursor_move = sunxi_mixer_cursor_move,
};

static const struct drm_crtc_funcs sunxi_mixer1_crtc_funcs = {
	.set_config = drm_crtc_helper_set_config,
	.destroy = sunxi_mixer_destroy,
	.page_flip = sunxi_mixer_page_flip,
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
sunxi_mixer_mode_set(struct drm_crtc *crtc, struct drm_display_mode *mode,
    struct drm_display_mode *adjusted_mode, int x, int y,
    struct drm_framebuffer *old_fb)
{
	struct sunxi_mixer_crtc *mixer_crtc = to_sunxi_mixer_crtc(crtc);
	struct sunxi_mixer_softc * const sc = mixer_crtc->sc;
	uint32_t val;
	u_int fbfmt;

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

	/* Enable UI overlay */
	if (crtc->primary->fb->pixel_format == DRM_FORMAT_XRGB8888)
		fbfmt = OVL_UI_ATTR_CTL_LAY_FBFMT_XRGB_8888;
	else
		fbfmt = OVL_UI_ATTR_CTL_LAY_FBFMT_ARGB_8888;
	val = OVL_UI_ATTR_CTL_LAY_EN | __SHIFTIN(fbfmt, OVL_UI_ATTR_CTL_LAY_FBFMT);
	OVL_UI_WRITE(sc, 0, OVL_UI_ATTR_CTL(0), val);
	/* Set UI overlay layer size */
	OVL_UI_WRITE(sc, 0, OVL_UI_MBSIZE(0), size);
	/* Set UI overlay offset */
	OVL_UI_WRITE(sc, 0, OVL_UI_COOR(0), offset);
	/* Set UI overlay line size */
	OVL_UI_WRITE(sc, 0, OVL_UI_PITCH(0), adjusted_mode->hdisplay * 4);
	/* Set UI overlay window size */
	OVL_UI_WRITE(sc, 0, OVL_UI_SIZE, size);

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
sunxi_mixer_overlay_rgb(uint32_t drm_format)
{
	switch (drm_format) {
	case DRM_FORMAT_ARGB8888:
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
	case DRM_FORMAT_ARGB8888:	return OVL_V_ATTCTL_LAY_FBFMT_ARGB_8888;
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

static const uint32_t lan3coefftab32_left[512] = {
	0x40000000, 0x40fe0000, 0x3ffd0100, 0x3efc0100,
	0x3efb0100, 0x3dfa0200, 0x3cf90200, 0x3bf80200,
	0x39f70200, 0x37f70200, 0x35f70200, 0x33f70200,
	0x31f70200, 0x2ef70200, 0x2cf70200, 0x2af70200,
	0x27f70200, 0x24f80100, 0x22f80100, 0x1ef90100,
	0x1cf90100, 0x19fa0100, 0x17fa0100, 0x14fb0100,
	0x11fc0000, 0x0ffc0000, 0x0cfd0000, 0x0afd0000,
	0x08fe0000, 0x05ff0000, 0x03ff0000, 0x02000000,

	0x40000000, 0x40fe0000, 0x3ffd0100, 0x3efc0100,
	0x3efb0100, 0x3dfa0200, 0x3cf90200, 0x3bf80200,
	0x39f70200, 0x37f70200, 0x35f70200, 0x33f70200,
	0x31f70200, 0x2ef70200, 0x2cf70200, 0x2af70200,
	0x27f70200, 0x24f80100, 0x22f80100, 0x1ef90100,
	0x1cf90100, 0x19fa0100, 0x17fa0100, 0x14fb0100,
	0x11fc0000, 0x0ffc0000, 0x0cfd0000, 0x0afd0000,
	0x08fe0000, 0x05ff0000, 0x03ff0000, 0x02000000,

	0x3806fc02, 0x3805fc02, 0x3803fd01, 0x3801fe01,
	0x3700fe01, 0x35ffff01, 0x35fdff01, 0x34fc0001,
	0x34fb0000, 0x33fa0000, 0x31fa0100, 0x2ff90100,
	0x2df80200, 0x2bf80200, 0x2af70200, 0x28f70200,
	0x27f70200, 0x24f70300, 0x22f70300, 0x1ff70300,
	0x1ef70300, 0x1cf70300, 0x1af70300, 0x18f70300,
	0x16f80300, 0x13f80300, 0x11f90300, 0x0ef90300,
	0x0efa0200, 0x0cfa0200, 0x0afb0200, 0x08fb0200,

	0x320bfa02, 0x3309fa02, 0x3208fb02, 0x3206fb02,
	0x3205fb02, 0x3104fc02, 0x3102fc01, 0x3001fd01,
	0x3000fd01, 0x2ffffd01, 0x2efefe01, 0x2dfdfe01,
	0x2bfcff01, 0x29fcff01, 0x28fbff01, 0x27fa0001,
	0x26fa0000, 0x24f90000, 0x22f90100, 0x20f90100,
	0x1ff80100, 0x1ef80100, 0x1cf80100, 0x1af80200,
	0x18f80200, 0x17f80200, 0x15f80200, 0x12f80200,
	0x11f90200, 0x0ff90200, 0x0df90200, 0x0cfa0200,

	0x2e0efa01, 0x2f0dfa01, 0x2f0bfa01, 0x2e0afa01,
	0x2e09fa01, 0x2e07fb01, 0x2d06fb01, 0x2d05fb01,
	0x2c04fb01, 0x2b03fc01, 0x2a02fc01, 0x2a01fc01,
	0x2800fd01, 0x28fffd01, 0x26fefd01, 0x25fefe01,
	0x24fdfe01, 0x23fcfe01, 0x21fcff01, 0x20fbff01,
	0x1efbff01, 0x1efbff00, 0x1cfa0000, 0x1bfa0000,
	0x19fa0000, 0x18fa0000, 0x17f90000, 0x15f90100,
	0x14f90100, 0x12f90100, 0x11f90100, 0x0ff90100,

	0x2b10fa00, 0x2b0ffa00, 0x2b0efa00, 0x2b0cfa00,
	0x2b0bfa00, 0x2a0afb01, 0x2a09fb01, 0x2908fb01,
	0x2807fb01, 0x2806fb01, 0x2805fb01, 0x2604fc01,
	0x2503fc01, 0x2502fc01, 0x2401fc01, 0x2301fc01,
	0x2100fd01, 0x21fffd01, 0x21fffd01, 0x20fefd01,
	0x1dfefe01, 0x1cfdfe01, 0x1cfdfe00, 0x1bfcfe00,
	0x19fcff00, 0x19fbff00, 0x17fbff00, 0x16fbff00,
	0x15fbff00, 0x14fb0000, 0x13fa0000, 0x11fa0000,

	0x2811fcff, 0x2810fcff, 0x280ffbff, 0x280efbff,
	0x270dfb00, 0x270cfb00, 0x270bfb00, 0x260afb00,
	0x2609fb00, 0x2508fb00, 0x2507fb00, 0x2407fb00,
	0x2406fc00, 0x2305fc00, 0x2204fc00, 0x2203fc00,
	0x2103fc00, 0x2002fc00, 0x1f01fd00, 0x1e01fd00,
	0x1d00fd00, 0x1dfffd00, 0x1cfffd00, 0x1bfefd00,
	0x1afefe00, 0x19fefe00, 0x18fdfe00, 0x17fdfe00,
	0x16fdfe00, 0x15fcff00, 0x13fcff00, 0x12fcff00,

	0x2512fdfe, 0x2511fdff, 0x2410fdff, 0x240ffdff,
	0x240efcff, 0x240dfcff, 0x240dfcff, 0x240cfcff,
	0x230bfcff, 0x230afc00, 0x2209fc00, 0x2108fc00,
	0x2108fc00, 0x2007fc00, 0x2006fc00, 0x2005fc00,
	0x1f05fc00, 0x1e04fc00, 0x1e03fc00, 0x1c03fd00,
	0x1c02fd00, 0x1b02fd00, 0x1b01fd00, 0x1a00fd00,
	0x1900fd00, 0x1800fd00, 0x17fffe00, 0x16fffe00,
	0x16fefe00, 0x14fefe00, 0x13fefe00, 0x13fdfe00,

	0x2212fffe, 0x2211fefe, 0x2211fefe, 0x2110fefe,
	0x210ffeff, 0x220efdff, 0x210dfdff, 0x210dfdff,
	0x210cfdff, 0x210bfdff, 0x200afdff, 0x200afdff,
	0x1f09fdff, 0x1f08fdff, 0x1d08fd00, 0x1c07fd00,
	0x1d06fd00, 0x1b06fd00, 0x1b05fd00, 0x1c04fd00,
	0x1b04fd00, 0x1a03fd00, 0x1a03fd00, 0x1902fd00,
	0x1802fd00, 0x1801fd00, 0x1701fd00, 0x1600fd00,
	0x1400fe00, 0x1400fe00, 0x14fffe00, 0x13fffe00,

	0x201200fe, 0x201100fe, 0x1f11fffe, 0x2010fffe,
	0x1f0ffffe, 0x1e0ffffe, 0x1f0efeff, 0x1f0dfeff,
	0x1f0dfeff, 0x1e0cfeff, 0x1e0bfeff, 0x1d0bfeff,
	0x1d0afeff, 0x1d09fdff, 0x1d09fdff, 0x1c08fdff,
	0x1c07fdff, 0x1b07fd00, 0x1b06fd00, 0x1a06fd00,
	0x1a05fd00, 0x1805fd00, 0x1904fd00, 0x1804fd00,
	0x1703fd00, 0x1703fd00, 0x1602fe00, 0x1502fe00,
	0x1501fe00, 0x1401fe00, 0x1301fe00, 0x1300fe00,

	0x1c1202fe, 0x1c1102fe, 0x1b1102fe, 0x1c1001fe,
	0x1b1001fe, 0x1b0f01ff, 0x1b0e00ff, 0x1b0e00ff,
	0x1b0d00ff, 0x1a0d00ff, 0x1a0c00ff, 0x1a0cffff,
	0x1a0bffff, 0x1a0bffff, 0x1a0affff, 0x180affff,
	0x1909ffff, 0x1809ffff, 0x1808ffff, 0x1808feff,
	0x1807feff, 0x1707fe00, 0x1606fe00, 0x1506fe00,
	0x1605fe00, 0x1505fe00, 0x1504fe00, 0x1304fe00,
	0x1304fe00, 0x1303fe00, 0x1203fe00, 0x1203fe00,

	0x181104ff, 0x191103ff, 0x191003ff, 0x181003ff,
	0x180f03ff, 0x190f02ff, 0x190e02ff, 0x180e02ff,
	0x180d02ff, 0x180d01ff, 0x180d01ff, 0x180c01ff,
	0x180c01ff, 0x180b00ff, 0x170b00ff, 0x170a00ff,
	0x170a00ff, 0x170900ff, 0x160900ff, 0x160900ff,
	0x1608ffff, 0x1508ffff, 0x1507ff00, 0x1507ff00,
	0x1407ff00, 0x1306ff00, 0x1306ff00, 0x1305ff00,
	0x1205ff00, 0x1105ff00, 0x1204ff00, 0x1104ff00,

	0x171005ff, 0x171005ff, 0x171004ff, 0x170f04ff,
	0x160f04ff, 0x170f03ff, 0x170e03ff, 0x160e03ff,
	0x160d03ff, 0x160d02ff, 0x160d02ff, 0x160c02ff,
	0x160c02ff, 0x160c02ff, 0x160b01ff, 0x150b01ff,
	0x150a01ff, 0x150a01ff, 0x150a01ff, 0x140901ff,
	0x14090000, 0x14090000, 0x14080000, 0x13080000,
	0x13070000, 0x12070000, 0x12070000, 0x12060000,
	0x11060000, 0x11060000, 0x11050000, 0x1105ff00,

	0x14100600, 0x15100500, 0x150f0500, 0x150f0500,
	0x140f0500, 0x150e0400, 0x140e0400, 0x130e0400,
	0x140d0400, 0x150d0300, 0x130d0300, 0x140c0300,
	0x140c0300, 0x140c0200, 0x140b0200, 0x130b0200,
	0x120b0200, 0x130a0200, 0x130a0200, 0x130a0100,
	0x13090100, 0x12090100, 0x11090100, 0x12080100,
	0x11080100, 0x10080100, 0x11070100, 0x11070000,
	0x10070000, 0x11060000, 0x10060000, 0x10060000,

	0x140f0600, 0x140f0600, 0x130f0600, 0x140f0500,
	0x140e0500, 0x130e0500, 0x130e0500, 0x140d0400,
	0x140d0400, 0x130d0400, 0x120d0400, 0x130c0400,
	0x130c0300, 0x130c0300, 0x130b0300, 0x130b0300,
	0x110b0300, 0x130a0200, 0x120a0200, 0x120a0200,
	0x120a0200, 0x12090200, 0x10090200, 0x11090100,
	0x11080100, 0x11080100, 0x10080100, 0x10080100,
	0x10070100, 0x10070100, 0x0f070100, 0x10060100,

	0x120f0701, 0x130f0601, 0x130e0601, 0x130e0601,
	0x120e0601, 0x130e0501, 0x130e0500, 0x130d0500,
	0x120d0500, 0x120d0500, 0x130c0400, 0x130c0400,
	0x120c0400, 0x110c0400, 0x120b0400, 0x120b0300,
	0x120b0300, 0x120b0300, 0x120a0300, 0x110a0300,
	0x110a0200, 0x11090200, 0x11090200, 0x10090200,
	0x10090200, 0x10080200, 0x10080200, 0x10080100,
	0x0f080100, 0x10070100, 0x0f070100, 0x0f070100
};

static const uint32_t lan3coefftab32_right[512] = {
	0x00000000, 0x00000002, 0x0000ff04, 0x0000ff06,
	0x0000fe08, 0x0000fd0a, 0x0000fd0c, 0x0000fc0f,
	0x0000fc12, 0x0001fb14, 0x0001fa17, 0x0001fa19,
	0x0001f91c, 0x0001f91f, 0x0001f822, 0x0001f824,
	0x0002f727, 0x0002f72a, 0x0002f72c, 0x0002f72f,
	0x0002f731, 0x0002f733, 0x0002f735, 0x0002f737,
	0x0002f73a, 0x0002f83b, 0x0002f93c, 0x0002fa3d,
	0x0001fb3e, 0x0001fc3f, 0x0001fd40, 0x0000fe40,

	0x00000000, 0x00000002, 0x0000ff04, 0x0000ff06,
	0x0000fe08, 0x0000fd0a, 0x0000fd0c, 0x0000fc0f,
	0x0000fc12, 0x0001fb14, 0x0001fa17, 0x0001fa19,
	0x0001f91c, 0x0001f91f, 0x0001f822, 0x0001f824,
	0x0002f727, 0x0002f72a, 0x0002f72c, 0x0002f72f,
	0x0002f731, 0x0002f733, 0x0002f735, 0x0002f737,
	0x0002f73a, 0x0002f83b, 0x0002f93c, 0x0002fa3d,
	0x0001fb3e, 0x0001fc3f, 0x0001fd40, 0x0000fe40,

	0x0002fc06, 0x0002fb08, 0x0002fb0a, 0x0002fa0c,
	0x0002fa0e, 0x0003f910, 0x0003f912, 0x0003f814,
	0x0003f816, 0x0003f719, 0x0003f71a, 0x0003f71d,
	0x0003f71f, 0x0003f721, 0x0003f723, 0x0003f725,
	0x0002f727, 0x0002f729, 0x0002f72b, 0x0002f82d,
	0x0002f82e, 0x0001f930, 0x0001fa31, 0x0000fa34,
	0x0000fb34, 0x0100fc35, 0x01fffd36, 0x01ffff37,
	0x01fe0037, 0x01fe0138, 0x01fd0338, 0x02fc0538,

	0x0002fa0b, 0x0002fa0c, 0x0002f90e, 0x0002f910,
	0x0002f911, 0x0002f813, 0x0002f816, 0x0002f817,
	0x0002f818, 0x0002f81a, 0x0001f81c, 0x0001f81e,
	0x0001f820, 0x0001f921, 0x0001f923, 0x0000f925,
	0x0000fa26, 0x0100fa28, 0x01fffb29, 0x01fffc2a,
	0x01fffc2c, 0x01fefd2d, 0x01fefe2e, 0x01fdff2f,
	0x01fd0030, 0x01fd0130, 0x01fc0232, 0x02fc0432,
	0x02fb0532, 0x02fb0633, 0x02fb0833, 0x02fa0933,

	0x0001fa0e, 0x0001f90f, 0x0001f911, 0x0001f913,
	0x0001f914, 0x0001f915, 0x0000f918, 0x0000fa18,
	0x0000fa1a, 0x0000fa1b, 0x0000fa1d, 0x00fffb1e,
	0x01fffb1f, 0x01fffb20, 0x01fffc22, 0x01fefc23,
	0x01fefd24, 0x01fefe25, 0x01fdfe27, 0x01fdff28,
	0x01fd0029, 0x01fc012a, 0x01fc022b, 0x01fc032b,
	0x01fb042d, 0x01fb052d, 0x01fb062e, 0x01fb072e,
	0x01fa092e, 0x01fa0a2f, 0x01fa0b2f, 0x01fa0d2f,

	0x0000fa11, 0x0000fa12, 0x0000fa13, 0x0000fb14,
	0x00fffb16, 0x00fffb16, 0x00fffb17, 0x00fffb19,
	0x00fffc1a, 0x00fefc1c, 0x00fefd1c, 0x01fefd1d,
	0x01fefe1e, 0x01fdfe20, 0x01fdff21, 0x01fdff22,
	0x01fd0023, 0x01fc0124, 0x01fc0124, 0x01fc0225,
	0x01fc0326, 0x01fc0427, 0x01fb0528, 0x01fb0629,
	0x01fb0729, 0x01fb0829, 0x01fb092a, 0x01fb0a2a,
	0x00fa0b2c, 0x00fa0c2b, 0x00fa0e2b, 0x00fa0f2c,

	0x00fffc11, 0x00fffc12, 0x00fffc14, 0x00fffc15,
	0x00fefd16, 0x00fefd17, 0x00fefd18, 0x00fefe19,
	0x00fefe1a, 0x00fdfe1d, 0x00fdff1d, 0x00fdff1e,
	0x00fd001d, 0x00fd011e, 0x00fd0120, 0x00fc0221,
	0x00fc0321, 0x00fc0323, 0x00fc0423, 0x00fc0523,
	0x00fc0624, 0x00fb0725, 0x00fb0726, 0x00fb0827,
	0x00fb0926, 0x00fb0a26, 0x00fb0b27, 0x00fb0c27,
	0x00fb0d27, 0xfffb0e28, 0xfffb0f29, 0xfffc1028,

	0x00fefd13, 0x00fefd13, 0x00fefe14, 0x00fefe15,
	0x00fefe17, 0x00feff17, 0x00feff17, 0x00fd0018,
	0x00fd001a, 0x00fd001a, 0x00fd011b, 0x00fd021c,
	0x00fd021c, 0x00fd031d, 0x00fc031f, 0x00fc041f,
	0x00fc051f, 0x00fc0521, 0x00fc0621, 0x00fc0721,
	0x00fc0821, 0x00fc0822, 0x00fc0922, 0x00fc0a23,
	0xfffc0b24, 0xfffc0c24, 0xfffc0d24, 0xfffc0d25,
	0xfffc0e25, 0xfffd0f25, 0xfffd1025, 0xfffd1125,

	0x00feff12, 0x00feff14, 0x00feff14, 0x00fe0015,
	0x00fe0015, 0x00fd0017, 0x00fd0118, 0x00fd0118,
	0x00fd0218, 0x00fd0219, 0x00fd031a, 0x00fd031a,
	0x00fd041b, 0x00fd041c, 0x00fd051c, 0x00fd061d,
	0x00fd061d, 0x00fd071e, 0x00fd081e, 0xfffd081f,
	0xfffd091f, 0xfffd0a20, 0xfffd0a20, 0xfffd0b21,
	0xfffd0c21, 0xfffd0d21, 0xfffd0d22, 0xfffd0e23,
	0xfffe0f22, 0xfefe1022, 0xfefe1122, 0xfefe1123,

	0x00fe0012, 0x00fe0013, 0x00fe0114, 0x00fe0114,
	0x00fe0116, 0x00fe0216, 0x00fe0216, 0x00fd0317,
	0x00fd0317, 0x00fd0418, 0x00fd0419, 0x00fd0519,
	0x00fd051a, 0x00fd061b, 0x00fd061b, 0x00fd071c,
	0xfffd071e, 0xfffd081d, 0xfffd091d, 0xfffd091e,
	0xfffe0a1d, 0xfffe0b1e, 0xfffe0b1e, 0xfffe0c1e,
	0xfffe0d1f, 0xfffe0d1f, 0xfffe0e1f, 0xfeff0f1f,
	0xfeff0f20, 0xfeff1020, 0xfeff1120, 0xfe001120,

	0x00fe0212, 0x00fe0312, 0x00fe0313, 0x00fe0314,
	0x00fe0414, 0x00fe0414, 0x00fe0416, 0x00fe0515,
	0x00fe0516, 0x00fe0616, 0x00fe0617, 0x00fe0717,
	0xfffe0719, 0xfffe0818, 0xffff0818, 0xffff0919,
	0xffff0919, 0xffff0a19, 0xffff0a1a, 0xffff0b1a,
	0xffff0b1b, 0xffff0c1a, 0xff000c1b, 0xff000d1b,
	0xff000d1b, 0xff000e1b, 0xff000e1c, 0xff010f1c,
	0xfe01101c, 0xfe01101d, 0xfe02111c, 0xfe02111c,

	0x00ff0411, 0x00ff0411, 0x00ff0412, 0x00ff0512,
	0x00ff0513, 0x00ff0513, 0x00ff0613, 0x00ff0614,
	0x00ff0714, 0x00ff0715, 0x00ff0715, 0xffff0816,
	0xffff0816, 0xff000916, 0xff000917, 0xff000918,
	0xff000a17, 0xff000a18, 0xff000b18, 0xff000b18,
	0xff010c18, 0xff010c19, 0xff010d18, 0xff010d18,
	0xff020d18, 0xff020e19, 0xff020e19, 0xff020f19,
	0xff030f19, 0xff031019, 0xff031019, 0xff031119,

	0x00ff0511, 0x00ff0511, 0x00000511, 0x00000611,
	0x00000612, 0x00000612, 0x00000712, 0x00000713,
	0x00000714, 0x00000814, 0x00000814, 0x00000914,
	0x00000914, 0xff010914, 0xff010a15, 0xff010a16,
	0xff010a17, 0xff010b16, 0xff010b16, 0xff020c16,
	0xff020c16, 0xff020c16, 0xff020d16, 0xff020d17,
	0xff030d17, 0xff030e17, 0xff030e17, 0xff030f17,
	0xff040f17, 0xff040f17, 0xff041017, 0xff051017,

	0x00000610, 0x00000610, 0x00000611, 0x00000611,
	0x00000711, 0x00000712, 0x00010712, 0x00010812,
	0x00010812, 0x00010812, 0x00010913, 0x00010913,
	0x00010913, 0x00010a13, 0x00020a13, 0x00020a14,
	0x00020b14, 0x00020b14, 0x00020b14, 0x00020c14,
	0x00030c14, 0x00030c15, 0x00030d15, 0x00030d15,
	0x00040d15, 0x00040e15, 0x00040e15, 0x00040e16,
	0x00050f15, 0x00050f15, 0x00050f16, 0x00051015,

	0x00000611, 0x00010610, 0x00010710, 0x00010710,
	0x00010711, 0x00010811, 0x00010811, 0x00010812,
	0x00010812, 0x00010912, 0x00020912, 0x00020912,
	0x00020a12, 0x00020a12, 0x00020a13, 0x00020a13,
	0x00030b13, 0x00030b13, 0x00030b14, 0x00030c13,
	0x00030c13, 0x00040c13, 0x00040d14, 0x00040d14,
	0x00040d15, 0x00040d15, 0x00050e14, 0x00050e14,
	0x00050e15, 0x00050f14, 0x00060f14, 0x00060f14,

	0x0001070f, 0x0001070f, 0x00010710, 0x00010710,
	0x00010810, 0x00010810, 0x00020810, 0x00020811,
	0x00020911, 0x00020911, 0x00020912, 0x00020912,
	0x00020a12, 0x00030a12, 0x00030a12, 0x00030b12,
	0x00030b12, 0x00030b12, 0x00040b12, 0x00040c12,
	0x00040c13, 0x00040c14, 0x00040c14, 0x00050d13,
	0x00050d13, 0x00050d14, 0x00050e13, 0x01050e13,
	0x01060e13, 0x01060e13, 0x01060e14, 0x01060f13
};

static const uint32_t lan2coefftab32[512] = {
	0x00004000, 0x000140ff, 0x00033ffe, 0x00043ffd, 0x00063efc, 0xff083dfc, 0x000a3bfb, 0xff0d39fb,
	0xff0f37fb, 0xff1136fa, 0xfe1433fb, 0xfe1631fb, 0xfd192ffb, 0xfd1c2cfb, 0xfd1f29fb, 0xfc2127fc,
	0xfc2424fc, 0xfc2721fc, 0xfb291ffd, 0xfb2c1cfd, 0xfb2f19fd, 0xfb3116fe, 0xfb3314fe, 0xfa3611ff,
	0xfb370fff, 0xfb390dff, 0xfb3b0a00, 0xfc3d08ff, 0xfc3e0600, 0xfd3f0400, 0xfe3f0300, 0xff400100,

	0x00004000, 0x000140ff, 0x00033ffe, 0x00043ffd, 0x00063efc, 0xff083dfc, 0x000a3bfb, 0xff0d39fb,
	0xff0f37fb, 0xff1136fa, 0xfe1433fb, 0xfe1631fb, 0xfd192ffb, 0xfd1c2cfb, 0xfd1f29fb, 0xfc2127fc,
	0xfc2424fc, 0xfc2721fc, 0xfb291ffd, 0xfb2c1cfd, 0xfb2f19fd, 0xfb3116fe, 0xfb3314fe, 0xfa3611ff,
	0xfb370fff, 0xfb390dff, 0xfb3b0a00, 0xfc3d08ff, 0xfc3e0600, 0xfd3f0400, 0xfe3f0300, 0xff400100,

	0xff053804, 0xff063803, 0xff083801, 0xff093701, 0xff0a3700, 0xff0c3500, 0xff0e34ff, 0xff1033fe,
	0xff1232fd, 0xfe1431fd, 0xfe162ffd, 0xfe182dfd, 0xfd1b2cfc, 0xfd1d2afc, 0xfd1f28fc, 0xfd2126fc,
	0xfd2323fd, 0xfc2621fd, 0xfc281ffd, 0xfc2a1dfd, 0xfc2c1bfd, 0xfd2d18fe, 0xfd2f16fe, 0xfd3114fe,
	0xfd3212ff, 0xfe3310ff, 0xff340eff, 0x00350cff, 0x00360a00, 0x01360900, 0x02370700, 0x03370600,

	0xff083207, 0xff093206, 0xff0a3205, 0xff0c3203, 0xff0d3103, 0xff0e3102, 0xfe113001, 0xfe132f00,
	0xfe142e00, 0xfe162dff, 0xfe182bff, 0xfe192aff, 0xfe1b29fe, 0xfe1d27fe, 0xfe1f25fe, 0xfd2124fe,
	0xfe2222fe, 0xfe2421fd, 0xfe251ffe, 0xfe271dfe, 0xfe291bfe, 0xff2a19fe, 0xff2b18fe, 0xff2d16fe,
	0x002e14fe, 0x002f12ff, 0x013010ff, 0x02300fff, 0x03310dff, 0x04310cff, 0x05310a00, 0x06310900,

	0xff0a2e09, 0xff0b2e08, 0xff0c2e07, 0xff0e2d06, 0xff0f2d05, 0xff102d04, 0xff122c03, 0xfe142c02,
	0xfe152b02, 0xfe172a01, 0xfe182901, 0xfe1a2800, 0xfe1b2700, 0xfe1d2500, 0xff1e24ff, 0xfe2023ff,
	0xff2121ff, 0xff2320fe, 0xff241eff, 0x00251dfe, 0x00261bff, 0x00281afe, 0x012818ff, 0x012a16ff,
	0x022a15ff, 0x032b13ff, 0x032c12ff, 0x052c10ff, 0x052d0fff, 0x062d0d00, 0x072d0c00, 0x082d0b00,

	0xff0c2a0b, 0xff0d2a0a, 0xff0e2a09, 0xff0f2a08, 0xff102a07, 0xff112a06, 0xff132905, 0xff142904,
	0xff162803, 0xff172703, 0xff182702, 0xff1a2601, 0xff1b2501, 0xff1c2401, 0xff1e2300, 0xff1f2200,
	0x00202000, 0x00211f00, 0x01221d00, 0x01231c00, 0x01251bff, 0x02251aff, 0x032618ff, 0x032717ff,
	0x042815ff, 0x052814ff, 0x052913ff, 0x06291100, 0x072a10ff, 0x082a0e00, 0x092a0d00, 0x0a2a0c00,

	0xff0d280c, 0xff0e280b, 0xff0f280a, 0xff102809, 0xff112808, 0xff122708, 0xff142706, 0xff152705,
	0xff162605, 0xff172604, 0xff192503, 0xff1a2403, 0x001b2302, 0x001c2202, 0x001d2201, 0x001e2101,
	0x011f1f01, 0x01211e00, 0x01221d00, 0x02221c00, 0x02231b00, 0x03241900, 0x04241800, 0x04251700,
	0x052616ff, 0x06261400, 0x072713ff, 0x08271100, 0x08271100, 0x09271000, 0x0a280e00, 0x0b280d00,

	0xff0e260d, 0xff0f260c, 0xff10260b, 0xff11260a, 0xff122609, 0xff132608, 0xff142508, 0xff152507,
	0x00152506, 0x00172405, 0x00182305, 0x00192304, 0x001b2203, 0x001c2103, 0x011d2002, 0x011d2002,
	0x011f1f01, 0x021f1e01, 0x02201d01, 0x03211c00, 0x03221b00, 0x04221a00, 0x04231801, 0x05241700,
	0x06241600, 0x07241500, 0x08251300, 0x09251200, 0x09261100, 0x0a261000, 0x0b260f00, 0x0c260e00,

	0xff0e250e, 0xff0f250d, 0xff10250c, 0xff11250b, 0x0011250a, 0x00132409, 0x00142408, 0x00152407,
	0x00162307, 0x00172306, 0x00182206, 0x00192205, 0x011a2104, 0x011b2004, 0x011c2003, 0x021c1f03,
	0x021e1e02, 0x031e1d02, 0x03201c01, 0x04201b01, 0x04211a01, 0x05221900, 0x05221801, 0x06231700,
	0x07231600, 0x07241500, 0x08241400, 0x09241300, 0x0a241200, 0x0b241100, 0x0c241000, 0x0d240f00,

	0x000e240e, 0x000f240d, 0x0010240c, 0x0011240b, 0x0013230a, 0x0013230a, 0x00142309, 0x00152308,
	0x00162208, 0x00172207, 0x01182106, 0x01192105, 0x011a2005, 0x021b1f04, 0x021b1f04, 0x021d1e03,
	0x031d1d03, 0x031e1d02, 0x041e1c02, 0x041f1b02, 0x05201a01, 0x05211901, 0x06211801, 0x07221700,
	0x07221601, 0x08231500, 0x09231400, 0x0a231300, 0x0a231300, 0x0b231200, 0x0c231100, 0x0d231000,

	0x000f220f, 0x0010220e, 0x0011220d, 0x0012220c, 0x0013220b, 0x0013220b, 0x0015210a, 0x0015210a,
	0x01162108, 0x01172008, 0x01182007, 0x02191f06, 0x02191f06, 0x021a1e06, 0x031a1e05, 0x031c1d04,
	0x041c1c04, 0x041d1c03, 0x051d1b03, 0x051e1a03, 0x061f1902, 0x061f1902, 0x07201801, 0x08201701,
	0x08211601, 0x09211501, 0x0a211500, 0x0b211400, 0x0b221300, 0x0c221200, 0x0d221100, 0x0e221000,

	0x0010210f, 0x0011210e, 0x0011210e, 0x0012210d, 0x0013210c, 0x0014200c, 0x0114200b, 0x0115200a,
	0x01161f0a, 0x01171f09, 0x02171f08, 0x02181e08, 0x03181e07, 0x031a1d06, 0x031a1d06, 0x041b1c05,
	0x041c1c04, 0x051c1b04, 0x051d1a04, 0x061d1a03, 0x071d1903, 0x071e1803, 0x081e1802, 0x081f1702,
	0x091f1602, 0x0a201501, 0x0b1f1501, 0x0b201401, 0x0c211300, 0x0d211200, 0x0e201200, 0x0e211100,

	0x00102010, 0x0011200f, 0x0012200e, 0x0013200d, 0x0013200d, 0x01141f0c, 0x01151f0b, 0x01151f0b,
	0x01161f0a, 0x02171e09, 0x02171e09, 0x03181d08, 0x03191d07, 0x03191d07, 0x041a1c06, 0x041b1c05,
	0x051b1b05, 0x051c1b04, 0x061c1a04, 0x071d1903, 0x071d1903, 0x081d1803, 0x081e1703, 0x091e1702,
	0x0a1f1601, 0x0a1f1502, 0x0b1f1501, 0x0c1f1401, 0x0d201300, 0x0d201300, 0x0e201200, 0x0f201100,

	0x00102010, 0x0011200f, 0x00121f0f, 0x00131f0e, 0x00141f0d, 0x01141f0c, 0x01141f0c, 0x01151e0c,
	0x02161e0a, 0x02171e09, 0x03171d09, 0x03181d08, 0x03181d08, 0x04191c07, 0x041a1c06, 0x051a1b06,
	0x051b1b05, 0x061b1a05, 0x061c1a04, 0x071c1904, 0x081c1903, 0x081d1803, 0x091d1703, 0x091e1702,
	0x0a1e1602, 0x0b1e1502, 0x0c1e1501, 0x0c1f1401, 0x0d1f1400, 0x0e1f1300, 0x0e1f1201, 0x0f1f1200,

	0x00111e11, 0x00121e10, 0x00131e0f, 0x00131e0f, 0x01131e0e, 0x01141d0e, 0x02151d0c, 0x02151d0c,
	0x02161d0b, 0x03161c0b, 0x03171c0a, 0x04171c09, 0x04181b09, 0x05181b08, 0x05191b07, 0x06191a07,
	0x061a1a06, 0x071a1906, 0x071b1905, 0x081b1805, 0x091b1804, 0x091c1704, 0x0a1c1703, 0x0a1c1604,
	0x0b1d1602, 0x0c1d1502, 0x0c1d1502, 0x0d1d1402, 0x0e1d1401, 0x0e1e1301, 0x0f1e1300, 0x101e1200,

	0x00111e11, 0x00121e10, 0x00131d10, 0x01131d0f, 0x01141d0e, 0x01141d0e, 0x02151c0d, 0x02151c0d,
	0x03161c0b, 0x03161c0b, 0x04171b0a, 0x04171b0a, 0x05171b09, 0x05181a09, 0x06181a08, 0x06191a07,
	0x07191907, 0x071a1906, 0x081a1806, 0x081a1806, 0x091a1805, 0x0a1b1704, 0x0a1b1704, 0x0b1c1603,
	0x0b1c1603, 0x0c1c1503, 0x0d1c1502, 0x0d1d1402, 0x0e1d1401, 0x0f1d1301, 0x0f1d1301, 0x101e1200,
};

static void
sunxi_mixer_vsu_init(struct sunxi_mixer_softc *sc, u_int src_w, u_int src_h,
    u_int crtc_w, u_int crtc_h, uint32_t pixel_format)
{
	const u_int hstep = (src_w << 16) / crtc_w;
	const u_int vstep = (src_h << 16) / crtc_h;

	const int hsub = drm_format_horz_chroma_subsampling(pixel_format);
	const int vsub = drm_format_vert_chroma_subsampling(pixel_format);

	const u_int src_cw = src_w / hsub;
	const u_int src_ch = src_h / vsub;

	VSU_WRITE(sc, VS_OUT_SIZE_REG, ((crtc_h - 1) << 16) | (crtc_w - 1));
	VSU_WRITE(sc, VS_Y_SIZE_REG, ((src_h - 1) << 16) | (src_w - 1));
	VSU_WRITE(sc, VS_Y_HSTEP_REG, hstep << 4);
	VSU_WRITE(sc, VS_Y_VSTEP_REG, vstep << 4);
	VSU_WRITE(sc, VS_Y_HPHASE_REG, 0);
	VSU_WRITE(sc, VS_Y_VPHASE0_REG, 0);
	VSU_WRITE(sc, VS_Y_VPHASE1_REG, 0);
	VSU_WRITE(sc, VS_C_SIZE_REG, ((src_ch - 1) << 16) | (src_cw - 1));
	VSU_WRITE(sc, VS_C_HSTEP_REG, (hstep / hsub) << 4);
	VSU_WRITE(sc, VS_C_VSTEP_REG, (vstep / vsub) << 4);
	VSU_WRITE(sc, VS_C_HPHASE_REG, 0);
	VSU_WRITE(sc, VS_C_VPHASE0_REG, 0);
	VSU_WRITE(sc, VS_C_VPHASE1_REG, 0);

	/* XXX */
	const u_int coef_base = 0;

	for (int i = 0; i < 32; i++) {
		VSU_WRITE(sc, VS_Y_HCOEF0_REG(i), lan3coefftab32_left[coef_base + i]);
		VSU_WRITE(sc, VS_Y_HCOEF1_REG(i), lan3coefftab32_right[coef_base + i]);
		VSU_WRITE(sc, VS_Y_VCOEF_REG(i), lan2coefftab32[coef_base + i]);
		VSU_WRITE(sc, VS_C_HCOEF0_REG(i), lan3coefftab32_left[coef_base + i]);
		VSU_WRITE(sc, VS_C_HCOEF1_REG(i), lan3coefftab32_right[coef_base + i]);
		VSU_WRITE(sc, VS_C_VCOEF_REG(i), lan2coefftab32[coef_base + i]);
	}

	/* Commit settings and enable scaler */
	VSU_WRITE(sc, VS_CTRL_REG, VS_CTRL_COEF_SWITCH_EN | VS_CTRL_EN);
}

static const u32 yuv2rgb[] = {
	0x000004A8, 0x00000000, 0x00000662, 0xFFFC865A,
	0x000004A8, 0xFFFFFE6F, 0xFFFFFCBF, 0x00021FF4,
	0x000004A8, 0x00000813, 0x00000000, 0xFFFBAE4A,
};

static void
sunxi_mixer_csc_init(struct sunxi_mixer_softc *sc, uint32_t pixel_format)
{
	const u_int crtc_index = drm_crtc_index(&sc->sc_crtc.base);

	for (int i = 0; i < __arraycount(yuv2rgb); i++)
		CSC_WRITE(sc, crtc_index, CSC_COEFF0_REG(0) + i * 4, yuv2rgb[i]);

	CSC_WRITE(sc, crtc_index, CSC_BYPASS_REG, CSC_BYPASS_DISABLE);
}

static void
sunxi_mixer_csc_disable(struct sunxi_mixer_softc *sc)
{
	const u_int crtc_index = drm_crtc_index(&sc->sc_crtc.base);

	CSC_WRITE(sc, crtc_index, CSC_BYPASS_REG, 0);
}

static int
sunxi_mixer_overlay_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
    struct drm_framebuffer *fb, int crtc_x, int crtc_y, u_int crtc_w, u_int crtc_h,
    uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	struct sunxi_mixer_plane *overlay = to_sunxi_mixer_plane(plane);
	struct sunxi_mixer_softc * const sc = overlay->sc;
	struct sunxi_drm_framebuffer *sfb = to_sunxi_drm_framebuffer(fb);
	uint32_t val;

	const u_int fbfmt = sunxi_mixer_overlay_format(fb->pixel_format);
	const uint64_t paddr = (uint64_t)sfb->obj->dmamap->dm_segs[0].ds_addr;

	const uint32_t input_size = (((src_h >> 16) - 1) << 16) | ((src_w >> 16) - 1);
	const uint32_t input_pos = ((src_y >> 16) << 16) | (src_x >> 16);

	OVL_V_WRITE(sc, OVL_V_MBSIZE(0), input_size);
	OVL_V_WRITE(sc, OVL_V_COOR(0), input_pos);

	/* Note: DRM and hardware's ideas of pitch 1 and 2 are swapped */

	OVL_V_WRITE(sc, OVL_V_PITCH0(0), fb->pitches[0]);
	OVL_V_WRITE(sc, OVL_V_PITCH1(0), fb->pitches[2]);
	OVL_V_WRITE(sc, OVL_V_PITCH2(0), fb->pitches[1]);

	const uint64_t paddr0 = paddr + fb->offsets[0] +
	    (src_x >> 16) * drm_format_plane_cpp(fb->pixel_format, 0) +
	    (src_y >> 16) * fb->pitches[0];
	const uint64_t paddr1 = paddr + fb->offsets[2] +
	    (src_x >> 16) * drm_format_plane_cpp(fb->pixel_format, 2) +
	    (src_y >> 16) * fb->pitches[2];
	const uint64_t paddr2 = paddr + fb->offsets[1] +
	    (src_x >> 16) * drm_format_plane_cpp(fb->pixel_format, 1) +
	    (src_y >> 16) * fb->pitches[1];

	OVL_V_WRITE(sc, OVL_V_TOP_HADD0, (paddr0 >> 32) & OVL_V_TOP_HADD_LAYER0);
	OVL_V_WRITE(sc, OVL_V_TOP_HADD1, (paddr1 >> 32) & OVL_V_TOP_HADD_LAYER0);
	OVL_V_WRITE(sc, OVL_V_TOP_HADD2, (paddr2 >> 32) & OVL_V_TOP_HADD_LAYER0);

	OVL_V_WRITE(sc, OVL_V_TOP_LADD0(0), paddr0 & 0xffffffff);
	OVL_V_WRITE(sc, OVL_V_TOP_LADD1(0), paddr1 & 0xffffffff);
	OVL_V_WRITE(sc, OVL_V_TOP_LADD2(0), paddr2 & 0xffffffff);

	OVL_V_WRITE(sc, OVL_V_SIZE, input_size);

	val = OVL_V_ATTCTL_LAY0_EN;
	val |= __SHIFTIN(fbfmt, OVL_V_ATTCTL_LAY_FBFMT);
	if (sunxi_mixer_overlay_rgb(fb->pixel_format) == true)
		val |= OVL_V_ATTCTL_VIDEO_UI_SEL;
	OVL_V_WRITE(sc, OVL_V_ATTCTL(0), val);

	/* Enable video scaler */
	sunxi_mixer_vsu_init(sc, src_w >> 16, src_h >> 16, crtc_w, crtc_h, fb->pixel_format);

	/* Enable colour space conversion for non-RGB formats */
	if (sunxi_mixer_overlay_rgb(fb->pixel_format) == false)
		sunxi_mixer_csc_init(sc, fb->pixel_format);
	else
		sunxi_mixer_csc_disable(sc);

	/* Set blender 1 input size */
	BLD_WRITE(sc, BLD_CH_ISIZE(1), ((crtc_h - 1) << 16) | (crtc_w - 1));
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
	struct sunxi_mixer_plane *overlay = to_sunxi_mixer_plane(plane);
	struct sunxi_mixer_softc * const sc = overlay->sc;
	uint32_t val;

	sunxi_mixer_csc_disable(sc);

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
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XRGB8888,
#if notyet
	DRM_FORMAT_VYUY,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_YUYV,
#endif
	DRM_FORMAT_YUV422,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_YUV411,
};

static int
sunxi_mixer_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct sunxi_mixer_softc * const sc = device_private(dev);
	struct drm_device *ddev;
	bus_size_t reg;

	if (!activate)
		return EINVAL;

	ddev = sunxi_drm_endpoint_device(ep);
	if (ddev == NULL) {
		DRM_ERROR("couldn't find DRM device\n");
		return ENXIO;
	}

	sc->sc_crtc.sc = sc;
	sc->sc_overlay.sc = sc;

	/* Initialize registers */
	for (reg = 0; reg < 0xc000; reg += 4)
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, 0);
	BLD_WRITE(sc, BLD_CTL(0), 0x03010301);
	BLD_WRITE(sc, BLD_CTL(1), 0x03010301);
	BLD_WRITE(sc, BLD_CTL(2), 0x03010301);
	BLD_WRITE(sc, BLD_CTL(3), 0x03010301);

	if (sc->sc_ovl_ui_count > 1)
		drm_crtc_init(ddev, &sc->sc_crtc.base, &sunxi_mixer0_crtc_funcs);
	else
		drm_crtc_init(ddev, &sc->sc_crtc.base, &sunxi_mixer1_crtc_funcs);
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

	return of_match_compat_data(faa->faa_phandle, compat_data);
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
	sc->sc_ovl_ui_count = of_search_compatible(phandle, compat_data)->data;

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
