/*	$NetBSD: envctrl.c,v 1.2.4.1 2007/05/22 17:27:32 matt Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tobias Nygren.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * SUNW,envctrl Sun Ultra Enterprise 450 environmental monitoring driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: envctrl.c,v 1.2.4.1 2007/05/22 17:27:32 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/envsys.h>

#include <machine/autoconf.h>
#include <machine/promlib.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>
#include <dev/i2c/i2cvar.h>
#include <sparc64/dev/envctrlreg.h>
#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/pcf8583reg.h>	/* for WDT */
#include <dev/ic/pcf8584var.h>

static int envctrlmatch(struct device *, struct cfdata *, void *);
static void envctrlattach(struct device *, struct device *, void *);

struct envctrl_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct pcf8584_handle sc_pcfiic;

	struct proc *sc_proc;
	kcondvar_t sc_sleepcond;
	kmutex_t sc_sleepmtx;

	struct sysmon_envsys sc_sysmon;
	struct envsys_basic_info sc_binfo[13];
	struct envsys_tre_data sc_sensor[13];
	uint8_t sc_keyswitch;
	uint8_t sc_fanstate;
	uint8_t sc_ps_state[3];
	int sc_ps_temp_factors[256];
	int sc_cpu_temp_factors[256];
	uint8_t sc_ps_fan_speeds[112];
	uint8_t sc_cpu_fan_speeds[112];
};

CFATTACH_DECL(envctrl, sizeof(struct envctrl_softc),
    envctrlmatch, envctrlattach, NULL, NULL);

static void deferred_envctrl_thread(void *);
static void envctrl_thread(void *);
static int envctrl_gtredata(struct sysmon_envsys *, struct envsys_tre_data *);
static int envctrl_streinfo(struct sysmon_envsys *, struct envsys_basic_info *);
static void envctrl_sleep(struct envctrl_softc *, int);
static int envctrl_write_1(struct envctrl_softc *, int, uint8_t);
static int envctrl_write_2(struct envctrl_softc *, int, uint8_t, uint8_t);
static int envctrl_read(struct envctrl_softc *, int, uint8_t *, int);
static int envctrl_get_cputemp(struct envctrl_softc *, uint8_t);
static int envctrl_get_pstemp(struct envctrl_softc *, uint8_t);
static int envctrl_get_ambtemp(struct envctrl_softc *);
static int envctrl_set_fanvoltage(struct envctrl_softc *, uint8_t, uint8_t);
static uint8_t envctrl_cputemp_to_voltage(struct envctrl_softc *, int);
static uint8_t envctrl_pstemp_to_voltage(struct envctrl_softc *, int);
static void envctrl_init_components(struct envctrl_softc *);
static int envctrl_init_tables(struct envctrl_softc *, int);
static void envctrl_update_sensors(struct envctrl_softc *);
static void envctrl_interpolate_ob_table(int *, uint8_t *);

static const struct envsys_range envctrl_ranges[] = {
	{  0,  7,	ENVSYS_STEMP},
	{  8,  9,	ENVSYS_SVOLTS_DC},
	{ 10, 11,	ENVSYS_INTEGER}, 
};

static int
envctrlmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ebus_attach_args *ea = aux;

	return (strcmp("SUNW,envctrl", ea->ea_name) == 0);
}

