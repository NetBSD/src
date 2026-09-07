/*	$NetBSD: envmon_ebus.c,v 1.1 2026/09/07 07:35:13 jdc Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
__KERNEL_RCSID(0, "$NetBSD: envmon_ebus.c,v 1.1 2026/09/07 07:35:13 jdc Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/types.h>

#include <sys/bus.h>
#include <machine/autoconf.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

#include <dev/led.h>
#include <dev/sysmon/sysmonvar.h>

#include <arch/sparc64/dev/envmon_ebusreg.h>

#define ENVMON_MACHINE_U25	0x19
#define ENVMON_MACHINE_U45	0x2d
#define ENVMON_MACHINE_V215	0xd7
#define ENVMON_MACHINE_V245	0xf5

#define ENVMON_PIC_MAX_TEMPS	1
#define ENVMON_PIC_WMIN_TEMP		273150000	/* 0C from Solaris */
#define ENVMON_PIC_WMAX_TEMP		318150000	/* 45C from Solaris */
#define ENVMON_PIC_CRIT_TEMP		338150000	/* 65C from Solaris */

struct envmon_epic_fans {
	const uint8_t	reg;
	const char	*desc;
};
static struct envmon_epic_fans envmon_epic_fans_table[] = {
	{ ENVMON_EPC_FAN_FT0_F0, "FT0.F0" },
	{ ENVMON_EPC_FAN_FT1_F0, "FT1.F0" },
	{ ENVMON_EPC_FAN_FT2_F0, "FT2.F0" },
	{ ENVMON_EPC_FAN_FT3_F0, "FT3.F0" },
	{ ENVMON_EPC_FAN_FT4_F0, "FT4.F0" },
	{ ENVMON_EPC_FAN_FT5_F0, "FT5.F0" },
	{ ENVMON_EPC_FAN_FT0_F1, "FT0.F1" },
	{ ENVMON_EPC_FAN_FT1_F1, "FT1.F1" },
	{ ENVMON_EPC_FAN_FT2_F1, "FT2.F1" },
	{ ENVMON_EPC_FAN_FT3_F1, "FT3.F1" },
	{ ENVMON_EPC_FAN_FT4_F1, "FT4.F1" },
	{ ENVMON_EPC_FAN_FT5_F1, "FT5.F1" },
};
#define ENVMON_EPC_MAX_FANS	(sizeof(envmon_epic_fans_table) / \
				    sizeof(envmon_epic_fans_table[0]))
#define ENVMON_EPC_DIV_V245		5625
#define ENVMON_EPC_DIV_V215		11250
#define ENVMON_EPC_CRIT_SPEED_V245	2022		/* From ALOM */
#define ENVMON_EPC_CRIT_SPEED_V215	5012		/* From ALOM */
#define ENVMON_EPC_WMIN_TEMP		273150000	/* 0C from ALOM */
#define ENVMON_EPC_WMAX_TEMP		318150000	/* 45C from ALOM */
#define ENVMON_EPC_CRIT_TEMP		323150000	/* 50C from ALOM */
#define ENVMON_EPC_VAL_TO_SPEED(div, val)	((div * 60) / val)

#define ENVMON_EPC_MAX_FAULTS	1

#define ENVMON_EPC_MAX_TEMPS	1

struct envmon_epic_ind {
	uint8_t		reg, set;
	const char	*desc;
};
static struct envmon_epic_ind envmon_epic_inds_table[] = {
	{ ENVMON_EPC_KEYSW, ENVMON_EPC_KEYSW_NORM, "keyswitch normal/diag" },
	{ ENVMON_EPC_KEYSW, ENVMON_EPC_KEYSW_LOCK, "keyswitch locked" },
};

#define ENVMON_EPC_MAX_INDS	(sizeof(envmon_epic_inds_table) / \
				    sizeof(envmon_epic_inds_table[0]))

