/*	$NetBSD: cpu_speedctl.c,v 1.1.2.2 2018/06/25 07:25:45 pgoyette Exp $ */

/*-
 * Copyright (c) 2006 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: cpu_speedctl.c,v 1.1.2.2 2018/06/25 07:25:45 pgoyette Exp $");

#include "opt_ppcparam.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/types.h>

#include <powerpc/psl.h>
#include <powerpc/spr.h>
#include <powerpc/oea/cpufeat.h>
#include <powerpc/oea/spr.h>

#include <sys/sysctl.h>
#include <dev/ofw/openfirm.h>

void init_scom_speedctl(void);
static int scom_speedctl = 0;
static uint32_t scom_speeds[2];
static uint32_t scom_reg[2];
static int  sysctl_cpuspeed_temp(SYSCTLFN_ARGS);
static int  sysctl_cpuspeed_cur(SYSCTLFN_ARGS);
static int  sysctl_cpuspeed_available(SYSCTLFN_ARGS);

void
init_scom_speedctl(void)
{
	int node;
	const struct sysctlnode *sysctl_node, *me, *freq;

	/* do this only once */
	if (scom_speedctl != 0) return;
	scom_speedctl = 1;

	node = OF_finddevice("/cpus/@0");
	if (OF_getprop(node, "power-mode-data", scom_reg, 8) != 8)
		return;
	if (OF_getprop(node, "clock-frequency", scom_speeds, 4) != 4)
		return;
	scom_speeds[0] = scom_speeds[0] / 1000000; /* Hz -> MHz */
	scom_speeds[1] = scom_speeds[0] / 2;

	sysctl_node = NULL;

	if (sysctl_createv(NULL, 0, NULL, 
	    &me, 
	    CTLFLAG_READWRITE, CTLTYPE_NODE, "cpu", NULL, NULL,
	    0, NULL, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL) != 0)
		printf("couldn't create 'cpu' node\n");
	
	if (sysctl_createv(NULL, 0, NULL, 
	    &freq, 
	    CTLFLAG_READWRITE, CTLTYPE_NODE, "frequency", NULL, NULL,
	    0, NULL, 0, CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL) != 0)
		printf("couldn't create 'frequency' node\n");

	if (sysctl_createv(NULL, 0, NULL, 
	    &sysctl_node, 
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "target", "CPU speed", sysctl_cpuspeed_temp, 
	    0, NULL, 0, CTL_MACHDEP, me->sysctl_num, freq->sysctl_num, 
	    CTL_CREATE, CTL_EOL) == 0) {
	} else
		printf("couldn't create 'target' node\n");

	if (sysctl_createv(NULL, 0, NULL, 
	    &sysctl_node, 
	    CTLFLAG_READWRITE,
	    CTLTYPE_INT, "current", NULL, sysctl_cpuspeed_cur, 
	    1, NULL, 0, CTL_MACHDEP, me->sysctl_num, freq->sysctl_num, 
	    CTL_CREATE, CTL_EOL) == 0) {
	} else
		printf("couldn't create 'current' node\n");

	if (sysctl_createv(NULL, 0, NULL, 
	    &sysctl_node, 
	    CTLFLAG_READWRITE,
	    CTLTYPE_STRING, "available", NULL, sysctl_cpuspeed_available, 
	    2, NULL, 0, CTL_MACHDEP, me->sysctl_num, freq->sysctl_num, 
	    CTL_CREATE, CTL_EOL) == 0) {
	} else
		printf("couldn't create 'available' node\n");
}

static int
sysctl_cpuspeed_temp(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	int speed, nspeed = -1, mhz;

	speed = (scom_read(SCOM_PSR) >> 56) & 3;
	if (speed > 1) speed = 1;
	mhz = scom_speeds[speed];
	node.sysctl_data = &mhz;

	if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
		int new_reg;

		new_reg = *(int *)node.sysctl_data;
		if (new_reg == scom_speeds[0]) {
			nspeed = 0;
		} else if (new_reg == scom_speeds[1]) {
			nspeed = 1;
		} else {
			printf("%s: new_reg %d\n", __func__, new_reg);
			return EINVAL;
		}
		if (nspeed != speed) {
			volatile uint64_t junk;
			scom_write(SCOM_PCR, 0x80000000);
			scom_write(SCOM_PCR, scom_reg[nspeed]);
			junk = scom_read(SCOM_PSR);
			__USE(junk);
		}
		return 0;
	}
	return EINVAL;
}

static int
sysctl_cpuspeed_cur(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	uint64_t speed;
	int mhz;

	speed = (scom_read(SCOM_PSR) >> 56) & 3;
	if (speed > 1) speed = 1;

	mhz = scom_speeds[speed];
	node.sysctl_data = &mhz;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
sysctl_cpuspeed_available(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	char buf[128];

	snprintf(buf, 128, "%d %d", scom_speeds[0], scom_speeds[1]);	
	node.sysctl_data = buf;
	return(sysctl_lookup(SYSCTLFN_CALL(&node)));
}

SYSCTL_SETUP(sysctl_ams_setup, "sysctl obio subtree setup")
{

	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
}
