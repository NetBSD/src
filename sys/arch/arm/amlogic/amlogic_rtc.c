/* $NetBSD: amlogic_rtc.c,v 1.1.20.2 2017/12/03 11:35:51 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: amlogic_rtc.c,v 1.1.20.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/clock_subr.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_rtcreg.h>
#include <arm/amlogic/amlogic_var.h>

#define RESET_RETRY_TIMES	3
#define RTC_COMM_DELAY		5
#define RTC_RESET_DELAY		100
#define RTC_STATIC_VALUE_INIT	0x180a	/* XXX: MAGIC? */

struct amlogic_rtc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct todr_chip_handle	sc_todr;
	int			sc_osc_failed;
	unsigned int		sc_busy;
};

static int amlogic_rtc_match(device_t, cfdata_t, void *);
static void amlogic_rtc_attach(device_t, device_t, void *);
static int amlogic_rtc_todr_gettime(todr_chip_handle_t, struct timeval *);
static int amlogic_rtc_todr_settime(todr_chip_handle_t, struct timeval *);

CFATTACH_DECL_NEW(amlogic_rtc, sizeof(struct amlogic_rtc_softc),
	amlogic_rtc_match, amlogic_rtc_attach, NULL, NULL);

#define RTC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define RTC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static inline void
setbits(struct amlogic_rtc_softc *sc, uint32_t reg, uint32_t bits)
{

	RTC_WRITE(sc, reg, RTC_READ(sc, reg) | bits);
}

static inline void
clrbits(struct amlogic_rtc_softc *sc, uint32_t reg, uint32_t bits)
{

	RTC_WRITE(sc, reg, RTC_READ(sc, reg) & ~bits);
}

static int
amlogic_rtc_check_osc_clk(struct amlogic_rtc_softc *sc)
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
amlogic_rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
amlogic_rtc_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_rtc_softc * const sc = device_private(self);
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	sc->sc_osc_failed = amlogic_rtc_check_osc_clk(sc);

	memset(&sc->sc_todr, 0, sizeof(sc->sc_todr));
	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = amlogic_rtc_todr_gettime;
	sc->sc_todr.todr_settime = amlogic_rtc_todr_settime;

	aprint_naive("\n");
	aprint_normal(": RTC");
	if (sc->sc_osc_failed) {
		aprint_error(" battery not present or discharged\n");
	} else {
		aprint_normal("\n");
		todr_attach(&sc->sc_todr);
	}
}

static void
amlogic_rtc_sclk_pulse(struct amlogic_rtc_softc *sc)
{

	delay(RTC_COMM_DELAY);
	setbits(sc, AO_RTC_REG0, AO_RTC_REG0_SCLK);

	delay(RTC_COMM_DELAY);
	clrbits(sc, AO_RTC_REG0, AO_RTC_REG0_SCLK);
}

static void
amlogic_rtc_send_bit(struct amlogic_rtc_softc *sc, uint32_t bitset)
{

	if (bitset)
		setbits(sc, AO_RTC_REG0, AO_RTC_REG0_SDI);
	else
		clrbits(sc, AO_RTC_REG0, AO_RTC_REG0_SDI);

	amlogic_rtc_sclk_pulse(sc);
}

#define SERIAL_ADDR_BITS	3
#define SERIAL_DATA_BITS	32
#define	SERIAL_TYPE_ADDR	(1 << (SERIAL_ADDR_BITS - 1))
#define	SERIAL_TYPE_DATA	(1 << (SERIAL_DATA_BITS - 1))

static void
amlogic_rtc_send_data(struct amlogic_rtc_softc *sc,
    uint32_t nextbit, uint32_t data)
{

	KASSERT(nextbit == SERIAL_TYPE_ADDR || nextbit == SERIAL_TYPE_DATA);

	while (nextbit) {
		amlogic_rtc_send_bit(sc, data & nextbit);
		nextbit >>= 1;
	}
}

