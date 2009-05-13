/*	$NetBSD: diskprobe.c,v 1.1.6.2 2009/05/13 17:18:54 jym Exp $	*/
/*	$OpenBSD: diskprobe.c,v 1.3 2006/10/13 00:00:55 krw Exp $	*/

/*
 * Copyright (c) 1997 Tobias Weingartner
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* We want the disk type names from disklabel.h */
#undef DKTYPENAMES

#include <sys/param.h>
#include <sys/bootblock.h>
#include <sys/disklabel.h>
#include <sys/queue.h>
#include <sys/reboot.h>

#include "boot.h"
#include "disk.h"
#include "unixdev.h"
#include "compat_linux.h"

/* Disk spin-up wait timeout. */
static u_int timeout = 10;

/* Local Prototypes */
static void hardprobe(char *buf, size_t bufsiz);

/* List of disk devices we found/probed */
struct disklist_lh disklist;

static char disk_devname[MAXDEVNAME];

/*
 * Probe for all hard disks.
 */
static void
hardprobe(char *buf, size_t bufsiz)
{
	/* XXX probe disks in this order */
	static const int order[] = { 0x80, 0x82, 0x00 };
	char devname[MAXDEVNAME];
	struct diskinfo *dip;
	u_int disk = 0;
	u_int hd_disk = 0;
	u_int mmcd_disk = 0;
	uint unit = 0;
	int first = 1;
	int i;

	buf[0] = '\0';

	/* Hard disks */
	for (i = 0; i < __arraycount(order); i++) {
		dip = alloc(sizeof(struct diskinfo));
		memset(dip, 0, sizeof(*dip));

		if (bios_getdiskinfo(order[i], &dip->bios_info) != NULL) {
			dealloc(dip, 0);
			continue;
		}

		bios_devname(order[i], devname, sizeof(devname));
		if (order[i] & 0x80) {
			unit = hd_disk;
			snprintf(dip->devname, sizeof(dip->devname), "%s%d",
			    devname, hd_disk++);
		} else {
			unit = mmcd_disk;
			snprintf(dip->devname, sizeof(dip->devname), "%s%d",
			    devname, mmcd_disk++);
		}
		strlcat(buf, dip->devname, bufsiz);
		disk++;

		/* Try to find the label, to figure out device type. */
		if (bios_getdisklabel(&dip->bios_info, &dip->disklabel)
		    == NULL) {
			strlcat(buf, "*", bufsiz);
			if (first) {
				first = 0;
				strlcpy(disk_devname, devname,
				    sizeof(disk_devname));
				default_devname = disk_devname;
				default_unit = unit;
			}
		} else {
			/* Best guess */
			switch (dip->disklabel.d_type) {
			case DTYPE_SCSI:
			case DTYPE_ESDI:
			case DTYPE_ST506:
				dip->bios_info.flags |= BDI_GOODLABEL;
				break;

			default:
				dip->bios_info.flags |= BDI_BADLABEL;
			}
		}

		/* Add to queue of disks. */
		TAILQ_INSERT_TAIL(&disklist, dip, list);

		strlcat(buf, " ", bufsiz);
	}
	if (disk == 0)
		strlcat(buf, "none...", bufsiz);
}

/* Probe for all BIOS supported disks */
void
diskprobe(char *buf, size_t bufsiz)
{

	/* Init stuff */
	TAILQ_INIT(&disklist);

	/* Do probes */
	hardprobe(buf, bufsiz);
}

/*
 * Find info on the disk given by major + unit number.
 */
struct diskinfo *
dkdevice(const char *devname, uint unit)
{
	char name[MAXDEVNAME];
	struct diskinfo *dip;

	snprintf(name, sizeof(name), "%s%d", devname, unit);
	for (dip = TAILQ_FIRST(&disklist); dip != NULL;
	     dip = TAILQ_NEXT(dip, list)) {
		if (strcmp(name, dip->devname) == 0) {
			return dip;
		}
	}
	return NULL;
}

