/*	$NetBSD: ucbsnd.c,v 1.1 2000/01/12 14:56:21 uch Exp $ */

/*
 * Copyright (c) 2000, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 */

/*
 * Device driver for PHILIPS UCB1200 Advanced modem/audio analog front-end
 *	Audio codec part.
 */
#define UCBSNDDEBUG

#include "opt_tx39_debug.h"
#include "opt_use_poll.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39sibvar.h>
#include <hpcmips/tx/tx39sibreg.h>
#include <hpcmips/tx/tx39icureg.h>
#include <hpcmips/tx/txsnd.h>

#include <hpcmips/dev/ucb1200var.h>
#include <hpcmips/dev/ucb1200reg.h>

#ifdef UCBSNDDEBUG
int	ucbsnd_debug = 1;
#define	DPRINTF(arg) if (ucbsnd_debug) printf arg;
#define	DPRINTFN(n, arg) if (ucbsnd_debug > (n)) printf arg;
#else
#define	DPRINTF(arg)
#define DPRINTFN(n, arg)
#endif

enum ucbsnd_state {
/* 0 */	UCBSND_IDLE,
/* 1 */	UCBSND_INIT,
/* 2 */ UCBSND_ENABLE_SAMPLERATE,
/* 3 */ UCBSND_ENABLE_OUTPUTPATH,
/* 5 */ UCBSND_ENABLE_SPEAKER0,
/* 6 */ UCBSND_ENABLE_SPEAKER1,
/* 7 */ UCBSND_TRANSITION_PIO,
/* 8 */ UCBSND_PIO,
/* 9 */ UCBSND_TRANSITION_DISABLE,
/*10 */ UCBSND_DISABLE_OUTPUTPATH,
/*11 */ UCBSND_DISABLE_SPEAKER0,
/*12 */ UCBSND_DISABLE_SPEAKER1,
/*13 */	UCBSND_DISABLE_SIB
};

struct ucbsnd_softc {
	struct device		sc_dev;
	struct device		*sc_sib; /* parent (TX39 SIB module) */
	struct device		*sc_ucb; /* parent (UCB1200 module) */
	tx_chipset_tag_t	sc_tc;

	struct	tx_sound_tag sc_tag;

	/* 
	 *  audio codec state machine 
	 */
	enum ucbsnd_state sa_state;

	int		sa_snd_rate; /* passed down from SIB module */
	int		sa_tel_rate;
	int		sa_enabled;
	void*		sa_sf0ih;
	void*		sa_sndih;
	int		sa_retry;
	int		sa_cnt; /* misc counter */
	
};

int	ucbsnd_match	__P((struct device*, struct cfdata*, void*));
void	ucbsnd_attach	__P((struct device*, struct device*, void*));

int	ucbsnd_exec_output	__P((void*));
int	ucbsnd_busy __P((void*));

void	ucbsnd_sound_init	__P((struct ucbsnd_softc*));
void	__ucbsnd_sound_click	__P((tx_sound_tag_t));

struct cfattach ucbsnd_ca = {
	sizeof(struct ucbsnd_softc), ucbsnd_match, ucbsnd_attach
};

int
ucbsnd_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
ucbsnd_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ucb1200_attach_args *ucba = aux;
	struct ucbsnd_softc *sc = (void*)self;
	tx_chipset_tag_t tc;

	tc = sc->sc_tc = ucba->ucba_tc;
	sc->sc_sib = ucba->ucba_sib;
	sc->sc_ucb = ucba->ucba_ucb;
#define SOUND_TEST
#ifdef SOUND_TEST
	ucbsnd_sound_init(sc);
#endif
	sc->sa_snd_rate = ucba->ucba_snd_rate;
	sc->sa_tel_rate = ucba->ucba_tel_rate;

