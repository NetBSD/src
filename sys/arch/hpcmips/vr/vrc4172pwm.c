/*	$Id: vrc4172pwm.c,v 1.3 2000/12/29 11:44:44 sato Exp $	*/

/*
 * Copyright (c) 2000 SATO Kazumi. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/bus.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrc4172pwmvar.h>
#include <hpcmips/vr/vrc4172pwmreg.h>

#include "locators.h"

#ifdef VRC2PWMDEBUG
#ifndef VRC2PWMDEBUG_CONF
#define VRC2PWMDEBUG_CONF 0
#endif /* VRC2PWMDEBUG_CONF */
int vrc4172pwmdebug = VRC2PWMDEBUG_CONF;
#define DPRINTF(arg) if (vrc4172pwmdebug) printf arg;
#define VPRINTF(arg) if (bootverbose||vrc4172pwmdebug) printf arg;
#define VDUMPREG(arg) if (bootverbose||vrc4172pwmdebug) vrc4172pwm_dumpreg(arg);
#else /* VRC2PWMDEBUG */
#define DPRINTF(arg)
#define VPRINTF(arg) if (bootverbose) printf arg;
#define VDUMPREG(arg) if (bootverbose) vrc4172pwm_dumpreg(arg);
#endif /* VRC2PWMDEBUG */

static int vrc4172pwmprobe __P((struct device *, struct cfdata *, void *));
static void vrc4172pwmattach __P((struct device *, struct device *, void *));

static void vrc4172pwm_write __P((struct vrc4172pwm_softc *, int, unsigned short));
static unsigned short vrc4172pwm_read __P((struct vrc4172pwm_softc *, int));

static int vrc4172pwm_event __P((void *, int, long, void *));
static int vrc4172pwm_pmevent __P((void *, int, long, void *));

static void vrc4172pwm_dumpreg __P((struct vrc4172pwm_softc *));
static void vrc4172pwm_init_brightness __P((struct vrc4172pwm_softc *));
void vrc4172pwm_light __P((struct vrc4172pwm_softc *, int));
int vrc4172pwm_get_light __P((struct vrc4172pwm_softc *));
int vrc4172pwm_get_brightness __P((struct vrc4172pwm_softc *));
void vrc4172pwm_set_brightness __P((struct vrc4172pwm_softc *, int));
int vrc4172pwm_rawduty2brightness __P((struct vrc4172pwm_softc *));
int vrc4172pwm_brightness2rawduty __P((struct vrc4172pwm_softc *));
struct vrc4172pwm_param * vrc4172pwm_getparam __P((void));
void vrc4172pwm_dumpreg __P((struct vrc4172pwm_softc *));

struct cfattach vrc4172pwm_ca = {
	sizeof(struct vrc4172pwm_softc), vrc4172pwmprobe, vrc4172pwmattach
};

/*
 * platform related parameters
 */
struct vrc4172pwm_param vrc4172pwm_mcr530_param = {
	8,
	{ 0x16, 0x1b, 0x20, 0x25, 0x2a, 0x30, 0x37, 0x3f }
};

struct vrc4172pwm_platid_param vrc4172pwm_platid_param_table[] = {
	{ &platid_mask_MACH_NEC_MCR_530A, 
		&vrc4172pwm_mcr530_param},
	{ &platid_mask_MACH_NEC_MCR_SIGMARION, 
		&vrc4172pwm_mcr530_param},
	{ NULL, NULL}
};

struct vrc4172pwm_softc *this_pwm;

static inline void
vrc4172pwm_write(sc, port, val)
	struct vrc4172pwm_softc *sc;
	int port;
	unsigned short val;
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, port, val);
}

static inline unsigned short
vrc4172pwm_read(sc, port)
	struct vrc4172pwm_softc *sc;
	int port;
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, port);
}

