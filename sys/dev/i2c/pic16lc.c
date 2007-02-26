/* $NetBSD: pic16lc.c,v 1.6.4.2 2007/02/26 09:10:03 yamt Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 *        This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Microsoft XBOX System Management Controller
 *
 * Documentation lives here:
 *  http://www.xbox-linux.org/wiki/PIC
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pic16lc.c,v 1.6.4.2 2007/02/26 09:10:03 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/xbox.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/i2c/pic16lcreg.h>

static int	pic16lc_match(struct device *, struct cfdata *, void *);
static void	pic16lc_attach(struct device *, struct device *, void *);

static int	pic16lc_intr(void *);

void		pic16lc_reboot(void);
void		pic16lc_poweroff(void);
void		pic16lc_setled(uint8_t);

struct pic16lc_softc {
	struct device	sc_dev;

	i2c_tag_t	sc_tag;
	i2c_addr_t	sc_addr;
	void *		sc_ih;

	struct envsys_tre_data sc_data[2];
	struct envsys_basic_info sc_info[2];
	struct sysmon_envsys sc_sysmon;
};

static struct pic16lc_softc *pic16lc = NULL;

#define XBOX_SENSOR_CPU		0
#define XBOX_SENSOR_BOARD	1
#define XBOX_NSENSORS		2

static const struct envsys_range pic16lc_ranges[] = {
	{ 0, 1, ENVSYS_STEMP },
};

static void	pic16lc_update(struct pic16lc_softc *);
static int	pic16lc_gtredata(struct sysmon_envsys *,
				 struct envsys_tre_data *);
static int	pic16lc_streinfo(struct sysmon_envsys *,
				 struct envsys_basic_info *);

static void	pic16lc_write_1(struct pic16lc_softc *, uint8_t, uint8_t);
static void	pic16lc_read_1(struct pic16lc_softc *, uint8_t, uint8_t *);

CFATTACH_DECL(pic16lc, sizeof(struct pic16lc_softc),
    pic16lc_match, pic16lc_attach, NULL, NULL);

static int
pic16lc_match(struct device *parent, struct cfdata *cf, void *opaque)
{
	struct i2c_attach_args *ia;

	ia = (struct i2c_attach_args *)opaque;

	if (pic16lc != NULL) /* we can only have one... */
		return 0;
	if (ia->ia_addr != 0x10) /* must live at 0x10 on xbox */
		return 0;

	return 1;
}

static void
pic16lc_attach(struct device *parent, struct device *self, void *opaque)
{
	struct pic16lc_softc *sc;
	struct i2c_attach_args *ia;
	u_char ver[4];

	sc = (struct pic16lc_softc *)self;
	ia = (struct i2c_attach_args *)opaque;

	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	pic16lc = sc;

	/* initialize CPU sensor */
	sc->sc_data[XBOX_SENSOR_CPU].sensor =
	    sc->sc_info[XBOX_SENSOR_CPU].sensor = XBOX_SENSOR_CPU;
	sc->sc_data[XBOX_SENSOR_CPU].units =
	    sc->sc_info[XBOX_SENSOR_CPU].units = ENVSYS_STEMP;
	strcpy(sc->sc_info[XBOX_SENSOR_CPU].desc, "Xbox CPU Temp");
	/* initialize board sensor */
	sc->sc_data[XBOX_SENSOR_BOARD].sensor =
	    sc->sc_info[XBOX_SENSOR_BOARD].sensor = XBOX_SENSOR_BOARD;
	sc->sc_data[XBOX_SENSOR_BOARD].units =
	    sc->sc_info[XBOX_SENSOR_BOARD].units = ENVSYS_STEMP;
	strcpy(sc->sc_info[XBOX_SENSOR_BOARD].desc, "Xbox Board Temp");

	/* Get initial sensor values */
	pic16lc_update(sc);

	/* hook into sysmon */
	sc->sc_sysmon.sme_ranges = pic16lc_ranges;
	sc->sc_sysmon.sme_sensor_info = sc->sc_info;
	sc->sc_sysmon.sme_sensor_data = sc->sc_data;
	sc->sc_sysmon.sme_cookie = sc;
	sc->sc_sysmon.sme_gtredata = pic16lc_gtredata;
	sc->sc_sysmon.sme_streinfo = pic16lc_streinfo;
	sc->sc_sysmon.sme_nsensors = XBOX_NSENSORS;
	sc->sc_sysmon.sme_envsys_version = 1000;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);

	if (iic_acquire_bus(sc->sc_tag, 0) != 0) {
		aprint_error(": unable to acquire i2c bus\n");
		return;
	}

	aprint_normal(": Xbox System Management Controller");

	/* find the PIC version */
	pic16lc_write_1(sc, PIC16LC_REG_VER, 0x00);
	pic16lc_read_1(sc, PIC16LC_REG_VER, &ver[0]);
	pic16lc_read_1(sc, PIC16LC_REG_VER, &ver[1]);
	pic16lc_read_1(sc, PIC16LC_REG_VER, &ver[2]);
	ver[3] = '\0';

	aprint_normal(" (rev. %s)\n", ver);

	pic16lc_setled(0xff);	/* orange */

	iic_release_bus(sc->sc_tag, 0);

	/* disable reset on eject */
	pic16lc_write_1(sc, PIC16LC_REG_RESETONEJECT, 0x01);

	sc->sc_ih = iic_smbus_intr_establish_proc(sc->sc_tag, pic16lc_intr, sc);
	if (sc->sc_ih == NULL)
		aprint_error("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);

	return;
}

