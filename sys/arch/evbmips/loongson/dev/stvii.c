/*	$NetBSD: stvii.c,v 1.3.12.2 2017/12/03 11:36:09 jdolecek Exp $	*/

/*-
 * Copyright (C) 2011 Michael Lorenz.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * a driver for the ST7 microcontroller found in Gdium Liberty 1000 notebooks
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: stvii.c,v 1.3.12.2 2017/12/03 11:36:09 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/kthread.h>
#include <sys/proc.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <dev/i2c/i2cvar.h>

#include "opt_stvii.h"

#ifdef STVII_DEBUG
#define DPRINTF aprint_error
#else
#define DPRINTF while (0) printf
#endif

/* register definitions from OpenBSD */
#define	ST7_VERSION	0x00	/* only on later mobos */

#define	ST7_STATUS	0x01
#define	STS_LID_CLOSED		0x01
#define	STS_POWER_BTN_DOWN	0x02
#define	STS_BATTERY_PRESENT	0x04	/* not available on old mobo */
#define	STS_POWER_AVAILABLE	0x08
#define	STS_WAVELAN_BTN_DOWN	0x10	/* ``enable'' on old mobo */
#define	STS_AC_AVAILABLE	0x20
#define	ST7_CONTROL	0x02
#define	STC_DDR_CLOCK		0x01
#define	STC_CHARGE_LED_LIT	0x02
#define	STC_BEEP		0x04
#define	STC_DDR_POWER		0x08
#define	STC_TRICKLE		0x10	/* trickle charge rate */
#define	STC_RADIO_ENABLE	0x20	/* enable wavelan rf, later mobos */
#define	STC_MAIN_POWER		0x40
#define	STC_CHARGE_ENABLE	0x80
#define	ST7_BATTERY_L	0x03
#define	ST7_BATTERY_H	0x04
#define	ST7_SIGNATURE	0x05
#define	STSIG_EC_CONTROL	0x00
#define	STSIG_OS_CONTROL	0xae
/* rough battery operating state limits */
#define STSEC_BAT_MIN_VOLT	7000000	/* 7V */
#define STSEC_BAT_MAX_VOLT	8000000	/* 8V */

#define BAT_AC_PRESENT		0
#define BAT_BATTERY_PRESENT	1
#define BAT_CHARGING		2
#define BAT_CHARGE		3
#define BAT_MAX_CHARGE		4
#define BAT_NSENSORS		5

struct stvii_softc {
	device_t sc_dev;
	i2c_tag_t sc_i2c;
	int sc_address, sc_version;
	int sc_sleep;
	int sc_flags, sc_charge, sc_bat_level;
	uint8_t sc_control;
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[BAT_NSENSORS];
	struct sysmon_pswitch sc_sm_acpower;
	struct sysmon_pswitch sc_sm_lid;
	struct sysmon_pswitch sc_sm_powerbutton;
};

static void stvii_attach(device_t, device_t, void *);
static int stvii_match(device_t, cfdata_t, void *);
static void stvii_writereg(struct stvii_softc *, int, uint8_t);
static int stvii_readreg(struct stvii_softc *, int);
static void stvii_worker(void *);
static void stvii_setup_envsys(struct stvii_softc *);
static void stvii_refresh(struct sysmon_envsys *, envsys_data_t *);
static int  stvii_battery_level(struct stvii_softc *);

CFATTACH_DECL_NEW(stvii, sizeof(struct stvii_softc),
    stvii_match, stvii_attach, NULL, NULL);

void stvii_poweroff(void);
static device_t stvii_dev = NULL;

#define BAT_FULL	8000000
#define BAT_LOW		7100000

static int
stvii_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *args = aux;
	int ret = -1;
	uint8_t out = ST7_VERSION, in = 0;

	/* see if we can talk to something at address 0x40 */
	if (args->ia_addr == 0x40) {
		iic_acquire_bus(args->ia_tag, 0);
		ret = iic_exec(args->ia_tag, I2C_OP_READ_WITH_STOP, args->ia_addr,
		    &out, 1, &in, 1, 0);
		DPRINTF("%02x\n", in);
		iic_release_bus(args->ia_tag, 0);
	}
	return (ret >= 0);
}

