/* $NetBSD: ahdilabel.c,v 1.1.1.1 2000/08/07 09:23:40 leo Exp $ */

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

/*
 * I think we can safely assume a fixed blocksize - AHDI won't support
 * something different...
 */
#define	BLPM            ((1024 * 1024) / DEV_BSIZE)
#define	UNITS_SECTORS	0
#define	UNITS_CTS	1

int		 main (int, char*[]);
void		 show_parts (struct ahdi_ptable*, int, int, int);
void		 get_input (char *, int);
char		*sec_to_cts (struct ahdi_ptable*, u_int32_t, char *);
u_int32_t	 read_sector (struct ahdi_ptable*,char *);
void		 change_part (struct ahdi_ptable*, int, int);

int
main (argc, argv)
	int	 argc;
	char	*argv[];
{
	struct ahdi_ptable	ptable;
	int			flags, rv, key, units;

	if (argc < 2) {
		fprintf (stderr, "usage: %s raw_disk\n", argv[0]);
		exit (EXIT_FAILURE);
	}
	
	flags = 0;
	while ((rv = ahdi_readlabel(&ptable, argv[1], flags)) != 1) {
		switch (rv) {
		case -1:
			fprintf (stderr,
			    "%s: %s: %s\n", argv[0], argv[1],
			    strerror (errno));
			exit (EXIT_FAILURE);
			break;
		case -2:
			fprintf (stderr,
			    "%s: disk not 512 bytes/sector\n", argv[0]);
			exit (EXIT_FAILURE);
			break;
		case -3:
			printf ("No AHDI partitions found.  Continue (y/N)?");
			if (toupper(getchar()) == 'Y') {
				(void) fpurge(stdin);
				flags |= FORCE_AHDI;
			} else
				exit (EXIT_FAILURE);
			break;
		case -4:
		case -5:
		case -6:
			printf ("Errors reading AHDI partition table.  Override (y/N)? ");
			if (toupper(getchar()) == 'Y') {
				(void) fpurge(stdin);
				flags |= AHDI_IGN_EXISTS | AHDI_IGN_EXT |
				    AHDI_IGN_CKSUM | AHDI_IGN_SPU;
			} else
				exit (EXIT_FAILURE);
			break;
		case 1:
			/* Everything is OK */
			break;
		default:
			exit (EXIT_FAILURE);
			break;
		}
	}

	units = UNITS_SECTORS;
	show_parts (&ptable, 0, ptable.nparts, units);
	key = 0;
	while (key != 'Q') {
		(void) fpurge(stdin);
		printf ("Change [a-p], r)ecalculate, s)how, u)nits, w)rite or q)uit ");
		key = toupper(getchar());
		if (key == EOF)
			key = 'Q';
		if (key >= 'A' && key <= 'P') {
			change_part (&ptable, key - 'A', units);
		}
		if (key == 'R') {
			if (ahdi_buildlabel (&ptable))
				printf ("Partiton table adjusted\n");
		}
		if (key == 'S') {
			show_parts (&ptable, 0, ptable.nparts, units);
		}
		if (key == 'U') {
			if (units == UNITS_SECTORS)
				units = UNITS_CTS;
			else
				units = UNITS_SECTORS;
		}
		if (key == 'W') {
			if ((rv = ahdi_writelabel (&ptable, argv[1], 0)) < 0) {
				if (rv == -1)
					perror ("\0");
				if (rv == -2)
					printf ("Invalid number of partitions!\n");
				if (rv == -3)
					printf ("GEM partition should be BGM or BGM partition should be GEM!\n");
				if (rv == -4)
					printf ("Partition overlaps root sector or bad sector list (starts before sector 2)!\n");
				if (rv == -5)
					printf ("Partition extends past end of disk!\n");
				if (rv == -6)
					printf ("Partitions overlap!\n");
				if (rv == -7)
					printf ("Partition overlaps auxilliary root!\n");
				if (rv == -8)
					printf ("More than 4 partitions in root sector!\n");
				if (rv == -9)
					printf ("More than 1 partition in an auxiliary root!\n");
				if (rv < -1 && ahdi_errp1 != -1)
					printf ("\tpartition %c has errors.\n",
					    ahdi_errp1 + 'a');
				if (rv < -1 && ahdi_errp2 != -1)
					printf ("\tpartition %c has errors.\n",
					    ahdi_errp2 + 'a');
			}
		}
	}
	return (0);
}

