/*-
 * Copyright (c) 2017 Mediatek Inc.
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
#ifdef _KERNEL_OPT
#include "opt_motg.h"
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/kmem.h>

#include <arm/mediatek/mtk_musb.h>
#include <dev/fdt/fdtvar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/motgvar.h>

/* #define MTKMUSB_DEBUG    1 */


#ifdef MTKMUSB_DEBUG
#define MTKMUSB_DBG(msg, ...)  aprint_normal(msg, ## __VA_ARGS__)
#else
#define MTKMUSB_DBG(msg, ...)
#endif

struct mtk_motg_softc {
	struct motg_softc		mtksc_motg;
	void *				mtksc_ih;
	struct fdtbus_phy *    		mtksc_phy;
	bus_addr_t			mtksc_addr;
	bus_size_t 			mtksc_size;
	bus_size_t 			mtksc_iobase;
	uint8_t				mtksc_loc_port;

	const struct mtk_hw_musb	*mtksc_hw;
	const struct fdt_attach_args	*mtksc_faa;

};

struct mtk_hw_musb {
		uint8_t spec;   /* 0: unknown, 1: USB1.1, 2: USB 2.0 */
		uint8_t hier;   /* range : (0, 4] */
		uint8_t type;   /* 0: unknown, 1: host, 2: Device, 3: dual role, 4: OTG */
		uint8_t hub;    /* bool Hub support or not*/
		
		uint8_t txhbcap;  /* means how many are there TX High BandWidth EP */
		uint8_t rxhbcap;  /* means how many are there RX High BandWidth EP*/
		uint8_t txcap;  /*  means how many are there TX EP */
		uint8_t rxcap;  /*  means how many are there RX EP */

		uint8_t dmacap; /* how many are there dma channel number */
		uint8_t qmu;    /* bool MTK USBQ support or not */ 
		uint8_t qmutxcap; /* means how many are there TX USBQ  */
		uint8_t qmurxcap; /* means how many are there RX USBQ  */

		uint32_t sramsz; /* how many are there in sram */
		uint8_t charger; /* 0: unknown, 1: No Charger, 2: WCP charger, 3: BC 1.1, 4: BC 1.2 */
		uint8_t lpm;     /* bool LPM support or not */
};
		
		

static __inline void
MGCBARR(struct mtk_motg_softc *mtksc)
{
	bus_space_barrier(mtksc->mtksc_motg.sc_iot, mtksc->mtksc_motg.sc_ioh, 0,
	    mtksc->mtksc_motg.sc_size,
	    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);
}

static __inline uint8_t
musb_readb(struct mtk_motg_softc *mtksc, bus_size_t offset)
{
	MGCBARR(mtksc);
	return bus_space_read_1(mtksc->mtksc_motg.sc_iot,
	    mtksc->mtksc_motg.sc_ioh, offset);
}

static __inline uint16_t
musb_readw(struct mtk_motg_softc *mtksc, bus_size_t offset)
{
	MGCBARR(mtksc);
	return bus_space_read_2(mtksc->mtksc_motg.sc_iot,
	    mtksc->mtksc_motg.sc_ioh, offset);
}

static __inline uint32_t
musb_readl(struct mtk_motg_softc *mtksc, bus_size_t offset)
{
	MGCBARR(mtksc);
	return bus_space_read_4(mtksc->mtksc_motg.sc_iot,
	    mtksc->mtksc_motg.sc_ioh, offset);
}

static __inline void
musb_writeb(struct mtk_motg_softc *mtksc, bus_size_t offset, uint8_t val)
{
	MGCBARR(mtksc);
	bus_space_write_1(mtksc->mtksc_motg.sc_iot, mtksc->mtksc_motg.sc_ioh,
	    offset, val);
	MTKMUSB_DBG("MUSB%iwriteb[%#lX] = %#x but %#X\n",
	    mtksc->mtksc_loc_port,
	    mtksc->mtksc_addr + mtksc->mtksc_iobase + offset, val,
	    musb_readb(mtksc, offset));
}

static __inline void
musb_writew(struct mtk_motg_softc *mtksc, bus_size_t offset, uint16_t val)
{
	MGCBARR(mtksc);
	bus_space_write_2(mtksc->mtksc_motg.sc_iot, mtksc->mtksc_motg.sc_ioh,
	    offset, val);
	MTKMUSB_DBG("MUSB%iwritew[%#lX] = %#x but %#X\n",
	    mtksc->mtksc_loc_port,
	    mtksc->mtksc_addr + mtksc->mtksc_iobase + offset, val,
	    musb_readw(mtksc, offset));
}

