/*	$NetBSD: ofw_machdep.c,v 1.11.14.1 2002/05/17 13:49:58 gehenna Exp $	*/

/*
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/stat.h>
#include <sys/systm.h>

#include <dev/ofw/openfirm.h>

#include <machine/powerpc.h>

#define	OFMEM_REGIONS	32
static struct mem_region OFmem[OFMEM_REGIONS + 1], OFavail[OFMEM_REGIONS + 3];

/*
 * This is called during initppc, before the system is really initialized.
 * It shall provide the total and the available regions of RAM.
 * Both lists must have a zero-size entry as terminator.
 * The available regions need not take the kernel into account, but needs
 * to provide space for two additional entry beyond the terminating one.
 */
void
mem_regions(struct mem_region **memp, struct mem_region **availp)
{
	int phandle, i, cnt;

	/*
	 * Get memory.
	 */
	if ((phandle = OF_finddevice("/memory")) == -1)
		goto error;

	memset(OFmem, 0, sizeof OFmem);
	cnt = OF_getprop(phandle, "reg",
		OFmem, sizeof OFmem[0] * OFMEM_REGIONS);
	if (cnt <= 0)
		goto error;

	/* Remove zero sized entry in the returned data. */
	cnt /= sizeof OFmem[0];
	for (i = 0; i < cnt; )
		if (OFmem[i].size == 0) {
			memmove(&OFmem[i], &OFmem[i + 1],
				(cnt - i) * sizeof OFmem[0]);
			cnt--;
		} else
			i++;

	memset(OFavail, 0, sizeof OFavail);
	cnt = OF_getprop(phandle, "available",
		OFavail, sizeof OFavail[0] * OFMEM_REGIONS);
	if (cnt <= 0)
		goto error;

	cnt /= sizeof OFavail[0];
	for (i = 0; i < cnt; )
		if (OFavail[i].size == 0) {
			memmove(&OFavail[i], &OFavail[i + 1],
				(cnt - i) * sizeof OFavail[0]);
			cnt--;
		} else
			i++;

	*memp = OFmem;
	*availp = OFavail;
	return;

error:
	panic("no memory?");
}

void
ppc_exit(void)
{
	OF_exit();
}

void
ppc_boot(char *str)
{
	OF_boot(str);
}

#ifdef __BROKEN_DK_ESTABLISH
/*
 * Establish a list of all available disks to allow specifying the
 * root/swap/dump dev.
 */
struct ofb_disk {
	LIST_ENTRY(ofb_disk) ofb_list;
	struct disk *ofb_dk;
	struct device *ofb_dev;
	int ofb_phandle;
	int ofb_unit;
};

#include <machine/autoconf.h>

static LIST_HEAD(ofb_list, ofb_disk) ofb_head;	/* LIST_INIT?		XXX */

void
dk_establish(struct disk *dk, struct device *dev)
{
	struct ofb_disk *od;
	struct ofbus_softc *ofp = (void *)dev;

	MALLOC(od, struct ofb_disk *, sizeof *od, M_TEMP, M_NOWAIT);
	if (!od)
		panic("dk_establish");
	od->ofb_dk = dk;
	od->ofb_dev = dev;
	od->ofb_phandle = ofp->sc_phandle;
	if (dev->dv_class == DV_DISK)				/* XXX */
		od->ofb_unit = ofp->sc_unit;
	else
		od->ofb_unit = -1;
	LIST_INSERT_HEAD(&ofb_head, od, ofb_list);
}

/*
 * Cleanup the list.
 */
void
dk_cleanup(void)
{
	struct ofb_disk *od, *nd;

	for (od = ofb_head.lh_first; od; od = nd) {
		nd = od->ofb_list.le_next;
		LIST_REMOVE(od, ofb_list);
		FREE(od, M_TEMP);
	}
}

