/* $NetBSD: sun4i_dma.c,v 1.1.4.1 2018/04/16 01:59:53 pgoyette Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sun4i_dma.c,v 1.1.4.1 2018/04/16 01:59:53 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/bitops.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

#define	DMA_MAX_TYPES		2
#define	 DMA_TYPE_NORMAL	0
#define	 DMA_TYPE_DEDICATED	1
#define	DMA_MAX_CHANNELS	8
#define	DMA_MAX_DRQS		32

#define	DRQ_TYPE_SDRAM		0x16

#define	DMA_IRQ_EN_REG		0x00
#define	DMA_IRQ_PEND_STAS_REG	0x04
#define	 DMA_IRQ_PEND_STAS_END_MASK	0xaaaaaaaa
#define	NDMA_CTRL_REG(n)	(0x100 + (n) * 0x20)
#define	 NDMA_CTRL_LOAD			__BIT(31)
#define	 NDMA_CTRL_CONTI_EN		__BIT(30)
#define	 NDMA_CTRL_WAIT_STATE		__BITS(29,27)
#define	 NDMA_CTRL_DST_DATA_WIDTH	__BITS(26,25)
#define	 NDMA_CTRL_DST_BST_LEN		__BITS(24,23)
#define	 NDMA_CTRL_DST_ADDR_TYPE	__BIT(21)
#define	 NDMA_CTRL_DST_DRQ_TYPE		__BITS(20,16)
#define	 NDMA_CTRL_BC_MODE_SEL		__BIT(15)
#define	 NDMA_CTRL_SRC_DATA_WIDTH	__BITS(10,9)
#define	 NDMA_CTRL_SRC_BST_LEN		__BITS(8,7)
#define	 NDMA_CTRL_SRC_ADDR_TYPE	__BIT(5)
#define	 NDMA_CTRL_SRC_DRQ_TYPE		__BITS(4,0)
#define	NDMA_SRC_ADDR_REG(n)	(0x100 + (n) * 0x20 + 0x4)
#define	NDMA_DEST_ADDR_REG(n)	(0x100 + (n) * 0x20 + 0x8)
#define	NDMA_BC_REG(n)		(0x100 + (n) * 0x20 + 0xc)
#define	DDMA_CTRL_REG(n)	(0x300 + (n) * 0x20)
#define	 DDMA_CTRL_LOAD			__BIT(31)
#define	 DDMA_CTRL_BSY_STA		__BIT(30)
#define	 DDMA_CTRL_CONTI_EN		__BIT(29)
#define	 DDMA_CTRL_DST_DATA_WIDTH	__BITS(26,25)
#define	 DDMA_CTRL_DST_BST_LEN		__BITS(24,23)
#define	 DDMA_CTRL_DST_ADDR_MODE	__BITS(22,21)
#define	 DDMA_CTRL_DST_DRQ_TYPE		__BITS(20,16)
#define	 DDMA_CTRL_BC_MODE_SEL		__BIT(15)
#define	 DDMA_CTRL_SRC_DATA_WIDTH	__BITS(10,9)
#define	 DDMA_CTRL_SRC_BST_LEN		__BITS(8,7)
#define	 DDMA_CTRL_SRC_ADDR_MODE	__BITS(6,5)
#define	 DDMA_CTRL_SRC_DRQ_TYPE		__BITS(4,0)
#define	DDMA_SRC_ADDR_REG(n)	(0x300 + (n) * 0x20 + 0x4)
#define	DDMA_DEST_ADDR_REG(n)	(0x300 + (n) * 0x20 + 0x8)
#define	DDMA_BC_REG(n)		(0x300 + (n) * 0x20 + 0xc)
#define	DDMA_PARA_REG(n)	(0x300 + (n) * 0x20 + 0x18)
#define	 DDMA_PARA_DST_DATA_BLK_SIZE	__BITS(31,24)
#define	 DDMA_PARA_DST_WAIT_CLK_CYC	__BITS(23,16)
#define	 DDMA_PARA_SRC_DATA_BLK_SIZE	__BITS(15,8)
#define	 DDMA_PARA_SRC_WAIT_CLK_CYC	__BITS(7,0)
#define	 DDMA_PARA_VALUE				\
	  (__SHIFTIN(1, DDMA_PARA_DST_DATA_BLK_SIZE) |	\
	   __SHIFTIN(1, DDMA_PARA_SRC_DATA_BLK_SIZE) |	\
	   __SHIFTIN(2, DDMA_PARA_DST_WAIT_CLK_CYC) |	\
	   __SHIFTIN(2, DDMA_PARA_SRC_WAIT_CLK_CYC))

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun4i-a10-dma",		1 },
	{ NULL }
};

struct sun4idma_channel {
	uint8_t			ch_type;
	uint8_t			ch_index;
	uint32_t		ch_irqmask;
	void			(*ch_callback)(void *);
	void			*ch_callbackarg;
	u_int			ch_drq;
};

struct sun4idma_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	int			sc_phandle;
	void			*sc_ih;

	kmutex_t		sc_lock;

	struct sun4idma_channel	sc_chan[DMA_MAX_TYPES][DMA_MAX_CHANNELS];
};

#define DMA_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define DMA_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void *
sun4idma_acquire(device_t dev, const void *data, size_t len,
    void (*cb)(void *), void *cbarg)
{
	struct sun4idma_softc *sc = device_private(dev);
	struct sun4idma_channel *ch = NULL;
	const uint32_t *specifier = data;
	uint32_t irqen;
	uint8_t index;

	if (len != 8)
		return NULL;

	const u_int type = be32toh(specifier[0]);
	const u_int drq = be32toh(specifier[1]);

	if (type >= DMA_MAX_TYPES || drq >= DMA_MAX_DRQS)
		return NULL;

	mutex_enter(&sc->sc_lock);

	for (index = 0; index < DMA_MAX_CHANNELS; index++) {
		if (sc->sc_chan[type][index].ch_callback == NULL) {
			ch = &sc->sc_chan[type][index];
			ch->ch_callback = cb;
			ch->ch_callbackarg = cbarg;
			ch->ch_drq = drq;

			irqen = DMA_READ(sc, DMA_IRQ_EN_REG);
			irqen |= ch->ch_irqmask;
			DMA_WRITE(sc, DMA_IRQ_EN_REG, irqen);

			break;
		}
	}

	mutex_exit(&sc->sc_lock);

	return ch;
}

static void
sun4idma_release(device_t dev, void *priv)
{
	struct sun4idma_softc *sc = device_private(dev);
	struct sun4idma_channel *ch = priv;
	uint32_t irqen;

	mutex_enter(&sc->sc_lock);

	irqen = DMA_READ(sc, DMA_IRQ_EN_REG);
	irqen &= ~ch->ch_irqmask;
	DMA_WRITE(sc, DMA_IRQ_EN_REG, irqen);

	ch->ch_callback = NULL;
	ch->ch_callbackarg = NULL;

	mutex_exit(&sc->sc_lock);
}

static int
sun4idma_transfer_ndma(struct sun4idma_softc *sc, struct sun4idma_channel *ch,
   struct fdtbus_dma_req *req)
{
	uint32_t cfg, mem_cfg, dev_cfg, src, dst;
	uint32_t mem_width, dev_width, mem_burst, dev_burst;

	mem_width = req->dreq_mem_opt.opt_bus_width >> 4;
	dev_width = req->dreq_dev_opt.opt_bus_width >> 4;
	mem_burst = req->dreq_mem_opt.opt_burst_len == 1 ? 0 :
		    (req->dreq_mem_opt.opt_burst_len >> 3) + 1;
	dev_burst = req->dreq_dev_opt.opt_burst_len == 1 ? 0 :
		    (req->dreq_dev_opt.opt_burst_len >> 3) + 1;

	mem_cfg = __SHIFTIN(mem_width, NDMA_CTRL_SRC_DATA_WIDTH) |
	    __SHIFTIN(mem_burst, NDMA_CTRL_SRC_BST_LEN) |
	    __SHIFTIN(DRQ_TYPE_SDRAM, NDMA_CTRL_SRC_DRQ_TYPE);
	dev_cfg = __SHIFTIN(dev_width, NDMA_CTRL_SRC_DATA_WIDTH) |
	    __SHIFTIN(dev_burst, NDMA_CTRL_SRC_BST_LEN) |
	    __SHIFTIN(ch->ch_drq, NDMA_CTRL_SRC_DRQ_TYPE) |
	    NDMA_CTRL_SRC_ADDR_TYPE;

	if (req->dreq_dir == FDT_DMA_READ) {
		src = req->dreq_dev_phys;
		dst = req->dreq_segs[0].ds_addr;
		cfg = mem_cfg << 16 | dev_cfg;
	} else {
		src = req->dreq_segs[0].ds_addr;
		dst = req->dreq_dev_phys;
		cfg = dev_cfg << 16 | mem_cfg;
	}

	DMA_WRITE(sc, NDMA_SRC_ADDR_REG(ch->ch_index), src);
	DMA_WRITE(sc, NDMA_DEST_ADDR_REG(ch->ch_index), dst);
	DMA_WRITE(sc, NDMA_BC_REG(ch->ch_index), req->dreq_segs[0].ds_len);
	DMA_WRITE(sc, NDMA_CTRL_REG(ch->ch_index), cfg | NDMA_CTRL_LOAD);

	return 0;
}

static int
sun4idma_transfer_ddma(struct sun4idma_softc *sc, struct sun4idma_channel *ch,
   struct fdtbus_dma_req *req)
{
	uint32_t cfg, mem_cfg, dev_cfg, src, dst;
	uint32_t mem_width, dev_width, mem_burst, dev_burst;

	mem_width = req->dreq_mem_opt.opt_bus_width >> 4;
	dev_width = req->dreq_dev_opt.opt_bus_width >> 4;
	mem_burst = req->dreq_mem_opt.opt_burst_len == 1 ? 0 :
		    (req->dreq_mem_opt.opt_burst_len >> 3) + 1;
	dev_burst = req->dreq_dev_opt.opt_burst_len == 1 ? 0 :
		    (req->dreq_dev_opt.opt_burst_len >> 3) + 1;

	mem_cfg = __SHIFTIN(mem_width, DDMA_CTRL_SRC_DATA_WIDTH) |
	    __SHIFTIN(mem_burst, DDMA_CTRL_SRC_BST_LEN) |
	    __SHIFTIN(DRQ_TYPE_SDRAM, DDMA_CTRL_SRC_DRQ_TYPE) |
	    __SHIFTIN(0, DDMA_CTRL_SRC_ADDR_MODE);
	dev_cfg = __SHIFTIN(dev_width, DDMA_CTRL_SRC_DATA_WIDTH) |
	    __SHIFTIN(dev_burst, DDMA_CTRL_SRC_BST_LEN) |
	    __SHIFTIN(ch->ch_drq, DDMA_CTRL_SRC_DRQ_TYPE) |
	    __SHIFTIN(1, DDMA_CTRL_SRC_ADDR_MODE);

	if (req->dreq_dir == FDT_DMA_READ) {
		src = req->dreq_dev_phys;
		dst = req->dreq_segs[0].ds_addr;
		cfg = mem_cfg << 16 | dev_cfg;
	} else {
		src = req->dreq_segs[0].ds_addr;
		dst = req->dreq_dev_phys;
		cfg = dev_cfg << 16 | mem_cfg;
	}

	DMA_WRITE(sc, DDMA_SRC_ADDR_REG(ch->ch_index), src);
	DMA_WRITE(sc, DDMA_DEST_ADDR_REG(ch->ch_index), dst);
	DMA_WRITE(sc, DDMA_BC_REG(ch->ch_index), req->dreq_segs[0].ds_len);
	DMA_WRITE(sc, DDMA_PARA_REG(ch->ch_index), DDMA_PARA_VALUE);
	DMA_WRITE(sc, DDMA_CTRL_REG(ch->ch_index), cfg | DDMA_CTRL_LOAD);

	return 0;
}

static int
sun4idma_transfer(device_t dev, void *priv, struct fdtbus_dma_req *req)
{
	struct sun4idma_softc *sc = device_private(dev);
	struct sun4idma_channel *ch = priv;

	if (req->dreq_nsegs != 1)
		return EINVAL;

	if (ch->ch_type == DMA_TYPE_NORMAL)
		return sun4idma_transfer_ndma(sc, ch, req);
	else
		return sun4idma_transfer_ddma(sc, ch, req);
}

static void
sun4idma_halt(device_t dev, void *priv)
{
	struct sun4idma_softc *sc = device_private(dev);
	struct sun4idma_channel *ch = priv;

	if (ch->ch_type == DMA_TYPE_NORMAL)
		DMA_WRITE(sc, NDMA_CTRL_REG(ch->ch_index), 0);
	else
		DMA_WRITE(sc, DDMA_CTRL_REG(ch->ch_index), 0);
}

static const struct fdtbus_dma_controller_func sun4idma_funcs = {
	.acquire = sun4idma_acquire,
	.release = sun4idma_release,
	.transfer = sun4idma_transfer,
	.halt = sun4idma_halt
};

static int
sun4idma_intr(void *priv)
{
	struct sun4idma_softc *sc = priv;
	uint32_t pend, mask, bit;
	uint8_t type, index;

	pend = DMA_READ(sc, DMA_IRQ_PEND_STAS_REG);
	if (pend == 0)
		return 0;

	DMA_WRITE(sc, DMA_IRQ_PEND_STAS_REG, pend);

	pend &= DMA_IRQ_PEND_STAS_END_MASK;

	while ((bit = ffs32(pend)) != 0) {
		mask = __BIT(bit - 1);
		pend &= ~mask;
		type = ((bit - 1) / 2) / 8;
		index = ((bit - 1) / 2) % 8;

		if (sc->sc_chan[type][index].ch_callback == NULL)
			continue;
		sc->sc_chan[type][index].ch_callback(
		    sc->sc_chan[type][index].ch_callbackarg);
	}

	return 1;
}

static int
sun4idma_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sun4idma_attach(device_t parent, device_t self, void *aux)
{
	struct sun4idma_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	u_int index, type;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if ((clk = fdtbus_clock_get_index(phandle, 0)) == NULL ||
	    clk_enable(clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SCHED);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": DMA controller\n");

	DMA_WRITE(sc, DMA_IRQ_EN_REG, 0);
	DMA_WRITE(sc, DMA_IRQ_PEND_STAS_REG, ~0);

	for (type = 0; type < DMA_MAX_TYPES; type++) {
		for (index = 0; index < DMA_MAX_CHANNELS; index++) {
			struct sun4idma_channel *ch = &sc->sc_chan[type][index];
			ch->ch_type = type;
			ch->ch_index = index;
			ch->ch_irqmask = __BIT((type * 16) + (index * 2) + 1);
			ch->ch_callback = NULL;
			ch->ch_callbackarg = NULL;

			if (type == DMA_TYPE_NORMAL)
				DMA_WRITE(sc, NDMA_CTRL_REG(index), 0);
			else
				DMA_WRITE(sc, DDMA_CTRL_REG(index), 0);
		}
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_SCHED,
	    FDT_INTR_MPSAFE, sun4idma_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish interrupt on %s\n", intrstr);
		return;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);

	fdtbus_register_dma_controller(self, phandle, &sun4idma_funcs);
}

CFATTACH_DECL_NEW(sun4i_dma, sizeof(struct sun4idma_softc),
        sun4idma_match, sun4idma_attach, NULL, NULL);
