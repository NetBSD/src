/*	$NetBSD: write.c,v 1.1.1.1.4.3 2001/03/12 13:27:57 bouyer Exp $	*/

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

#define BSL_MAGIC	0xa5
#define BSL_OFFSET	1
#define BSL_SIZE	1

/*
 * Write AHDI partitions to disk
 */

int
ahdi_writelabel (ptable, diskname, flags)
	struct ahdi_ptable	*ptable;
	char			*diskname;
	int			 flags;
{
	int			 fd, i, j, k, firstxgm, keep, cksum_ok;
	struct ahdi_root	*root;
	u_int			 rsec;
	u_int32_t		 xgmsec, nbdsec;

	if (!(fd = openraw (diskname, O_RDWR)))
		return (-1);

	if ((i = ahdi_checklabel (ptable)) < 0) {
		close (fd);
		return (i);
	}

	if (flags & AHDI_KEEP_BOOT) {
		if ((root = disk_read (fd, AHDI_BBLOCK, 1)) == NULL) {
			return (-1);
		}
		cksum_ok = ahdi_cksum (root) == root->ar_checksum;
#ifdef DEBUG
		printf ("Previous root sector checksum was ");
		cksum_ok ? printf (" correct\n") : printf (" incorrect\n");
#endif
		bzero ((void *) root->ar_parts,
		    sizeof (struct ahdi_part) * AHDI_MAXRPD);
	} else {
		if ((root = malloc (sizeof (struct ahdi_root))) == NULL) {
			close (fd);
			return (-1);
		}
		bzero ((void *) root, sizeof (struct ahdi_root));
		cksum_ok = 0;
#ifdef DEBUG
		printf ("Clearing root sector - forcing incorrect checksum\n");
#endif
	}

	nbdsec = 0;
#ifdef DEBUG
	printf ("Writing root sector\n");
#endif

        /* All partitions in root sector (including first XGM) */
	j = 0;
	firstxgm = 0;
	for (i = 0; i < ptable->nparts; i++) {
		if (ptable->parts[i].root == 0) {
#ifdef DEBUG
			printf ("  Partition %d - ", j);
#endif
			root->ar_parts[j].ap_flg = 0x01;
			for (k = 0; k < 3; k++) {
				root->ar_parts[j].ap_id[k] =
				    ptable->parts[i].id[k];
#ifdef DEBUG
				printf ("%c", root->ar_parts[j].ap_id[k]);
#endif
			}
			root->ar_parts[j].ap_st = ptable->parts[i].start;
			root->ar_parts[j].ap_size = ptable->parts[i].size;
#ifdef DEBUG
			printf ("/%u/%u\n", root->ar_parts[j].ap_st,
			    root->ar_parts[j].ap_size);
#endif

			j++;
		} else if (!firstxgm) {
			root->ar_parts[j].ap_flg = 0x01;
			root->ar_parts[j].ap_id[0] = 'X';
			root->ar_parts[j].ap_id[1] = 'G';
			root->ar_parts[j].ap_id[2] = 'M';
			root->ar_parts[j].ap_st = ptable->parts[i].root;
			root->ar_parts[j].ap_size = ptable->parts[i].size + 1;
			firstxgm = i;
			xgmsec = ptable->parts[i].root;
#ifdef DEBUG
			printf ("  Partition %d - XGM/%u/%u\n", j,
			    root->ar_parts[j].ap_st,
			    root->ar_parts[j].ap_size);
#endif
			j++;
		}
		/*
		 * Note first netbsd partition for invalidate_netbsd_label().
		 */
		if (!nbdsec && AHDI_MKPID (ptable->parts[i].id[0],
		    ptable->parts[i].id[1], ptable->parts[i].id[2])
		    == AHDI_PID_NBD) {
			nbdsec = ptable->parts[i].start;
		}
	}

	root->ar_hdsize = ptable->secperunit;
	if (!(flags & AHDI_KEEP_BSL)) {
		root->ar_bslst = (u_int32_t) BSL_OFFSET;
		root->ar_bslsize = (u_int32_t) BSL_SIZE;
	}

	/* Write correct checksum? */
	root->ar_checksum = ahdi_cksum (root);
	if (!cksum_ok) {
		root->ar_checksum ^= 0x5555;
#ifdef DEBUG
		printf ("Setting incorrect checksum\n");
	} else {
		printf ("Setting correct checksum\n");
#endif
	}

	if (!disk_write (fd, AHDI_BBLOCK, 1, root)) {
		free (root);
		close (fd);
		return (-1);
	}

	/* Auxiliary roots */
	for (i = firstxgm; i < ptable->nparts; i++) {
		j = 0;
		if (ptable->parts[i].root == 0)
			continue;
#ifdef DEBUG
	printf ("Writing auxiliary root at sector %u\n",
	    ptable->parts[i].root);
#endif
		bzero ((void *) root, sizeof (struct ahdi_root));
		rsec = ptable->parts[i].root;
#ifdef DEBUG
			printf ("  Partition %d - ", j);
#endif
		root->ar_parts[j].ap_flg = 0x01;
		for (k = 0; k < 3; k++) {
			root->ar_parts[j].ap_id[k] =
			    ptable->parts[i].id[k];
#ifdef DEBUG
			printf ("%c", root->ar_parts[j].ap_id[k]);
#endif
		}
		root->ar_parts[j].ap_st = ptable->parts[i].start -
		    rsec;
		root->ar_parts[j].ap_size = ptable->parts[i].size;
#ifdef DEBUG
		printf ("/%u/%u\n", root->ar_parts[j].ap_st,
		    root->ar_parts[j].ap_size);
#endif
		j++;
		if (i < ptable->nparts - 1) {
			/* Need an XGM? */
			if (ptable->parts[i].root != ptable->parts[i+1].root &&
			    ptable->parts[i+1].root != 0) {
				root->ar_parts[j].ap_flg = 0x01;
				root->ar_parts[j].ap_id[0] = 'X';
				root->ar_parts[j].ap_id[1] = 'G';
				root->ar_parts[j].ap_id[2] = 'M';
				root->ar_parts[j].ap_st =
					ptable->parts[i+1].root - xgmsec;
				root->ar_parts[j].ap_size =
					ptable->parts[i+1].size + 1;
#ifdef DEBUG
				printf ("  Partition %d - XGM/%u/%u\n", j,
				    root->ar_parts[j].ap_st,
				    root->ar_parts[j].ap_size);
#endif
			}
			if (ptable->parts[i].root == ptable->parts[i+1].root) {
				/* Next partition has same auxiliary root */
#ifdef DEBUG
				printf ("  Partition %d - ", j);
#endif
				root->ar_parts[j].ap_flg = 0x01;
				for (k = 0; k < 3; k++) {
					root->ar_parts[j].ap_id[k] =
			    		ptable->parts[i+1].id[k];
#ifdef DEBUG
				printf ("%c", root->ar_parts[j].ap_id[k]);
#endif
				}
				root->ar_parts[j].ap_st =
				    ptable->parts[i+1].start - rsec;
				root->ar_parts[j].ap_size =
				    ptable->parts[i+1].size;
#ifdef DEBUG
				printf ("/%u/%u\n", root->ar_parts[j].ap_st,
				    root->ar_parts[j].ap_size);
#endif
				i++;
			}
			j++;
		}

		if (!disk_write (fd, rsec, 1, root)) {
			close (fd);
			free (root);
			return (-1);
		}
		
		/*
		 * Note first netbsd partition for invalidate_netbsd_label().
		 */
		if (!nbdsec && AHDI_MKPID (ptable->parts[i].id[0],
		    ptable->parts[i].id[1], ptable->parts[i].id[2])
		    == AHDI_PID_NBD) {
			nbdsec = ptable->parts[i].start;
		}
	}

	free (root);

	if (!(flags & AHDI_KEEP_BSL) && !write_bsl (fd)) {
		close (fd);
		return (-1);
	}

	if (!(flags & AHDI_KEEP_NBDA) && !invalidate_netbsd_label(fd, nbdsec)) {
		close (fd);
		return (-1);
	}

#ifdef DEBUG
	printf ("Forcing disk label re-read\n");
#endif
	keep = 0;
	if (ioctl (fd, DIOCKLABEL, &keep) < 0) {
		close (fd);
		return (-1);
	}

	close (fd);
	return (1);
}

