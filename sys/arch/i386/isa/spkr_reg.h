/*
 * PIT port addresses and speaker control values
 *
 *	$Id: spkr_reg.h,v 1.2.2.1 1993/11/25 20:16:32 mycroft Exp $
 */

#define PITAUX_PORT	0x61	/* port of Programmable Peripheral Interface */
#define PIT_ENABLETMR2	0x01	/* Enable timer/counter 2 */
#define PIT_SPKRDATA	0x02	/* Direct to speaker */

#define PIT_SPKR	(PIT_ENABLETMR2|PIT_SPKRDATA)
#define	PIT_MODE	(TIMER_SEL2|TIMER_MSB|TIMER_SQWAVE)
