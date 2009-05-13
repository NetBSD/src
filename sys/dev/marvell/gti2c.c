/*	$NetBSD: gti2c.c,v 1.10.18.1 2009/05/13 17:20:04 jym Exp $	*/

/*
 * Copyright (c) 2005 Brocade Communcations, inc.
 * All rights reserved.
 *
 * Written by Matt Thomas for Brocade Communcations, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Brocade Communications, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BROCADE COMMUNICATIONS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL EITHER BROCADE COMMUNICATIONS, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gti2c.c,v 1.10.18.1 2009/05/13 17:20:04 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/marvell/gtintrreg.h>
#include <dev/marvell/gti2creg.h>

#include <dev/marvell/gtvar.h>

#include <dev/i2c/i2cvar.h>

struct gti2c_softc {
	struct device sc_dev;
	struct evcnt sc_ev_intr;
	struct i2c_controller sc_i2c;
	struct gt_softc *sc_gt;
	kmutex_t sc_lock;
};

static int gt_i2c_match(device_t, cfdata_t, void *);
static void gt_i2c_attach(device_t, device_t, void *);

CFATTACH_DECL(gtiic, sizeof(struct gti2c_softc),
    gt_i2c_match, gt_i2c_attach, NULL, NULL);

extern struct cfdriver gtiic_cd;

static int
gt_i2c_wait(struct gti2c_softc *sc, uint32_t control,
    uint32_t desired_status, int flags)
{
	uint32_t status;
	int error = 0;

    again:
	if (flags & I2C_F_POLL)
		control |= I2C_Control_IntEn;
	if (desired_status != I2C_Status_MasterReadAck)
		gt_write(sc->sc_gt, I2C_REG_Control, control);

	for (;;) {
		control = gt_read(sc->sc_gt, I2C_REG_Control);
		if (control & I2C_Control_IFlg)
			break;
		error = tsleep(sc, PZERO, "gti2cwait",
		    (flags & I2C_F_POLL) ? 1 : 0);
		if (error && (error != ETIMEDOUT || !(flags & I2C_F_POLL)))
			return error;
	}

	status = gt_read(sc->sc_gt, I2C_REG_Status);
	if (status != desired_status)
		return EIO;

	if ((flags & I2C_F_LAST) &&
	    desired_status != I2C_Status_MasterReadAck) {
		control = I2C_Control_Stop;
		goto again;
	}

	return error;
}

static int
gt_i2c_acquire_bus(void *cookie, int flags)
{
	struct gti2c_softc * const sc = cookie;
	uint32_t status;

	if (flags & I2C_F_POLL)
		return 0;

	mutex_enter(&sc->sc_lock);
	status = gt_read(sc->sc_gt, I2C_REG_Status);
	if (status != I2C_Status_Idle) {
		gt_write(sc->sc_gt, I2C_REG_SoftReset, 1);
	}

	return 0;
}

static void
gt_i2c_release_bus(void *cookie, int flags)
{
	struct gti2c_softc * const sc = cookie;

	mutex_exit(&sc->sc_lock);
}

static int
gt_i2c_send_start(void *cookie, int flags)
{
	struct gti2c_softc * const sc = cookie;

	return gt_i2c_wait(sc, I2C_Control_Start, I2C_Status_Started, flags);
}

static int
gt_i2c_send_stop(void *cookie, int flags)
{
	struct gti2c_softc * const sc = cookie;

	return gt_i2c_wait(sc, I2C_Control_Stop, I2C_Status_Idle, flags);
}

static int
gt_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	struct gti2c_softc * const sc = cookie;
	uint32_t data, wanted_status;
	uint8_t read_mask = (flags & I2C_F_READ) != 0;
	int error;

	if (read_mask) {
		wanted_status = I2C_Status_AddrReadAck;
	} else {
		wanted_status = I2C_Status_AddrWriteAck;
	}
	/*
	 * First byte contains whether this xfer is a read or write.
	 */
	data = read_mask;
	if (addr > 0x7f) {
		/*
		 * If this is a 10bit request, the first address byte is
		 * 0b11110<b9><b8><r/w>.
		 */
		data |= 0xf0 | ((addr & 0x30) >> 7);
		gt_write(sc->sc_gt, I2C_REG_Data, data);
		error = gt_i2c_wait(sc, 0, wanted_status, flags);
		if (error)
			return error;
		/*
		 * The first address byte has been sent, now to send
		 * the second one.
		 */
		if (read_mask) {
			wanted_status = I2C_Status_2ndAddrReadAck;
		} else {
			wanted_status = I2C_Status_2ndAddrWriteAck;
		}
		data = (uint8_t) addr;
	} else {
		data |= (addr << 1);
	}

	gt_write(sc->sc_gt, I2C_REG_Data, data);
	return gt_i2c_wait(sc, 0, wanted_status, flags);
}

static int
gt_i2c_read_byte(void *cookie, uint8_t *dp, int flags)
{
	struct gti2c_softc * const sc = cookie;
	int error;

	gt_write(sc->sc_gt, I2C_REG_Data, *dp);
	error = gt_i2c_wait(sc, 0, I2C_Status_MasterReadAck, flags);
	if (error == 0) {
		*dp = gt_read(sc->sc_gt, I2C_REG_Data);
	}
	if (flags & I2C_F_LAST)
		gt_write(sc->sc_gt, I2C_REG_Control, 0);
	return error;
}

static int
gt_i2c_write_byte(void *cookie, uint8_t v, int flags)
{
	struct gti2c_softc * const sc = cookie;

	gt_write(sc->sc_gt, I2C_REG_Data, v);
	return gt_i2c_wait(sc, 0, I2C_Status_MasterWriteAck, flags);
}

static int
gt_i2c_intr(void *aux)
{
	struct gti2c_softc * const sc = aux;
	uint32_t v;

	v = gt_read(sc->sc_gt, I2C_REG_Control);
	if ((v & I2C_Control_IFlg) == 0)
		return 0;
	gt_write(sc->sc_gt, I2C_REG_Control, v & ~I2C_Control_IntEn);

	sc->sc_ev_intr.ev_count++;

	wakeup(sc);
	return 1;
}

int
gt_i2c_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct gt_softc * const gt = device_private(parent);
	struct gt_attach_args * const ga = aux;

	return GT_I2COK(gt, ga, &gtiic_cd);
}

void
gt_i2c_attach(device_t parent, device_t self, void *aux)
{
	struct gt_softc * const gt = device_private(parent);
	struct gti2c_softc * const sc = device_private(self);
	struct gt_attach_args * const ga = aux;
	struct i2cbus_attach_args iba;

	sc->sc_gt = gt;

	GT_I2CFOUND(gt, ga);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = gt_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = gt_i2c_release_bus;
	sc->sc_i2c.ic_release_bus = gt_i2c_release_bus;
	sc->sc_i2c.ic_send_start = gt_i2c_send_start;
	sc->sc_i2c.ic_send_stop = gt_i2c_send_stop;
	sc->sc_i2c.ic_initiate_xfer = gt_i2c_initiate_xfer;
	sc->sc_i2c.ic_read_byte = gt_i2c_read_byte;
	sc->sc_i2c.ic_write_byte = gt_i2c_write_byte;

	intr_establish(IRQ_I2C, IST_LEVEL, IPL_VM, gt_i2c_intr, sc);

	evcnt_attach_dynamic(&sc->sc_ev_intr, EVCNT_TYPE_INTR, NULL,
		device_xname(&sc->sc_dev), "intr");

	iba.iba_tag = &sc->sc_i2c;
	config_found_ia(self, "i2cbus", &iba, iicbus_print);
}
