/*	$NetBSD: vrled.c,v 1.3.4.2 2002/02/28 04:10:07 nathanw Exp $	*/

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

#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/vrledvar.h>
#include <hpcmips/vr/vrledreg.h>


#ifdef VRLEDDEBUG
#ifndef VRLEDDEBUG_CONF
#define VRLEDDEBUG_CONF 0
#endif /* VRLEDDEBUG_CONF */
int vrleddebug = VRLEDDEBUG_CONF;
#define DPRINTF(arg) if (vrleddebug) printf arg;
#define VPRINTF(arg) if (bootverbose||vrleddebug) printf arg;
#else /* VRLEDDEBUG */
#define DPRINTF(arg)
#define VPRINTF(arg) if (bootverbose) printf arg;
#endif /* VRLEDDEBUG */

static int vrledmatch(struct device *, struct cfdata *, void *);
static void vrledattach(struct device *, struct device *, void *);

static void vrled_write(struct vrled_softc *, int, unsigned short);
static unsigned short vrled_read(struct vrled_softc *, int);

static void vrled_stop(struct vrled_softc *);
static void vrled_on(struct vrled_softc *);
static void vrled_blink(struct vrled_softc *);
static void vrled_flash(struct vrled_softc *);
static void vrled_change_state(struct vrled_softc *);
static int vrled_event(void *, int, long, void *);

int vrled_intr(void *);

struct cfattach vrled_ca = {
	sizeof(struct vrled_softc), vrledmatch, vrledattach
};

struct vrled_softc *this_led;

static inline void
vrled_write(struct vrled_softc *sc, int port, unsigned short val)
{

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, port, val);
}

static inline unsigned short
vrled_read(struct vrled_softc *sc, int port)
{

	return (bus_space_read_2(sc->sc_iot, sc->sc_ioh, port));
}

