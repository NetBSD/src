/*	$NetBSD: ucbtp.c,v 1.4 2000/05/22 17:17:45 uch Exp $ */

/*-
 * Copyright (c) 2000 UCHIYAMA Yasushi.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * Device driver for PHILIPS UCB1200 Advanced modem/audio analog front-end
 *	Touch panel part.
 */
#define UCBTPDEBUG

#include "opt_tx39_debug.h"
#include "opt_use_poll.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/bootinfo.h> /* bootinfo */

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <hpcmips/dev/tpcalibvar.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39sibvar.h>
#include <hpcmips/tx/tx39sibreg.h>
#include <hpcmips/tx/tx39icureg.h>

#include <hpcmips/dev/ucb1200var.h>
#include <hpcmips/dev/ucb1200reg.h>

#include <hpcmips/tx/txsnd.h>
#include <hpcmips/dev/video_subr.h> /* debug */

#ifdef UCBTPDEBUG
int	ucbtp_debug = 0;
#define	DPRINTF(arg) if (ucbtp_debug) printf arg;
#define	DPRINTFN(n, arg) if (ucbtp_debug > (n)) printf arg;
#else
#define	DPRINTF(arg)
#define DPRINTFN(n, arg)
#endif

enum ucbts_stat {
	UCBTS_STAT_DISABLE,
	UCBTS_STAT_RELEASE,
	UCBTS_STAT_TOUCH,
	UCBTS_STAT_DRAG,
};

#define UCBTS_POSX	1
#define UCBTS_POSY	2
#define UCBTS_PRESS	3

#define UCBTS_PRESS_THRESHOLD	80
#define UCBTS_TAP_THRESHOLD	5

enum ucbadc_state {
/* 0 */	UCBADC_IDLE,
/* 1 */	UCBADC_ADC_INIT,
/* 2 */	UCBADC_ADC_FINI,
/* 3 */	UCBADC_MEASUMENT_INIT,
/* 4 */	UCBADC_MEASUMENT_FINI,
/* 5 */	UCBADC_ADC_ENABLE,
/* 6 */	UCBADC_ADC_START0,
/* 7 */	UCBADC_ADC_START1,
/* 8 */	UCBADC_ADC_DATAREAD,
/* 9 */	UCBADC_ADC_DATAREAD_WAIT,
/*10 */	UCBADC_ADC_DISABLE,
/*11 */	UCBADC_ADC_INTRMODE,
/*12 */	UCBADC_ADC_INPUT,
/*13 */	UCBADC_INTR_ACK0,
/*14 */	UCBADC_INTR_ACK1,
/*15 */	UCBADC_INTR_ACK2,
/*16 */	UCBADC_REGREAD,
/*17 */	UCBADC_REGWRITE
};

struct ucbtp_softc {
	struct device sc_dev;
	struct device *sc_sib; /* parent (TX39 SIB module) */
	struct device *sc_ucb; /* parent (UCB1200 module) */
	tx_chipset_tag_t sc_tc;

	enum ucbts_stat sc_stat;
	int sc_polling;
	int sc_polling_finish;
	void *sc_pollh;

	struct tpcalib_softc sc_tpcalib;
	int sc_calibrated;

	/* measurement value */
	int sc_x, sc_y, sc_p;
	int sc_ox, sc_oy;
	int sc_xy_reverse; /* some platform pin connect interchanged */

	/* 
	 * touch panel state machine 
	 */
	void *sm_ih; /* TX39 SIB subframe 0 interrupt handler */

	int sm_addr; /* UCB1200 register address */
	u_int32_t sm_reg;  /* UCB1200 register data & TX39 SIB header */
	int sm_tmpreg;
#define UCBADC_RETRY_DEFAULT		200
	int sm_retry; /* retry counter */

	enum ucbadc_state sm_state;
	int		sm_measurement; /* X, Y, Pressure */
#define	UCBADC_MEASUREMENT_X		0
#define	UCBADC_MEASUREMENT_Y		1
#define	UCBADC_MEASUREMENT_PRESSURE	2
	int sm_returnstate;

