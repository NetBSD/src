/*	$NetBSD: x86_autoconf.c,v 1.86 2022/02/12 03:24:35 riastradh Exp $	*/

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
 *	@(#)autoconf.c	7.1 (Berkeley) 5/9/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: x86_autoconf.c,v 1.86 2022/02/12 03:24:35 riastradh Exp $");

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
#include <sys/md5.h>
#include <sys/kauth.h>

#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/pio.h>

#include <xen/xen.h>

#include <dev/i2c/i2cvar.h>

#include "acpica.h"
#include "wsdisplay.h"
#ifndef XENPV
#include "hyperv.h"
#endif

#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#endif
#if NHYPERV > 0
#include <x86/x86/hypervvar.h>
#endif

struct disklist *x86_alldisks;
int x86_ndisks;
int x86_found_console;

#ifdef DEBUG_GEOM
#define DPRINTF(a) printf a
#else
#define DPRINTF(a)
#endif

static void
dmatch(const char *func, device_t dv, const char *method)
{

	printf("WARNING: %s: double match for boot device (%s:%s %s:%s)\n",
	    func, booted_method, device_xname(booted_device),
	    method, device_xname(dv));
}

static int
is_valid_disk(device_t dv)
{

	if (device_class(dv) != DV_DISK)
		return 0;

	return (device_is_a(dv, "dk") ||
		device_is_a(dv, "sd") ||
		device_is_a(dv, "wd") ||
		device_is_a(dv, "ld") ||
		device_is_a(dv, "xbd") ||
		device_is_a(dv, "ed"));
}

/*
 * XXX Ugly bit of code.  But, this is the only safe time that the
 * match between BIOS disks and native disks can be done.
 */
static void
matchbiosdisks(void)
{
	struct btinfo_biosgeom *big;
	struct bi_biosgeom_entry *be;
	device_t dv;
	deviter_t di;
	int i, ck, error, m, n;
	struct vnode *tv;
	char mbr[DEV_BSIZE];
	int dklist_size;
	int numbig;

	if (x86_ndisks)
		return;
	big = lookup_bootinfo(BTINFO_BIOSGEOM);

	numbig = big ? big->num : 0;

	/* First, count all native disks. */
	for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST); dv != NULL;
	     dv = deviter_next(&di)) {
		if (is_valid_disk(dv))
			x86_ndisks++;
	}
	deviter_release(&di);

	dklist_size = sizeof(struct disklist) + (x86_ndisks - 1) *
	    sizeof(struct nativedisk_info);

	/* XXX M_TEMP is wrong */
	x86_alldisks = malloc(dklist_size, M_TEMP, M_WAITOK | M_ZERO);
	x86_alldisks->dl_nnativedisks = x86_ndisks;
	x86_alldisks->dl_nbiosdisks = numbig;
	for (i = 0; i < numbig; i++) {
		x86_alldisks->dl_biosdisks[i].bi_dev = big->disk[i].dev;
		x86_alldisks->dl_biosdisks[i].bi_sec = big->disk[i].sec;
		x86_alldisks->dl_biosdisks[i].bi_head = big->disk[i].head;
		x86_alldisks->dl_biosdisks[i].bi_cyl = big->disk[i].cyl;
		x86_alldisks->dl_biosdisks[i].bi_lbasecs = big->disk[i].totsec;
		x86_alldisks->dl_biosdisks[i].bi_flags = big->disk[i].flags;
		DPRINTF(("%s: disk %x: flags %x",
		    __func__, big->disk[i].dev, big->disk[i].flags));
#ifdef BIOSDISK_EXTINFO_V3
		DPRINTF((", interface %x, device %llx",
		    big->disk[i].interface_path, big->disk[i].device_path));
#endif
		DPRINTF(("\n"));
	}

	/* XXX Code duplication from findroot(). */
	n = -1;
	for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST); dv != NULL;
	     dv = deviter_next(&di)) {
		if (!is_valid_disk(dv))
			continue;
		DPRINTF(("%s: trying to match (%s) %s: ", __func__,
		    device_xname(dv), device_cfdata(dv)->cf_name));
		n++;
		snprintf(x86_alldisks->dl_nativedisks[n].ni_devname,
		    sizeof(x86_alldisks->dl_nativedisks[n].ni_devname),
		    "%s", device_xname(dv));

		if ((tv = opendisk(dv)) == NULL) {
			DPRINTF(("cannot open\n"));
			continue;
		}

		error = vn_rdwr(UIO_READ, tv, mbr, DEV_BSIZE, 0, UIO_SYSSPACE,
		    0, NOCRED, NULL, NULL);
		VOP_CLOSE(tv, FREAD, NOCRED);
		vput(tv);
		if (error) {
			DPRINTF(("MBR read failure %d\n", error));
			continue;
		}

		for (ck = i = 0; i < DEV_BSIZE; i++)
			ck += mbr[i];
		for (m = i = 0; i < numbig; i++) {
			be = &big->disk[i];
			if (be->flags & BI_GEOM_INVALID)
				continue;
			DPRINTF(("matched with %d dev ck %x bios ck %x\n",
			    i, ck, be->cksum));
			if (be->cksum == ck && memcmp(&mbr[MBR_PART_OFFSET],
			    be->mbrparts, MBR_PART_COUNT
			    * sizeof(struct mbr_partition)) == 0) {
				DPRINTF(("%s: matched BIOS disk %x with %s\n",
				    __func__, be->dev, device_xname(dv)));
				x86_alldisks->dl_nativedisks[n].
				    ni_biosmatches[m++] = i;
			}
		}
		x86_alldisks->dl_nativedisks[n].ni_nmatches = m;
	}
	deviter_release(&di);
}

