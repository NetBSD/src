/* $NetBSD: ausmbus_psc.c,v 1.11.6.1 2017/12/03 11:36:26 jdolecek Exp $ */

/*-
 * Copyright (c) 2006 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ausmbus_psc.c,v 1.11.6.1 2017/12/03 11:36:26 jdolecek Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <sys/bus.h>
#include <machine/cpu.h>

#include <mips/alchemy/dev/aupscreg.h>
#include <mips/alchemy/dev/aupscvar.h>
#include <mips/alchemy/dev/ausmbus_pscreg.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

struct ausmbus_softc {
	device_t			sc_dev;

	/* protocol comoon fields */
	struct aupsc_controller		sc_ctrl;

	/* protocol specific fields */
	struct i2c_controller		sc_i2c;
	i2c_addr_t			sc_smbus_slave_addr;
	int				sc_smbus_timeout;
};

#define	ausmbus_reg_read(sc, reg) \
	bus_space_read_4(sc->sc_ctrl.psc_bust, sc->sc_ctrl.psc_bush, reg)
#define	ausmbus_reg_write(sc, reg, val) \
	bus_space_write_4(sc->sc_ctrl.psc_bust, sc->sc_ctrl.psc_bush, reg, \
		val); \
	delay(100);

static int	ausmbus_match(device_t, struct cfdata *, void *);
static void	ausmbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ausmbus, sizeof(struct ausmbus_softc),
	ausmbus_match, ausmbus_attach, NULL, NULL);

/* fuctions for i2c_controller */
static int	ausmbus_acquire_bus(void *, int);
static void	ausmbus_release_bus(void *, int);
static int	ausmbus_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
				const void *cmd, size_t cmdlen, void *vbuf,
				size_t buflen, int flags);

/* subroutine functions for i2c_controller */
static int	ausmbus_quick_write(struct ausmbus_softc *);
static int	ausmbus_quick_read(struct ausmbus_softc *);
static int	ausmbus_receive_1(struct ausmbus_softc *, uint8_t *);
static int	ausmbus_read_1(struct ausmbus_softc *, uint8_t, uint8_t *);
static int	ausmbus_read_2(struct ausmbus_softc *, uint8_t, uint16_t *);
static int	ausmbus_send_1(struct ausmbus_softc *, uint8_t);
static int	ausmbus_write_1(struct ausmbus_softc *, uint8_t, uint8_t);
static int	ausmbus_write_2(struct ausmbus_softc *, uint8_t, uint16_t);
static int	ausmbus_wait_mastertx(struct ausmbus_softc *sc);
static int	ausmbus_wait_masterrx(struct ausmbus_softc *sc);
static int	ausmbus_initiate_xfer(void *, i2c_addr_t, int);
static int	ausmbus_read_byte(void *arg, uint8_t *vp, int flags);
static int	ausmbus_write_byte(void *arg, uint8_t v, int flags);


