/*	$NetBSD: ofdev.c,v 1.24 2009/10/26 19:16:57 cegger Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
/*
 * Device I/O routines using Open Firmware
 */
#include <sys/param.h>
#include <sys/disklabel.h>
#ifdef NETBOOT
#include <netinet/in.h>
#endif

#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/cd9660.h>
#ifdef NETBOOT
#include <lib/libsa/nfs.h>
#include <lib/libsa/tftp.h>
#endif
#include <lib/libkern/libkern.h>

#include <dev/sun/disklabel.h>
#include <dev/raidframe/raidframevar.h>

#include <machine/promlib.h>

#include "ofdev.h"
#include "boot.h"

extern char bootdev[];

/*
 * This is ugly.  A path on a sparc machine is something like this:
 *
 *	[device] [-<options] [path] [-options] [otherstuff] [-<more options]
 *
 */

char *
filename(char *str, char *ppart)
{
	char *cp, *lp;
	char savec;
	int dhandle;
	char devtype[16];

	lp = str;
	devtype[0] = 0;
	*ppart = '\0';
	for (cp = str; *cp; lp = cp) {
		/* For each component of the path name... */
		while (*++cp && *cp != '/');
		savec = *cp;
		*cp = 0;
		/* ...look whether there is a device with this name */
		dhandle = prom_finddevice(str);
		DPRINTF(("filename: prom_finddevice(%s) returned %x\n",
		       str, dhandle));
		*cp = savec;
		if (dhandle == -1) {
			/*
			 * if not, lp is the delimiter between device and
			 * path.  if the last component was a block device.
			 */
			if (!strcmp(devtype, "block")) {
				/* search for arguments */
				DPRINTF(("filename: hunting for arguments "
				       "in %s\n", lp));
				for (cp = lp; ; ) {
					cp--;
					if (cp < str ||
					    cp[0] == '/' ||
					    (cp[0] == ' ' && (cp+1) != lp &&
					     cp[1] == '-'))
						break;
				}
				if (cp >= str && *cp == '-')
					/* found arguments, make firmware
					   ignore them */
					*cp = 0;
				for (cp = lp; *--cp && *cp != ',' 
					&& *cp != ':';)
						;
				if (cp[0] == ':' && cp[1] >= 'a' &&
				    cp[1] <= 'a' + MAXPARTITIONS) {
					*ppart = cp[1];
					cp[0] = '\0';
				}
			}
			DPRINTF(("filename: found %s\n",lp));
			return lp;
		} else if (_prom_getprop(dhandle, "device_type", devtype,
				sizeof devtype) < 0)
			devtype[0] = 0;
	}
	DPRINTF(("filename: not found\n",lp));
	return 0;
}

static int
strategy(void *devdata, int rw, daddr_t blk, size_t size, void *buf, size_t *rsize)
{
	struct of_dev *dev = devdata;
	u_quad_t pos;
	int n;

	if (rw != F_READ)
		return EPERM;
	if (dev->type != OFDEV_DISK)
		panic("strategy");

#ifdef NON_DEBUG
	printf("strategy: block %lx, partition offset %lx, blksz %lx\n",
	       (long)blk, (long)dev->partoff, (long)dev->bsize);
	printf("strategy: seek position should be: %lx\n",
	       (long)((blk + dev->partoff) * dev->bsize));
#endif
	pos = (u_quad_t)(blk + dev->partoff) * dev->bsize;

	for (;;) {
#ifdef NON_DEBUG
		printf("strategy: seeking to %lx\n", (long)pos);
#endif
		if (prom_seek(dev->handle, pos) < 0)
			break;
#ifdef NON_DEBUG
		printf("strategy: reading %lx at %p\n", (long)size, buf);
#endif
		n = prom_read(dev->handle, buf, size);
		if (n == -2)
			continue;
		if (n < 0)
			break;
		*rsize = n;
		return 0;
	}
	return EIO;
}

static int
devclose(struct open_file *of)
{
	struct of_dev *op = of->f_devdata;

#ifdef NETBOOT
	if (op->type == OFDEV_NET)
		net_close(op);
#endif
	prom_close(op->handle);
	op->handle = -1;
}

