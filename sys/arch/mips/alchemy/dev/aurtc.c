/* $NetBSD: aurtc.c,v 1.6 2003/07/15 02:43:35 lukem Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aurtc.c,v 1.6 2003/07/15 02:43:35 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <evbmips/evbmips/clockvar.h>
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/aubusvar.h>

static int	aurtc_match(struct device *, struct cfdata *, void *);
static void	aurtc_attach(struct device *, struct device *, void *);

static void	aurtc_init(struct device *);
static void	aurtc_get(struct device *, time_t, struct clocktime *);
static void	aurtc_set(struct device *, struct clocktime *);

CFATTACH_DECL(aurtc, sizeof (struct device),
    aurtc_match, aurtc_attach, NULL, NULL);

const struct clockfns aurtc_clockfns = {
	aurtc_init, aurtc_get, aurtc_set,
};

int
aurtc_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct aubus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
aurtc_attach(struct device *parent, struct device *self, void *aux)
{

	printf(": Au1X00 programmable clock");	/* \n in clockattach */
	clockattach(self, &aurtc_clockfns);
}

void
aurtc_init(struct device *dev)
{

	/* We don't use the aurtc for the hardclock interrupt. */
}

/*
 * Note the fake "aurtc" on the Alchemy pb1000 uses a different year base.
 */
#define	PB1000_YEAR_OFFSET	100

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
void
aurtc_get(struct device *dev, time_t base, struct clocktime *ct)
{
	struct clock_ymdhms ymdhms;
	time_t secs;

	secs = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(PC_BASE + PC_COUNTER_READ_0);

	clock_secs_to_ymdhms(secs, &ymdhms);

	ct->sec = ymdhms.dt_sec;
	ct->min = ymdhms.dt_min;
	ct->hour = ymdhms.dt_hour;
	ct->dow = ymdhms.dt_wday;
	ct->day = ymdhms.dt_day;
	ct->mon = ymdhms.dt_mon;
	ct->year = ymdhms.dt_year - PB1000_YEAR_OFFSET;
}

/*
 * Reset the TODR based on the time value.
 */
void
aurtc_set(struct device *dev, struct clocktime *ct)
{
	struct clock_ymdhms ymdhms;
	time_t secs;

	ymdhms.dt_sec = ct->sec;
	ymdhms.dt_min = ct->min;
	ymdhms.dt_hour = ct->hour;
	ymdhms.dt_wday = ct->dow;
	ymdhms.dt_day = ct->day;
	ymdhms.dt_mon = ct->mon;
	ymdhms.dt_year = ct->year + PB1000_YEAR_OFFSET;

	secs = clock_ymdhms_to_secs(&ymdhms);

	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(PC_BASE + PC_COUNTER_READ_0) = secs;
}
