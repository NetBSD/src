/*	$NetBSD: interact.c,v 1.10 1999/09/05 07:27:55 abs Exp $	*/

/*
 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: interact.c,v 1.10 1999/09/05 07:27:55 abs Exp $");
#endif /* lint */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util.h>
#include <sys/types.h>
#include <sys/param.h>
#define FSTYPENAMES
#include <sys/disklabel.h>

#include "extern.h"

static void cmd_help __P((struct disklabel *, char *, int));
static void cmd_chain __P((struct disklabel *, char *, int));
static void cmd_print __P((struct disklabel *, char *, int));
static void cmd_part __P((struct disklabel *, char *, int));
static void cmd_label __P((struct disklabel *, char *, int));
static void cmd_round __P((struct disklabel *, char *, int));
static void cmd_name __P((struct disklabel *, char *, int));
static int runcmd __P((char *, struct disklabel *, int));
static int getinput __P((const char *, const char *, const char *, char *));
static void defnum __P((char *, struct disklabel *, int));
static int getnum __P((char *, struct disklabel *));
static void deffstypename __P((char *, int));
static int getfstypename __P((const char *));

static int rounding = 0;	/* sector rounding */
static int chaining = 0;	/* make partitions contiguous */

static struct cmds {
	const char *name;
	void (*func) __P((struct disklabel *, char *, int));
	const char *help;
} cmds[] = {
	{ "?",	cmd_help,	"print this menu" },
	{ "C",	cmd_chain,	"make partitions contiguous" },
	{ "N",	cmd_name,	"name the label" },
	{ "P",	cmd_print,	"print current partition table" },
	{ "Q",	NULL,		"quit" },
	{ "R",	cmd_round,	"rounding (c)ylinders (s)ectors" },
	{ "W",	cmd_label,	"write the current partition table" },
	{ NULL, NULL,		NULL }
};
	
	

static void
cmd_help(lp, s, fd)
	struct disklabel *lp;
	char *s;
	int fd;
{
	struct cmds *cmd;

	for (cmd = cmds; cmd->name != NULL; cmd++)
		printf("%s\t%s\n", cmd->name, cmd->help);
	printf("[a-%c]\tdefine named partition\n",
	    'a' + getmaxpartitions() - 1);
}


static void
cmd_chain(lp, s, fd)
	struct disklabel *lp;
	char *s;
	int fd;
{
	int i;
	char line[BUFSIZ];

	i = getinput(":", "Automatically adjust partitions",
	    chaining ? "yes" : "no", line);

	if (i <= 0)
		return;

	switch (line[0]) {
	case 'y':
		chaining = 1;
		return;
	case 'n':
		chaining = 0;
		return;
	default:
		printf("Invalid answer\n");
		return;
	}
}

static void
cmd_print(lp, s, fd)
	struct disklabel *lp;
	char *s;
	int fd;
{
	showpartitions(stdout, lp);
}


static void
cmd_name(lp, s, fd)
	struct disklabel *lp;
	char *s;
	int fd;
{
	char line[BUFSIZ];
	int i = getinput(":", "Label name", lp->d_packname, line);

	if (i <= 0)
		return;
	(void) strncpy(lp->d_packname, line, sizeof(lp->d_packname));
}


static void
cmd_round(lp, s, fd)
	struct disklabel *lp;
	char *s;
	int fd;
{
	int i;
	char line[BUFSIZ];

	i = getinput(":", "Rounding", rounding ? "cylinders" : "sectors", line);

	if (i <= 0)
		return;

	switch (line[0]) {
	case 'c':
		rounding = 1;
		return;
	case 's':
		rounding = 0;
		return;
	default:
		printf("Rounding can be (c)ylinders or (s)ectors\n");
		return;
	}
}


