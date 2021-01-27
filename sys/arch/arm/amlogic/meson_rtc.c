/* $NetBSD: meson_rtc.c,v 1.3 2021/01/27 03:10:18 thorpej Exp $ */

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: meson_rtc.c,v 1.3 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/clock_subr.h>

#include <arm/amlogic/meson_rtcreg.h>

#include <dev/fdt/fdtvar.h>

#define RESET_RETRY_TIMES	3
#define RTC_COMM_DELAY		5
#define RTC_RESET_DELAY		100
#define RTC_STATIC_VALUE_INIT	0x180a	/* XXX: MAGIC? */

struct meson_rtc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct todr_chip_handle	sc_todr;
	int			sc_osc_failed;
	unsigned int		sc_busy;
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson8b-rtc" },
	DEVICE_COMPAT_EOL
};

static int meson_rtc_match(device_t, cfdata_t, void *);
static void meson_rtc_attach(device_t, device_t, void *);
static int meson_rtc_todr_gettime(todr_chip_handle_t, struct timeval *);
static int meson_rtc_todr_settime(todr_chip_handle_t, struct timeval *);

CFATTACH_DECL_NEW(meson_rtc, sizeof(struct meson_rtc_softc),
	meson_rtc_match, meson_rtc_attach, NULL, NULL);

#define RTC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define RTC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static inline void
setbits(struct meson_rtc_softc *sc, uint32_t reg, uint32_t bits)
{

	RTC_WRITE(sc, reg, RTC_READ(sc, reg) | bits);
}

static inline void
clrbits(struct meson_rtc_softc *sc, uint32_t reg, uint32_t bits)
{

	RTC_WRITE(sc, reg, RTC_READ(sc, reg) & ~bits);
}

static int
meson_rtc_check_osc_clk(struct meson_rtc_softc *sc)
{
	uint32_t cnt1, cnt2;

	setbits(sc, AO_RTC_REG3, AO_RTC_REG3_COUNT_ALWAYS);

	/*
	 * Wait for 50uS.  32.768khz is 30.5uS.  This should be long
	 * enough for one full cycle of 32.768 khz.
	 */
	cnt1 = RTC_READ(sc, AO_RTC_REG2);
	delay(50);
	cnt2 = RTC_READ(sc, AO_RTC_REG2);

	clrbits(sc, AO_RTC_REG3, AO_RTC_REG3_COUNT_ALWAYS);

	return cnt1 == cnt2;
}

static int
meson_rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson_rtc_attach(device_t parent, device_t self, void *aux)
{
	struct meson_rtc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_osc_failed = meson_rtc_check_osc_clk(sc);

	memset(&sc->sc_todr, 0, sizeof(sc->sc_todr));
	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = meson_rtc_todr_gettime;
	sc->sc_todr.todr_settime = meson_rtc_todr_settime;

	aprint_naive("\n");
	aprint_normal(": RTC");
	if (sc->sc_osc_failed) {
		aprint_normal(" battery not present or discharged\n");
	} else {
		aprint_normal("\n");
		fdtbus_todr_attach(self, phandle, &sc->sc_todr);
	}
}

static void
meson_rtc_sclk_pulse(struct meson_rtc_softc *sc)
{

	delay(RTC_COMM_DELAY);
	setbits(sc, AO_RTC_REG0, AO_RTC_REG0_SCLK);

	delay(RTC_COMM_DELAY);
	clrbits(sc, AO_RTC_REG0, AO_RTC_REG0_SCLK);
}

static void
meson_rtc_send_bit(struct meson_rtc_softc *sc, uint32_t bitset)
{

	if (bitset)
		setbits(sc, AO_RTC_REG0, AO_RTC_REG0_SDI);
	else
		clrbits(sc, AO_RTC_REG0, AO_RTC_REG0_SDI);

	meson_rtc_sclk_pulse(sc);
}

#define SERIAL_ADDR_BITS	3
#define SERIAL_DATA_BITS	32
#define	SERIAL_TYPE_ADDR	(1 << (SERIAL_ADDR_BITS - 1))
#define	SERIAL_TYPE_DATA	(1 << (SERIAL_DATA_BITS - 1))

static void
meson_rtc_send_data(struct meson_rtc_softc *sc,
    uint32_t nextbit, uint32_t data)
{

	KASSERT(nextbit == SERIAL_TYPE_ADDR || nextbit == SERIAL_TYPE_DATA);

	while (nextbit) {
		meson_rtc_send_bit(sc, data & nextbit);
		nextbit >>= 1;
	}
}

static uint32_t
meson_rtc_get_data(struct meson_rtc_softc *sc)
{
	uint32_t data;
	size_t i;

	data = 0;
	for (i = 0; i < SERIAL_DATA_BITS; i++) {
		meson_rtc_sclk_pulse(sc);
		data <<= 1;
		data |= __SHIFTOUT(RTC_READ(sc, AO_RTC_REG1), AO_RTC_REG1_SDO);
	}
	return data;
}

enum serial_mode {
	SERIAL_MODE_READ,
	SERIAL_MODE_WRITE,
};

static void
meson_rtc_set_mode(struct meson_rtc_softc *sc, enum serial_mode mode)
{

	clrbits(sc, AO_RTC_REG0, AO_RTC_REG0_SEN);

	switch(mode) {
	case SERIAL_MODE_READ:
		clrbits(sc, AO_RTC_REG0, AO_RTC_REG0_SDI);
		break;
	case SERIAL_MODE_WRITE:
		setbits(sc, AO_RTC_REG0, AO_RTC_REG0_SDI);
		break;
	default:
		KASSERT(1);
		return;
	}
	meson_rtc_sclk_pulse(sc);

	clrbits(sc, AO_RTC_REG0, AO_RTC_REG0_SDI);
}

