/*	$NetBSD: vrc4172pwm.c,v 1.2.2.2 2000/11/22 16:00:14 bouyer Exp $	*/

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

#include <hpcmips/vr/vrc4172pwmvar.h>
#include <hpcmips/vr/vrc4172pwmreg.h>


#ifdef VRC2PWMDEBUG
#ifndef VRC2PWMDEBUG_CONF
#define VRC2PWMDEBUG_CONF 0
#endif /* VRC2PWMDEBUG_CONF */
int vrc4172pwmdebug = VRC2PWMDEBUG_CONF;
#define DPRINTF(arg) if (vrc4172pwmdebug) printf arg;
#define VPRINTF(arg) if (bootverbose||vrc4172pwmdebug) printf arg;
#define VDUMPREG(arg) if (bootverbose||vrc4172pwmdebug) vrc4172_dumpreg arg;
#else /* VRC2PWMDEBUG */
#define DPRINTF(arg)
#define VPRINTF(arg) if (bootverbose) printf arg;
#define VDUMPREG(arg) if (bootverbose) vrc4172_dumpreg arg;
#endif /* VRC2PWMDEBUG */

static int vrc4172pwmmatch __P((struct device *, struct cfdata *, void *));
static void vrc4172pwmattach __P((struct device *, struct device *, void *));

static void vrc4172pwm_write __P((struct vrc4172pwm_softc *, int, unsigned short));
static unsigned short vrc4172pwm_read __P((struct vrc4172pwm_softc *, int));

static int vrc4172pwm_event __P((void *, int, long, void *));
static int vrc4172pwm_pmevent __P((void *, int, long, void *));

static void vrc4172pwm_dumpreg __P((struct vrc4172pwm_softc *));
static void vrc4172pwm_init_brightness __P((struct vr4172pwm_softc *));
void vrc4172pwm_light __P((struct vr4172pwm_softc *, int));
int vrc4172pwm_get_brightness __P((struct vr4172pwm_softc *));
void vrc4172pwm_set_brightness __P((struct vr4172pwm_softc *), int);


struct cfattach vrc4172pwm_ca = {
	sizeof(struct vrc4172pwm_softc), vrc4172pwmmatch, vrc4172pwmattach
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
vrc4172pwmmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
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
					CONFIG_PMEVENT_HARDPOWER,
					CONFIG_HOOK_SHARE,
					vrc4172pwm_pmevent, sc);
	sc->sc_hook = config_hook(CONFIG_HOOK_POWERCONTROL,
					CONFIG_POWERCONTROL_LCDLIGHT,
					CONFIG_HOOK_SHARE,
					vrc4172pwm_event, sc);
	sc->sc_sethook = config_hook(CONFIG_HOOK_SET,
					CONFIG_SET_BRIGHTNESS,
					CONFIG_HOOK_SHARE,
					vrc4172pwm_event, sc);
	sc->sc_gethook = config_hook(CONFIG_HOOK_GET,
					CONFIG_GET_BRIGHTNESS,
					CONFIG_HOOK_SHARE,
					vrc4172pwm_event, sc);

	vr4172pwm_init_brightness(sc);
	this_pwm = sc;
}

/*
 *
 * Initialize PWM brightness parameters
 *
 */
void
vrc4172pwm_init_brightness(sc)
{
	sc->sc_raw_freq = vrc4172pwm_read(sc, VRC2_PWM_LCDFREQ);
	sc->sc_raw_duty = vrc4172pwm_read(sc, VRC2_PWM_LCDDUTY);
	sc->sc_param = vrc4172pwm_getparam();
	sc->sc_brightness = vrc4172pwm_raw_duty2brightness(sc);
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
		&& id == CONFIG_HOOK_GET_BRIGHTNESS) {
		*(int *)msg = vr4172pwm_get_brightness(sc);
	} else if (type == CONFIG_HOOK_SET
		&& id == CONFIG_HOOK_GET_BRIGHTNESS) {
		vr4172pwm_set_brightness(sc, *(int *)msg);
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
	case PWR_STANBY:
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
 *
 */
void
vrc4172pwm_light(sc, on)
	struct vrc4172pwm_softc *sc;
	int on;
{
	if (on) 
		vrc4172pwm_write(sc, VRC2_PWM_LCDDUTYEN, VRC2_PWM_LCDEN);
	else
		vrc4172pwm_write(sc, VRC2_PWM_LCDDUTYEN, VRC2_PWM_LCDDIS);
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

	if (val > VRC2_PWM_MAX_BRIGHTNESS)
		val = VRC2_PWM_MAX_BRIGHTNESS;
	if (val > sc->sc_param->max_brightness)
		val = sc->sc_param->max_brightness;
	raw = vrc4172pwm_brightness2rawduty(sc, val);
	vrc4172pwm_write(sc, VRC2_PWM_LCDDUTY, raw);
	sc->sc_brightness = val;
}

/*
 * get brightness
 */
int
vrc4172pwm_set_brightness(sc)
	struct vrc4172pwm_softc *sc;
{
	return sc->sc_brightness;
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
