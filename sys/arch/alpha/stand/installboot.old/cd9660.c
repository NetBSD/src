/*	$NetBSD: cd9660.c,v 1.2 1999/04/05 03:02:03 cgd Exp $	*/

/*
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
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
 * File lookup for ISO-9660 file systems.  Taken from:
 *
 *	src/sys/lib/libsa/cd9660.c
 *
 *	NetBSD: cd9660.c,v 1.5 1997/06/26 19:11:33 drochner Exp
 */

/*
 * Stand-alone ISO9660 file reading package.
 *
 * Note: This doesn't support Rock Ridge extensions, extended attributes,
 * blocksizes other than 2048 bytes, multi-extent files, etc.
 */

#include <sys/param.h>
#include <isofs/cd9660/iso.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

struct ptable_ent {
	char namlen	[ISODCL( 1, 1)];	/* 711 */
	char extlen	[ISODCL( 2, 2)];	/* 711 */
	char block	[ISODCL( 3, 6)];	/* 732 */
	char parent	[ISODCL( 7, 8)];	/* 722 */
	char name	[1];
};
#define	PTFIXSZ		8
#define	PTSIZE(pp)	roundup(PTFIXSZ + isonum_711((pp)->namlen), 2)

#define	cdb2devb(bno)	((bno) * ISO_DEFAULT_BLOCK_SIZE / DEV_BSIZE)

static int pnmatch __P((char *, struct ptable_ent *));
static int dirmatch __P((char *, struct iso_directory_record *));
static int strategy __P((int, off_t, size_t, void *, size_t *));

static int
pnmatch(path, pp)
	char *path;
	struct ptable_ent *pp;
{
	char *cp;
	int i;
	
	cp = pp->name;
	for (i = isonum_711(pp->namlen); --i >= 0; path++, cp++) {
		if (toupper(*path) == *cp)
			continue;
		return 0;
	}
	if (*path != '/')
		return 0;
	return 1;
}

static int
dirmatch(path, dp)
	char *path;
	struct iso_directory_record *dp;
{
	char *cp;
	int i;

	/* This needs to be a regular file */
	if (dp->flags[0] & 6)
		return 0;

	cp = dp->name;
	for (i = isonum_711(dp->name_len); --i >= 0; path++, cp++) {
		if (!*path)
			break;
		if (toupper(*path) == *cp)
			continue;
		return 0;
	}
	if (*path)
		return 0;
	/*
	 * Allow stripping of trailing dots and the version number.
	 * Note that this will find the first instead of the last version
	 * of a file.
	 */
	if (i >= 0 && (*cp == ';' || *cp == '.')) {
		/* This is to prevent matching of numeric extensions */
		if (*cp == '.' && cp[1] != ';')
			return 0;
		while (--i >= 0)
			if (*++cp != ';' && (*cp < '0' || *cp > '9'))
				return 0;
	}
	return 1;
}

static int
strategy(fd, blkno, size, buf, cnt)
	int fd;
	off_t blkno;
	size_t size;
	void *buf;
	size_t *cnt;
{
	off_t offset = blkno * DEV_BSIZE;

	if (lseek(fd, offset, SEEK_SET) != offset)
		return (1);
	*cnt = read(fd, buf, size);
	if (*cnt != size)
		return (1);
	return (0);
}

int
cd9660_lookup(path, fd, blknop, sizep)
	char *path;
	int fd;
	u_long *blknop, *sizep;
{
	void *buf;
	struct iso_primary_descriptor *vd;
	size_t buf_size, read, psize, dsize;
	daddr_t bno;
	int parent, ent;
	struct ptable_ent *pp;
	struct iso_directory_record *dp = 0;
	int rc;
	
	/* First find the volume descriptor */
	buf = malloc(buf_size = ISO_DEFAULT_BLOCK_SIZE);
	vd = buf;
	for (bno = 16;; bno++) {
		rc = strategy(fd, cdb2devb(bno), ISO_DEFAULT_BLOCK_SIZE,
		    buf, &read);
		if (rc)
			goto out;
		if (read != ISO_DEFAULT_BLOCK_SIZE) {
			rc = EIO;
			goto out;
		}
		rc = EINVAL;
		if (bcmp(vd->id, ISO_STANDARD_ID, sizeof vd->id) != 0)
			goto out;
		if (isonum_711(vd->type) == ISO_VD_END)
			goto out;
		if (isonum_711(vd->type) == ISO_VD_PRIMARY)
			break;
	}
	if (isonum_723(vd->logical_block_size) != ISO_DEFAULT_BLOCK_SIZE)
		goto out;
	
	/* Now get the path table and lookup the directory of the file */
	bno = isonum_732(vd->type_m_path_table);
	psize = isonum_733(vd->path_table_size);
	
	if (psize > ISO_DEFAULT_BLOCK_SIZE) {
		free(buf);
		buf = malloc(buf_size = roundup(psize, ISO_DEFAULT_BLOCK_SIZE));
	}

	rc = strategy(fd, cdb2devb(bno), buf_size, buf, &read);
	if (rc)
		goto out;
	if (read != buf_size) {
		rc = EIO;
		goto out;
	}
	
	parent = 1;
	pp = (struct ptable_ent *)buf;
	ent = 1;
	bno = isonum_732(pp->block) + isonum_711(pp->extlen);
	
	rc = ENOENT;
	while (*path) {
		if ((u_long)pp >= (u_long)buf + psize)
			break;
		if (isonum_722(pp->parent) != parent)
			break;
		if (!pnmatch(path, pp)) {
			pp = (struct ptable_ent *)((u_long)pp + PTSIZE(pp));
			ent++;
			continue;
		}
		path += isonum_711(pp->namlen) + 1;
		parent = ent;
		bno = isonum_732(pp->block) + isonum_711(pp->extlen);
		while ((u_long)pp < (u_long)buf + psize) {
			if (isonum_722(pp->parent) == parent)
				break;
			pp = (struct ptable_ent *)((u_long)pp + PTSIZE(pp));
			ent++;
		}
	}

	/* Now bno has the start of the directory that supposedly contains the file */
	bno--;
	dsize = 1;		/* Something stupid, but > 0			XXX */
	for (psize = 0; psize < dsize;) {
		if (!(psize % ISO_DEFAULT_BLOCK_SIZE)) {
			bno++;
			rc = strategy(fd, cdb2devb(bno),
			    ISO_DEFAULT_BLOCK_SIZE, buf, &read);
			if (rc)
				goto out;
			if (read != ISO_DEFAULT_BLOCK_SIZE) {
				rc = EIO;
				goto out;
			}
			dp = (struct iso_directory_record *)buf;
		}
		if (!isonum_711(dp->length)) {
			if ((void *)dp == buf)
				psize += ISO_DEFAULT_BLOCK_SIZE;
			else
				psize = roundup(psize, ISO_DEFAULT_BLOCK_SIZE);
			continue;
		}
		if (dsize == 1)
			dsize = isonum_733(dp->size);
		if (dirmatch(path, dp))
			break;
		psize += isonum_711(dp->length);
		dp = (struct iso_directory_record *)((u_long)dp + isonum_711(dp->length));
	}

	if (psize >= dsize) {
		rc = ENOENT;
		goto out;
	}
	
	*blknop = isonum_733(dp->extent);
	*sizep = isonum_733(dp->size);
	free(buf);
	
	return 0;
	
out:
	free(buf);
	
	return rc;
}
