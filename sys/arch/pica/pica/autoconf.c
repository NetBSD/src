/*	$NetBSD: autoconf.c,v 1.9 1999/06/07 20:16:12 thorpej Exp $	*/

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
 *	from: @(#)autoconf.c	8.1 (Berkeley) 6/10/93
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
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/autoconf.h>

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold = 1;	/* if 1, still working on cold-start */
int	cpuspeed = 150;	/* approx # instr per usec. */
extern	int pica_boardtype;

void	findroot __P((struct device **devpp, int *partp));
int	getpno __P((char **cp));


/*
 *  Configure all devices found that we know about.
 *  This is done at boot time.
 */
void
configure()
{

	(void)splhigh();	/* To be really shure.. */
	if(config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");
	(void)spl0();
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

	setroot(booted_device, booted_partition);
}

u_long	bootdev;		/* should be dev_t, but not until 32 bits */

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
findroot(devpp, partp)
	struct device **devpp;
	int *partp;
{
	int i, majdev, unit, part;
	struct device *dv;
	char buf[32];

	/*
	 * Default to "not found."
	 */
	*devpp = NULL;
	*partp = 0;

#if 0
	printf("howto %x bootdev %x ", boothowto, bootdev);
#endif

	if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;

	majdev = B_TYPE(bootdev);
	for (i = 0; dev_name2blk[i].d_name != NULL; i++)
		if (majdev == dev_name2blk[i].d_maj)
			break;
	if (dev_name2blk[i].d_name == NULL)
		return;

	part = B_PARTITION(bootdev);
	unit = B_UNIT(bootdev);

	sprintf(buf, "%s%d", dev_name2blk[i].d_name, unit);
	for (dv = alldevs.tqh_first; dv != NULL;
	    dv = dv->dv_list.tqe_next) {
		if (strcmp(buf, dv->dv_xname) == 0) {
			*devpp = dv;
			*partp = part;
			return;
		}
	}
}



/*
 * Look at the string 'cp' and decode the boot device.
 * Boot names look like: scsi()disk(n)rdisk()partition(1)\bsd
 */
void
makebootdev(cp)
	char *cp;
{
	int majdev, unit, ctrl;
	char dv[8];

	bootdev = B_DEVMAGIC;

	dv[0] = *cp;
	ctrl = getpno(&cp);
	if(*cp++ == ')') {
		dv[1] = *cp;
		unit = getpno(&cp);

		for (majdev = 0; majdev < sizeof(dev_name2blk)/sizeof(dev_name2blk[0]); majdev++)
			if (dv[0] == dev_name2blk[majdev].d_name[0] &&
			    dv[1] == dev_name2blk[majdev].d_name[1] &&
			    cp[0] == ')')
				bootdev = MAKEBOOTDEV(majdev, 0, ctrl, unit,0);
	}
}


int
getpno(cp)
	char **cp;
{
	int val = 0;
	char *cx = *cp;

	while(*cx && *cx != '(')
		cx++;
	if(*cx == '(') {
		cx++;
		while(*cx && *cx != ')') {
			val = val * 10 + *cx - '0';
			cx++;
		}
	}
	*cp = cx;
	return val;
}