static void
cmd_part(lp, s, fd)
	struct disklabel *lp;
	char *s;
	int fd;
{
	int i;
	char line[BUFSIZ];
	char def[BUFSIZ];
	int part = *s - 'a';
	struct partition *p = &lp->d_partitions[part];

	if (part >= lp->d_npartitions)
		lp->d_npartitions = part + 1;

	for (;;) {
		deffstypename(def, p->p_fstype);
		i = getinput(":", "Filesystem type", def, line);
		if (i <= 0)
			break;
		if ((i = getfstypename(line)) == -1) {
			printf("Invalid file system typename `%s'\n", line);
			continue;
		}
		p->p_fstype = i;
		break;
	}
	for (;;) {
		defnum(def, lp, p->p_offset);
		i = getinput(":", "Start offset", def, line);
		if (i <= 0)
			break;
		if ((i = getnum(line, lp)) == -1) {
			printf("Bad offset `%s'\n", line);
			continue;
		}
		p->p_offset = i;
		break;
	}
	for (;;) {
		defnum(def, lp, p->p_size);
		i = getinput(":", "Partition size", def, line);
		if (i <= 0)
			break;
		if ((i = getnum(line, lp)) == -1) {
			printf("Bad size `%s'\n", line);
			continue;
		}
		p->p_size = i;
		break;
	}

	if (chaining) {
		int offs = p[0].p_offset + p[0].p_size;
		p = lp->d_partitions;
		part = getrawpartition();
		for (i = 1; i < lp->d_npartitions; i++) {
			if (i != part && p[i].p_fstype) {
				p[i].p_offset = offs;
				offs = p[i].p_offset + p[i].p_size;
			}
		}
	}
}


static void
cmd_label(lp, s, fd)
	struct disklabel *lp;
	char *s;
	int fd;
{
	char line[BUFSIZ];
	int i;

	i = getinput("?", "Label disk", "n", line);

	if (i <= 0 || (*line != 'y' && *line != 'Y') )
		return;

	if (checklabel(lp) != 0) {
		printf("Label not written\n");
		return;
	}

	if (writelabel(fd, bootarea, lp) != 0) {
		printf("Label not written\n");
		return;
	}
	printf("Label written\n");
}


static int
runcmd(line, lp, fd)
	char *line;
	struct disklabel *lp;
	int fd;
{
	struct cmds *cmd;

	for (cmd = cmds; cmd->name != NULL; cmd++)
		if (strncmp(line, cmd->name, strlen(cmd->name)) == 0) {
			if (cmd->func == NULL)
				return -1;
			(*cmd->func)(lp, line, fd);
			return 0;
		}

	if (line[1] == '\0' &&
	    line[0] >= 'a' && line[0] < 'a' + getmaxpartitions()) {
		cmd_part(lp, line, fd);
		return 0;
	}
		
	printf("Unknown command %s\n", line);
	return 1;
}


static int
getinput(sep, prompt, def, line)
	const char *sep;
	const char *prompt;
	const char *def;
	char *line;
{
	for (;;) {
		printf("%s", prompt);
		if (def)
			printf(" [%s]", def);
		printf("%s ", sep);

		if (fgets(line, BUFSIZ, stdin) == NULL)
			return -1;
		if (line[0] == '\n' || line[0] == '\0') {
			if (def)
				return 0;
		}
		else {
			char *p;

			if ((p = strrchr(line, '\n')) != NULL)
				*p = '\0';
			return 1;
		}
	}
}


static void
defnum(buf, lp, size)
	char *buf;
	struct disklabel *lp;
	int size;
{
	(void) snprintf(buf, BUFSIZ, "%gc, %ds, %gM",
	    size / (float) lp->d_secpercyl,
	    size, size  * (lp->d_secsize / (float) (1024 * 1024)));
}


static int
getnum(buf, lp)
	char *buf;
	struct disklabel *lp;
{
	char *ep;
	double d = strtod(buf, &ep);
	int rv;

	if (buf == ep)
		return -1;

#define ROUND(a)	((a / lp->d_secpercyl) + \
			 ((a % lp->d_secpercyl) ? 1 : 0)) * lp->d_secpercyl

	switch (*ep) {
	case '\0':
	case 's':
		rv = (int) d;
		break;

	case 'c':
		rv = (int) (d * lp->d_secpercyl);
		break;

	case 'M':
		rv =  (int) (d * 1024 * 1024 / lp->d_secsize);
		break;

	default:
		printf("Unit error %c\n", *ep);
		return -1;
	}

	if (rounding)
		return ROUND(rv);
	else
		return rv;
}


static void
deffstypename(buf, i)
	char *buf;
	int i;
{
	if (i < 0 || i >= FSMAXTYPES)
		i = 0;
	(void) strcpy(buf, fstypenames[i]);
}


static int
getfstypename(buf)
	const char *buf;
{
	int i;

	for (i = 0; i < FSMAXTYPES; i++)
		if (strcmp(buf, fstypenames[i]) == 0)
			return i;
	return -1;
}


void
interact(lp, fd)
	struct disklabel *lp;
	int fd;
{
	char line[BUFSIZ];

	for (;;) {
		if (getinput(">", "partition", NULL, line) == -1)
			return;
		if (runcmd(line, lp, fd) == -1)
			return;
	}
}
