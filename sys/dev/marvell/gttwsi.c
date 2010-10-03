/*	$NetBSD: gttwsi.c,v 1.4 2010/10/03 07:14:33 kiyohara Exp $	*/
/*
 * Copyright (c) 2008 Eiji Kawauchi.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Eiji Kawauchi.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
//#define TWSI_DEBUG

/*
 * Marvell Two-Wire Serial Interface (aka I2C) master driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gttwsi.c,v 1.4 2010/10/03 07:14:33 kiyohara Exp $");
#include "locators.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/param.h>

#include <dev/i2c/i2cvar.h>

#include <dev/marvell/marvellvar.h>
#include <dev/marvell/gttwsireg.h>

struct gttwsi_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	uint8_t sc_started;
	struct i2c_controller sc_i2c;
	kmutex_t sc_buslock;
	kmutex_t sc_mtx;
	kcondvar_t sc_cv;
};

static int	gttwsi_match(device_t, cfdata_t, void *);
static void	gttwsi_attach(device_t, device_t, void *);

static int	gttwsi_intr(void *);

static int	gttwsi_acquire_bus(void *, int);
static void	gttwsi_release_bus(void *, int);
static int	gttwsi_send_start(void *v, int flags);
static int	gttwsi_send_stop(void *v, int flags);
static int	gttwsi_initiate_xfer(void *v, i2c_addr_t addr, int flags);
static int	gttwsi_read_byte(void *v, uint8_t *valp, int flags);
static int	gttwsi_write_byte(void *v, uint8_t val, int flags);

static int	gttwsi_wait(struct gttwsi_softc *, uint32_t, uint32_t, int);

static __inline u_int32_t RREG(struct gttwsi_softc *, u_int32_t);
static __inline void WREG(struct gttwsi_softc *, u_int32_t, u_int32_t);


CFATTACH_DECL_NEW(gttwsi_gt, sizeof(struct gttwsi_softc),
    gttwsi_match, gttwsi_attach, NULL, NULL);
CFATTACH_DECL_NEW(gttwsi_mbus, sizeof(struct gttwsi_softc),
    gttwsi_match, gttwsi_attach, NULL, NULL);


static __inline u_int32_t
RREG(struct gttwsi_softc *sc, u_int32_t reg)
{
	u_int32_t val;

	val = bus_space_read_4(sc->sc_bust, sc->sc_bush, reg);
#ifdef TWSI_DEBUG
	printf("I2C:R:%02x:%02x\n", reg, val);
#else
	DELAY(TWSI_READ_DELAY);
#endif
	return val;
}

static __inline void
WREG(struct gttwsi_softc *sc, u_int32_t reg, u_int32_t val)
{
	bus_space_write_4(sc->sc_bust, sc->sc_bush, reg, val);
#ifdef TWSI_DEBUG
	printf("I2C:W:%02x:%02x\n", reg, val);
#else
	DELAY(TWSI_WRITE_DELAY);
#endif
	return;
}


/* ARGSUSED */
static int
gttwsi_match(device_t parent, cfdata_t match, void *aux)
{
	struct marvell_attach_args *mva = aux;

	if (strcmp(mva->mva_name, match->cf_name) != 0)
		return 0;
	if (mva->mva_offset == MVA_OFFSET_DEFAULT ||
	    mva->mva_irq == MVA_IRQ_DEFAULT)
		return 0;

	mva->mva_size = GTTWSI_SIZE;
	return 1;
}

/* ARGSUSED */
static void
gttwsi_attach(device_t parent, device_t self, void *args)
{
	struct gttwsi_softc *sc = device_private(self);
	struct marvell_attach_args *mva = args;
	struct i2cbus_attach_args iba;

	aprint_naive("\n");
	aprint_normal(": Marvell TWSI controller\n");

	sc->sc_dev = self;
	sc->sc_bust = mva->mva_iot;
	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh, mva->mva_offset,
	    mva->mva_size, &sc->sc_bush)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}

	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_cv, "gttwsi");

	sc->sc_started = 0;
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = gttwsi_acquire_bus;
	sc->sc_i2c.ic_release_bus = gttwsi_release_bus;
	sc->sc_i2c.ic_exec = NULL;
	sc->sc_i2c.ic_send_start = gttwsi_send_start;
	sc->sc_i2c.ic_send_stop = gttwsi_send_stop;
	sc->sc_i2c.ic_initiate_xfer = gttwsi_initiate_xfer;
	sc->sc_i2c.ic_read_byte = gttwsi_read_byte;
	sc->sc_i2c.ic_write_byte = gttwsi_write_byte;

	marvell_intr_establish(mva->mva_irq, IPL_BIO, gttwsi_intr, sc);

	/*
	 * Put the controller into Soft Reset.
	 */
	/* reset */
	WREG(sc, TWSI_SOFTRESET, SOFTRESET_VAL);

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	(void) config_found_ia(sc->sc_dev, "i2cbus", &iba, iicbus_print);
}

