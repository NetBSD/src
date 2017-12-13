/*-
 * Copyright (c) 2017 MediaTek Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timevar.h>

#include <dev/fdt/fdtvar.h>
#include <arm/mediatek/mercury_reg.h>
#include <arm/mediatek/mtk_pwm.h>

/*======================================================*/
static const unsigned int mtk_pwm_reg_offset[] = {
	0x0010, 0x0050, 0x0090, 0x00d0, 0x0110, 0x0150, 0x0190, 0x0220
};

static const struct mtk_pwm_platform_data mtk_pwm_data = {
	.num_pwms = 3,
};
/*======================================================*/

static const char * const compatible[] = {
	"mediatek,mercury-pwm",
	NULL
};

uint32_t mtk_pwm_readl(struct mtk_pwm_softc *sc, unsigned int offset);
void mtk_pwm_writel(struct mtk_pwm_softc *sc, unsigned int offset, uint32_t value);
int mtk_pwm_clk_enable(void);
void mtk_pwm_clk_disable(void);

#define read32(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define write32(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

uint32_t mtk_pwm_readl(struct mtk_pwm_softc *sc, unsigned int offset)
{
	return read32(sc, mtk_pwm_reg_offset[sc->sc_pwm_idx] + offset);
}

void mtk_pwm_writel(struct mtk_pwm_softc *sc, unsigned int offset, uint32_t value)
{
	write32(sc, mtk_pwm_reg_offset[sc->sc_pwm_idx] + offset,value);
}

int mtk_pwm_clk_enable(void)
{
        // TODO: add CLK enable
	return 0;
}

void mtk_pwm_clk_disable(void)
{
        // TODO: add CLK disable
}

int mtk_pwm_config(struct mtk_pwm_softc *sc, int duty_ns, int period_ns)
{
	uint32_t resolution, clkdiv = 0;
	int ret = 0;

	ret = mtk_pwm_clk_enable();
	if (ret < 0)
		return ret;

	resolution = NSEC_PER_SEC / PWM_DEFAULT_SRC_CLK;

	while (period_ns / resolution > 8191) {
		resolution *= 2;
		clkdiv++;
	}

	if (clkdiv > PWM_CLK_DIV_MAX) {
		mtk_pwm_clk_disable();
                aprint_normal("\n period %d not supported\n", period_ns);
		return ret;
	}

	mtk_pwm_writel(sc, PWMCON, BIT(15) | clkdiv);
	mtk_pwm_writel(sc, PWMDWIDTH, period_ns / resolution);
	mtk_pwm_writel(sc, PWMTHRES, duty_ns / resolution);

	mtk_pwm_clk_disable();

	return ret;
}

int mtk_pwm_enable(struct mtk_pwm_softc *sc)
{
	uint32_t value;
	int ret = 0;

	ret = mtk_pwm_clk_enable();
	if (ret < 0)
		return ret;

	value = read32(sc,PWM_EN);
	value |= BIT(sc->sc_pwm_idx);
	write32(sc, PWM_EN, value);

	return ret;
}

void mtk_pwm_disable(struct mtk_pwm_softc *sc)
{
	uint32_t value;

	value = read32(sc,PWM_EN);
	value &= ~BIT(sc->sc_pwm_idx);
	write32(sc, PWM_EN, value);

	mtk_pwm_clk_disable();
}

/*==========================================================*/
enum {
	PWM_CON,
	PWM_HDURATION,
	PWM_LDURATION,
	PWM_GDURATION,
	PWM_BUF0_BASE_ADDR,
	PWM_BUF0_SIZE,
	PWM_BUF1_BASE_ADDR,
	PWM_BUF1_SIZE,
	PWM_SEND_DATA0,
	PWM_SEND_DATA1,
	PWM_WAVE_NUM,
	PWM_DATA_WIDTH,
	PWM_THRESH,
	PWM_SEND_WAVENUM,
	PWM_VALID
} PWM_REG_OFF;


//void mtk_pwm_dump_regs(void)
void mtk_pwm_dump_regs(struct mtk_pwm_softc *sc)
{
	//struct mtk_pwm_softc *sc;
	//device_t dev;
	int i;
	uint32_t reg_val;

	aprint_normal("\n----mtk_pwm_dump_regs----\n ");
#if 0
	dev = device_find_by_driver_unit("mtk_pwm", 0);
	if (dev == NULL)
		return;
	sc = device_private(dev);
#endif
	reg_val = read32(sc,PWM_EN);
	aprint_normal("\r\n[PWM_ENABLE is:0x%x]\n\r ", reg_val);
	reg_val = read32(sc,PWM_CK_26M_SEL);
	aprint_normal("\r\n[PWM_26M_SEL is:0x%x]\n\r ", reg_val);

	for (i = 0; i < mtk_pwm_data.num_pwms; i++) {
		reg_val = read32(sc, mtk_pwm_reg_offset[i] + 4 * PWM_CON);
		aprint_normal("\r\n[PWM%d_CON is:0x%x]\r\n", i , reg_val);

 		reg_val = read32(sc, mtk_pwm_reg_offset[i] + 4 * PWM_WAVE_NUM);
		aprint_normal("[PWM%d_WAVE_NUM is:0x%x]\r\n", i , reg_val);
		reg_val = read32(sc, mtk_pwm_reg_offset[i] + 4 * PWM_DATA_WIDTH);
		aprint_normal("[PWM%d_WIDTH is:0x%x]\r\n", i , reg_val);
		reg_val = read32(sc, mtk_pwm_reg_offset[i] + 4 * PWM_THRESH);
		aprint_normal("[PWM%d_THRESH is:0x%x]\r\n", i , reg_val);
		reg_val = read32(sc, mtk_pwm_reg_offset[i] + 4 * PWM_SEND_WAVENUM);
		aprint_normal("[PWM%d_SEND_WAVENUM is:0x%x]\r\n", i , reg_val);

	}
}

/*==========================================================*/

CFATTACH_DECL_NEW(mtk_pwm, sizeof(struct mtk_pwm_softc),
	mtk_pwm_match, mtk_pwm_attach, NULL, NULL);

static int mtk_pwm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void mtk_pwm_attach(device_t parent, device_t self, void *aux)
{
	struct mtk_pwm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	aprint_normal("\npwm attached done\n ");
}