static void
envctrlattach(struct device *parent, struct device *self, void *aux)
{
	struct envctrl_softc *sc = (struct envctrl_softc *)self;
	struct ebus_attach_args *ea = aux;
	bus_addr_t devaddr;
	int i;

	sc->sc_iot = ea->ea_bustag;
	devaddr = EBUS_ADDR_FROM_REG(&ea->ea_reg[0]);
	if (bus_space_map(sc->sc_iot, devaddr, ea->ea_reg[0].size,
		0, &sc->sc_ioh) != 0) {
		aprint_error(": unable to map device registers\n");
		return;
	}
	if (envctrl_init_tables(sc, ea->ea_node) != 0) {
		aprint_error(": unable to initialize tables\n");
		return;
	}
	/*
	 * initialize envctrl bus
	 */
	sc->sc_pcfiic.ha_parent = self;
	sc->sc_pcfiic.ha_iot = sc->sc_iot;
	sc->sc_pcfiic.ha_ioh = sc->sc_ioh;
	pcf8584_init(&sc->sc_pcfiic);

	if (envctrl_write_1(sc, ENVCTRL_FANFAIL_ADDR, 0xFF)) {
		aprint_error(": i2c probe failed\n");
		return;
	}
	aprint_normal("\n%s: Ultra Enterprise 450 environmental monitoring\n",
	    sc->sc_dev.dv_xname);

	envctrl_init_components(sc);

	/*
	 * fill envsys sensor structures
	 */
	for (i = 0; i < 12; i++) {
		sc->sc_binfo[i].sensor = i;
		sc->sc_sensor[i].sensor = i;
		sc->sc_binfo[i].validflags = ENVSYS_FVALID;
		sc->sc_sensor[i].warnflags = ENVSYS_WARN_OK;
		sc->sc_sensor[i].validflags = ENVSYS_FVALID;
	}

	for (i = 0; i < 8; i++)
		sc->sc_binfo[i].units = sc->sc_sensor[i].units = ENVSYS_STEMP;

	for (i = 0; i < 4; i++)
		sprintf(sc->sc_binfo[i].desc, "CPU%i", i);

	for (i = 4; i < 7; i++)
		sprintf(sc->sc_binfo[i].desc, "PS%i", i - 4);

	for (i = 8; i < 10; i++)
		sc->sc_binfo[i].units = sc->sc_sensor[i].units =
		    ENVSYS_SVOLTS_DC;

	for (i = 10; i < 12; i++)
		sc->sc_binfo[i].units = sc->sc_sensor[i].units =
		    ENVSYS_INTEGER;

	sprintf(sc->sc_binfo[7].desc, "ambient");
	sprintf(sc->sc_binfo[8].desc, "cpufan voltage");
	sprintf(sc->sc_binfo[9].desc, "psfan voltage");
	sprintf(sc->sc_binfo[10].desc, "ps failed");
	sprintf(sc->sc_binfo[11].desc, "fans failed");

	sc->sc_sysmon.sme_ranges = envctrl_ranges;
	sc->sc_sysmon.sme_sensor_info = sc->sc_binfo;
	sc->sc_sysmon.sme_sensor_data = sc->sc_sensor;
	sc->sc_sysmon.sme_cookie = sc;
	sc->sc_sysmon.sme_gtredata = envctrl_gtredata;
	sc->sc_sysmon.sme_streinfo = envctrl_streinfo;
	sc->sc_sysmon.sme_nsensors = 12;
	sc->sc_sysmon.sme_envsys_version = 1000;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);

	envctrl_update_sensors(sc);

	for (i = 0; i < 3; i++) {
		aprint_normal("%s: PS %i: ", sc->sc_dev.dv_xname, i);
		if (sc->sc_ps_state[i] & ENVCTRL_PS_PRESENT) {
			aprint_normal("absent\n");
			continue;
		}
		aprint_normal("%iW, %s",
		    (sc->sc_ps_state[i] & ENVCTRL_PS_550W) ? 650 : 550,
		    (sc->sc_ps_state[i] & ENVCTRL_PS_OK) ?
		    "online" : "FAILED");
		if ((sc->sc_ps_state[i] & ENVCTRL_PS_OVERLOAD) == 0)
			aprint_normal(" ***OVERLOADED***");
		if ((sc->sc_ps_state[i] & ENVCTRL_PS_LOADSHARE_ERROR) == 0)
			aprint_normal(" ***LOADSHARE ERROR***");
		aprint_normal("\n");
	}

	aprint_verbose("%s: keyswitch is ", sc->sc_dev.dv_xname);
	if ((sc->sc_keyswitch & ENVCTRL_KEY_LOCK) == 0)
		aprint_verbose("unlocked\n");
	else {
		if ((sc->sc_keyswitch & ENVCTRL_KEY_DIAG) == 0)
			aprint_verbose("in diagnostic mode\n");
		else
			aprint_verbose("locked\n");
	}

	mutex_init(&sc->sc_sleepmtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_sleepcond, "envidle");
	kthread_create(deferred_envctrl_thread, sc);

}

