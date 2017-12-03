/* $NetBSD: awin_mp.c,v 1.1.20.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_mp.c,v 1.1.20.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/wscons/wsconsio.h>

struct awin_mp_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;
	kcondvar_t sc_cv;
	void *sc_ih;

	paddr_t sc_membase;
	size_t sc_memsize;

	uint32_t sc_intr_sts;
};

#define MP_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define MP_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	awin_mp_match(device_t, cfdata_t, void *);
static void	awin_mp_attach(device_t, device_t, void *);
static void	awin_mp_clkinit(struct awin_mp_softc *, bus_space_handle_t);

static int	awin_mp_intr(void *);
static int	awin_mp_wait(struct awin_mp_softc *);
static uint64_t	awin_mp_addr(struct awin_mp_softc *, uint32_t, uint32_t,
		    uint32_t, uint32_t);
static int	awin_mp_exec(struct awin_mp_softc *);

static int	awin_mp_fill(struct awin_mp_softc *,
		    const struct wsdisplayio_fill *);
static int	awin_mp_copy(struct awin_mp_softc *,
		    const struct wsdisplayio_copy *);

#ifdef DDB
void		awin_mp_dump_regs(void);
#endif

CFATTACH_DECL_NEW(awin_mp, sizeof(struct awin_mp_softc),
	awin_mp_match, awin_mp_attach, NULL, NULL);

static int
awin_mp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_mp_attach(device_t parent, device_t self, void *aux)
{
	struct awin_mp_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc->sc_cv, "awinmp");
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": Mixer processor\n");

	awin_mp_clkinit(sc, aio->aio_ccm_bsh);

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SCHED, IST_LEVEL,
	    awin_mp_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting at irq %d\n", loc->loc_intr);
}

static void
awin_mp_clkinit(struct awin_mp_softc *sc, bus_space_handle_t ccm_bsh)
{
	/* Set PLL7 to 297 MHz */
	awin_pll7_enable();

	switch (awin_chip_id()) {
	case AWIN_CHIP_ID_A31:
		/* Soft reset */
		awin_reg_set_clear(sc->sc_bst, ccm_bsh, AWIN_A31_AHB_RESET1_REG,
		    AWIN_A31_AHB_RESET1_MP_RST, 0);
		/* DRAM clock gating */
		awin_reg_set_clear(sc->sc_bst, ccm_bsh, AWIN_DRAM_CLK_REG,
		    AWIN_DRAM_CLK_DE_MP_DCLK_ENABLE, 0);
		/* MP clock source PLL7, 99 MHz, enable */
		awin_reg_set_clear(sc->sc_bst, ccm_bsh, AWIN_MP_CLK_REG,
		    __SHIFTIN(AWIN_CLK_SRC_SEL_MP_PLL7, AWIN_CLK_SRC_SEL) |
		    __SHIFTIN(2, AWIN_CLK_DIV_RATIO_M) |
		    AWIN_CLK_ENABLE,
		    AWIN_CLK_SRC_SEL | AWIN_CLK_DIV_RATIO_M);
		break;
	}

	/* Enable */
	awin_reg_set_clear(sc->sc_bst, ccm_bsh, AWIN_AHB_GATING1_REG,
	    AWIN_AHB_GATING1_MP, 0);
}

static int
awin_mp_intr(void *priv)
{
	struct awin_mp_softc *sc = priv;
	uint32_t sts;

	sts = MP_READ(sc, AWIN_MP_STS_REG);
	if (!sts)
		return 0;

	MP_WRITE(sc, AWIN_MP_STS_REG, sts);

	mutex_enter(&sc->sc_lock);
	sc->sc_intr_sts |= sts;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

	return 1;
}

static int
awin_mp_wait(struct awin_mp_softc *sc)
{
	int error = EINVAL;

	KASSERT(mutex_owned(&sc->sc_lock));

	sc->sc_intr_sts |= MP_READ(sc, AWIN_MP_STS_REG);

	for (;;) {
		if (sc->sc_intr_sts & AWIN_MP_STS_FINISHIRQ_FLAG)
			return 0;

		error = cv_timedwait(&sc->sc_cv, &sc->sc_lock, mstohz(200));
		if (error)
			break;
	}

	return error;
}

static uint64_t
awin_mp_addr(struct awin_mp_softc *sc, uint32_t offset, uint32_t stride,
    uint32_t x, uint32_t y)
{
	return (sc->sc_membase + (uint64_t)offset + stride * y + (x * 4)) * 8;
}

