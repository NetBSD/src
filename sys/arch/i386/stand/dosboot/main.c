/*	$NetBSD: main.c,v 1.3 1997/06/13 13:24:10 drochner Exp $	 */

/*
 * Copyright (c) 1996, 1997
 * 	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996, 1997
 * 	Perry E. Metzger.  All rights reserved.
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
 *    must display the following acknowledgements:
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 *	This product includes software developed for the NetBSD Project
 *	by Perry E. Metzger.
 * 4. The names of the authors may not be used to endorse or promote products
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


#include <sys/reboot.h>

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>

#include <libi386.h>

extern void ls  __P((char *));
extern int getopt __P((int, char **, const char *));

int             errno;
static char    *consdev;

extern char     version[];

#define MAXDEVNAME 16

static char    *current_fsmode;
static char    *default_devname;
static int      default_unit, default_partition;
static char    *default_filename;

int
parsebootfile(fname, fsmode, devname, unit, partition, file)
	const char     *fname;
	char          **fsmode;
	char          **devname;/* out */
	unsigned int   *unit, *partition;	/* out */
	const char    **file;	/* out */
{
	const char     *col, *help;

	*fsmode = current_fsmode;
	*devname = default_devname;
	*unit = default_unit;
	*partition = default_partition;
	*file = default_filename;

	if (!fname)
		return (0);

	if ((col = strchr(fname, ':'))) {	/* device given */
		static char     savedevname[MAXDEVNAME + 1];
		int             devlen;
		unsigned int    u = 0, p = 0;
		int             i = 0;

		devlen = col - fname;
		if (devlen > MAXDEVNAME)
			return (EINVAL);

#define isvalidname(c) ((c) >= 'a' && (c) <= 'z')
		if (!isvalidname(fname[i]))
			return (EINVAL);
		do {
			savedevname[i] = fname[i];
			i++;
		} while (isvalidname(fname[i]));
		savedevname[i] = '\0';

#define isnum(c) ((c) >= '0' && (c) <= '9')
		if (i < devlen) {
			if (!isnum(fname[i]))
				return (EUNIT);
			do {
				u *= 10;
				u += fname[i++] - '0';
			} while (isnum(fname[i]));
		}
#define isvalidpart(c) ((c) >= 'a' && (c) <= 'z')
		if (i < devlen) {
			if (!isvalidpart(fname[i]))
				return (EPART);
			p = fname[i++] - 'a';
		}
		if (i != devlen)
			return (ENXIO);

		*devname = savedevname;
		*unit = u;
		*partition = p;
		help = col + 1;
	} else
		help = fname;

	if (*help)
		*file = help;

	return (0);
}

static void
print_bootsel(filename)
	char           *filename;
{
	char           *fsname;
	char           *devname;
	int             unit, partition;
	const char     *file;

	if (!parsebootfile(filename, &fsname, &devname, &unit,
	    &partition, &file)) {
		if (!strcmp(fsname, "dos"))
			printf("booting %s\n", file);
		else if (!strcmp(fsname, "ufs"))
			printf("booting %s%d%c:%s\n", devname, unit,
			       'a' + partition, file);
	}
}

static void
bootit(filename, howto, tell)
	const char     *filename;
	int             howto, tell;
{
	if (tell)
		print_bootsel(filename);

	if (exec_netbsd(filename, 0, howto, 0, consdev) < 0)
		printf("boot: %s\n", strerror(errno));
	else
		printf("boot returned\n");
}

static void 
helpme()
{
	printf("commands are:\n"
	       "boot [xdNx:][filename] [-adrs]\n"
	       "     (ex. \"sd0a:netbsd.old -s\"\n"
	       "ls [path]\n"
	       "mode ufs|dos\n"
	       "help|?\n"
	       "quit\n");
}

/*
 * chops the head from the arguments and returns the arguments if any,
 * or possibly an empty string.
 */
static char    *
gettrailer(arg)
	char           *arg;
{
	char           *options;

	if ((options = strchr(arg, ' ')) == NULL)
		options = "";
	else
		*options++ = '\0';
	/* trim leading blanks */
	while (*options && *options == ' ')
		options++;

	return (options);
}