static void
deferred_envctrl_thread(void *arg)
{
	int error;
	struct envctrl_softc *sc = arg;

	error = kthread_create1(envctrl_thread, arg,
	    &sc->sc_proc, "envctrl");
	if (error)
		panic("cannot start envctrl thread; error %d", error);
}

static int
envctrl_gtredata(struct sysmon_envsys *sme, struct envsys_tre_data *tred)
{
	struct envctrl_softc *sc = sme->sme_cookie;

	*tred = sc->sc_sensor[tred->sensor];

	return 0;
}

static int
envctrl_streinfo(struct sysmon_envsys *sme, struct envsys_basic_info *binfo)
{
	struct envctrl_softc *sc = sme->sme_cookie;

	memcpy(sc->sc_binfo[binfo->sensor].desc, binfo->desc,
	    sizeof(sc->sc_binfo[binfo->sensor].desc));
	sc->sc_binfo[binfo->sensor].desc[
	    sizeof(sc->sc_binfo[binfo->sensor].desc) - 1] = '\0';

	binfo->validflags = ENVSYS_FVALID;

	return 0;
}

static void
envctrl_sleep(struct envctrl_softc *sc, int ms)
{

	cv_timedwait(&sc->sc_sleepcond, &sc->sc_sleepmtx, mstohz(ms));
}

/*
 * read cpu temperature, in microCelcius
 */
static int
envctrl_get_cputemp(struct envctrl_softc *sc, uint8_t cpu)
{
	uint8_t r;

	if (envctrl_write_1(sc, ENVCTRL_CPUTEMP_ADDR, (1 << 6) | cpu))
		return -1;
	if (envctrl_read(sc, ENVCTRL_CPUTEMP_ADDR, &r, 1))
		return -1;
	if (envctrl_write_1(sc, ENVCTRL_CPUTEMP_ADDR, 0))
		return -1;

	return sc->sc_cpu_temp_factors[r];
}

/*
 * read power supply temperature in microCelcius
 */
static int
envctrl_get_pstemp(struct envctrl_softc *sc, uint8_t ps)
{
	uint8_t r;

	if (envctrl_write_1(sc, ENVCTRL_PS0TEMP_ADDR + ps, (1 << 6)))
		return -1;
	if (envctrl_read(sc, ENVCTRL_PS0TEMP_ADDR + ps, &r, 1))
		return -1;
	if (envctrl_write_1(sc, ENVCTRL_PS0TEMP_ADDR + ps, 0))
		return -1;

	return sc->sc_ps_temp_factors[r];
}

/*
 * read ambient temperature in microCelcius
 */
static int
envctrl_get_ambtemp(struct envctrl_softc *sc)
{
	int8_t temp[2];

	if (envctrl_write_1(sc, ENVCTRL_AMB_ADDR, 0))
		return -1;
	if (envctrl_read(sc, ENVCTRL_AMB_ADDR, temp, 2))
		return -1;

	return ((temp[0] << 1) | ((temp[1] >> 7) & 1)) * 500000;
}

/*
 * set fan voltage, 0 - 63, scaled to 0V-12V.
 */
static int
envctrl_set_fanvoltage(struct envctrl_softc *sc, uint8_t port, uint8_t value)
{

	return envctrl_write_2(sc, ENVCTRL_FANVOLTAGE_ADDR, port, value);
}

static uint8_t
envctrl_cputemp_to_voltage(struct envctrl_softc *sc, int temp)
{
	uint8_t ret;

	temp -= 20;		/* magic offset, from opensolaris */

	if (temp < 0)
		temp = 0;
	if (temp > 111)
		temp = 111;

	ret = sc->sc_cpu_fan_speeds[temp];
	if (ret < ENVCTRL_FANVOLTAGE_MIN)
		return ENVCTRL_FANVOLTAGE_MIN;
	if (ret > ENVCTRL_FANVOLTAGE_MAX)
		return ENVCTRL_FANVOLTAGE_MAX;
	return ret;
}