static int
vrc4172pwmprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	platid_mask_t mask;
	struct vrip_attach_args *va = aux;
	bus_space_handle_t ioh;
	int data;
	int ret = 0;

	if (va->va_addr == VRIPCF_ADDR_DEFAULT)
		return 0;
 
	if (cf->cf_loc[NEWGPBUSIFCF_PLATFORM] == 0)
		return 0;
	if (cf->cf_loc[NEWGPBUSIFCF_PLATFORM] != -1) { /* if specify */
		mask = PLATID_DEREF(cf->cf_loc[NEWGPBUSIFCF_PLATFORM]);
		if (platid_match(&platid, &mask) == 0)	
			return 0;
	}
	if (bus_space_map(va->va_iot, va->va_addr, va->va_size, 0, &ioh)) {
		return 0;
	}
	data =  bus_space_read_2(va->va_iot, ioh, VRC2_PWM_LCDDUTYEN);
	bus_space_write_2(va->va_iot, ioh, VRC2_PWM_LCDDUTYEN, 0xff);
	if (bus_space_read_2(va->va_iot, ioh, VRC2_PWM_LCDDUTYEN) 
		== VRC2_PWM_LCDEN_MASK) {
		ret = 1;
	}
	bus_space_write_2(va->va_iot, ioh, VRC2_PWM_LCDDUTYEN, data);
	bus_space_unmap(va->va_iot, ioh, va->va_size);

	return ret;
}

static void
vrc4172pwmattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct vrc4172pwm_softc *sc = (struct vrc4172pwm_softc *)self;
	struct vrip_attach_args *va = aux;

	bus_space_tag_t iot = va->va_iot;
	bus_space_handle_t ioh;

	if (bus_space_map(iot, va->va_addr, 1, 0, &ioh)) {
		printf(": can't map bus space\n");
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	printf("\n");

	VDUMPREG(sc);
	/* basic setup */
	sc->sc_pmhook = config_hook(CONFIG_HOOK_PMEVENT,
					CONFIG_HOOK_PMEVENT_HARDPOWER,
					CONFIG_HOOK_SHARE,
					vrc4172pwm_pmevent, sc);
	sc->sc_lcdhook = config_hook(CONFIG_HOOK_POWERCONTROL,
					CONFIG_HOOK_POWERCONTROL_LCDLIGHT,
					CONFIG_HOOK_SHARE,
					vrc4172pwm_event, sc);
	sc->sc_getlcdhook = config_hook(CONFIG_HOOK_GET,
					CONFIG_HOOK_POWER_LCDLIGHT,
					CONFIG_HOOK_SHARE,
					vrc4172pwm_event, sc);
	sc->sc_sethook = config_hook(CONFIG_HOOK_SET,
					CONFIG_HOOK_BRIGHTNESS,
					CONFIG_HOOK_SHARE,
					vrc4172pwm_event, sc);
	sc->sc_gethook = config_hook(CONFIG_HOOK_GET,
					CONFIG_HOOK_BRIGHTNESS,
					CONFIG_HOOK_SHARE,
					vrc4172pwm_event, sc);
	sc->sc_getmaxhook = config_hook(CONFIG_HOOK_GET,
					CONFIG_HOOK_BRIGHTNESS_MAX,
					CONFIG_HOOK_SHARE,
					vrc4172pwm_event, sc);

	vrc4172pwm_init_brightness(sc);
	this_pwm = sc;
}

/* 
 * get platform related brightness paramerters
 */
struct vrc4172pwm_param *
vrc4172pwm_getparam()
{
	struct vrc4172pwm_platid_param *p;

	if ((p = (struct vrc4172pwm_platid_param *)
		platid_search(&platid, (void *)vrc4172pwm_platid_param_table)))
		return p->param;
	return NULL;
}

/*
 *
 * Initialize PWM brightness parameters
 *
 */
void
vrc4172pwm_init_brightness(sc)
struct vrc4172pwm_softc *sc;
{
	sc->sc_param = vrc4172pwm_getparam();
	sc->sc_raw_freq = vrc4172pwm_read(sc, VRC2_PWM_LCDFREQ);
	sc->sc_raw_duty = vrc4172pwm_read(sc, VRC2_PWM_LCDDUTY);
	sc->sc_brightness = vrc4172pwm_rawduty2brightness(sc);
}
/*
 * backlight on/off
 */
void
vrc4172pwm_light(sc, on)
	struct vrc4172pwm_softc *sc;
	int on;
{
	if (on) 
		vrc4172pwm_write(sc, VRC2_PWM_LCDDUTYEN, VRC2_PWM_LCD_EN);
	else
		vrc4172pwm_write(sc, VRC2_PWM_LCDDUTYEN, VRC2_PWM_LCD_DIS);
}

/*
 * get backlight on/off
 */
inline int
vrc4172pwm_get_light(sc)
	struct vrc4172pwm_softc *sc;
{
		return vrc4172pwm_read(sc, VRC2_PWM_LCDDUTYEN);
}

/*
 * set brightness
 */