static __inline void
musb_writel(struct mtk_motg_softc *mtksc, bus_size_t offset, uint32_t val)
{
	MGCBARR(mtksc);
	bus_space_write_4(mtksc->mtksc_motg.sc_iot, mtksc->mtksc_motg.sc_ioh,
	    offset, val);
	MTKMUSB_DBG("MUSB%iwritel[%#lX] = %#x but %#X\n",
	    mtksc->mtksc_loc_port,
	    mtksc->mtksc_addr + mtksc->mtksc_iobase + offset, val,
	    musb_readl(mtksc, offset));
}

static const struct mtk_hw_musb mtk_musb[2] = {
	{
		.spec = 2,
		.hier = 1,
		.type = 3,
		.hub  = true,
		
		.txhbcap = 8,
		.rxhbcap = 8,
		.txcap = 8,
		.rxcap = 8,

		.dmacap = 8,
		.qmu = true,
		.qmutxcap = 4,
		.qmurxcap = 4,

		.sramsz = 8256,
		.charger = 1,
		.lpm = false
	}, {
		.spec = 2,
		.hier = 1,
		.type = 3,
		.hub  = true,
		
		.txhbcap = 8,
		.rxhbcap = 8,
		.txcap = 8,
		.rxcap = 8,
	
		.dmacap = 8,
		.qmu = true,
		.qmutxcap = 4,
		.qmurxcap = 4,
	
		.sramsz = 8256,
		.charger = 1,
		.lpm = false
	},
};

static int	mtk_musb_match(device_t, cfdata_t, void *);
static void	mtk_musb_attach(device_t, device_t, void *);
static void	mtk_musb_init(struct mtk_motg_softc *);
static int	mtk_musb_intr(void *);
static void	mtk_musb_poll(void *);

CFATTACH_DECL_NEW(mtk_musb, sizeof(struct mtk_motg_softc),
	mtk_musb_match, mtk_musb_attach, NULL, NULL);

static int
mtk_musb_match(device_t parent, cfdata_t cf, void *aux)
{
	static const char * const fdt_mtkmusbbus_compatible[] = { 
		"mediatek,mercury-usb20",
		NULL
		};
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	int match;

	/* Check compatible string */
	match = of_match_compatible(phandle, fdt_mtkmusbbus_compatible);

	return match;
}

static void
mtk_musb_attach(device_t parent, device_t self, void *aux)
{
	struct mtk_motg_softc *mtksc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;
	char intrstr[128];
	//struct clk *clk;
	//u_int n;



	MTKMUSB_DBG("\n");
	MTKMUSB_DBG(": OTG\n");

	MTKMUSB_DBG("mtk_musb_attach: faa->faa_name: %s\n", faa->faa_name);
	MTKMUSB_DBG("mtk_musb_attach: faa->faa_bst: %p\n", faa->faa_bst);
	MTKMUSB_DBG("mtk_musb_attach: faa->faa_a4x_bst: %p\n", faa->faa_a4x_bst);
	MTKMUSB_DBG("mtk_musb_attach: faa->faa_dmat: %p\n", faa->faa_dmat);
	MTKMUSB_DBG("mtk_musb_attach: faa->faa_phandle: %i\n", faa->faa_phandle);
	MTKMUSB_DBG("mtk_musb_attach: faa->faa_quiet: %i\n", faa->faa_quiet);

	mtksc->mtksc_faa			= faa;
	mtksc->mtksc_motg.sc_dev		= self;
	mtksc->mtksc_motg.sc_bus.ub_dmatag	= faa->faa_dmat;
	mtksc->mtksc_motg.sc_iot		= faa->faa_bst;

	error = fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size);
	if ( error != 0) {
		aprint_error(": couldn't get registers since %i\n", error);
		return;
	}
	aprint_error("mtk_motg_attach: addr: %#lX with %lu\n", addr, size);

	mtksc->mtksc_addr			= addr;
	mtksc->mtksc_size			= size;
	mtksc->mtksc_iobase			= 0;
	mtksc->mtksc_loc_port			= faa->faa_name[3] - '0';
	
	error = bus_space_map(faa->faa_bst, addr, size, 0,
	    &mtksc->mtksc_motg.sc_ioh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	mtksc->mtksc_motg.sc_size		= size;

	mtksc->mtksc_motg.sc_intr_poll		= mtk_musb_poll;
	mtksc->mtksc_motg.sc_intr_poll_arg	= mtksc;

	mtksc->mtksc_motg.sc_mode		= MOTG_MODE_HOST;
	mtksc->mtksc_motg.sc_ep_max		= 5;
	mtksc->mtksc_motg.sc_ep_fifosize	= 512;
	mtksc->mtksc_hw				= &mtk_musb[0];

#if 0
    /* Enable optional phy */
	mtksc->mtksc_phy = fdtbus_phy_get(phandle, faa->faa_name);
	if (mtksc->mtksc_phy) {
		aprint_error(": couldn't enable phy\n");
		return;
	}


	error = fdtbus_phy_enable(mtksc->mtksc_phy, true);
	 if (error != 0) {
		aprint_error(": couldn't enable phy since %i\n", error);
		return;
	}


	/* Enable clocks */
	for (n = 0; (clk = fdtbus_clock_get_index(phandle, n)) != NULL; n++) {
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock #%d\n", n);
			return;
		}
	}
