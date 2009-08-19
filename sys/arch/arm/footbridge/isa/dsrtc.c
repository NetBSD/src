/*	$NetBSD: dsrtc.c,v 1.10.50.1 2009/08/19 18:45:59 yamt Exp $	*/

/*
 * Copyright (c) 1998 Mark Brinicombe.
 * Copyright (c) 1998 Causality Limited.
 * All rights reserved.
 *
 * Written by Mark Brinicombe, Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUASLITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dsrtc.c,v 1.10.50.1 2009/08/19 18:45:59 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <dev/clock_subr.h>
#include <arm/footbridge/isa/ds1687reg.h>

#include <dev/isa/isavar.h>

#define NRTC_PORTS	2

struct dsrtc_softc {
	struct device	sc_dev;
	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;
	struct todr_chip_handle sc_todr;
};

void dsrtcattach(struct device *parent, struct device *self, void *aux);
int dsrtcmatch(struct device *parent, struct cfdata *cf, void *aux);
int ds1687_read(struct dsrtc_softc *sc, int addr);
void ds1687_write(struct dsrtc_softc *sc, int addr, int data);
#if 0
int ds1687_ram_read(struct dsrtc_softc *sc, int addr);
void ds1687_ram_write(struct dsrtc_softc *sc, int addr, int data);
#endif
static void ds1687_bank_select(struct dsrtc_softc *, int);
static int dsrtc_write(todr_chip_handle_t, struct clock_ymdhms *);
static int dsrtc_read(todr_chip_handle_t, struct clock_ymdhms *);

int
ds1687_read(struct dsrtc_softc *sc, int addr)
{

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_ADDR_REG, addr);
	return(bus_space_read_1(sc->sc_iot, sc->sc_ioh, RTC_DATA_REG));
}

void
ds1687_write(struct dsrtc_softc *sc, int addr, int data)
{

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_ADDR_REG, addr);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_DATA_REG, data);
}

static void
ds1687_bank_select(struct dsrtc_softc *sc, int bank)
{
	int data;

	data = ds1687_read(sc, RTC_REG_A);
	data &= ~RTC_REG_A_BANK_MASK;
	if (bank)
		data |= RTC_REG_A_BANK1;
	ds1687_write(sc, RTC_REG_A, data);
}

#if 0
/* Nothing uses these yet */
int
ds1687_ram_read(struct dsrtc_softc *sc, int addr)
{
	if (addr < RTC_PC_RAM_SIZE)
		return(ds1687_read(sc, RTC_PC_RAM_START + addr));

	addr -= RTC_PC_RAM_SIZE;
	if (addr < RTC_BANK0_RAM_SIZE)
		return(ds1687_read(sc, RTC_BANK0_RAM_START + addr));		

	addr -= RTC_BANK0_RAM_SIZE;
	if (addr < RTC_EXT_RAM_SIZE) {
		int data;

		ds1687_bank_select(sc, 1);
		ds1687_write(sc, RTC_EXT_RAM_ADDRESS, addr);
		data = ds1687_read(sc, RTC_EXT_RAM_DATA);
		ds1687_bank_select(sc, 0);
		return(data);
	}
	return(-1);
}

void
ds1687_ram_write(struct dsrtc_softc *sc, int addr, int val)
{
	if (addr < RTC_PC_RAM_SIZE)
		return(ds1687_write(sc, RTC_PC_RAM_START + addr, val));

	addr -= RTC_PC_RAM_SIZE;
	if (addr < RTC_BANK0_RAM_SIZE)
		return(ds1687_write(sc, RTC_BANK0_RAM_START + addr, val));

	addr -= RTC_BANK0_RAM_SIZE;
	if (addr < RTC_EXT_RAM_SIZE) {
		ds1687_bank_select(sc, 1);
		ds1687_write(sc, RTC_EXT_RAM_ADDRESS, addr);
		ds1687_write(sc, RTC_EXT_RAM_DATA, val);
		ds1687_bank_select(sc, 0);
	}
}
#endif

static int
dsrtc_write(todr_chip_handle_t tc, struct clock_ymdhms *dt)
{
	struct dsrtc_softc *sc = tc->cookie;

	ds1687_write(sc, RTC_SECONDS, dt->dt_sec);
	ds1687_write(sc, RTC_MINUTES, dt->dt_min);
	ds1687_write(sc, RTC_HOURS, dt->dt_hour);
	ds1687_write(sc, RTC_DAYOFMONTH, dt->dt_day);
	ds1687_write(sc, RTC_MONTH, dt->dt_mon);
	ds1687_write(sc, RTC_YEAR, dt->dt_year % 100);
	ds1687_bank_select(sc, 1);
	ds1687_write(sc, RTC_CENTURY, dt->dt_year / 100);
	ds1687_bank_select(sc, 0);
	return(0);
}

static int
dsrtc_read(todr_chip_handle_t tc, struct clock_ymdhms *dt)
{
	struct dsrtc_softc *sc = tc->cookie;

	dt->dt_sec   = ds1687_read(sc, RTC_SECONDS);
	dt->dt_min   = ds1687_read(sc, RTC_MINUTES);
	dt->dt_hour  = ds1687_read(sc, RTC_HOURS);
	dt->dt_day   = ds1687_read(sc, RTC_DAYOFMONTH);
	dt->dt_mon   = ds1687_read(sc, RTC_MONTH);
	dt->dt_year  = ds1687_read(sc, RTC_YEAR);
	ds1687_bank_select(sc, 1);
	dt->dt_year  += ds1687_read(sc, RTC_CENTURY) * 100;
	ds1687_bank_select(sc, 0);

	return(0);
}

/* device and attach structures */
CFATTACH_DECL_NEW(ds1687rtc, sizeof(struct dsrtc_softc),
    dsrtcmatch, dsrtcattach, NULL, NULL);

/*
 * dsrtcmatch()
 *
 * Validate the IIC address to make sure its an RTC we understand
 */

int
dsrtcmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct isa_attach_args *ia = aux;

	if (ia->ia_nio < 1 ||
	    ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return (0);

	ia->ia_nio = 1;
	ia->ia_io[0].ir_size = NRTC_PORTS;

	ia->ia_niomem = 0;
	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;

	return(1);
}

/*
 * dsrtcattach()
 *
 * Attach the rtc device
 */

void
dsrtcattach(device_t parent, device_t self, void *aux)
{
	struct dsrtc_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;
	
	sc->sc_iot = ia->ia_iot;
	if (bus_space_map(sc->sc_iot, ia->ia_io[0].ir_addr,
	    ia->ia_io[0].ir_size, 0, &sc->sc_ioh)) {
		aprint_error(": cannot map I/O space\n");
		return;
	}

	ds1687_write(sc, RTC_REG_A, RTC_REG_A_DV1);
	ds1687_write(sc, RTC_REG_B, RTC_REG_B_BINARY | RTC_REG_B_24_HOUR);

	if (!(ds1687_read(sc, RTC_REG_D) & RTC_REG_D_VRT))
		aprint_error(": lithium cell is dead, RTC unreliable");
	aprint_normal("\n");

	sc->sc_todr.todr_gettime_ymdhms = dsrtc_read;
	sc->sc_todr.todr_settime_ymdhms = dsrtc_write;
	sc->sc_todr.cookie = sc;
	todr_attach(&sc->sc_todr);
}

/* End of dsrtc.c */
