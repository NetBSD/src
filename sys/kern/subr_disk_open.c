/*	$NetBSD: subr_disk_open.c,v 1.7 2012/04/07 05:38:07 christos Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: subr_disk_open.c,v 1.7 2012/04/07 05:38:07 christos Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/vnode.h>
#include <miscfs/specfs/specdev.h>

struct vnode *
opendisk(struct device *dv)
{
	int bmajor, bminor;
	struct vnode *tmpvn;
	int error;
	dev_t dev;
	
	/*
	 * Lookup major number for disk block device.
	 */
	bmajor = devsw_name2blk(device_xname(dv), NULL, 0);
	if (bmajor == -1)
		return NULL;
	
	bminor = minor(device_unit(dv));
	/*
	 * Fake a temporary vnode for the disk, open it, and read
	 * and hash the sectors.
	 */
	dev = device_is_a(dv, "dk") ? makedev(bmajor, bminor) :
	    MAKEDISKDEV(bmajor, bminor, RAW_PART);
	if (bdevvp(dev, &tmpvn))
		panic("%s: can't alloc vnode for %s", __func__,
		    device_xname(dv));
	error = VOP_OPEN(tmpvn, FREAD | FSILENT, NOCRED);
	if (error) {
		/*
		 * Ignore errors caused by missing device, partition,
		 * medium, or busy [presumably because of a wedge covering it]
		 */
		switch (error) {
		case ENXIO:
		case ENODEV:
		case EBUSY:
			break;
		default:
			printf("%s: can't open dev %s (%d)\n",
			    __func__, device_xname(dv), error);
			break;
		}
		vput(tmpvn);
		return NULL;
	}

	return tmpvn;
}

int
getdisksize(struct vnode *vp, uint64_t *numsecp, unsigned *secsizep)
{
	struct partinfo dpart;
	struct dkwedge_info dkw;
	struct disk *pdk;
	int error;

	error = VOP_IOCTL(vp, DIOCGPART, &dpart, FREAD, NOCRED);
	if (error == 0) {
		*secsizep = dpart.disklab->d_secsize;
		*numsecp  = dpart.part->p_size;
		return 0;
	}

	error = VOP_IOCTL(vp, DIOCGWEDGEINFO, &dkw, FREAD, NOCRED);
	if (error == 0) {
		pdk = disk_find(dkw.dkw_parent);
		if (pdk != NULL) {
			*secsizep = DEV_BSIZE << pdk->dk_blkshift;
			*numsecp  = dkw.dkw_size;
		} else
			error = ENODEV;
	}

	return error;
}

int
getdiskinfo(struct vnode *vp, struct dkwedge_info *dkw)
{
	struct partinfo dpart;
	int error;
	dev_t dev = vp->v_specnode->sn_rdev;

	if (VOP_IOCTL(vp, DIOCGWEDGEINFO, dkw, FREAD, NOCRED) == 0)
		return 0;
	
	if ((error = VOP_IOCTL(vp, DIOCGPART, &dpart, FREAD, NOCRED)) != 0)
		return error;

	snprintf(dkw->dkw_devname, sizeof(dkw->dkw_devname), "%s%" PRId32 "%c",
	    devsw_blk2name(major(dev)), DISKUNIT(dev), (char)DISKPART(dev) +
	    'a');

	dkw->dkw_wname[0] = '\0';

	strlcpy(dkw->dkw_parent, dkw->dkw_devname, sizeof(dkw->dkw_parent));

	dkw->dkw_size = dpart.part->p_size;
	dkw->dkw_offset = dpart.part->p_offset;

	strlcpy(dkw->dkw_ptype, getfstypename(dpart.part->p_fstype),
	    sizeof(dkw->dkw_ptype));

	return 0;
}
