/*	$NetBSD: ucb1200.c,v 1.1 2000/01/08 21:07:04 uch Exp $ */

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
 */
#define UCB1200DEBUG

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

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39sibvar.h>
#include <hpcmips/tx/tx39sibreg.h>
#include <hpcmips/tx/tx39icureg.h>

#include <hpcmips/tx/tx3912videovar.h> /* debug */

#include <hpcmips/dev/ucb1200var.h>
#include <hpcmips/dev/ucb1200reg.h>

#ifdef UCB1200DEBUG
int	ucb1200_debug = 1;
#define	DPRINTF(arg) if (ucb1200_debug) printf arg;
#define	DPRINTFN(n, arg) if (ucb1200_debug > (n)) printf arg;
#else
#define	DPRINTF(arg)
#endif

int	ucb1200_match	__P((struct device*, struct cfdata*, void*));
void	ucb1200_attach	__P((struct device*, struct device*, void*));
int	ucb1200_idcheck __P((bus_space_tag_t));

void	ucb1200_dump __P((struct ucb1200_softc*));
int	ucb1200_sibintr	__P((void*));
int	ucb1200_poll __P((void*));

int	ucb1200_adc_async __P((void*));
int	ucb1200_input __P((struct ucb1200_softc*));

void	ucb1200_intr_ack_sync __P((struct ucb1200_softc*));
int	ucb1200_adc_sync __P((struct ucb1200_softc*, int, int*));

int	ucb_ts_enable __P((void*));
int	ucb_ts_ioctl __P((void*, u_long, caddr_t, int, struct proc*));
void	ucb_ts_disable __P((void*));

/* mra is defined in mra.c */
int mra_Y_AX1_BX2_C __P((int *y, int ys, int *x1, int x1s, int *x2, int x2s,
			 int n, int scale, int *a, int *b, int *c));

struct cfattach ucb_ca = {
	sizeof(struct ucb1200_softc), ucb1200_match, ucb1200_attach
};

const struct wsmouse_accessops ucb_ts_accessops = {
	ucb_ts_enable,
	ucb_ts_ioctl,
	ucb_ts_disable,
};

/* 
 * XXX currently no calibration method. this is temporary hack.
 */
#include <machine/platid.h>
#define NSAMPLE	5

struct calibration_sample *calibration_sample_lookup __P((void));
int	ucb1200_tp_calibration __P((struct ucb1200_softc*));

struct calibration_sample {
	int cs_xraw, cs_yraw, cs_x, cs_y;	
};

struct calibration_sample_table {
	platid_t	cst_platform;
	struct calibration_sample cst_sample[NSAMPLE];
} calibration_sample_table[] = {
	{{{PLATID_WILD, PLATID_MACH_COMPAQ_C_8XX}},  /* uch machine */
	 {{ 507, 510, 320, 120 },
	  { 898, 757,  40,  40 },
	  { 900, 255,  40, 200 },
	  { 109, 249, 600, 200 },
	  { 110, 753, 600,  40 }}},
	
	{{{PLATID_WILD, PLATID_MACH_COMPAQ_C_2010}}, /* uch machine */
	 {{ 506, 487, 320, 120 },
	  { 880, 250,  40,  40 },
	  { 880, 718,  40, 200 },
	  { 140, 726, 600, 200 },
	  { 137, 250, 600,  40 }}},
	 
	{{{PLATID_WILD, PLATID_MACH_SHARP_MOBILON_HC4100}}, /* uch machine */
	 {{ 497, 501, 320, 120 },
	  { 752, 893,  40,  40 },
	  { 242, 891,  40, 200 },
	  { 241, 115, 600, 200 },
	  { 747, 101, 600,  40 }}},
	
	{{{PLATID_UNKNOWN, PLATID_UNKNOWN}}, 
	 {{0, 0, 0, 0}, 
	  {0, 0, 0, 0}, 
	  {0, 0, 0, 0}, 
	  {0, 0, 0, 0}, 
	  {0, 0, 0, 0}}},
};

