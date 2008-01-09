/*	$NetBSD: m41t00.c,v 1.8.20.2 2008/01/09 01:52:41 matt Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford and Jason R. Thorpe for Wasabi Systems, Inc.
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
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: m41t00.c,v 1.8.20.2 2008/01/09 01:52:41 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/event.h>

#include <sys/bus.h>

#include <dev/clock_subr.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/m41t00reg.h>

struct m41t00_softc {
	struct device sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;
	int sc_open;
	struct todr_chip_handle sc_todr;
};

static int  m41t00_match(struct device *, struct cfdata *, void *);
static void m41t00_attach(struct device *, struct device *, void *);

CFATTACH_DECL(m41trtc, sizeof(struct m41t00_softc),
	m41t00_match, m41t00_attach, NULL, NULL);
extern struct cfdriver m41trtc_cd;

dev_type_open(m41t00_open);
dev_type_close(m41t00_close);
dev_type_read(m41t00_read);
dev_type_write(m41t00_write);

const struct cdevsw m41t00_cdevsw = {
	m41t00_open, m41t00_close, m41t00_read, m41t00_write, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER
};

static int m41t00_clock_read(struct m41t00_softc *, struct clock_ymdhms *);
static int m41t00_clock_write(struct m41t00_softc *, struct clock_ymdhms *);
static int m41t00_gettime(struct todr_chip_handle *, volatile struct timeval *);
static int m41t00_settime(struct todr_chip_handle *, volatile struct timeval *);

int
m41t00_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_addr == M41T00_ADDR) {
		return 1;
	}

	return 0;
}

void
m41t00_attach(struct device *parent, struct device *self, void *aux)
{
	struct m41t00_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;

	aprint_naive(": Real-time Clock\n");
	aprint_normal(": M41T00 Real-time Clock\n");

	sc->sc_open = 0;
	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = m41t00_gettime;
	sc->sc_todr.todr_settime = m41t00_settime;
	sc->sc_todr.todr_setwen = NULL;

	todr_attach(&sc->sc_todr);
}

/*ARGSUSED*/
int
m41t00_open(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct m41t00_softc *sc;

	if ((sc = device_lookup(&m41trtc_cd, minor(dev))) == NULL)
		return ENXIO;

	/* XXX: Locking */

	if (sc->sc_open)
		return EBUSY;

	sc->sc_open = 1;
	return 0;
}

/*ARGSUSED*/
int
m41t00_close(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct m41t00_softc *sc;

	if ((sc = device_lookup(&m41trtc_cd, minor(dev))) == NULL)
		return ENXIO;

	sc->sc_open = 0;
	return 0;
}

/*ARGSUSED*/
int
m41t00_read(dev_t dev, struct uio *uio, int flags)
{
	struct m41t00_softc *sc;
	u_int8_t ch, cmdbuf[1];
	int a, error;

	if ((sc = device_lookup(&m41trtc_cd, minor(dev))) == NULL)
		return (ENXIO);

	if (uio->uio_offset >= M41T00_NBYTES)
		return (EINVAL);

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return (error);

	while (uio->uio_resid && uio->uio_offset < M41T00_NBYTES) {
		a = (int)uio->uio_offset;
		cmdbuf[0] = a;
		if ((error = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
				      sc->sc_address, cmdbuf, 1,
				      &ch, 1, 0)) != 0) {
			iic_release_bus(sc->sc_tag, 0);
			printf("%s: m41t00_read: read failed at 0x%x\n",
			    sc->sc_dev.dv_xname, a);
			return (error);
		}
		if ((error = uiomove(&ch, 1, uio)) != 0) {
			iic_release_bus(sc->sc_tag, 0);
			return (error);
		}
	}

	iic_release_bus(sc->sc_tag, 0);

	return (0);
}

/*ARGSUSED*/
int
m41t00_write(dev_t dev, struct uio *uio, int flags)
{
	struct m41t00_softc *sc;
	u_int8_t cmdbuf[2];
	int a, error;

	if ((sc = device_lookup(&m41trtc_cd, minor(dev))) == NULL)
		return (ENXIO);

	if (uio->uio_offset >= M41T00_NBYTES)
		return (EINVAL);

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return (error);

	while (uio->uio_resid && uio->uio_offset < M41T00_NBYTES) {
		a = (int)uio->uio_offset;

		cmdbuf[0] = a;
		if ((error = uiomove(&cmdbuf[1], 1, uio)) != 0)
			break;

		if ((error = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
				      sc->sc_address,
				      cmdbuf, 1, &cmdbuf[1], 1, 0)) != 0) {
			printf("%s: m41t00_write: write failed at 0x%x\n",
			    sc->sc_dev.dv_xname, a);
			break;
		}
	}

	iic_release_bus(sc->sc_tag, 0);

	return (error);
}

static int
m41t00_gettime(struct todr_chip_handle *ch, volatile struct timeval *tv)
{
	struct m41t00_softc *sc = ch->cookie;
	struct clock_ymdhms dt;

	if (m41t00_clock_read(sc, &dt) == 0)
		return (-1);

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = 0;

	return (0);
}

static int
m41t00_settime(struct todr_chip_handle *ch, volatile struct timeval *tv)
{
	struct m41t00_softc *sc = ch->cookie;
	struct clock_ymdhms dt;

	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	if (m41t00_clock_write(sc, &dt) == 0)
		return (-1);

	return (0);
}

static int m41t00_rtc_offset[] = {
	M41T00_SEC,
	M41T00_MIN,
	M41T00_CENHR,
	M41T00_DAY,
	M41T00_DATE,
	M41T00_MONTH,
	M41T00_YEAR,
};