	int sm_read_state, sm_write_state;
	int sm_writing;	/* writing state flag */
	u_int32_t sm_write_val;	/* temporary buffer */

	int sm_rw_retry; /* retry counter for r/w */

	/* wsmouse */
	struct device *sc_wsmousedev;
};

int	ucbtp_match	__P((struct device*, struct cfdata*, void*));
void	ucbtp_attach	__P((struct device*, struct device*, void*));

int	ucbtp_sibintr	__P((void*));
int	ucbtp_poll __P((void*));
int	ucbtp_adc_async __P((void*));
int	ucbtp_input __P((struct ucbtp_softc*));
int	ucbtp_busy __P((void*));

int	ucbtp_enable __P((void*));
int	ucbtp_ioctl __P((void*, u_long, caddr_t, int, struct proc*));
void	ucbtp_disable __P((void*));

struct cfattach ucbtp_ca = {
	sizeof(struct ucbtp_softc), ucbtp_match, ucbtp_attach
};

const struct wsmouse_accessops ucbtp_accessops = {
	ucbtp_enable,
	ucbtp_ioctl,
	ucbtp_disable,
};

/* 
 * XXX currently no calibration method. this is temporary hack.
 */
#include <machine/platid.h>

struct	wsmouse_calibcoords *calibration_sample_lookup __P((void));
int	ucbtp_calibration __P((struct ucbtp_softc*));

struct calibration_sample_table {
	platid_t	cst_platform;
	struct wsmouse_calibcoords cst_sample;
} calibration_sample_table[] = {
	{{{PLATID_WILD, PLATID_MACH_COMPAQ_C_8XX}},  /* uch machine */
	 { 0, 0, 639, 239, 5,
	   {{ 507, 510, 320, 120 },
	    { 898, 757,  40,  40 },
	    { 900, 255,  40, 200 },
	    { 109, 249, 600, 200 },
	    { 110, 753, 600,  40 }}}},
	
	{{{PLATID_WILD, PLATID_MACH_COMPAQ_C_2010}}, /* uch machine */
	 { 0, 0, 639, 239, 5,
	   {{ 506, 487, 320, 120 },
	    { 880, 250,  40,  40 },
	    { 880, 718,  40, 200 },
	    { 140, 726, 600, 200 },
	    { 137, 250, 600,  40 }}}},
	 
	{{{PLATID_WILD, PLATID_MACH_SHARP_MOBILON_HC4100}}, /* uch machine */
	 { 0, 0, 639, 239, 5,
	   {{ 497, 501, 320, 120 },
	    { 752, 893,  40,  40 },
	    { 242, 891,  40, 200 },
	    { 241, 115, 600, 200 },
	    { 747, 101, 600,  40 }}}},
	
	{{{PLATID_WILD, PLATID_MACH_SHARP_TELIOS_HCAJ1}}, /* uch machine */
	 { 0, 0, 799, 479, 5,
	   {{ 850, 150,   1,   1 },
	    { 850, 880,   1, 479 },
	    { 850, 880,   1, 479 },
	    {  85, 880, 799, 479 },
	    {  85, 150, 799,   1 }}}},

	{{{PLATID_UNKNOWN, PLATID_UNKNOWN}}, 
	 { 0, 0, 639, 239, 5,
	   {{0, 0, 0, 0}, 
	    {0, 0, 0, 0}, 
	    {0, 0, 0, 0}, 
	    {0, 0, 0, 0}, 
	    {0, 0, 0, 0}}}},
};

struct wsmouse_calibcoords *
calibration_sample_lookup()
{
	struct calibration_sample_table *tab;
	platid_mask_t mask;
	
	for (tab = calibration_sample_table; 
	     tab->cst_platform.dw.dw1 != PLATID_UNKNOWN; tab++) {

		mask = PLATID_DEREF(&tab->cst_platform);		
		
		if (platid_match(&platid, &mask)) {
			return (&tab->cst_sample);
		}
	}
	
	return (0);
}

int
ucbtp_calibration(sc)
	struct ucbtp_softc *sc;
{
	struct wsmouse_calibcoords *cs;