static uint8_t
envctrl_pstemp_to_voltage(struct envctrl_softc *sc, int temp)
{
	uint8_t ret;

	temp -= 30;		/* magic offset, from opensolaris */

	if (temp < 0)
		temp = 0;

	if (temp > 111)
		temp = 111;

	ret = sc->sc_ps_fan_speeds[temp];
	if (ret < ENVCTRL_FANVOLTAGE_MIN)
		return ENVCTRL_FANVOLTAGE_MIN;
	if (ret > ENVCTRL_FANVOLTAGE_MAX)
		return ENVCTRL_FANVOLTAGE_MAX;
	return ret;
}

static void
envctrl_init_components(struct envctrl_softc *sc)
{

	/* configure ports as pure inputs */
	envctrl_write_1(sc, ENVCTRL_FANFAIL_ADDR, ~0);
	envctrl_write_1(sc, ENVCTRL_PS0_ADDR, ~0);
	envctrl_write_1(sc, ENVCTRL_PS1_ADDR, ~0);
	envctrl_write_1(sc, ENVCTRL_PS2_ADDR, ~0);

	/* light up power LED */
	envctrl_write_1(sc, ENVCTRL_LED_ADDR, ~ENVCTRL_LED_PWR);

	/*
	 * Set fans to full speed. The last two ports are usually not
	 * connected, but it doesn't hurt to enable them.
	 */
	envctrl_set_fanvoltage(sc, ENVCTRL_FANPORT_CPU, ENVCTRL_FANVOLTAGE_MAX);
	envctrl_set_fanvoltage(sc, ENVCTRL_FANPORT_PS, ENVCTRL_FANVOLTAGE_MAX);
	envctrl_set_fanvoltage(sc, ENVCTRL_FANPORT_AFB, ENVCTRL_FANVOLTAGE_MAX);
	envctrl_set_fanvoltage(sc, 3, ENVCTRL_FANVOLTAGE_MAX);

	/* set fan watchdog timer to a 60 sec timeout */
	envctrl_write_2(sc, ENVCTRL_WATCHDOG_ADDR, PCF8583_REG_CSR, 0x80);
	envctrl_write_2(sc, ENVCTRL_WATCHDOG_ADDR, PCF8583_REG_CSR, 0x80);
	envctrl_write_2(sc, ENVCTRL_WATCHDOG_ADDR, PCF8583_REG_TIMER, 0);
	envctrl_write_2(sc, ENVCTRL_WATCHDOG_ADDR, PCF8583_REG_ALMTIMER, 60);
	envctrl_write_2(sc, ENVCTRL_WATCHDOG_ADDR, PCF8583_REG_ALMCTL, 0xca);
	envctrl_write_2(sc, ENVCTRL_WATCHDOG_ADDR, PCF8583_REG_CSR, 0x04);

	/* recover from previous watchdog failure, if any */
	envctrl_write_1(sc, ENVCTRL_INTR_ADDR, ~0);
	envctrl_write_1(sc, ENVCTRL_INTR_ADDR, ~ENVCTRL_INTR_WDT_RST);
	envctrl_write_1(sc, ENVCTRL_INTR_ADDR, (uint8_t) ~ENVCTRL_INTR_ENABLE);
}

#if 0
/*
 * Service routine for environmental monitoring events. Currently unused.
 */
static void
envctrl_isr(void)
{
	uint8_t intstate;
	uint8_t v;

	envctrl_read(ENVCTRL_INTR_ADDR, 1, &intstate);
	if ((intstate & ENVCTRL_INTR_PS0) == 0 ||
	    (intstate & ENVCTRL_INTR_PS1) == 0 ||
	    (intstate & ENVCTRL_INTR_PS2) == 0) {
		printf("envctrl: power supply event\n");
		envctrl_read(ENVCTRL_PS0_ADDR, 1, &v);
		envctrl_read(ENVCTRL_PS1_ADDR, 1, &v);
		envctrl_read(ENVCTRL_PS2_ADDR, 1, &v);
	}
	if ((intstate & ENVCTRL_INTR_FANFAIL) == 0) {
		printf("envctrl: fan failure event\n");
		envctrl_read(ENVCTRL_FANFAIL_ADDR, 1, &v);
	}
	if ((intstate & ENVCTRL_INTR_UNKNOWN1) == 0) {
	}
	if ((intstate & ENVCTRL_INTR_UNKNOWN2) == 0) {
	}
}
#endif

/*
 * Refresh all sensor readings
 */
