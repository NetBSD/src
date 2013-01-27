/*	$NetBSD: msdosfs_vnops.c,v 1.6 2013/01/27 12:25:13 martin Exp $	*/

/*-
 * Copyright (C) 1994, 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1994, 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
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
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: msdosfs_vnops.c,v 1.6 2013/01/27 12:25:13 martin Exp $");

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>

#include <ffs/buf.h>

#include <fs/msdosfs/bpb.h>
#include <fs/msdosfs/direntry.h>
#include <fs/msdosfs/denode.h>
#include <fs/msdosfs/msdosfsmount.h>
#include <fs/msdosfs/fat.h>

#include "makefs.h"
#include "msdos.h"

/*
 * Some general notes:
 *
 * In the ufs filesystem the inodes, superblocks, and indirect blocks are
 * read/written using the vnode for the filesystem. Blocks that represent
 * the contents of a file are read/written using the vnode for the file
 * (including directories when they are read/written as files). This
 * presents problems for the dos filesystem because data that should be in
 * an inode (if dos had them) resides in the directory itself.  Since we
 * must update directory entries without the benefit of having the vnode
 * for the directory we must use the vnode for the filesystem.  This means
 * that when a directory is actually read/written (via read, write, or
 * readdir, or seek) we must use the vnode for the filesystem instead of
 * the vnode for the directory as would happen in ufs. This is to insure we
 * retrieve the correct block from the buffer cache since the hash value is
 * based upon the vnode address and the desired block number.
 */

static int msdosfs_wfile(const char *, struct denode *, fsnode *);

static void
msdosfs_times(struct msdosfsmount *pmp, struct denode *dep,
    const struct stat *st)
{
#ifndef HAVE_NBTOOL_CONFIG_H
	struct timespec at = st->st_atimespec;
	struct timespec mt = st->st_mtimespec;
#else
	struct timespec at = { st->st_atime, 0 };
	struct timespec mt = { st->st_mtime, 0 };
#endif
	unix2dostime(&at, pmp->pm_gmtoff, &dep->de_ADate, NULL, NULL);
	unix2dostime(&mt, pmp->pm_gmtoff, &dep->de_MDate, &dep->de_MTime, NULL);
}

/*
 * Create a regular file. On entry the directory to contain the file being
 * created is locked.  We must release before we return.
 */
struct denode *
msdosfs_mkfile(const char *path, struct denode *pdep, fsnode *node)
{
	struct componentname cn;
	struct denode ndirent;
	struct denode *dep;
	int error;
	struct stat *st = &node->inode->st;
	struct msdosfsmount *pmp = pdep->de_pmp;

	cn.cn_nameptr = node->name;
	cn.cn_namelen = strlen(node->name);

#ifdef MSDOSFS_DEBUG
	printf("msdosfs_create(name %s, mode 0%o size %zu\n", node->name,
	    st->st_mode, (size_t)st->st_size);
#endif

	/*
	 * If this is the root directory and there is no space left we
	 * can't do anything.  This is because the root directory can not
	 * change size.
	 */
	if (pdep->de_StartCluster == MSDOSFSROOT
	    && pdep->de_fndoffset >= pdep->de_FileSize) {
		error = ENOSPC;
		goto bad;
	}

	/*
	 * Create a directory entry for the file, then call createde() to
	 * have it installed. NOTE: DOS files are always executable.  We
	 * use the absence of the owner write bit to make the file
	 * readonly.
	 */
	memset(&ndirent, 0, sizeof(ndirent));
	if ((error = uniqdosname(pdep, &cn, ndirent.de_Name)) != 0)
		goto bad;

	ndirent.de_Attributes = (st->st_mode & S_IWUSR) ?
				ATTR_ARCHIVE : ATTR_ARCHIVE | ATTR_READONLY;
	ndirent.de_StartCluster = 0;
	ndirent.de_FileSize = 0;
	ndirent.de_dev = pdep->de_dev;
	ndirent.de_devvp = pdep->de_devvp;
	ndirent.de_pmp = pdep->de_pmp;
	ndirent.de_flag = DE_ACCESS | DE_CREATE | DE_UPDATE;
	msdosfs_times(pmp, &ndirent, st);
	if ((error = createde(&ndirent, pdep, &dep, &cn)) != 0)
		goto bad;
	if ((error = msdosfs_wfile(path, dep, node)) == -1)
		goto bad;
	return dep;

bad:
	errno = error;
	return NULL;
}

