/*
 * PIT port addresses and speaker control values
 *
 *	$Id: spkr_reg.h,v 1.2.2.2 1993/12/02 05:09:49 mycroft Exp $
 */

#define PITAUX_PORT	0x61	/* port of Programmable Peripheral Interface */
#define PIT_ENABLETMR2	0x01	/* Enable timer/counter 2 */
#define PIT_SPKRDATA	0x02	/* Direct to speaker */

#define PIT_SPKR	(PIT_ENABLETMR2|PIT_SPKRDATA)
#define	PIT_MODE	(TIMER_SEL2|TIMER_16BIT|TIMER_SQWAVE)
