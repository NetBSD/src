/*	$NetBSD: interact.c,v 1.35.12.1 2013/02/25 00:28:04 tls Exp $	*/

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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: interact.c,v 1.35.12.1 2013/02/25 00:28:04 tls Exp $");
#endif /* lint */

#include <sys/param.h>
#define FSTYPENAMES
#define DKTYPENAMES

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#if HAVE_NBTOOL_CONFIG_H
#define	getmaxpartitions()	MAXPARTITIONS
#include <nbinclude/sys/disklabel.h>
#else
#include <util.h>
#include <sys/disklabel.h>
#endif /* HAVE_NBTOOL_CONFIG_H */

#include "extern.h"

static void	cmd_help(struct disklabel *, char *, int);
static void	cmd_adjust(struct disklabel *, char *, int);
static void	cmd_chain(struct disklabel *, char *, int);
static void	cmd_print(struct disklabel *, char *, int);
static void	cmd_printall(struct disklabel *, char *, int);
static void	cmd_info(struct disklabel *, char *, int);
static void	cmd_part(struct disklabel *, char *, int);
static void	cmd_label(struct disklabel *, char *, int);
static void	cmd_round(struct disklabel *, char *, int);
static void	cmd_name(struct disklabel *, char *, int);
static void	cmd_listfstypes(struct disklabel *, char *, int);
static int	runcmd(struct disklabel *, char *, int);
static int	getinput(char *, const char *, ...) __printflike(2, 3);
static int	alphacmp(const void *, const void *);
static void	defnum(struct disklabel *, char *, uint32_t);
static void	dumpnames(const char *, const char * const *, size_t);
static intmax_t	getnum(struct disklabel *, char *, intmax_t);

static int rounding = 0;	/* sector rounding */
static int chaining = 0;	/* make partitions contiguous */

static struct cmds {
	const char *name;
	void (*func)(struct disklabel *, char *, int);
	const char *help;
} cmds[] = {
	{ "?",	cmd_help,	"print this menu" },
	{ "A",	cmd_adjust,	"adjust the label size to the max disk size" },
	{ "C",	cmd_chain,	"make partitions contiguous" },
	{ "E",	cmd_printall,	"print disk label and current partition table"},
	{ "I",	cmd_info,	"change label information" },
	{ "L",	cmd_listfstypes,"list all known file system types" },
	{ "N",	cmd_name,	"name the label" },
	{ "P",	cmd_print,	"print current partition table" },
	{ "Q",	NULL,		"quit" },
	{ "R",	cmd_round,	"rounding (c)ylinders (s)ectors" },
	{ "W",	cmd_label,	"write the current partition table" },
	{ NULL, NULL,		NULL }
};
	
	
static void
cmd_help(struct disklabel *lp, char *s, int fd)
{
	struct cmds *cmd;

	for (cmd = cmds; cmd->name != NULL; cmd++)
		printf("%s\t%s\n", cmd->name, cmd->help);
	printf("[a-%c]\tdefine named partition\n",
	    'a' + getmaxpartitions() - 1);
}


static void
cmd_adjust(struct disklabel *lp, char *s, int fd)
{
	struct disklabel dl;

	if (dk_ioctl(fd, DIOCGDEFLABEL, &dl) == -1) {
		warn("Cannot get default label");
		return;
	}

	if (dl.d_secperunit != lp->d_secperunit) {
		char line[BUFSIZ];
		int i = getinput(line, "Adjust disklabel sector from %" PRIu32
		    " to %" PRIu32 " [n]? ", lp->d_secperunit, dl.d_secperunit);
		if (i <= 0)
			return;
		if (line[0] != 'Y' && line[0] != 'y')
			return;
		lp->d_secperunit = dl.d_secperunit;
		return;
	} 

	printf("Already at %" PRIu32 " sectors\n", dl.d_secperunit);
	return;
}

