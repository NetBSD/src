/*	$NetBSD: biosdisk.c,v 1.3 1997/03/22 01:41:34 thorpej Exp $	*/

/*
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
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
 *
 */

/*
 * raw BIOS disk device for libsa. needs lowlevel parts from bios_disk.S and
 * biosdisk_ll.c partly from netbsd:sys/arch/i386/boot/disk.c no bad144
 * handling!
 */

/*
 * Ported to boot 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 *
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <sys/types.h>
#include <sys/disklabel.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/saerrno.h>

#include "libi386.h"
#include "biosdisk_ll.h"
/* XXX don't include biosdisk.h for now - vararg prototype */

#define BUFSIZE (1 * BIOSDISK_SECSIZE)

struct biosdisk {
	struct biosdisk_ll ll;
#ifdef COMPAT_OLDBOOT
	int             disktype;
#endif
	int             boff;
	char            buf[BUFSIZE];
};

int 
biosdiskstrategy(devdata, flag, dblk, size, buf, rsize)
	void           *devdata;
	int             flag;
	daddr_t         dblk;
	size_t          size;
	void           *buf;
	size_t         *rsize;
{
	struct biosdisk *d;
	int             blks, frag;

	if (flag != F_READ)
		return (EROFS);

	d = (struct biosdisk *) devdata;

	dblk += d->boff;

	blks = size / BIOSDISK_SECSIZE;
	if (blks && readsects(&d->ll, dblk, blks, buf, 0)) {
		if (rsize)
			*rsize = 0;
		return (EIO);
	}
	/* do we really need this? */
	frag = size % BIOSDISK_SECSIZE;
	if (frag) {
		if (readsects(&d->ll, dblk + blks, 1, d->buf, 0)) {
			if (rsize)
				*rsize = blks * BIOSDISK_SECSIZE;
			return (EIO);
		}
		bcopy(d->buf, buf + blks * BIOSDISK_SECSIZE, frag);
	}
	if (rsize)
		*rsize = size;
	return (0);
}

#ifdef COMPAT_OLDBOOT
int 
biosdisk_gettype(f)
	struct open_file *f;
{
	struct biosdisk *d = f->f_devdata;
	return (d->disktype);
}
#endif

int 
biosdiskopen(f, biosdev, partition)
	struct open_file *f;
	int             biosdev;
	unsigned int    partition;
{
	struct biosdisk *d;
	struct dos_partition *dptr;
	int             sector;
	int             error = 0, i;
#ifndef NO_DISKLABEL
	struct disklabel *lp;
#endif

	d = (struct biosdisk *) alloc(sizeof(struct biosdisk));
	if (!d) {
#ifdef DEBUG
		printf("biosdiskopen: no memory\n");
#endif
		return (ENOMEM);
	}
	d->ll.dev = biosdev;
	if (set_geometry(&d->ll)) {
#ifdef DISK_DEBUG
		printf("no geometry information\n");
#endif
		error = ENXIO;
		goto out;
	}
	/*
	 * find NetBSD Partition in DOS partition table XXX check magic???
	 */
	if (readsects(&d->ll, 0, 1, d->buf, 0)) {
#ifdef DISK_DEBUG
		printf("error reading mbr\n");
#endif
		error = EIO;
		goto out;
	}
	dptr = (struct dos_partition *) & d->buf[DOSPARTOFF];
	sector = -1;
	for (i = 0; i < NDOSPART; i++, dptr++)
		if (dptr->dp_typ == DOSPTYP_NETBSD) {
			sector = dptr->dp_start;
			break;
		}
	if (sector == -1) {
		/*
		 * One of two things:
		 * 	1. no MBR
		 *	2. no NetBSD partition in MBR
		 *
		 * We simply default to "start of disk" in this case and
		 * press on.
		 */
		sector = 0;
	}
#ifdef NO_DISKLABEL
	d->boff = sector;
#else
	/* find partition in NetBSD disklabel */
	if (readsects(&d->ll, sector + LABELSECTOR, 1, d->buf, 0)) {
#ifdef DISK_DEBUG
		printf("Error reading disklabel\n");
#endif
		error = EIO;
		goto out;
	}
	lp = (struct disklabel *) (d->buf + LABELOFFSET);
	if (lp->d_magic != DISKMAGIC) {
#ifdef DISK_DEBUG
		printf("warning: no disklabel\n");
#endif
		d->boff = sector;
	} else if (partition >= lp->d_npartitions ||
		   lp->d_partitions[partition].p_fstype == FS_UNUSED) {
#ifdef DISK_DEBUG
		printf("illegal partition\n");
#endif
		error = EPART;
		goto out;
	} else {
		d->boff = lp->d_partitions[partition].p_offset;
#ifdef COMPAT_OLDBOOT
		d->disktype = lp->d_type;
#endif
	}
#endif				/* NO_DISKLABEL */

#ifdef DISK_DEBUG
	printf("partition @%d\n", d->boff);
#endif

	f->f_devdata = d;
out:
	if (error)
		free(d, sizeof(struct biosdisk));
	return (error);
}

int 
biosdiskclose(f)
	struct open_file *f;
{
	struct biosdisk *d = f->f_devdata;

	if (!(d->ll.dev & 0x80))/* let the floppy drive go off */
		delay(3000000);	/* 2s is enough on all PCs I found */

	free(d, sizeof(struct biosdisk));
	f->f_devdata = NULL;
	return (0);
}

int 
biosdiskioctl(f, cmd, arg)
	struct open_file *f;
	u_long          cmd;
	void           *arg;
{
	return EIO;
}