struct envmon_epic_led {
	uint8_t		reg, v_on;
	const char	*desc;
};
static struct envmon_epic_led envmon_epic_led_table[] = {
	{ ENVMON_EPC_LED6, ENVMON_EPC_LED6_LOC, "locator" },
	{ ENVMON_EPC_LED6, ENVMON_EPC_LED6_SRV, "service" },
	{ ENVMON_EPC_LED6, ENVMON_EPC_LED6_SRV_FSH, "service_flash" },
	{ ENVMON_EPC_LED6, ENVMON_EPC_LED6_PWR, "power" },
	{ ENVMON_EPC_LED6, ENVMON_EPC_LED6_TF_FLT, "top_fan_fault" },
	{ ENVMON_EPC_LED6, ENVMON_EPC_LED6_TF_FLT_FSH, "top_fan_fault_flash" },
	{ ENVMON_EPC_LED7, ENVMON_EPC_LED7_PSU, "psu_fault" },
	{ ENVMON_EPC_LED7, ENVMON_EPC_LED7_PSU_FSH, "psu_fault_flash" },
	{ ENVMON_EPC_LED7, ENVMON_EPC_LED7_CTEMP, "cpu_overtemp" },
	{ ENVMON_EPC_LED7, ENVMON_EPC_LED7_CTEMP_FSH, "cpu_overtemp_flash" },
};
#define ENVMON_EPC_MAX_LEDS	(sizeof(envmon_epic_led_table) / \
				    sizeof(envmon_epic_led_table[0]))

#define ENVMON_EPC_MAX_SENSORS	\
    (ENVMON_EPC_MAX_FANS + ENVMON_EPC_MAX_FAULTS + \
    ENVMON_EPC_MAX_TEMPS + ENVMON_EPC_MAX_INDS)

#define ENVMON_MAX_TEMPS \
     MAX(ENVMON_PIC_MAX_TEMPS, ENVMON_EPC_MAX_TEMPS)
#define ENVMON_MAX_SENSORS \
    MAX(ENVMON_PIC_MAX_TEMPS, ENVMON_EPC_MAX_SENSORS)

struct envmon_sc_fan {
	uint8_t		reg;
	int		cmin;
};

struct envmon_sc_temp {
	uint8_t		reg;
	int		wmin, wmax, cmax;
};

struct envmon_sc_ind {
	uint8_t		reg, set;
};
struct envmon_sc_led {
	void			*cookie;
	struct led_device	*led;
	uint8_t			reg, v_on;
};

struct envmon_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_bst;	
	bus_space_handle_t	sc_bsh;

	int			sc_machine_type, sc_nfans;
	struct envmon_sc_fan	sc_fan[ENVMON_EPC_MAX_FANS];
	struct envmon_sc_temp	sc_temp[ENVMON_MAX_TEMPS];
	struct envmon_sc_ind	sc_ind[ENVMON_EPC_MAX_INDS];
	struct envmon_sc_led	sc_led[ENVMON_EPC_MAX_LEDS];
	struct sysmon_envsys	*sc_sme;
	envsys_data_t		sc_sensor[ENVMON_MAX_SENSORS];

#ifdef ENVMON_DEBUG
	#define ENVMON_MAX_REG	0x3d
	uint8_t			sc_reg[ENVMON_MAX_REG];
	#define ENVMON_TIMER	60
	callout_t		sc_timer;
#endif
};

static int envmon_ebus_match(device_t, cfdata_t, void *);
static void envmon_ebus_attach(device_t, device_t, void *);
static void envmon_pic_attach(struct envmon_softc *, int);
static void envmon_epic_attach(struct envmon_softc *, int);
void envmon_pic_refresh(struct sysmon_envsys *, envsys_data_t *);
void envmon_epic_refresh(struct sysmon_envsys *, envsys_data_t *);
void envmon_epic_refresh_fan(struct envmon_softc *, envsys_data_t *);
void envmon_epic_refresh_fault(struct envmon_softc *, envsys_data_t *);
void envmon_refresh_temp(struct envmon_softc *, envsys_data_t *);
void envmon_epic_refresh_ind(struct envmon_softc *, envsys_data_t *);
void envmon_pic_get_limits(struct sysmon_envsys *, envsys_data_t *,
    sysmon_envsys_lim_t *, uint32_t *);
