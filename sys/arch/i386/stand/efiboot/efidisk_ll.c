/*	$NetBSD: efidisk_ll.c,v 1.1.12.1 2018/03/13 14:54:52 martin Exp $	 */
/*	NetBSD: biosdisk_ll.c,v 1.31 2011/02/21 02:58:02 jakllsch Exp	 */

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bang Jun-Young.
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

#include "efiboot.h"

#include "biosdisk_ll.h"
#include "diskbuf.h"
#include "efidisk.h"

static int do_read(struct biosdisk_ll *, daddr_t, int, char *);

#ifndef BIOSDISK_RETRIES
#define BIOSDISK_RETRIES 5
#endif

int
set_geometry(struct biosdisk_ll *d, struct biosdisk_extinfo *ed)
{
	const struct efidiskinfo *edi;
	EFI_BLOCK_IO_MEDIA *media;

	edi = efidisk_getinfo(d->dev);
	if (edi == NULL)
		return 1;

	media = edi->bio->Media;

	d->secsize = media->BlockSize;
	d->type = edi->type;
	d->flags = BIOSDISK_INT13EXT;

	if (ed != NULL) {
		ed->totsec = media->LastBlock + 1;
		ed->sbytes = media->BlockSize;
		ed->flags = 0;
		if (media->RemovableMedia)
			ed->flags |= EXTINFO_REMOVABLE;
	}

	return 0;
}

/*
 * Global shared "diskbuf" is used as read ahead buffer.  For reading from
 * floppies, the bootstrap has to be loaded on a 64K boundary to ensure that
 * this buffer doesn't cross a 64K DMA boundary.
 */
static int      ra_dev;
static daddr_t  ra_end;
static daddr_t  ra_first;

static int
do_read(struct biosdisk_ll *d, daddr_t dblk, int num, char *buf)
{
	EFI_STATUS status;
	const struct efidiskinfo *edi;

	edi = efidisk_getinfo(d->dev);
	if (edi == NULL)
		return -1;

	status = uefi_call_wrapper(edi->bio->ReadBlocks, 5, edi->bio,
	    edi->media_id, dblk, num * d->secsize, buf);
	if (EFI_ERROR(status))
		return -1;
	return num;
}

/*
 * NB if 'cold' is set below not all of the program is loaded, so
 * mustn't use data segment, bss, call library functions or do read-ahead.
 */
int
readsects(struct biosdisk_ll *d, daddr_t dblk, int num, char *buf, int cold)
{
	while (num) {
		int nsec;

		/* check for usable data in read-ahead buffer */
		if (cold || diskbuf_user != &ra_dev || d->dev != ra_dev
		    || dblk < ra_first || dblk >= ra_end) {

			/* no, read from disk */
			char *trbuf;
			int maxsecs;
			int retries = BIOSDISK_RETRIES;

			if (cold) {
				/* transfer directly to buffer */
				trbuf = buf;
				maxsecs = num;
			} else {
				/* fill read-ahead buffer */
				trbuf = alloc_diskbuf(0); /* no data yet */
				maxsecs = DISKBUFSIZE / d->secsize;
			}

			while ((nsec = do_read(d, dblk, maxsecs, trbuf)) < 0) {
#ifdef DISK_DEBUG
				if (!cold)
					printf("read error dblk %"PRId64"-%"PRId64"\n",
					    dblk, (dblk + maxsecs - 1));
#endif
				if (--retries >= 0)
					continue;
				return -1;	/* XXX cannot output here if
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
			memcpy(buf,
			       diskbufp + (dblk - ra_first) * d->secsize,
			       nsec * d->secsize);
		}
		buf += nsec * d->secsize;
		num -= nsec;
		dblk += nsec;
	}

	return 0;
}