static int
m41t00_clock_read(struct m41t00_softc *sc, struct clock_ymdhms *dt)
{
	u_int8_t bcd[M41T00_NBYTES], cmdbuf[1];
	int i, n;

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		printf("%s: m41t00_clock_read: failed to acquire I2C bus\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}

	/* Read each timekeeping register in order. */
	n = sizeof(m41t00_rtc_offset) / sizeof(m41t00_rtc_offset[0]);
	for (i = 0; i < n ; i++) {
		cmdbuf[0] = m41t00_rtc_offset[i];

		if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
			     sc->sc_address, cmdbuf, 1,
			     &bcd[i], 1, I2C_F_POLL)) {
			iic_release_bus(sc->sc_tag, I2C_F_POLL);
			printf("%s: m41t00_clock_read: failed to read rtc "
			    "at 0x%x\n", sc->sc_dev.dv_xname,
			    m41t00_rtc_offset[i]);
			return (0);
		}
	}

	/* Done with I2C */
	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	/*
	 * Convert the M41T00's register values into something useable
	 */
	dt->dt_sec = FROMBCD(bcd[M41T00_SEC] & M41T00_SEC_MASK);
	dt->dt_min = FROMBCD(bcd[M41T00_MIN] & M41T00_MIN_MASK);
	dt->dt_hour = FROMBCD(bcd[M41T00_CENHR] & M41T00_HOUR_MASK);
	dt->dt_day = FROMBCD(bcd[M41T00_DATE] & M41T00_DATE_MASK);
	dt->dt_wday = FROMBCD(bcd[M41T00_DAY] & M41T00_DAY_MASK);
	dt->dt_mon = FROMBCD(bcd[M41T00_MONTH] & M41T00_MONTH_MASK);
	dt->dt_year = FROMBCD(bcd[M41T00_YEAR] & M41T00_YEAR_MASK);

	/*
	 * Since the m41t00 just stores 00-99, and this is 2003 as I write
	 * this comment, use 2000 as a base year
	 */
	dt->dt_year += 2000;

	return (1);
}

static int
m41t00_clock_write(struct m41t00_softc *sc, struct clock_ymdhms *dt)
{
	uint8_t bcd[M41T00_DATE_BYTES], cmdbuf[2];
	uint8_t init_seconds, final_seconds;
	int i;

	/*
	 * Convert our time representation into something the MAX6900
	 * can understand.
	 */
	bcd[M41T00_SEC] = TOBCD(dt->dt_sec);
	bcd[M41T00_MIN] = TOBCD(dt->dt_min);
	bcd[M41T00_CENHR] = TOBCD(dt->dt_hour);
	bcd[M41T00_DATE] = TOBCD(dt->dt_day);
	bcd[M41T00_DAY] = TOBCD(dt->dt_wday);
	bcd[M41T00_MONTH] = TOBCD(dt->dt_mon);
	bcd[M41T00_YEAR] = TOBCD(dt->dt_year % 100);

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		printf("%s: m41t00_clock_write: failed to acquire I2C bus\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}

	/*
	 * The MAX6900 RTC manual recommends ensuring "atomicity" of
	 * a non-burst write by:
	 *
	 *	- writing SECONDS
	 *	- reading back SECONDS, remembering it as "initial seconds"
	 *	- write the remaing RTC registers
	 *	- read back SECONDS as "final seconds"
	 *	- if "initial seconds" == 59, ensure "final seconds" == 59
	 *	- else, ensure "final seconds" is no more than one second
	 *	  beyond "initial seconds".
	 *
	 * This sounds reasonable for the M41T00, too.
	 */
 again:
	cmdbuf[0] = M41T00_SEC;
	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_address,
		     cmdbuf, 1, &bcd[M41T00_SEC], 1, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		printf("%s: m41t00_clock_write: failed to write SECONDS\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}

	cmdbuf[0] = M41T00_SEC;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_address,
		     cmdbuf, 1, &init_seconds, 1, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		printf("%s: m41t00_clock_write: failed to read "
		    "INITIAL SECONDS\n", sc->sc_dev.dv_xname);
		return (0);
	}
	init_seconds = FROMBCD(init_seconds & M41T00_SEC_MASK);

	for (i = 1; i < M41T00_DATE_BYTES; i++) {
		cmdbuf[0] = m41t00_rtc_offset[i];
		if (iic_exec(sc->sc_tag,
			     I2C_OP_WRITE_WITH_STOP, sc->sc_address,
			     cmdbuf, 1, &bcd[i], 1, I2C_F_POLL)) {
			iic_release_bus(sc->sc_tag, I2C_F_POLL);
			printf("%s: m41t00_clock_write: failed to write rtc "
			    " at 0x%x\n", sc->sc_dev.dv_xname,
			    m41t00_rtc_offset[i]);
			return (0);
		}
	}

	cmdbuf[0] = M41T00_SEC;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_address,
		     cmdbuf, 1, &final_seconds, 1, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		printf("%s: m41t00_clock_write: failed to read "
		    "FINAL SECONDS\n", sc->sc_dev.dv_xname);
		return (0);
	}
	final_seconds = FROMBCD(final_seconds & M41T00_SEC_MASK);

	if ((init_seconds != final_seconds) &&
	    (((init_seconds + 1) % 60) != final_seconds)) {
#if 1
		printf("%s: m41t00_clock_write: init %d, final %d, try again\n",
		    sc->sc_dev.dv_xname, init_seconds, final_seconds);
#endif
		goto again;
	}

	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return (1);
}