/*
 * Helper function for findroot():
 * Return non-zero if wedge device matches bootinfo.
 */
static int
match_bootwedge(device_t dv, struct btinfo_bootwedge *biw)
{
	MD5_CTX ctx;
	struct vnode *tmpvn;
	int error;
	uint8_t bf[DEV_BSIZE];
	uint8_t hash[16];
	int found = 0;
	daddr_t blk;
	uint64_t nblks;

	/*
	 * If the boot loader didn't specify the sector, abort.
	 */
	if (biw->matchblk == -1) {
		DPRINTF(("%s: no sector specified for %s\n", __func__,
			device_xname(dv)));
		return 0;
	}

	if ((tmpvn = opendisk(dv)) == NULL) {
		DPRINTF(("%s: can't open %s\n", __func__, device_xname(dv)));
		return 0;
	}

	MD5Init(&ctx);
	for (blk = biw->matchblk, nblks = biw->matchnblks;
	     nblks != 0; nblks--, blk++) {
		error = vn_rdwr(UIO_READ, tmpvn, (void *) bf,
		    sizeof(bf), blk * DEV_BSIZE, UIO_SYSSPACE,
		    0, NOCRED, NULL, NULL);
		if (error) {
			if (error != EINVAL) {
				aprint_error("%s: unable to read block %"
				    PRId64 " " "of dev %s (%d)\n", __func__,
				    blk, device_xname(dv), error);
			}
			goto closeout;
		}
		MD5Update(&ctx, bf, sizeof(bf));
	}
	MD5Final(hash, &ctx);

	/* Compare with the provided hash. */
	found = memcmp(biw->matchhash, hash, sizeof(hash)) == 0;
	DPRINTF(("%s: %s found=%d\n", __func__, device_xname(dv), found));

 closeout:
	VOP_CLOSE(tmpvn, FREAD, NOCRED);
	vput(tmpvn);
	return found;
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

	if (device_is_a(dv, "dk")) {
		DPRINTF(("%s: dk %s\n", __func__, device_xname(dv)));
		return 0;
	}

	/*
	 * A disklabel is required here.  The boot loader doesn't refuse
	 * to boot from a disk without a label, but this is normally not
	 * wanted.
	 */
	if (bid->labelsector == -1) {
		DPRINTF(("%s: no label %s\n", __func__, device_xname(dv)));
		return 0;
	}

	if ((tmpvn = opendisk(dv)) == NULL) {
		DPRINTF(("%s: can't open %s\n", __func__, device_xname(dv)));
		return 0;
	}

	error = VOP_IOCTL(tmpvn, DIOCGDINFO, &label, FREAD, NOCRED);
	if (error) {
		/*
		 * XXX Can't happen -- open() would have errored out
		 * or faked one up.
		 */
		printf("%s: can't get label for dev %s (%d)\n", __func__,
		    device_xname(dv), error);
		goto closeout;
	}

	/* Compare with our data. */
	if (label.d_type == bid->label.type &&
	    label.d_checksum == bid->label.checksum &&
	    strncmp(label.d_packname, bid->label.packname, 16) == 0)
		found = 1;

	DPRINTF(("%s: %s found=%d\n", __func__, device_xname(dv), found));
 closeout:
	VOP_CLOSE(tmpvn, FREAD, NOCRED);
	vput(tmpvn);
	return found;
}

