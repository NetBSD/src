/*	$NetBSD: owtemp.c,v 1.19 2019/11/30 23:06:52 ad Exp $	*/
/*	$OpenBSD: owtemp.c,v 1.1 2006/03/04 16:27:03 grange Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Copyright (c) 2006 Alexander Yurchenko <grange@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * 1-Wire temperature family type device driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: owtemp.c,v 1.19 2019/11/30 23:06:52 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/onewire/onewiredevs.h>
#include <dev/onewire/onewirereg.h>
#include <dev/onewire/onewirevar.h>

#define DS_CMD_CONVERT		0x44
#define DS_CMD_READ_SCRATCHPAD	0xbe

struct owtemp_softc {
	device_t			sc_dv;
	void				*sc_onewire;
	u_int64_t			sc_rom;
	const char			*sc_chipname;

	envsys_data_t			sc_sensor;
	struct sysmon_envsys		*sc_sme;

	uint32_t			(*sc_owtemp_decode)(const uint8_t *);

	int				sc_dying;

	struct evcnt			sc_ev_update;
	struct evcnt			sc_ev_rsterr;
	struct evcnt			sc_ev_crcerr;
};

static int	owtemp_match(device_t, cfdata_t, void *);
static void	owtemp_attach(device_t, device_t, void *);
static int	owtemp_detach(device_t, int);
static int	owtemp_activate(device_t, enum devact);
static bool	owtemp_update(struct owtemp_softc *sc, uint32_t *temp);
static void	owtemp_refresh(struct sysmon_envsys *, envsys_data_t *);
static uint32_t	owtemp_decode_ds18b20(const uint8_t *);
static uint32_t	owtemp_decode_ds1920(const uint8_t *);

CFATTACH_DECL_NEW(owtemp, sizeof(struct owtemp_softc),
	owtemp_match, owtemp_attach, owtemp_detach, owtemp_activate);

extern struct cfdriver owtemp_cd;

static const struct onewire_matchfam owtemp_fams[] = {
	{ ONEWIRE_FAMILY_DS1920 }, /* also DS1820 */
	{ ONEWIRE_FAMILY_DS18B20 },
	{ ONEWIRE_FAMILY_DS1822 },
};

int	owtemp_retries = 3;

static int
owtemp_match(device_t parent, cfdata_t match, void *aux)
{
	return (onewire_matchbyfam(aux, owtemp_fams,
	    __arraycount(owtemp_fams)));
}

static void
owtemp_attach(device_t parent, device_t self, void *aux)
{
	struct owtemp_softc *sc = device_private(self);
	struct onewire_attach_args *oa = aux;

	aprint_naive("\n");

	sc->sc_dv = self;
	sc->sc_onewire = oa->oa_onewire;
	sc->sc_rom = oa->oa_rom;

	switch(ONEWIRE_ROM_FAMILY_TYPE(sc->sc_rom)) {
	case ONEWIRE_FAMILY_DS18B20:
		sc->sc_chipname = "DS18B20";
		sc->sc_owtemp_decode = owtemp_decode_ds18b20;
		break;
	case ONEWIRE_FAMILY_DS1822:
		sc->sc_chipname = "DS1822";
		sc->sc_owtemp_decode = owtemp_decode_ds18b20;
		break;
	case ONEWIRE_FAMILY_DS1920:
		sc->sc_chipname = "DS1920";
		sc->sc_owtemp_decode = owtemp_decode_ds1920;
		break;
	}

	evcnt_attach_dynamic(&sc->sc_ev_update, EVCNT_TYPE_MISC, NULL,
	   device_xname(self), "update");
	evcnt_attach_dynamic(&sc->sc_ev_rsterr, EVCNT_TYPE_MISC, NULL,
	   device_xname(self), "reset error");
	evcnt_attach_dynamic(&sc->sc_ev_crcerr, EVCNT_TYPE_MISC, NULL,
	   device_xname(self), "crc error");

	sc->sc_sme = sysmon_envsys_create();

	/* Initialize sensor */
	sc->sc_sensor.units = ENVSYS_STEMP;
	sc->sc_sensor.state = ENVSYS_SINVALID;
	(void)strlcpy(sc->sc_sensor.desc,
	    device_xname(self), sizeof(sc->sc_sensor.desc));
	(void)snprintf(sc->sc_sensor.desc, sizeof(sc->sc_sensor.desc),
	    "%s S/N %012" PRIx64, sc->sc_chipname, ONEWIRE_ROM_SN(sc->sc_rom));
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	/* Hook into system monitor. */
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = owtemp_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	aprint_normal("\n");
}