static int
vrledmatch(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

static void
vrledattach(struct device *parent, struct device *self, void *aux)
{
	struct vrled_softc *sc = (struct vrled_softc *)self;
	struct vrip_attach_args *va = aux;

	bus_space_tag_t iot = va->va_iot;
	bus_space_handle_t ioh;

	if (bus_space_map(iot, va->va_addr, 1, 0, &ioh)) {
		printf(": can't map bus space\n");
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	if (!(sc->sc_handler = 
	    vrip_intr_establish(va->va_vc, va->va_unit, 0, IPL_TTY,
		vrled_intr, sc))) {
		printf (": can't map interrupt line.\n");
		return;
	}

	printf("\n");
	/* clear interrupt status */
	vrled_write(sc, LEDINT_REG_W, LEDINT_ALL);

	/* basic setup */
	sc->sc_state_cnt = 1;
	vrled_write(sc, LEDASTC_REG_W, 1); /* 1time */
	vrled_write(sc, LEDCNT_REG_W, LEDCNT_AUTOSTOP);	
	vrled_stop(sc);

	sc->sc_hook = config_hook(CONFIG_HOOK_SET,
	    CONFIG_HOOK_LED, CONFIG_HOOK_SHARE, vrled_event, sc);

	this_led = sc;
}


/*
 * LED interrupt handler.
 *
 */
int
vrled_intr(void *arg)
{
        struct vrled_softc *sc = arg;
	unsigned int intstat;

	intstat = vrled_read(sc, LEDINT_REG_W);
	/* clear interrupt status */
	vrled_write(sc, LEDINT_REG_W, intstat);
	if (intstat&LEDINT_AUTOSTOP) {
		vrled_change_state(sc);
	}
	return (0);
}

/*
 * LED turn OFF
 *
 */
void
vrled_stop(struct vrled_softc *sc)
{
	vrled_write(sc, LEDHTS_REG_W, LEDHTS_DIV16SEC);
	vrled_write(sc, LEDLTS_REG_W, LEDLTS_4SEC);
	vrled_write(sc, LEDASTC_REG_W, 2); /* 2time */
	vrled_write(sc, LEDCNT_REG_W, LEDCNT_AUTOSTOP);	

	sc->sc_state = LEDOFF;
	sc->sc_next = LEDOFF;
}

/*
 * LED turn ON
 *
 */
void
vrled_on(struct vrled_softc *sc)
{
	vrled_write(sc, LEDHTS_REG_W, LEDHTS_SEC);
	vrled_write(sc, LEDLTS_REG_W, LEDLTS_DIV16SEC);
	vrled_write(sc, LEDASTC_REG_W, 2); /* 2time */
	vrled_write(sc, LEDCNT_REG_W, LEDCNT_AUTOSTOP|LEDCNT_BLINK);	

	sc->sc_state = LEDON;
	sc->sc_next = LEDON;
}

/*
 * LED blink
 *
 */
void
vrled_blink(struct vrled_softc *sc)
{
	int ledhts;
	int ledlts;

	switch (sc->sc_next) {
	case LED1SB:
		ledhts = LEDHTS_DIV2SEC;
		ledlts = LEDLTS_DIV2SEC;
		break;
	case LED2SB: 
		ledhts = LEDHTS_SEC;
		ledlts = LEDLTS_SEC;
		break;
	default:
		vrled_stop(sc);
		return;
	}

	vrled_write(sc, LEDHTS_REG_W, ledhts);
	vrled_write(sc, LEDLTS_REG_W, ledlts);
	vrled_write(sc, LEDASTC_REG_W, 2); /* 2time */
	vrled_write(sc, LEDCNT_REG_W, LEDCNT_AUTOSTOP);	
	vrled_write(sc, LEDCNT_REG_W, LEDCNT_AUTOSTOP|LEDCNT_BLINK);	

	sc->sc_state = sc->sc_next;
}

/*
 * LED flash once
 *
 */
void
vrled_flash(struct vrled_softc *sc)
{
	int ledhts;
	int ledlts;

	switch (sc->sc_next) {
	case LED8DIVF:
		ledhts = LEDHTS_DIV16SEC;
		ledlts = LEDLTS_DIV16SEC;
		break;
	case LED4DIVF:
		ledhts = LEDHTS_DIV8SEC;
		ledlts = LEDLTS_DIV8SEC;
		break;
	case LED2DIVF:
		ledhts = LEDHTS_DIV4SEC;
		ledlts = LEDLTS_DIV4SEC;
		break;
	case LED1SF:
		ledhts = LEDHTS_DIV2SEC;
		ledlts = LEDLTS_DIV2SEC;
		break;
	default:
		vrled_stop(sc);
		return;
	}

	vrled_write(sc, LEDHTS_REG_W, ledhts);
	vrled_write(sc, LEDLTS_REG_W, ledlts);
	vrled_write(sc, LEDASTC_REG_W, 2); /* 2time */
	vrled_write(sc, LEDCNT_REG_W, LEDCNT_AUTOSTOP|LEDCNT_BLINK);	

	sc->sc_state = sc->sc_next;
	sc->sc_next = LEDOFF;
	sc->sc_state_cnt = 1;
}

/*
 * Change LED state
 *
 */
void
vrled_change_state(struct vrled_softc *sc)
{

	switch (sc->sc_next) {
	case LEDOFF:
		vrled_stop(sc);
		break;
	case LEDON:
		vrled_on(sc);
		break;
	case LED1SB:
	case LED2SB: 
		vrled_blink(sc);
		break;
	case LED8DIVF:
	case LED4DIVF:
	case LED2DIVF:
	case LED1SF:
		vrled_flash(sc);
		break;
	default:
		vrled_stop(sc);
		break;
	}
}

/*
 * Set LED state
 *
 */
void
vrled_set_state(struct vrled_softc *sc, vrled_status state)
{

	int ledstate;

	ledstate = vrled_read(sc, LEDCNT_REG_W);
	if (ledstate&LEDCNT_BLINK) { /* currently processing */
		if (sc->sc_next == state)
			sc->sc_state_cnt++;
		switch (sc->sc_next) {
		case LEDOFF:
		case LEDON:
			sc->sc_next = state;
			break;
		case LED8DIVF:
		case LED4DIVF:
		case LED2DIVF:
		case LED1SF:
			switch (state) {
			case LEDOFF:
			case LED8DIVF:
			case LED4DIVF:
			case LED2DIVF:
			case LED1SF:
				sc->sc_next = state;
				break;
			default:
				break;
			}
			break;
		case LED1SB:
		case LED2SB: 
			switch (state) {
			case LEDOFF:
			case LEDON:
			case LED1SB:
			case LED2SB:
				sc->sc_next = state;
				break;
			default:
				break;
			}
			break;
		default:
			sc->sc_next = LEDOFF;
			break;
		}
		return;
	}
	sc->sc_next = state;
	vrled_change_state(sc);
}

/*
 * LED config hook events 
 *
 */
int
vrled_event(void *ctx, int type, long id, void *msg)
{
	struct vrled_softc *sc = (struct vrled_softc *)ctx;
        int why =*(int *)msg;

	if (type != CONFIG_HOOK_SET 
	    || id != CONFIG_HOOK_LED)
		return (1);
	if (msg == NULL)
		return (1); 
		
        switch (why) {
        case CONFIG_HOOK_LED_OFF:
		vrled_set_state(sc, LEDOFF);
                break;
        case CONFIG_HOOK_LED_ON:
		vrled_set_state(sc, LEDON);
                break;
        case CONFIG_HOOK_LED_FLASH:
		vrled_set_state(sc, LED8DIVF);
                break;
        case CONFIG_HOOK_LED_FLASH2:
		vrled_set_state(sc, LED4DIVF);
                break;
        case CONFIG_HOOK_LED_FLASH5:
		vrled_set_state(sc, LED2DIVF);
                break;
        case CONFIG_HOOK_LED_BLINK:
		vrled_set_state(sc, LED1SB);
                break;
        case CONFIG_HOOK_LED_BLINK2:
		vrled_set_state(sc, LED2SB);
                break;
	default:
		vrled_set_state(sc, LEDOFF);
        }
        return (0);
}

/* end */