void envmon_pic_set_limits(struct sysmon_envsys *, envsys_data_t *,
    sysmon_envsys_lim_t *, uint32_t *);
void envmon_epic_get_limits(struct sysmon_envsys *, envsys_data_t *,
    sysmon_envsys_lim_t *, uint32_t *);
void envmon_epic_set_limits(struct sysmon_envsys *, envsys_data_t *,
    sysmon_envsys_lim_t *, uint32_t *);
static void envmon_epic_get_fan_limits(struct envmon_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void envmon_get_temp_limits(struct envmon_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void envmon_epic_set_fan_limits(struct envmon_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
static void envmon_set_temp_limits(struct envmon_softc *,
    envsys_data_t *, sysmon_envsys_lim_t *, uint32_t *);
int envmon_epic_get_led(void *);
void envmon_epic_set_led(void *, int);
#ifdef ENVMON_DEBUG
static void envmon_timeout(void *);
#endif

CFATTACH_DECL_NEW(envmon_ebus, sizeof(struct envmon_softc),
    envmon_ebus_match, envmon_ebus_attach, NULL, NULL);

#define	envmon_read(sc, reg) \
    bus_space_read_1(sc->sc_bst, sc->sc_bsh, reg)
#define envmon_write(sc, reg, val) \
    bus_space_write_1(sc->sc_bst, sc->sc_bsh, reg, val)

static int
envmon_ebus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *ea = aux;
	char *compat;

	if (strcmp("env-monitor", ea->ea_name) != 0)
		return 0;

	compat = prom_getpropstring(ea->ea_node, "compatible");
	if (compat != NULL && (!strcmp(compat, "SUNW,ebus-pic16f747-env") ||
	    !strcmp(compat, "epic")))
		return 1;

	return 0;
}

static void
envmon_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct envmon_softc *sc = device_private(self);
	struct ebus_attach_args *ea = aux;
	char compat[32];
	int sz;
#ifdef ENVMON_DEBUG
	uint8_t	reg;
#endif

	sc->sc_dev = self;
	sc->sc_bst = ea->ea_bustag;

	sz = ea->ea_reg[0].size;

	if (bus_space_map(sc->sc_bst,
			 EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
			 sz, 0,
			 &sc->sc_bsh) != 0) {
		aprint_error(": can't map register\n");
		return;
	}

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;

	OF_getprop(ea->ea_node, "compatible", compat, sizeof(compat));

	if (!strcmp(compat, "SUNW,ebus-pic16f747-env")) {
		envmon_pic_attach(sc, ea->ea_node);
	}
	if (!strcmp(compat, "epic")) {
		envmon_epic_attach(sc, ea->ea_node);
	}

#ifdef ENVMON_DEBUG
	for (reg = 0; reg < ENVMON_MAX_REG; reg++) {
		envmon_write(sc, ENVMON_ADDR, reg);
		sc->sc_reg[reg] = envmon_read(sc, ENVMON_DATA);
	}
	callout_init(&sc->sc_timer, CALLOUT_MPSAFE);
	callout_reset(&sc->sc_timer, hz * ENVMON_TIMER, envmon_timeout, sc);
#endif
}

/*
 * We only add the front panel temperature here.
 * Other U45 sensors are handled by the ADT7462 driver.
 */
static void
envmon_pic_attach(struct envmon_softc *sc, int node)
{
	int32_t vers;

	vers = prom_getpropint(node, "version", 0);
	aprint_normal(": pic16f747 (ver. %d)\n", vers);

	if (!strcmp(machine_model, "SUNW,A70")) {
		sc->sc_machine_type = ENVMON_MACHINE_U45;
	} else if (!strcmp(machine_model, "SUNW,Ultra-25")) {
		sc->sc_machine_type = ENVMON_MACHINE_U25;
	} else {
		aprint_error_dev(sc->sc_dev, "unsupported machine model\n");
		return;
	}

	sc->sc_sme->sme_refresh = envmon_pic_refresh;
	sc->sc_sme->sme_get_limits = envmon_pic_get_limits;
	sc->sc_sme->sme_set_limits = envmon_pic_set_limits;

	strlcpy(sc->sc_sensor[0].desc, "Front_panel",
	    sizeof(sc->sc_sensor[0].desc));
	sc->sc_sensor[0].units = ENVSYS_STEMP;
	sc->sc_sensor[0].state = ENVSYS_SINVALID;
	sc->sc_sensor[0].flags = ENVSYS_FHAS_ENTROPY | ENVSYS_FMONLIMITS |
	    ENVSYS_FMONCRITICAL;
	sc->sc_temp[0].reg = ENVMON_PIC_TEMP_FP;
	sc->sc_temp[0].wmin = ENVMON_PIC_WMIN_TEMP;
	sc->sc_temp[0].wmax = ENVMON_PIC_WMAX_TEMP;
	sc->sc_temp[0].cmax = ENVMON_PIC_CRIT_TEMP;
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[0])) {
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		aprint_error_dev(sc->sc_dev,
		    "unable to attach sensor to sysmon\n");
		return;
	}

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev,
		    "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}

	return;
}

