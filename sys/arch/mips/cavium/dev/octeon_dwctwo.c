/*	$NetBSD: octeon_dwctwo.c,v 1.2.2.5 2016/10/05 20:55:31 skrll Exp $	*/

/*
 * Copyright (c) 2015 Masao Uebayashi <uebayasi@tombiinc.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 2015 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_dwctwo.c,v 1.2.2.5 2016/10/05 20:55:31 skrll Exp $");

#include "opt_octeon.h"
#include "opt_usb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/workqueue.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_usbnreg.h>
#include <mips/cavium/dev/octeon_usbnvar.h>
#include <mips/cavium/dev/octeon_usbcreg.h>
#include <mips/cavium/dev/octeon_usbcvar.h>
#include <mips/cavium/octeonvar.h>

#include <dwc2/dwc2var.h>
#include <dwc2/dwc2.h>
#include "dwc2_core.h"

struct octeon_dwc2_softc {
	struct dwc2_softc sc_dwc2;
	/* USBC bus space tag */
	struct mips_bus_space sc_dwc2_bust;

	/* USBN bus space */
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_regh;
	bus_space_handle_t sc_reg2h;

	void *sc_ih;
};

static int		octeon_dwc2_match(device_t, struct cfdata *, void *);
static void		octeon_dwc2_attach(device_t, device_t, void *);
static uint32_t		octeon_dwc2_rd_4(void *, bus_space_handle_t,
			    bus_size_t);
static void		octeon_dwc2_wr_4(void *, bus_space_handle_t,
			    bus_size_t, uint32_t);
int			octeon_dwc2_set_dma_addr(device_t, bus_addr_t, int);
static inline void	octeon_dwc2_reg_assert(struct octeon_dwc2_softc *,
			    bus_size_t, uint64_t);
static inline void 	octeon_dwc2_reg_deassert(struct octeon_dwc2_softc *,
			    bus_size_t, uint64_t);
static inline uint64_t	octeon_dwc2_reg_rd(struct octeon_dwc2_softc *,
			    bus_size_t);
static inline void	octeon_dwc2_reg_wr(struct octeon_dwc2_softc *,
			    bus_size_t, uint64_t);
static inline void 	octeon_dwc2_reg2_assert(struct octeon_dwc2_softc *,
			    bus_size_t, uint64_t);
static inline void 	octeon_dwc2_reg2_deassert(struct octeon_dwc2_softc *,
			    bus_size_t, uint64_t);
static inline uint64_t	octeon_dwc2_reg2_rd(struct octeon_dwc2_softc *,
			    bus_size_t);
static inline void	octeon_dwc2_reg2_wr(struct octeon_dwc2_softc *,
			    bus_size_t, uint64_t);

static struct dwc2_core_params octeon_dwc2_params = {
	.otg_cap			= 2,	/* 2 - No HNP/SRP capable */
	.otg_ver			= 0,
	.dma_enable			= 1,
	.dma_desc_enable		= 0,
	.speed				= 0,	/* 0 - High Speed */
	.enable_dynamic_fifo		= 1,
	.en_multiple_tx_fifo		= 0,
	.host_rx_fifo_size		= 456,
	.host_nperio_tx_fifo_size	= 912,
	.host_perio_tx_fifo_size	= 256,
	.max_transfer_size		= 65535,
	.max_packet_count		= 511,
	.host_channels			= 8,
	.phy_type			= 1,	/* UTMI */
	.phy_utmi_width			= 16,	/* 16 bits */
	.phy_ulpi_ddr			= 0,
	.phy_ulpi_ext_vbus		= 0,
	.i2c_enable			= 0,
	.ulpi_fs_ls			= 0,
	.host_support_fs_ls_low_power	= 0,
	.host_ls_low_power_phy_clk	= 0,	/* 48 MHz */
	.ts_dline			= 0,
	.reload_ctl			= 0,
	.ahbcfg				= 0,	/* XXX */
	.uframe_sched			= 1,
	.external_id_pin_ctl		= -1,
	.hibernation			= -1,
};

CFATTACH_DECL_NEW(octeon_dwctwo, sizeof(struct octeon_dwc2_softc),
    octeon_dwc2_match, octeon_dwc2_attach, NULL, NULL);

static int
octeon_dwc2_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;

	return 1;
}

