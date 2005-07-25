/* $NetBSD: main.c,v 1.3 2005/07/25 11:26:40 drochner Exp $ */

/*
 * Copyright (c) 2002, 2005
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
#include "mscdlabel.h"

static int getcdtoc(int);
static int getfaketoc(int);
int main(int, char **);

const char *disk = "cd0";
int ntracks;
struct cd_toc_entry *tocbuf;

static int
getcdtoc(int fd)
{
	int res;
	struct ioc_toc_header th;
	struct ioc_read_toc_entry te;
	size_t tocbufsize;

	memset(&th, 0, sizeof(th));
	res = ioctl(fd, CDIOREADTOCHEADER, &th);
	if (res < 0) {
		warn("CDIOREADTOCHEADER");
		return (-1);
	}

	ntracks = th.ending_track - th.starting_track + 1;
	/* one more for leadout track, for tracklen calculation */
	tocbufsize = (ntracks + 1) * sizeof(struct cd_toc_entry);
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
	return (0);
}

static int
getfaketoc(int fd)
{
	int res;
	struct stat st;

	res = fstat(fd, &st);
	if (res < 0)
		err(4, "fstat");

	if (st.st_size % 2048) {
		warnx("size not multiple of 2048");
		return (-1);
	}

	tocbuf = malloc(2 * sizeof(struct cd_toc_entry));
	if (!tocbuf)
		err(3, "alloc TOC buffer");

	/*
	 * fake up a data track spanning the whole file and a leadout track,
	 * just as much as necessary for the scan below
	 */
	tocbuf[0].addr.lba = 0;
	tocbuf[0].control = 4;
	tocbuf[1].addr.lba = st.st_size / 2048;
	tocbuf[1].control = 0;
	ntracks = 1;
	return (0);
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	int fd, res, i, j, rawpart;
	char fullname[MAXPATHLEN];
	struct cd_toc_entry *track;
	struct disklabel label;
	struct partition *p;
	int readonly = 0;

	if (argc > 1)
		disk = argv[1];

	fd = opendisk(disk, O_RDWR, fullname, MAXPATHLEN, 0);
	if (fd < 0) {
		warn("opendisk (read-write) %s", disk);
		fd = opendisk(disk, O_RDONLY, fullname, MAXPATHLEN, 0);
		if (fd < 0)
			err(1, "opendisk %s", disk);
		readonly = 1;
	}

	/*
	 * Get the TOC: first try to read a real one from a CD drive.
	 * If this fails we might have something else keeping an ISO image
	 * (eg. vnd(4) or plain file).
	 */
	if (getcdtoc(fd) < 0 && getfaketoc(fd) < 0)
		exit(2);

	/*
	 * Get label template. If this fails we might have a plain file.
	 * Proceed to print out possible ISO label information, but
	 * don't try to write a label back.
	 */
	res = ioctl(fd, DIOCGDINFO, &label);
	if (res < 0) {
		warn("DIOCGDINFO");
		readonly = 1;
	}

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
		if ((track->control & 4) /* data track */
		    && check_primary_vd(fd, track->addr.lba,
		      (track+1)->addr.lba - track->addr.lba)) {
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
	if (!j) /* no ISO track, let the label alone */
		readonly = 1;

	if (!readonly) {
		/* write back label */
		if (label.d_npartitions < j)
			label.d_npartitions = j;
		strncpy(label.d_packname, "mscdlabel's", 16);
		label.d_checksum = 0;
		label.d_checksum = dkcksum(&label);
		res = ioctl(fd, DIOCSDINFO, &label);
		if (res < 0)
			err(6, "DIOCSDINFO");
	} else
		printf("disklabel not written\n");

	free(tocbuf);
	close(fd);
	return (0);
}
