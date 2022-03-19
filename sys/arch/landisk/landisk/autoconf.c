/*	$NetBSD: autoconf.c,v 1.9 2022/03/19 13:49:21 hannken Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.9 2022/03/19 13:49:21 hannken Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/kauth.h>

#include <machine/bootinfo.h>
#include <machine/intr.h>

void
cpu_configure(void)
{

	/* Start configuration */
	splhigh();
	intr_init();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/* Configuration is finished, turn on interrupts. */
	spl0();
}

static int
is_valid_disk(device_t dv)
{
	const char *name;

	if (device_class(dv) != DV_DISK)
		return (0);
	
	name = device_cfdata(dv)->cf_name;

	return (strcmp(name, "sd") == 0 || strcmp(name, "wd") == 0 ||
		strcmp(name, "ld") == 0);
}

/*
 * Helper function for findroot():
 * Return non-zero if disk device matches bootinfo.
 */
static int
match_bootdisk(device_t dv, struct btinfo_bootdisk *bid)
{
	struct vnode *tmpvn;
	int error;
	struct disklabel label;
	int found = 0;
	int bmajor;

	/*
	 * A disklabel is required here.  The boot loader doesn't refuse
	 * to boot from a disk without a label, but this is normally not
	 * wanted.
	 */
	if (bid->labelsector == -1)
		return (0);
	
	/*
	 * Lookup major number for disk block device.
	 */
	bmajor = devsw_name2blk(device_xname(dv), NULL, 0);
	if (bmajor == -1)
		return (0);	/* XXX panic ??? */

	/*
	 * Fake a temporary vnode for the disk, open it, and read
	 * the disklabel for comparison.
	 */
	if (bdevvp(MAKEDISKDEV(bmajor, device_unit(dv), RAW_PART), &tmpvn))
		panic("match_bootdisk: can't alloc vnode");
	vn_lock(tmpvn, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_OPEN(tmpvn, FREAD, NOCRED);
	if (error) {
#ifndef DEBUG
		/*
		 * Ignore errors caused by missing device, partition,
		 * or medium.
		 */
		if (error != ENXIO && error != ENODEV)
#endif
			aprint_error("match_bootdisk: can't open dev %s (%d)\n",
			    device_xname(dv), error);
		vput(tmpvn);
		return (0);
	}
	error = VOP_IOCTL(tmpvn, DIOCGDINFO, &label, FREAD, NOCRED);
	if (error) {
		/*
		 * XXX Can't happen -- open() would have errored out
		 * or faked one up.
		 */
		aprint_error("match_bootdisk: can't get label for dev %s (%d)\n",
		    device_xname(dv), error);
		goto closeout;
	}

	/* Compare with our data. */
	if (label.d_type == bid->label.type &&
	    label.d_checksum == bid->label.checksum &&
	    strncmp(label.d_packname, bid->label.packname, 16) == 0)
	    	found = 1;

closeout:
	VOP_CLOSE(tmpvn, FREAD, NOCRED);
	vput(tmpvn);
	return (found);
}

/*
 * Attempt to find the device from which we were booted.  If we can do so,
 * and not instructed not to do so, change rootdev to correspond to the
 * load device.
 */
static void
findroot(void)
{
	struct btinfo_bootdisk *bid;
	device_t dv;
	deviter_t di;

	if (booted_device)
		return;

	if ((bid = lookup_bootinfo(BTINFO_BOOTDISK)) != NULL) {
		/*
		 * Scan all disk devices for ones that match the passed data.
		 * Don't break if one is found, to get possible multiple
		 * matches - for problem tracking.  Use the first match anyway
		 * because lower device numbers are more likely to be the
		 * boot device.
		 */
		for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST);
		     dv != NULL;
		     dv = deviter_next(&di)) {
			if (device_class(dv) != DV_DISK)
				continue;

			if (is_valid_disk(dv)) {
				if (match_bootdisk(dv, bid) == 0)
				    	continue;
				goto bootdisk_found;
			}
			continue;

bootdisk_found:
			if (booted_device) {
				aprint_error("WARNING: double match for boot "
				    "device (%s, %s)\n",
				    device_xname(booted_device), device_xname(dv));
				continue;
			}
			booted_device = dv;
			booted_partition = bid->partition;
		}
		deviter_release(&di);

		if (booted_device)
			return;
	}
}

void
cpu_rootconf(void)
{

	findroot();

	aprint_normal("boot device: %s\n",
	    booted_device ? device_xname(booted_device) : "<unknown>");
	rootconf();
}