static void
octeon_dwc2_attach(device_t parent, device_t self, void *aux)
{
	struct octeon_dwc2_softc *sc = device_private(self);
	struct iobus_attach_args *aa = aux;
	uint64_t clk;
	int status;

	aprint_normal("\n");

	sc->sc_dwc2.sc_dev = self;
	sc->sc_bust = aa->aa_bust;

	sc->sc_dwc2_bust.bs_cookie = sc;
	sc->sc_dwc2_bust.bs_map = aa->aa_bust->bs_map;
	sc->sc_dwc2_bust.bs_unmap = aa->aa_bust->bs_unmap;
	sc->sc_dwc2_bust.bs_r_4 = octeon_dwc2_rd_4;
	sc->sc_dwc2_bust.bs_w_4 = octeon_dwc2_wr_4;

	sc->sc_dwc2.sc_iot = &sc->sc_dwc2_bust;
	sc->sc_dwc2.sc_bus.ub_dmatag = aa->aa_dmat;
	sc->sc_dwc2.sc_params = &octeon_dwc2_params;
	sc->sc_dwc2.sc_set_dma_addr = octeon_dwc2_set_dma_addr;

	status = bus_space_map(sc->sc_dwc2.sc_iot, USBC_BASE, USBC_SIZE,
	    0, &sc->sc_dwc2.sc_ioh);
	if (status != 0)
		panic("can't map USBC space");

	status = bus_space_map(sc->sc_bust, USBN_BASE, USBN_SIZE,
	    0, &sc->sc_regh);
	if (status != 0)
		panic("can't map USBN space");

	status = bus_space_map(sc->sc_bust, USBN_2_BASE, USBN_2_SIZE,
	    0, &sc->sc_reg2h);
	if (status != 0)
		panic("can't map USBN_2 space");

	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_CN50XX:
		/*
		 * 2. Configure the reference clock, PHY, and HCLK:
		 * a. Write USBN_CLK_CTL[POR] = 1 and
		 *    USBN_CLK_CTL[HRST,PRST,HCLK_RST] = 0
		 */
		clk = octeon_dwc2_reg_rd(sc, USBN_CLK_CTL_OFFSET);
		clk |= USBN_CLK_CTL_POR;
		clk &= ~(USBN_CLK_CTL_HRST | USBN_CLK_CTL_PRST |
		    USBN_CLK_CTL_HCLK_RST | USBN_CLK_CTL_ENABLE);
		/*
		 * b. Select the USB reference clock/crystal parameters by writing
		 *    appropriate values to USBN_CLK_CTL[P_C_SEL, P_RTYPE, P_COM_ON].
		 */
		/* XXX board specific */
		clk &= ~(USBN_CLK_CTL_P_C_SEL | USBN_CLK_CTL_P_RTYPE |
		    USBN_CLK_CTL_P_COM_ON);
		/*
		 * c. Select the HCLK via writing USBN_CLK_CTL[DIVIDE, DIVIDE2] and
		 *    setting USBN_CLK_CTL[ENABLE] = 1.
		 */
		/* XXX board specific */
		clk &= ~(USBN_CLK_CTL_DIVIDE | USBN_CLK_CTL_DIVIDE2);
		clk |= SET_USBN_CLK_CTL_DIVIDE(0x4ULL)
			| SET_USBN_CLK_CTL_DIVIDE2(0x0ULL);
		octeon_dwc2_reg_wr(sc, USBN_CLK_CTL_OFFSET, clk);
		/*
		 * d. Write USBN_CLK_CTL[HCLK_RST] = 1.
		 */
		octeon_dwc2_reg_assert(sc, USBN_CLK_CTL_OFFSET, USBN_CLK_CTL_HCLK_RST);
		/*
		 * e. Wait 64 core-clock cycles for HCLK to stabilize.
		 */
		delay(1);
		break;
	case MIPS_CN31XX:
	case MIPS_CN30XX:
		/*
		 * 2. If changing the HCLK divide value:
		 * a. write USBN_CLK_CTL[DIVIDE] with the new divide value.
		 */
		clk = octeon_dwc2_reg_rd(sc, USBN_CLK_CTL_OFFSET);
		clk |= 0x4ULL & USBN_CLK_CTL_DIVIDE;
		octeon_dwc2_reg_wr(sc, USBN_CLK_CTL_OFFSET, clk);
		/*
		 * b. Wait 64 core-clock cycles for HCLK to stabilize.
		 */
		delay(1);
		break;
	default:
		panic("unknown H/W type"); /* XXX */
	}

	/*
	 * 3. Program the power-on reset field in the USBN clock-control register:
	 *    USBN_CLK_CTL[POR] = 0
	 */
	octeon_dwc2_reg_deassert(sc, USBN_CLK_CTL_OFFSET, USBN_CLK_CTL_POR);
	/*
	 * 4. Wait 40 us for PHY clock to start (CN3xxx)
	 * 4. Wait 1 ms for PHY clock to start (CN50xx)
	 */
	delay(1000);

	/*
	 * 5. Program the Reset input from automatic test equipment field
	 *    in the USBP control and status register:
	 *    USBN_USBP_CTL_STATUS[ATE_RESET] = 1
	 */
	octeon_dwc2_reg_assert(sc, USBN_USBP_CTL_STATUS_OFFSET,
			USBN_USBP_CTL_STATUS_ATE_RESET);
	/*
	 * 6. Wait 10 cycles.
	 */
	delay(1);
	/*
	 * 7. Clear ATE_RESET field in the USBN clock-control register:
	 *    USBN_USBP_CTL_STATUS[ATE_RESET] = 0
	 */
	octeon_dwc2_reg_deassert(sc, USBN_USBP_CTL_STATUS_OFFSET,
			USBN_USBP_CTL_STATUS_ATE_RESET);
	/*
	 * 8. Program the PHY reset field in the USBN clock-control register:
	 *    USBN_CLK_CTL[PRST] = 1
	 */
	octeon_dwc2_reg_assert(sc, USBN_CLK_CTL_OFFSET, USBN_CLK_CTL_PRST);
	/*
	 * 9. Program the USBP control and status register to select host or device mode.
	 *    USBN_USBP_CTL_STATUS[HST_MODE] = 0 for host, = 1 for device
	 */
	octeon_dwc2_reg_deassert(sc, USBN_USBP_CTL_STATUS_OFFSET,
			USBN_USBP_CTL_STATUS_HST_MODE);
	/*
	 * 10. Wait 1 us.
	 */
	delay(1);

	/*
	 * 11. Program the hreset_n field in the USBN clock-control register:
	 *     USBN_CLK_CTL[HRST] = 1
	 */
	octeon_dwc2_reg_assert(sc, USBN_CLK_CTL_OFFSET, USBN_CLK_CTL_HRST);

	delay(1);

	/* Finally, enable clock */
	octeon_dwc2_reg_assert(sc, USBN_CLK_CTL_OFFSET, USBN_CLK_CTL_ENABLE);

	delay(10);

	status = dwc2_init(&sc->sc_dwc2);
	if (status != 0)
		panic("can't initialize dwc2, error=%d\n", status);

	sc->sc_dwc2.sc_child =
	    config_found(sc->sc_dwc2.sc_dev, &sc->sc_dwc2.sc_bus, usbctlprint);

	sc->sc_ih = octeon_intr_establish(ffs64(CIU_INTX_SUM0_USB) - 1,
	    IPL_VM, dwc2_intr, sc);
	if (sc->sc_ih == NULL)
		panic("can't establish common interrupt\n");
}

