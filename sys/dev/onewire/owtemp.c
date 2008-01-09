/*	$NetBSD: owtemp.c,v 1.10.8.2 2008/01/09 01:53:27 matt Exp $ */
/*	$OpenBSD: owtemp.c,v 1.1 2006/03/04 16:27:03 grange Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: owtemp.c,v 1.10.8.2 2008/01/09 01:53:27 matt Exp $");

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
	struct device			sc_dev;

	void *				sc_onewire;
	u_int64_t			sc_rom;

	envsys_data_t			sc_sensor;
	struct sysmon_envsys		*sc_sme;

	uint32_t			(*sc_owtemp_decode)(const uint8_t *);

	int				sc_dying;
};

int	owtemp_match(struct device *, struct cfdata *, void *);
void	owtemp_attach(struct device *, struct device *, void *);
int	owtemp_detach(struct device *, int);
int	owtemp_activate(struct device *, enum devact);

void	owtemp_update(void *);

CFATTACH_DECL(owtemp, sizeof(struct owtemp_softc),
	owtemp_match, owtemp_attach, owtemp_detach, owtemp_activate);

extern struct cfdriver owtemp_cd;

static const struct onewire_matchfam owtemp_fams[] = {
	{ ONEWIRE_FAMILY_DS1920 },
	{ ONEWIRE_FAMILY_DS18B20 },
	{ ONEWIRE_FAMILY_DS1822 },
};

static void	owtemp_refresh(struct sysmon_envsys *, envsys_data_t *);

static uint32_t	owtemp_decode_ds18b20(const uint8_t *);
static uint32_t	owtemp_decode_ds1920(const uint8_t *);

int
owtemp_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (onewire_matchbyfam(aux, owtemp_fams,
	    __arraycount(owtemp_fams)));
}

void
owtemp_attach(struct device *parent, struct device *self, void *aux)
{
	struct owtemp_softc *sc = device_private(self);
	struct onewire_attach_args *oa = aux;

	sc->sc_onewire = oa->oa_onewire;
	sc->sc_rom = oa->oa_rom;

	switch(ONEWIRE_ROM_FAMILY_TYPE(sc->sc_rom)) {
	case ONEWIRE_FAMILY_DS18B20:
	case ONEWIRE_FAMILY_DS1822:
		sc->sc_owtemp_decode = owtemp_decode_ds18b20;
		break;
	case ONEWIRE_FAMILY_DS1920:
		sc->sc_owtemp_decode = owtemp_decode_ds1920;
		break;
	}

	sc->sc_sme = sysmon_envsys_create();

	/* Initialize sensor */
	sc->sc_sensor.units = ENVSYS_STEMP;
	(void)strlcpy(sc->sc_sensor.desc,
	    sc->sc_dev.dv_xname, sizeof(sc->sc_sensor.desc));
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	/* Hook into system monitor. */
	sc->sc_sme->sme_name = sc->sc_dev.dv_xname;
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = owtemp_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	printf("\n");
}

int
owtemp_detach(struct device *self, int flags)
{
	struct owtemp_softc *sc = device_private(self);

	sysmon_envsys_unregister(sc->sc_sme);

	return 0;
}

int
owtemp_activate(struct device *self, enum devact act)
{
	struct owtemp_softc *sc = device_private(self);

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		break;
	}

	return (0);
}

void
owtemp_update(void *arg)
{
	struct owtemp_softc *sc = arg;
	u_int8_t data[9];

	onewire_lock(sc->sc_onewire);
	if (onewire_reset(sc->sc_onewire) != 0)
		goto done;
	onewire_matchrom(sc->sc_onewire, sc->sc_rom);

	/*
	 * Start temperature conversion. The conversion takes up to 750ms.
	 * After sending the command, the data line must be held high for
	 * at least 750ms to provide power during the conversion process.
	 * As such, no other activity may take place on the 1-Wire bus for
	 * at least this period.
	 */
	onewire_write_byte(sc->sc_onewire, DS_CMD_CONVERT);
	tsleep(sc, PRIBIO, "owtemp", hz);

	if (onewire_reset(sc->sc_onewire) != 0)
		goto done;
	onewire_matchrom(sc->sc_onewire, sc->sc_rom);

	/*
	 * The result of the temperature measurement is placed in the
	 * first two bytes of the scratchpad.
	 */
	onewire_write_byte(sc->sc_onewire, DS_CMD_READ_SCRATCHPAD);
	onewire_read_block(sc->sc_onewire, data, 9);
#if 0
	if (onewire_crc(data, 8) == data[8]) {
		sc->sc_sensor.value = 273150000 +
		    (int)((u_int16_t)data[1] << 8 | data[0]) * 500000;
	}
#endif

	sc->sc_sensor.value_cur = sc->sc_owtemp_decode(data);
	sc->sc_sensor.state = ENVSYS_SVALID;

done:
	onewire_unlock(sc->sc_onewire);
}

static void
owtemp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct owtemp_softc *sc = sme->sme_cookie;

	owtemp_update(sc);
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

	/* Convert to uK */
	return (temp * 500000 + 273150000);
}
