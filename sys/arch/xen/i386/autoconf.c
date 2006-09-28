/*	$NetBSD: autoconf.c,v 1.18 2006/09/28 18:53:15 bouyer Exp $	*/
/*	NetBSD: autoconf.c,v 1.75 2003/12/30 12:33:22 pk Exp 	*/

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

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.18 2006/09/28 18:53:15 bouyer Exp $");

#include "opt_xen.h"
#include "opt_compat_oldboot.h"
#include "opt_multiprocessor.h"
#include "opt_nfs_boot.h"
#include "xennet_hypervisor.h"
#include "xennet_xenbus.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#ifdef COMPAT_OLDBOOT
#include <sys/reboot.h>
#endif
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/dkio.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kauth.h>

#ifdef NFS_BOOT_BOOTSTATIC
#include <net/if.h>
#include <net/if_ether.h>
#include <netinet/in.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsdiskless.h>
#include <machine/if_xennetvar.h>
#endif

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/gdt.h>
#include <machine/pcb.h>
#include <machine/bootinfo.h>

static int match_harddisk(struct device *, struct btinfo_bootdisk *);
static void matchbiosdisks(void);
static void findroot(void);
static int is_valid_disk(struct device *);

struct disklist *x86_alldisks;
int x86_ndisks;

#include "bios32.h"
#if NBIOS32 > 0
#include <machine/bios32.h>
#endif

#include "opt_pcibios.h"
#ifdef PCIBIOS
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <i386/pci/pcibios.h>
#endif

#include "opt_kvm86.h"
#ifdef KVM86
#include <machine/kvm86.h>
#endif

#include "opt_xen.h"

/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure(void)
{

	startrtclock();

#if NBIOS32 > 0
	bios32_init();
#endif
#ifdef PCIBIOS
	pcibios_init();
#endif

	/* kvm86 needs a TSS */
	i386_proc0_tss_ldt_init();
#ifdef KVM86
	kvm86_init();
#endif

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

#ifdef INTRDEBUG
	intr_printconfig();
#endif

	/* resync cr0 after FPU configuration */
	lwp0.l_addr->u_pcb.pcb_cr0 = rcr0();
#ifdef MULTIPROCESSOR
	/* propagate this to the idle pcb's. */
	cpu_init_idle_pcbs();
#endif

	spl0();
}

