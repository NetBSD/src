/*	$NetBSD: main.c,v 1.21 2005/06/22 20:42:45 dyoung Exp $	 */

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
#include <lib/libsa/ufs.h>

#include <libi386.h>

#ifdef SUPPORT_LYNX
extern int exec_lynx(const char*, int);
#endif

int errno;

extern	char bootprog_name[], bootprog_rev[], bootprog_date[],
	bootprog_maker[];

#define MAXDEVNAME 16

static char    *current_fsmode;
static char    *default_devname;
static int      default_unit, default_partition;
static char    *default_filename;

char *sprint_bootsel(const char *);
static void bootit(const char *, int, int);
void usage(void);
int main(int, char **);

void	command_help(char *);
void	command_ls(char *);
void	command_quit(char *);
void	command_boot(char *);
void	command_mode(char *);
void	command_dev(char *);

const struct bootblk_command commands[] = {
	{ "help",	command_help },
	{ "?",		command_help },
	{ "ls",		command_ls },
	{ "quit",	command_quit },
	{ "boot",	command_boot },
	{ "mode",	command_mode },
	{ "dev",	command_dev },
	{ NULL,		NULL },
};

int
parsebootfile(fname, fsmode, devname, unit, partition, file)
	const char     *fname;
	char          **fsmode; /* out */
	char          **devname; /* out */
	int            *unit, *partition; /* out */
	const char    **file; /* out */
{
	const char     *col, *help;

	*fsmode = current_fsmode;
	*devname = default_devname;
	*unit = default_unit;
	*partition = default_partition;
	*file = default_filename;

	if (fname == NULL)
		return (0);

	if (strcmp(current_fsmode, "dos") && (col = strchr(fname, ':'))) {
		/* no DOS, device given */
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

char *
sprint_bootsel(filename)
	const char *filename;
{
	char *fsname, *devname;
	int unit, partition;
	const char *file;
	static char buf[80];
	
	if (parsebootfile(filename, &fsname, &devname, &unit,
			  &partition, &file) == 0) {
		if (!strcmp(fsname, "dos"))
			sprintf(buf, "dos:%s", file);
		else if (!strcmp(fsname, "ufs"))
			sprintf(buf, "%s%d%c:%s", devname, unit,
				'a' + partition, file);
		else goto bad;
		return (buf);
	}
bad:
	return ("(invalid)");
}

static void
bootit(filename, howto, tell)
	const char     *filename;
	int             howto, tell;
{
	if (tell) {
		printf("booting %s", sprint_bootsel(filename));
		if (howto)
			printf(" (howto 0x%x)", howto);
		printf("\n");
	}
#ifdef SUPPORT_LYNX
	if(exec_netbsd(filename, 0, howto) < 0)
		printf("boot netbsd: %s: %s\n", sprint_bootsel(filename),
		       strerror(errno));
	else {
		printf("boot netbsd returned\n");
		return;
	}
	if (exec_lynx(filename, 0) < 0)
		printf("boot lynx: %s: %s\n", sprint_bootsel(filename),
		       strerror(errno));
	else
		printf("boot lynx returned\n");
#else
	if (exec_netbsd(filename, 0, howto) < 0)
		printf("boot: %s: %s\n", sprint_bootsel(filename),
		       strerror(errno));
	else
		printf("boot returned\n");
#endif
}

static void
print_banner(void)
{
	int extmem = getextmem();
	char *s = "";

#ifdef XMS
	u_long xmsmem;
	if (getextmem1() == 0 && (xmsmem = checkxms()) != 0) {
		/*
		 * With "CONSERVATIVE_MEMDETECT", extmem is 0 because
		 *  getextmem() is getextmem1(). Without, the "smart"
		 *  methods could fail to report all memory as well.
		 * xmsmem is a few kB less than the actual size, but
		 *  better than nothing.
		 */
		if ((int)xmsmem > extmem)
			extmem = xmsmem;
		s = "(xms) ";
	}
#endif

	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
	printf(">> Memory: %d/%d %sk\n", getbasemem(), extmem, s);
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

#ifdef	SUPPORT_SERIAL
	initio(SUPPORT_SERIAL);
#else
	initio(CONSDEV_PC);
#endif
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

	if (interactive) {
		printf("type \"?\" or \"help\" for help.\n");
		bootmenu();
	}

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

/* ARGSUSED */
void
command_help(arg)
	char *arg;
{
	printf("commands are:\n"
	       "boot [xdNx:][filename] [-acdqsv]\n"
	       "     (ex. \"sd0a:netbsd.old -s\"\n"
	       "ls [path]\n"
	       "mode ufs|dos\n"
	       "dev xd[N[x]]:\n"
	       "help|?\n"
	       "quit\n");
}

void
command_ls(arg)
	char *arg;
{
	char *help = default_filename;
	if (strcmp(current_fsmode, "ufs")) {
		printf("UFS only\n");
		return;
	}
	default_filename = "/";
	ufs_ls(arg);
	default_filename = help;
}

/* ARGSUSED */
void
command_quit(arg)
	char *arg;
{
	printf("Exiting... goodbye...\n");
	exit(0);
}

void
command_boot(arg)
	char *arg;
{
	char *filename;
	int howto;

	if (parseboot(arg, &filename, &howto))
		bootit(filename, howto, 1);
}

void
command_mode(arg)
	char *arg;
{
	if (!strcmp("dos", arg))
		current_fsmode = "dos";
	else if (!strcmp("ufs", arg))
		current_fsmode = "ufs";
	else
		printf("invalid mode\n");
}

void
command_dev(arg)
	char *arg;
{
	static char savedevname[MAXDEVNAME + 1];
	char *fsname, *devname;
	const char *file; /* dummy */

	if (!strcmp(current_fsmode, "dos")) {
		printf("not available in DOS mode\n");
		return;
	}

	if (*arg == '\0') {
		printf("%s%d%c:\n", default_devname, default_unit,
		       'a' + default_partition);
		return;
	}

	if (!strchr(arg, ':') ||
	    parsebootfile(arg, &fsname, &devname, &default_unit,
			  &default_partition, &file)) {
		command_help(NULL);
		return;
	}
	    
	/* put to own static storage */
	strncpy(savedevname, devname, MAXDEVNAME + 1);
	default_devname = savedevname;
}