static void
pic16lc_write_1(struct pic16lc_softc *sc, uint8_t cmd, uint8_t val)
{
	iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_addr, &cmd, 1, &val, 1, 0);
}

static void
pic16lc_read_1(struct pic16lc_softc *sc, uint8_t cmd, uint8_t *valp)
{
	iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, &cmd, 1, valp, 1, 0);
}

static void
pic16lc_update(struct pic16lc_softc *sc)
{
	uint8_t cputemp, boardtemp;

	if (iic_acquire_bus(sc->sc_tag, 0) != 0) {
		aprint_error(": unable to acquire i2c bus\n");
		return;
	}

	pic16lc_read_1(sc, PIC16LC_REG_CPUTEMP, &cputemp);
	sc->sc_info[XBOX_SENSOR_CPU].validflags = ENVSYS_FVALID;
	sc->sc_data[XBOX_SENSOR_CPU].validflags = ENVSYS_FVALID;
	sc->sc_data[XBOX_SENSOR_CPU].cur.data_us =
	    (int)cputemp * 1000000 + 273150000;
	sc->sc_data[XBOX_SENSOR_CPU].validflags |= ENVSYS_FCURVALID;

	pic16lc_read_1(sc, PIC16LC_REG_BOARDTEMP, &boardtemp);
	sc->sc_info[XBOX_SENSOR_BOARD].validflags = ENVSYS_FVALID;
	sc->sc_data[XBOX_SENSOR_BOARD].validflags = ENVSYS_FVALID;
	sc->sc_data[XBOX_SENSOR_BOARD].cur.data_us =
	    (int)boardtemp * 1000000 + 273150000;
	sc->sc_data[XBOX_SENSOR_BOARD].validflags |= ENVSYS_FCURVALID;

	iic_release_bus(sc->sc_tag, 0);

	return;
}

static int
pic16lc_gtredata(struct sysmon_envsys *sme, struct envsys_tre_data *data)
{
	struct pic16lc_softc *sc;

	sc = (struct pic16lc_softc *)sme->sme_cookie;

	pic16lc_update(sc);
	*data = sc->sc_data[data->sensor];

	return 0;
}

static int
pic16lc_streinfo(struct sysmon_envsys *sme, struct envsys_basic_info *info)
{
	/* not implemented */
	info->validflags = 0;

	return 0;
}

static int
pic16lc_intr(void *opaque)
{
	struct pic16lc_softc *sc;
	uint8_t val;
	int rv;

	sc = (struct pic16lc_softc *)opaque;
	rv = 0;

	pic16lc_read_1(sc, PIC16LC_REG_INTSTATUS, &val);
	if (val == 0)
		return rv;

	if (val & PIC16LC_REG_INTSTATUS_POWER)
		printf("%s: power button pressed\n", pic16lc->sc_dev.dv_xname);
	if (val & PIC16LC_REG_INTSTATUS_TRAYCLOSED)
		printf("%s: tray closed\n", pic16lc->sc_dev.dv_xname);
	if (val & PIC16LC_REG_INTSTATUS_TRAYOPENING) {
		printf("%s: tray opening\n", pic16lc->sc_dev.dv_xname);
		pic16lc_write_1(sc, PIC16LC_REG_INTACK, 0x02);
	}
	if (val & PIC16LC_REG_INTSTATUS_AVPACK_UNPLUG)
		printf("%s: A/V pack removed\n", pic16lc->sc_dev.dv_xname);
	if (val & PIC16LC_REG_INTSTATUS_AVPACK_PLUG)
		printf("%s: A/V pack inserted\n", pic16lc->sc_dev.dv_xname);
	if (val & PIC16LC_REG_INTSTATUS_EJECT_BUTTON) {
		pic16lc_write_1(sc, PIC16LC_REG_INTACK, 0x04);
		pic16lc_write_1(sc, PIC16LC_REG_TRAYEJECT, 0x00);
		++rv;
	}
	if (val & PIC16LC_REG_INTSTATUS_TRAYCLOSING)
		printf("%s: tray closing\n", pic16lc->sc_dev.dv_xname);

	return rv;
}

void
pic16lc_reboot(void)
{
	if (pic16lc == NULL) {
		printf("pic16lc_reboot: driver not attached!\n");
		return;
	}
	pic16lc_write_1(pic16lc, PIC16LC_REG_POWER, PIC16LC_REG_POWER_RESET);
}

void
pic16lc_poweroff(void)
{
	if (pic16lc == NULL) {
		printf("pic16lc_poweroff: driver not attached!\n");
		return;
	}
	pic16lc_write_1(pic16lc, PIC16LC_REG_POWER, PIC16LC_REG_POWER_SHUTDOWN);
}

void
pic16lc_setled(uint8_t val)
{
	if (pic16lc == NULL) {
		printf("pic16lc_setled: driver not attached!\n");
		return;
	}
	pic16lc_write_1(pic16lc, PIC16LC_REG_LEDSEQ, val);
	pic16lc_write_1(pic16lc, PIC16LC_REG_LEDMODE, 0x01);
}
