/* 
 * Copyright (c) 1987-1993, The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	from: Utah $Hdr: hilinfo.c 1.3 94/04/04$
 *	$NetBSD: hilinfo.c,v 1.5 2002/05/30 18:01:03 thorpej Exp $
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: hilinfo.c,v 1.5 2002/05/30 18:01:03 thorpej Exp $");
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <dev/hilioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int aflg = 0;
int tflg = 1;
char *dname;
struct _hilbuf11 hi;
struct _hilbuf16 sc;

struct hil_info {
	u_char	hil_lo;
	u_char	hil_hi;
	char	*hil_name;
} info[] = {
	{0xA0,	0xFF,	"keyboard"},
	{0x60,	0x6B,	"mouse"},
	{0x90,	0x97,	"tablet"},
	{0x34,	0x34,	"id-module"},
	{0x30,	0x30,	"button-box"},
	{0x00,	0x00,	"unknown"},
};

int	getinfo __P((void));
int	main __P((int, char **));
char   *tname __P((void));
void	printall __P((void));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	int c;
	int multi;

	while ((c = getopt(argc, argv, "at")) != -1)
		switch (c) {
		/* everything */
		case 'a':
			aflg++;
			tflg = 0;
			break;
		/* type */
		case 't':
			tflg++;
			aflg = 0;
			break;
		/* bogon */
		case '?':
			usage();
		}
	if (optind == argc)
		usage();
	multi = (argc - optind) - 1;
	while (optind < argc) {
		dname = argv[optind];
		if (multi)
			printf("%s: ", dname);
		if (getinfo()) {
			if (aflg)
				printall();
			else if (tflg)
				printf("%s\n", tname());
		}
		optind++;
	}
	exit(0);
}

int
getinfo()
{
	int f;

	f = open(dname, 0);
	if (f < 0 || ioctl(f, HILIOCID, &hi) < 0) {
		if (tflg)
			printf(errno == EBUSY ? "busy\n" : "none\n");
		else {
			printf("error\n");
			perror(dname);
		}
		close(f);
		return(0);
	}
	(void) ioctl(f, HILIOCSC, &sc);
	close(f);
	return(1);
}

void
printall()
{
	int i;

	printf("%s: %s, info: ", dname, tname());
	for (i = 0; i < 11; i++)
		printf("%2.2x", hi.string[i]);
	if (strcmp(tname(), "id-module") == 0) {
		printf(", sc: ");
		for (i = 0; i < 16; i++)
			printf("%2.2x", sc.string[i]);
	}
	printf("\n");
}

char *
tname()
{
	struct hil_info *hp;

	for (hp = info; hp->hil_lo; hp++)
		if (hi.string[0] >= hp->hil_lo && hi.string[0] <= hp->hil_hi)
			break;
	if (hi.string[0] == 0x61 && hi.string[1] == 0x13)
		return("knobs");
	return(hp->hil_name);
}

void
usage()
{

	fprintf(stderr, "usage: %s [-at] device\n", getprogname());
	exit(1);
}