static int
meson_rtc_wait_s_ready(struct meson_rtc_softc *sc)
{
	size_t s_nrdy_cnt, retry_cnt;

	s_nrdy_cnt = 40000;
	retry_cnt = 0;
	while (!(RTC_READ(sc, AO_RTC_REG1) & AO_RTC_REG1_S_READY)) {
		if (s_nrdy_cnt-- == 0) {
			s_nrdy_cnt = 40000;
			if (retry_cnt++ == RESET_RETRY_TIMES)
				return 0;
			/* XXX: reset_s_ready?  Linux does not. */
			setbits(sc, AO_RTC_REG1, AO_RTC_REG1_S_READY);
			delay(RTC_RESET_DELAY);
		}
	}
	return 1;
}

static int
meson_rtc_comm_init(struct meson_rtc_softc *sc)
{

	clrbits(sc, AO_RTC_REG0,
	    AO_RTC_REG0_SEN | AO_RTC_REG0_SCLK | AO_RTC_REG0_SDI);

	if (meson_rtc_wait_s_ready(sc)) {
		setbits(sc, AO_RTC_REG0, AO_RTC_REG0_SEN);
		return 0;
	}
	return -1;
}

static void
meson_rtc_static_register_write(struct meson_rtc_softc *sc, uint32_t data)
{
	uint32_t u;

	/* Program MSB 15-8 */
	u = RTC_READ(sc, AO_RTC_REG4);
	u &= AO_RTC_REG4_STATIC_REG_MSB;
	u |= __SHIFTIN(data, AO_RTC_REG4_STATIC_REG_MSB);
	RTC_WRITE(sc, AO_RTC_REG4, u);

	/* Program LSB 7-0, and start serializing */
	u = RTC_READ(sc, AO_RTC_REG0);
	u &= ~AO_RTC_REG0_STATIC_REG_LSB;
	u |= __SHIFTIN(data, AO_RTC_REG0_STATIC_REG_LSB);
	u |= AO_RTC_REG0_SERIAL_START;
	RTC_WRITE(sc, AO_RTC_REG0, u);

	/* Poll auto_serializer_busy bit until it's low (IDLE) */
	while ((RTC_READ(sc, AO_RTC_REG0) & AO_RTC_REG0_SERIAL_BUSY) != 0)
		continue;
}

static void
meson_rtc_reset(struct meson_rtc_softc *sc)
{

	meson_rtc_static_register_write(sc, RTC_STATIC_VALUE_INIT);
}

static int
meson_rtc_serial_init(struct meson_rtc_softc *sc)
{
	size_t init_cnt, retry_cnt;

	init_cnt = 0;
	retry_cnt = 0;
	while (meson_rtc_comm_init(sc) == -1) {
		if (init_cnt++ == RESET_RETRY_TIMES) {
			init_cnt = 0;
			if (retry_cnt++ == RESET_RETRY_TIMES) {
				aprint_error_dev(sc->sc_dev,
				    "cannot init rtc\n");
				return -1;
			}
			meson_rtc_reset(sc);
		}
		delay(RTC_RESET_DELAY);
	}
	return 0;
}

static int
meson_rtc_serial_read(struct meson_rtc_softc *sc, uint32_t addr,
    uint32_t *sec)
{

	if (meson_rtc_serial_init(sc) == -1)
		return EIO;

	meson_rtc_send_data(sc, SERIAL_TYPE_ADDR, addr);
	meson_rtc_set_mode(sc, SERIAL_MODE_READ);
	*sec = meson_rtc_get_data(sc);
	return 0;
}

static int
meson_rtc_serial_write(struct meson_rtc_softc *sc, uint32_t addr,
    uint32_t data)
{

	if (meson_rtc_serial_init(sc) == -1)
		return EIO;

	meson_rtc_send_data(sc, SERIAL_TYPE_DATA, data);
	meson_rtc_send_data(sc, SERIAL_TYPE_ADDR, addr);
	meson_rtc_set_mode(sc, SERIAL_MODE_WRITE);
	return 0;
}

static int
meson_rtc_todr_gettime(todr_chip_handle_t ch, struct timeval *tv)
{
	struct meson_rtc_softc * const sc = ch->cookie;
	uint32_t sec;
	int rv;

	if (atomic_swap_uint(&sc->sc_busy, 1))
		return EBUSY;	/* XXX: EAGAIN? */

	rv = meson_rtc_serial_read(sc, RTC_COUNTER_ADDR, &sec);
	sc->sc_busy = 0;

	if (rv == 0) {
		tv->tv_sec = sec;
		tv->tv_usec = 0;
	}
	return rv;
}

static int
meson_rtc_todr_settime(todr_chip_handle_t ch, struct timeval *tv)
{
	struct meson_rtc_softc * const sc = ch->cookie;
	int rv;

	if (atomic_swap_uint(&sc->sc_busy, 1))
		return EBUSY;	/* XXX: EAGAIN? */

	rv = meson_rtc_serial_write(sc, RTC_COUNTER_ADDR, tv->tv_sec);
	sc->sc_busy = 0;

	return rv;
}