/*
 * Write data to a file or directory.
 */
static int
msdosfs_wfile(const char *path, struct denode *dep, fsnode *node)
{
	int error, fd;
	size_t osize = dep->de_FileSize;
	struct stat *st = &node->inode->st;
	size_t nsize;
	size_t offs = 0;
	struct msdosfsmount *pmp = dep->de_pmp;
	struct buf *bp;
	char *dat;
	u_long cn;

#ifdef MSDOSFS_DEBUG
	printf("msdosfs_write(): diroff %lu, dirclust %lu, startcluster %lu\n",
	    dep->de_diroffset, dep->de_dirclust, dep->de_StartCluster);
#endif
	if (st->st_size == 0)
		return 0;

	/* Don't bother to try to write files larger than the fs limit */
	if (st->st_size > (off_t)min(MSDOSFS_FILESIZE_MAX,__SIZE_MAX__)) {
		errno = EFBIG;
		return -1;
	}

	nsize = st->st_size;
	if (nsize > osize) {
		if ((error = deextend(dep, nsize, NULL)) != 0) {
			errno = error;
			return -1;
		}
	}

	if ((fd = open(path, O_RDONLY)) == -1)
		err(1, "open %s", path);

	if ((dat = mmap(0, (size_t)nsize, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0))
	    == MAP_FAILED)
		err(1, "mmap %s", node->name);
	close(fd);

        for (cn = 0;; cn++) {
		int blsize, cpsize;
		daddr_t bn;
                if (pcbmap(dep, cn, &bn, 0, &blsize)) {
			fprintf(stderr, "pcbmap %ld\n", cn);
			goto out;
		}

                if (bread(pmp->pm_devvp, de_bn2kb(pmp, bn), blsize, NULL,
                    0, &bp)) {
			fprintf(stderr, "bread\n");
                } 
		cpsize = MIN((int)(nsize - offs), blsize);
		memcpy(bp->b_data, dat + offs, cpsize);
		bwrite(bp);
		brelse(bp, 0);
		offs += blsize;
		if (offs >= nsize)
			break;
	}

	munmap(dat, nsize);
	return 0;
out:
	munmap(dat, nsize);
	return -1;
}


static const struct {
	struct direntry dot;
	struct direntry dotdot;
} dosdirtemplate = {
	{	".       ", "   ",			/* the . entry */
		ATTR_DIRECTORY,				/* file attribute */
		0,	 				/* reserved */
		0, { 0, 0 }, { 0, 0 },			/* create time & date */
		{ 0, 0 },				/* access date */
		{ 0, 0 },				/* high bits of start cluster */
		{ 210, 4 }, { 210, 4 },			/* modify time & date */
		{ 0, 0 },				/* startcluster */
		{ 0, 0, 0, 0 } 				/* filesize */
	},
	{	"..      ", "   ",			/* the .. entry */
		ATTR_DIRECTORY,				/* file attribute */
		0,	 				/* reserved */
		0, { 0, 0 }, { 0, 0 },			/* create time & date */
		{ 0, 0 },				/* access date */
		{ 0, 0 },				/* high bits of start cluster */
		{ 210, 4 }, { 210, 4 },			/* modify time & date */
		{ 0, 0 },				/* startcluster */
		{ 0, 0, 0, 0 }				/* filesize */
	}
};

