/*	$NetBSD: mopchk.c,v 1.16 2022/05/28 21:14:57 andvar Exp $	*/

/*
 * Copyright (c) 1995-96 Mats O Jansson.  All rights reserved.
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
 */

#include "port.h"
#ifndef lint
__RCSID("$NetBSD: mopchk.c,v 1.16 2022/05/28 21:14:57 andvar Exp $");
#endif

/*
 * mopchk - MOP Check Utility
 *
 * Usage:	mopchk [-a] [-v] [filename...]
 */

#include "os.h"
#include "common.h"
#include "device.h"
#include "file.h"
#include "mopdef.h"
#include "pf.h"
#include "log.h"

/*
 * The list of all interfaces that are being listened to.  rarp_loop()
 * "selects" on the descriptors in this list.
 */
extern struct if_info *iflist;

__dead static void	Usage(void);
void	mopProcess(struct if_info *, u_char *);

int     AllFlag = 0;		/* listen on "all" interfaces  */
int	VersionFlag = 0;	/* Show version */
int	promisc = 0;		/* promisc mode not needed */

extern char	version[];

int
main(int argc, char  **argv)
{
	struct dllist dl;
	int     op, i;
	char   *filename;
	struct if_info *ii;
	int	error;

	mopInteractive = 1;

	opterr = 0;
	while ((op = getopt(argc, argv, "av")) != -1) {
		switch (op) {
		case 'a':
			AllFlag++;
			break;
		case 'v':
			VersionFlag++;
			break;
		default:
			Usage();
			/* NOTREACHED */
		}
	}
	
	if (VersionFlag)
		printf("%s: Version %s\n", getprogname(), version);

	if (AllFlag) {
		if (VersionFlag)
			printf("\n");
		iflist = NULL;
		deviceInitAll();
		if (iflist == NULL)
			printf("No interface\n");
		else {
			printf("Interface Address\n");
			for (ii = iflist; ii; ii = ii->next)
				printf("%-9s %x:%x:%x:%x:%x:%x\n",
				    ii->if_name, ii->eaddr[0], ii->eaddr[1],
				    ii->eaddr[2], ii->eaddr[3], ii->eaddr[4],
				    ii->eaddr[5]);
		}
	}
	
	if (VersionFlag || AllFlag)
		i = 1;
	else
		i = 0;

	while (argc > optind) {
		if (i)	printf("\n");
		i++;
		filename = argv[optind++];
		printf("Checking: %s\n", filename);
		dl.ldfd = open(filename, O_RDONLY, 0);
		if (dl.ldfd == -1)
			printf("Unknown file.\n");
		else {
			if ((error = CheckElfFile(dl.ldfd)) == 0) {
				if (GetElfFileInfo(&dl) < 0) {
					printf(
					"Some failure in GetElfFileInfo\n");
				}
			} else if ((error = CheckAOutFile(dl.ldfd)) == 0) {
				if (GetAOutFileInfo(&dl) < 0) {
					printf(
					"Some failure in GetAOutFileInfo\n");
				}
			} else if ((error = CheckMopFile(dl.ldfd)) == 0) {
				if (GetMopFileInfo(&dl) < 0) {
					printf(
					    "Some failure in GetMopFileInfo\n");
				}
			}
		}
		(void) close(dl.ldfd);
	}
	return (0);
}

static void
Usage(void)
{
	(void) fprintf(stderr, "usage: %s [-a] [-v] [filename...]\n",
	    getprogname());
	exit(1);
}

/*
 * Process incoming packages.
 * Doesn't actually do anything for mopchk(1)
 */
void
mopProcess(struct if_info *ii, u_char *pkt)
{
}