static int
gttwsi_intr(void *arg)
{
	struct gttwsi_softc *sc = arg;
	uint32_t val;

	val = RREG(sc, TWSI_CONTROL);
	if (val & CONTROL_IFLG) {
		WREG(sc, TWSI_CONTROL, val & ~CONTROL_INTEN);
		mutex_enter(&sc->sc_mtx);
		cv_signal(&sc->sc_cv);
		mutex_exit(&sc->sc_mtx);

		return 1;	/* handled */
	}
	return 0;
}

/* ARGSUSED */
static int
gttwsi_acquire_bus(void *arg, int flags)
{
	struct gttwsi_softc *sc = arg;

	mutex_enter(&sc->sc_buslock);
	return 0;
}

/* ARGSUSED */
static void
gttwsi_release_bus(void *arg, int flags)
{
	struct gttwsi_softc *sc = arg;

	mutex_exit(&sc->sc_buslock);
}

static int
gttwsi_send_start(void *v, int flags)
{
	struct gttwsi_softc *sc = v;
	int expect;

	if (sc->sc_started)
		expect = STAT_RSCT;
	else
		expect = STAT_SCT;
	sc->sc_started = 1;
	return gttwsi_wait(sc, CONTROL_START, expect, flags);
}

static int
gttwsi_send_stop(void *v, int flags)
{
	struct gttwsi_softc *sc = v;
	int retry = TWSI_RETRY_COUNT;

	sc->sc_started = 0;

	/* Interrupt is not generated for STAT_NRS. */
	WREG(sc, TWSI_CONTROL, CONTROL_STOP | CONTROL_TWSIEN);
	while (retry > 0) {
		if (RREG(sc, TWSI_STATUS) == STAT_NRS)
			return 0;
		retry--;
		DELAY(TWSI_STAT_DELAY);
	}

	aprint_error_dev(sc->sc_dev, "send STOP failed\n");
	return -1;
}

static int
gttwsi_initiate_xfer(void *v, i2c_addr_t addr, int flags)
{
	struct gttwsi_softc *sc = v;
	uint32_t data, expect;
	int error, read;

	gttwsi_send_start(v, flags);

	read = (flags & I2C_F_READ) != 0;
	if (read)
		expect = STAT_ARBT_AR;
	else
		expect = STAT_AWBT_AR;

	/*
	 * First byte contains whether this xfer is a read or write.
	 */
	data = read;
	if (addr > 0x7f) {
		/*
		 * If this is a 10bit request, the first address byte is
		 * 0b11110<b9><b8><r/w>.
		 */
		data |= 0xf0 | ((addr & 0x300) >> 7);
		WREG(sc, TWSI_DATA, data);
		error = gttwsi_wait(sc, 0, expect, flags);
		if (error)
			return error;
		/*
		 * The first address byte has been sent, now to send
		 * the second one.
		 */
		if (read)
			expect = STAT_SARBT_AR;
		else
			expect = STAT_SAWBT_AR;
		data = (uint8_t)addr;
	} else
		data |= (addr << 1);

	WREG(sc, TWSI_DATA, data);
	return gttwsi_wait(sc, 0, expect, flags);
}

static int
gttwsi_read_byte(void *v, uint8_t *valp, int flags)
{
	struct gttwsi_softc *sc = v;
	int error;

	if (flags & I2C_F_STOP)
		error = gttwsi_wait(sc, 0, STAT_MRRD_ANT, flags);
	else
		error = gttwsi_wait(sc, CONTROL_ACK, STAT_MRRD_AT, flags);
	if (!error)
		*valp = RREG(sc, TWSI_DATA);
	if (flags & I2C_F_LAST)
		WREG(sc, TWSI_CONTROL, 0);
	return error;
}

static int
gttwsi_write_byte(void *v, uint8_t val, int flags)
{
	struct gttwsi_softc *sc = v;

	WREG(sc, TWSI_DATA, val);
	return gttwsi_wait(sc, 0, STAT_MTDB_AR, flags);
}

static int
gttwsi_wait(struct gttwsi_softc *sc, uint32_t control, uint32_t expect,
	    int flags)
{
	uint32_t status;
	int error = 0;

	DELAY(5);
	if (!(flags & I2C_F_POLL))
		control |= CONTROL_INTEN;
	WREG(sc, TWSI_CONTROL, control | CONTROL_TWSIEN);

	for (;;) {
		control = RREG(sc, TWSI_CONTROL);
		if (control & CONTROL_IFLG)
			break;
		if (!(flags & I2C_F_POLL)) {
			mutex_enter(&sc->sc_mtx);
			error = cv_timedwait_sig(&sc->sc_cv, &sc->sc_mtx, hz);
			mutex_exit(&sc->sc_mtx);
			if (error)
				return error;
		}
		DELAY(TWSI_RETRY_DELAY);
	}

	status = RREG(sc, TWSI_STATUS);
	if (status != expect) {
		aprint_error_dev(sc->sc_dev,
		    "unexpected status 0x%x: expect 0x%x\n", status, expect);
		return EIO;
	}

	if ((flags & I2C_F_STOP) && expect != STAT_MRRD_AT)
		error = gttwsi_send_stop(sc, flags);

	return error;
}
