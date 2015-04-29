/*	$NetBSD: octeon_usbn.c,v 1.1 2015/04/29 08:32:01 hikaru Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: octeon_usbn.c,v 1.1 2015/04/29 08:32:01 hikaru Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/rnd.h>

#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <mips/locore.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_corereg.h>
#include <mips/cavium/dev/octeon_usbnreg.h>
#include <mips/cavium/dev/octeon_usbnvar.h>
#include <mips/cavium/dev/octeon_usbcvar.h>
#include <mips/cavium/octeonvar.h>

#ifdef USBN_DEBUG
int usbn_debug = 0;
#define DPRINTF(x) if (usbn_debug) printf x
#else
#define DPRINTF(x)
#endif

struct octeon_usbn_softc {
	struct octeon_usbc_softc	sc_usbc;	/* USB core softc */

	bus_space_tag_t		sc_bust;	/* iobus space */
	bus_space_handle_t	sc_regh;	/* usbn register space */
/*        bus_space_handle_t	sc_regh2;	|+ usbn register space +|*/
};
#ifdef USBN_DEBUG
struct octeon_usbc_reg;
#endif

static int		octeon_usbn_match(device_t, struct cfdata *, void *);
static void		octeon_usbn_attach(device_t, device_t, void *);

static inline void 	octeon_usbn_reg_assert(struct octeon_usbn_softc *, bus_size_t, u_int64_t);
static inline void 	octeon_usbn_reg_deassert(struct octeon_usbn_softc *, bus_size_t, u_int64_t);
static inline u_int64_t	octeon_usbn_reg_rd(struct octeon_usbn_softc *, bus_size_t);
static inline void	octeon_usbn_reg_wr(struct octeon_usbn_softc *, bus_size_t, u_int64_t);
static inline void 	octeon_usbn_reg2_assert(struct octeon_usbn_softc *, bus_size_t, u_int64_t);
static inline void 	octeon_usbn_reg2_deassert(struct octeon_usbn_softc *, bus_size_t, u_int64_t);
static inline u_int64_t	octeon_usbn_reg2_rd(struct octeon_usbn_softc *, bus_size_t);
static inline void	octeon_usbn_reg2_wr(struct octeon_usbn_softc *, bus_size_t, u_int64_t);
#ifdef USBN_DEBUG
static void		octeon_usbn_dumpregs(struct octeon_usbn_softc *);
#endif

CFATTACH_DECL_NEW(octeon_usbn, sizeof(struct octeon_usbn_softc),
    octeon_usbn_match, octeon_usbn_attach, NULL, NULL);

static int
octeon_usbn_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;
	int result = 0;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		goto out;
	if (cf->cf_unit != aa->aa_unitno)
		goto out;
	result = 1;

out:
	return result;
}