void
show_parts (ptable, start, finish, units)
	struct ahdi_ptable	*ptable;
	int			 start, finish, units;
{
	int	i;

	printf ("Disk information :\n");
	printf ("  sectors/track: %d\n", ptable->nsectors);
	printf ("  tracks/cylinder: %d\n", ptable->ntracks);
	printf ("  sectors/cylinder: %d\n", ptable->secpercyl);
	printf ("  cylinders: %d\n", ptable->ncylinders);
	printf ("  total sectors: %d\n", ptable->secperunit);

	if (units == UNITS_SECTORS) {
		printf ("  #  id      root     start       end      size   MBs\n");
		for (i = start; i < finish; i++) {
			printf ("  %c %c%c%c  %8u  %8u  %8u  %8u  (%4u)\n",
			    i + 'a', ptable->parts[i].id[0],
			    ptable->parts[i].id[1], ptable->parts[i].id[2],
			    ptable->parts[i].root, ptable->parts[i].start,
			    ptable->parts[i].start +
			    (ptable->parts[i].size ?
			    ptable->parts[i].size - 1 : 0),
			    ptable->parts[i].size,
			    (ptable->parts[i].size + (BLPM >> 1)) / BLPM);
		}
	} else {
		u_int32_t	cylinder, track, sector;
		printf ("  #  id          root         start           end          size    MBs\n");
		for (i = start; i < finish; i++) {
			printf ("  %c %c%c%c  ", i + 'a',
			    ptable->parts[i].id[0], ptable->parts[i].id[1],
			    ptable->parts[i].id[2]);
			sector = ptable->parts[i].root;
			cylinder = sector / ptable->secpercyl;
			sector -= cylinder * ptable->secpercyl;
			track = sector / ptable->nsectors;
			sector -= track * ptable->nsectors;
			printf ("%5u/%2u/%3u  ", cylinder, track, sector);
			sector = ptable->parts[i].start;
			cylinder = sector / ptable->secpercyl;
			sector -= cylinder * ptable->secpercyl;
			track = sector / ptable->nsectors;
			sector -= track * ptable->nsectors;
			printf ("%5u/%2u/%3u  ", cylinder, track, sector);
			sector = ptable->parts[i].start +
			    (ptable->parts[i].size ?
			    ptable->parts[i].size - 1 : 0),
			cylinder = sector / ptable->secpercyl;
			sector -= cylinder * ptable->secpercyl;
			track = sector / ptable->nsectors;
			sector -= track * ptable->nsectors;
			printf ("%5u/%2u/%3u  ", cylinder, track, sector);
			sector = ptable->parts[i].size;
			cylinder = sector / ptable->secpercyl;
			sector -= cylinder * ptable->secpercyl;
			track = sector / ptable->nsectors;
			sector -= track * ptable->nsectors;
			printf ("%5u/%2u/%3u   ", cylinder, track, sector);
			printf ("(%4u)\n",
			    (ptable->parts[i].size + (BLPM >> 1)) / BLPM);
		}
	}
}

void
get_input (buf, len)
	char	*buf;
	int	 len;
{
	int count, key;

	count = 0;
	(void) fpurge(stdin);
	while (count < (len - 1) && key != '\n' && key != '\r') {
		key = getchar();
		buf[count] = key;
		count++;
	}
	buf[count] = '\0';
}