#define KHZ(a) ((a) / 1000), (((a) % 1000))
	printf(": audio %d.%03d kHz telecom %d.%03d kHz",
	       KHZ((tx39sib_clock(sc->sc_sib) * 2) / 
		   (sc->sa_snd_rate * 64)), 
	       KHZ((tx39sib_clock(sc->sc_sib) * 2) / 
		   (sc->sa_tel_rate * 64)));

	ucb1200_state_install(parent, ucbsnd_busy, self, 
			      UCB1200_SND_MODULE);
	
	printf("\n");
}

int
ucbsnd_busy(arg)
	void *arg;
{
	struct ucbsnd_softc *sc = arg;
	
	return sc->sa_state != UCBSND_IDLE;
}

int
ucbsnd_exec_output(arg)
	void *arg;
{
	struct ucbsnd_softc *sc = arg;	
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;

	switch (sc->sa_state) {
	default:
		panic("ucbsnd_exec_output: invalid state %d", sc->sa_state);
		/* NOTREACHED */
		break;

	case UCBSND_IDLE:
		/* nothing to do */
		return 0;

	case UCBSND_INIT:
		sc->sa_sf0ih = tx_intr_establish(
			tc, MAKEINTR(1, TX39_INTRSTATUS1_SIBSF0INT),
			IST_EDGE, IPL_TTY, ucbsnd_exec_output, sc);

		sc->sa_state = UCBSND_ENABLE_SAMPLERATE;
		return 0;
		
	case UCBSND_ENABLE_SAMPLERATE:
		/* Enable UCB1200 side sample rate */
		reg = TX39_SIBSF0_WRITE;
		reg = TX39_SIBSF0_REGADDR_SET(reg, UCB1200_AUDIOCTRLA_REG);
		reg = TX39_SIBSF0_REGDATA_SET(reg, sc->sa_snd_rate);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		sc->sa_state = UCBSND_ENABLE_OUTPUTPATH;
		return 0;

	case UCBSND_ENABLE_OUTPUTPATH:
		/* Enable UCB1200 side */
		reg = TX39_SIBSF0_WRITE;
		reg = TX39_SIBSF0_REGADDR_SET(reg, UCB1200_AUDIOCTRLB_REG);
		reg = TX39_SIBSF0_REGDATA_SET(reg, 
					      UCB1200_AUDIOCTRLB_OUTEN);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		/* Enable SIB side */
		reg = tx_conf_read(tc, TX39_SIBCTRL_REG);	
		tx_conf_write(tc, TX39_SIBCTRL_REG, 
			      reg | TX39_SIBCTRL_ENSND);

		sc->sa_state = UCBSND_ENABLE_SPEAKER0;
		sc->sa_retry = 10;
		return 0;

	case UCBSND_ENABLE_SPEAKER0:
		/* Speaker on */

		reg = TX39_SIBSF0_REGADDR_SET(0, UCB1200_IO_DATA_REG);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		sc->sa_state = UCBSND_ENABLE_SPEAKER1;
		return 0;

	case UCBSND_ENABLE_SPEAKER1:
		reg = tx_conf_read(tc, TX39_SIBSF0STAT_REG);
		if ((TX39_SIBSF0_REGADDR(reg) != UCB1200_IO_DATA_REG) && 
		    --sc->sa_retry > 0) {

			sc->sa_state = UCBSND_ENABLE_SPEAKER0;
			return 0;
		}
		
		if (sc->sa_retry <= 0) {
			printf("ucbsnd_exec_output: subframe0 busy\n");
			
			sc->sa_state = UCBSND_IDLE;
			return 0;
		}

		reg |= TX39_SIBSF0_WRITE;		
		reg |= UCB1200_IO_DATA_SPEAKER;
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		sc->sa_state = UCBSND_TRANSITION_PIO;
		sc->sa_cnt = 0;
		return 0;

	case UCBSND_TRANSITION_PIO:
		/* change interrupt source */
		tx_intr_disestablish(tc, sc->sa_sf0ih);

		sc->sa_sndih = tx_intr_establish(
			tc, MAKEINTR(1, TX39_INTRSTATUS1_SNDININT),
			IST_EDGE, IPL_TTY, ucbsnd_exec_output, sc);
		sc->sa_enabled = 1;

		sc->sa_state = UCBSND_PIO;
		return 0;

	case UCBSND_PIO:
		sc->sa_cnt++;

		if ((sc->sa_cnt % 20) == 0)
			tx_conf_write(tc, TX39_SIBSNDHOLD_REG, sc->sa_cnt + 5000);
		else
			tx_conf_write(tc, TX39_SIBSNDHOLD_REG, 0);

		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, TX39_SIBSF0_SNDVALID);
		
		if (sc->sa_cnt > 1000)
			sc->sa_state = UCBSND_TRANSITION_DISABLE;
		
		return 0;

	case UCBSND_TRANSITION_DISABLE:
		/* change interrupt source */
		sc->sa_enabled = 0;
		tx_intr_disestablish(tc, sc->sa_sndih);
		sc->sa_sf0ih = tx_intr_establish(
			tc, MAKEINTR(1, TX39_INTRSTATUS1_SIBSF0INT),
			IST_EDGE, IPL_TTY, ucbsnd_exec_output, sc);
		
		sc->sa_state = UCBSND_DISABLE_OUTPUTPATH;
		return 0;
	
	case UCBSND_DISABLE_OUTPUTPATH:
		/* disable codec output path and mute */
		reg = TX39_SIBSF0_WRITE;
		reg = TX39_SIBSF0_REGADDR_SET(reg, UCB1200_AUDIOCTRLB_REG);
		reg = TX39_SIBSF0_REGDATA_SET(reg, UCB1200_AUDIOCTRLB_MUTE);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		sc->sa_state = UCBSND_DISABLE_SPEAKER0;
		sc->sa_retry = 10;
		return 0;

	case UCBSND_DISABLE_SPEAKER0:
		/* Speaker off */
		reg = TX39_SIBSF0_REGADDR_SET(0, UCB1200_IO_DATA_REG);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		sc->sa_state = UCBSND_DISABLE_SPEAKER1;
		return 0;
		
	case UCBSND_DISABLE_SPEAKER1:
		reg = tx_conf_read(tc, TX39_SIBSF0STAT_REG);
		if ((TX39_SIBSF0_REGADDR(reg) != UCB1200_IO_DATA_REG) && 
		    --sc->sa_retry > 0) {

			sc->sa_state = UCBSND_DISABLE_SPEAKER0;
			return 0;
		}
		
		if (sc->sa_retry <= 0) {
			printf("ucbsnd_exec_output: subframe0 busy\n");

			sc->sa_state = UCBSND_IDLE;
			return 0;
		}

		reg |= TX39_SIBSF0_WRITE;
		reg &= ~UCB1200_IO_DATA_SPEAKER;
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		sc->sa_state = UCBSND_DISABLE_SIB;
		return 0;

	case UCBSND_DISABLE_SIB:
		/* Disable SIB side */
		reg = tx_conf_read(tc, TX39_SIBCTRL_REG);	
		reg &= ~TX39_SIBCTRL_ENSND;
		tx_conf_write(tc, TX39_SIBCTRL_REG, reg);
		
		/* end audio disable sequence */
		tx_intr_disestablish(tc, sc->sa_sf0ih);
		sc->sa_state = UCBSND_IDLE;

		return 0;
	}

	return 0;
}

/*
 * global sound interface.
 */
void
ucbsnd_sound_init(sc)
	struct ucbsnd_softc *sc;
{
	tx_sound_tag_t ts = &sc->sc_tag;
	tx_chipset_tag_t tc = sc->sc_tc;

	ts->ts_v = sc;
	ts->ts_click = __ucbsnd_sound_click;

	tx_conf_register_sound(tc, ts);
}

void
__ucbsnd_sound_click(arg)
	tx_sound_tag_t arg;
{
	struct ucbsnd_softc *sc = (void*)arg;
	
	if (sc->sa_state == UCBSND_IDLE) {
		sc->sa_state = UCBSND_INIT;
		ucbsnd_exec_output((void*)sc);
	}
}


