/* $NetBSD: main.c,v 1.1 2002/05/29 19:39:07 drochner Exp $ */

/*
 * Copyright (c) 2002
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
 * Generate an in-core disklabel for a CD, containing entries for
 * previous data tracks (supposed to be of previous sessions).
 * TODO:
 *  - check ISO9660 volume descriptor, print volume labels, perhaps
 *    allow selection by creation date
 *  - support simulation of multisession CDs in a vnd(4) disk
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/cdio.h>
#include <sys/disklabel.h>
#include <sys/param.h>
#include <err.h>
#include <util.h>
#include <string.h>

#include "dkcksum.h"

int main(int, char **);

char *disk = "cd0";

int
main(argc, argv)
	int argc;
	char **argv;
{
	int fd, res, i, j, rawpart;
	char fullname[MAXPATHLEN];
	struct ioc_toc_header th;
	struct ioc_read_toc_entry te;
	int ntracks;
	size_t tocbufsize;
	struct cd_toc_entry *tocbuf, *track;
	struct disklabel label;
	struct partition *p;

	if (argc > 1)
		disk = argv[1];

	fd = opendisk(disk, O_RDWR, fullname, MAXPATHLEN, 0);
	if (fd < 0)
		err(1, "opendisk %s", disk);

	/*
	 * get the TOC
	 */
	memset(&th, 0, sizeof(th));
	res = ioctl(fd, CDIOREADTOCHEADER, &th);
	if (res < 0)
		err(2, "CDIOREADTOCHEADER");
	ntracks = th.ending_track - th.starting_track + 1;
	tocbufsize = ntracks * sizeof(struct cd_toc_entry);
	tocbuf = malloc(tocbufsize);
	if (!tocbuf)
		err(3, "alloc TOC buffer");
	memset(&te, 0, sizeof(te));
	te.address_format = CD_LBA_FORMAT;
	te.starting_track = th.starting_track; /* always 1 ??? */
	te.data_len = tocbufsize;
	te.data = tocbuf;
	res = ioctl(fd, CDIOREADTOCENTRIES, &te);
	if (res < 0)
		err(4, "CDIOREADTOCENTRIES");

	/* get label template */
	res = ioctl(fd, DIOCGDINFO, &label);
	if (res < 0)
		err(5, "DIOCGDINFO");

	/*
	 * We want entries for the sessions beginning with the most recent
	 * one, in reversed time order.
	 * We don't have session information here, but it is uncommon
	 * to have more than one data track in one session, so we get
	 * the same result.
	 */
	rawpart = getrawpartition();
	i = ntracks;
	j = 0;
	while (--i >= 0 && j < MAXPARTITIONS) {
		track = &tocbuf[i];
		printf("track (ctl=%d) at sector %d\n", track->control,
		       track->addr.lba);
		if (track->control & 4) { /* data track */
			printf(" adding as '%c'\n", 'a' + j);
			p = &label.d_partitions[j];
			memset(p, 0, sizeof(struct partition));
			p->p_size = label.d_partitions[rawpart].p_size;
			p->p_fstype = FS_ISO9660;
			p->p_cdsession = track->addr.lba;
			if (++j == rawpart)
				j++;
		}
	}
	if (label.d_npartitions < j)
		label.d_npartitions = j;
	strncpy(label.d_packname, "mscdlabel's", 16);

	/* write back label */
	label.d_checksum = 0;
	label.d_checksum = dkcksum(&label);
	res = ioctl(fd, DIOCSDINFO, &label);
	if (res < 0)
		err(6, "DIOCSDINFO");

	free(tocbuf);
	close(fd);
	return (0);
}