static struct devsw ofdevsw[1] = {
	"OpenFirmware",
	strategy,
	(int (*)(struct open_file *, ...))nodev,
	devclose,
	noioctl
};
int ndevs = sizeof ofdevsw / sizeof ofdevsw[0];

#ifdef SPARC_BOOT_UFS
static struct fs_ops file_system_ufs = FS_OPS(ufs);
#endif
#ifdef SPARC_BOOT_CD9660
static struct fs_ops file_system_cd9660 = FS_OPS(cd9660);
#endif
#ifdef NETBOOT
static struct fs_ops file_system_nfs = FS_OPS(nfs);
static struct fs_ops file_system_tftp = FS_OPS(tftp);
#endif

struct fs_ops file_system[3];
int nfsys;

static struct of_dev ofdev = {
	-1,
};

char opened_name[256];
int floppyboot;

static u_long
get_long(const void *p)
{
	const unsigned char *cp = p;

	return cp[0] | (cp[1] << 8) | (cp[2] << 16) | (cp[3] << 24);
}
/************************************************************************
 *
 * The rest of this was taken from arch/sparc64/scsi/sun_disklabel.c
 * and then substantially rewritten by Gordon W. Ross
 *
 ************************************************************************/

/* What partition types to assume for Sun disklabels: */
static u_char
sun_fstypes[8] = {
	FS_BSDFFS,	/* a */
	FS_SWAP,	/* b */
	FS_OTHER,	/* c - whole disk */
	FS_BSDFFS,	/* d */
	FS_BSDFFS,	/* e */
	FS_BSDFFS,	/* f */
	FS_BSDFFS,	/* g */
	FS_BSDFFS,	/* h */
};

/*
 * Given a SunOS disk label, set lp to a BSD disk label.
 * Returns NULL on success, else an error string.
 *
 * The BSD label is cleared out before this is called.
 */
static char *
disklabel_sun_to_bsd(char *cp, struct disklabel *lp)
{
	struct sun_disklabel *sl;
	struct partition *npp;
	struct sun_dkpart *spp;
	int i, secpercyl;
	u_short cksum, *sp1, *sp2;

	sl = (struct sun_disklabel *)cp;

	/* Verify the XOR check. */
	sp1 = (u_short *)sl;
	sp2 = (u_short *)(sl + 1);
	cksum = 0;
	while (sp1 < sp2)
		cksum ^= *sp1++;
	if (cksum != 0)
		return("SunOS disk label, bad checksum");

	/* Format conversion. */
	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	memcpy(lp->d_packname, sl->sl_text, sizeof(lp->d_packname));

	lp->d_secsize = 512;
	lp->d_nsectors   = sl->sl_nsectors;
	lp->d_ntracks    = sl->sl_ntracks;
	lp->d_ncylinders = sl->sl_ncylinders;

	secpercyl = sl->sl_nsectors * sl->sl_ntracks;
	lp->d_secpercyl  = secpercyl;
	lp->d_secperunit = secpercyl * sl->sl_ncylinders;

	lp->d_sparespercyl = sl->sl_sparespercyl;
	lp->d_acylinders   = sl->sl_acylinders;
	lp->d_rpm          = sl->sl_rpm;
	lp->d_interleave   = sl->sl_interleave;

	lp->d_npartitions = 8;
	/* These are as defined in <ufs/ffs/fs.h> */
	lp->d_bbsize = 8192;	/* XXX */
	lp->d_sbsize = 8192;	/* XXX */

	for (i = 0; i < 8; i++) {
		spp = &sl->sl_part[i];
		npp = &lp->d_partitions[i];
		npp->p_offset = spp->sdkp_cyloffset * secpercyl;
		npp->p_size = spp->sdkp_nsectors;
		DPRINTF(("partition %d start %x size %x\n", i, (int)npp->p_offset, (int)npp->p_size));
		if (npp->p_size == 0) {
			npp->p_fstype = FS_UNUSED;
		} else {
			npp->p_fstype = sun_fstypes[i];
			if (npp->p_fstype == FS_BSDFFS) {
				/*
				 * The sun label does not store the FFS fields,
				 * so just set them with default values here.
				 */
				npp->p_fsize = 1024;
				npp->p_frag = 8;
				npp->p_cpg = 16;
			}
		}
	}

	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	DPRINTF(("disklabel_sun_to_bsd: success!\n"));
	return (NULL);
}

