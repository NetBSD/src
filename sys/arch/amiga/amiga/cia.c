/*
 *  this file provides an interface to CIA-generated interrupts.
 *  Since the interrupt control register of a CIA is cleared
 *  when it's read, it is essential that different interrupt
 *  sources are managed from one central handler, or interrupts
 *  can get lost. 
 *
 *  if you write a handler dealing with a yet unused interrupt
 *  bit (handler == not_used), enter your interrupt handler 
 *  in the appropriate table below. If your handler must poll
 *  for an interrupt flag to come active, *always* call
 *  dispatch_cia_ints() afterwards with bits in the mask
 *  register your code didn't already deal with. 
 *
 *	$Id: cia.c,v 1.4 1994/05/08 05:52:15 chopps Exp $
 */
#include <sys/types.h>
#include <sys/cdefs.h>
#include <amiga/amiga/cia.h>
#include "par.h"
#include "kbd.h"

struct cia_intr_dispatch {
  u_char	mask;
  void		(*handler) __P ((int));
};

static void not_used __P((int));
void kbdintr  __P((int));
void parintr  __P((int));

/* handlers for CIA-A (IPL-2) */
static struct cia_intr_dispatch ciaa_ints[] = {
	{ CIA_ICR_TA,		not_used },
	{ CIA_ICR_TB,		not_used },
	{ CIA_ICR_ALARM,	not_used },
#if NKBD > 0
	{ CIA_ICR_SP,	kbdintr },
#else
	{ CIA_ICR_SP,	not_used },
#endif
#if NPAR > 0
	{ CIA_ICR_FLG,	parintr },
#else
	{ CIA_ICR_FLG,	not_used },
#endif
	{ 0,		0 },
};

/* handlers for CIA-B (IPL-6) */
static struct cia_intr_dispatch ciab_ints[] = {
	{ CIA_ICR_TA,	not_used },	/* used directly in locore.s */
	{ CIA_ICR_TB,	not_used },	/* "" */
	{ CIA_ICR_ALARM,	not_used },
	{ CIA_ICR_SP,	not_used },
	{ CIA_ICR_FLG,	not_used },
	{ 0,		0 },
};



void
dispatch_cia_ints(which, mask)
	int which;
	int mask;
{
	struct cia_intr_dispatch *disp;

	disp = (which == 0) ? ciaa_ints : ciab_ints;

	for (;disp->mask; disp++)
		if (mask & disp->mask)
			disp->handler(disp->mask);
}

void
ciaa_intr()
{
	dispatch_cia_ints (0, ciaa.icr);
}

/*
 * NOTE: ciab_intr() is *not* currently called. If you want to support
 * the FLG interrupt, which is used to indicate a disk-index
 * interrupt, you'll have to hack a call to ciab_intr() into
 * the lev6 interrupt handler in locore.s !
 */
void
ciab_intr()
{
	dispatch_cia_ints (1, ciab.icr);
}


static void
not_used (mask)
     int mask;
{
}