static void
cmd_chain(struct disklabel *lp, char *s, int fd)
{
	int	i;
	char	line[BUFSIZ];

	i = getinput(line, "Automatically adjust partitions [%s]? ",
	    chaining ? "yes" : "no");
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
cmd_printall(struct disklabel *lp, char *s, int fd)
{

	showinfo(stdout, lp, specname);
	showpartitions(stdout, lp, Cflag);
}


static void
cmd_print(struct disklabel *lp, char *s, int fd)
{

	showpartitions(stdout, lp, Cflag);
}


static void
cmd_info(struct disklabel *lp, char *s, int fd)
{
	char	line[BUFSIZ];
	int	v, i;
	u_int32_t u;

	printf("# Current values:\n");
	showinfo(stdout, lp, specname);

	/* d_type */
	for (;;) {
		i = lp->d_type;
		if (i < 0 || i >= DKMAXTYPES)
			i = 0;
		i = getinput(line, "Disk type [%s]: ", dktypenames[i]);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (!strcmp(line, "?")) {
			dumpnames("Supported disk types", dktypenames,
			    DKMAXTYPES);
			continue;
		}
		for (i = 0; i < DKMAXTYPES; i++) {
			if (!strcasecmp(dktypenames[i], line)) {
				lp->d_type = i;
				goto done_typename;
			}
		}
		v = atoi(line);
		if ((unsigned)v >= DKMAXTYPES) {
			warnx("Unknown disk type: %s", line);
			continue;
		}
		lp->d_type = v;
 done_typename:
		break;
	}

	/* d_typename */
	i = getinput(line, "Disk name [%.*s]: ", 
	    (int) sizeof(lp->d_typename), lp->d_typename);
	if (i == -1)
		return;
	else if (i == 1)
		(void) strncpy(lp->d_typename, line, sizeof(lp->d_typename));

	/* d_packname */
	cmd_name(lp, s, fd);

	/* d_npartitions */
	for (;;) {
		i = getinput(line, "Number of partitions [%" PRIu16 "]: ",
		    lp->d_npartitions);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid number of partitions `%s'\n", line);
			continue;
		}
		lp->d_npartitions = u;
		break;
	}

	/* d_secsize */
	for (;;) {
		i = getinput(line, "Sector size (bytes) [%" PRIu32 "]: ",
		    lp->d_secsize);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid sector size `%s'\n", line);
			continue;
		}
		lp->d_secsize = u;
		break;
	}

	/* d_nsectors */
	for (;;) {
		i = getinput(line, "Number of sectors per track [%" PRIu32
		    "]: ", lp->d_nsectors);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid number of sectors `%s'\n", line);
			continue;
		}
		lp->d_nsectors = u;
		break;
	}

	/* d_ntracks */
	for (;;) {
		i = getinput(line, "Number of tracks per cylinder [%" PRIu32
		    "]: ", lp->d_ntracks);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid number of tracks `%s'\n", line);
			continue;
		}
		lp->d_ntracks = u;
		break;
	}

	/* d_secpercyl */
	for (;;) {
		i = getinput(line, "Number of sectors/cylinder [%" PRIu32 "]: ",
		    lp->d_secpercyl);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid number of sector/cylinder `%s'\n",
			    line);
			continue;
		}
		lp->d_secpercyl = u;
		break;
	}

	/* d_ncylinders */
	for (;;) {
		i = getinput(line, "Total number of cylinders [%" PRIu32 "]: ",
		    lp->d_ncylinders);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid sector size `%s'\n", line);
			continue;
		}
		lp->d_ncylinders = u;
		break;
	}

	/* d_secperunit */
	for (;;) {
		i = getinput(line, "Total number of sectors [%" PRIu32 "]: ",
		    lp->d_secperunit);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid number of sectors `%s'\n", line);
			continue;
		}
		lp->d_secperunit = u;
		break;
	}

	/* d_rpm */

	/* d_interleave */
	for (;;) {
		i = getinput(line, "Hardware sectors interleave [%" PRIu16
		    "]: ", lp->d_interleave);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid sector interleave `%s'\n", line);
			continue;
		}
		lp->d_interleave = u;
		break;
	}

	/* d_trackskew */
	for (;;) {
		i = getinput(line, "Sector 0 skew, per track [%" PRIu16 "]: ",
		    lp->d_trackskew);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid track sector skew `%s'\n", line);
			continue;
		}
		lp->d_trackskew = u;
		break;
	}

	/* d_cylskew */
	for (;;) {
		i = getinput(line, "Sector 0 skew, per cylinder [%" PRIu16
		    "]: ", lp->d_cylskew);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid cylinder sector `%s'\n", line);
			continue;
		}
		lp->d_cylskew = u;
		break;
	}

	/* d_headswitch */
	for (;;) {
		i = getinput(line, "Head switch time (usec) [%" PRIu32 "]: ",
		    lp->d_headswitch);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid head switch time `%s'\n", line);
			continue;
		}
		lp->d_headswitch = u;
		break;
	}

	/* d_trkseek */
	for (;;) {
		i = getinput(line, "Track seek time (usec) [%" PRIu32 "]:",
		    lp->d_trkseek);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (sscanf(line, "%" SCNu32, &u) != 1) {
			printf("Invalid track seek time `%s'\n", line);
			continue;
		}
		lp->d_trkseek = u;
		break;
	}
}


