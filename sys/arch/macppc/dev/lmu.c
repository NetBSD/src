/* $NetBSD: lmu.c,v 1.9 2021/06/18 22:52:04 macallan Exp $ */

/*-
 * Copyright (c) 2020 Michael Lorenz
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

/*
 * ambient light controller found in PowerBook5,6
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lmu.c,v 1.9 2021/06/18 22:52:04 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/time.h>
#include <sys/callout.h>
#include <sys/sysctl.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>
#include "opt_lmu.h"

#ifdef LMU_DEBUG
#define DPRINTF printf
#else
#define DPRINTF if (0) printf
#endif

struct lmu_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_node;

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensors[2];
	callout_t	sc_adjust;
	int		sc_thresh, sc_hyst, sc_level, sc_target, sc_current;
	int		sc_lux[2];
	time_t		sc_last;
	int		sc_lid_state, sc_video_state;
};

static int	lmu_match(device_t, cfdata_t, void *);
static void	lmu_attach(device_t, device_t, void *);

static void	lmu_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	lmu_set_brightness(struct lmu_softc *, int);
static int	lmu_get_brightness(struct lmu_softc *, int);
static void	lmu_adjust(void *);
static int 	lmu_sysctl(SYSCTLFN_ARGS);
static int 	lmu_sysctl_thresh(SYSCTLFN_ARGS);

CFATTACH_DECL_NEW(lmu, sizeof(struct lmu_softc),
    lmu_match, lmu_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "lmu-micro" },
	{ .compat = "lmu-controller" },
	DEVICE_COMPAT_EOL
};

/* time between polling the light sensors */
#define LMU_POLL	(hz * 2)
/* time between updates to keyboard brightness */
#define LMU_FADE	(hz / 16)

static void
lmu_lid_open(device_t dev)
{
	struct lmu_softc * const sc = device_private(dev);

	sc->sc_lid_state = true;
}

static void
lmu_lid_close(device_t dev)
{
	struct lmu_softc * const sc = device_private(dev);

	sc->sc_lid_state = false;
}

static void
lmu_video_on(device_t dev)
{
	struct lmu_softc * const sc = device_private(dev);

	sc->sc_video_state = true;
}

static void
lmu_video_off(device_t dev)
{
	struct lmu_softc * const sc = device_private(dev);

	sc->sc_video_state = false;
}

static void
lmu_kbd_brightness_up(device_t dev)
{
	struct lmu_softc * const sc = device_private(dev);

	sc->sc_level = __MIN(16, sc->sc_level + 2);
	sc->sc_target = sc->sc_level;
	callout_schedule(&sc->sc_adjust, LMU_FADE);
}

static void
lmu_kbd_brightness_down(device_t dev)
{
	struct lmu_softc * const sc = device_private(dev);

	sc->sc_level = __MAX(0, sc->sc_level - 2);
	sc->sc_target = sc->sc_level;
	callout_schedule(&sc->sc_adjust, LMU_FADE);
}

static int
lmu_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	return 0;
}

static void
lmu_attach(device_t parent, device_t self, void *aux)
{
	struct lmu_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	envsys_data_t *s;
	const struct sysctlnode *me;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_node = ia->ia_cookie;
	sc->sc_last = 0;

	aprint_naive("\n");
	aprint_normal(": ambient light sensor\n");

	sc->sc_lid_state = true;
	pmf_event_register(sc->sc_dev, PMFE_CHASSIS_LID_OPEN,
	    lmu_lid_open, true);
	pmf_event_register(sc->sc_dev, PMFE_CHASSIS_LID_CLOSE,
	    lmu_lid_close, true);
	sc->sc_video_state = true;
	pmf_event_register(sc->sc_dev, PMFE_DISPLAY_ON,
	    lmu_video_on, true);
	pmf_event_register(sc->sc_dev, PMFE_DISPLAY_OFF,
	    lmu_video_off, true);
	pmf_event_register(sc->sc_dev, PMFE_KEYBOARD_BRIGHTNESS_UP,
	    lmu_kbd_brightness_up, true);
	pmf_event_register(sc->sc_dev, PMFE_KEYBOARD_BRIGHTNESS_DOWN,
	    lmu_kbd_brightness_down, true);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = lmu_sensors_refresh;

	s = &sc->sc_sensors[0];
	s->state = ENVSYS_SINVALID;
	s->units = ENVSYS_LUX;
	strcpy(s->desc, "right");
	s->private = 0;
	sysmon_envsys_sensor_attach(sc->sc_sme, s);

	s = &sc->sc_sensors[1];
	s->state = ENVSYS_SINVALID;
	s->units = ENVSYS_LUX;
	strcpy(s->desc, "left");
	s->private = 1;
	sysmon_envsys_sensor_attach(sc->sc_sme, s);

	sysmon_envsys_register(sc->sc_sme);

	sc->sc_thresh = 300;
	sc->sc_hyst = 30;
	sc->sc_level = 16;
	sc->sc_target = 0;
	sc->sc_current = 0;

	sysctl_createv(NULL, 0, NULL, &me,
		CTLFLAG_READWRITE,
		CTLTYPE_NODE, "lmu",
		SYSCTL_DESCR("LMU driver"),
		NULL, 0, NULL, 0,
		CTL_HW, CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, NULL, NULL,
		CTLFLAG_READWRITE,
		CTLTYPE_INT, "level",
		SYSCTL_DESCR("keyboard brightness"),
		lmu_sysctl, 0, (void *)sc, 0,
		CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, NULL, NULL,
		CTLFLAG_READWRITE,
		CTLTYPE_INT, "threshold",
		SYSCTL_DESCR("environmental light threshold"),
		lmu_sysctl_thresh, 1, (void *)sc, 0,
		CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);

	callout_init(&sc->sc_adjust, 0);
	callout_setfunc(&sc->sc_adjust, lmu_adjust, sc);
	callout_schedule(&sc->sc_adjust, 0);
}

