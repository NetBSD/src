/* $NetBSD: fancontrol.c,v 1.3 2021/07/30 22:07:14 macallan Exp $ */

/*-
 * Copyright (c) 2021 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: fancontrol.c,v 1.3 2021/07/30 22:07:14 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/sysctl.h>
#include <dev/sysmon/sysmonvar.h>

#include <macppc/dev/fancontrolvar.h>
#include "opt_fancontrol.h"

#ifdef FANCONTROL_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

int 
fancontrol_adjust_zone(fancontrol_zone_t *z)
{
	int temp, i, speed, diff, step;
	
	if (z->nfans <= 0)
		return -1;

	temp = sysmon_envsys_get_max_value(z->filter, true);
	if (temp == 0) {
		/* no sensor data - leave fan alone */
		DPRINTF("nodata\n");
		return -1;
	}

	if (z->Tmin < 30) z->Tmin = 30;
	if (z->Tmin > 60) z->Tmin = 60;
	if (z->Tmax > 95) z->Tmax = 95;
	if (z->Tmax < (z->Tmin + 10)) z->Tmax = z->Tmin + 10;
	temp = (temp - 273150000) / 1000000;
	diff = temp - z->Tmin;
	DPRINTF("%s %d %d\n", z->name, temp, z->Tmin);
	if (diff < 0) diff = 0;
	diff = (100 * diff) / (z->Tmax - z->Tmin);

	/* now adjust each fan to the new speed */
	for (i = 0; i < z->nfans; i++) {
		step = (z->fans[i].max_rpm - z->fans[i].min_rpm) / 100;
		speed = z->fans[i].min_rpm + diff * step;
		DPRINTF("diff %d base %d %d sp %d\n",
		    diff, z->fans[i].min_rpm, z->fans[i].max_rpm, speed);
		z->set_rpm(z->cookie, z->fans[i].num, speed);
	}
	return 0;
}

int
fancontrol_init_zone(fancontrol_zone_t *z, struct sysctlnode *me)
{
	struct sysctlnode *zone_node, *node;

	if (z->nfans <= 0) return 0;

	sysctl_createv(NULL, 0, NULL, (void *) &zone_node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_NODE, z->name, NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP,
	    me->sysctl_num,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, NULL, (void *) &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "Tmin", "minimum temperature",
	    NULL, 0, (void *)&z->Tmin, 0,
	    CTL_MACHDEP,
	    me->sysctl_num,
	    zone_node->sysctl_num,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, NULL, (void *) &node,
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "Tmax", "maximum temperature",
	    NULL, 0, (void *)&z->Tmax, 0,
	    CTL_MACHDEP,
	    me->sysctl_num,
	    zone_node->sysctl_num,
	    CTL_CREATE, CTL_EOL);

	return 0;
}