static void
stvii_attach(device_t parent, device_t self, void *aux)
{
	struct stvii_softc *sc = device_private(self);
	struct i2c_attach_args *args = aux;
	uint8_t ver, reg;

	sc->sc_dev = self;
	stvii_dev = self;
	sc->sc_address = args->ia_addr;
	aprint_normal(": ST7 Microcontroller\n");
	sc->sc_i2c = args->ia_tag;
	ver = stvii_readreg(sc, ST7_VERSION);
	sc->sc_version = ver;
	aprint_normal_dev(sc->sc_dev, "firmware version %d.%d\n", (ver >> 4) & 0xf, ver & 0xf);
#ifdef STVII_DEBUG
	{
		int i;

		for (i = 0; i < 6; i++) {
			printf("%02x ", stvii_readreg(sc, i));
		}
		printf("\n");
	}
#endif
	stvii_writereg(sc, ST7_SIGNATURE, STSIG_OS_CONTROL);
	reg = stvii_readreg(sc, ST7_CONTROL);
	reg |= STC_RADIO_ENABLE;
	stvii_writereg(sc, ST7_CONTROL, reg);
	sc->sc_control = reg;
	reg = stvii_readreg(sc, ST7_CONTROL);

	sc->sc_bat_level = stvii_battery_level(sc);

	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL, stvii_worker, sc,
	    NULL, "stvii") != 0) {
	    	aprint_error_dev(sc->sc_dev, "Failed to start kernel thread\n");
	}

	memset(&sc->sc_sm_acpower, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_acpower.smpsw_name = "AC Power";
	sc->sc_sm_acpower.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	if (sysmon_pswitch_register(&sc->sc_sm_acpower) != 0)
		printf("%s: unable to register AC power status with sysmon\n",
		    device_xname(sc->sc_dev));
	memset(&sc->sc_sm_lid, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_lid.smpsw_name = "Lid Switch";
	sc->sc_sm_lid.smpsw_type = PSWITCH_TYPE_LID;
	if (sysmon_pswitch_register(&sc->sc_sm_lid) != 0)
		printf("%s: unable to register lid switch with sysmon\n",
		    device_xname(sc->sc_dev));
	memset(&sc->sc_sm_powerbutton, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_powerbutton.smpsw_name = "Power Button";
	sc->sc_sm_powerbutton.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->sc_sm_powerbutton) != 0)
		printf("%s: unable to register power button with sysmon\n",
		    device_xname(sc->sc_dev));
	stvii_setup_envsys(sc);
}

static void
stvii_writereg(struct stvii_softc *sc, int reg, uint8_t val)
{
	uint8_t out[2] = {reg, val};

	if ((reg < 0) || (reg > 5))
		return;

	iic_acquire_bus(sc->sc_i2c, 0);
	iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_address, out, 2, NULL, 0, 0);
	iic_release_bus(sc->sc_i2c, 0);
}

static int
stvii_readreg(struct stvii_softc *sc, int reg)
{
	uint8_t inreg[1], outreg[1];
	int ret = 1, bail = 0;

	if ((reg < 0) || (reg > 5))
		return 0xff;
	inreg[0] = 0x77;
	outreg[0] = reg;
	iic_acquire_bus(sc->sc_i2c, 0);
	while ((ret != 0) && (bail < 10)) {
		ret = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_address, outreg, 1, inreg, 1, 0);
		bail++;
		delay(10);
	}
	iic_release_bus(sc->sc_i2c, 0);
	if (ret != 0)
		return -1;
	return inreg[0];
}

static int
stvii_battery_level(struct stvii_softc *sc)
{
	int bl, bh, ret;

	bl = stvii_readreg(sc, ST7_BATTERY_L);
	bh = stvii_readreg(sc, ST7_BATTERY_H);
	ret = (bl & 3) | (bh << 2);
	ret = ((ret * 10000) / 1024) * 1000;
	return ret;
}