static void
envctrl_update_sensors(struct envctrl_softc *sc)
{
	int cputemp_max;
	int pstemp_max;
	uint8_t cpufan_voltage;
	uint8_t psfan_voltage;
	int temp;
	int nfail;
	int i;

	/* read cpu temperatures */
	cputemp_max = -1;
	for (i = 0; i < 4; i++) {
		temp = envctrl_get_cputemp(sc, i);
		if (temp != -1) {
			if (cputemp_max < temp)
				cputemp_max = temp;
			sc->sc_sensor[i].cur.data_us = temp + 273150000;
			sc->sc_sensor[i].validflags |= ENVSYS_FCURVALID;
		} else
			sc->sc_sensor[i].validflags &= ~ENVSYS_FCURVALID;
	}

	/* read power supply state & temperature */
	pstemp_max = -1;
	nfail = 0;
	for (i = 0; i < 3; i++) {
		if (envctrl_read(sc, ENVCTRL_PS0_ADDR - i,
			&sc->sc_ps_state[i], 1))
			sc->sc_ps_state[i] = ENVCTRL_PS_PRESENT;

		if ((sc->sc_ps_state[i] & ENVCTRL_PS_PRESENT) == 0) {
			if ((sc->sc_ps_state[i] & 0x38) != 0x38)
				nfail++;
			temp = envctrl_get_pstemp(sc, i);
			if (pstemp_max < temp)
				pstemp_max = temp;
			sc->sc_sensor[i + 4].cur.data_us = temp + 273150000;
			sc->sc_sensor[i + 4].validflags |= ENVSYS_FCURVALID;
		} else
			sc->sc_sensor[i + 4].validflags &= ~ENVSYS_FCURVALID;
	}
	sc->sc_sensor[10].cur.data_s = nfail;
	sc->sc_sensor[10].validflags |= ENVSYS_FCURVALID;

	/* read ambient temperature */
	temp = envctrl_get_ambtemp(sc);
	if (temp != -1) {
		sc->sc_sensor[7].cur.data_us = temp + 273150000;
		sc->sc_sensor[7].validflags |= ENVSYS_FCURVALID;
	} else
		sc->sc_sensor[7].validflags &= ~ENVSYS_FCURVALID;

	/* read fan state */
	if (envctrl_read(sc, ENVCTRL_FANFAIL_ADDR, &sc->sc_fanstate, 1)) {
		sc->sc_fanstate = 0;
		sc->sc_sensor[11].validflags &= ~ENVSYS_FCURVALID;
	} else {
		nfail = 0;
		for (i = 0; i < 8; i++) {
			if (((sc->sc_fanstate >> i) & 1) == 0)
				nfail++;
		}
		sc->sc_sensor[11].cur.data_s = nfail;
		sc->sc_sensor[11].validflags |= ENVSYS_FCURVALID;
	}

	/* read keyswitch */
	envctrl_read(sc, ENVCTRL_LED_ADDR, &sc->sc_keyswitch, 1);

	/*
	 * Update fan voltages. If any fans have failed, set voltage
	 * to max to compensate for lost air flow.
	 */
	cpufan_voltage = envctrl_cputemp_to_voltage(sc, cputemp_max / 1000000);
	if (cputemp_max == -1 || sc->sc_fanstate != 0xFF)
		cpufan_voltage = ENVCTRL_FANVOLTAGE_MAX;

	psfan_voltage = envctrl_pstemp_to_voltage(sc, pstemp_max / 1000000);
	if (pstemp_max == -1 || sc->sc_fanstate != 0xFF)
		psfan_voltage = ENVCTRL_FANVOLTAGE_MAX;

	envctrl_set_fanvoltage(sc, ENVCTRL_FANPORT_CPU, cpufan_voltage);
	envctrl_set_fanvoltage(sc, ENVCTRL_FANPORT_PS, psfan_voltage);

	sc->sc_sensor[8].cur.data_us = cpufan_voltage * ENVCTRL_UVFACT;
	sc->sc_sensor[9].cur.data_us = psfan_voltage * ENVCTRL_UVFACT;
	sc->sc_sensor[8].validflags |= ENVSYS_FCURVALID;
	sc->sc_sensor[9].validflags |= ENVSYS_FCURVALID;
}

