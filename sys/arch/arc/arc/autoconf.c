/*	$NetBSD: autoconf.c,v 1.15 2000/06/01 15:38:22 matt Exp $	*/
/*	$OpenBSD: autoconf.c,v 1.9 1997/05/18 13:45:20 pefo Exp $	*/

/*
 * Copyright (c) 1996 Per Fogelstrom
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

#include <machine/cpu.h>
#include <machine/autoconf.h>

static void findroot __P((void));
int getpno __P((char **cp));

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cpuspeed = 150;	/* approx # instr per usec. */
struct device *booted_device;
int booted_partition;

/*
 *  Configure all devices found that we know about.
 *  This is done at boot time.
 */
void
cpu_configure()
{
	(void)splhigh();	/* To be really sure.. */
	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");

	/* Configuration is finished, turn on interrupts. */
	_splnone();	/* enable all source forcing SOFT_INTs cleared */
}

#if defined(NFS_BOOT_BOOTP) || defined(NFS_BOOT_DHCP)
int nfs_boot_rfc951 = 1;
#endif

void
cpu_rootconf()
{
	findroot();

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
findroot(void)
{
	int i, majdev, unit, part;
	struct device *dv;
	char buf[32];

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
			booted_device = dv;
			booted_partition = part;
			return;
		}
	}
}

struct devmap {
	char *attachment;
	char *dev;
};

/*
 * Look at the string 'cp' and decode the boot device.
 * Boot names look like: scsi()disk(n)rdisk()partition(1)\bsd
 * (beware for empty scsi id's...)
 */
void
makebootdev(cp)
	char *cp;
{
	int ctrl, unit, part, i;
	static struct devmap devmap[] = {
		{ "multi", "fd" },
		{ "eisa", "wd" },
		{ "scsi", "sd" },
		{ NULL, NULL }
	};
	struct devmap *dp = &devmap[0];

	bootdev = B_DEVMAGIC;

	while (dp->attachment) {
		if (strncmp (cp, dp->attachment, strlen(dp->attachment)) == 0)
			break;
		dp++;
	}
	if (!dp->attachment) {
		printf("Warning: boot device unrecognized: %s\n", cp);
		return;
	}
	ctrl = getpno(&cp);
	if (*cp++ == ')')
		unit = getpno(&cp);
	else
		unit = 0;
	if (*cp++ == ')')
		getpno(&cp);
#if 0 /* ignore partition number */
	if (*cp++ == ')')
		part = getpno(&cp) - 1;
	else
#endif
		part = 0;

	for (i = 0; dev_name2blk[i].d_name != NULL; i++)
		if (strcmp(dp->dev, dev_name2blk[i].d_name) == 0)
			bootdev = MAKEBOOTDEV(dev_name2blk[i].d_maj, 0,
			    ctrl, unit, part);
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
