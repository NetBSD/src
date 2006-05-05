/*	$NetBSD: owtemp.c,v 1.3 2006/05/05 18:04:42 thorpej Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: owtemp.c,v 1.3 2006/05/05 18:04:42 thorpej Exp $");

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

	struct envsys_tre_data		sc_sensor[1];
	struct envsys_basic_info	sc_info[1];

	struct sysmon_envsys		sc_sysmon;

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

static const struct envsys_range owtemp_ranges[] = {
	{ 0, 1,		ENVSYS_STEMP },
	{ 1, 0,		-1 },
};

static int	owtemp_gtredata(struct sysmon_envsys *,
				struct envsys_tre_data *);
static int	owtemp_streinfo(struct sysmon_envsys *,
				struct envsys_basic_info *);

static uint32_t	owtemp_decode_ds18b20(const uint8_t *);
static uint32_t	owtemp_decode_ds1920(const uint8_t *);

int
owtemp_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (onewire_matchbyfam(aux, owtemp_fams,
	    sizeof(owtemp_fams) /sizeof(owtemp_fams[0])));
}

void
owtemp_attach(struct device *parent, struct device *self, void *aux)
{
	struct owtemp_softc *sc = device_private(self);
	struct onewire_attach_args *oa = aux;
	prop_string_t desc;

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

	/* Initialize sensor */
	sc->sc_sensor[0].sensor = sc->sc_info[0].sensor = 0;
	sc->sc_sensor[0].validflags = ENVSYS_FVALID;
	sc->sc_info[0].validflags = ENVSYS_FVALID;
	sc->sc_sensor[0].warnflags = ENVSYS_WARN_OK;

	sc->sc_sensor[0].units = sc->sc_info[0].units = ENVSYS_STEMP;
	desc = prop_dictionary_get(device_properties(&sc->sc_dev),
				   "description");
	if (desc != NULL &&
	    prop_object_type(desc) == PROP_TYPE_STRING &&
	    prop_string_size(desc) > 0)
		strcpy(sc->sc_info[0].desc, prop_string_cstring_nocopy(desc));
	else
		strcpy(sc->sc_info[0].desc, sc->sc_dev.dv_xname);

	/* Hook into system monitor. */
	sc->sc_sysmon.sme_ranges = owtemp_ranges;
	sc->sc_sysmon.sme_sensor_info = sc->sc_info;
	sc->sc_sysmon.sme_sensor_data = sc->sc_sensor;
	sc->sc_sysmon.sme_cookie = sc;

	sc->sc_sysmon.sme_gtredata = owtemp_gtredata;
	sc->sc_sysmon.sme_streinfo = owtemp_streinfo;

	sc->sc_sysmon.sme_nsensors = 1;
	sc->sc_sysmon.sme_envsys_version = 1000;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);

#if 0  /* Old OpenBSD code */
	strlcpy(sc->sc_sensor.device, sc->sc_dev.dv_xname,
	    sizeof(sc->sc_sensor.device));
	sc->sc_sensor.type = SENSOR_TEMP;
	strlcpy(sc->sc_sensor.desc, "Temp", sizeof(sc->sc_sensor.desc));

	if (sensor_task_register(sc, owtemp_update, 5)) {
		printf(": unable to register update task\n");
		return;
	}
	sensor_add(&sc->sc_sensor);
#endif

	printf("\n");
}

int
owtemp_detach(struct device *self, int flags)
{
	struct owtemp_softc *sc = device_private(self);

	sysmon_envsys_unregister(&sc->sc_sysmon);

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

	onewire_lock(sc->sc_onewire, 0);
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

	sc->sc_sensor[0].cur.data_us = sc->sc_owtemp_decode(data);
	sc->sc_sensor[0].validflags |= ENVSYS_FCURVALID;

done:
	onewire_unlock(sc->sc_onewire);
}

static int
owtemp_gtredata(struct sysmon_envsys *sme, struct envsys_tre_data *tred)
{
	struct owtemp_softc *sc = sme->sme_cookie;

	owtemp_update(sc);
	*tred = sc->sc_sensor[tred->sensor];

	return 0;
}

static int
owtemp_streinfo(struct sysmon_envsys *sme, struct envsys_basic_info *binfo)
{
	struct owtemp_softc *sc = sme->sme_cookie;

	onewire_lock(sc->sc_onewire,0);	/* Also locks our instance */

	memcpy(sc->sc_info[binfo->sensor].desc, binfo->desc,
	    sizeof(sc->sc_info[binfo->sensor].desc));
	sc->sc_info[binfo->sensor].desc[
	    sizeof(sc->sc_info[binfo->sensor].desc) - 1] = '\0';

	onewire_unlock(sc->sc_onewire);

	binfo->validflags = ENVSYS_FVALID;

	return 0;
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
