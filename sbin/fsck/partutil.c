/*	$NetBSD: partutil.c,v 1.3.6.1 2009/05/13 19:19:01 jym Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: partutil.c,v 1.3.6.1 2009/05/13 19:19:01 jym Exp $");

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <disktab.h>
#include <fcntl.h>
#include <util.h>
#include <unistd.h>
#include <err.h>
#include <string.h>
#include <errno.h>

#include "partutil.h"

static void
label2geom(struct disk_geom *geo, const struct disklabel *lp)
{
	geo->dg_secperunit = lp->d_secperunit;
	geo->dg_secsize = lp->d_secsize;
	geo->dg_nsectors = lp->d_nsectors;
	geo->dg_ntracks = lp->d_ntracks;
	geo->dg_ncylinders = lp->d_ncylinders;
	geo->dg_secpercyl = lp->d_secpercyl;
	geo->dg_pcylinders = lp->d_ncylinders;
	geo->dg_sparespertrack = lp->d_sparespertrack;
	geo->dg_sparespercyl = lp->d_sparespercyl;
	geo->dg_acylinders = lp->d_acylinders;
}

static void
part2wedge(struct dkwedge_info *dkw, const struct disklabel *lp, const char *s)
{
	struct stat sb;
	const struct partition *pp;
	int ptn;

	(void)memset(dkw, 0, sizeof(*dkw));
	if (stat(s, &sb) == -1)
		return;

	ptn = strchr(s, '\0')[-1] - 'a';
	if ((unsigned)ptn >= lp->d_npartitions ||
	    (devminor_t)ptn != DISKPART(sb.st_rdev))
		return;

	pp = &lp->d_partitions[ptn];
	dkw->dkw_offset = pp->p_offset;
	dkw->dkw_size = pp->p_size;
	dkw->dkw_parent[0] = '*';
	switch (pp->p_fstype) {
	default:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_UNKNOWN);
		break;
	case FS_UNUSED:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_UNUSED);
		break;
	case FS_SWAP:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_SWAP);
		break;
	case FS_BSDFFS:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_FFS);
		break;
	case FS_BSDLFS:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_LFS);
		break;
	case FS_EX2FS:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_EXT2FS);
		break;
	case FS_ISO9660:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_ISO9660);
		break;
	case FS_ADOS:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_AMIGADOS);
		break;
	case FS_HFS:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_APPLEHFS);
		break;
	case FS_MSDOS:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_FAT);
		break;
	case FS_FILECORE:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_FILECORE);
		break;
	case FS_APPLEUFS:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_APPLEUFS);
		break;
	case FS_NTFS:
		(void)strcpy(dkw->dkw_ptype, DKW_PTYPE_NTFS);
		break;
	}
}

int
getdiskinfo(const char *s, int fd, const char *dt, struct disk_geom *geo,
    struct dkwedge_info *dkw)
{
	struct disklabel lab;
	struct disklabel *lp = &lab;
	char parent[1024];

	if (dt) {
		lp = getdiskbyname(dt);
		if (lp == NULL)
			errx(1, "%s: unknown disk type", dt);
		goto part;
	}

	if (ioctl(fd, DIOCGDINFO, lp) == -1) {
		if (errno == ENOTTY) {
			int pfd;
			if (ioctl(fd, DIOCGWEDGEINFO, dkw) == -1) {
				warn("ioctl (DIOCGWEDGEINFO)");
				goto bad;
			}
			pfd = opendisk(dkw->dkw_parent, O_RDONLY,
			    parent, sizeof(parent), 0);
			if (pfd == -1) {
				warn("Cannot open `%s'", dkw->dkw_parent);
				goto bad;
			}
			if (ioctl(pfd, DIOCGDINFO, lp) != -1) {
				(void)close(pfd);
				goto label;
			} else {
				int serrno = errno;
				(void)close(pfd);
				errno = serrno;
			}
		}
		warn("ioctl (DIOCGDINFO)");
		goto bad;
	}
part:
	part2wedge(dkw, lp, s);
label:
	label2geom(geo, lp);
	return 0;
bad:
	return -1;
}