int
bios_devname(int biosdev, char *devname, int size)
{

	if ((biosdev & 0x80) != 0) {
		strlcpy(devname, devname_hd, size);
	} else {
		strlcpy(devname, devname_mmcd, size);
	}
	return 0;
}

/*
 * Find the Linux device path that corresponds to the given "BIOS" disk,
 * where 0x80 corresponds to /dev/hda, 0x81 to /dev/hdb, and so on.
 */
void
bios_devpath(int dev, int part, char *p)
{
	char devname[MAXDEVNAME];
	const char *q;

	*p++ = '/';
	*p++ = 'd';
	*p++ = 'e';
	*p++ = 'v';
	*p++ = '/';

	bios_devname(dev, devname, sizeof(devname));
	q = devname;
	while (*q != '\0')
		*p++ = *q++;

	*p++ = 'a' + (dev & 0x7f);
	if (part >= 0)
		*p++ = '1' + part;
	*p = '\0';
}

/*
 * Fill out a bios_diskinfo_t for this device.
 */
char *
bios_getdiskinfo(int dev, bios_diskinfo_t *bdi)
{
	static char path[PATH_MAX];
	struct linux_stat sb;

	memset(bdi, 0, sizeof *bdi);
	bdi->bios_number = -1;

	bios_devpath(dev, -1, path);

	if (ustat(path, &sb) != 0)
		return "no device node";

	bdi->bios_number = dev;

	if (bios_getdospart(bdi) < 0)
		return "no NetBSD partition";

	return NULL;
}

int
bios_getdospart(bios_diskinfo_t *bdi)
{
	char path[PATH_MAX];
	char buf[DEV_BSIZE];
	struct mbr_partition *mp;
	int fd;
	u_int part;
	size_t rsize;

	bios_devpath(bdi->bios_number, -1, path);

	/*
	 * Give disk devices some time to become ready when the first open
	 * fails.  Even when open succeeds the disk is sometimes not ready.
	 */
	if ((fd = uopen(path, LINUX_O_RDONLY)) == -1 && errno == ENXIO) {
		while (fd == -1 && timeout > 0) {
			timeout--;
			sleep(1);
			fd = uopen(path, LINUX_O_RDONLY);
		}
		if (fd != -1)
			sleep(2);
	}
	if (fd == -1)
		return -1;

	/* Read the disk's MBR. */
	if (unixstrategy((void *)fd, F_READ, MBR_BBSECTOR, DEV_BSIZE, buf,
	    &rsize) != 0 || rsize != DEV_BSIZE) {
		uclose(fd);
		errno = EIO;
		return -1;
	}

	/* Find NetBSD primary partition in the disk's MBR. */
	mp = (struct mbr_partition *)&buf[MBR_PART_OFFSET];
	for (part = 0; part < MBR_PART_COUNT; part++) {
		if (mp[part].mbrp_type == MBR_PTYPE_NETBSD)
			break;
	}
	if (part == MBR_PART_COUNT) {
		uclose(fd);
		errno = ERDLAB;
		return -1;
	}
	uclose(fd);

	return part;
}

char *
bios_getdisklabel(bios_diskinfo_t *bdi, struct disklabel *label)
{
	char path[PATH_MAX];
	char buf[DEV_BSIZE];
	int part;
	int fd;
	size_t rsize;

	part = bios_getdospart(bdi);
	if (part < 0)
		return "no NetBSD partition";

	bios_devpath(bdi->bios_number, part, path);

	/* Test if the NetBSD partition has a valid disklabel. */
	if ((fd = uopen(path, LINUX_O_RDONLY)) != -1) {
		char *msg = "failed to read disklabel";

		if (unixstrategy((void *)fd, F_READ, LABELSECTOR,
		    DEV_BSIZE, buf, &rsize) == 0 && rsize == DEV_BSIZE)
			msg = getdisklabel(buf, label);
		uclose(fd);
		/* Don't wait for other disks if this label is ok. */
		if (msg == NULL)
			timeout = 0;
		return msg;
	}

	return "failed to open partition";
}
