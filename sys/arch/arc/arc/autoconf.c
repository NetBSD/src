/*	$NetBSD: autoconf.c,v 1.33.10.1 2012/08/08 15:51:12 martin Exp $	*/
/*	$OpenBSD: autoconf.c,v 1.9 1997/05/18 13:45:20 pefo Exp $	*/

/*
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
 * 3. Neither the name of the University nor the names of its contributors
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
 * Copyright (c) 1996 Per Fogelstrom
 * Copyright (c) 1988 University of Utah.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.33.10.1 2012/08/08 15:51:12 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <arc/arc/timervar.h>

struct bootdev_data {
	const char *dev_type;
	int	bus;
	int	unit;
	int	partition;
};

static int getpno(const char **, int *);

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
struct bootdev_data *bootdev_data;

/*
 *  Configure all devices found that we know about.
 *  This is done at boot time.
 */
void
cpu_configure(void)
{

#ifdef ENABLE_INT5_STATCLOCK
	evcnt_attach_static(&statclock_ev);
#endif

	(void)splhigh();	/* To be really sure.. */
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/* Configuration is finished, turn on interrupts. */
#ifdef ENABLE_INT5_STATCLOCK
	/*
	 * Enable interrupt sources.
	 * We can't enable CPU INT5 which is used by statclock(9) here
	 * until cpu_initclocks(9) is called because there is no way
	 * to disable it other than setting status register by spl(9).
	 */
	_spllower(MIPS_INT_MASK_5);
#error need fix
#else
	/* enable all source forcing SOFT_INTs cleared */
	spl0();
#endif
}

#if defined(NFS_BOOT_BOOTP) || defined(NFS_BOOT_DHCP)
int nfs_boot_rfc951 = 1;
#endif

void
cpu_rootconf(void)
{

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	booted_partition = booted_device ? bootdev_data->partition : 0;
	rootconf();
}

struct devmap {
	const char *attachment;
	const char *dev;
};

/*
 * Look at the string 'cp' and decode the boot device.
 * Boot names look like: scsi()disk(n)rdisk()partition(1)\bsd
 * (beware for empty scsi id's...)
 */
void
makebootdev(const char *cp)
{
	int ok, junk;
	static struct devmap devmap[] = {
		{ "multi", "fd" },
		{ "eisa", "wd" },
		{ "scsi", "sd" },
		{ NULL, NULL }
	};
	struct devmap *dp = &devmap[0];
	static struct bootdev_data bd;

	/* "scsi()" */
	while (dp->attachment) {
		if (strncmp(cp, dp->attachment, strlen(dp->attachment)) == 0)
			break;
		dp++;
	}
	if (!dp->attachment) {
		printf("Warning: boot device unrecognized: %s\n", cp);
		return;
	}
	bd.dev_type = dp->dev;
	ok = getpno(&cp, &bd.bus);

	/* "multi(2)scsi(0)disk(0)rdisk(0)partition(1)" case */
	if (ok && strcmp(dp->attachment, "multi") == 0 &&
	    memcmp(cp, "scsi", 4) == 0) {
		bd.dev_type = "sd";
		ok = getpno(&cp, &bd.bus);
	}

	/* "disk(N)" */
	if (ok)
		ok = getpno(&cp, &bd.unit);
	else
		bd.unit = 0;

	/* "rdisk()" */
	if (*cp++ == ')')
		ok = getpno(&cp, &junk);

	/* "partition(1)" */
#if 0 /* ignore partition number */
	if (ok && getpno(&cp, &bd.partition))
		--bd.partition;
	else
#endif
		bd.partition = 0;

	bootdev_data = &bd;
}

static int
getpno(const char **cp, int *np)
{
	int val = 0;
	const char *s = *cp;
	int got = 0;

	*np = 0;

	while (*s && *s != '(')
		s++;
	if (*s == '(') {
		for (s++; *s; s++) {
			if (*s == ')') {
				s++;
				got = 1;
				*np = val;
				break;
			}
			val = val * 10 + *s - '0';
		}
	}
	*cp = s;
	return (got);
}

/*
 * Attempt to find the device from which we were booted.
 */
void
device_register(struct device *dev, void *aux)
{
	struct bootdev_data *b = bootdev_data;
	struct device *parent = device_parent(dev);

	static int found = 0, initted = 0, scsiboot = 0;
	static struct device *scsibusdev = NULL;

	if (b == NULL)
		return;	/* There is no hope. */
	if (found)
		return;

	if (!initted) {
		if (strcmp(b->dev_type, "sd") == 0)
			scsiboot = 1;
		initted = 1;
	}

	if (scsiboot && device_is_a(dev, "scsibus")) {
		/* XXX device_unit() abuse */
		if (device_unit(dev) == b->bus) {
			scsibusdev = dev;
#if 0
			printf("\nscsibus = %s\n", dev->dv_xname);
#endif
		}
		return;
	}

	if (!device_is_a(dev, b->dev_type))
		return;

	if (device_is_a(dev, "sd")) {
		struct scsipibus_attach_args *sa = aux;

		if (scsiboot && scsibusdev && parent == scsibusdev &&
		    sa->sa_periph->periph_target == b->unit) {
			booted_device = dev;
#if 0
			printf("\nbooted_device = %s\n", dev->dv_xname);
#endif
			found = 1;
		}
		return;
	}
	/* XXX device_unit() abuse */
	if (device_unit(dev) == b->unit) {
		booted_device = dev;
#if 0
		printf("\nbooted_device = %s\n", dev->dv_xname);
#endif
		found = 1;
	}
}