	if (sc->sc_tc->tc_videot)
		video_calibration_pattern(sc->sc_tc->tc_videot); /* debug */

	tpcalib_init(&sc->sc_tpcalib);

	if (!(cs = calibration_sample_lookup())) {
		DPRINTF(("no calibration data"));
		return (1);
	}

	sc->sc_calibrated = 
		tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
			      (caddr_t)cs, 0, 0) == 0 ? 1 : 0;

	if (!sc->sc_calibrated)
		printf("not ");
	printf("calibrated");

	return (0);
}

int
ucbtp_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (1);
}

void
ucbtp_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ucb1200_attach_args *ucba = aux;
	struct ucbtp_softc *sc = (void*)self;
	struct wsmousedev_attach_args wsmaa;
	tx_chipset_tag_t tc;

	tc = sc->sc_tc = ucba->ucba_tc;
	sc->sc_sib = ucba->ucba_sib;
	sc->sc_ucb = ucba->ucba_ucb;
	
	printf(": ");
	/* touch panel interrupt */
	tx_intr_establish(tc, MAKEINTR(1, TX39_INTRSTATUS1_SIBIRQPOSINT),
			  IST_EDGE, IPL_TTY, ucbtp_sibintr, sc);
	
	/* attempt to calibrate touch panel */
	ucbtp_calibration(sc);
#ifdef TX392X /* hack for Telios HC-VJ1C */
	sc->sc_xy_reverse = 1;
#endif	

	printf("\n");

	wsmaa.accessops = &ucbtp_accessops;
	wsmaa.accesscookie = sc;

	ucb1200_state_install(parent, ucbtp_busy, self, UCB1200_TP_MODULE);
			      
	/*
	 * attach the wsmouse
	 */
	sc->sc_wsmousedev = config_found(self, &wsmaa, wsmousedevprint);
}

int
ucbtp_busy(arg)
	void *arg;
{
	struct ucbtp_softc *sc = arg;
	
	return (sc->sm_state != UCBADC_IDLE);
}

int
ucbtp_poll(arg)
	void *arg;
{
	struct ucbtp_softc *sc = arg;	
	
	if (!ucb1200_state_idle(sc->sc_ucb)) /* subframe0 busy */
		return (POLL_CONT);

	if (sc->sc_polling_finish) {
		sc->sc_polling_finish = 0;
		return (POLL_END);
	}
	
	/* execute A-D converter */
	sc->sm_state = UCBADC_ADC_INIT;
	ucbtp_adc_async(sc);

	return (POLL_CONT);
}

int
ucbtp_sibintr(arg)
	void *arg;
{
	struct ucbtp_softc *sc = arg;
	
	sc->sc_stat = UCBTS_STAT_TOUCH;

	/* click! */
	tx_sound_click(sc->sc_tc);

	/* invoke touch panel polling */
	if (!sc->sc_polling) {
		sc->sc_pollh = tx39_poll_establish(sc->sc_tc, 1, IST_EDGE,
						   ucbtp_poll, sc);
		if (!sc->sc_pollh) {
			printf("%s: can't poll\n", sc->sc_dev.dv_xname);
		}
	}

	/* don't acknoledge interrupt until polling finish */
	
	return (0);
}

#define REGWRITE(addr, reg, ret) (					\
	sc->sm_addr = (addr),						\
	sc->sm_reg = (reg),						\
	sc->sm_returnstate = (ret),					\
	sc->sm_state = UCBADC_REGWRITE)	
#define REGREAD(addr, ret) (						\
	sc->sm_addr = (addr),						\
	sc->sm_returnstate = (ret),					\
	sc->sm_state = UCBADC_REGREAD)