static void
envmon_epic_attach(struct envmon_softc *sc, int node)
{
	const char *vers;
	int cmin, i, j;

	vers = prom_getpropstring(node, "version");
	aprint_normal(": epic (ver. %s)\n", vers);

	/* Fans */
	if (!strcmp(machine_model, "SUNW,Sun-Fire-V245")) {
		sc->sc_machine_type = ENVMON_MACHINE_V245;
		sc->sc_nfans = 6;
		cmin = ENVMON_EPC_CRIT_SPEED_V245;
	} else if (!strcmp(machine_model, "SUNW,Sun-Fire-V215")) {
		sc->sc_machine_type = ENVMON_MACHINE_V215;
		sc->sc_nfans = 12;
		cmin = ENVMON_EPC_CRIT_SPEED_V215;
	} else {
		aprint_error_dev(sc->sc_dev, "unsupported machine model\n");
		return;
	}

	sc->sc_sme->sme_refresh = envmon_epic_refresh;
	sc->sc_sme->sme_get_limits = envmon_epic_get_limits;
	sc->sc_sme->sme_set_limits = envmon_epic_set_limits;

	for (i = 0; i < sc->sc_nfans; i++) {
		strlcpy(sc->sc_sensor[i].desc, envmon_epic_fans_table[i].desc,
		    sizeof(sc->sc_sensor[i].desc));
		sc->sc_sensor[i].units = ENVSYS_SFANRPM;
		sc->sc_sensor[i].state = ENVSYS_SINVALID;
		sc->sc_sensor[i].flags = ENVSYS_FMONLIMITS |
		    ENVSYS_FMONCRITICAL;
		sc->sc_fan[i].reg = envmon_epic_fans_table[i].reg;
		sc->sc_fan[i].cmin = cmin;
		if (sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensor[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			sc->sc_sme = NULL;
			aprint_error_dev(sc->sc_dev,
			    "unable to attach sensor to sysmon\n");
			goto leds;
		}
	}

	/* Fan fault */
	i = sc->sc_nfans;
	strlcpy(sc->sc_sensor[i].desc, "fan fault",
	    sizeof(sc->sc_sensor[i].desc));
	sc->sc_sensor[i].units = ENVSYS_INTEGER;
	sc->sc_sensor[i].state = ENVSYS_SINVALID;
	sc->sc_sensor[i].flags = ENVSYS_FMONCRITICAL;
	if (sysmon_envsys_sensor_attach(
	    sc->sc_sme, &sc->sc_sensor[i])) {
		aprint_error_dev(sc->sc_dev,
		    "unable to attach fan fault at sysmon\n");
		goto leds;
	}
	i++;

	/* Temperature */
	strlcpy(sc->sc_sensor[i].desc, "FIOB.T_AMB",
	    sizeof(sc->sc_sensor[i].desc));
	sc->sc_sensor[i].units = ENVSYS_STEMP;
	sc->sc_sensor[i].state = ENVSYS_SINVALID;
	sc->sc_sensor[i].flags = ENVSYS_FHAS_ENTROPY | ENVSYS_FMONLIMITS |
	    ENVSYS_FMONCRITICAL;
	sc->sc_temp[0].reg = ENVMON_EPC_TEMP_FP;
	sc->sc_temp[0].wmin = ENVMON_EPC_WMIN_TEMP;
	sc->sc_temp[0].wmax = ENVMON_EPC_WMAX_TEMP;
	sc->sc_temp[0].cmax = ENVMON_EPC_CRIT_TEMP;
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[i])) {
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		aprint_error_dev(sc->sc_dev,
			    "unable to attach sensor to sysmon\n");
		goto leds;
	}
	i++;

	/* Indicators */
	for (j = 0; j < ENVMON_EPC_MAX_INDS; j++, i++) {
		strlcpy(sc->sc_sensor[i].desc, envmon_epic_inds_table[j].desc,
		    sizeof(sc->sc_sensor[i].desc));
		sc->sc_sensor[i].units = ENVSYS_INDICATOR;
		sc->sc_sensor[i].state = ENVSYS_SINVALID;
		sc->sc_ind[j].reg = envmon_epic_inds_table[j].reg;
		sc->sc_ind[j].set = envmon_epic_inds_table[j].set;
		if (sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensor[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			sc->sc_sme = NULL;
			aprint_error_dev(sc->sc_dev,
			    "unable to attach sensor to sysmon\n");
			goto leds;
		}
	}
	
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(sc->sc_dev,
		    "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
	}

leds:
	/* LED's */
	for (i = 0; i < ENVMON_EPC_MAX_LEDS; i++) {
		sc->sc_led[i].cookie = sc;
		sc->sc_led[i].reg = envmon_epic_led_table[i].reg;
		sc->sc_led[i].v_on = envmon_epic_led_table[i].v_on;
		led_attach(envmon_epic_led_table[i].desc,
		    &sc->sc_led[i], envmon_epic_get_led, envmon_epic_set_led);
	}
	
	return;
}