static void
octeon_usbn_attach(device_t parent, device_t self, void *aux)
{
	struct octeon_usbn_softc *sc = device_private(self);
	struct iobus_attach_args *aa = aux;
	int status;
	u_int64_t clk;
/*        u_int64_t ctlstat;*/

	aprint_normal("\n");

	sc->sc_usbc.sc_dev = self;
	sc->sc_usbc.sc_bus.hci_private = self;

	sc->sc_usbc.sc_bust = sc->sc_bust = aa->aa_bust;
	status = bus_space_map(aa->aa_bust, aa->aa_unit->addr, USBN_SIZE,
	    0, &sc->sc_regh);
	if (status != 0)
		panic(": can't map i/o space");
	sc->sc_usbc.sc_regh = sc->sc_regh;
	sc->sc_usbc.sc_bus.dmatag = aa->aa_dmat;

	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_CN50XX:
		/*
		 * 2. Configure the reference clock, PHY, and HCLK: 
		 * a. Write USBN_CLK_CTL[POR] = 1 and
		 *    USBN_CLK_CTL[HRST,PRST,HCLK_RST] = 0
		 */
		clk = octeon_usbn_reg_rd(sc, USBN_CLK_CTL_OFFSET);
		clk |= USBN_CLK_CTL_POR;
		clk &= ~(USBN_CLK_CTL_HRST | USBN_CLK_CTL_PRST | USBN_CLK_CTL_HCLK_RST);
		octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, clk);
		/* 
		 * b. Select the USB reference clock/crystal parameters by writing
		 *    appropriate values to USBN_CLK_CTL[P_C_SEL, P_RTYPE, P_COM_ON].
		 */
		clk &= ~(USBN_CLK_CTL_P_C_SEL | USBN_CLK_CTL_P_RTYPE | USBN_CLK_CTL_P_COM_ON);
		clk |= SET_USBN_CLK_CTL_P_C_SEL(0x2ULL)
			| SET_USBN_CLK_CTL_P_RTYPE(0x2ULL)
			| USBN_CLK_CTL_P_COM_ON;
		octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, clk);
		/*
		 * c. Select the HCLK via writing USBN_CLK_CTL[DIVIDE, DIVIDE2] and
		 *    setting USBN_CLK_CTL[ENABLE] = 1.
		 */
		clk &= ~(USBN_CLK_CTL_DIVIDE | USBN_CLK_CTL_DIVIDE2);
		clk |= SET_USBN_CLK_CTL_DIVIDE(0x4ULL)
			| SET_USBN_CLK_CTL_DIVIDE2(0x0ULL);
		octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, clk);
		octeon_usbn_reg_assert(sc, USBN_CLK_CTL_OFFSET, USBN_CLK_CTL_ENABLE);
		/*
		 * d. Write USBN_CLK_CTL[HCLK_RST] = 1.
		 */
		octeon_usbn_reg_assert(sc, USBN_CLK_CTL_OFFSET, USBN_CLK_CTL_HCLK_RST);
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
		clk = octeon_usbn_reg_rd(sc, USBN_CLK_CTL_OFFSET);
		clk |= 0x4ULL & USBN_CLK_CTL_DIVIDE;
		octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, clk);
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
	clk = octeon_usbn_reg_rd(sc, USBN_CLK_CTL_OFFSET);
	clk &= ~(USBN_CLK_CTL_POR | USBN_CLK_CTL_P_XENBN);
	clk |= USBN_CLK_CTL_P_RCLK;
	octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, clk);
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
	octeon_usbn_reg_assert(sc, USBN_USBP_CTL_STATUS_OFFSET,
			USBN_USBP_CTL_STATUS_ATE_RESET);
	/*
	 * 6. Wait 10 cycles.
	 */
	delay(1);
	/*
	 * 7. Clear ATE_RESET field in the USBN clock-control register:
	 *    USBN_USBP_CTL_STATUS[ATE_RESET] = 0
	 */
	octeon_usbn_reg_deassert(sc, USBN_USBP_CTL_STATUS_OFFSET,
			USBN_USBP_CTL_STATUS_ATE_RESET);
	/*
	 * 8. Program the PHY reset field in the USBN clock-control register:
	 *    USBN_CLK_CTL[PRST] = 1
	 */
	clk = octeon_usbn_reg_rd(sc, USBN_CLK_CTL_OFFSET);
	clk &= ~(USBN_CLK_CTL_POR | USBN_CLK_CTL_P_XENBN);
	clk |= USBN_CLK_CTL_P_RCLK;
	clk |= USBN_CLK_CTL_PRST;
	octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, clk);
	/*
	 * 9. Program the USBP control and status register to select host or device mode.
	 *    USBN_USBP_CTL_STATUS[HST_MODE] = 0 for host, = 1 for device
	 */
	octeon_usbn_reg_deassert(sc, USBN_USBP_CTL_STATUS_OFFSET,
			USBN_USBP_CTL_STATUS_HST_MODE);
	/*
	 * 10. Wait 1 us.
	 */
	delay(1);

	/*
	 * 11. Program the hreset_n field in the USBN clock-control register:
	 *     USBN_CLK_CTL[HRST] = 1
	 */
	clk |= USBN_CLK_CTL_HRST;
	octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, clk);

	delay(1);
#if 0
	/* PMC suggest */
	DPRINTF(("initialize USB PHY\n"));

	octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, 0x33064);
	delay(1);
	octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, 0x33024);
	delay(40);
	octeon_usbn_reg_wr(sc, USBN_USBP_CTL_STATUS_OFFSET, 0xB880001);
	delay(1);
	octeon_usbn_reg_wr(sc, USBN_USBP_CTL_STATUS_OFFSET, 0xB880000);
	octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, 0x33034);
	octeon_usbn_reg_wr(sc, USBN_USBP_CTL_STATUS_OFFSET, 0xB480000);
	delay(1);
	octeon_usbn_reg_wr(sc, USBN_CLK_CTL_OFFSET, 0x3303C);
	delay(20);