/*
 * Attempt to find the device from which we were booted.  If we can do so,
 * and not instructed not to do so, change rootdev to correspond to the
 * load device.
 */
static void
findroot(void)
{
	struct btinfo_rootdevice *biv;
	struct btinfo_bootdisk *bid;
	struct btinfo_bootwedge *biw;
	struct btinfo_biosgeom *big;
	device_t dv;
	deviter_t di;
	static char bootspecbuf[sizeof(biv->devname)+1];

	if (booted_device)
		return;

	if (lookup_bootinfo(BTINFO_NETIF) != NULL) {
		/*
		 * We got netboot interface information, but device_register()
		 * failed to match it to a configured device.  Boot disk
		 * information cannot be present at the same time, so give
		 * up.
		 */
		printf("%s: netboot interface not found.\n", __func__);
		return;
	}

	if ((biv = lookup_bootinfo(BTINFO_ROOTDEVICE)) != NULL) {
		for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST);
		     dv != NULL;
		     dv = deviter_next(&di)) {
			cfdata_t cd;
			size_t len;

			if (device_class(dv) != DV_DISK)
				continue;

			cd = device_cfdata(dv);
			len = strlen(cd->cf_name);

			if (strncmp(cd->cf_name, biv->devname, len) == 0 &&
			    biv->devname[len] - '0' == device_unit(dv)) {
				booted_device = dv;
				booted_method = "bootinfo/rootdevice";
				booted_partition = biv->devname[len + 1] - 'a';
				booted_nblks = 0;
				break;
			}
		}
		DPRINTF(("%s: BTINFO_ROOTDEVICE %s\n", __func__,
		    booted_device ? device_xname(booted_device) : "not found"));
		deviter_release(&di);
		if (dv != NULL)
			return;

		if (biv->devname[0] != '\0') {
			strlcpy(bootspecbuf, biv->devname, sizeof(bootspecbuf));
			bootspec = bootspecbuf;
			return;
		}
	}

	bid = lookup_bootinfo(BTINFO_BOOTDISK);
	biw = lookup_bootinfo(BTINFO_BOOTWEDGE);

	if (biw != NULL) {
		/*
		 * Scan all disk devices for ones that match the passed data.
		 * Don't break if one is found, to get possible multiple
		 * matches - for problem tracking.  Use the first match anyway
		 * because lower devices numbers are more likely to be the
		 * boot device.
		 */
		for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST);
		     dv != NULL;
		     dv = deviter_next(&di)) {
			if (is_valid_disk(dv)) {
				/*
				 * Don't trust BIOS device numbers, try
				 * to match the information passed by the
				 * boot loader instead.
				 */
				if ((biw->biosdev & 0x80) == 0 ||
				    match_bootwedge(dv, biw) == 0)
					continue;
				goto bootwedge_found;
			}

			continue;
 bootwedge_found:
			if (booted_device) {
				dmatch(__func__, dv, "bootinfo/bootwedge");
				continue;
			}
			booted_device = dv;
			booted_method = "bootinfo/bootwedge";
			booted_partition = bid != NULL ? bid->partition : 0;
			booted_nblks = biw->nblks;
			booted_startblk = biw->startblk;
		}
		deviter_release(&di);

		DPRINTF(("%s: BTINFO_BOOTWEDGE %s\n", __func__,
		    booted_device ? device_xname(booted_device) : "not found"));
		if (booted_nblks)
			return;
	}

	if (bid != NULL) {
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

			if (device_is_a(dv, "fd") &&
			    device_class(dv) == DV_DISK) {
				/*
				 * Assume the configured unit number matches
				 * the BIOS device number.  (This is the old
				 * behavior.)  Needs some ideas how to handle
				 * the BIOS's "swap floppy drive" options.
				 */
				/* XXX device_unit() abuse */
				if ((bid->biosdev & 0x80) != 0 ||
				    device_unit(dv) != bid->biosdev)
					continue;
				goto bootdisk_found;
			}

			if (is_valid_disk(dv)) {
				/*
				 * Don't trust BIOS device numbers, try
				 * to match the information passed by the
				 * boot loader instead.
				 */
				if ((bid->biosdev & 0x80) == 0 ||
				    match_bootdisk(dv, bid) == 0)
					continue;
				goto bootdisk_found;
			}

			continue;
 bootdisk_found:
			if (booted_device) {
				dmatch(__func__, dv, "bootinfo/bootdisk");
				continue;
			}
			booted_device = dv;
			booted_method = "bootinfo/bootdisk";
			booted_partition = bid->partition;
			booted_nblks = 0;
		}
		deviter_release(&di);

		DPRINTF(("%s: BTINFO_BOOTDISK %s\n", __func__,
		    booted_device ? device_xname(booted_device) : "not found"));
		if (booted_device)
			return;

		/*
		 * No booted device found; check CD-ROM boot at last.
		 *
		 * Our bootloader assumes CD-ROM boot if biosdev is larger
		 * or equal than the number of hard drives recognized by the
		 * BIOS. The number of drives can be found in BTINFO_BIOSGEOM
		 * here. For example, if we have wd0, wd1, and cd0:
		 *
		 *	big->num = 2 (for wd0 and wd1)
		 *	bid->biosdev = 0x80 (wd0)
		 *	bid->biosdev = 0x81 (wd1)
		 *	bid->biosdev = 0x82 (cd0)
		 *
		 * See src/sys/arch/i386/stand/boot/devopen.c and
		 * src/sys/arch/i386/stand/lib/bootinfo_biosgeom.c .
		 */
		if ((big = lookup_bootinfo(BTINFO_BIOSGEOM)) != NULL &&
		    bid->biosdev >= 0x80 + big->num) {
			/*
			 * XXX
			 * There is no proper way to detect which unit is
			 * recognized as a bootable CD-ROM drive by the BIOS.
			 * Assume the first unit is the one.
			 */
			for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST);
			     dv != NULL;
			     dv = deviter_next(&di)) {
				if (device_class(dv) == DV_DISK &&
				    device_is_a(dv, "cd")) {
					booted_device = dv;
					booted_method = "bootinfo/biosgeom";
					booted_partition = 0;
					booted_nblks = 0;
					break;
				}
			}
			deviter_release(&di);
			DPRINTF(("%s: BTINFO_BIOSGEOM %s\n", __func__,
			    booted_device ? device_xname(booted_device) :
			    "not found"));
		}
	}
}