static void
lmu_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct lmu_softc *sc = sme->sme_cookie;
	int ret;

	if ( edata->private < 3) {
		ret = lmu_get_brightness(sc, edata->private);
		if (ret == -1) return;
		edata->value_cur = ret;
	}
	edata->state = ENVSYS_SVALID;
}

static int
lmu_get_brightness(struct lmu_softc *sc, int reg)
{
	int error, i;
	uint16_t buf[2];
	uint8_t cmd = 0;

	if (reg > 1) return -1;
	if (time_second == sc->sc_last)
		return sc->sc_lux[reg];

	iic_acquire_bus(sc->sc_i2c, 0);
	error = iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		sc->sc_addr, &cmd, 1, buf, 4, 0);
	iic_release_bus(sc->sc_i2c, 0);
	if (error) return -1;
	sc->sc_last = time_second;

	for (i = 0; i < 2; i++)
		sc->sc_lux[i] = be16toh(buf[i]);

	DPRINTF("<%d %04x %04x>", reg, buf[0], buf[1]);
	
	return (sc->sc_lux[reg]);
}

static void
lmu_set_brightness(struct lmu_softc *sc, int b)
{
	uint8_t cmd[3];

	cmd[0] = 1;

	cmd[1] = (b & 0xff);
	cmd[2] = (b & 0xff) >> 8;

	iic_acquire_bus(sc->sc_i2c, 0);
	iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		sc->sc_addr, &cmd, 3, NULL, 0, 0);
	iic_release_bus(sc->sc_i2c, 0);
}

static void
lmu_adjust(void *cookie)
{
	struct lmu_softc *sc = cookie;
	int left, right, b, offset;

	left = lmu_get_brightness(sc, 1);
	right = lmu_get_brightness(sc, 0);
	b = left > right ? left : right;

	if ((b > (sc->sc_thresh + sc->sc_hyst)) ||
	   !(sc->sc_lid_state && sc->sc_video_state)) {
		sc->sc_target = 0;
	} else if (b < sc->sc_thresh) {
		sc->sc_target = sc->sc_level;
	}

	if (sc->sc_target == sc->sc_current) {
		/* no update needed, check again later */
		callout_schedule(&sc->sc_adjust, LMU_POLL);
		return;
	}	


	offset = ((sc->sc_target - sc->sc_current) > 0) ? 2 : -2;
	sc->sc_current += offset;
	if (sc->sc_current > sc->sc_level) sc->sc_current = sc->sc_level;
	if (sc->sc_current < 0) sc->sc_current = 0;

	DPRINTF("[%d]", sc->sc_current);

	lmu_set_brightness(sc, sc->sc_current);

	if (sc->sc_target == sc->sc_current) {
		/* no update needed, check again later */
		callout_schedule(&sc->sc_adjust, LMU_POLL);
		return;
	}	

	/* more updates upcoming */
	callout_schedule(&sc->sc_adjust, LMU_FADE);
}

static int
lmu_sysctl(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct lmu_softc *sc = node.sysctl_data;
	int target;

	target = sc->sc_level;
	node.sysctl_data = &target;
	if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
		int new_reg;

		new_reg = *(int *)node.sysctl_data;
		if (new_reg != sc->sc_target) {
			sc->sc_level = target;
			sc->sc_target = target;
			
		}
		return 0;
	}
	return EINVAL;
}

static int
lmu_sysctl_thresh(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct lmu_softc *sc = node.sysctl_data;
	int thresh;

	thresh = sc->sc_thresh;
	node.sysctl_data = &thresh;
	if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
		int new_reg;

		new_reg = *(int *)node.sysctl_data;
		if (new_reg != sc->sc_thresh && new_reg > 0) {
			sc->sc_thresh = new_reg;
		}
		return 0;
	}
	return EINVAL;
}