#endif
	octeon_usbn_reg2_assert(sc, USBN_CTL_STATUS_OFFSET,
		0x1);
/*                USBN_CTL_STATUS_INV_A2 | 0x1);*/
	delay(10);

#ifdef USBN_DEBUG
	octeon_usbn_dumpregs(sc);
#endif

	status = octeon_usbc_init((octeon_usbc_softc_t *)sc);
/*        status = octeon_usbc_init(&sc->sc_usbc);*/
	if (USBD_NORMAL_COMPLETION != status) {
		aprint_error("%s: init failed, error=%d\n", 
		    device_xname(sc->sc_usbc.sc_dev), status);
		return;
	}
#ifdef USBN_DEBUG
	else
		aprint_normal("%s: attached\n",
		    device_xname(sc->sc_usbc.sc_dev));
#endif

	sc->sc_usbc.sc_child = config_found((void *)sc, &sc->sc_usbc.sc_bus,
					    usbctlprint);
}

#define USBN_CALC_OFFSET(base,n) ( (base) + ( 0x08 * (n) ) )
void
octeon_usbn_set_phyaddr(void *v, usb_dma_t *buffdma, int ch_num, u_int8_t direction, u_int32_t offset)
{
	struct octeon_usbn_softc *sc = (struct octeon_usbn_softc *) v;
	u_int64_t value = 0;

	value |= DMAADDR(buffdma, offset);
/*        value |= buffdma;*/

	DPRINTF(("octeon_usbn_set_phyaddr: dmaaddr=%llx, kernaddr=%p, ch_num=%d\n",
				value, KERNADDR(buffdma, offset), ch_num));
/*        if (direction == USBC_ENDPT_DIRECTION_IN) {*/
		octeon_usbn_reg2_wr(sc,
			USBN_CALC_OFFSET(USBN_DMA0_INB_CHN0_OFFSET, ch_num),
			value & USBN_DMA0_INB_CHNX_ADDR);
/*        } else if (direction == USBC_ENDPT_DIRECTION_OUT){*/
		octeon_usbn_reg2_wr(sc,
			USBN_CALC_OFFSET(USBN_DMA0_OUTB_CHN0_OFFSET, ch_num),
			value & USBN_DMA0_OUTB_CHNX_ADDR);
/*        } else {*/
/*                panic("unknown direction");*/
/*        }*/
#ifdef USBN_DEBUG
	{
		u_int64_t value;
		value = octeon_usbn_reg2_rd(sc,
			USBN_CALC_OFFSET(USBN_DMA0_INB_CHN0_OFFSET, ch_num));
		DPRINTF(("\tUSBN_DMA0_INB_CHN%d     : %llx\n", ch_num, value));

		value = octeon_usbn_reg2_rd(sc,
			USBN_CALC_OFFSET(USBN_DMA0_OUTB_CHN0_OFFSET, ch_num));
		DPRINTF(("\tUSBN_DMA0_OUTB_CHN%d    : %llx\n", ch_num, value));
	}
#endif
}
#undef USBN_CALC_OFFSET

static inline void
octeon_usbn_reg_assert(struct octeon_usbn_softc *sc, bus_size_t offset, u_int64_t bits)
{
	u_int64_t value;
	
	value = octeon_usbn_reg_rd(sc, offset);
	value |= bits;
	octeon_usbn_reg_wr(sc, offset, value);
}

static inline void
octeon_usbn_reg_deassert(struct octeon_usbn_softc *sc, bus_size_t offset, u_int64_t bits)
{
	u_int64_t value;
	
	value = octeon_usbn_reg_rd(sc, offset);
	value &= ~bits;
	octeon_usbn_reg_wr(sc, offset, value);
}

static inline u_int64_t
octeon_usbn_reg_rd(struct octeon_usbn_softc *sc, bus_size_t offset)
{
/*        return bus_space_read_8(sc->sc_bust, sc->sc_regh, offset);*/
	return octeon_read_csr(USBN_BASE + offset);
}

static inline void
octeon_usbn_reg_wr(struct octeon_usbn_softc *sc, bus_size_t offset, u_int64_t value)
{
	bus_space_write_8(sc->sc_bust, sc->sc_regh, offset, value);
	/* guarantee completion of the store operation on RSL registers*/
	bus_space_read_8(sc->sc_bust, sc->sc_regh, offset);
}