static int
awin_mp_exec(struct awin_mp_softc *sc)
{
	KASSERT(mutex_owned(&sc->sc_lock));

	sc->sc_intr_sts = 0;

	MP_WRITE(sc, AWIN_MP_STS_REG, MP_READ(sc, AWIN_MP_STS_REG));
	MP_WRITE(sc, AWIN_MP_CTL_REG, 0);
	MP_WRITE(sc, AWIN_MP_CTL_REG,
	    AWIN_MP_CTL_HWERRIRQ_EN |
	    AWIN_MP_CTL_FINISHIRQ_EN |
	    AWIN_MP_CTL_START_CTL |
	    AWIN_MP_CTL_MP_EN);

	return awin_mp_wait(sc);
}

static int
awin_mp_fill(struct awin_mp_softc *sc, const struct wsdisplayio_fill *fill)
{
	uint64_t outaddr;
	uint32_t w, h;

	KASSERT(mutex_owned(&sc->sc_lock));

	w = fill->x2 - fill->x1;
	h = fill->y2 - fill->y1;
	outaddr = awin_mp_addr(sc, fill->dst.offset, fill->dst.stride,
	    fill->x1, fill->y1);

	MP_WRITE(sc, AWIN_MP_IDMAGLBCTL_REG,
	    __SHIFTIN(AWIN_MP_IDMAGLBCTL_MEMSCANORDER_TD_LR,
		      AWIN_MP_IDMAGLBCTL_MEMSCANORDER));

	MP_WRITE(sc, AWIN_MP_IDMASIZE_REG(0),
	    __SHIFTIN(h - 1, AWIN_MP_IDMASIZE_HEIGHT) |
	    __SHIFTIN(w - 1, AWIN_MP_IDMASIZE_WIDTH));
	MP_WRITE(sc, AWIN_MP_IDMACOOR_REG(0), 0);
	MP_WRITE(sc, AWIN_MP_IDMASET_REG(0),
	    __SHIFTIN(AWIN_MP_IDMASET_IDMA_FMT_ARGB8888,
		      AWIN_MP_IDMASET_IDMA_FMT) |
	    __SHIFTIN(AWIN_MP_IDMASET_IDMA_ROPMIRCTL_NORMAL,
		      AWIN_MP_IDMASET_IDMA_ROPMIRCTL) |
	    AWIN_MP_IDMASET_IDMA_FCMODEN |
	    AWIN_MP_IDMASET_IDMA_EN);
	MP_WRITE(sc, AWIN_MP_IDMAFILLCOLOR_REG(0), fill->fg);

	MP_WRITE(sc, AWIN_MP_ROPIDX0CTL_REG,
	    __SHIFTIN(1, AWIN_MP_ROPIDXxCTL_NOD6_CTL) |
	    __SHIFTIN(1, AWIN_MP_ROPIDXxCTL_NOD4_CTL));

	MP_WRITE(sc, AWIN_MP_OUTSIZE_REG,
	    __SHIFTIN(h - 1, AWIN_MP_OUTSIZE_OUT_HEIGHT) |
	    __SHIFTIN(w - 1, AWIN_MP_OUTSIZE_OUT_WIDTH));
	MP_WRITE(sc, AWIN_MP_OUTH4ADD_REG,
	    __SHIFTIN((outaddr >> 32) & 0xf, AWIN_MP_OUTH4ADD_OUTCH0_H4ADD));
	MP_WRITE(sc, AWIN_MP_OUTL32ADD_REG(0), outaddr & 0xffffffff);
	MP_WRITE(sc, AWIN_MP_OUTLINEWIDTH_REG(0), fill->dst.stride * NBBY);
	MP_WRITE(sc, AWIN_MP_OUTCTL_REG,
	    AWIN_MP_OUTCTL_RND_EN |
	    __SHIFTIN(AWIN_MP_OUTCTL_OUT_FMT_ARGB8888, AWIN_MP_OUTCTL_OUT_FMT));

	return awin_mp_exec(sc);
}

