/*	$NetBSD: biosdisk_ll.c,v 1.9.6.1 1999/12/27 18:32:29 wrstuden Exp $	 */

/*
 * Copyright (c) 1996
 * 	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996
 * 	Perry E. Metzger.  All rights reserved.
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
 *    must display the following acknowledgements:
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 *	This product includes software developed for the NetBSD Project
 *	by Perry E. Metzger.
 * 4. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * shared by bootsector startup (bootsectmain) and biosdisk.c
 * needs lowlevel parts from bios_disk.S
 */

#include <lib/libsa/stand.h>

#include "biosdisk_ll.h"
#include "diskbuf.h"

extern long ourseg;

extern int get_diskinfo __P((int));
extern void int13_getextinfo __P((int, struct biosdisk_ext13info *));
extern int int13_extension __P((int));
extern int biosread __P((int, int, int, int, int, char *));
extern int biosextread __P((int, void *));
static int do_read __P((struct biosdisk_ll *, int, int, char *));

/*
 * we get from get_diskinfo():
 * xxxx  %ch  %cl  %dh (registers after int13/8), ie
 * xxxx cccc Csss hhhh
 */
#define	SPT(di)		(((di)>>8)&0x3f)
#define	HEADS(di)	(((di)&0xff)+1)
#define CYL(di)		(((((di)>>16)&0xff)|(((di)>>6)&0x300))+1)

#ifndef BIOSDISK_RETRIES
#define BIOSDISK_RETRIES 5
#endif

int 
set_geometry(d, ed)
	struct biosdisk_ll *d;
	struct biosdisk_ext13info *ed;
{
	int diskinfo;

	diskinfo = get_diskinfo(d->dev);
	d->sec = SPT(diskinfo);
	d->head = HEADS(diskinfo);
	d->cyl = CYL(diskinfo);
	d->chs_sectors = d->sec * d->head * d->cyl;

	d->flags = 0;
	if ((d->dev & 0x80) && int13_extension(d->dev)) {
		d->flags |= BIOSDISK_EXT13;
		if (ed != NULL)
			int13_getextinfo(d->dev, ed);
	}

	/*
	 * get_diskinfo assumes floppy if BIOS call fails. Check at least
	 * "valid" geometry.
	 */
	return (!d->sec || !d->head);
}

/*
 * Global shared "diskbuf" is used as read ahead buffer.  For reading from
 * floppies, the bootstrap has to be loaded on a 64K boundary to ensure that
 * this buffer doesn't cross a 64K DMA boundary.
 */
#define RA_SECTORS      (DISKBUFSIZE / BIOSDISK_SECSIZE)
static int      ra_dev;
static int      ra_end;
static int      ra_first;

static int
do_read(d, dblk, num, buf)
	struct		biosdisk_ll *d;
	int		dblk, num;
	char	       *buf;
{
	int		cyl, head, sec, nsec, spc;
	struct {
		int8_t	size;
		int8_t	resvd;
		int16_t	cnt;
		int16_t	off;
		int16_t	seg;
		int64_t	sec;
	}		ext;

	if ((d->dev & 0x80) && (dblk + num) >= d->chs_sectors) {
		if (!(d->flags & BIOSDISK_EXT13))
			return -1;
		ext.size = sizeof(ext);
		ext.resvd = 0;
		ext.cnt = num;
		ext.off = (int32_t)buf;
		ext.seg = ourseg;
		ext.sec = dblk;

		if (biosextread(d->dev, &ext))
			return -1;

		return ext.cnt;
	} else {
		spc = d->head * d->sec;
		cyl = dblk / spc;
		head = (dblk % spc) / d->sec;
		sec = dblk % d->sec;
		nsec = d->sec - sec;

		if (nsec > num)
			nsec = num;

		if (biosread(d->dev, cyl, head, sec, nsec, buf))
			return -1;

		return nsec;
	}
}

int 
readsects(d, dblk, num, buf, cold)	/* reads ahead if (!cold) */
	struct biosdisk_ll *d;
	int             dblk, num;
	char           *buf;
	int             cold;	/* don't use data segment or bss, don't call
				 * library functions */
{
	while (num) {
		int             nsec;

		/* check for usable data in read-ahead buffer */
		if (cold || diskbuf_user != &ra_dev || d->dev != ra_dev
		    || dblk < ra_first || dblk >= ra_end) {

			/* no, read from disk */
			char           *trbuf;
			int maxsecs;
			int retries = BIOSDISK_RETRIES;

			if (cold) {
				/* transfer directly to buffer */
				trbuf = buf;
				maxsecs = num;
			} else {
				/* fill read-ahead buffer */
				trbuf = diskbuf;
				maxsecs = RA_SECTORS;
				diskbuf_user = 0; /* not yet valid */
			}

			while ((nsec = do_read(d, dblk, maxsecs, trbuf)) < 0) {
#ifdef DISK_DEBUG
				if (!cold)
					printf("read error dblk %d-%d\n", dblk,
					       dblk + maxsecs - 1);
#endif
				if (--retries >= 0)
					continue;
				return (-1);	/* XXX cannot output here if
						 * (cold) */
			}
			if (!cold) {
				ra_dev = d->dev;
				ra_first = dblk;
				ra_end = dblk + nsec;
				diskbuf_user = &ra_dev;
			}
		} else		/* can take blocks from end of read-ahead
				 * buffer */
			nsec = ra_end - dblk;

		if (!cold) {
			/* copy data from read-ahead to user buffer */
			if (nsec > num)
				nsec = num;
			bcopy(diskbuf + (dblk - ra_first) * BIOSDISK_SECSIZE,
			    buf, nsec * BIOSDISK_SECSIZE);
		}
		buf += nsec * BIOSDISK_SECSIZE;
		num -= nsec;
		dblk += nsec;
	}

	return (0);
}