static void
dk_setroot(struct ofb_disk *od, int part)
{
	const struct bdevsw *bdev;
	char type[8];
	int unit;
	struct disklabel *lp;
	dev_t tmpdev;
	char *cp;
	int bmajor;

	if (OF_getprop(od->ofb_phandle, "device_type", type, sizeof type) < 0)
		panic("OF_getproperty");

	if (strcmp(type, "block") == 0) {
		bmajor = devsw_name2blk(od->ofb_dev->dv_xname, NULL, 0);
		if (bmajor == -1)
			panic("dk_setroot: impossible");
		bdev = bdevsw_lookup(makedev(bmajor, 0));
		if (bdev == NULL)
			panic("dk_setroot: impossible");

		/*
		 * Find the unit.
		 */
		unit = 0;
		for (cp = od->ofb_dk->dk_name; *cp; cp++) {
			if (*cp >= '0' && *cp <= '9')
				unit = unit * 10 + *cp - '0';
			else
				/* Start anew */
				unit = 0;
		}

		/*
		 * Find a default partition; try partition `a', then
		 * fall back on RAW_PART.
		 */
		if (part == -1) {
			/*
			 * Open the disk to force an update of the in-core
			 * disklabel.  Use RAW_PART because all disk
			 * drivers allow RAW_PART to be opened.
			 */
			tmpdev = MAKEDISKDEV(bmajor, unit, RAW_PART);

			if ((*bdev->d_open)(tmpdev, FREAD, S_IFBLK, 0)) {
				/*
				 * Open failed.  Device is probably not
				 * configured.  setroot() can handle this.
				 */
				return;
			}
			(void)(*bdev->d_close)(tmpdev, FREAD, S_IFBLK, 0);
			lp = od->ofb_dk->dk_label;

			/* Check for a valid `a' partition. */
			if (lp->d_partitions[0].p_size > 0 &&
			    lp->d_partitions[0].p_fstype != FS_UNUSED)
				part = 0;
			else
				part = RAW_PART;
		}
		booted_device = od->ofb_dev;
		booted_partition = part;
	} else if (strcmp(type, "network") == 0) {
		booted_device = od->ofb_dev;
		booted_partition = 0;
	}

	/* "Not found."  setroot() will ask for the root device. */
}

/*
 * Try to find a disk with the given name.
 * This allows either the OpenFirmware device name,
 * or the NetBSD device name, both with optional trailing partition.
 */
int
dk_match(char *name)
{
	struct ofb_disk *od;
	char *cp;
	int phandle;
	int part, unit;
	int l;

	for (od = ofb_head.lh_first; od; od = od->ofb_list.le_next) {
		/*
		 * First try the NetBSD name.
		 */
		l = strlen(od->ofb_dev->dv_xname);
		if (!memcmp(name, od->ofb_dev->dv_xname, l)) {
			if (name[l] == '\0') {
				/* Default partition, (or none at all) */
				dk_setroot(od, -1);
				return 0;
			}
			if (name[l + 1] == '\0') {
				switch (name[l]) {
				case '*':
					/* Default partition */
					dk_setroot(od, -1);
					return 0;
				default:
					if (name[l] >= 'a'
					    && name[l] < 'a' + MAXPARTITIONS) {
						/* specified partition */
						dk_setroot(od, name[l] - 'a');
						return 0;
					}
					break;
				}
			}
		}
	}
	/*
	 * Now try the OpenFirmware name
	 */
	l = strlen(name);
	for (cp = name + l; --cp >= name;)
		if (*cp == '/' || *cp == ':')
			break;
	if (cp >= name && *cp == ':')
		*cp++ = 0;
	else
		cp = name + l;
	part = *cp >= 'a' && *cp < 'a' + MAXPARTITIONS
		? *cp - 'a'
		: -1;
	while (cp > name && cp[-1] != '@' && cp[-1] != '/')
		--cp;
	if (cp > name && cp[-1] == '@') {
		for (unit = 0; *++cp >= '0' && *cp <= '9';)
			unit = unit * 10 + *cp - '0';
	} else
		unit = -1;

	if ((phandle = OF_finddevice(name)) != -1) {
		for (od = ofb_head.lh_first; od; od = od->ofb_list.le_next) {
			if (phandle == od->ofb_phandle) {
				/* Check for matching units */
				if (od->ofb_dk &&
				    unit != -1 &&
				    od->ofb_unit != unit)
					continue;
				dk_setroot(od, part);
				return 0;
			}
		}
	}
	return ENODEV;
}
#endif /* __BROKEN_DK_ESTABLISH */