/*
 * Write a bad sector list (empty).
 */
int
write_bsl (fd)
	int	fd;
{
	u_int8_t	*bsl;

	if ((bsl = malloc (sizeof (u_int8_t) * BSL_SIZE * DEV_BSIZE)) == NULL)
		return (0);
	bzero ((void *) bsl, sizeof (u_int8_t) * DEV_BSIZE);

#ifdef DEBUG
	printf ("Writing bad sector list\n");
#endif
	bsl[3] = BSL_MAGIC;
	if (!disk_write (fd, (u_int) BSL_OFFSET, (u_int) BSL_SIZE, bsl)) {
		free (bsl);
		return (0);
	}
	free (bsl);
	return (1);
}

/*
 * Invalidate any previous AHDI/NBDA disklabel.
 * Otherwise this make take precedence when we next open the disk.
 */
int
invalidate_netbsd_label (fd, nbdsec)
	int		 fd;
	u_int32_t	nbdsec;
{
	struct bootblock	*bb;
	u_int			 nsec;

	nsec = (BBMINSIZE + (DEV_BSIZE - 1)) / DEV_BSIZE;

	if ((bb = disk_read (fd, nbdsec, nsec)) == NULL) {
		return (0);
	}

	if (bb->bb_magic == NBDAMAGIC || bb->bb_magic == AHDIMAGIC) {
		bb->bb_magic = bb->bb_magic & 0xffffff00;
		bb->bb_magic = bb->bb_magic | 0x5f;

#ifdef DEBUG
		printf ("Invalidating old NBDA/AHDI label (sector %u)\n",
		    nbdsec);
#endif
		if (!disk_write (fd, nbdsec, nsec, bb)) {
			free (bb);
			return (0);
		}
	}

	free (bb);
	return (1);
}

int
disk_write (fd, start, count, buf)
	int	 fd;
	u_int	 start,
		 count;
	void	*buf;
{
	off_t	 offset;
	size_t	 size;

	size   = count * DEV_BSIZE;
	offset = start * DEV_BSIZE;

	if (lseek (fd, offset, SEEK_SET) != offset)
		return (0);
	if (write (fd, buf, size) != size)
		return (0);
	return (1);
}