static void
cmd_name(struct disklabel *lp, char *s, int fd)
{
	char	line[BUFSIZ];
	int	i;

	i = getinput(line, "Label name [%.*s]: ",
	    (int) sizeof(lp->d_packname), lp->d_packname);
	if (i <= 0)
		return;
	(void) strncpy(lp->d_packname, line, sizeof(lp->d_packname));
}


static void
cmd_round(struct disklabel *lp, char *s, int fd)
{
	int	i;
	char	line[BUFSIZ];

	i = getinput(line, "Rounding [%s]: ", rounding ? "cylinders" :
	    "sectors");
	if (i <= 0)
		return;

	switch (line[0]) {
	case 'c':
	case 'C':
		rounding = 1;
		return;
	case 's':
	case 'S':
		rounding = 0;
		return;
	default:
		printf("Rounding can be (c)ylinders or (s)ectors\n");
		return;
	}
}


static void
cmd_part(struct disklabel *lp, char *s, int fd)
{
	int	i;
	intmax_t im;
	char	line[BUFSIZ];
	char	def[BUFSIZ];
	int	part;
	struct partition *p, ps;

	part = s[0] - 'a';
	p = &lp->d_partitions[part];
	if (part >= lp->d_npartitions)
		lp->d_npartitions = part + 1;

	(void)memcpy(&ps, p, sizeof(ps));

	for (;;) {
		i = p->p_fstype;
		if (i < 0 || i >= FSMAXTYPES)
			i = 0;
		i = getinput(line, "Filesystem type [%s]: ", fstypenames[i]);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (!strcmp(line, "?")) {
			dumpnames("Supported file system types",
			    fstypenames, FSMAXTYPES);
			continue;
		}
		for (i = 0; i < FSMAXTYPES; i++)
			if (!strcasecmp(line, fstypenames[i])) {
				p->p_fstype = i;
				goto done_typename;
			}
		printf("Invalid file system typename `%s'\n", line);
		continue;
 done_typename:
		break;
	}
	for (;;) {
		defnum(lp, def, p->p_offset);
		i = getinput(line, "Start offset ('x' to start after partition"
		" 'x') [%s]: ", def);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if (line[1] == '\0' &&
	    		line[0] >= 'a' && line[0] < 'a' + getmaxpartitions()) {
			struct partition *cp = lp->d_partitions;

			if ((cp[line[0] - 'a'].p_offset +
			    cp[line[0] - 'a'].p_size) >= lp->d_secperunit) {
				printf("Bad offset `%s'\n", line);
				continue;
			} else {
				p->p_offset = cp[line[0] - 'a'].p_offset +
				    cp[line[0] - 'a'].p_size;
			}
		} else {
			if ((im = getnum(lp, line, 0)) == -1 || im < 0) {
				printf("Bad offset `%s'\n", line);
				continue;
			} else if (im > 0xffffffffLL ||
				   (uint32_t)im > lp->d_secperunit) {
				printf("Offset `%s' out of range\n", line);
				continue;
			}
			p->p_offset = (uint32_t)im;
		}
		break;
	}
	for (;;) {
		defnum(lp, def, p->p_size);
		i = getinput(line, "Partition size ('$' for all remaining) "
		    "[%s]: ", def);
		if (i == -1)
			return;
		else if (i == 0)
			break;
		if ((im = getnum(lp, line, lp->d_secperunit - p->p_offset))
		    == -1) {
			printf("Bad size `%s'\n", line);
			continue;
		} else if (im > 0xffffffffLL ||
			   (im + p->p_offset) > lp->d_secperunit) {
			printf("Size `%s' out of range\n", line);
			continue;
		}
		p->p_size = im;
		break;
	}

	if (memcmp(&ps, p, sizeof(ps)))
		showpartition(stdout, lp, part, Cflag);
	if (chaining) {
		int offs = -1;
		struct partition *cp = lp->d_partitions;
		for (i = 0; i < lp->d_npartitions; i++) {
			if (cp[i].p_fstype != FS_UNUSED) {
				if (offs != -1 && cp[i].p_offset != (uint32_t)offs) {
					cp[i].p_offset = offs;
					showpartition(stdout, lp, i, Cflag);
					}
				offs = cp[i].p_offset + cp[i].p_size;
			}
		}
	}
}


static void
cmd_label(struct disklabel *lp, char *s, int fd)
{
	char	line[BUFSIZ];
	int	i;

	i = getinput(line, "Label disk [n]?");
	if (i <= 0 || (*line != 'y' && *line != 'Y') )
		return;

	if (checklabel(lp) != 0) {
		printf("Label not written\n");
		return;
	}

	if (writelabel(fd, lp) != 0) {
		printf("Label not written\n");
		return;
	}
	printf("Label written\n");
}


