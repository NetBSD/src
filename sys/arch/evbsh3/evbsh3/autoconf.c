/*	$NetBSD: autoconf.c,v 1.2 2002/02/22 19:44:00 uch Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)autoconf.c	7.1 (Berkeley) 5/9/91
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/dkio.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/bootinfo.h>

static int match_harddisk __P((struct device *, struct btinfo_bootdisk *));

struct device *booted_device;
int booted_partition;

static void findroot __P((void));

/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure()
{

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	printf("biomask %0x netmask %0x ttymask %0x\n",
	       imask[IPL_BIO], imask[IPL_NET], imask[IPL_TTY]);

	spl0();
}

void
cpu_rootconf()
{
	findroot();

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

u_long	bootdev = 0;		/* should be dev_t, but not until 32 bits */
struct device *booted_device;

/*
 * helper function for "findroot()":
 * return nonzero if disk device matches bootinfo
 */
static int
match_harddisk(dv, bid)
	struct device *dv;
	struct btinfo_bootdisk *bid;
{
	struct devnametobdevmaj *i;
	struct vnode *tmpvn;
	int error;
	struct disklabel label;
	int found = 0;

	/*
	 * A disklabel is required here.  The
	 * bootblocks don't refuse to boot from
	 * a disk without a label, but this is
	 * normally not wanted.
	 */
	if (bid->labelsector == -1)
		return(0);

	/*
	 * lookup major number for disk block device
	 */
	i = dev_name2blk;
	while (i->d_name &&
	       strcmp(i->d_name, dv->dv_cfdata->cf_driver->cd_name))
		i++;
	if (i->d_name == NULL)
		return(0); /* XXX panic() ??? */

	/*
	 * Fake a temporary vnode for the disk, open
	 * it, and read the disklabel for comparison.
	 */
	if (bdevvp(MAKEDISKDEV(i->d_maj, dv->dv_unit, bid->partition), &tmpvn))
		panic("findroot can't alloc vnode");
	error = VOP_OPEN(tmpvn, FREAD, NOCRED, 0);
	if (error) {
#ifndef DEBUG
		/*
		 * Ignore errors caused by missing
		 * device, partition or medium.
		 */
		if (error != ENXIO && error != ENODEV)
#endif
			printf("findroot: can't open dev %s%c (%d)\n",
			       dv->dv_xname, 'a' + bid->partition, error);
		vrele(tmpvn);
		return(0);
	}
	error = VOP_IOCTL(tmpvn, DIOCGDINFO, (caddr_t)&label, FREAD, NOCRED, 0);
	if (error) {
		/*
		 * XXX can't happen - open() would
		 * have errored out (or faked up one)
		 */
		printf("can't get label for dev %s%c (%d)\n",
		       dv->dv_xname, 'a' + bid->partition, error);
		goto closeout;
	}

	/* compare with our data */
	if (label.d_type == bid->label.type &&
	    label.d_checksum == bid->label.checksum &&
	    !strncmp(label.d_packname, bid->label.packname, 16))
		found = 1;

closeout:
	VOP_CLOSE(tmpvn, FREAD, NOCRED, 0);
	vrele(tmpvn);

	return(found);
}

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
findroot(void)
{
	struct btinfo_bootdisk *bid;
	struct device *dv;
	int i, majdev, unit, part;
	char buf[32];

	if (booted_device)
		return;

	if (lookup_bootinfo(BTINFO_NETIF)) {
		/*
		 * We got netboot interface information, but
		 * "device_register()" couldn't match it to a configured
		 * device. Bootdisk information cannot be present at the
		 * same time, so give up.
		 */
		printf("findroot: netboot interface not found\n");
		return;
	}

	bid = lookup_bootinfo(BTINFO_BOOTDISK);
	if (bid) {
		/*
		 * Scan all disk devices for ones that match the passed data.
		 * Don't break if one is found, to get possible multiple
		 * matches - for problem tracking. Use the first match anyway
		 * because lower device numbers are more likely to be the
		 * boot device.
		 */
		for (dv = alldevs.tqh_first; dv != NULL;
		dv = dv->dv_list.tqe_next) {
			if (dv->dv_class != DV_DISK)
				continue;

			if (!strcmp(dv->dv_cfdata->cf_driver->cd_name, "fd")) {
				/*
				 * Assume the configured unit number matches
				 * the BIOS device number.  (This is the old
				 * behaviour.)  Needs some ideas how to handle
				 * BIOS's "swap floppy drive" options.
				 */
				if ((bid->biosdev & 0x80) ||
				    dv->dv_unit != bid->biosdev)
					continue;

				goto found;
			}

			if (!strcmp(dv->dv_cfdata->cf_driver->cd_name, "sd") ||
			    !strcmp(dv->dv_cfdata->cf_driver->cd_name, "wd")) {
				/*
				 * Don't trust BIOS device numbers, try
				 * to match the information passed by the
				 * bootloader instead.
				 */
				if ((bid->biosdev & 0x80) == 0 ||
				    !match_harddisk(dv, bid))
					continue;

				goto found;
			}

			/* no "fd", "wd" or "sd" */
			continue;

found:
			if (booted_device) {
				printf("warning: double match for boot "
				    "device (%s, %s)\n", booted_device->dv_xname,
				    dv->dv_xname);
				continue;
			}
			booted_device = dv;
			booted_partition = bid->partition;
		}

		if (booted_device)
			return;
	}

#if 0
	printf("howto %x bootdev %x ", boothowto, bootdev);
#endif

	if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;

	majdev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
	for (i = 0; dev_name2blk[i].d_name != NULL; i++)
		if (majdev == dev_name2blk[i].d_maj)
			break;
	if (dev_name2blk[i].d_name == NULL)
		return;

	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;

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