int
ucbtp_adc_async(arg)
	void *arg;
{
	struct ucbtp_softc *sc = arg;		
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;
	u_int16_t reg16;

	DPRINTFN(9, ("state: %d\n", sc->sm_state));

	switch (sc->sm_state) {
	default:
		panic("ucbtp_adc: invalid state %d", sc->sm_state);
		/* NOTREACHED */
		break;

	case UCBADC_IDLE:
		/* nothing to do */
		break;		

	case UCBADC_ADC_INIT:
		sc->sc_polling++;
		sc->sc_stat = UCBTS_STAT_DRAG;
		/* enable heart beat of this state machine */
		sc->sm_ih = tx_intr_establish(
			tc,
			MAKEINTR(1, TX39_INTRSTATUS1_SIBSF0INT),
			IST_EDGE, IPL_TTY, ucbtp_adc_async, sc);

		sc->sm_state = UCBADC_MEASUMENT_INIT;
		break;

	case UCBADC_ADC_FINI:
		/* disable heart beat of this state machine */
		tx_intr_disestablish(tc, sc->sm_ih);
		sc->sm_state = UCBADC_IDLE;
		break;

	case UCBADC_MEASUMENT_INIT:
		switch (sc->sm_measurement) {
		default:
			panic("unknown measurement spec.");
			/* NOTREACHED */
			break;
		case UCBADC_MEASUREMENT_X:
			REGWRITE(UCB1200_TSCTRL_REG,
				 UCB1200_TSCTRL_XPOSITION,
				 UCBADC_ADC_ENABLE);
			break;
		case UCBADC_MEASUREMENT_Y:			
			REGWRITE(UCB1200_TSCTRL_REG,
				 UCB1200_TSCTRL_YPOSITION,
				 UCBADC_ADC_ENABLE);
			break;
		case UCBADC_MEASUREMENT_PRESSURE:
			REGWRITE(UCB1200_TSCTRL_REG,
				 UCB1200_TSCTRL_PRESSURE,
				 UCBADC_ADC_ENABLE);
			break;
		}
		break;		

	case UCBADC_MEASUMENT_FINI:
		switch (sc->sm_measurement) {
		case UCBADC_MEASUREMENT_X:
			sc->sm_measurement = UCBADC_MEASUREMENT_Y;
			sc->sm_state = UCBADC_MEASUMENT_INIT;
			break;
		case UCBADC_MEASUREMENT_Y:			
			sc->sm_measurement = UCBADC_MEASUREMENT_PRESSURE;
			sc->sm_state = UCBADC_MEASUMENT_INIT;
			break;
		case UCBADC_MEASUREMENT_PRESSURE:
			sc->sm_measurement = UCBADC_MEASUREMENT_X;
			/* measument complete. pass down to wsmouse_input */
			sc->sm_state = UCBADC_ADC_INPUT;
			break;
		}
		break;		

	case UCBADC_ADC_ENABLE:
		switch (sc->sm_measurement) {
		case UCBADC_MEASUREMENT_PRESSURE:
			/* FALLTHROUGH */
		case UCBADC_MEASUREMENT_X:
			sc->sm_tmpreg = UCB1200_ADCCTRL_INPUT_SET(
				UCB1200_ADCCTRL_ENABLE,
				UCB1200_ADCCTRL_INPUT_TSPX);
			REGWRITE(UCB1200_ADCCTRL_REG, sc->sm_tmpreg, 
				 UCBADC_ADC_START0);
			break;
		case UCBADC_MEASUREMENT_Y:			
			sc->sm_tmpreg = UCB1200_ADCCTRL_INPUT_SET(
				UCB1200_ADCCTRL_ENABLE,
				UCB1200_ADCCTRL_INPUT_TSPY);
			REGWRITE(UCB1200_ADCCTRL_REG, sc->sm_tmpreg, 
				 UCBADC_ADC_START0);
			break;
		}
		break;	

	case UCBADC_ADC_START0:
		REGWRITE(UCB1200_ADCCTRL_REG, 
			 sc->sm_tmpreg | UCB1200_ADCCTRL_START,
			 UCBADC_ADC_START1);
		break;		

	case UCBADC_ADC_START1:
		REGWRITE(UCB1200_ADCCTRL_REG, 
			 sc->sm_tmpreg,
			 UCBADC_ADC_DATAREAD);
		sc->sm_retry = UCBADC_RETRY_DEFAULT;
		break;

	case UCBADC_ADC_DATAREAD:
		REGREAD(UCB1200_ADCDATA_REG, UCBADC_ADC_DATAREAD_WAIT);
		break;

	case UCBADC_ADC_DATAREAD_WAIT:
		reg16 = TX39_SIBSF0_REGDATA(sc->sm_reg);
		if (!(reg16 & UCB1200_ADCDATA_INPROGRESS) && 
		    --sc->sm_retry > 0) {
			sc->sm_state = UCBADC_ADC_DATAREAD;
		} else {
			if (sc->sm_retry <= 0) {
				printf("dataread failed\n");
				sc->sm_state = UCBADC_ADC_FINI;
				break;
			}

			switch (sc->sm_measurement) {
			case UCBADC_MEASUREMENT_X:
				sc->sc_x = UCB1200_ADCDATA(reg16);
				DPRINTFN(9, ("x=%d\n", sc->sc_x));
				break;
			case UCBADC_MEASUREMENT_Y:			
				sc->sc_y = UCB1200_ADCDATA(reg16);
				DPRINTFN(9, ("y=%d\n", sc->sc_y));
				break;
			case UCBADC_MEASUREMENT_PRESSURE:
				sc->sc_p = UCB1200_ADCDATA(reg16);
				DPRINTFN(9, ("p=%d\n", sc->sc_p));
				break;
			}
			
			sc->sm_state = UCBADC_ADC_DISABLE;
		}

		break;

	case UCBADC_ADC_DISABLE:
		REGWRITE(UCB1200_ADCCTRL_REG, 0, UCBADC_ADC_INTRMODE);

		break;
	case UCBADC_ADC_INTRMODE:
		REGWRITE(UCB1200_TSCTRL_REG, UCB1200_TSCTRL_INTERRUPT,
			 UCBADC_MEASUMENT_FINI);
		break;

	case UCBADC_ADC_INPUT:
		if (ucbtp_input(sc) == 0)
			sc->sm_state = UCBADC_ADC_FINI;
		else 
			sc->sm_state = UCBADC_INTR_ACK0;
		break;

	case UCBADC_INTR_ACK0:
		REGREAD(UCB1200_INTSTAT_REG, UCBADC_INTR_ACK1);
		break;

	case UCBADC_INTR_ACK1:
		REGWRITE(UCB1200_INTSTAT_REG, sc->sm_reg, UCBADC_INTR_ACK2);
		break;

	case UCBADC_INTR_ACK2:
		sc->sc_polling_finish = 1;
		REGWRITE(UCB1200_INTSTAT_REG, 0, UCBADC_ADC_FINI);
		break;

	/*
	 * UCB1200 register access state
	 */
	case UCBADC_REGREAD:
		/* 
		 * In	: sc->sm_addr
		 * Out	: sc->sm_reg  (with SIBtag)
		 */
#define TXSIB_REGREAD_INIT	0
#define TXSIB_REGREAD_READ	1
		switch (sc->sm_read_state) {
		case TXSIB_REGREAD_INIT:
			reg = TX39_SIBSF0_REGADDR_SET(0, sc->sm_addr);
			tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);
			sc->sm_rw_retry = UCBADC_RETRY_DEFAULT;
			sc->sm_read_state = TXSIB_REGREAD_READ;
			break;
		case TXSIB_REGREAD_READ:
			reg = tx_conf_read(tc, TX39_SIBSF0STAT_REG);
			if ((TX39_SIBSF0_REGADDR(reg) != sc->sm_addr) &&
			    --sc->sm_rw_retry > 0) {
				break;
			}

			if (sc->sm_rw_retry <= 0) {
				printf("sf0read: command failed\n");
				sc->sm_state = UCBADC_ADC_FINI;
			} else {
				sc->sm_reg = reg;
				sc->sm_read_state = TXSIB_REGREAD_INIT;
				DPRINTFN(9, ("%08x\n", reg));
				if (sc->sm_writing)
					sc->sm_state = UCBADC_REGWRITE;
				else
					sc->sm_state = sc->sm_returnstate;
			}
			break;
		}
		break;
		
	case UCBADC_REGWRITE:
		/*
		 * In	: sc->sm_addr, sc->sm_reg (lower 16bit only)
		 */
#define TXSIB_REGWRITE_INIT	0
#define TXSIB_REGWRITE_WRITE	1
		switch (sc->sm_write_state) {
		case TXSIB_REGWRITE_INIT:
			sc->sm_writing = 1;
			sc->sm_write_state = TXSIB_REGWRITE_WRITE;
			sc->sm_state = UCBADC_REGREAD;

			sc->sm_write_val = sc->sm_reg;
			break;
		case TXSIB_REGWRITE_WRITE:
			sc->sm_writing = 0;
			sc->sm_write_state = TXSIB_REGWRITE_INIT;
			sc->sm_state = sc->sm_returnstate;

			reg = sc->sm_reg;
			reg |= TX39_SIBSF0_WRITE;
			TX39_SIBSF0_REGDATA_CLR(reg);
			reg = TX39_SIBSF0_REGDATA_SET(reg, sc->sm_write_val);
			tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);
			break;
		}
		break;
	}

	return (0);
}

