/*	$NetBSD: ofdev.c,v 1.26.32.1 2018/11/18 19:33:44 martin Exp $	*/

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

#include "ofdev.h"
#include "openfirm.h"

#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/bootblock.h>

#include <netinet/in.h>

#include <lib/libkern/libkern.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/byteorder.h>
#include <lib/libsa/cd9660.h>
#include <lib/libsa/dosfs.h>
#include <lib/libsa/nfs.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/lfs.h>
#include <lib/libsa/ustarfs.h>

#include "hfs.h"
#include "net.h"

#ifdef DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

static int
strategy(void *devdata, int rw, daddr_t blk, size_t size, void *buf,
	 size_t *rsize)
{
	struct of_dev *dev = devdata;
	u_quad_t pos;
	int n;

	if (rw != F_READ)
		return EPERM;
	if (dev->type != OFDEV_DISK)
		panic("strategy");

	pos = (u_quad_t)(blk + dev->partoff) * dev->bsize;

	for (;;) {
		if (OF_seek(dev->handle, pos) < 0)
			break;
		n = OF_read(dev->handle, buf, size);
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
devopen_dummy(struct open_file *of, ...) {
	return -1;
}

static int
devclose(struct open_file *of)
{
	struct of_dev *op = of->f_devdata;

	if (op->type == OFDEV_NET)
		net_close(op);
	OF_call_method("dma-free", op->handle, 2, 0, op->dmabuf, MAXPHYS);
	OF_close(op->handle);
	op->handle = -1;
	return 0;
}

static struct devsw of_devsw[1] = {
	{ "OpenFirmware", strategy, devopen_dummy, devclose, noioctl }
};
int ndevs = sizeof of_devsw / sizeof of_devsw[0];

static struct fs_ops file_system_ffsv1 = FS_OPS(ffsv1);
static struct fs_ops file_system_ffsv2 = FS_OPS(ffsv2);
static struct fs_ops file_system_lfsv1 = FS_OPS(lfsv1);
static struct fs_ops file_system_lfsv2 = FS_OPS(lfsv2);
static struct fs_ops file_system_hfs = FS_OPS(hfs);
static struct fs_ops file_system_ustarfs = FS_OPS(ustarfs);
static struct fs_ops file_system_cd9660 = FS_OPS(cd9660);
static struct fs_ops file_system_nfs = FS_OPS(nfs);
static struct fs_ops file_system_dosfs = FS_OPS(dosfs);

struct fs_ops file_system[9];
int nfsys;

static struct of_dev ofdev = {
	-1,
};

char opened_name[MAXBOOTPATHLEN];

/*
 * Check if this APM partition is a suitable root partition and return
 * its file system type or zero.
 */
static u_int8_t
check_apm_root(struct part_map_entry *part, int *clust)
{
	struct blockzeroblock *bzb;
	char typestr[32], *s;
	u_int8_t fstype;

	*clust = 0;  /* may become 1 for A/UX partitions */
	fstype = 0;
	bzb = (struct blockzeroblock *)(&part->pmBootArgs);

	/* convert partition type name to upper case */
	strncpy(typestr, (char *)part->pmPartType, sizeof(typestr));
	typestr[sizeof(typestr) - 1] = '\0';
	for (s = typestr; *s; s++)
		if ((*s >= 'a') && (*s <= 'z'))
			*s = (*s - 'a' + 'A');

	if (strcmp(PART_TYPE_NBSD_PPCBOOT, typestr) == 0) {
		if ((bzb->bzbMagic == BZB_MAGIC) &&
		    (bzb->bzbType < FSMAXTYPES))
			fstype = bzb->bzbType;
		else
			fstype = FS_BSDFFS;
	} else if (strcmp(PART_TYPE_UNIX, typestr) == 0 &&
	    bzb->bzbMagic == BZB_MAGIC && (bzb->bzbFlags & BZB_ROOTFS)) {
		*clust = bzb->bzbCluster;
		fstype = FS_BSDFFS;
	}

	return fstype;
}

/*
 * Build a disklabel from APM partitions.
 * We will just look for a suitable root partition and insert it into
 * the 'a' slot. Should be sufficient to boot a kernel from it.
 */
static int
search_mac_label(struct of_dev *devp, char *buf, struct disklabel *lp)
{
	struct part_map_entry *pme;
	struct partition *a_part;
	size_t nread;
	int blkno, clust, lastblk, lastclust;
	u_int8_t fstype;

	pme = (struct part_map_entry *)buf;
	a_part = &lp->d_partitions[0];		/* disklabel 'a' partition */
	lastclust = -1;

	for (blkno = lastblk = 1; blkno <= lastblk; blkno++) {
		if (strategy(devp, F_READ, blkno, DEV_BSIZE, pme, &nread)
		    || nread != DEV_BSIZE)
			return ERDLAB;
		if (pme->pmSig != PART_ENTRY_MAGIC ||
		    pme->pmPartType[0] == '\0')
			break;
		lastblk = pme->pmMapBlkCnt;

		fstype = check_apm_root(pme, &clust);

		if (fstype && (lastclust == -1 || clust < lastclust)) {
			a_part->p_size = pme->pmPartBlkCnt;
			a_part->p_offset = pme->pmPyPartStart;
			a_part->p_fstype = fstype;
			if ((lastclust = clust) == 0)
				break;	/* we won't find a better match */
		}
	}
	if (lastclust < 0)
		return ERDLAB;		/* no root partition found */

	/* pretend we only have partitions 'a', 'b' and 'c' */
	lp->d_npartitions = RAW_PART + 1;
	return 0;
}

static u_long
get_long(const void *p)
{
	const unsigned char *cp = p;

	return cp[0] | (cp[1] << 8) | (cp[2] << 16) | (cp[3] << 24);
}

/*
 * Find a valid disklabel from MBR partitions.
 */
static int
search_dos_label(struct of_dev *devp, u_long off, char *buf,
    struct disklabel *lp, u_long off0)
{
	size_t nread;
	struct mbr_partition *p;
	int i;
	u_long poff;
	static int recursion;

	if (strategy(devp, F_READ, off, DEV_BSIZE, buf, &nread)
	    || nread != DEV_BSIZE)
		return ERDLAB;

	if (*(u_int16_t *)&buf[MBR_MAGIC_OFFSET] != sa_htole16(MBR_MAGIC))
		return ERDLAB;

	if (recursion++ <= 1)
		off0 += off;
	for (p = (struct mbr_partition *)(buf + MBR_PART_OFFSET), i = 4;
	     --i >= 0; p++) {
		if (p->mbrp_type == MBR_PTYPE_NETBSD
#ifdef COMPAT_386BSD_MBRPART
		    || (p->mbrp_type == MBR_PTYPE_386BSD &&
			(printf("WARNING: old BSD partition ID!\n"), 1)
			/* XXX XXX - libsa printf() is void */ )
#endif
		    ) {
			poff = get_long(&p->mbrp_start) + off0;
			if (strategy(devp, F_READ, poff + 1,
				     DEV_BSIZE, buf, &nread) == 0
			    && nread == DEV_BSIZE) {
				if (!getdisklabel(buf, lp)) {
					recursion--;
					return 0;
				}
			}
			if (strategy(devp, F_READ, off, DEV_BSIZE, buf, &nread)
			    || nread != DEV_BSIZE) {
				recursion--;
				return ERDLAB;
			}
		} else if (p->mbrp_type == MBR_PTYPE_EXT) {
			poff = get_long(&p->mbrp_start);
			if (!search_dos_label(devp, poff, buf, lp, off0)) {
				recursion--;
				return 0;
			}
			if (strategy(devp, F_READ, off, DEV_BSIZE, buf, &nread)
			    || nread != DEV_BSIZE) {
				recursion--;
				return ERDLAB;
			}
		}
	}
	recursion--;
	return ERDLAB;
}

bool
parsefilepath(const char *path, char *devname, char *fname, char *ppart)
{
	char *cp, *lp;  
	char savec;     
	int dhandle;            
	char str[MAXBOOTPATHLEN];
	char devtype[16];

	DPRINTF("%s: path = %s\n", __func__, path);

	devtype[0] = '\0';

	if (devname != NULL)
		devname[0] = '\0';
	if (fname != NULL)
		fname[0] = '\0';
	if (ppart != NULL)
		*ppart = 0;

	strlcpy(str, path, sizeof(str));
	lp = str;               
	for (cp = str; *cp != '\0'; lp = cp) {      
		/* For each component of the path name... */
		while (*++cp != '\0' && *cp != '/')
			continue;
		savec = *cp;
		*cp = '\0';
		/* ...look whether there is a device with this name */
		dhandle = OF_finddevice(str);
		DPRINTF("%s: Checking %s: dhandle = %d\n",
		    __func__, str, dhandle);
		*cp = savec;
		if (dhandle != -1) {
			/*
			 * If it's a vaild device, lp is a delimiter
			 * in the OF device path.
			 */
			if (OF_getprop(dhandle, "device_type", devtype,
			    sizeof devtype) < 0)
				devtype[0] = '\0';
			continue;
		}
		
		/*
		 * If not, lp is the delimiter between OF device path
		 * and the specified filename.
		 */

		/* Check if the last component was a block device... */
		if (strcmp(devtype, "block") == 0) {
			/* search for arguments */
			for (cp = lp;
			    --cp >= str && *cp != '/' && *cp != ':';)
				continue;
			if (cp >= str && *cp == ':') {
				/* found arguments */
				for (cp = lp;
				    *--cp != ':' && *cp != ',';)
					continue;
				if (*++cp >= 'a' &&
				    *cp <= 'a' + MAXPARTITIONS &&
				    ppart != NULL)
					*ppart = *cp;
			}
		}
		if (*lp != '\0') {
			/* set filename */
			DPRINTF("%s: filename = %s\n", __func__, lp);
			if (fname != NULL)
				strcpy(fname, lp);
			if (str != lp) {
				/* set device path */
				*lp = '\0';
				DPRINTF("%s: device path = %s\n",
				    __func__, str);
				if (devname != NULL)
					strcpy(devname, str);
			}
		}
		return true;
	}

	DPRINTF("%s: invalid path?\n", __func__);
	return false;
}

int
devopen(struct open_file *of, const char *name, char **file)
{
	char *cp;
	char partition;
	char devname[MAXBOOTPATHLEN];
	char filename[MAXBOOTPATHLEN];
	char buf[DEV_BSIZE];
	struct disklabel label;
	int handle, part;
	size_t nread;
	int error = 0;

	if (ofdev.handle != -1)
		panic("devopen");
	if (of->f_flags != F_READ)
		return EPERM;

	if (!parsefilepath(name, devname, filename, &partition))
		return ENOENT;

	if (filename[0] == '\0')
		/* no filename */
		return ENOENT;

	if (devname[0] == '\0')
		/* no device, use default bootdev */
		strlcpy(devname, bootdev, sizeof(devname));

	DPRINTF("%s: devname =  %s, filename = %s\n",
	    __func__, devname, filename);

	strlcpy(opened_name, devname, sizeof(opened_name));
	if (partition) {
		cp = opened_name + strlen(opened_name);
		*cp++ = ':';
		*cp++ = partition;
		*cp = 0;
	}
	if (filename[0] != '/')
		strlcat(opened_name, "/", sizeof(opened_name));
	strlcat(opened_name, filename, sizeof(opened_name));

	DPRINTF("%s: opened_name =  %s\n", __func__, opened_name);

	*file = opened_name + strlen(devname) + 1;
	if ((handle = OF_finddevice(devname)) == -1)
		return ENOENT;
	if (OF_getprop(handle, "device_type", buf, sizeof buf) < 0)
		return ENXIO;
	if (!strcmp(buf, "block") && strrchr(devname, ':') == NULL)
		/* indicate raw partition, when missing */
		if (ofw_version >= 3)
			strlcat(devname, ":0", sizeof(devname));
	if ((handle = OF_open(devname)) == -1)
		return ENXIO;
	memset(&ofdev, 0, sizeof ofdev);
	ofdev.handle = handle;
	ofdev.dmabuf = NULL;
	OF_call_method("dma-alloc", handle, 1, 1, MAXPHYS, &ofdev.dmabuf);
	if (!strcmp(buf, "block")) {
		ofdev.type = OFDEV_DISK;
		ofdev.bsize = DEV_BSIZE;
		/* First try to find a disklabel without partitions */
		if (!floppyboot &&
		    (strategy(&ofdev, F_READ,
			      LABELSECTOR, DEV_BSIZE, buf, &nread) != 0
		     || nread != DEV_BSIZE
		     || getdisklabel(buf, &label))) {
			/* Else try APM or MBR partitions */
			struct drvr_map *map = (struct drvr_map *)buf;

			if (map->sbSig == DRIVER_MAP_MAGIC)
				error = search_mac_label(&ofdev, buf, &label);
			else
				error = search_dos_label(&ofdev, 0, buf,
				    &label, 0);
			if (error && error != ERDLAB)
				goto bad;
		}

		if (error == ERDLAB) {
			if (partition)
				/*
				 * User specified a parititon,
				 * but there is none
				 */
				goto bad;
			/* No label, just use complete disk */
			ofdev.partoff = 0;
		} else {
			part = partition ? partition - 'a' : 0;
			ofdev.partoff = label.d_partitions[part].p_offset;
		}

		of->f_dev = of_devsw;
		of->f_devdata = &ofdev;
		file_system[0] = file_system_ffsv1;
		file_system[1] = file_system_ffsv2;
		file_system[2] = file_system_lfsv1;
		file_system[3] = file_system_lfsv2;
		file_system[4] = file_system_ustarfs;
		file_system[5] = file_system_cd9660;
		file_system[6] = file_system_hfs;
		file_system[7] = file_system_dosfs;
		nfsys = 8;
		return 0;
	}
	if (!strcmp(buf, "network")) {
		ofdev.type = OFDEV_NET;
		of->f_dev = of_devsw;
		of->f_devdata = &ofdev;
		file_system[0] = file_system_nfs;
		nfsys = 1;
		if ((error = net_open(&ofdev)))
			goto bad;
		return 0;
	}
	error = EFTYPE;
bad:
	OF_close(handle);
	ofdev.handle = -1;
	return error;
}