static int
awin_mp_copy(struct awin_mp_softc *sc, const struct wsdisplayio_copy *copy)
{
	uint64_t inaddr, outaddr;
	uint32_t xoff, yoff;
	u_int mso;

	KASSERT(mutex_owned(&sc->sc_lock));

	xoff = yoff = 0;
	mso = AWIN_MP_IDMAGLBCTL_MEMSCANORDER_TD_LR;

	if (copy->src.offset == copy->dst.offset) {
		if (copy->dsty > copy->srcy) {
			mso = AWIN_MP_IDMAGLBCTL_MEMSCANORDER_DT_LR;
			yoff += (copy->height - 1);
		} else if (copy->dsty == copy->srcy) {
			if (copy->dstx > copy->srcx) {
				mso = AWIN_MP_IDMAGLBCTL_MEMSCANORDER_TD_RL;
				xoff += (copy->width - 1);
			}
		}
	}

	MP_WRITE(sc, AWIN_MP_IDMAGLBCTL_REG,
	    __SHIFTIN(mso, AWIN_MP_IDMAGLBCTL_MEMSCANORDER));

	inaddr = awin_mp_addr(sc, copy->src.offset, copy->src.stride,
	    copy->srcx, copy->srcy);
	outaddr = awin_mp_addr(sc, copy->dst.offset, copy->dst.stride,
	    copy->dstx + xoff, copy->dsty + yoff);

	MP_WRITE(sc, AWIN_MP_IDMA_H4ADD_REG,
	    __SHIFTIN((inaddr >> 32) & 0xf, AWIN_MP_IDMA_H4ADD_IDMA0_H4ADD));
	MP_WRITE(sc, AWIN_MP_IDMA_L32ADD_REG(0), inaddr & 0xffffffff);

	MP_WRITE(sc, AWIN_MP_IDMALINEWIDTH_REG(0), copy->src.stride * NBBY);
	MP_WRITE(sc, AWIN_MP_IDMASIZE_REG(0),
	    __SHIFTIN(copy->height - 1, AWIN_MP_IDMASIZE_HEIGHT) |
	    __SHIFTIN(copy->width - 1, AWIN_MP_IDMASIZE_WIDTH));
	MP_WRITE(sc, AWIN_MP_IDMACOOR_REG(0), 0);
	MP_WRITE(sc, AWIN_MP_IDMASET_REG(0),
	    __SHIFTIN(AWIN_MP_IDMASET_IDMA_FMT_ARGB8888,
		      AWIN_MP_IDMASET_IDMA_FMT) |
	    __SHIFTIN(AWIN_MP_IDMASET_IDMA_ROPMIRCTL_NORMAL,
		      AWIN_MP_IDMASET_IDMA_ROPMIRCTL) |
	    __SHIFTIN(0xff, AWIN_MP_IDMASET_IDMA_GLBALPHA) |
	    __SHIFTIN(1, AWIN_MP_IDMASET_IDMA_ALPHACTL) |
	    AWIN_MP_IDMASET_IDMA_EN);

	MP_WRITE(sc, AWIN_MP_ROPIDX0CTL_REG,
	    __SHIFTIN(1, AWIN_MP_ROPIDXxCTL_NOD6_CTL) |
	    __SHIFTIN(1, AWIN_MP_ROPIDXxCTL_NOD4_CTL));

	MP_WRITE(sc, AWIN_MP_OUTSIZE_REG,
	    __SHIFTIN(copy->height - 1, AWIN_MP_OUTSIZE_OUT_HEIGHT) |
	    __SHIFTIN(copy->width - 1, AWIN_MP_OUTSIZE_OUT_WIDTH));
	MP_WRITE(sc, AWIN_MP_OUTH4ADD_REG,
	    __SHIFTIN((outaddr >> 32) & 0xf, AWIN_MP_OUTH4ADD_OUTCH0_H4ADD));
	MP_WRITE(sc, AWIN_MP_OUTL32ADD_REG(0), outaddr & 0xffffffff);
	MP_WRITE(sc, AWIN_MP_OUTLINEWIDTH_REG(0), copy->dst.stride * NBBY);
	MP_WRITE(sc, AWIN_MP_OUTCTL_REG,
	    AWIN_MP_OUTCTL_RND_EN |
	    __SHIFTIN(AWIN_MP_OUTCTL_OUT_FMT_ARGB8888, AWIN_MP_OUTCTL_OUT_FMT));

	return awin_mp_exec(sc);
}

void
awin_mp_setbase(device_t dev, paddr_t base, size_t size)
{
	struct awin_mp_softc *sc = device_private(dev);

	sc->sc_membase = base;
	sc->sc_memsize = size;
}

int
awin_mp_ioctl(device_t dev, u_long cmd, void *data)
{
	struct awin_mp_softc *sc = device_private(dev);
	int error;

	if (sc->sc_membase == 0)
		return EPASSTHROUGH;

	mutex_enter(&sc->sc_lock);

	switch (cmd) {
	case WSDISPLAYIO_FILL:
		error = awin_mp_fill(sc, data);
		break;
	case WSDISPLAYIO_COPY:
		error = awin_mp_copy(sc, data);
		break;
	case WSDISPLAYIO_SYNC:
		error = awin_mp_wait(sc);
		break;
	default:
		error = EPASSTHROUGH;
		break;
	}

	mutex_exit(&sc->sc_lock);

	return error;
}

#ifdef DDB
void
awin_mp_dump_regs(void)
{
	struct awin_mp_softc *sc;
	device_t dev;
	uint32_t v;

	dev = device_find_by_driver_unit("awinmp", 0);
	if (dev == NULL)
		return;
	sc = device_private(dev);

	for (int i = 0; i <= AWIN_MP_CMDQUEADD_REG; i += 4) {
		v = MP_READ(sc, i);
		printf("MP: 0x%04x = 0x%08x\n", i, v);
	}
}
#endif