void
envmon_pic_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct envmon_softc *sc = sme->sme_cookie;

	envmon_refresh_temp(sc, edata);
}

void
envmon_epic_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct envmon_softc *sc = sme->sme_cookie;

	if (edata->sensor < sc->sc_nfans)
		envmon_epic_refresh_fan(sc, edata);
	else if (edata->sensor < sc->sc_nfans + ENVMON_EPC_MAX_FAULTS)
		envmon_epic_refresh_fault(sc, edata);
	else if (edata->sensor <
	    sc->sc_nfans + ENVMON_EPC_MAX_FAULTS + ENVMON_EPC_MAX_TEMPS)
		envmon_refresh_temp(sc, edata);
	else
		envmon_epic_refresh_ind(sc, edata);

	return;
}

void
envmon_epic_refresh_fan(struct envmon_softc *sc, envsys_data_t *edata)
{
	uint8_t val;
	int div;
	
	if (sc->sc_machine_type == ENVMON_MACHINE_V245)
		div = ENVMON_EPC_DIV_V245;
	else
		div = ENVMON_EPC_DIV_V215;

	envmon_write(sc, ENVMON_ADDR, sc->sc_fan[edata->sensor].reg);
	val = envmon_read(sc, ENVMON_DATA);
	if (val == 0xff || val == 0x0)
		edata->value_cur = 0;
	else {
		edata->value_cur = ENVMON_EPC_VAL_TO_SPEED(div, val);
	}
	if (edata->value_cur < sc->sc_fan[edata->sensor].cmin)
		edata->state = ENVSYS_SCRITICAL;
	else
		edata->state = ENVSYS_SVALID;
}

void
envmon_epic_refresh_fault(struct envmon_softc *sc, envsys_data_t *edata)
{
	uint8_t val;
	int32_t which, total;
	int i, j;

	envmon_write(sc, ENVMON_ADDR, ENVMON_EPC_FAN_FLT);
	val = envmon_read(sc, ENVMON_DATA);

	total = 0;
	if (val == ENVMON_EPC_FAN_FLT_NONE) {
		edata->state = ENVSYS_SVALID;
	} else {
		edata->state = ENVSYS_SCRITICAL;
		for (i = 0; i < ENVMON_EPC_NUM_FAN_FLTS; i++) {
			if (!(val & __BIT(i))) {
				which = 1;
				for (j = 0; j < i; j++)
					which *= 10;
				total += which;
			}
		}
	}
	edata->value_cur = total;
}