int
ucbtp_input(sc)
	struct ucbtp_softc *sc;
{
	int rx, ry, x, y, p;

	rx = sc->sc_x;
	ry = sc->sc_y;
	p = sc->sc_p;

	if (!sc->sc_calibrated) {
		DPRINTFN(2, ("x=%4d y=%4d p=%4d\n", rx, ry, p));
		DPRINTF(("ucbtp_input: no calibration data\n"));
	}

	if (p < UCBTS_PRESS_THRESHOLD || rx == 0x3ff || ry == 0x3ff ||
	    rx == 0 || ry == 0) {
		sc->sc_stat = UCBTS_STAT_RELEASE;
		if (sc->sc_polling < UCBTS_TAP_THRESHOLD) {
			DPRINTFN(2, ("TAP!\n"));
			/* button 0 DOWN */
			wsmouse_input(sc->sc_wsmousedev, 1, 0, 0, 0, 0);
			/* button 0 UP */
			wsmouse_input(sc->sc_wsmousedev, 0, 0, 0, 0, 0);
		} else {
			wsmouse_input(sc->sc_wsmousedev, 0, 
				      sc->sc_ox, sc->sc_oy, 0, 
				      WSMOUSE_INPUT_ABSOLUTE_X |
				      WSMOUSE_INPUT_ABSOLUTE_Y);

			DPRINTFN(2, ("RELEASE\n"));
		}
		sc->sc_polling = 0;

		return (1);
	}