static uint32_t
amlogic_rtc_get_data(struct amlogic_rtc_softc *sc)
{
	uint32_t data;
	size_t i;

	data = 0;
	for (i = 0; i < SERIAL_DATA_BITS; i++) {
		amlogic_rtc_sclk_pulse(sc);
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
amlogic_rtc_set_mode(struct amlogic_rtc_softc *sc, enum serial_mode mode)
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
	amlogic_rtc_sclk_pulse(sc);

	clrbits(sc, AO_RTC_REG0, AO_RTC_REG0_SDI);
}

static int
amlogic_rtc_wait_s_ready(struct amlogic_rtc_softc *sc)
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
amlogic_rtc_comm_init(struct amlogic_rtc_softc *sc)
{

	clrbits(sc, AO_RTC_REG0,
	    AO_RTC_REG0_SEN | AO_RTC_REG0_SCLK | AO_RTC_REG0_SDI);

	if (amlogic_rtc_wait_s_ready(sc)) {
		setbits(sc, AO_RTC_REG0, AO_RTC_REG0_SEN);
		return 0;
	}
	return -1;
}

static void
amlogic_rtc_static_register_write(struct amlogic_rtc_softc *sc, uint32_t data)
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
amlogic_rtc_reset(struct amlogic_rtc_softc *sc)
{

	amlogic_rtc_static_register_write(sc, RTC_STATIC_VALUE_INIT);
}

static int
amlogic_rtc_serial_init(struct amlogic_rtc_softc *sc)
{
	size_t init_cnt, retry_cnt;

	init_cnt = 0;
	retry_cnt = 0;
	while (amlogic_rtc_comm_init(sc) == -1) {
		if (init_cnt++ == RESET_RETRY_TIMES) {
			init_cnt = 0;
			if (retry_cnt++ == RESET_RETRY_TIMES) {
				aprint_error_dev(sc->sc_dev,
				    "cannot init rtc\n");
				return -1;
			}
			amlogic_rtc_reset(sc);
		}
		delay(RTC_RESET_DELAY);
	}
	return 0;
}

static int
amlogic_rtc_serial_read(struct amlogic_rtc_softc *sc, uint32_t addr,
    uint32_t *sec)
{

	if (amlogic_rtc_serial_init(sc) == -1)
		return EIO;

	amlogic_rtc_send_data(sc, SERIAL_TYPE_ADDR, addr);
	amlogic_rtc_set_mode(sc, SERIAL_MODE_READ);
	*sec = amlogic_rtc_get_data(sc);
	return 0;
}

static int
amlogic_rtc_serial_write(struct amlogic_rtc_softc *sc, uint32_t addr,
    uint32_t data)
{

	if (amlogic_rtc_serial_init(sc) == -1)
		return EIO;

	amlogic_rtc_send_data(sc, SERIAL_TYPE_DATA, data);
	amlogic_rtc_send_data(sc, SERIAL_TYPE_ADDR, addr);
	amlogic_rtc_set_mode(sc, SERIAL_MODE_WRITE);
	return 0;
}

static int
amlogic_rtc_todr_gettime(todr_chip_handle_t ch, struct timeval *tv)
{
	struct amlogic_rtc_softc * const sc = ch->cookie;
	uint32_t sec;
	int rv;

	if (atomic_swap_uint(&sc->sc_busy, 1))
		return EBUSY;	/* XXX: EAGAIN? */

	rv = amlogic_rtc_serial_read(sc, RTC_COUNTER_ADDR, &sec);
	sc->sc_busy = 0;

	if (rv == 0) {
		tv->tv_sec = sec;
		tv->tv_usec = 0;
	}
	return rv;
}

static int
amlogic_rtc_todr_settime(todr_chip_handle_t ch, struct timeval *tv)
{
	struct amlogic_rtc_softc * const sc = ch->cookie;
	int rv;

	if (atomic_swap_uint(&sc->sc_busy, 1))
		return EBUSY;	/* XXX: EAGAIN? */

	rv = amlogic_rtc_serial_write(sc, RTC_COUNTER_ADDR, tv->tv_sec);
	sc->sc_busy = 0;

	return rv;
}