char *
sec_to_cts (ptable, sector, cts)
	struct ahdi_ptable	*ptable;
	u_int32_t	 sector;
	char		*cts;
{
	u_int32_t	cylinder, track;

	cylinder = sector / ptable->secpercyl;
	sector -= cylinder * ptable->secpercyl;
	track = sector / ptable->nsectors;
	sector -= track * ptable->nsectors;
	sprintf (cts, "%u/%u/%u", cylinder, track, sector);
	return (cts);
}

u_int32_t
read_sector (ptable, buf)
	struct ahdi_ptable	*ptable;
	char	*buf;
{
	u_int32_t	sector, track, cylinder;

	sector = track = cylinder = 0;
	if ((strchr (buf, '/') != NULL) &&
	    ((sscanf (buf, "%u/%u/%u", &cylinder, &track, &sector) == 3) ||
	    (sscanf (buf, "%u/%u/", &cylinder, &track) == 2) ||
	    (sscanf (buf, "%u/", &cylinder) == 1))) {
		if (sector > ptable->nsectors || track > ptable->ntracks ||
		    cylinder > ptable->ncylinders)
			return (0);
		sector += ptable->nsectors * track;
		sector += ptable->secpercyl * cylinder;
		return (sector);
	}
	if (sscanf (buf, "%u", &sector) == 1)
		return (sector);
	return (0);
}

void
change_part (ptable, part, units)
	struct ahdi_ptable	*ptable;
	int			 part, units;
{
#define BUFLEN	20
#define CTSLEN	64
	char		buf[BUFLEN], cts[CTSLEN];
	u_int32_t	sector;
	
	if (part > ptable->nparts) {
		part = ptable->nparts;
		printf ("Changing partition %c!\n", part + 'a');
		ptable->nparts++;
	}
	if (part == ptable->nparts)
		ptable->nparts++;
	show_parts (ptable, part, part + 1, units);

	printf ("id [%c%c%c] ", ptable->parts[part].id[0],
	    ptable->parts[part].id[1], ptable->parts[part].id[2]);
	get_input (&buf[0], BUFLEN);
	if (buf[0] != '\n' && buf[0] != '\r') {
		ptable->parts[part].id[0] = buf[0];
		ptable->parts[part].id[1] = buf[1];
		ptable->parts[part].id[2] = buf[2];
	}

	printf ("root [%8u (%s)] ", ptable->parts[part].root,
	    sec_to_cts (ptable, ptable->parts[part].root, &cts[0]));
	get_input (&buf[0], BUFLEN);
	if (buf[0] != '\n' && buf[0] != '\r') {
		sector = read_sector (ptable, buf);
			ptable->parts[part].root = sector;
	}

	printf ("start [%8u (%s)] ", ptable->parts[part].start,
	    sec_to_cts (ptable, ptable->parts[part].start, &cts[0]));
	get_input (&buf[0], BUFLEN);
	if (buf[0] != '\n' && buf[0] != '\r') {
		sector = read_sector (ptable, buf);
		if (sector)
			ptable->parts[part].start = sector;
	}

	printf ("size [%8u (%s)] ", ptable->parts[part].size,
	    sec_to_cts (ptable, ptable->parts[part].size, &cts[0]));
	get_input (&buf[0], BUFLEN);
	if (buf[0] != '\n' && buf[0] != '\r') {
		sector = read_sector (ptable, buf);
		if (sector)
			ptable->parts[part].size = sector;
	}

/*
	printf ("NetBSD disk letter [%c] ", ptable->parts[part].letter + 'a');
	get_input (&buf[0], BUFLEN);
	if (buf[0] != '\n' && buf[0] != '\r')
		if (buf[0] == 'a' || (buf[0] >= 'd' && buf[0] <= 'p'))
			ptable->parts[part].letter = buf[0] - 'a';
*/

	if (!ptable->parts[part].start && !ptable->parts[part].size) {
	    if (part == ptable->nparts - 1)
		ptable->nparts--;
	}
}