struct denode *
msdosfs_mkdire(const char *path, struct denode *pdep, fsnode *node) {
	struct denode ndirent;
	struct denode *dep;
	struct componentname cn;
	struct stat *st = &node->inode->st;
	struct msdosfsmount *pmp = pdep->de_pmp;
	int error;
	u_long newcluster, pcl, bn;
	daddr_t lbn;
	struct direntry *denp;
	struct buf *bp;

	cn.cn_nameptr = node->name;
	cn.cn_namelen = strlen(node->name);
	/*
	 * If this is the root directory and there is no space left we
	 * can't do anything.  This is because the root directory can not
	 * change size.
	 */
	if (pdep->de_StartCluster == MSDOSFSROOT
	    && pdep->de_fndoffset >= pdep->de_FileSize) {
		error = ENOSPC;
		goto bad2;
	}

	/*
	 * Allocate a cluster to hold the about to be created directory.
	 */
	error = clusteralloc(pmp, 0, 1, &newcluster, NULL);
	if (error)
		goto bad2;

	memset(&ndirent, 0, sizeof(ndirent));
	ndirent.de_pmp = pmp;
	ndirent.de_flag = DE_ACCESS | DE_CREATE | DE_UPDATE;
	msdosfs_times(pmp, &ndirent, st);

	/*
	 * Now fill the cluster with the "." and ".." entries. And write
	 * the cluster to disk.  This way it is there for the parent
	 * directory to be pointing at if there were a crash.
	 */
	bn = cntobn(pmp, newcluster);
	lbn = de_bn2kb(pmp, bn);
	/* always succeeds */
	bp = getblk(pmp->pm_devvp, lbn, pmp->pm_bpcluster, 0, 0);
	memset(bp->b_data, 0, pmp->pm_bpcluster);
	memcpy(bp->b_data, &dosdirtemplate, sizeof dosdirtemplate);
	denp = (struct direntry *)bp->b_data;
	putushort(denp[0].deStartCluster, newcluster);
	putushort(denp[0].deCDate, ndirent.de_CDate);
	putushort(denp[0].deCTime, ndirent.de_CTime);
	denp[0].deCHundredth = ndirent.de_CHun;
	putushort(denp[0].deADate, ndirent.de_ADate);
	putushort(denp[0].deMDate, ndirent.de_MDate);
	putushort(denp[0].deMTime, ndirent.de_MTime);
	pcl = pdep->de_StartCluster;
	if (FAT32(pmp) && pcl == pmp->pm_rootdirblk)
		pcl = 0;
	putushort(denp[1].deStartCluster, pcl);
	putushort(denp[1].deCDate, ndirent.de_CDate);
	putushort(denp[1].deCTime, ndirent.de_CTime);
	denp[1].deCHundredth = ndirent.de_CHun;
	putushort(denp[1].deADate, ndirent.de_ADate);
	putushort(denp[1].deMDate, ndirent.de_MDate);
	putushort(denp[1].deMTime, ndirent.de_MTime);
	if (FAT32(pmp)) {
		putushort(denp[0].deHighClust, newcluster >> 16);
		putushort(denp[1].deHighClust, pdep->de_StartCluster >> 16);
	} else {
		putushort(denp[0].deHighClust, 0);
		putushort(denp[1].deHighClust, 0);
	}

	if ((error = bwrite(bp)) != 0)
		goto bad;

	/*
	 * Now build up a directory entry pointing to the newly allocated
	 * cluster.  This will be written to an empty slot in the parent
	 * directory.
	 */
	if ((error = uniqdosname(pdep, &cn, ndirent.de_Name)) != 0)
		goto bad;

	ndirent.de_Attributes = ATTR_DIRECTORY;
	ndirent.de_StartCluster = newcluster;
	ndirent.de_FileSize = 0;
	ndirent.de_dev = pdep->de_dev;
	ndirent.de_devvp = pdep->de_devvp;
	if ((error = createde(&ndirent, pdep, &dep, &cn)) != 0)
		goto bad;
	return dep;

bad:
	clusterfree(pmp, newcluster, NULL);
bad2:
	errno = error;
	return NULL;
}