struct calibration_sample*
calibration_sample_lookup()
{
	struct calibration_sample_table *tab;
	platid_mask_t mask;
	
	for (tab = calibration_sample_table; 
	     tab->cst_platform.dw.dw1 != PLATID_UNKNOWN; tab++) {

		mask = PLATID_DEREF(&tab->cst_platform);		
		
		if (platid_match(&platid, &mask)) {
			return tab->cst_sample;
		}
	}
	
	return 0;
}

int
ucb1200_tp_calibration(sc)
	struct ucb1200_softc *sc;
{
#define SCALE	(1024*1024)
	struct calibration_sample *cs;
	int s, n;

	tx3912video_calibration_pattern();

	sc->sc_prmxs = bootinfo->fb_width;
	sc->sc_prmys = bootinfo->fb_height;
	sc->sc_maxx = bootinfo->fb_width - 1;
	sc->sc_maxy = bootinfo->fb_height - 1;

	if (!(cs = calibration_sample_lookup())) {
		printf(": no calibration data\n");
		return 1;
	}

	s = sizeof(struct calibration_sample);
	n = NSAMPLE;
	
	if (mra_Y_AX1_BX2_C(&cs->cs_x, s, &cs->cs_xraw, s, &cs->cs_yraw, s,
			    n, SCALE, &sc->sc_prmax, &sc->sc_prmbx,
			    &sc->sc_prmcx) ||
	    mra_Y_AX1_BX2_C(&cs->cs_y, s, &cs->cs_xraw, s, &cs->cs_yraw, s,
			    n, SCALE, &sc->sc_prmay,
			    &sc->sc_prmby, &sc->sc_prmcy)) {
		printf(": MRA error");

		return 1;
	} else {
		DPRINTF((": Ax=%d Bx=%d Cx=%d",
			 sc->sc_prmax, sc->sc_prmbx, sc->sc_prmcx));
		DPRINTF((" Ay=%d By=%d Cy=%d\n",
			 sc->sc_prmay, sc->sc_prmby, sc->sc_prmcy));
	}
	
	return 0;
}

int
ucb1200_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct txsib_attach_args *sa = aux;
	
	if (sa->sa_slot != 0) /* UCB1200 must be subframe 0 */
		return 0;

	return txsibsf0_reg_read(sa->sa_tc, UCB1200_ID_REG) == UCB1200_ID 
		? 1 : 0;
}

void
ucb1200_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsib_attach_args *sa = aux;
	struct ucb1200_softc *sc = (void*)self;
	struct wsmousedev_attach_args wsmaa;

	sc->sc_tc = sa->sa_tc;
	sc->sc_parent = parent;
	
	tx_intr_establish(sc->sc_tc, 
			  MAKEINTR(1, TX39_INTRSTATUS1_SIBIRQPOSINT),
			  IST_EDGE, IPL_TTY, ucb1200_sibintr, sc);

	ucb1200_tp_calibration(sc);
#ifdef UCB1200DEBUG	
	ucb1200_dump(sc);
#endif

	wsmaa.accessops = &ucb_ts_accessops;
	wsmaa.accesscookie = sc;

	printf("\n");

	/*
	 * attach the wsmouse
	 */
	sc->sc_wsmousedev = config_found(self, &wsmaa, wsmousedevprint);
}

int
ucb1200_poll(arg)
	void *arg;
{
	struct ucb1200_softc *sc = arg;	
	
	if (sc->sm_state != UCBADC_IDLE) {
		printf("%s: %s busy\n", sc->sc_dev.dv_xname, 
		       sc->sc_parent->dv_xname);
		return POLL_CONT;
	}

	if (sc->sc_polling_finish) {
		sc->sc_polling_finish = 0;
		return POLL_END;
	}
	
	/* execute A-D converter */
	sc->sm_state = UCBADC_ADC_INIT;
	ucb1200_adc_async(sc);

	return POLL_CONT;
}

#define REGWRITE(addr, reg, ret) ( \
	sc->sm_addr = (addr), \
	sc->sm_reg = (reg), \
	sc->sm_returnstate = (ret),\
	sc->sm_state = UCBADC_REGWRITE)	