	if (sc->sc_xy_reverse)
		tpcalib_trans(&sc->sc_tpcalib, ry, rx, &x, &y);
	else
		tpcalib_trans(&sc->sc_tpcalib, rx, ry, &x, &y);
		
	DPRINTFN(2, ("x: %4d->%4d y: %4d->%4d pressure=%4d\n",
		     rx, x, ry, y, p));

	/* debug draw */	
	if (sc->sc_tc->tc_videot) {
		if (sc->sc_polling == 1)
			video_dot(sc->sc_tc->tc_videot, x, y);
		else 
			video_line(sc->sc_tc->tc_videot, sc->sc_ox,
				   sc->sc_oy, x, y);
	}

	sc->sc_ox = x, sc->sc_oy = y;

	wsmouse_input(sc->sc_wsmousedev, 1, x, y, 0, 
		      WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
	
	return (0);
}

/*
 * access ops.
 */

int
ucbtp_enable(v)
	void *v;
{
	/* not yet */
	return (0);
}

void
ucbtp_disable(v)
	void *v;
{
	/* not yet */
}

int
ucbtp_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct ucbtp_softc *sc = v;

	DPRINTF(("%s(%d): ucbtp_ioctl(%08lx)\n", __FILE__, __LINE__, cmd));

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_TPANEL;
		break;
		
	case WSMOUSEIO_SRES:
		printf("%s(%d): WSMOUSRIO_SRES is not supported",
		       __FILE__, __LINE__);
		break;

	case WSMOUSEIO_SCALIBCOORDS:
	case WSMOUSEIO_GCALIBCOORDS:
                return tpcalib_ioctl(&sc->sc_tpcalib, cmd, data, flag, p);
		
	default:
		return (-1);
	}

	return (0);
}
