/*	$NetBSD: component.c,v 1.5 2010/10/19 11:58:01 pgoyette Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: component.c,v 1.5 2010/10/19 11:58:01 pgoyette Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/stat.h>

#include <dev/sysmon/sysmon_taskq.h>
#include <dev/sysmon/sysmonvar.h>

#include "rump_private.h"
#include "rump_dev_private.h"
#include "rump_vfs_private.h"

#include "ioconf.c"

void swwdogattach(int);
void swsensorattach(int);

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	extern const struct cdevsw sysmon_cdevsw;
	devmajor_t bmaj, cmaj;
	int error;

	if ((error = config_init_component(cfdriver_ioconf_swwdog,
	    cfattach_ioconf_swwdog, cfdata_ioconf_swwdog)) != 0) {
		printf("cannot attach swwdog: %d\n", error);
		return;
	}

	bmaj = cmaj = -1;
	if ((error = devsw_attach("sysmon", NULL, &bmaj,
	    &sysmon_cdevsw, &cmaj)) != 0)
		panic("sysmon devsw attach failed: %d", error);

	if ((error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/sysmon",
	    cmaj, SYSMON_MINOR_ENVSYS)) != 0)
		panic("cannot create /dev/sysmon: %d", error);
	if ((error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/watchdog",
	    cmaj, SYSMON_MINOR_WDOG)) != 0)
		panic("cannot create /dev/watchdog: %d", error);
	if ((error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/power",
	    cmaj, SYSMON_MINOR_POWER)) != 0)
		panic("cannot create /dev/power: %d", error);

	sysmon_task_queue_preinit();
	sysmon_task_queue_init();
	sysmon_envsys_init();
	sysmon_power_init();
	sysmon_wdog_init();

	rump_pdev_add(swwdogattach, 0);
	rump_pdev_add(swsensorattach, 0);
}
