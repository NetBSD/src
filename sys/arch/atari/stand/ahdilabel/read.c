/*	$NetBSD: read.c,v 1.1.1.1.4.3 2001/03/12 13:27:57 bouyer Exp $	*/

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman, Waldi Ravens and Leo Weppelman.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "privahdi.h"
#include <fcntl.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>

/*
 * Read AHDI partitions from disk.
 */
int
ahdi_readlabel (ptable, diskname, flags)
	struct ahdi_ptable	*ptable;
	char			*diskname;
	int			 flags;
{
	int			 fd, rv;
	struct disklabel	*dl;

	if (!(fd = openraw (diskname, O_RDONLY)))
		return (-1);

	if ((dl = read_dl (fd)) == NULL) {
		close (fd);
		return (-1);
	}

	if (dl->d_secsize != AHDI_BSIZE) {
		close (fd);
		return (-2);
	}

	if ((rv = check_magic(fd, LABELSECTOR, flags)) < 0) {
		close (fd);
		return (rv);
	}

	bzero ((void *) ptable, sizeof (struct ahdi_ptable));

	if ((rv = read_rsec (fd, ptable, AHDI_BBLOCK, AHDI_BBLOCK, flags))
	    < 0) {
		close (fd);
		return (rv);
	}

	if (dl->d_secperunit != ptable->secperunit) {
		if (flags & AHDI_IGN_SPU)
			ptable->secperunit = dl->d_secperunit;
		else {
			close (fd);
			return (-6);
		}
	}

	ptable->nsectors = dl->d_nsectors;
	ptable->ntracks = dl->d_ntracks;
	ptable->ncylinders = dl->d_ncylinders;
	ptable->secpercyl = dl->d_secpercyl;

	assign_letters (ptable);

	close (fd);
	return (1);
}

/*
 * Read AHDI partitions from root sector/auxillary root sector.
 */
int
read_rsec (fd, ptable, rsec, esec, flags)
	int			 fd;
	struct ahdi_ptable	*ptable;
	u_int			 rsec, esec;
	int			 flags;
{
	struct ahdi_part	*part, *end;
	struct ahdi_root	*root;
	u_int16_t		 cksum, newcksum;
	int			 rv;

	if ((root = disk_read (fd, rsec, 1)) == NULL) {
		return (-1);
	}

	if (rsec == AHDI_BBLOCK) {
		end = &root->ar_parts[AHDI_MAXRPD];
		if (root->ar_checksum) {
			cksum = root->ar_checksum;
			root->ar_checksum = 0;
			newcksum = ahdi_cksum (root);
			if ((cksum != newcksum) && !(flags & AHDI_IGN_CKSUM)) {
				free (root);
				return (-4);
			}
		}
		ptable->secperunit=root->ar_hdsize;
	} else
		end = &root->ar_parts[AHDI_MAXARPD];
	for (part = root->ar_parts; part < end; ++part) {
#ifdef DEBUG
		printf ("Found partitions at sector %u:\n", rsec);
		printf ("  flags : %02x\n", part->ap_flg);
		printf ("  id    : %c%c%c\n", part->ap_id[0], part->ap_id[1],
		    part->ap_id[2]);
		printf ("  start : %u\n", part->ap_st);
		printf ("  size  : %u\n", part->ap_size);
#endif
		if (!(part->ap_flg & 0x01)) {
			if ((part->ap_id[0] || part->ap_id[1] ||
			    part->ap_id[2]) && (flags & AHDI_IGN_EXISTS))
				part->ap_flg &= 0x01;
			else
				continue;
		}
			
		if (AHDI_MKPID (part->ap_id[0], part->ap_id[1],
		    part->ap_id[2]) == AHDI_PID_XGM) {
			u_int	offs = part->ap_st + esec;
			if ((rv = read_rsec (fd, ptable, offs,
			    esec == AHDI_BBLOCK ? offs : esec, flags)) < 0) {
				free (root);
				return (rv);
			}
		} else {
			/* Attempt to check for junk values */
			if (((part->ap_st + rsec) > ptable->secperunit ||
			    (part->ap_st + rsec + part->ap_size -1) >
			    ptable->secperunit)) {
				if (flags & AHDI_IGN_EXT) {
					/* Fake previous partition */
					ptable->parts[ptable->nparts].id[0] =
					    part->ap_id[0];
					ptable->parts[ptable->nparts].id[1] =
					    part->ap_id[1];
					ptable->parts[ptable->nparts].id[2] =
					    part->ap_id[2];
				} else {
					free (root);
					return (-5);
				}
			}
			ptable->parts[ptable->nparts].flag = part->ap_flg;
			ptable->parts[ptable->nparts].id[0] = part->ap_id[0];
			ptable->parts[ptable->nparts].id[1] = part->ap_id[1];
			ptable->parts[ptable->nparts].id[2] = part->ap_id[2];
			ptable->parts[ptable->nparts].root = rsec;
			ptable->parts[ptable->nparts].start =
			    part->ap_st + rsec;
			ptable->parts[ptable->nparts].size = part->ap_size;
			ptable->nparts++;
		}
	}
	free (root);
	if (ptable->nparts || FORCE_AHDI)
		return (1);
	else
		return (-3);
}

/*
 * Read a sector from the disk.
 */
void *
disk_read (fd, start, count)
	int	 fd;
	u_int	 start,
		 count;
{
	void	*buffer;
	off_t	offset;
	size_t	size;


	size   = count * DEV_BSIZE;
	offset = start * DEV_BSIZE;

	if ((buffer = malloc (size)) == NULL)
		return (NULL);

	if (lseek (fd, offset, SEEK_SET) != offset) {
		free (buffer);
		return (NULL);
	}
	if (read (fd, buffer, size) != size) {
		free (buffer);
		return (NULL);
	}
	return (buffer);
}

/*
 * Assign NetBSD drive letters to partitions
 */
void
assign_letters (ptable)
	struct ahdi_ptable	*ptable;
{
	int	 	i, have_root, pno;
	u_int32_t	pid;

#define ROOT_PART 0

	have_root = 0;
	pno = 0;
	
	for (i = 0; i < ptable->nparts; i++) {
		while (pno == ROOT_PART || pno == SWAP_PART || pno == RAW_PART)
			pno++;
		pid = AHDI_MKPID (ptable->parts[i].id[0],
		    ptable->parts[i].id[1], ptable->parts[i].id[2]);
		if (!have_root && pid == AHDI_PID_NBD) {
			ptable->parts[i].letter = ROOT_PART;
			have_root = 1;
		} else if (pid == AHDI_PID_SWP)
			ptable->parts[i].letter = SWAP_PART;
		else {
			ptable->parts[i].letter = pno;
			pno++;
		}
	}
}

/*
 * Read disklabel for disk.
 */
struct disklabel *
read_dl (fd)
	int	 fd;
{
	struct disklabel	*dl;

	if ((dl = malloc (sizeof (struct disklabel))) == NULL) {
		return (NULL);
	}

	if (ioctl (fd, DIOCGDINFO, dl) < 0) {
		free (dl);
		return (NULL);
	}
	return (dl);
}
