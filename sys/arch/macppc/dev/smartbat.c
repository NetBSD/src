/*	$NetBSD: smartbat.c,v 1.1 2007/03/25 23:22:46 macallan Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: smartbat.c,v 1.1 2007/03/25 23:22:46 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <macppc/dev/pmuvar.h>
#include <macppc/dev/batteryvar.h>
#include <machine/bus.h>
#include "opt_battery.h"

#ifdef SMARTBAT_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

#define BAT_CPU_TEMPERATURE	0
#define BAT_AC_PRESENT		1
#define BAT_PRESENT		2
#define BAT_VOLTAGE		3
#define BAT_CURRENT		4
#define BAT_MAX_CHARGE		5
#define BAT_CHARGE		6
#define BAT_CHARGING		7
#define BAT_DISCHARGING		8
#define BAT_FULL		9
#define BAT_TEMPERATURE		10
#define BAT_NSENSORS		11  /* number of sensors */

struct smartbat_softc {
	struct device sc_dev;
	struct pmu_ops *sc_pmu_ops;
	int sc_num;
	
	/* envsys stuff */
	struct sysmon_envsys sc_sysmon;
	struct envsys_basic_info sc_sinfo[BAT_NSENSORS];
	struct envsys_tre_data sc_sdata[BAT_NSENSORS];
};

static void smartbat_attach(struct device *, struct device *, void *);
static int smartbat_match(struct device *, struct cfdata *, void *);

CFATTACH_DECL(smartbat, sizeof(struct smartbat_softc),
    smartbat_match, smartbat_attach, NULL, NULL);

static int
smartbat_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct battery_attach_args *baa = aux;

	if (baa->baa_type == BATTERY_TYPE_SMART)
		return 1;

	return 0;
}

static void
smartbat_attach(struct device *parent, struct device *self, void *aux)
{
	struct battery_attach_args *baa = aux;
	struct smartbat_softc *sc = (struct smartbat_softc *)self;

	sc->sc_pmu_ops = baa->baa_pmu_ops;
	sc->sc_num = baa->baa_num;

	printf(" addr %d: smart battery\n", sc->sc_num);

}
