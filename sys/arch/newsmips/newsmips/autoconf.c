/*	$NetBSD: autoconf.c,v 1.2 1998/03/04 22:22:36 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: autoconf.c 1.31 91/01/21
 *
 *	@(#)autoconf.c	8.1 (Berkeley) 6/10/93
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/conf.h>
#include <sys/dmap.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <newsmips/newsmips/machid.h>
#include <machine/cpu.h>

void dumpconf __P((void)); 	/* XXX */

#if 0
/*
 * XXX system-dependent, should call through a pointer.
 * (spl0 should _NOT_ enable TC interrupts on a 3MIN.)
 *
 */
int spl0 __P((void));
#endif


/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold = 1;	/* if 1, still working on cold-start */
int	cpuspeed = 7;	/* approx # instr per usec. */

extern int cputype;	/* glue for new-style config */
int cputype;

extern int initcpu __P((void));		/*XXX*/

void	findroot __P((struct device **, int *));

struct devnametobdevmaj pmax_nam2blk[] = {
	{ "sd",		0 },
	{ "fd",		1 },
	{ "md",		2 },
	{ "cd",		16 },
	{ "st",		17 },
	{ NULL,		0 },
};

extern struct idrom	idrom;

/*
 * Determine mass storage and memory configuration for a machine.
 * Print cpu type, and then iterate over an array of devices
 * found on the baseboard or in turbochannel option slots.
 * Once devices are configured, enable interrupts, and probe
 * for attached scsi devices.
 */
void
configure()
{
	int s;

	printf("SONY NET WORK STATION, Model %s, ", idrom.id_model);
	printf("Machine ID #%d\n", idrom.id_serial);

	cputype = idrom.id_modelid;

	/*
	 * Kick off autoconfiguration
	 */
	s = splhigh();
	if (config_rootfound("mainbus", "mainbus") == NULL)
	    panic("no mainbus found");

	initcpu();

	cold = 0;
}

void
cpu_rootconf()
{
	struct device *booted_device;
	int booted_partition;

	findroot(&booted_device, &booted_partition);

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition, pmax_nam2blk);
}

u_long	bootdev = 0;		/* should be dev_t, but not until 32 bits */

/*
 * Attempt to find the device from which we were booted.
 */
void
findroot(devpp, partp)
	struct device **devpp;
	int *partp;
{
	int i, majdev, unit, part, controller;
	struct device *dv;
	char buf[32];
	const char *bootdv_name;

	/*
	 * Default to "not found".
	 */
	*devpp = NULL;
	*partp = 0;
	bootdv_name = NULL;

	if ((bootdev & B_MAGICMASK) != 0x50000000) /* NEWS-OS's B_DEVMAGIC */
		return;

	majdev = B_TYPE(bootdev);
	for (i = 0; pmax_nam2blk[i].d_name != NULL; i++) {
		if (majdev == pmax_nam2blk[i].d_maj) {
			bootdv_name = pmax_nam2blk[i].d_name;
			break;
		}
	}

	if (bootdv_name == NULL) {
#if defined(DEBUG)
		printf("findroot(): no name2blk for boot device %d\n", majdev);
#endif
		return;
	}

	controller = B_CONTROLLER(bootdev);
	/* part = B_PARTITION(bootdev); */
	/* unit = B_UNIT(bootdev); */
	part = (bootdev >> 8) & 0x0f;
	unit = (bootdev >> 20) & 0x0f;

	sprintf(buf, "%s%d", pmax_nam2blk[i].d_name, unit);
	for (dv = alldevs.tqh_first; dv; dv=dv->dv_list.tqe_next) {
		if (strcmp(buf, dv->dv_xname) == 0) {
			*devpp = dv;
			*partp = part;
			return;
		}
	}
}