void
envmon_refresh_temp(struct envmon_softc *sc, envsys_data_t *edata)
{
	uint8_t val;

	/* We currently only have 1 temperature sensor */
	envmon_write(sc, ENVMON_ADDR, sc->sc_temp[0].reg);
	val = envmon_read(sc, ENVMON_DATA);
	edata->value_cur = ENVMON_TEMP_BASE + val * 1000000;
	if (edata->value_cur > sc->sc_temp[0].cmax)
		edata->state = ENVSYS_SCRITICAL;
	else
		edata->state = ENVSYS_SVALID;
}

void
envmon_epic_refresh_ind(struct envmon_softc *sc, envsys_data_t *edata)
{
	int ind = edata->sensor -
	    sc->sc_nfans - ENVMON_EPC_MAX_TEMPS - ENVMON_EPC_MAX_FAULTS;
	uint8_t val;

	envmon_write(sc, ENVMON_ADDR, sc->sc_ind[ind].reg);
	val = envmon_read(sc, ENVMON_DATA);
	
	if (val == sc->sc_ind[ind].set)
		edata->value_cur = TRUE;
	else
		edata->value_cur = FALSE;
	edata->state = ENVSYS_SVALID;
}

void
envmon_pic_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct envmon_softc *sc = sme->sme_cookie;

	envmon_get_temp_limits(sc, edata, limits, props);
}

void
envmon_pic_set_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct envmon_softc *sc = sme->sme_cookie;

	envmon_set_temp_limits(sc, edata, limits, props);
}

void
envmon_epic_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct envmon_softc *sc = sme->sme_cookie;

	if (edata->sensor < sc->sc_nfans)
		envmon_epic_get_fan_limits(sc, edata, limits, props);
	else if (edata->sensor < sc->sc_nfans + ENVMON_EPC_MAX_TEMPS)
		envmon_get_temp_limits(sc, edata, limits, props);
}

void
envmon_epic_set_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct envmon_softc *sc = sme->sme_cookie;

	if (edata->sensor < sc->sc_nfans)
		envmon_epic_set_fan_limits(sc, edata, limits, props);
	else if (edata->sensor < sc->sc_nfans + ENVMON_EPC_MAX_TEMPS)
		envmon_set_temp_limits(sc, edata, limits, props);
}

static void
envmon_epic_get_fan_limits(struct envmon_softc *sc,
    envsys_data_t *edata, sysmon_envsys_lim_t *limits, uint32_t *props)
{

	*props = PROP_CRITMIN;
	limits->sel_critmin = sc->sc_fan[edata->sensor].cmin;
}

static void
envmon_get_temp_limits(struct envmon_softc *sc,
    envsys_data_t *edata, sysmon_envsys_lim_t *limits, uint32_t *props)
{

	*props = PROP_WARNMIN | PROP_WARNMAX | PROP_CRITMAX;
	limits->sel_warnmin = sc->sc_temp[0].wmin;
	limits->sel_warnmax = sc->sc_temp[0].wmax;
	limits->sel_critmax = sc->sc_temp[0].cmax;
}

static void
envmon_epic_set_fan_limits(struct envmon_softc *sc,
    envsys_data_t *edata, sysmon_envsys_lim_t *limits, uint32_t *props)
{

	if (limits == NULL || *props & PROP_CRITMIN) {
		if (limits == NULL)	/* Restore defaults */
			switch (sc->sc_machine_type) {
				case ENVMON_MACHINE_V215:
					sc->sc_fan[edata->sensor].cmin =
					    ENVMON_EPC_CRIT_SPEED_V215;
					break;
				case ENVMON_MACHINE_V245:
					sc->sc_fan[edata->sensor].cmin =
					    ENVMON_EPC_CRIT_SPEED_V245;
					break;
			}
		else
			sc->sc_fan[edata->sensor].cmin = limits->sel_critmin;
	}
}

