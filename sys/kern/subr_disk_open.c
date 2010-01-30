/*	$NetBSD: subr_disk_open.c,v 1.2 2010/01/30 11:57:17 mlelstv Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: subr_disk_open.c,v 1.2 2010/01/30 11:57:17 mlelstv Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/vnode.h>

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
	error = VOP_OPEN(tmpvn, FREAD, NOCRED);
	if (error) {
#ifndef DEBUG
		/*
		 * Ignore errors caused by missing device, partition,
		 * or medium.
		 */
		if (error != ENXIO && error != ENODEV)
#endif
			printf("%s: can't open dev %s (%d)\n",
			    __func__, device_xname(dv), error);
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