static void
cmd_listfstypes(struct disklabel *lp, char *s, int fd)
{

	(void)list_fs_types();
}


static int
runcmd(struct disklabel *lp, char *line, int fd)
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
getinput(char *line, const char *prompt, ...)
{

	for (;;) {
		va_list ap;
		va_start(ap, prompt);
		vprintf(prompt, ap);
		va_end(ap);

		if (fgets(line, BUFSIZ, stdin) == NULL)
			return -1;
		if (line[0] == '\n' || line[0] == '\0')
			return 0;
		else {
			char *p;

			if ((p = strrchr(line, '\n')) != NULL)
				*p = '\0';
			return 1;
		}
	}
}

static int
alphacmp(const void *a, const void *b)
{

	return (strcasecmp(*(const char * const*)a, *(const char * const*)b));
}


static void
dumpnames(const char *prompt, const char * const *olist, size_t numentries)
{
	int	w;
	size_t	i, entry, lines;
	int	columns, width;
	const char *p;
	const char **list;

	if ((list = (const char **)malloc(sizeof(char *) * numentries)) == NULL)
		err(1, "malloc");
	width = 0;
	printf("%s:\n", prompt);
	for (i = 0; i < numentries; i++) {
		list[i] = olist[i];
		w = strlen(list[i]);
		if (w > width)
			width = w;
	}
#if 0
	for (i = 0; i < numentries; i++)
		printf("%s%s", i == 0 ? "" : ", ", list[i]);
	puts("");
#endif
	(void)qsort(list, numentries, sizeof(char *), alphacmp);
	width++;		/* want two spaces between items */
	width = (width + 8) &~ 7;

#define ttywidth 72
	columns = ttywidth / width;
#undef ttywidth
	if (columns == 0)
		columns = 1;
	lines = (numentries + columns - 1) / columns;
	/* Output sorted by columns */
	for (i = 0; i < lines; i++) {
		putc('\t', stdout);
		entry = i;
		for (;;) {
			p = list[entry];
			fputs(p, stdout);
			entry += lines;
			if (entry >= numentries)
				break;
			w = strlen(p);
			while (w < width) {
				w = (w + 8) & ~7;
				putc('\t', stdout);
			}
		}
		putc('\n', stdout);
	}
	free(list);
}


static void
defnum(struct disklabel *lp, char *buf, uint32_t size)
{

	(void) snprintf(buf, BUFSIZ, "%.40gc, %" PRIu32 "s, %.40gM",
	    size / (float) lp->d_secpercyl,
	    size, size  * (lp->d_secsize / (float) (1024 * 1024)));
}


static intmax_t
getnum(struct disklabel *lp, char *buf, intmax_t defaultval)
{
	char	*ep;
	double	 d;
	intmax_t rv;

	if (defaultval && buf[0] == '$' && buf[1] == 0)
		return defaultval;

	d = strtod(buf, &ep);
	if (buf == ep)
		return -1;

#define ROUND(a)	((((a) / lp->d_secpercyl) + \
		 	 (((a) % lp->d_secpercyl) ? 1 : 0)) * lp->d_secpercyl)

	switch (*ep) {
	case '\0':
	case 's':
	case 'S':
		rv = (intmax_t) d;
		break;

	case 'c':
	case 'C':
		rv = (intmax_t) (d * lp->d_secpercyl);
		break;

	case 'k':
	case 'K':
		rv =  (intmax_t) (d * 1024 / lp->d_secsize);
		break;

	case 'm':
	case 'M':
		rv =  (intmax_t) (d * 1024 * 1024 / lp->d_secsize);
		break;

	case 'g':
	case 'G':
		rv =  (intmax_t) (d * 1024 * 1024 * 1024 / lp->d_secsize);
		break;

	case 't':
	case 'T':
		rv =  (intmax_t) (d * 1024 * 1024 * 1024 * 1024 / lp->d_secsize);
		break;

	default:
		printf("Unit error %c\n", *ep);
		printf("Valid units: (S)ectors, (C)ylinders, (K)ilo, (M)ega, "
		    "(G)iga, (T)era");
		return -1;
	}

	if (rounding)
		return ROUND(rv);
	else
		return rv;
}


void
interact(struct disklabel *lp, int fd)
{
	char	line[BUFSIZ];

	puts("Enter '?' for help");
	for (;;) {
		int i = getinput(line, "partition>");
		if (i == -1)
			return;
		if (i == 0)
			continue;
		if (runcmd(lp, line, fd) == -1)
			return;
	}
}
