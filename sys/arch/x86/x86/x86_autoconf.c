/*	$NetBSD: x86_autoconf.c,v 1.26 2007/06/24 01:43:34 dyoung Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#ifdef COMPAT_OLDBOOT
#include <sys/reboot.h>
#endif
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/md5.h>
#include <sys/kauth.h>

#include <machine/bootinfo.h>

#include "pci.h"

#include <dev/isa/isavar.h>
#if NPCI > 0
#include <dev/pci/pcivar.h>
#endif

struct disklist *x86_alldisks;
int x86_ndisks;

static void
handle_wedges(struct device *dv, int par)
{
	if (config_handle_wedges(dv, par) == 0)
		return;
	booted_device = dv;
	booted_partition = par;
}

static int
is_valid_disk(struct device *dv)
{

	if (device_class(dv) != DV_DISK)
		return (0);
	
	return (device_is_a(dv, "dk") ||
		device_is_a(dv, "sd") ||
		device_is_a(dv, "wd") ||
		device_is_a(dv, "ld") ||
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
	struct device *dv;
	int i, ck, error, m, n;
	struct vnode *tv;
	char mbr[DEV_BSIZE];
	int dklist_size;
	int numbig;

	big = lookup_bootinfo(BTINFO_BIOSGEOM);

	numbig = big ? big->num : 0;

	/* First, count all native disks. */
	for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
	     dv = TAILQ_NEXT(dv, dv_list)) {
		if (is_valid_disk(dv))
			x86_ndisks++;
	}

	dklist_size = sizeof(struct disklist) + (x86_ndisks - 1) *
	    sizeof(struct nativedisk_info);

	/* XXX M_TEMP is wrong */
	x86_alldisks = malloc(dklist_size, M_TEMP, M_NOWAIT | M_ZERO);
	if (x86_alldisks == NULL)
		return;

	x86_alldisks->dl_nnativedisks = x86_ndisks;
	x86_alldisks->dl_nbiosdisks = numbig;
	for (i = 0; i < numbig; i++) {
		x86_alldisks->dl_biosdisks[i].bi_dev = big->disk[i].dev;
		x86_alldisks->dl_biosdisks[i].bi_sec = big->disk[i].sec;
		x86_alldisks->dl_biosdisks[i].bi_head = big->disk[i].head;
		x86_alldisks->dl_biosdisks[i].bi_cyl = big->disk[i].cyl;
		x86_alldisks->dl_biosdisks[i].bi_lbasecs = big->disk[i].totsec;
		x86_alldisks->dl_biosdisks[i].bi_flags = big->disk[i].flags;
#ifdef GEOM_DEBUG
#ifdef notyet
		printf("disk %x: flags %x, interface %x, device %llx\n",
		       big->disk[i].dev, big->disk[i].flags,
		       big->disk[i].interface_path, big->disk[i].device_path);
#endif
#endif
	}

	/* XXX Code duplication from findroot(). */
	n = -1;
	for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
	     dv = TAILQ_NEXT(dv, dv_list)) {
		if (device_class(dv) != DV_DISK)
			continue;
#ifdef GEOM_DEBUG
		printf("matchbiosdisks: trying to match (%s) %s\n",
		    dv->dv_xname, device_cfdata(dv)->cf_name);
#endif
		if (is_valid_disk(dv)) {
			n++;
			/* XXXJRT why not just dv_xname?? */
			snprintf(x86_alldisks->dl_nativedisks[n].ni_devname,
			    sizeof(x86_alldisks->dl_nativedisks[n].ni_devname),
			    "%s", dv->dv_xname);

			if ((tv = opendisk(dv)) == NULL)
				continue;

			error = vn_rdwr(UIO_READ, tv, mbr, DEV_BSIZE, 0,
			    UIO_SYSSPACE, 0, NOCRED, NULL, NULL);
			VOP_CLOSE(tv, FREAD, NOCRED, 0);
			if (error) {
#ifdef GEOM_DEBUG
				printf("matchbiosdisks: %s: MBR read failure\n",
				    dv->dv_xname);
#endif
				continue;
			}

			for (ck = i = 0; i < DEV_BSIZE; i++)
				ck += mbr[i];
			for (m = i = 0; i < numbig; i++) {
				be = &big->disk[i];
#ifdef GEOM_DEBUG
				printf("match %s with %d "
				    "dev ck %x bios ck %x\n", dv->dv_xname, i,
				    ck, be->cksum);
#endif
				if (be->flags & BI_GEOM_INVALID)
					continue;
				if (be->cksum == ck &&
				    memcmp(&mbr[MBR_PART_OFFSET], be->dosparts,
				        MBR_PART_COUNT *
					  sizeof(struct mbr_partition)) == 0) {
#ifdef GEOM_DEBUG
					printf("matched BIOS disk %x with %s\n",
					    be->dev, dv->dv_xname);
#endif
					x86_alldisks->dl_nativedisks[n].
					    ni_biosmatches[m++] = i;
				}
			}
			x86_alldisks->dl_nativedisks[n].ni_nmatches = m;
			vput(tv);
		}
	}
}