/*
 * envctrl worker thread. Reads sensors periodically.
 */
static void
envctrl_thread(void *arg)
{

#ifdef BLINK
	int i;
#endif
	struct envctrl_softc *sc = arg;

	mutex_enter(&sc->sc_sleepmtx);

	/*
	 * Poll sensors every 15 seconds. Optionally blink the activity LED.
	 */
	for (;;) {
		/* kick wdt */
		envctrl_write_2(sc, ENVCTRL_WATCHDOG_ADDR,
		    PCF8583_REG_TIMER, 0);

		/* refresh sensor readings, update fan speeds */
		envctrl_update_sensors(sc);

#ifdef BLINK
		for (i = 0; i < 15; i++) {
			envctrl_write_1(sc, ENVCTRL_LED_ADDR,
			    ~(ENVCTRL_LED_PWR | ENVCTRL_LED_ACT));
			envctrl_sleep(sc, 500);
			envctrl_write_1(sc, ENVCTRL_LED_ADDR,
			    ~ENVCTRL_LED_PWR);
			envctrl_sleep(sc, 500);
		}
#else
		envctrl_sleep(sc, 15 * 1000);
#endif
	}
}

/*
 * Make a table with interpolated values based on an OPB temperature table.
 */
static void
envctrl_interpolate_ob_table(int *ftbl, uint8_t *utbl)
{
	int i;
	int e;
	int k;
	int y;

	i = 255;
	e = i;
	while (e >= 0) {
		e = i;
		y = utbl[i] * 1000000;
		do {
			i--;
		}
		while (utbl[e] == utbl[i] && i > 0);
		k = (utbl[i] * 1000000 - y) / (e - i);
		while (e >= i) {
			ftbl[e] = y;
			y += k;
			e--;
		}
	}
}

/*
 * Copy temperature tables from OBP. Return 0 on success.
 */
static int
envctrl_init_tables(struct envctrl_softc *sc, int node)
{
	uint8_t buf[256];
	uint8_t *p;
	int nitem;
	int err;

	p = buf;
	nitem = 1;
	err = prom_getprop(node, "cpu-temp-factors", 254, &nitem, &p);
	if (nitem != 1 || err != 0)
		return -1;

	buf[255] = buf[253];
	buf[254] = buf[253];
	envctrl_interpolate_ob_table(sc->sc_cpu_temp_factors, buf);

	p = buf;
	nitem = 1;
	err = prom_getprop(node, "ps-temp-factors", 254, &nitem, &p);
	if (nitem != 1 || err != 0)
		return -1;

	buf[255] = buf[253];
	buf[254] = buf[253];
	envctrl_interpolate_ob_table(sc->sc_ps_temp_factors, buf);

	p = sc->sc_cpu_fan_speeds;
	nitem = 1;
	err = prom_getprop(node, "cpu-fan-speeds", 112, &nitem, &p);
	if (nitem != 1 || err != 0)
		return -1;

	p = sc->sc_ps_fan_speeds;
	nitem = 1;
	err = prom_getprop(node, "ps-fan-speeds", 112, &nitem, &p);
	if (nitem != 1 || err != 0)
		return -1;

	return 0;
}

static int
envctrl_write_1(struct envctrl_softc *sc, int addr, uint8_t v1)
{

	return iic_exec(&sc->sc_pcfiic.ha_i2c, I2C_OP_WRITE_WITH_STOP, addr,
	    NULL, 0, &v1, 1, cold ? I2C_F_POLL : 0);
}


static int
envctrl_write_2(struct envctrl_softc *sc, int addr, uint8_t v1, uint8_t v2)
{

	uint8_t buf[] = {v1, v2};
	return iic_exec(&sc->sc_pcfiic.ha_i2c, I2C_OP_WRITE_WITH_STOP, addr,
	    NULL, 0, buf, 2, cold ? I2C_F_POLL : 0);
}

static int
envctrl_read(struct envctrl_softc *sc, int addr, uint8_t *buf, int len)
{

	return iic_exec(&sc->sc_pcfiic.ha_i2c, I2C_OP_READ_WITH_STOP, addr,
	    NULL, 0, buf, len,  cold ? I2C_F_POLL : 0);
}
