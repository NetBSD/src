/*	$NetBSD: ucb1200var.h,v 1.1 2000/01/08 21:07:04 uch Exp $ */

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

enum ucbts_stat {
	UCBTS_STAT_DISABLE,
	UCBTS_STAT_RELEASE,
	UCBTS_STAT_TOUCH,
	UCBTS_STAT_DRAG,
};

#define UCBTS_POSX	1
#define UCBTS_POSY	2
#define UCBTS_PRESS	3

#define UCBTS_PRESS_THRESHOLD	100
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

struct ucb1200_softc {
	struct device		sc_dev;
	struct device		*sc_parent; /* parent (TX39 SIB module) */
	tx_chipset_tag_t	sc_tc;

	enum ucbts_stat sc_stat;
	int sc_polling;
	int sc_polling_finish;
	void *sc_pollh;

	/* correction parameters */
	int sc_prmax, sc_prmbx, sc_prmcx, sc_prmxs;
	int sc_prmay, sc_prmby, sc_prmcy, sc_prmys;
	/* limit */
	int sc_maxx, sc_maxy;

	/* measurement value */
	int sc_x, sc_y, sc_p;

	/* SIB frame 0 state machine */
	void		*sm_ih; /* TX39 SIB subframe 0 interrupt handler */

	int		sm_addr; /* UCB1200 register address */
	u_int32_t	sm_reg;  /* UCB1200 register data & TX39 SIB header */
	int		sm_tmpreg;
	int		sm_retry; /* retry counter */

	enum ucbadc_state sm_state;
	int		sm_measurement; /* X, Y, Pressure */
#define	UCBADC_MEASUREMENT_X		0
#define	UCBADC_MEASUREMENT_Y		1
#define	UCBADC_MEASUREMENT_PRESSURE	2
	int		sm_returnstate;

	int		sm_read_state, sm_write_state;
	int		sm_writing;	/* writing state flag */
	u_int32_t	sm_write_val;	/* temporary buffer */

	int		sm_rw_retry; /* retry counter for r/w */

	/* wsmouse */
	struct device *sc_wsmousedev;
};

