/* $NetBSD: awin_ir.c,v 1.6.16.2 2017/12/03 11:35:51 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: awin_ir.c,v 1.6.16.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/select.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/ir/ir.h>
#include <dev/ir/cirio.h>
#include <dev/ir/cirvar.h>

#define AWIN_IR_RXSTA_MASK	__BITS(6,0)

struct awin_ir_softc {
	device_t sc_dev;
	device_t sc_cirdev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;
	kcondvar_t sc_cv;
	device_t sc_i2cdev;
	void *sc_ih;
	size_t sc_avail;
	int sc_port;
};

#define IR_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define IR_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	awin_ir_match(device_t, cfdata_t, void *);
static void	awin_ir_attach(device_t, device_t, void *);

static void	awin_ir_init(struct awin_ir_softc *,
			     struct awinio_attach_args * const);

static int	awin_ir_intr(void *);

static int	awin_ir_open(void *, int, int, struct proc *);
static int	awin_ir_close(void *, int, int, struct proc *);
static int	awin_ir_read(void *, struct uio *, int);
static int	awin_ir_write(void *, struct uio *, int);
static int	awin_ir_setparams(void *, struct cir_params *);

#ifdef DDB
void		awin_ir_dump_regs(void);
#endif

static const struct cir_methods awin_ir_methods = {
	.im_open = awin_ir_open,
	.im_close = awin_ir_close,
	.im_read = awin_ir_read,
	.im_write = awin_ir_write,
	.im_setparams = awin_ir_setparams,
};

CFATTACH_DECL_NEW(awin_ir, sizeof(struct awin_ir_softc),
	awin_ir_match, awin_ir_attach, NULL, NULL);

static int
awin_ir_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_ir_attach(device_t parent, device_t self, void *aux)
{
	struct awin_ir_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	struct ir_attach_args iaa;
	bus_space_handle_t bsh = awin_chip_id() == AWIN_CHIP_ID_A80 ?
	    aio->aio_a80_rcpus_bsh : aio->aio_core_bsh;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_port = loc->loc_port;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_IR);
	cv_init(&sc->sc_cv, "awinir");
	bus_space_subregion(sc->sc_bst, bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": IR\n");

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_IR, IST_LEVEL,
	    awin_ir_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	awin_ir_init(sc, aio);

	memset(&iaa, 0, sizeof(iaa));
	iaa.ia_type = IR_TYPE_CIR;
	iaa.ia_methods = &awin_ir_methods;
	iaa.ia_handle = sc;
	sc->sc_cirdev = config_found_ia(self, "irbus", &iaa, ir_print);
}

static void
awin_ir_init(struct awin_ir_softc *sc, struct awinio_attach_args * const aio)
{
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		const struct awin_gpio_pinset pinset =
		    { 'L', AWIN_A31_PIO_PL_IR_FUNC, AWIN_A31_PIO_PL_IR_PINS,
		           GPIO_PIN_PULLUP };
		bus_space_handle_t prcm_bsh;
		bus_size_t prcm_size = 0x200;
		uint32_t clk, reset, gating;
		
		bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
		    AWIN_A31_PRCM_OFFSET, prcm_size, &prcm_bsh);

		awin_gpio_pinset_acquire(&pinset);

		gating = bus_space_read_4(sc->sc_bst, prcm_bsh,
		    AWIN_A31_PRCM_APB0_GATING_REG);
		gating |= AWIN_A31_PRCM_APB0_GATING_CIR;
		bus_space_write_4(sc->sc_bst, prcm_bsh,
		    AWIN_A31_PRCM_APB0_GATING_REG, gating);

		reset = bus_space_read_4(sc->sc_bst, prcm_bsh,
		    AWIN_A31_PRCM_APB0_RESET_REG);
		reset |= AWIN_A31_PRCM_APB0_RESET_CIR;
		bus_space_write_4(sc->sc_bst, prcm_bsh,
		    AWIN_A31_PRCM_APB0_RESET_REG, reset);

		clk = bus_space_read_4(sc->sc_bst, prcm_bsh,
		    AWIN_A31_PRCM_CIR_CLK_REG);
		clk &= ~AWIN_CLK_SRC_SEL;
		clk |= __SHIFTIN(AWIN_CLK_SRC_SEL_CIR_HOSC, AWIN_CLK_SRC_SEL);
		clk &= ~AWIN_CLK_DIV_RATIO_M;
		clk |= __SHIFTIN(7, AWIN_CLK_DIV_RATIO_M);
		clk &= ~AWIN_CLK_DIV_RATIO_N;
		clk |= __SHIFTIN(0, AWIN_CLK_DIV_RATIO_N);
		clk |= AWIN_CLK_ENABLE;
		bus_space_write_4(sc->sc_bst, prcm_bsh,
		    AWIN_A31_PRCM_CIR_CLK_REG, clk);

		bus_space_unmap(sc->sc_bst, prcm_bsh, prcm_size);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		const struct awin_gpio_pinset pinset =
		    { 'L', AWIN_A80_PIO_PL_CIR_FUNC, AWIN_A80_PIO_PL_CIR_PINS,
		           GPIO_PIN_PULLUP };
		bus_space_handle_t prcm_bsh;
		bus_size_t prcm_size = 0x200;

		bus_space_subregion(sc->sc_bst, aio->aio_a80_rcpus_bsh,
		    AWIN_A80_RPRCM_OFFSET, prcm_size, &prcm_bsh);

		awin_gpio_pinset_acquire(&pinset);

		awin_reg_set_clear(sc->sc_bst, prcm_bsh,
		    AWIN_A80_RPRCM_APB0_GATING_REG,
		    AWIN_A80_RPRCM_APB0_GATING_CIR, 0);
		awin_reg_set_clear(sc->sc_bst, prcm_bsh,
		    AWIN_A80_RPRCM_APB0_RST_REG,
		    AWIN_A80_RPRCM_APB0_RST_CIR, 0);
		awin_reg_set_clear(sc->sc_bst, prcm_bsh,
		    AWIN_A80_RPRCM_CIR_CLK_REG,
		    __SHIFTIN(AWIN_CLK_SRC_SEL_CIR_HOSC, AWIN_CLK_SRC_SEL) |
		    __SHIFTIN(7, AWIN_CLK_DIV_RATIO_M) |
		    __SHIFTIN(0, AWIN_CLK_DIV_RATIO_N) |
		    AWIN_CLK_ENABLE,
		    AWIN_CLK_SRC_SEL |
		    AWIN_CLK_DIV_RATIO_M | AWIN_CLK_DIV_RATIO_N);

		bus_space_unmap(sc->sc_bst, prcm_bsh, prcm_size);
	} else {
		const struct awin_gpio_pinset pinset =
		    { 'B', AWIN_PIO_PB_IR0_FUNC, AWIN_PIO_PB_IR0_PINS };
		uint32_t clk;

		awin_gpio_pinset_acquire(&pinset);

		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_APB0_GATING_REG,
		    AWIN_APB_GATING0_IR0 << sc->sc_port,
		    0);

		clk = bus_space_read_4(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_IR0_CLK_REG + (sc->sc_port * 4));
		clk &= ~AWIN_CLK_SRC_SEL;
		clk |= __SHIFTIN(AWIN_CLK_SRC_SEL_OSC24M, AWIN_CLK_SRC_SEL);
		clk &= ~AWIN_CLK_DIV_RATIO_M;
		clk |= __SHIFTIN(7, AWIN_CLK_DIV_RATIO_M);
		clk &= ~AWIN_CLK_DIV_RATIO_N;
		clk |= __SHIFTIN(0, AWIN_CLK_DIV_RATIO_N);
		clk |= AWIN_CLK_ENABLE;
		bus_space_write_4(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_IR0_CLK_REG + (sc->sc_port * 4), clk);
	}
}

static int
awin_ir_intr(void *priv)
{
	struct awin_ir_softc *sc = priv;
	uint32_t sta;

	sta = IR_READ(sc, AWIN_IR_RXSTA_REG);

#ifdef AWIN_IR_DEBUG
	printf("%s: sta = 0x%08x\n", __func__, sta);
#endif

	if ((sta & AWIN_IR_RXSTA_MASK) == 0)
		return 0;

	IR_WRITE(sc, AWIN_IR_RXSTA_REG, sta & AWIN_IR_RXSTA_MASK);

	if (sta & AWIN_IR_RXSTA_RPE) {
		mutex_enter(&sc->sc_lock);
		sc->sc_avail = __SHIFTOUT(sta, AWIN_IR_RXSTA_RAC);
		cv_broadcast(&sc->sc_cv);
		mutex_exit(&sc->sc_lock);
	}

	return 1;
}

static int
awin_ir_open(void *priv, int flag, int mode, struct proc *p)
{
	struct awin_ir_softc *sc = priv;
	uint32_t ctl, rxint, cir;

	ctl = __SHIFTIN(AWIN_IR_CTL_MD_CIR, AWIN_IR_CTL_MD);
	IR_WRITE(sc, AWIN_IR_CTL_REG, ctl);

	cir = __SHIFTIN(3, AWIN_IR_CIR_SCS);
	cir |= __SHIFTIN(0, AWIN_IR_CIR_SCS2);
	cir |= __SHIFTIN(8, AWIN_IR_CIR_NTHR);
	cir |= __SHIFTIN(2, AWIN_IR_CIR_ITHR);
	if (awin_chip_id() == AWIN_CHIP_ID_A31 ||
	    awin_chip_id() == AWIN_CHIP_ID_A80) {
		cir |= __SHIFTIN(99, AWIN_IR_CIR_ATHR);
		cir |= __SHIFTIN(0, AWIN_IR_CIR_ATHC);
	}
	IR_WRITE(sc, AWIN_IR_CIR_REG, cir);

	IR_WRITE(sc, AWIN_IR_RXCTL_REG, AWIN_IR_RXCTL_RPPI);

	IR_WRITE(sc, AWIN_IR_RXSTA_REG, AWIN_IR_RXSTA_MASK);

	rxint = AWIN_IR_RXINT_RPEI_EN;
	rxint |= AWIN_IR_RXINT_ROI_EN;
	rxint |= AWIN_IR_RXINT_RAI_EN;
	rxint |= __SHIFTIN(31, AWIN_IR_RXINT_RAL);
	IR_WRITE(sc, AWIN_IR_RXINT_REG, rxint);

	ctl |= AWIN_IR_CTL_GEN;
	ctl |= AWIN_IR_CTL_RXEN;
	IR_WRITE(sc, AWIN_IR_CTL_REG, ctl);

	return 0;
}

static int
awin_ir_close(void *priv, int flag, int mode, struct proc *p)
{
	struct awin_ir_softc *sc = priv;

	IR_WRITE(sc, AWIN_IR_RXINT_REG, 0);
	IR_WRITE(sc, AWIN_IR_CTL_REG, 0);
	sc->sc_avail = 0;

	return 0;
}

static int
awin_ir_read(void *priv, struct uio *uio, int flag)
{
	struct awin_ir_softc *sc = priv;
	uint8_t data;
	int error = 0;

	mutex_enter(&sc->sc_lock);
	while (uio->uio_resid > 0) {
		if (sc->sc_avail == 0) {
			error = cv_wait_sig(&sc->sc_cv, &sc->sc_lock);
			if (error) {
				break;
			}
		}
		if (sc->sc_avail > 0) {
			--sc->sc_avail;
			data = IR_READ(sc, AWIN_IR_RXFIFO_REG) &
			    AWIN_IR_RXFIFO_DATA;
			error = uiomove(&data, sizeof(data), uio);
			if (error) {
				break;
			}
		}
	}
	mutex_exit(&sc->sc_lock);

	return error;
}

static int
awin_ir_write(void *priv, struct uio *uio, int flag)
{
	return EIO;
}

static int
awin_ir_setparams(void *priv, struct cir_params *params)
{
	return 0;
}

#ifdef DDB
void
awin_ir_dump_regs(void)
{
	struct awin_ir_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("awinir", 0);
	if (dev == NULL)
		return;
	sc = device_private(dev);

	printf("CTL:    0x%08x\n", IR_READ(sc, AWIN_IR_CTL_REG));
	printf("RXCTL:  0x%08x\n", IR_READ(sc, AWIN_IR_RXCTL_REG));
	printf("RXADR:  0x%08x\n", IR_READ(sc, AWIN_IR_RXADR_REG));
	printf("RXFIFO: ...\n");
	printf("RXINT:  0x%08x\n", IR_READ(sc, AWIN_IR_RXINT_REG));
	printf("RXSTA:  0x%08x\n", IR_READ(sc, AWIN_IR_RXSTA_REG));
	printf("CIR:    0x%08x\n", IR_READ(sc, AWIN_IR_CIR_REG));
}
#endif