/*
 * Helper function for findroot():
 * Return non-zero if wedge device matches bootinfo.
 */
static int
match_bootwedge(struct device *dv, struct btinfo_bootwedge *biw)
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
	if (biw->matchblk == -1)
		return (0);

	if ((tmpvn = opendisk(dv)) == NULL)
		return 0;

	MD5Init(&ctx);
	for (blk = biw->matchblk, nblks = biw->matchnblks;
	     nblks != 0; nblks--, blk++) {
		error = vn_rdwr(UIO_READ, tmpvn, (void *) bf,
		    sizeof(bf), blk * DEV_BSIZE, UIO_SYSSPACE,
		    0, NOCRED, NULL, NULL);
		if (error) {
			printf("findroot: unable to read block %" PRIu64 "\n",
			    blk);
			goto closeout;
		}
		MD5Update(&ctx, bf, sizeof(bf));
	}
	MD5Final(hash, &ctx);

	/* Compare with the provided hash. */
	found = memcmp(biw->matchhash, hash, sizeof(hash)) == 0;

 closeout:
	VOP_CLOSE(tmpvn, FREAD, NOCRED, 0);
	vput(tmpvn);
	return (found);
}

/*
 * Helper function for findroot():
 * Return non-zero if disk device matches bootinfo.
 */
static int
match_bootdisk(struct device *dv, struct btinfo_bootdisk *bid)
{
	struct vnode *tmpvn;
	int error;
	struct disklabel label;
	int found = 0;

	if (device_is_a(dv, "dk"))
		return 0;

	/*
	 * A disklabel is required here.  The boot loader doesn't refuse
	 * to boot from a disk without a label, but this is normally not
	 * wanted.
	 */
	if (bid->labelsector == -1)
		return (0);
	
	if ((tmpvn = opendisk(dv)) == NULL)
		return 0;

	error = VOP_IOCTL(tmpvn, DIOCGDINFO, &label, FREAD, NOCRED, 0);
	if (error) {
		/*
		 * XXX Can't happen -- open() would have errored out
		 * or faked one up.
		 */
		printf("findroot: can't get label for dev %s (%d)\n",
		    dv->dv_xname, error);
		goto closeout;
	}

	/* Compare with our data. */
	if (label.d_type == bid->label.type &&
	    label.d_checksum == bid->label.checksum &&
	    strncmp(label.d_packname, bid->label.packname, 16) == 0)
		found = 1;

 closeout:
	VOP_CLOSE(tmpvn, FREAD, NOCRED, 0);
	vput(tmpvn);
	return (found);
}

#ifdef __x86_64__
/* Old style bootdev never existed on amd64. */
#undef COMPAT_OLDBOOT
#endif

#ifdef COMPAT_OLDBOOT
uint32_t bootdev = 0;
#endif

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
	struct device *dv;
#ifdef COMPAT_OLDBOOT
	const char *name;
	int majdev, unit, part;
	char bf[32];