#endif

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	mtksc->mtksc_ih = fdtbus_intr_establish(phandle, 0, IPL_USB, 0,
	    mtk_musb_intr, mtksc);
	if (mtksc->mtksc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	device_printf(self, "interrupting at irq %s cb: %p(%p)\n", intrstr,
	    mtk_musb_intr, mtksc);

	mtk_musb_init(mtksc);

	motg_init(&mtksc->mtksc_motg);
}

static void
mtk_musb_init(struct mtk_motg_softc *mtksc)
{
	uint8_t	busperf3;
	uint32_t val;
	uint8_t intusb = 0;
	uint16_t inttx = 0, intrx = 0;
	uint32_t intmtkusb;

	/* resolve CR ALPS01823375 */
	busperf3 = musb_readb(mtksc, 0x74);
	busperf3 &= ~(0x40);
	busperf3 |= 0x80;
	musb_writeb(mtksc, 0x74, busperf3);
	/* resolve CR ALPS01823375 */

	/* disable h/w debounce feature
	 * mac 0x070[ 14] = 1b'1
	 */
	val  = musb_readl(mtksc, 0x70);
	val |= (0x01 << 14);
	musb_writel(mtksc, 0x70, val);

	intmtkusb  = musb_readl(mtksc, USB_L1INTS);
	musb_writel(mtksc, USB_L1INTS, intmtkusb);

	intusb	   = musb_readb(mtksc, MUSB2_REG_INTUSB);
	musb_writeb(mtksc, MUSB2_REG_INTUSB, intusb);

	inttx  	   = musb_readw(mtksc, MUSB2_REG_INTTX);
	musb_writew(mtksc, MUSB2_REG_INTTX, inttx);

	intrx	   = musb_readw(mtksc, MUSB2_REG_INTRX);
	musb_writel(mtksc, MUSB2_REG_INTRX, intrx);

	musb_writel(mtksc, USB_L1INTM, MTK_USB_BUS_EVTS | MTK_USB_XFER_EVTS);

}

static int
mtk_musb_intr(void *priv)
{
	struct mtk_motg_softc *mtksc = priv;
	uint8_t intusb = 0;
	uint16_t inttx = 0;
	uint16_t intrx = 0;
	uint32_t intmtkusb;

	mutex_enter(&mtksc->mtksc_motg.sc_intr_lock);

	intmtkusb  = musb_readl(mtksc, USB_L1INTS);
	intmtkusb &= musb_readl(mtksc, USB_L1INTM);
	if (intmtkusb & (MTK_USB_BUS_EVTS | MTK_USB_XFER_EVTS | MTK_USB_ADVXFER_EVTS) ) {
		intusb	   = musb_readb(mtksc, MUSB2_REG_INTUSB);
		intusb 	  &= musb_readb(mtksc, MUSB2_REG_INTUSBE);
		inttx  	   = musb_readw(mtksc, MUSB2_REG_INTTX);
		inttx  	  &= musb_readw(mtksc, MUSB2_REG_INTTXE);
		intrx	   = musb_readw(mtksc, MUSB2_REG_INTRX);
		intrx  	  &= musb_readw(mtksc, MUSB2_REG_INTRXE);

		if (intusb || inttx || intrx) {
#ifdef MTKMUSB_DEBUG
			device_printf(mtksc->mtksc_motg.sc_dev,
				"mtk_musb_intr: mtk: 0x%08X, bus.0x%02x tx.0x%04x rx.0x%04x\n",
				intmtkusb, intusb, inttx, intrx);
#endif
			if (intusb)
				musb_writeb(mtksc, MUSB2_REG_INTUSB, intusb);

			if (inttx)
				musb_writew(mtksc, MUSB2_REG_INTTX, inttx);

			if (intrx)
				musb_writel(mtksc, MUSB2_REG_INTRX, intrx);

			musb_writel(mtksc, USB_L1INTS, intmtkusb);

			motg_intr(&mtksc->mtksc_motg, intrx, inttx, intusb);
		}
	}

	mutex_exit(&mtksc->mtksc_motg.sc_intr_lock);

	return 1;
}

static void
mtk_musb_poll(void *priv)
{
	mtk_musb_intr(priv);
}