static int
parseopts(opts, howto)
	char           *opts;
	int            *howto;
{
	int             tmpopt = 0;

	opts++;			/* skip - */
	while (*opts && *opts != ' ') {
		tmpopt |= netbsd_opt(*opts);
		if (tmpopt == -1) {
			printf("-%c: unknown flag\n", *opts);
			helpme();
			return (0);
		}
		opts++;
	}
	*howto = tmpopt;
	return (1);
}

static int
parseboot(arg, filename, howto)
	char           *arg;
	char          **filename;
	int            *howto;
{
	char           *opts = NULL;

	*filename = 0;
	*howto = 0;

	/* if there were no arguments */
	if (!*arg)
		return (1);

	/* format is... */
	/* [[xxNx:]filename] [-adrs] */

	/* check for just args */
	if (arg[0] == '-') {
		opts = arg;
	} else {		/* at least a file name */
		*filename = arg;

		opts = gettrailer(arg);
		if (!*opts)
			opts = NULL;
		else if (*opts != '-') {
			printf("invalid arguments\n");
			helpme();
			return (0);
		}
	}
	/* at this point, we have dealt with filenames. */

	/* now, deal with options */
	if (opts) {
		if (!parseopts(opts, howto))
			return (0);
	}
	return (1);
}

static void
parsemode(arg, mode)
	char           *arg;
	char          **mode;
{
	if (!strcmp("dos", arg))
		*mode = "dos";
	else if (!strcmp("ufs", arg))
		*mode = "ufs";
	else
		printf("invalid mode\n");
}

static void
docommand(arg)
	char           *arg;
{
	char           *options;

	options = gettrailer(arg);

	if ((strcmp("help", arg) == 0) ||
	    (strcmp("?", arg) == 0)) {
		helpme();
		return;
	}
	if (strcmp("ls", arg) == 0) {
		char           *help = default_filename;
		if (strcmp(current_fsmode, "ufs")) {
			printf("UFS only\n");
			return;
		}
		default_filename = "/";
		ls(options);
		default_filename = help;
		return;
	}
	if (strcmp("quit", arg) == 0) {
		printf("Exiting... goodbye...\n");
		exit(0);
	}
	if (strcmp("boot", arg) == 0) {
		char           *filename;
		int             howto;
		if (parseboot(options, &filename, &howto))
			bootit(filename, howto, 1);
		return;
	}
	if (strcmp("mode", arg) == 0) {
		parsemode(options, &current_fsmode);
		return;
	}
	printf("unknown command\n");
	helpme();
}

void 
bootmenu()
{
	printf("\ntype \"?\" or \"help\" for help.\n");
	for (;;) {
		char            input[80];

		input[0] = '\0';
		printf("> ");
		gets(input);

		docommand(input);
	}
}

static void
print_banner(void)
{
	printf("\n"
	       ">> NetBSD BOOT: %d/%d k [%s]\n",
	       getbasemem(),
	       getextmem(),
	       version);
}

void 
usage()
{
	printf("dosboot [-u] [-c <commands>] [-i] [filename [-bootopts]]\n");
}

int 
main(argc, argv)
	int             argc;
	char          **argv;
{
	int             ch;
	int             interactive = 0;
	int             howto;
	extern char    *optarg;
	extern int      optind;

	consdev = initio(CONSDEV_PC);
	gateA20();

	print_banner();

	current_fsmode = "dos";
	default_devname = "hd";
	default_unit = 0;
	default_partition = 0;
	default_filename = "netbsd";

	while ((ch = getopt(argc, argv, "c:iu")) != -1) {
		switch (ch) {
		case 'c':
			docommand(optarg);
			return (1);
			break;
		case 'i':
			interactive = 1;
			break;
		case 'u':
			current_fsmode = "ufs";
			break;
		default:
			usage();
			return (1);
		}
	}

	if (interactive)
		bootmenu();

	argc -= optind;
	argv += optind;

	if (argc > 2) {
		usage();
		return (1);
	}
	howto = 0;
	if (argc > 1 && !parseopts(argv[1], &howto))
		return (1);

	bootit((argc > 0 ? argv[0] : "netbsd"), howto, 1);
	return (1);
}