#define REGREAD(addr, ret) ( \
	sc->sm_addr = (addr), \
	sc->sm_returnstate = (ret), \
	sc->sm_state = UCBADC_REGREAD)

int
ucb1200_adc_async(arg)
	void *arg;
{
	struct ucb1200_softc *sc = arg;		
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;
	u_int16_t reg16;

	DPRINTFN(9, ("state: %d\n", sc->sm_state));
	
	switch (sc->sm_state) {
	default:
		panic("ucb1200_adc: invalid state %d", sc->sm_state);
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
			IST_EDGE, IPL_TTY, ucb1200_adc_async, sc);

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
		sc->sm_retry = 10;
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
		if (ucb1200_input(sc) == 0)
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
			sc->sm_rw_retry = 3;
			sc->sm_read_state = TXSIB_REGREAD_READ;
			break;
		case TXSIB_REGREAD_READ:
			reg = tx_conf_read(tc, TX39_SIBSF0STAT_REG);
			if ((TX39_SIBSF0_REGADDR(reg) != sc->sm_addr) &&
			    --sc->sm_rw_retry > 0) {
				printf("retry!\n");
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

	return 0;
}

int
ucb1200_input(sc)
	struct ucb1200_softc *sc;
{
	static int _x, _y;
	int xraw, yraw, pval, x, y;

	xraw = sc->sc_x;
	yraw = sc->sc_y;
	pval = sc->sc_p;

	x = (sc->sc_prmax * xraw + sc->sc_prmbx*yraw) / SCALE + 
		sc->sc_prmcx;
	y = (sc->sc_prmay * xraw + sc->sc_prmby*yraw) / SCALE + 
		sc->sc_prmcy;

	if (pval < UCBTS_PRESS_THRESHOLD) {
		sc->sc_stat = UCBTS_STAT_RELEASE;
		if (sc->sc_polling < UCBTS_TAP_THRESHOLD) {
			DPRINTFN(1, ("TAP!\n"));
			/* button 0 DOWN */
			wsmouse_input(sc->sc_wsmousedev, 1, 0, 0, 0, 0);
			/* button 0 UP */
			wsmouse_input(sc->sc_wsmousedev, 0, 0, 0, 0, 0);
		} else {
			wsmouse_input(sc->sc_wsmousedev, 0, _x, _y, 0, 
				      WSMOUSE_INPUT_ABSOLUTE_X |
				      WSMOUSE_INPUT_ABSOLUTE_Y);

			DPRINTFN(1, ("RELEASE\n"));
		}
		sc->sc_polling = 0;

		return 1;
	}
	DPRINTFN(1, ("xraw=%d yraw=%d pressure=%d\n", xraw, yraw, pval));
	
	if (x < 0 || x > sc->sc_maxx || y < 0 || y > sc->sc_maxy)
		return 0;

	if (sc->sc_polling == 1)
		tx3912video_dot(_x = x, _y = y);
	else
		tx3912video_line(_x, _y, _x = x, _y = y);
	
	wsmouse_input(sc->sc_wsmousedev, 1, x, y, 0, 
		      WSMOUSE_INPUT_ABSOLUTE_X |
		      WSMOUSE_INPUT_ABSOLUTE_Y);
	
	return 0;
}

int
ucb1200_sibintr(arg)
	void *arg;
{
	struct ucb1200_softc *sc = arg;
	
	sc->sc_stat = UCBTS_STAT_TOUCH;

	/* invoke touch screen polling */
	if (!sc->sc_polling) {
		sc->sc_pollh = tx39_poll_establish(sc->sc_tc, 1, IST_EDGE,
						   ucb1200_poll, sc);
		if (!sc->sc_pollh) {
			printf("%s: can't poll\n", sc->sc_dev.dv_xname);
		}
	}

	/* don't acknoledge interrupt until polling finish */
	
	return 0;
}

/*
 * sync functions. don't use runtime.
 */

void
ucb1200_intr_ack_sync(sc)
	struct ucb1200_softc *sc;	
{
	tx_chipset_tag_t tc = sc->sc_tc;
	u_int16_t reg;
	
	/* clear interrupt */
	reg = txsibsf0_reg_read(tc, UCB1200_INTSTAT_REG);
	txsibsf0_reg_write(tc, UCB1200_INTSTAT_REG, reg);
	txsibsf0_reg_write(tc, UCB1200_INTSTAT_REG, 0);
}

int
ucb1200_adc_sync(sc, src, valp)
	struct ucb1200_softc *sc;
	int src;
	int *valp;
{
	tx_chipset_tag_t tc = sc->sc_tc;
	u_int16_t reg;
	int i = 100; /* retry max */

	reg = UCB1200_ADCCTRL_ENABLE;

	switch (src) {
	case UCBTS_POSX:
		txsibsf0_reg_write(tc, UCB1200_TSCTRL_REG,
				   UCB1200_TSCTRL_XPOSITION);
		reg = UCB1200_ADCCTRL_INPUT_SET(reg, 
						UCB1200_ADCCTRL_INPUT_TSPX);
		break;
	case UCBTS_POSY:
		txsibsf0_reg_write(tc, UCB1200_TSCTRL_REG,
				   UCB1200_TSCTRL_YPOSITION);
		reg = UCB1200_ADCCTRL_INPUT_SET(reg, 
						UCB1200_ADCCTRL_INPUT_TSPY);
		break;
	case UCBTS_PRESS:
		txsibsf0_reg_write(tc, UCB1200_TSCTRL_REG,
				   UCB1200_TSCTRL_PRESSURE);
		reg = UCB1200_ADCCTRL_INPUT_SET(reg, 
						UCB1200_ADCCTRL_INPUT_TSPX);
		break;
	}
	/* enable ADC */
	txsibsf0_reg_write(tc, UCB1200_ADCCTRL_REG, reg);

	/* start A-D convert */
	txsibsf0_reg_write(tc, UCB1200_ADCCTRL_REG, 
			   reg | UCB1200_ADCCTRL_START);
	txsibsf0_reg_write(tc, UCB1200_ADCCTRL_REG, reg);

	/* inquire converted value */
	do {
		reg = txsibsf0_reg_read(tc, UCB1200_ADCDATA_REG);
	} while (!(reg & UCB1200_ADCDATA_INPROGRESS) && --i > 0);

	/* disable ADC */
	txsibsf0_reg_write(tc, UCB1200_ADCCTRL_REG, 0);

	/* turn to interrupt mode */
	txsibsf0_reg_write(tc, UCB1200_TSCTRL_REG,
			   UCB1200_TSCTRL_INTERRUPT);

	if (i == 0) {
		printf("ADC not complete.\n");
		return 1;
	} else {
		*valp = UCB1200_ADCDATA(reg);
	}

	return 0;
}

/*
 * access ops.
 */

int
ucb_ts_enable(v)
	void *v;
{
	/* not yet */
	return 0;
}

void
ucb_ts_disable(v)
	void *v;
{
	/* not yet */
}

int
ucb_ts_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	/* not yet */
	return 0;
}

void
ucb1200_dump(sc)
	struct ucb1200_softc *sc;
{
        const char *regname[] = {
                "IO_DATA        ",
                "IO_DIR         ",
                "POSINTEN       ",
                "NEGINTEN       ",
                "INTSTAT        ",
                "TELECOMCTRLA   ",
                "TELECOMCTRLB   ",
                "AUDIOCTRLA     ",
                "AUDIOCTRLB     ",
                "TOUCHSCREENCTRL",
                "ADCCTRL        ",
                "ADCDATA        ",
                "ID             ",
                "MODE           ",
                "RESERVED       ",
                "NULL           "
        };
	tx_chipset_tag_t tc = sc->sc_tc;
	u_int16_t reg;
	int i;

	for (i = 0; i < 16; i++) {
		reg = txsibsf0_reg_read(tc, i);
		printf("%s(%02d) 0x%04x ", regname[i], i, reg);
		bitdisp(reg);
	}
}