#endif

	if (booted_device)
		return;
	
	if (lookup_bootinfo(BTINFO_NETIF) != NULL) {
		/*
		 * We got netboot interface information, but device_register()
		 * failed to match it to a configured device.  Boot disk
		 * information cannot be present at the same time, so give
		 * up.
		 */
		printf("findroot: netboot interface not found.\n");
		return;
	}

	if ((biv = lookup_bootinfo(BTINFO_ROOTDEVICE)) != NULL) {
		for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
		     dv = TAILQ_NEXT(dv, dv_list)) {
			struct cfdata *cd;
			size_t len;

			if (device_class(dv) != DV_DISK)
				continue;

			cd = device_cfdata(dv);
			len = strlen(cd->cf_name);

			if (strncmp(cd->cf_name, biv->devname, len) == 0 &&
			    biv->devname[len] - '0' == cd->cf_unit) {
				handle_wedges(dv, biv->devname[len + 1] - 'a');
				return;
			}
		}
	}

	if ((biw = lookup_bootinfo(BTINFO_BOOTWEDGE)) != NULL) {
		/*
		 * Scan all disk devices for ones that match the passed data.
		 * Don't break if one is found, to get possible multiple
		 * matches - for problem tracking.  Use the first match anyway
		 * because lower devices numbers are more likely to be the
		 * boot device.
		 */
		for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
		     dv = TAILQ_NEXT(dv, dv_list)) {
			if (device_class(dv) != DV_DISK)
				continue;

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
				printf("WARNING: double match for boot "
				    "device (%s, %s)\n",
				    booted_device->dv_xname, dv->dv_xname);
				continue;
			}
			dkwedge_set_bootwedge(dv, biw->startblk, biw->nblks);
		}

		if (booted_wedge)
			return;
	}

	if ((bid = lookup_bootinfo(BTINFO_BOOTDISK)) != NULL) {
		/*
		 * Scan all disk devices for ones that match the passed data.
		 * Don't break if one is found, to get possible multiple
		 * matches - for problem tracking.  Use the first match anyway
		 * because lower device numbers are more likely to be the
		 * boot device.
		 */
		for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
		     dv = TAILQ_NEXT(dv, dv_list)) {
			if (device_class(dv) != DV_DISK)
				continue;

			if (device_is_a(dv, "fd")) {
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
				printf("WARNING: double match for boot "
				    "device (%s, %s)\n",
				    booted_device->dv_xname, dv->dv_xname);
				continue;
			}
			handle_wedges(dv, bid->partition);
		}

		if (booted_device)
			return;
	}

#ifdef COMPAT_OLDBOOT
#if 0
	printf("findroot: howto %x bootdev %x\n", boothowto, bootdev);
#endif
	
	if ((bootdev & B_MAGICMASK) != B_DEVMAGIC)
		return;
	
	majdev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
	name = devsw_blk2name(majdev);
	if (name == NULL)
		return;
	
	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;

	snprintf(bf, sizeof(bf), "%s%d", name, unit);
	for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
	     dv = TAILQ_NEXT(dv, dv_list)) {
		if (strcmp(bf, dv->dv_xname) == 0) {
			booted_device = dv;
			booted_partition = part;
			return;
		}
	}
#endif /* COMPAT_OLDBOOT */
}

void
cpu_rootconf(void)
{

	findroot();
	matchbiosdisks();

	if (booted_wedge) {
		KASSERT(booted_device != NULL);
		printf("boot device: %s (%s)\n",
		    booted_wedge->dv_xname, booted_device->dv_xname);
		setroot(booted_wedge, 0);
	} else {
		printf("boot device: %s\n",
		    booted_device ? booted_device->dv_xname : "<unknown>");
		setroot(booted_device, booted_partition);
	}
}

void
device_register(struct device *dev, void *aux)
{

	/*
	 * Handle network interfaces here, the attachment information is
	 * not available driver-independently later.
	 *
	 * For disks, there is nothing useful available at attach time.
	 */
	if (device_class(dev) == DV_IFNET) {
		struct btinfo_netif *bin = lookup_bootinfo(BTINFO_NETIF);
		if (bin == NULL)
			return;

		/*
		 * We don't check the driver name against the device name
		 * passed by the boot ROM.  The ROM should stay usable if
		 * the driver becomes obsolete.  The physical attachment
		 * information (checked below) must be sufficient to
		 * idenfity the device.
		 */
		if (bin->bus == BI_BUS_ISA &&
		    device_is_a(device_parent(dev), "isa")) {
			struct isa_attach_args *iaa = aux;

			/* Compare IO base address */
			/* XXXJRT What about multiple IO addrs? */
			if (iaa->ia_nio > 0 &&
			    bin->addr.iobase == iaa->ia_io[0].ir_addr)
			    	goto found;
		}
#if NPCI > 0
		if (bin->bus == BI_BUS_PCI &&
		    device_is_a(device_parent(dev), "pci")) {
			struct pci_attach_args *paa = aux;
			int b, d, f;

			/*
			 * Calculate BIOS representation of:
			 *
			 *	<bus,device,function>
			 *
			 * and compare.
			 */
			pci_decompose_tag(paa->pa_pc, paa->pa_tag, &b, &d, &f);
			if (bin->addr.tag == ((b << 8) | (d << 3) | f))
				goto found;
		}
#endif /* NPCI > 0 */
	}
	return;

 found:
	if (booted_device) {
		/* XXX should be a panic() */
		printf("WARNING: double match for boot device (%s, %s)\n",
		    booted_device->dv_xname, dev->dv_xname);
		return;
	}
	booted_device = dev;
}
