/*
 * Keyboard definitions
 *
 *	$Id: kbdreg.h,v 1.1 1993/09/14 17:32:43 mycroft Exp $
 */

/* uses Intel 8042 controller */
#include "i386/isa/ic/i8042.h"

/* commands and responses */
#define	KBC_RESET	0xFF	/* Reset the keyboard */
#define	KBC_STSIND	0xED	/* set keyboard status indicators */
#define	KBR_OVERRUN	0xFE	/* Keyboard flooded */
#define	KBR_RESEND	0xFE	/* Keyboard needs resend of command */
#define	KBR_ACK		0xFA	/* Keyboard did receive command */
#define	KBR_RSTDONE	0xAA	/* Keyboard reset complete */
