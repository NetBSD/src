/* $NetBSD: tegra_apbdma.c,v 1.4 2017/09/23 23:58:18 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: tegra_apbdma.c,v 1.4 2017/09/23 23:58:18 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_apbdmareg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

#define	TEGRA_APBDMA_NCHAN	32

static void *	tegra_apbdma_acquire(device_t, const void *, size_t,
				     void (*)(void *), void *);
static void	tegra_apbdma_release(device_t, void *);
static int	tegra_apbdma_transfer(device_t, void *,
				      struct fdtbus_dma_req *);
static void	tegra_apbdma_halt(device_t, void *);

static const struct fdtbus_dma_controller_func tegra_apbdma_funcs = {
	.acquire = tegra_apbdma_acquire,
	.release = tegra_apbdma_release,
	.transfer = tegra_apbdma_transfer,
	.halt = tegra_apbdma_halt
};

static int	tegra_apbdma_match(device_t, cfdata_t, void *);
static void	tegra_apbdma_attach(device_t, device_t, void *);

static int	tegra_apbdma_intr(void *);

struct tegra_apbdma_softc;

struct tegra_apbdma_chan {
	struct tegra_apbdma_softc *ch_sc;
	u_int			ch_n;
	void			*ch_ih;
	void			(*ch_cb)(void *);
	void			*ch_cbarg;
	u_int			ch_req;
};

struct tegra_apbdma_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	struct tegra_apbdma_chan sc_chan[TEGRA_APBDMA_NCHAN];
};

CFATTACH_DECL_NEW(tegra_apbdma, sizeof(struct tegra_apbdma_softc),
	tegra_apbdma_match, tegra_apbdma_attach, NULL, NULL);

#define	APBDMA_READ(sc, reg)						\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	APBDMA_WRITE(sc, reg, val)					\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
tegra_apbdma_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"nvidia,tegra210-apbdma",
		"nvidia,tegra124-apbdma",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_apbdma_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_apbdma_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	int error;
	u_int n;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}
	rst = fdtbus_reset_get(phandle, "dma");
	if (rst == NULL) {
		aprint_error(": couldn't get reset dma\n");
		return;
	}

	fdtbus_reset_assert(rst);
	error = clk_enable(clk);
	if (error) {
		aprint_error(": couldn't enable clock dma: %d\n", error);
		return;
	}
	fdtbus_reset_deassert(rst);

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = phandle;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}
	for (n = 0; n < TEGRA_APBDMA_NCHAN; n++) {
		sc->sc_chan[n].ch_sc = sc;
		sc->sc_chan[n].ch_n = n;
	}

	aprint_naive("\n");
	aprint_normal(": APBDMA\n");

	/* Stop all channels */
	for (n = 0; n < TEGRA_APBDMA_NCHAN; n++)
		APBDMA_WRITE(sc, APBDMACHAN_CSR_REG(n), 0);

	/* Mask interrupts */
	APBDMA_WRITE(sc, APBDMA_IRQ_MASK_REG, 0);

	/* Global enable */
	APBDMA_WRITE(sc, APBDMA_COMMAND_REG, APBDMA_COMMAND_GEN);

	fdtbus_register_dma_controller(self, phandle, &tegra_apbdma_funcs);
}

static int
tegra_apbdma_intr(void *priv)
{
	struct tegra_apbdma_chan *ch = priv;
	struct tegra_apbdma_softc *sc = ch->ch_sc;
	const u_int n = ch->ch_n;
	uint32_t sta;

	sta = APBDMA_READ(sc, APBDMACHAN_STA_REG(n));
	APBDMA_WRITE(sc, APBDMACHAN_STA_REG(n), sta);	/* clear EOC */

	ch->ch_cb(ch->ch_cbarg);

	return 1;
}