static int
ausmbus_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct aupsc_attach_args *aa = (struct aupsc_attach_args *)aux;

	if (strcmp(aa->aupsc_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

static void
ausmbus_attach(device_t parent, device_t self, void *aux)
{
	struct ausmbus_softc *sc = device_private(self);
	struct aupsc_attach_args *aa = (struct aupsc_attach_args *)aux;
	struct i2cbus_attach_args iba;

	aprint_normal(": Alchemy PSC SMBus protocol\n");

	sc->sc_dev = self;

	/* Initialize PSC */
	sc->sc_ctrl = aa->aupsc_ctrl;

	/* Initialize i2c_controller for SMBus */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = ausmbus_acquire_bus;
	sc->sc_i2c.ic_release_bus = ausmbus_release_bus;
	sc->sc_i2c.ic_send_start = NULL;
	sc->sc_i2c.ic_send_stop = NULL;
	sc->sc_i2c.ic_initiate_xfer = NULL;
	sc->sc_i2c.ic_read_byte = NULL;
	sc->sc_i2c.ic_write_byte = NULL;
	sc->sc_i2c.ic_exec = ausmbus_exec;
	sc->sc_smbus_timeout = 10;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	(void) config_found_ia(self, "i2cbus", &iba, iicbus_print);
}

static int
ausmbus_acquire_bus(void *arg, int flags)
{
	struct ausmbus_softc *sc = arg;
	uint32_t v;

	/* Select SMBus Protocol & Enable PSC */
	sc->sc_ctrl.psc_enable(sc, AUPSC_SEL_SMBUS);
	v = ausmbus_reg_read(sc, AUPSC_SMBSTAT);
	if ((v & SMBUS_STAT_SR) == 0) {
		/* PSC is not ready */
		return -1;
	}

	/* Setup SMBus Configuration register */
	v = SMBUS_CFG_DD;				/* Disable DMA */
	v |= SMBUS_CFG_RT_SET(SMBUS_CFG_RT_FIFO8);	/* Rx FIFO 8data */
	v |= SMBUS_CFG_TT_SET(SMBUS_CFG_TT_FIFO8);	/* Tx FIFO 8data */
	v |= SMBUS_CFG_DIV_SET(SMBUS_CFG_DIV8);		/* pscn_mainclk/8 */
	v &= ~SMBUS_CFG_SFM;				/* Standard Mode */
	ausmbus_reg_write(sc, AUPSC_SMBCFG, v);

	/* Setup SMBus Protocol Timing register */
	v = SMBUS_TMR_TH_SET(SMBUS_TMR_STD_TH)
		| SMBUS_TMR_PS_SET(SMBUS_TMR_STD_PS)
		| SMBUS_TMR_PU_SET(SMBUS_TMR_STD_PU)
		| SMBUS_TMR_SH_SET(SMBUS_TMR_STD_SH)
		| SMBUS_TMR_SU_SET(SMBUS_TMR_STD_SU)
		| SMBUS_TMR_CL_SET(SMBUS_TMR_STD_CL)
		| SMBUS_TMR_CH_SET(SMBUS_TMR_STD_CH);
	ausmbus_reg_write(sc, AUPSC_SMBTMR, v);

	/* Setup SMBus Mask register */
	v = SMBUS_MSK_ALLMASK;
	ausmbus_reg_write(sc, AUPSC_SMBMSK, v);

	/* SMBus Enable */
	v = ausmbus_reg_read(sc, AUPSC_SMBCFG);
	v |= SMBUS_CFG_DE;
	ausmbus_reg_write(sc, AUPSC_SMBCFG, v);
	v = ausmbus_reg_read(sc, AUPSC_SMBSTAT);
	if ((v & SMBUS_STAT_SR) == 0) {
		/* SMBus is not ready */
		return -1;
	}

#ifdef AUSMBUS_PSC_DEBUG
	aprint_normal("AuSMBus enabled.\n");
	aprint_normal("AuSMBus smbconfig: 0x%08x\n",
			ausmbus_reg_read(sc, AUPSC_SMBCFG));
	aprint_normal("AuSMBus smbstatus: 0x%08x\n",
			ausmbus_reg_read(sc, AUPSC_SMBSTAT));
	aprint_normal("AuSMBus smbtmr   : 0x%08x\n",
			ausmbus_reg_read(sc, AUPSC_SMBTMR));
	aprint_normal("AuSMBus smbmask  : 0x%08x\n",
			ausmbus_reg_read(sc, AUPSC_SMBMSK));
#endif

	return 0;
}

static void
ausmbus_release_bus(void *arg, int flags)
{
	struct ausmbus_softc *sc = arg;

	ausmbus_reg_write(sc, AUPSC_SMBCFG, 0);
	sc->sc_ctrl.psc_disable(sc);

	return;
}

static int
ausmbus_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *vcmd,
	size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct ausmbus_softc *sc  = (struct ausmbus_softc *)cookie;
	const uint8_t *cmd = vcmd;

	sc->sc_smbus_slave_addr  = addr;

	/* Receive byte */
	if ((I2C_OP_READ_P(op)) && (cmdlen == 0) && (buflen == 1)) {
		return ausmbus_receive_1(sc, (uint8_t *)vbuf);
	}

	/* Read byte */
	if ((I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 1)) {
		return ausmbus_read_1(sc, *cmd, (uint8_t *)vbuf);
	}

	/* Read word */
	if ((I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 2)) {
		return ausmbus_read_2(sc, *cmd, (uint16_t *)vbuf);
	}

	/* Read quick */
	if ((I2C_OP_READ_P(op)) && (cmdlen == 0) && (buflen == 0)) {
		return ausmbus_quick_read(sc);
	}

	/* Send byte */
	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 0) && (buflen == 1)) {
		return ausmbus_send_1(sc, *((uint8_t *)vbuf));
	}

	/* Write byte */
	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 1)) {
		return ausmbus_write_1(sc, *cmd, *((uint8_t *)vbuf));
	}

	/* Write word */
	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 2)) {
		return ausmbus_write_2(sc, *cmd, *((uint16_t *)vbuf));
	}

	/* Write quick */
	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 0) && (buflen == 0)) {
		return ausmbus_quick_write(sc);
	}

	/*
	 * XXX: TODO Please Support other protocols defined in SMBus 2.0
	 * - Process call
	 * - Block write/read
	 * - Clock write-block read process cal
	 * - SMBus host notify protocol
	 *
	 * - Read quick and write quick have not been tested!
	 */

	return -1;
}