static uint32_t
octeon_dwc2_rd_4(void *v, bus_space_handle_t h, bus_size_t off)
{

	/* dwc2 uses little-endian addressing */
	return mips_lwu((h + off) ^ 4);
}

static void
octeon_dwc2_wr_4(void *v, bus_space_handle_t h, bus_size_t off,
    uint32_t val)
{

	/* dwc2 uses little-endian addressing */
	mips_sw((h + off) ^ 4, val);
}

int
octeon_dwc2_set_dma_addr(device_t self, dma_addr_t dma_addr, int ch)
{
	struct octeon_dwc2_softc *sc = device_private(self);

	octeon_dwc2_reg2_wr(sc,
	    USBN_DMA0_INB_CHN0_OFFSET + ch * 0x8, dma_addr);
	octeon_dwc2_reg2_wr(sc,
	    USBN_DMA0_OUTB_CHN0_OFFSET + ch * 0x8, dma_addr);
	return 0;
}


static inline void
octeon_dwc2_reg_assert(struct octeon_dwc2_softc *sc, bus_size_t offset,
    uint64_t bits)
{
	uint64_t value;

	value = octeon_dwc2_reg_rd(sc, offset);
	value |= bits;
	octeon_dwc2_reg_wr(sc, offset, value);
}

static inline void
octeon_dwc2_reg_deassert(struct octeon_dwc2_softc *sc, bus_size_t offset,
    uint64_t bits)
{
	uint64_t value;

	value = octeon_dwc2_reg_rd(sc, offset);
	value &= ~bits;
	octeon_dwc2_reg_wr(sc, offset, value);
}

static inline uint64_t
octeon_dwc2_reg_rd(struct octeon_dwc2_softc *sc, bus_size_t off)
{
	return bus_space_read_8(sc->sc_bust, sc->sc_regh, off);
}

static inline void
octeon_dwc2_reg_wr(struct octeon_dwc2_softc *sc, bus_size_t off, uint64_t val)
{
	bus_space_write_8(sc->sc_bust, sc->sc_regh, off, val);
	/* guarantee completion of the store operation on RSL registers*/
	bus_space_read_8(sc->sc_bust, sc->sc_regh, off);
}

static inline void
octeon_dwc2_reg2_assert(struct octeon_dwc2_softc *sc, bus_size_t off,
    uint64_t bits)
{
	uint64_t val;

	val = octeon_dwc2_reg2_rd(sc, off);
	val |= bits;
	octeon_dwc2_reg2_wr(sc, off, val);
}

static inline void
octeon_dwc2_reg2_deassert(struct octeon_dwc2_softc *sc, bus_size_t off,
    uint64_t bits)
{
	uint64_t val;

	val = octeon_dwc2_reg2_rd(sc, off);
	val &= ~bits;
	octeon_dwc2_reg2_wr(sc, off, val);
}

static inline uint64_t
octeon_dwc2_reg2_rd(struct octeon_dwc2_softc *sc, bus_size_t off)
{
	return bus_space_read_8(sc->sc_bust, sc->sc_reg2h, off);
}

static inline void
octeon_dwc2_reg2_wr(struct octeon_dwc2_softc *sc, bus_size_t off, uint64_t val)
{
	bus_space_write_8(sc->sc_bust, sc->sc_reg2h, off, val);
	/* guarantee completion of the store operation on RSL registers*/
	bus_space_read_8(sc->sc_bust, sc->sc_reg2h, off);
}