static int
owtemp_detach(device_t self, int flags)
{
	struct owtemp_softc *sc = device_private(self);

	sysmon_envsys_unregister(sc->sc_sme);
	evcnt_detach(&sc->sc_ev_rsterr);
	evcnt_detach(&sc->sc_ev_update);
	evcnt_detach(&sc->sc_ev_crcerr);

	return 0;
}

static int
owtemp_activate(device_t self, enum devact act)
{
	struct owtemp_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static bool
owtemp_update(struct owtemp_softc *sc, uint32_t *temp)
{
	u_int8_t data[9];

	sc->sc_ev_update.ev_count++;

	/*
	 * Start temperature conversion. The conversion takes up to 750ms.
	 * After sending the command, the data line must be held high for
	 * at least 750ms to provide power during the conversion process.
	 * As such, no other activity may take place on the 1-Wire bus for
	 * at least this period.  Keep the parent bus locked while waiting.
	 */
	if (onewire_reset(sc->sc_onewire) != 0) {
		sc->sc_ev_rsterr.ev_count++;
		return false;
	}
	onewire_matchrom(sc->sc_onewire, sc->sc_rom);
	onewire_write_byte(sc->sc_onewire, DS_CMD_CONVERT);
	(void)kpause("owtemp", false, mstohz(750 + 10), NULL);

	/*
	 * The result of the temperature measurement is placed in the
	 * first two bytes of the scratchpad.  Perform the caculation
	 * only if the CRC is correct.
	 */
	if (onewire_reset(sc->sc_onewire) != 0) {
		sc->sc_ev_rsterr.ev_count++;
		return false;
	}
	onewire_matchrom(sc->sc_onewire, sc->sc_rom);
	onewire_write_byte(sc->sc_onewire, DS_CMD_READ_SCRATCHPAD);
	onewire_read_block(sc->sc_onewire, data, 9);
	if (onewire_crc(data, 8) != data[8]) {
		sc->sc_ev_crcerr.ev_count++;
		return false;
	}
	*temp = sc->sc_owtemp_decode(data);
	return true;
}

static void
owtemp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct owtemp_softc *sc = sme->sme_cookie;
	uint32_t reading;
	int retry;

	onewire_lock(sc->sc_onewire);
	for (retry = 0; retry < owtemp_retries; retry++) {
		if (owtemp_update(sc, &reading)) {
			onewire_unlock(sc->sc_onewire);
			sc->sc_sensor.value_cur = reading;
			sc->sc_sensor.state = ENVSYS_SVALID;
			return;
		}
	}
	onewire_unlock(sc->sc_onewire);
	aprint_error_dev(sc->sc_dv,
	    "update failed - use vmstat(8) to check event counters\n");
	sc->sc_sensor.state = ENVSYS_SINVALID;
}

static uint32_t
owtemp_decode_ds18b20(const uint8_t *buf)
{
	int temp;

	/*
	 * Sign-extend the MSB byte, and add in the fractions of a
	 * degree contained in the LSB (precision 1/16th DegC).
	 */
	temp = (int8_t)buf[1];
	temp = (temp << 8) | buf[0];

	/*
	 * Conversion to uK is simple.
	 */
	return (temp * 62500 + 273150000);
}

static uint32_t
owtemp_decode_ds1920(const uint8_t *buf)
{
	int temp;

	/*
	 * Sign-extend the MSB byte, and add in the fractions of a
	 * degree contained in the LSB (precision 1/2 DegC).
	 */
	temp = (int8_t)buf[1];
	temp = (temp << 8) | buf[0];

	if (buf[7] != 0) {
		/*
	 	 * interpolate for higher precision using the count registers
	 	 *
	 	 * buf[7]: COUNT_PER_C(elsius)
	 	 * buf[6]: COUNT_REMAIN
	 	 *
	 	 * T = TEMP - 0.25 + (COUNT_PER_C - COUNT_REMAIN) / COUNT_PER_C
	 	 */
		temp &= ~1;
        	temp += 500000 * temp + (500000 * (buf[7] - buf[6])) / buf[7] - 250000;
	} else {
		temp *= 500000;
	}

	/* convert to uK */
	return (temp + 273150000);
}