static void
envmon_set_temp_limits(struct envmon_softc *sc,
    envsys_data_t *edata, sysmon_envsys_lim_t *limits, uint32_t *props)
{

	if (limits == NULL || *props & PROP_CRITMAX) {
		if (limits == NULL)	/* Restore defaults */
			switch (sc->sc_machine_type) {
				case ENVMON_MACHINE_U25:
				case ENVMON_MACHINE_U45:
					sc->sc_temp[0].cmax =
					    ENVMON_PIC_CRIT_TEMP;
					break;
				case ENVMON_MACHINE_V215:
				case ENVMON_MACHINE_V245:
					sc->sc_temp[0].cmax =
					    ENVMON_EPC_CRIT_TEMP;
					break;
			}
		else
			sc->sc_temp[edata->sensor].cmax = limits->sel_critmax;
	}

	if (limits == NULL || *props & PROP_WARNMAX) {
		if (limits == NULL)	/* Restore defaults */
			switch (sc->sc_machine_type) {
				case ENVMON_MACHINE_U25:
				case ENVMON_MACHINE_U45:
					sc->sc_temp[0].wmax =
					    ENVMON_PIC_WMAX_TEMP;
					break;
				case ENVMON_MACHINE_V215:
				case ENVMON_MACHINE_V245:
					sc->sc_temp[0].wmax =
					    ENVMON_EPC_WMAX_TEMP;
					break;
			}
		else
			sc->sc_temp[edata->sensor].wmax = limits->sel_warnmax;
	}

	if (limits == NULL || *props & PROP_WARNMIN) {
		if (limits == NULL)	/* Restore defaults */
			switch (sc->sc_machine_type) {
				case ENVMON_MACHINE_U25:
				case ENVMON_MACHINE_U45:
					sc->sc_temp[0].wmin =
					    ENVMON_PIC_WMIN_TEMP;
					break;
				case ENVMON_MACHINE_V215:
				case ENVMON_MACHINE_V245:
					sc->sc_temp[0].wmin =
					    ENVMON_EPC_WMIN_TEMP;
					break;
			}
		else
			sc->sc_temp[edata->sensor].wmin = limits->sel_warnmin;
	}

}

int
envmon_epic_get_led(void *cookie)
{
	struct envmon_sc_led *l = cookie;
	struct envmon_softc *sc = l->cookie;
	uint8_t val;
	
	envmon_write(sc, ENVMON_ADDR, l->reg);
	val = envmon_read(sc, ENVMON_DATA);
	return ((val & l->v_on) == l->v_on);
}

void
envmon_epic_set_led(void *cookie, int val)
{
	struct envmon_sc_led *l = cookie;
	struct envmon_softc *sc = l->cookie;
	uint8_t oldval, newval;
	
	envmon_write(sc, ENVMON_ADDR, l->reg);
	oldval = envmon_read(sc, ENVMON_DATA);
	newval = oldval & ~(l->v_on);
	newval |= val ? l->v_on : 0x0;
	if (newval != oldval) {
		envmon_write(sc, ENVMON_EPC_LED_MASK, l->v_on);
		envmon_write(sc, ENVMON_ADDR, l->reg);
		delay(10000);
		envmon_write(sc, ENVMON_DATA, newval);
	}
}

#ifdef ENVMON_DEBUG
static void
envmon_timeout(void *v)
{
	struct envmon_softc *sc = v;
	uint8_t reg, val;

	for (reg = 0; reg < ENVMON_MAX_REG; reg++) {
		envmon_write(sc, ENVMON_ADDR, reg);
		val = envmon_read(sc, ENVMON_DATA);
		if (val != sc->sc_reg[reg]) {
			printf("%s: reg 0x%02x: 0x%02x > 0x%02x\n",
			    device_xname(sc->sc_dev), reg,
			    sc->sc_reg[reg], val);
			sc->sc_reg[reg] = val;
		}
	}
	callout_reset(&sc->sc_timer, hz * ENVMON_TIMER, envmon_timeout, sc);
}
#endif