/*
 * Find a valid disklabel.
 */
static char *
search_label(struct of_dev *devp, u_long off, char *buf,
	     struct disklabel *lp, u_long off0)
{
	size_t read;
	struct mbr_partition *p;
	int i;
	u_long poff;
	static int recursion;

	struct disklabel *dlp;
	struct sun_disklabel *slp;
	int error;

	/* minimal requirements for archtypal disk label */
	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	lp->d_npartitions = 1;
	if (lp->d_partitions[0].p_size == 0)
		lp->d_partitions[0].p_size = 0x1fffffff;
	lp->d_partitions[0].p_offset = 0;

	if (strategy(devp, F_READ, LABELSECTOR, DEV_BSIZE, buf, &read)
	    || read != DEV_BSIZE)
		return ("Cannot read label");
	/* Check for a NetBSD disk label. */
	dlp = (struct disklabel *) (buf + LABELOFFSET);
	if (dlp->d_magic == DISKMAGIC) {
		if (dkcksum(dlp))
			return ("NetBSD disk label corrupted");
		*lp = *dlp;
		DPRINTF(("search_label: found NetBSD label\n"));
		return (NULL);
	}

	/* Check for a Sun disk label (for PROM compatibility). */
	slp = (struct sun_disklabel *) buf;
	if (slp->sl_magic == SUN_DKMAGIC)
		return (disklabel_sun_to_bsd(buf, lp));


	memset(buf, 0, sizeof(buf));
	return ("no disk label");
}