void
cpu_bootconf(void)
{
#ifdef XEN
	if (vm_guest == VM_GUEST_XENPVH) {
		xen_bootconf();
		return;
	}
#endif
	findroot();
	matchbiosdisks();
}

void
cpu_rootconf(void)
{
	cpu_bootconf();

	aprint_normal("boot device: %s\n",
	    booted_device ? device_xname(booted_device) : "<unknown>");
	rootconf();
}

void
device_register(device_t dev, void *aux)
{
	device_t isaboot, pciboot;

	/*
	 * The Intel Integrated Memory Controller has a built-in i2c
	 * controller that's rather limited in capability; it is intended
	 * only for reading memory module EERPOMs and sensors.
	 */
	if (device_is_a(dev, "iic") &&
	    device_is_a(device_parent(dev), "imcsmb")) {
		static const char *imcsmb_device_permitlist[] = {
			"spdmem",
			"sdtemp",
			NULL,
		};
		prop_array_t permitlist = prop_array_create();
		prop_dictionary_t props = device_properties(dev);
		int i;

		for (i = 0; imcsmb_device_permitlist[i] != NULL; i++) {
			prop_string_t pstr = prop_string_create_nocopy(
			    imcsmb_device_permitlist[i]);
			(void) prop_array_add(permitlist, pstr);
			prop_object_release(pstr);
		}
		(void) prop_dictionary_set(props,
					   I2C_PROP_INDIRECT_DEVICE_PERMITLIST,
					   permitlist);
		(void) prop_dictionary_set_string_nocopy(props,
					   I2C_PROP_INDIRECT_PROBE_STRATEGY,
					   I2C_PROBE_STRATEGY_NONE);
	}

	device_acpi_register(dev, aux);

	isaboot = device_isa_register(dev, aux);
	pciboot = device_pci_register(dev, aux);
#if NHYPERV > 0
	(void)device_hyperv_register(dev, aux);
#endif

	if (isaboot == NULL && pciboot == NULL)
		return;

	if (booted_device != NULL) {
		/* XXX should be a panic() */
		dmatch(__func__, dev, "device/register");
	} else {
		booted_device = (isaboot != NULL) ? isaboot : pciboot;
		booted_method = "device/register";
	}
}
