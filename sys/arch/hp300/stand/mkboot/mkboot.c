/*	$NetBSD: mkboot.c,v 1.1.26.1 2001/01/05 17:34:14 bouyer Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mkboot.c	8.1 (Berkeley) 7/15/93
 */

#include <sys/cdefs.h>

#ifndef lint
__COPYRIGHT(
"@(#) Copyright (c) 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#ifdef notdef
static char sccsid[] = "@(#)mkboot.c	7.2 (Berkeley) 12/16/90";
#endif
__RCSID("$NetBSD: mkboot.c,v 1.1.26.1 2001/01/05 17:34:14 bouyer Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "volhdr.h"
#include "loadfile.h"

#define LIF_NUMDIR	8

#define LIF_VOLSTART	0
#define LIF_VOLSIZE	sizeof(struct lifvol)
#define LIF_DIRSTART	512
#define LIF_DIRSIZE	(LIF_NUMDIR * sizeof(struct lifdir))
#define LIF_FILESTART	8192

#define btolifs(b)	(((b) + (SECTSIZE - 1)) / SECTSIZE)
#define lifstob(s)	((s) * SECTSIZE)

int	lpflag;
int	loadpoint;
int	entrypoint;
struct	load ld;
struct	lifvol lifv;
struct	lifdir lifd[LIF_NUMDIR];
char	buf[10240];

int	 main(int, char **);
void	 bcddate(char *, char *);
char	*lifname(char *);
void	 putfile(char *, int);
void	 usage(void);

/*
 * Old Format:
 *	sector 0:	LIF volume header (40 bytes)
 *	sector 1:	<unused>
 *	sector 2:	LIF directory (8 x 32 == 256 bytes)
 *	sector 3-:	LIF file 0, LIF file 1, etc.
 * where sectors are 256 bytes.
 *
 * New Format:
 *	sector 0:	LIF volume header (40 bytes)
 *	sector 1:	<unused>
 *	sector 2:	LIF directory (8 x 32 == 256 bytes)
 *	sector 3:	<unused>
 *	sector 4-31:	disklabel (~300 bytes right now)
 *	sector 32-:	LIF file 0, LIF file 1, etc.
 */
int
main(int argc, char **argv)
{
	char *n1, *n2, *n3;
	int n, to;

	--argc;
	++argv;
	if (argc == 0)
		usage();
	if (!strcmp(argv[0], "-l")) {
		argv++;
		argc--;
		if (argc == 0)
			usage();
		sscanf(argv[0], "0x%x", &loadpoint);
		lpflag++;
		argv++;
		argc--;
	}
	if (argc == 0)
		usage();
	n1 = argv[0];
	argv++;
	argc--;
	if (argc == 0)
		usage();
	if (argc > 1) {
		n2 = argv[0];
		argv++;
		argc--;
		if (argc > 1) {
			n3 = argv[0];
			argv++;
			argc--;
		} else
			n3 = NULL;
	} else
		n2 = n3 = NULL;
	to = open(argv[0], O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (to < 0) {
		perror("open");
		exit(1);
	}
	/* clear possibly unused directory entries */
	strncpy(lifd[1].dir_name, "	     ", 10);
	lifd[1].dir_type = -1;
	lifd[1].dir_addr = 0;
	lifd[1].dir_length = 0;
	lifd[1].dir_flag = 0xFF;
	lifd[1].dir_exec = 0;
	lifd[7] = lifd[6] = lifd[5] = lifd[4] = lifd[3] = lifd[2] = lifd[1];
	/* record volume info */
	lifv.vol_id = VOL_ID;
	strncpy(lifv.vol_label, "BOOT43", 6);
	lifv.vol_addr = btolifs(LIF_DIRSTART);
	lifv.vol_oct = VOL_OCT;
	lifv.vol_dirsize = btolifs(LIF_DIRSIZE);
	lifv.vol_version = 1;
	/* output bootfile one */
	lseek(to, LIF_FILESTART, SEEK_SET);
	putfile(n1, to);
	n = btolifs(ld.count + sizeof(ld));
	strcpy(lifd[0].dir_name, lifname(n1));
	lifd[0].dir_type = DIR_TYPE;
	lifd[0].dir_addr = btolifs(LIF_FILESTART);
	lifd[0].dir_length = n;
	bcddate(n1, lifd[0].dir_toc);
	lifd[0].dir_flag = DIR_FLAG;
	lifd[0].dir_exec = lpflag? loadpoint + entrypoint : entrypoint;
	lifv.vol_length = lifd[0].dir_addr + lifd[0].dir_length;
	/* if there is an optional second boot program, output it */
	if (n2) {
		lseek(to, LIF_FILESTART+lifstob(n), SEEK_SET);
		putfile(n2, to);
		n = btolifs(ld.count + sizeof(ld));
		strcpy(lifd[1].dir_name, lifname(n2));
		lifd[1].dir_type = DIR_TYPE;
		lifd[1].dir_addr = lifv.vol_length;
		lifd[1].dir_length = n;
		bcddate(n2, lifd[1].dir_toc);
		lifd[1].dir_flag = DIR_FLAG;
		lifd[1].dir_exec = lpflag? loadpoint + entrypoint : entrypoint;
		lifv.vol_length = lifd[1].dir_addr + lifd[1].dir_length;
	}
	/* ditto for three */
	if (n3) {
		lseek(to, LIF_FILESTART+lifstob(lifd[0].dir_length+n), SEEK_SET);
		putfile(n3, to);
		n = btolifs(ld.count + sizeof(ld));
		strcpy(lifd[2].dir_name, lifname(n3));
		lifd[2].dir_type = DIR_TYPE;
		lifd[2].dir_addr = lifv.vol_length;
		lifd[2].dir_length = n;
		bcddate(n3, lifd[2].dir_toc);
		lifd[2].dir_flag = DIR_FLAG;
		lifd[2].dir_exec = lpflag? loadpoint + entrypoint : entrypoint;
		lifv.vol_length = lifd[2].dir_addr + lifd[2].dir_length;
	}
	/* output volume/directory header info */
	lseek(to, LIF_VOLSTART, SEEK_SET);
	write(to, &lifv, LIF_VOLSIZE);
	lseek(to, LIF_DIRSTART, SEEK_SET);
	write(to, lifd, LIF_DIRSIZE);
	exit(0);
}

void
putfile(from, to)
	char *from;
	int to;
{
	int fd;
	u_long bp;
	u_long marks[MARK_MAX];

	marks[MARK_START] = 0;
	if ((fd = loadfile(from, marks, COUNT_TEXT|COUNT_DATA)) == -1)
		exit(1);
	(void)close(fd);

	entrypoint = marks[MARK_ENTRY];
	ld.address = lpflag ? loadpoint : entrypoint;
	ld.count = marks[MARK_END] - marks[MARK_START];
	bp = (u_long)malloc(ld.count);
	marks[MARK_START] = bp - marks[MARK_START];

	if ((fd = loadfile(from, marks, LOAD_TEXT|LOAD_DATA)) == -1)
		exit(1);
	(void)close(fd);

	write(to, &ld, sizeof(ld));
	write(to, (void *)bp, ld.count);
}

void
usage(void)
{

	fprintf(stderr,
		"usage:	 mkboot [-l loadpoint] prog1 [ prog2 ] outfile\n");
	exit(1);
}

char *
lifname(char *str)
{
	static char lname[10] = "SYS_XXXXX";
	char *cp;
	int i;

	if ((cp = strrchr(str, '/')) != NULL)
		str = ++cp;
	for (i = 4; i < 9; i++) {
		if (islower(*str))
			lname[i] = toupper(*str);
		else if (isalnum(*str) || *str == '_')
			lname[i] = *str;
		else
			break;
		str++;
	}
	for ( ; i < 10; i++)
		lname[i] = '\0';
	return(lname);
}

void
bcddate(char *name, char *toc)
{
	struct stat statb;
	struct tm *tm;

	stat(name, &statb);
	tm = localtime(&statb.st_ctime);
	*toc = ((tm->tm_mon+1) / 10) << 4;
	*toc++ |= (tm->tm_mon+1) % 10;
	*toc = (tm->tm_mday / 10) << 4;
	*toc++ |= tm->tm_mday % 10;
	*toc = (tm->tm_year / 10) << 4;
	*toc++ |= tm->tm_year % 10;
	*toc = (tm->tm_hour / 10) << 4;
	*toc++ |= tm->tm_hour % 10;
	*toc = (tm->tm_min / 10) << 4;
	*toc++ |= tm->tm_min % 10;
	*toc = (tm->tm_sec / 10) << 4;
	*toc |= tm->tm_sec % 10;
}
