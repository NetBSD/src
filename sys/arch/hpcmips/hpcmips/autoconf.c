/*	$NetBSD: autoconf.c,v 1.8.2.2 2002/02/11 20:08:06 jdolecek Exp $	*/

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.8.2.2 2002/02/11 20:08:06 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>	/* setroot() */

#include <machine/disklabel.h>

#include <machine/sysconf.h>
#include <machine/config_hook.h>

void makebootdev(const char *);

struct device *booted_device;
int booted_partition;
static char __booted_device_name[16];
static void get_device(const char *);

/*
 * Setup the system to run on the current machine.
 *
 * cpu_configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */
void
cpu_configure()
{

	softintr_init();

	/* Kick off autoconfiguration. */
	(void)splhigh();

	config_hook_init();

	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");

	/* Configuration is finished, turn on interrupts. */
	_splnone();	/* enable all source forcing SOFT_INTs cleared */
}

void
cpu_rootconf()
{

	get_device(__booted_device_name);

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

void
makebootdev(const char *cp)
{

	strncpy(__booted_device_name, cp, 16);
}

static void
get_device(const char *name)
{
	int loop, unit, part;
	char buf[32];
	const char *cp;
	struct device *dv;

	if (strncmp(name, "/dev/", 5) == 0)
		name += 5;

	for (loop = 0; dev_name2blk[loop].d_name != NULL; ++loop) {
		if (strncmp(name, dev_name2blk[loop].d_name,
		    strlen(dev_name2blk[loop].d_name)) == 0) {
			name += strlen(dev_name2blk[loop].d_name);
			unit = part = 0;

			cp = name;
			while (*cp >= '0' && *cp <= '9')
				unit = (unit * 10) + (*cp++ - '0');
			if (cp == name)
				return;

			if (*cp >= 'a' && *cp <= ('a' + MAXPARTITIONS))
				part = *cp - 'a';
			else if (*cp != '\0' && *cp != ' ')
				return;
			sprintf(buf, "%s%d", dev_name2blk[loop].d_name, unit);
			for (dv = alldevs.tqh_first; dv != NULL;
			    dv = dv->dv_list.tqe_next) {
				if (strcmp(buf, dv->dv_xname) == 0) {
					booted_device = dv;
					booted_partition = part;
					return;
				}
			}
		}
	} 
}