void
vrc4172pwm_set_brightness(sc, val)
	struct vrc4172pwm_softc *sc;
	int val;
{
	int raw;

	if (sc->sc_param == NULL)
		return;
	if (val > VRC2_PWM_MAX_BRIGHTNESS)
		val = VRC2_PWM_MAX_BRIGHTNESS;
	if (val > sc->sc_param->n_brightness)
		val = sc->sc_param->n_brightness;
	sc->sc_brightness = val;
	raw = vrc4172pwm_brightness2rawduty(sc);
	vrc4172pwm_write(sc, VRC2_PWM_LCDDUTY, raw);
}

/*
 * get brightness
 */
int
vrc4172pwm_get_brightness(sc)
	struct vrc4172pwm_softc *sc;
{
	if (sc->sc_param == NULL)
		return VRC2_PWM_MAX_BRIGHTNESS;
	return sc->sc_brightness;
}

/*
 * PWM duty to brightness
 */
int
vrc4172pwm_rawduty2brightness(sc)
struct vrc4172pwm_softc *sc;
{
	int i;

	if (sc->sc_param == NULL)
		return VRC2_PWM_LCDDUTY_MASK;
	for (i = 0; i < sc->sc_param->n_brightness; i++) {
		if (sc->sc_raw_duty <= sc->sc_param->values[i])
			break;
	}
	if (i >= sc->sc_param->n_brightness-1)
		return sc->sc_param->n_brightness-1;
	else
		return i;

}

/*
 * brightness to raw duty
 */
int
vrc4172pwm_brightness2rawduty(sc)
struct vrc4172pwm_softc *sc;
{
	if (sc->sc_param == NULL)
		return VRC2_PWM_MAX_BRIGHTNESS;
	return sc->sc_param->values[sc->sc_brightness];
}


/*
 * PWM config hook events 
 *
 */
int
vrc4172pwm_event(ctx, type, id, msg)
	void *ctx;
        int type;
        long id;
        void *msg;
{
	struct vrc4172pwm_softc *sc = (struct vrc4172pwm_softc *)ctx;
        int why =(int)msg;

	if (type == CONFIG_HOOK_POWERCONTROL 
		&& id == CONFIG_HOOK_POWERCONTROL_LCDLIGHT) {
		vrc4172pwm_light(sc, why);
	} else if (type == CONFIG_HOOK_GET
		&& id == CONFIG_HOOK_POWER_LCDLIGHT) {
		*(int *)msg = vrc4172pwm_get_light(sc);
	} else if (type == CONFIG_HOOK_GET
		&& id == CONFIG_HOOK_BRIGHTNESS) {
		*(int *)msg = vrc4172pwm_get_brightness(sc);
	} else if (type == CONFIG_HOOK_GET
		&& id == CONFIG_HOOK_BRIGHTNESS_MAX) {
		if (sc->sc_param == NULL)
			*(int *)msg = VRC2_PWM_MAX_BRIGHTNESS;
		else
			*(int *)msg = sc->sc_param->n_brightness-1;
	} else if (type == CONFIG_HOOK_SET
		&& id == CONFIG_HOOK_BRIGHTNESS) {
		vrc4172pwm_set_brightness(sc, *(int *)msg);
	} else
		return 1;
	
        return (0);
}


/*
 * PWM config hook events 
 *
 */
int
vrc4172pwm_pmevent(ctx, type, id, msg)
	void *ctx;
        int type;
        long id;
        void *msg;
{
	struct vrc4172pwm_softc *sc = (struct vrc4172pwm_softc *)ctx;
        int why =(int)msg;

	if (type != CONFIG_HOOK_PMEVENT)
		return 1;
		
        switch (why) {
	case PWR_STANDBY:
	case PWR_SUSPEND:
		vrc4172pwm_light(sc, 0);
		break;
	case PWR_RESUME:
		vrc4172pwm_light(sc, 1);
		break;
	default:
		return 1;
        }

        return (0);
}

/*
 * dump pwm registers
 */
void
vrc4172pwm_dumpreg(sc)
	struct vrc4172pwm_softc *sc;
{
	int en, freq, duty;

	en = vrc4172pwm_read(sc, VRC2_PWM_LCDDUTYEN);
	freq = vrc4172pwm_read(sc, VRC2_PWM_LCDFREQ);
	duty = vrc4172pwm_read(sc, VRC2_PWM_LCDDUTY);

	printf("vrc4172pwm: lightenable = %d, freq = 0x%x, duty = 0x%x\n",
		en, freq, duty);
}

/* end */