static void *
tegra_apbdma_acquire(device_t dev, const void *data, size_t len,
    void (*cb)(void *), void *cbarg)
{
	struct tegra_apbdma_softc *sc = device_private(dev);
	struct tegra_apbdma_chan *ch;
	u_int n;
	char intrstr[128];

	if (len != 4)
		return NULL;

	const u_int req = be32dec(data);
	if (req > __SHIFTOUT_MASK(APBDMACHAN_CSR_REQ_SEL))
		return NULL;

	for (n = 0; n < TEGRA_APBDMA_NCHAN; n++) {
		ch = &sc->sc_chan[n];
		if (ch->ch_ih == NULL)
			break;
	}
	if (n >= TEGRA_APBDMA_NCHAN) {
		aprint_error_dev(dev, "no free DMA channel\n");
		return NULL;
	}

	if (!fdtbus_intr_str(sc->sc_phandle, n, intrstr, sizeof(intrstr))) {
		aprint_error_dev(dev, "failed to decode interrupt %u\n", n);
		return NULL;
	}

	ch->ch_ih = fdtbus_intr_establish(sc->sc_phandle, n, IPL_VM,
	    FDT_INTR_MPSAFE, tegra_apbdma_intr, ch);
	if (ch->ch_ih == NULL) {
		aprint_error_dev(dev, "failed to establish interrupt on %s\n",
		    intrstr);
		return NULL;
	}
	aprint_normal_dev(dev, "interrupting on %s (channel %u)\n", intrstr, n);

	ch->ch_cb = cb;
	ch->ch_cbarg = cbarg;
	ch->ch_req = req;

	/* Unmask interrupts for this channel */
	APBDMA_WRITE(sc, APBDMA_IRQ_MASK_SET_REG, __BIT(n));

	return ch;
}
static void
tegra_apbdma_release(device_t dev, void *priv)
{
	struct tegra_apbdma_softc *sc = device_private(dev);
	struct tegra_apbdma_chan *ch = priv;
	const u_int n = ch->ch_n;

	KASSERT(ch->ch_ih != NULL);

	/* Halt the channel */
	APBDMA_WRITE(sc, APBDMACHAN_CSR_REG(n), 0);

	/* Mask interrupts for this channel */
	APBDMA_WRITE(sc, APBDMA_IRQ_MASK_CLR_REG, __BIT(n));

	fdtbus_intr_disestablish(sc->sc_phandle, ch->ch_ih);

	ch->ch_cb = NULL;
	ch->ch_cbarg = NULL;
}

static int
tegra_apbdma_transfer(device_t dev, void *priv, struct fdtbus_dma_req *req)
    