static void
stvii_worker(void *cookie)
{
	struct stvii_softc *sc = cookie;
	int status = 0, st, cnt = 4;
	int bl;
	int charging = 0;
	int ok = TRUE;
	uint8_t nctrl;

	/* if we were charging when we took over, keep charging */
	if (sc->sc_control & STC_CHARGE_ENABLE)
		charging = 1;

	while (ok) {
		st = stvii_readreg(sc, ST7_STATUS);
		/*
		 * I get i2c timeouts when the power button is pressed.
		 * According to the linux driver this happens on firmware
		 * version 0x13 and newer, mine is 0x16.
		 * So, when we see read errors on the right version we assume
		 * it's the power button as long as the lid is open
		 * ( the button is inside the lid )
		 */
		if ((st == -1) && (sc->sc_version >= 0x13)) {
			if ((status & (STS_LID_CLOSED | STS_POWER_BTN_DOWN) )
			    == 0) {
			    	st = status | STS_POWER_BTN_DOWN;
			}
		}
		if ((st != -1) && (st != status)) {
			if ((status ^ st) & STS_LID_CLOSED) {
				sysmon_pswitch_event(&sc->sc_sm_lid, 
				    ((st & STS_LID_CLOSED) ?
				     PSWITCH_EVENT_PRESSED :
				     PSWITCH_EVENT_RELEASED));
			}
			if ((status ^ st) & STS_AC_AVAILABLE) {
				sysmon_pswitch_event(&sc->sc_sm_acpower, 
				    ((st & STS_AC_AVAILABLE) ?
				     PSWITCH_EVENT_PRESSED :
				     PSWITCH_EVENT_RELEASED));
			}
			if ((status ^ st) & STS_POWER_BTN_DOWN) {
				sysmon_pswitch_event(&sc->sc_sm_powerbutton, 
				    ((st & STS_POWER_BTN_DOWN) ?
				     PSWITCH_EVENT_PRESSED :
				     PSWITCH_EVENT_RELEASED));
			}
			status = st;
		}
		sc->sc_flags = status;
		if (cnt >= 4) {
			nctrl = sc->sc_control & ~(STC_TRICKLE | STC_CHARGE_ENABLE);
			bl = stvii_battery_level(sc);
			sc->sc_bat_level = bl;
			if (charging && (bl > BAT_FULL)) {
				/* stop charging, we're full */
				charging = 0;
			} else if (!charging && (bl < BAT_LOW)) {
				charging = 1;
			}
			if (st & STS_AC_AVAILABLE) {
				if (charging) {
					nctrl |= STC_CHARGE_ENABLE;
				} else
					nctrl |= STC_TRICKLE;
			}
			if (nctrl != sc->sc_control) {
				sc->sc_control = nctrl;
				stvii_writereg(sc, ST7_CONTROL, sc->sc_control);
			}
			cnt = 0;				
		} else
			cnt++;
		tsleep(&sc->sc_sleep, 0, "stvii", hz / 2);
	}
}

#define INITDATA(index, unit, string)					\
	sc->sc_sensor[index].units = unit;     				\
	sc->sc_sensor[index].state = ENVSYS_SINVALID;			\
	snprintf(sc->sc_sensor[index].desc,				\
	    sizeof(sc->sc_sensor[index].desc), "%s", string);

static void
stvii_setup_envsys(struct stvii_softc *sc)
{
	int i;

	sc->sc_sme = sysmon_envsys_create();

	INITDATA(BAT_AC_PRESENT, ENVSYS_INDICATOR, "AC present");
	INITDATA(BAT_BATTERY_PRESENT, ENVSYS_INDICATOR, "Battery present");
	INITDATA(BAT_CHARGING, ENVSYS_BATTERY_CHARGE, "Battery charging");
	INITDATA(BAT_CHARGE, ENVSYS_SVOLTS_DC, "Battery voltage");
	INITDATA(BAT_MAX_CHARGE, ENVSYS_SVOLTS_DC, "Battery design cap");
#undef INITDATA

	for (i = 0; i < BAT_NSENSORS; i++) {
		if (sysmon_envsys_sensor_attach(sc->sc_sme,
						&sc->sc_sensor[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			return;
		}
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = stvii_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev,
		    "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
stvii_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct stvii_softc *sc = sme->sme_cookie;
	int which = edata->sensor;

	edata->state = ENVSYS_SINVALID;
	switch (which) {
	case BAT_AC_PRESENT:
		edata->value_cur = (sc->sc_flags & STS_AC_AVAILABLE);
		edata->state = ENVSYS_SVALID;
		break;
	case BAT_BATTERY_PRESENT:
		edata->value_cur = (sc->sc_flags & STS_BATTERY_PRESENT);
		edata->state = ENVSYS_SVALID;
		break;
	case BAT_CHARGE:
		if (sc->sc_flags & STS_BATTERY_PRESENT) {
			edata->value_cur = sc->sc_bat_level;
			edata->state = ENVSYS_SVALID;
		}
		break;
	case BAT_MAX_CHARGE:
		if (sc->sc_flags & STS_BATTERY_PRESENT) {
			edata->value_cur = 8000000;
			/*edata->state = ENVSYS_SVALID;*/
		}
		break;
	case BAT_CHARGING:
		edata->value_cur = sc->sc_control & STC_CHARGE_ENABLE;
		edata->state = ENVSYS_SVALID;
		break;
	}
}

void
stvii_poweroff(void)
{
	struct stvii_softc *sc = device_private(stvii_dev);
	int ctl;

	if (sc == NULL)
		return;
	ctl = stvii_readreg(sc, ST7_CONTROL);
	if (ctl == -1)
		return;
	stvii_writereg(sc, ST7_CONTROL, ctl & ~(STC_MAIN_POWER | STC_DDR_POWER));
}