void
cpu_rootconf(void)
{
	findroot();
	matchbiosdisks();

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

/*
 * XXX ugly bit of code. But, this is the only safe time that the
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
	int  dklist_size;
	int bmajor;

	big = lookup_bootinfo(BTINFO_BIOSGEOM);

	if (big == NULL)
		return;

	/*
	 * First, count all native disks
	 */
	for (dv = alldevs.tqh_first; dv != NULL; dv = dv->dv_list.tqe_next)
		if (is_valid_disk(dv))
			x86_ndisks++;

	if (x86_ndisks == 0)
		return;

	dklist_size = sizeof (struct disklist) + (x86_ndisks - 1) *
	    sizeof (struct nativedisk_info);

	/* XXX M_TEMP is wrong */
	x86_alldisks = malloc(dklist_size, M_TEMP, M_NOWAIT);
	if (x86_alldisks == NULL)
		return;

	memset(x86_alldisks, 0, dklist_size);

	x86_alldisks->dl_nnativedisks = x86_ndisks;
	x86_alldisks->dl_nbiosdisks = big->num;
	for (i = 0; i < big->num; i++) {
		x86_alldisks->dl_biosdisks[i].bi_dev = big->disk[i].dev;
		x86_alldisks->dl_biosdisks[i].bi_sec = big->disk[i].sec;
		x86_alldisks->dl_biosdisks[i].bi_head = big->disk[i].head;
		x86_alldisks->dl_biosdisks[i].bi_cyl = big->disk[i].cyl;
		x86_alldisks->dl_biosdisks[i].bi_lbasecs = big->disk[i].totsec;
		x86_alldisks->dl_biosdisks[i].bi_flags = big->disk[i].flags;
#ifdef GEOM_DEBUG
#ifdef NOTYET
		printf("disk %x: flags %x, interface %x, device %llx\n",
			big->disk[i].dev, big->disk[i].flags,
			big->disk[i].interface_path, big->disk[i].device_path);
#endif
#endif
	}

	/*
	 * XXX code duplication from findroot()
	 */
	n = -1;
	for (dv = alldevs.tqh_first; dv != NULL; dv = dv->dv_list.tqe_next) {
		if (device_class(dv) != DV_DISK)
			continue;
#ifdef GEOM_DEBUG
		printf("matchbiosdisks: trying to match (%s) %s\n",
		    dv->dv_xname, device_cfdata(dv)->cf_name);
#endif
		if (is_valid_disk(dv)) {
			n++;
			sprintf(x86_alldisks->dl_nativedisks[n].ni_devname,
			    "%s", dv->dv_xname);

			bmajor = devsw_name2blk(dv->dv_xname, NULL, 0);
			if (bmajor == -1)
				return;

			if (bdevvp(MAKEDISKDEV(bmajor, device_unit(dv),
				   RAW_PART), &tv))
				panic("matchbiosdisks: can't alloc vnode");

			error = VOP_OPEN(tv, FREAD, NOCRED, 0);
			if (error) {
				vput(tv);
				continue;
			}
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
			for (m = i = 0; i < big->num; i++) {
				be = &big->disk[i];
#ifdef GEOM_DEBUG
				printf("match %s with %d ", dv->dv_xname, i);
				printf("dev ck %x bios ck %x\n", ck, be->cksum);
#endif
				if (be->flags & BI_GEOM_INVALID)
					continue;
				if (be->cksum == ck &&
				    !memcmp(&mbr[MBR_PART_OFFSET], be->dosparts,
					MBR_PART_COUNT *
					    sizeof (struct mbr_partition))) {
#ifdef GEOM_DEBUG
					printf("matched bios disk %x with %s\n",
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

#ifdef COMPAT_OLDBOOT
u_long	bootdev = 0;		/* should be dev_t, but not until 32 bits */
#endif

/*
 * helper function for "findroot()":
 * return nonzero if disk device matches bootinfo
 */
static int
match_harddisk(struct device *dv, struct btinfo_bootdisk *bid)
{
	struct vnode *tmpvn;
	int error;
	struct disklabel label;
	int found = 0;
	int bmajor;

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
	bmajor = devsw_name2blk(dv->dv_xname, NULL, 0);
	if (bmajor == -1)
		return(0); /* XXX panic() ??? */

	/*
	 * Fake a temporary vnode for the disk, open
	 * it, and read the disklabel for comparison.
	 */
	if (bdevvp(MAKEDISKDEV(bmajor, device_unit(dv), bid->partition),
			       &tmpvn))
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
		vput(tmpvn);
		return(0);
	}
	error = VOP_IOCTL(tmpvn, DIOCGDINFO, &label, FREAD, NOCRED, 0);
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
	vput(tmpvn);
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
	union xen_cmdline_parseinfo xcp;
#ifdef COMPAT_OLDBOOT
	int i, majdev, unit, part;
	char buf[32];
#endif

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
			if (device_class(dv) != DV_DISK)
				continue;

			if (device_is_a(dv, "fd")) {
				/*
				 * Assume the configured unit number matches
				 * the BIOS device number.  (This is the old
				 * behaviour.)  Needs some ideas how to handle
				 * BIOS's "swap floppy drive" options.
				 */
				/* XXX device_unit() abuse */
				if ((bid->biosdev & 0x80) ||
				    device_unit(dv) != bid->biosdev)
					continue;

				goto found;
			}

			if (is_valid_disk(dv)) {
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

			/* no "fd", "wd", "sd", "ld", "ed" */
			continue;

found:
			if (booted_device) {
				printf("warning: double match for boot "
				    "device (%s, %s)\n",
				    booted_device->dv_xname, dv->dv_xname);
				continue;
			}
			booted_device = dv;
			booted_partition = bid->partition;
		}

		if (booted_device)
			return;
	}

	xen_parse_cmdline(XEN_PARSE_BOOTDEV, &xcp);

	for (dv = alldevs.tqh_first; dv != NULL; dv = dv->dv_list.tqe_next) {
		if (is_valid_disk(dv) == 0)
			continue;

		if (xcp.xcp_bootdev[0] == 0) {
			booted_device = dv;
			break;
		}

		if (strncmp(xcp.xcp_bootdev, dv->dv_xname,
		    strlen(dv->dv_xname)))
			continue;

		if (strlen(xcp.xcp_bootdev) > strlen(dv->dv_xname)) {
			booted_partition = toupper(
				xcp.xcp_bootdev[strlen(dv->dv_xname)]) - 'A';
		}

		booted_device = dv;
		break;
	}

	if (booted_device)
		return;

#ifdef COMPAT_OLDBOOT
#if 0
	printf("howto %x bootdev %x ", boothowto, bootdev);
#endif

	if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;

	majdev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
	name = devsw_blk2name(majdev);
	if (name == NULL)
		return;

	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;

	sprintf(buf, "%s%d", name, unit);
	for (dv = alldevs.tqh_first; dv != NULL; dv = dv->dv_list.tqe_next) {
		if (strcmp(buf, dv->dv_xname) == 0) {
			booted_device = dv;
			booted_partition = part;
			return;
		}
	}
#endif
}

#include "pci.h"

#include <dev/isa/isavar.h>
#if NPCI > 0
#include <dev/pci/pcivar.h>
#endif

void
device_register(struct device *dev, void *aux)
{
	/*
	 * Handle network interfaces here, the attachment information is
	 * not available driver independantly later.
	 * For disks, there is nothing useful available at attach time.
	 */
#if NXENNET_HYPERVISOR > 0 || NXENNET_XENBUS > 0
	if (device_class(dev) == DV_IFNET) {
		union xen_cmdline_parseinfo xcp;

		xen_parse_cmdline(XEN_PARSE_BOOTDEV, &xcp);
		if (strncmp(xcp.xcp_bootdev, dev->dv_xname, 16) == 0) {
#ifdef NFS_BOOT_BOOTSTATIC
			nfs_bootstatic_callback = xennet_bootstatic_callback;
#endif
			goto found;
		}
	}
#endif
	if (device_class(dev) == DV_IFNET) {
		struct btinfo_netif *bin = lookup_bootinfo(BTINFO_NETIF);
		if (bin == NULL)
			return;

		/*
		 * We don't check the driver name against the device name
		 * passed by the boot ROM. The ROM should stay usable
		 * if the driver gets obsoleted.
		 * The physical attachment information (checked below)
		 * must be sufficient to identify the device.
		 */

		if (bin->bus == BI_BUS_ISA &&
		    device_is_a(device_parent(dev), "isa")) {
			struct isa_attach_args *iaa = aux;

			/* compare IO base address */
			/* XXXJRT what about multiple I/O addrs? */
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
#endif
	}
	return;

found:
	if (booted_device) {
		/* XXX should be a "panic()" */
		printf("warning: double match for boot device (%s, %s)\n",
		    booted_device->dv_xname, dev->dv_xname);
		return;
	}
	booted_device = dev;
}

static int
is_valid_disk(struct device *dv)
{

	if (device_class(dv) != DV_DISK)
		return (0);

	return (device_is_a(dv, "sd") ||
		device_is_a(dv, "wd") ||
		device_is_a(dv, "ld") ||
		device_is_a(dv, "ed") ||
		device_is_a(dv, "xbd"));
}
