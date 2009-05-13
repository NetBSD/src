/*	$NetBSD: x86_autoconf.c,v 1.35.8.1 2009/05/13 17:18:45 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: x86_autoconf.c,v 1.35.8.1 2009/05/13 17:18:45 jym Exp $");

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

#include <machine/bootinfo.h>
#include <machine/pio.h>

#include "pci.h"
#include "genfb.h"
#include "wsdisplay.h"

#include <dev/isa/isavar.h>
#if NPCI > 0
#include <dev/pci/pcivar.h>
#endif
#include <dev/wsfb/genfbvar.h>
#include <dev/ic/vgareg.h>

struct genfb_colormap_callback gfb_cb;

struct disklist *x86_alldisks;
int x86_ndisks;

static void
x86_genfb_set_mapreg(void *opaque, int index, int r, int g, int b)
{
	outb(0x3c0 + VGA_DAC_ADDRW, index);
	outb(0x3c0 + VGA_DAC_PALETTE, (uint8_t)r >> 2);
	outb(0x3c0 + VGA_DAC_PALETTE, (uint8_t)g >> 2);
	outb(0x3c0 + VGA_DAC_PALETTE, (uint8_t)b >> 2);
}

static void
handle_wedges(device_t dv, int par)
{
	if (config_handle_wedges(dv, par) == 0)
		return;
	booted_device = dv;
	booted_partition = par;
}

static int
is_valid_disk(device_t dv)
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
	device_t dv;
	int i, ck, error, m, n;
	struct vnode *tv;
	char mbr[DEV_BSIZE];
	int dklist_size;
	int numbig;

	big = lookup_bootinfo(BTINFO_BIOSGEOM);

	numbig = big ? big->num : 0;

	/* First, count all native disks. */
	TAILQ_FOREACH(dv, &alldevs, dv_list) {
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
	TAILQ_FOREACH(dv, &alldevs, dv_list) {
		if (device_class(dv) != DV_DISK)
			continue;
#ifdef GEOM_DEBUG
		printf("matchbiosdisks: trying to match (%s) %s\n",
		    device_xname(dv), device_cfdata(dv)->cf_name);
#endif
		if (is_valid_disk(dv)) {
			n++;
			/* XXXJRT why not just dv_xname?? */
			snprintf(x86_alldisks->dl_nativedisks[n].ni_devname,
			    sizeof(x86_alldisks->dl_nativedisks[n].ni_devname),
			    "%s", device_xname(dv));

			if ((tv = opendisk(dv)) == NULL)
				continue;

			error = vn_rdwr(UIO_READ, tv, mbr, DEV_BSIZE, 0,
			    UIO_SYSSPACE, 0, NOCRED, NULL, NULL);
			VOP_CLOSE(tv, FREAD, NOCRED);
			if (error) {
#ifdef GEOM_DEBUG
				printf("matchbiosdisks: %s: MBR read failure\n",
				    device_xname(dv));
#endif
				continue;
			}

			for (ck = i = 0; i < DEV_BSIZE; i++)
				ck += mbr[i];
			for (m = i = 0; i < numbig; i++) {
				be = &big->disk[i];
#ifdef GEOM_DEBUG
				printf("match %s with %d "
				    "dev ck %x bios ck %x\n", device_xname(dv), i,
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
					    be->dev, device_xname(dv));
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
	VOP_CLOSE(tmpvn, FREAD, NOCRED);
	vput(tmpvn);
	return (found);
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

	error = VOP_IOCTL(tmpvn, DIOCGDINFO, &label, FREAD, NOCRED);
	if (error) {
		/*
		 * XXX Can't happen -- open() would have errored out
		 * or faked one up.
		 */
		printf("findroot: can't get label for dev %s (%d)\n",
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
	struct btinfo_rootdevice *biv;
	struct btinfo_bootdisk *bid;
	struct btinfo_bootwedge *biw;
	struct btinfo_biosgeom *big;
	device_t dv;

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
		TAILQ_FOREACH(dv, &alldevs, dv_list) {
			cfdata_t cd;
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
		TAILQ_FOREACH(dv, &alldevs, dv_list) {
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
				    device_xname(booted_device),
				    device_xname(dv));
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
		TAILQ_FOREACH(dv, &alldevs, dv_list) {
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
				    device_xname(booted_device),
				    device_xname(dv));
				continue;
			}
			handle_wedges(dv, bid->partition);
		}

		if (booted_device)
			return;

		/*
		 * No booted device found; check CD-ROM boot at last.
		 *
		 * Our bootloader assumes CD-ROM boot if biosdev is larger
		 * than the number of hard drives recognized by the BIOS.
		 * The number of drives can be found in BTINFO_BIOSGEOM here.
		 *
		 * See src/sys/arch/i386/stand/boot/devopen.c and
		 * src/sys/arch/i386/stand/lib/bootinfo_biosgeom.c .
		 */
		if ((big = lookup_bootinfo(BTINFO_BIOSGEOM)) != NULL &&
		    bid->biosdev > 0x80 + big->num) {
			/*
			 * XXX
			 * There is no proper way to detect which unit is
			 * recognized as a bootable CD-ROM drive by the BIOS.
			 * Assume the first unit is the one.
			 */
			TAILQ_FOREACH(dv, &alldevs, dv_list) {
				if (device_class(dv) == DV_DISK &&
				    device_is_a(dv, "cd")) {
					booted_device = dv;
					booted_partition = 0;
					break;
				}
			}
		}
	}
}

void
cpu_rootconf(void)
{

	findroot();
	matchbiosdisks();

	if (booted_wedge) {
		KASSERT(booted_device != NULL);
		printf("boot device: %s (%s)\n",
		    device_xname(booted_wedge), device_xname(booted_device));
		setroot(booted_wedge, 0);
	} else {
		printf("boot device: %s\n",
		    booted_device ? device_xname(booted_device) : "<unknown>");
		setroot(booted_device, booted_partition);
	}
}

void
device_register(device_t dev, void *aux)
{
	static bool found_console = false;

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
#if NPCI > 0
	if (device_parent(dev) && device_is_a(device_parent(dev), "pci") &&
	    found_console == false) {
		struct btinfo_framebuffer *fbinfo;
		struct pci_attach_args *pa = aux;
		prop_dictionary_t dict;

		if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY) {
#if NWSDISPLAY > 0 && NGENFB > 0
			extern struct vcons_screen x86_genfb_console_screen;
#endif

			fbinfo = lookup_bootinfo(BTINFO_FRAMEBUFFER);
			if (fbinfo == NULL || fbinfo->physaddr == 0)
				return;
			dict = device_properties(dev);
			prop_dictionary_set_uint32(dict, "width",
			    fbinfo->width);
			prop_dictionary_set_uint32(dict, "height",
			    fbinfo->height);
			prop_dictionary_set_uint8(dict, "depth",
			    fbinfo->depth);
			prop_dictionary_set_uint64(dict, "address",
			    fbinfo->physaddr);
			prop_dictionary_set_uint16(dict, "linebytes",
			    fbinfo->stride);
			prop_dictionary_set_bool(dict, "is_console", true);
			prop_dictionary_set_bool(dict, "clear-screen", false);
#if NWSDISPLAY > 0 && NGENFB > 0
			prop_dictionary_set_uint16(dict, "cursor-row",
			    x86_genfb_console_screen.scr_ri.ri_crow);
#endif
#if notyet
			prop_dictionary_set_bool(dict, "splash",
			    fbinfo->flags & BI_FB_SPLASH ? true : false);
#endif
			if (fbinfo->depth == 8) {
				gfb_cb.gcc_cookie = NULL;
				gfb_cb.gcc_set_mapreg = x86_genfb_set_mapreg;
				prop_dictionary_set_uint64(dict,
				    "cmap_callback", (uint64_t)&gfb_cb);
			}
			found_console = true;
			return;
		}
	}
#endif
	return;

 found:
	if (booted_device) {
		/* XXX should be a panic() */
		printf("WARNING: double match for boot device (%s, %s)\n",
		    device_xname(booted_device), device_xname(dev));
		return;
	}
	booted_device = dev;
}