{
	struct tegra_apbdma_softc *sc = device_private(dev);
	struct tegra_apbdma_chan *ch = priv;
	const u_int n = ch->ch_n;
	uint32_t csr = 0;
	uint32_t csre = 0;
	uint32_t ahb_seq = 0;
	uint32_t apb_seq = 0;

	/* Scatter-gather not supported */
	if (req->dreq_nsegs != 1)
		return EINVAL;

	/* Addresses must be aligned to 32-bits */
	if ((req->dreq_segs[0].ds_addr & 3) != 0 ||
	    (req->dreq_dev_phys & 3) != 0)
		return EINVAL;

	/* Length must be a multiple of 32-bits */
	if ((req->dreq_segs[0].ds_len & 3) != 0)
		return EINVAL;

	csr |= __SHIFTIN(ch->ch_req, APBDMACHAN_CSR_REQ_SEL);

	/*
	 * Set DMA transfer direction.
	 * APBDMACHAN_CSR_DIR=0 means "APB read to AHB write", and
	 * APBDMACHAN_CSR_DIR=1 means "AHB read to APB write".
	 */
	if (req->dreq_dir == FDT_DMA_WRITE)
		csr |= APBDMACHAN_CSR_DIR; 

	/*
	 * Generate interrupt when DMA block transfer completes.
	 */
	if (req->dreq_block_irq)
		csr |= APBDMACHAN_CSR_IE_EOC;

	/*
	 * Single or multiple block transfer
	 */
	if (!req->dreq_block_multi)
		csr |= APBDMACHAN_CSR_ONCE;

	/*
	 * Flow control enable
	 */
	if (req->dreq_flow)
		csr |= APBDMACHAN_CSR_FLOW;

	/*
	 * Route interrupt to CPU. 1 = CPU, 0 = COP
	 */
	ahb_seq |= APBDMACHAN_AHB_SEQ_INTR_ENB;

	/*
	 * AHB is a 32-bit bus.
	 */
	if (req->dreq_mem_opt.opt_bus_width != 32)
		return EINVAL;
	ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_BUS_WIDTH_32,
			     APBDMACHAN_AHB_SEQ_BUS_WIDTH);

	/*
	 * AHB data swap.
	 */
	if (req->dreq_mem_opt.opt_swap)
		ahb_seq |= APBDMACHAN_AHB_SEQ_DATA_SWAP;

	/*
	 * AHB burst size.
	 */
	switch (req->dreq_mem_opt.opt_burst_len) {
	case 32:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_BURST_1,
				     APBDMACHAN_AHB_SEQ_BURST);
		break;
	case 128:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_BURST_4,
				     APBDMACHAN_AHB_SEQ_BURST);
		break;
	case 256:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_BURST_8,
				     APBDMACHAN_AHB_SEQ_BURST);
		break;
	default:
		return EINVAL;
	}

	/*
	 * 2X double buffering mode. Only supported in run-multiple mode
	 * with no-wrap operations.
	 */
	if (req->dreq_mem_opt.opt_dblbuf) {
		if (req->dreq_mem_opt.opt_wrap_len != 0)
			return EINVAL;
		if (!req->dreq_block_multi)
			return EINVAL;
		ahb_seq |= APBDMACHAN_AHB_SEQ_DBL_BUF;
	}

	/*
	 * AHB address wrap.
	 */
	switch (req->dreq_mem_opt.opt_wrap_len) {
	case 0:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_WRAP_NO_WRAP,
				     APBDMACHAN_AHB_SEQ_WRAP);
		break;
	case 128:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_WRAP_32,
				     APBDMACHAN_AHB_SEQ_WRAP);
		break;
	case 256:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_WRAP_64,
				     APBDMACHAN_AHB_SEQ_WRAP);
		break;
	case 512:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_WRAP_128,
				     APBDMACHAN_AHB_SEQ_WRAP);
		break;
	case 1024:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_WRAP_256,
				     APBDMACHAN_AHB_SEQ_WRAP);
		break;
	case 2048:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_WRAP_512,
				     APBDMACHAN_AHB_SEQ_WRAP);
		break;
	case 4096:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_WRAP_1024,
				     APBDMACHAN_AHB_SEQ_WRAP);
		break;
	case 8192:
		ahb_seq |= __SHIFTIN(APBDMACHAN_AHB_SEQ_WRAP_2048,
				     APBDMACHAN_AHB_SEQ_WRAP);
		break;
	default:
		return EINVAL;
	}

	/*
	 * APB bus width.
	 */
	switch (req->dreq_dev_opt.opt_bus_width) {
	case 8:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_BUS_WIDTH_8,
				     APBDMACHAN_APB_SEQ_BUS_WIDTH);
		break;
	case 16:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_BUS_WIDTH_16,
				     APBDMACHAN_APB_SEQ_BUS_WIDTH);
		break;
	case 32:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_BUS_WIDTH_32,
				     APBDMACHAN_APB_SEQ_BUS_WIDTH);
		break;
	default:
		return EINVAL;
	}

	/*
	 * APB data swap.
	 */
	if (req->dreq_dev_opt.opt_swap)
		apb_seq |= APBDMACHAN_APB_SEQ_DATA_SWAP;

	/*
	 * APB address wrap-around window.
	 */
	switch (req->dreq_dev_opt.opt_wrap_len) {
	case 0:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_WRAP_NO_WRAP,
				     APBDMACHAN_APB_SEQ_WRAP);
		break;
	case 4:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_WRAP_1,
				     APBDMACHAN_APB_SEQ_WRAP);
		break;
	case 8:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_WRAP_2,
				     APBDMACHAN_APB_SEQ_WRAP);
		break;
	case 16:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_WRAP_4,
				     APBDMACHAN_APB_SEQ_WRAP);
		break;
	case 32:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_WRAP_8,
				     APBDMACHAN_APB_SEQ_WRAP);
		break;
	case 64:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_WRAP_16,
				     APBDMACHAN_APB_SEQ_WRAP);
		break;
	case 128:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_WRAP_32,
				     APBDMACHAN_APB_SEQ_WRAP);
		break;
	case 256:
		apb_seq |= __SHIFTIN(APBDMACHAN_APB_SEQ_WRAP_64,
				     APBDMACHAN_APB_SEQ_WRAP);
		break;
	default:
		return EINVAL;
	}

	/*
	 * Program all channel registers before setting the channel enable bit.
	 */
	APBDMA_WRITE(sc, APBDMACHAN_AHB_PTR_REG(n), req->dreq_segs[0].ds_addr);
	APBDMA_WRITE(sc, APBDMACHAN_APB_PTR_REG(n), req->dreq_dev_phys);
	APBDMA_WRITE(sc, APBDMACHAN_AHB_SEQ_REG(n), ahb_seq);
	APBDMA_WRITE(sc, APBDMACHAN_APB_SEQ_REG(n), apb_seq);
	APBDMA_WRITE(sc, APBDMACHAN_WCOUNT_REG(n), req->dreq_segs[0].ds_len);
	APBDMA_WRITE(sc, APBDMACHAN_CSRE_REG(n), csre);
	APBDMA_WRITE(sc, APBDMACHAN_CSR_REG(n), csr | APBDMACHAN_CSR_ENB);

	return 0;
}

static void
tegra_apbdma_halt(device_t dev, void *priv)
{
	struct tegra_apbdma_softc *sc = device_private(dev);
	struct tegra_apbdma_chan *ch = priv;
	const u_int n = ch->ch_n;
	uint32_t v;

	v = APBDMA_READ(sc, APBDMACHAN_CSR_REG(n));
	v &= ~APBDMACHAN_CSR_ENB;
	APBDMA_WRITE(sc, APBDMACHAN_CSR_REG(n), v);
}