static int
ausmbus_receive_1(struct ausmbus_softc *sc, uint8_t *vp)
{
	int error;

	error = ausmbus_initiate_xfer(sc, sc->sc_smbus_slave_addr, I2C_F_READ);
	if (error != 0) {
		return error;
	}
	error = ausmbus_read_byte(sc, vp, I2C_F_STOP);
	if (error != 0) {
		return error;
	}

	return 0;
}

static int
ausmbus_read_1(struct ausmbus_softc *sc, uint8_t cmd, uint8_t *vp)
{
	int error;

	error = ausmbus_initiate_xfer(sc, sc->sc_smbus_slave_addr, I2C_F_WRITE);
	if (error != 0) {
		return error;
	}

	error = ausmbus_write_byte(sc, cmd, I2C_F_READ);
	if (error != 0) {
		return error;
	}

	error = ausmbus_initiate_xfer(sc, sc->sc_smbus_slave_addr, I2C_F_READ);
	if (error != 0) {
		return error;
	}

	error = ausmbus_read_byte(sc, vp, I2C_F_STOP);
	if (error != 0) {
		return error;
	}

	return 0;
}

static int
ausmbus_read_2(struct ausmbus_softc *sc, uint8_t cmd, uint16_t *vp)
{
	int error;
	uint8_t high, low;

	error = ausmbus_initiate_xfer(sc, sc->sc_smbus_slave_addr, I2C_F_WRITE);
	if (error != 0) {
		return error;
	}

	error = ausmbus_write_byte(sc, cmd, I2C_F_READ);
	if (error != 0) {
		return error;
	}

	error = ausmbus_initiate_xfer(sc, sc->sc_smbus_slave_addr, I2C_F_READ);
	if (error != 0) {
		return error;
	}

	error = ausmbus_read_byte(sc, &low, 0);
	if (error != 0) {
		return error;
	}

	error = ausmbus_read_byte(sc, &high, I2C_F_STOP);
	if (error != 0) {
		return error;
	}

	*vp = (high << 8) | low;

	return 0;
}

static int
ausmbus_send_1(struct ausmbus_softc *sc, uint8_t val)
{
	int error;

	error = ausmbus_initiate_xfer(sc, sc->sc_smbus_slave_addr, I2C_F_WRITE);
	if (error != 0) {
		return error;
	}

	error = ausmbus_write_byte(sc, val, I2C_F_STOP);
	if (error != 0) {
		return error;
	}

	return 0;
}

static int
ausmbus_write_1(struct ausmbus_softc *sc, uint8_t cmd, uint8_t val)
{
	int error;

	error = ausmbus_initiate_xfer(sc, sc->sc_smbus_slave_addr, I2C_F_WRITE);
	if (error != 0) {
		return error;
	}

	error = ausmbus_write_byte(sc, cmd, 0);
	if (error != 0) {
		return error;
	}

	error = ausmbus_write_byte(sc, val, I2C_F_STOP);
	if (error != 0) {
		return error;
	}

	return 0;
}

static int
ausmbus_write_2(struct ausmbus_softc *sc, uint8_t cmd, uint16_t val)
{
	int error;
	uint8_t high, low;

	high = (val >> 8) & 0xff;
	low = val & 0xff;

	error = ausmbus_initiate_xfer(sc, sc->sc_smbus_slave_addr, I2C_F_WRITE);
	if (error != 0) {
		return error;
	}

	error = ausmbus_write_byte(sc, cmd, 0);
	if (error != 0) {
		return error;
	}

	error = ausmbus_write_byte(sc, low, 0);
	if (error != 0) {
		return error;
	}

	error = ausmbus_write_byte(sc, high, I2C_F_STOP);
	if (error != 0) {
		return error;
	}

	return 0;
}

/*
 * XXX The quick_write() and quick_read() routines have not been tested!
 */
static int
ausmbus_quick_write(struct ausmbus_softc *sc)
{
	return ausmbus_initiate_xfer(sc, sc->sc_smbus_slave_addr,
			I2C_F_STOP | I2C_F_WRITE);
}

static int
ausmbus_quick_read(struct ausmbus_softc *sc)
{
	return ausmbus_initiate_xfer(sc, sc->sc_smbus_slave_addr,
			I2C_F_STOP | I2C_F_READ);
}

