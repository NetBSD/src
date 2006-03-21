/* $Id: rtc.c,v 1.1 2006/03/21 08:15:19 gdamore Exp $ */
/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * This code was written by Garrett D'Amore for the Champaign-Urbana
 * Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.1 2006/03/21 08:15:19 gdamore Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/clock_subr.h>
#include <evbmips/evbmips/clockvar.h>

/*
 * AR513x parts don't have an onboard RTC, so we supply a stub for them.
 */

static int	rtc_match(struct device *, struct cfdata *, void *);
static void	rtc_attach(struct device *, struct device *, void *);
static void	rtc_get(struct device *, time_t, struct clocktime *);
static void	rtc_set(struct device *, struct clocktime *);
static void	rtc_init(struct device *);

CFATTACH_DECL(rtc, sizeof (struct device), rtc_match, rtc_attach, NULL, NULL);

static const struct clockfns rtc_clockfns = {
	rtc_init, rtc_get, rtc_set
};


int
rtc_match(struct device *parent, struct cfdata *match, void *aux)
{

	return 1;
}

void
rtc_attach(struct device *parent, struct device *self, void *aux)
{

	clockattach(self, &rtc_clockfns);
}

void
rtc_get(struct device *dev, time_t base, struct clocktime *ct)
{
	struct clock_ymdhms ymdhms;

	clock_secs_to_ymdhms(base, &ymdhms);

	ct->sec = ymdhms.dt_sec;
	ct->min = ymdhms.dt_min;
	ct->hour = ymdhms.dt_hour;
	ct->dow = ymdhms.dt_wday;
	ct->day = ymdhms.dt_day;
	ct->mon = ymdhms.dt_mon;
	ct->year = ymdhms.dt_year;
}

void
rtc_set(struct device *dev, struct clocktime *ct)
{
}

void
rtc_init(struct device *dev)
{
}