static inline void
octeon_usbn_reg2_assert(struct octeon_usbn_softc *sc, bus_size_t offset, u_int64_t bits)
{
	u_int64_t value;
	
	value = octeon_usbn_reg_rd(sc, offset);
	value |= bits;
	octeon_usbn_reg2_wr(sc, offset, value);
}

static inline void
octeon_usbn_reg2_deassert(struct octeon_usbn_softc *sc, bus_size_t offset, u_int64_t bits)
{
	u_int64_t value;
	
	value = octeon_usbn_reg_rd(sc, offset);
	value &= ~bits;
	octeon_usbn_reg2_wr(sc, offset, value);
}

static inline u_int64_t
octeon_usbn_reg2_rd(struct octeon_usbn_softc *sc, bus_size_t offset)
{
/*        return bus_space_read_8(sc->sc_bust, sc->sc_regh2, offset);*/
/*        return mips3_o32_64_ld(MIPS64_PHYS_TO_XKPHYS((USBN_2_BASE + offset)));*/
	return octeon_read_csr(USBN_2_BASE + offset);
}

static inline void
octeon_usbn_reg2_wr(struct octeon_usbn_softc *sc, bus_size_t offset, u_int64_t value)
{
/*        bus_space_write_8(sc->sc_bust, sc->sc_regh2, offset, value);*/
/*        mips3_o32_64_sd(MIPS64_PHYS_TO_XKPHYS((USBN_2_BASE + offset)), value);*/
	octeon_write_csr(USBN_2_BASE + offset, value);
	/* guarantee completion of the store operation on RSL registers*/
/*        bus_space_read_8(sc->sc_bust, sc->sc_regh2, offset);*/
/*        mips3_o32_64_ld(MIPS64_PHYS_TO_XKPHYS((USBN_2_BASE + offset)));*/
	octeon_read_csr(USBN_2_BASE + offset);
}

#ifdef USBN_DEBUG
void
octeon_usbn_dumpregs(struct octeon_usbn_softc *sc)
{
	u_int64_t value;
	char buf[2048];

	value = octeon_usbn_reg_rd(sc, USBN_CLK_CTL_OFFSET);
	snprintb(buf, sizeof(buf), USBN_CLK_CTL_BITS, value);
	DPRINTF(("\t%-24s: %s\n", "USBN_CLK_CTL", buf));
	value = octeon_usbn_reg_rd(sc, USBN_USBP_CTL_STATUS_OFFSET);
	snprintb(buf, sizeof(buf), USBN_USBP_CTL_STATUS_BITS, value);
	DPRINTF(("\t%-24s: %s\n", "USBN_USBP_CTL_STATUS", buf));
	value = octeon_usbn_reg_rd(sc, USBN_BIST_STATUS_OFFSET);
	snprintb(buf, sizeof(buf), USBN_BIST_STATUS_BITS, value);
	DPRINTF(("\t%-24s: %s\n", "USBN_BIST_STATUS", buf));

	value = octeon_usbn_reg2_rd(sc, USBN_CTL_STATUS_OFFSET);
	snprintb(buf, sizeof(buf), USBN_CTL_STATUS_BITS, value);
	DPRINTF(("\t%-24s: %s\n", "USBN_CTL_STATUS", buf));
	value = octeon_usbn_reg2_rd(sc, USBN_DMA_TEST_OFFSET);
	snprintb(buf, sizeof(buf), USBN_DMA_TEST_BITS, value);
	DPRINTF(("\t%-24s: %s\n", "USBN_DMA_TEST", buf));
}
void octeon_usbn_debug_DMA_TEST_reg_dump(void * v)
{
	u_int64_t value;
	char buf[2048];
	struct octeon_usbn_softc *sc = (struct octeon_usbn_softc *)v;

	value = octeon_usbn_reg2_rd(sc, USBN_DMA_TEST_OFFSET);
	snprintb(buf, sizeof(buf), USBN_DMA_TEST_BITS, value);
	DPRINTF(("\t%-24s: %s\n", "USBN_DMA_TEST", buf));
}
inline void
octeon_usbn_debug_dumpregs(void *v)
{
	struct octeon_usbn_softc *sc = (struct octeon_usbn_softc *)v;
	octeon_usbn_dumpregs(sc);
}
#endif