static int
ausmbus_wait_mastertx(struct ausmbus_softc *sc)
{
	uint32_t v;
	int timeout;
	int txerr = 0;

	timeout = sc->sc_smbus_timeout;

	do {
		v = ausmbus_reg_read(sc, AUPSC_SMBEVNT);
#ifdef AUSMBUS_PSC_DEBUG
		aprint_normal("AuSMBus: ausmbus_wait_mastertx(): psc_smbevnt=0x%08x\n", v);
#endif
		if ((v & SMBUS_EVNT_TU) != 0)
			break;
		if ((v & SMBUS_EVNT_MD) != 0)
			break;
		if ((v & (SMBUS_EVNT_DN | SMBUS_EVNT_AN | SMBUS_EVNT_AL))
			!= 0) {
			txerr = 1;
			break;
		}
		timeout--;
		delay(1);
	} while (timeout > 0);

	if (txerr != 0) {
		ausmbus_reg_write(sc, AUPSC_SMBEVNT,
			SMBUS_EVNT_DN | SMBUS_EVNT_AN | SMBUS_EVNT_AL);
#ifdef AUSMBUS_PSC_DEBUG
		aprint_normal("AuSMBus: ausmbus_wait_mastertx(): Tx error\n");
#endif
		return -1;
	}

	/* Reset Event TU (Tx Underflow) */
	ausmbus_reg_write(sc, AUPSC_SMBEVNT, SMBUS_EVNT_TU | SMBUS_EVNT_MD);

#ifdef AUSMBUS_PSC_DEBUG
	v = ausmbus_reg_read(sc, AUPSC_SMBEVNT);
	aprint_normal("AuSMBus: ausmbus_wait_mastertx(): psc_smbevnt=0x%08x (reset)\n", v);
#endif
	return 0;
}

static int
ausmbus_wait_masterrx(struct ausmbus_softc *sc)
{
	uint32_t v;
	int timeout;
	timeout = sc->sc_smbus_timeout;

	if (ausmbus_wait_mastertx(sc) != 0)
		return -1;

	do {
		v = ausmbus_reg_read(sc, AUPSC_SMBSTAT);
#ifdef AUSMBUS_PSC_DEBUG
		aprint_normal("AuSMBus: ausmbus_wait_masterrx(): psc_smbstat=0x%08x\n", v);
#endif
		if ((v & SMBUS_STAT_RE) == 0)
			break;
		timeout--;
		delay(1);
	} while (timeout > 0);

	return 0;
}

static int
ausmbus_initiate_xfer(void *arg, i2c_addr_t addr, int flags)
{
	struct ausmbus_softc *sc = arg;
	uint32_t v;

	/* Tx/Rx Slave Address */
	v = (addr << 1) & SMBUS_TXRX_ADDRDATA;
	if ((flags & I2C_F_READ) != 0)
		v |= 1;
	if ((flags & I2C_F_STOP) != 0)
		v |= SMBUS_TXRX_STP;
	ausmbus_reg_write(sc, AUPSC_SMBTXRX, v);

	/* Master Start */
	ausmbus_reg_write(sc, AUPSC_SMBPCR, SMBUS_PCR_MS);

	if (ausmbus_wait_mastertx(sc) != 0)
		return -1;

	return 0;
}

static int
ausmbus_read_byte(void *arg, uint8_t *vp, int flags)
{
	struct ausmbus_softc *sc = arg;
	uint32_t v;

	if ((flags & I2C_F_STOP) != 0) {
		ausmbus_reg_write(sc, AUPSC_SMBTXRX, SMBUS_TXRX_STP);
	} else {
		ausmbus_reg_write(sc, AUPSC_SMBTXRX, 0);
	}

	if (ausmbus_wait_masterrx(sc) != 0)
		return -1;

	v = ausmbus_reg_read(sc, AUPSC_SMBTXRX);
	*vp = v & SMBUS_TXRX_ADDRDATA;

	return 0;
}

static int
ausmbus_write_byte(void *arg, uint8_t v, int flags)
{
	struct ausmbus_softc *sc = arg;

	if ((flags & I2C_F_STOP) != 0)  {
		ausmbus_reg_write(sc, AUPSC_SMBTXRX, (v | SMBUS_TXRX_STP));
	} else if ((flags & I2C_F_READ) != 0) {
		ausmbus_reg_write(sc, AUPSC_SMBTXRX, (v | SMBUS_TXRX_RSR));
	} else {
		ausmbus_reg_write(sc, AUPSC_SMBTXRX, v);
	}

	if (ausmbus_wait_mastertx(sc) != 0)
		return -1;

	return 0;
}