int
devopen(struct open_file *of, const char *name, char **file)
{
	char *cp;
	char partition;
	char fname[256], devname[256];
	union {
		char buf[DEV_BSIZE];
		struct disklabel label;
	} b;
	struct disklabel label;
	int handle, part, try = 0;
	size_t read;
	char *errmsg = NULL, *pp, savedpart = 0;
	int error = 0;

	if (ofdev.handle != -1)
		panic("devopen");
	if (of->f_flags != F_READ)
		return EPERM;
	DPRINTF(("devopen: you want %s\n", name));
	strcpy(fname, name);
	cp = filename(fname, &partition);
	if (cp) {
		strcpy(b.buf, cp);
		*cp = 0;
	}
	if (!cp || !b.buf[0])
		strcpy(b.buf, DEFAULT_KERNEL);
	if (!*fname)
		strcpy(fname, bootdev);
	strcpy(opened_name, fname);
	if (partition) {
		cp = opened_name + strlen(opened_name);
		*cp++ = ':';
		*cp++ = partition;
		*cp = 0;
	}
	*file = opened_name + strlen(opened_name);
	if (b.buf[0] != '/')
		strcat(opened_name, "/");
	strcat(opened_name, b.buf);
	DPRINTF(("devopen: trying %s\n", fname));
	if ((handle = prom_finddevice(fname)) == -1)
		return ENOENT;
	DPRINTF(("devopen: found %s\n", fname));
	if (_prom_getprop(handle, "name", b.buf, sizeof b.buf) < 0)
		return ENXIO;
	DPRINTF(("devopen: %s is called %s\n", fname, b.buf));
	floppyboot = !strcmp(b.buf, "floppy");
	if (_prom_getprop(handle, "device_type", b.buf, sizeof b.buf) < 0)
		return ENXIO;
	DPRINTF(("devopen: %s is a %s device\n", fname, b.buf));
	if (!strcmp(b.buf, "block")) {
		pp = strrchr(fname, ':');
		if (pp && pp[1] >= 'a' && pp[1] <= 'f' && pp[2] == 0) {
			savedpart = pp[1];
		} else {
			savedpart = 'a';
			handle = prom_open(fname);
			if (handle != -1) {
				OF_instance_to_path(handle, devname,
				    sizeof(devname));
				DPRINTF(("real path: %s\n", devname));
				prom_close(handle);
				pp = devname + strlen(devname);
				if (pp > devname + 3) pp -= 2;
				if (pp[0] == ':')
					savedpart = pp[1];
			}
			pp = fname + strlen(fname);
			pp[0] = ':';
			pp[2] = '\0';
		}
		pp[1] = 'c';
		DPRINTF(("devopen: replacing by whole disk device %s\n",
		    fname));
		if (savedpart)
			partition = savedpart;
	}

open_again:
	DPRINTF(("devopen: opening %s\n", fname));
	if ((handle = prom_open(fname)) == -1) {
		DPRINTF(("devopen: open of %s failed\n", fname));
		if (pp && savedpart) {
			if (try == 0) {
				pp[0] = '\0';
				try = 1;
			} else {
				pp[0] = ':';
				pp[1] = savedpart;
				pp = NULL;
				savedpart = '\0';
			}
			goto open_again;
		}
		return ENXIO;
	}
	DPRINTF(("devopen: %s is now open\n", fname));
	memset(&ofdev, 0, sizeof ofdev);
	ofdev.handle = handle;
	if (!strcmp(b.buf, "block")) {
		ofdev.type = OFDEV_DISK;
		ofdev.bsize = DEV_BSIZE;
		/* First try to find a disklabel without MBR partitions */
		DPRINTF(("devopen: trying to read disklabel\n"));
		if (strategy(&ofdev, F_READ,
			     LABELSECTOR, DEV_BSIZE, b.buf, &read) != 0
		    || read != DEV_BSIZE
		    || (errmsg = getdisklabel(b.buf, &label))) {
			if (errmsg) printf("devopen: getdisklabel returned %s\n", errmsg);
			/* Else try MBR partitions */
			errmsg = search_label(&ofdev, 0, b.buf, &label, 0);
			if (errmsg) {
				printf("devopen: search_label returned %s\n", errmsg);
				error = ERDLAB;
			}
			if (error && error != ERDLAB)
				goto bad;
		}

		if (error == ERDLAB) {
			/* No, label, just use complete disk */
			ofdev.partoff = 0;
			if (pp && savedpart) {
				pp[1] = savedpart;
				prom_close(handle);
				if ((handle = prom_open(fname)) == -1) {
					DPRINTF(("devopen: open of %s failed\n",
						fname));
					return ENXIO;
				}
				ofdev.handle = handle;
				DPRINTF(("devopen: back to original device %s\n",
					fname));
			}
		} else {
			part = partition ? partition - 'a' : 0;
			ofdev.partoff = label.d_partitions[part].p_offset;
			DPRINTF(("devopen: setting partition %d offset %x\n",
			       part, ofdev.partoff));
			if (label.d_partitions[part].p_fstype == FS_RAID) {
				ofdev.partoff += RF_PROTECTED_SECTORS;
				DPRINTF(("devopen: found RAID partition, "
				    "adjusting offset to %x\n", ofdev.partoff));
			}
		}

		nfsys = 0;
		of->f_dev = ofdevsw;
		of->f_devdata = &ofdev;
#ifdef SPARC_BOOT_UFS
		memcpy(&file_system[nfsys++], &file_system_ufs, sizeof file_system[0]);
#endif
#ifdef SPARC_BOOT_CD9660
		memcpy(&file_system[nfsys++], &file_system_cd9660,
		    sizeof file_system[0]);
#endif
		DPRINTF(("devopen: return 0\n"));
		return 0;
	}
#ifdef NETBOOT
	if (!strcmp(b.buf, "network")) {
		if (error = net_open(&ofdev))
			goto bad;

		ofdev.type = OFDEV_NET;
		of->f_dev = ofdevsw;
		of->f_devdata = NULL;

		if (!strncmp(*file,"/tftp:",6)) {
			*file += 6;
			memcpy(&file_system[0], &file_system_tftp, sizeof file_system[0]);
			if (net_tftp_bootp(&of->f_devdata)) {
				net_close(&ofdev);
				goto bad;
			}
		} else {
			memcpy(&file_system[0], &file_system_nfs, sizeof file_system[0]);
			if (error = net_mountroot()) {
				net_close(&ofdev);
				goto bad;
			}
		}
		nfsys = 1;
		return 0;
	}
#endif
	error = EFTYPE;
bad:
	DPRINTF(("devopen: error %d, cannot open device\n", error));
	prom_close(handle);
	ofdev.handle = -1;
	return error;
}
