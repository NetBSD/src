/*	$NetBSD: boot.c,v 1.5.6.1 2014/08/20 00:03:30 tls Exp $	*/

/*
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
 * All rights reserved.
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

#include <sys/types.h>
#include <sys/bootblock.h>
#include <sys/boot_flag.h>

#include "boot.h"
#include "bootinfo.h"
#include "bootmenu.h"
#include "disk.h"
#include "unixdev.h"
#include "pathnames.h"

#include <lib/libsa/loadfile.h>
#include <lib/libsa/ufs.h>

#include "compat_linux.h"

static const char * const names[][2] = {
	{ "netbsd", "netbsd.gz" },
	{ "netbsd.old", "netbsd.old.gz", },
	{ "onetbsd", "onetbsd.gz" },
};

char *default_devname;
uint default_unit, default_partition;
const char *default_filename;
int default_timeout = 5;

static char probed_disks[256];
static char bootconfpath[1024];

static void bootcmd_help(char *);
static void bootcmd_ls(char *);
static void bootcmd_quit(char *);
static void bootcmd_boot(char *);
static void bootcmd_disk(char *);
#ifdef SUPPORT_CONSDEV
static void bootcmd_consdev(char *);
#endif

static const struct bootblk_command {
	const char *c_name;
	void (*c_fn)(char *arg);
} bootcmds[] = {
	{ "help",	bootcmd_help },
	{ "?",		bootcmd_help },
	{ "quit",	bootcmd_quit },
	{ "ls",		bootcmd_ls },
	{ "boot",	bootcmd_boot },
	{ "disk",	bootcmd_disk },
#ifdef SUPPORT_CONSDEV
	{ "consdev",	bootcmd_consdev },
#endif
	{ NULL,		NULL },
};

static struct btinfo_howto bi_howto;

static void print_banner(void);
static int exec_netbsd(const char *file, int howto);

int
parsebootfile(const char *fname, char **fsname, char **devname,
	uint *unit, uint *partition, const char **file)
{
	const char *col;

	*fsname = "ufs";
	*devname = default_devname;
	*unit = default_unit;
	*partition = default_partition;
	*file = default_filename;

	if (fname == NULL)
		return 0;

	if ((col = strchr(fname, ':'))) {	/* device given */
		static char savedevname[MAXDEVNAME+1];
		int devlen;
		unsigned int u = 0, p = 0;
		int i = 0;

		devlen = col - fname;
		if (devlen > MAXDEVNAME)
			return EINVAL;

#define isvalidname(c) ((c) >= 'a' && (c) <= 'z')
		if (!isvalidname(fname[i]))
			return EINVAL;
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

#define isvalidpart(c) ((c) >= 'a' && (c) < 'a' + MAXPARTITIONS)
		if (i < devlen) {
			if (!isvalidpart(fname[i]))
				return (EPART);
			p = fname[i++] - 'a';
		}

		if (i != devlen)
			return ENXIO;

		*devname = savedevname;
		*unit = u;
		*partition = p;
		fname = col + 1;
	}

	if (*fname)
		*file = fname;

	return 0;
}

char *
sprint_bootsel(const char *filename)
{
	static char buf[80];
	char *fsname, *devname;
	uint unit, partition;
	const char *file;

	if (parsebootfile(filename, &fsname, &devname, &unit, &partition,
	    &file) == 0) {
		snprintf(buf, sizeof(buf), "%s%d%c:%s", devname, unit,
		    'a' + partition, file);
		return buf;
	}
	return "(invalid)";
}

static void
print_banner(void)
{
	extern const char bootprog_name[];
	extern const char bootprog_rev[];

	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
}

void
boot(dev_t bootdev)
{
	extern char twiddle_toggle;
	int currname;
	int c;

	consinit(CONSDEV_GLASS, -1);

	twiddle_toggle = 1;	/* no twiddling until we're ready */

	/* set default value: hd0a:netbsd */
	default_devname = "hd";
	default_unit = 0;
	default_partition = 0;
	default_filename = names[0][0];

	diskprobe(probed_disks, sizeof(probed_disks));

	snprintf(bootconfpath, sizeof(bootconfpath), "%s%d%c:%s",
	    default_devname, default_unit, 'a' + default_partition,
	    BOOTCFG_FILENAME);
	parsebootconf(bootconfpath);

#ifdef SUPPORT_CONSDEV
	/*
	 * If console set in boot.cfg, switch to it.
	 * This will print the banner, so we don't need to explicitly do it
	 */
	if (bootcfg_info.consdev)
		bootcmd_consdev(bootcfg_info.consdev);
	else 
#endif
		print_banner();

	printf("\ndisks: %s\n", probed_disks);

	/* Display the menu, if applicable */
	twiddle_toggle = 0;
	if (bootcfg_info.nummenu > 0) {
		/* Does not return */
		doboottypemenu();
	}

	printf("Press return to boot now, any other key for boot menu\n");
	currname = 0;
	for (currname = 0; currname < __arraycount(names); currname++) {
		printf("booting %s - starting in ", 
		    sprint_bootsel(names[currname][0]));

		c = awaitkey((bootcfg_info.timeout < 0) ? 0
		    : bootcfg_info.timeout, 1);
		if ((c != '\r') && (c != '\n') && (c != '\0')) {
			printf("type \"?\" or \"help\" for help.\n");
			bootmenu(); /* does not return */
		}

		/*
		 * try pairs of names[] entries, foo and foo.gz
		 */
		/* don't print "booting..." again */
		bootit(names[currname][0], 0, 0);
		/* since it failed, try compressed bootfile. */
		bootit(names[currname][1], 0, 1);
	}

	bootmenu(); /* does not return */
}

void
bootit(const char *filename, int howto, int tell)
{

	if (tell) {
		printf("booting %s", sprint_bootsel(filename));
		if (howto)
			printf(" (howto 0x%x)", howto);
		printf("\n");
	}

	if (exec_netbsd(filename, howto) < 0) {
		printf("boot: %s: %s\n", sprint_bootsel(filename),
		       strerror(errno));
	} else {
		printf("boot returned\n");
	}
}

static int
exec_netbsd(const char *file, int howto)
{
	u_long marks[MARK_MAX];

	BI_ALLOC(BTINFO_MAX);

	bi_howto.howto = howto;
	BI_ADD(&bi_howto, BTINFO_HOWTO, sizeof(bi_howto));

	if (loadfile_zboot(file, marks, LOAD_KERNEL) == -1)
		goto out;

	/*NOTREACHED*/
	return 0;

out:
	BI_FREE();
	bootinfo = 0;
	return -1;
}

/*
 * bootmenu
 */
static char *gettrailer(char *arg);
static int parseopts(const char *opts, int *howto);
static int parseboot(char *arg, char **filename, int *howto);

/* ARGSUSED */
static void
bootcmd_help(char *arg)
{

	printf("commands are:\n"
	    "boot [xdNx:][filename] [-1acdqsv]\n"
	    "     (ex. \"boot hd0a:netbsd.old -s\")\n"
	    "     (ex. \"boot path:/mnt/card/netbsd -1\")\n"
	    "ls [path]\n"
#ifdef SUPPORT_CONSDEV
	    "consdev {glass|com [speed]}\n"
#endif
	    "disk\n"
	    "help|?\n"
	    "quit\n");
}

/* ARGSUSED */
static void
bootcmd_quit(char *arg)
{

	printf("Exiting...\n");
	exit(0);
	/*NOTREACHED*/
}

static void
bootcmd_ls(char *arg)
{
	const char *save = default_filename;

	default_filename = "/";
	ls(arg);
	default_filename = save;
}

static void
bootcmd_boot(char *arg)
{
	char *filename;
	int howto;

	if (parseboot(arg, &filename, &howto)) {
		bootit(filename, howto, 1);
	}
}

/* ARGSUSED */
static void
bootcmd_disk(char *arg)
{

	printf("disks: %s\n", probed_disks);
}

#ifdef SUPPORT_CONSDEV
static const struct cons_devs {
	const char	*name;
	int		tag;
} cons_devs[] = {
	{ "glass",	CONSDEV_GLASS },
	{ "com",	CONSDEV_COM0 },
	{ "com0",	CONSDEV_COM0 },
	{ NULL,		0 }
};

static void
bootcmd_consdev(char *arg)
{
	const struct cons_devs *cdp;
	char *p;
	int speed = 9600;

	p = strchr(arg, ' ');
	if (p != NULL) {
		*p++ = '\0';
		speed = atoi(p);
	}
	for (cdp = cons_devs; cdp->name != NULL; cdp++) {
		if (strcmp(arg, cdp->name) == 0) {
			consinit(cdp->tag, speed);
			print_banner();
			return;
		}
	}
	printf("invalid console device.\n");
}
#endif

void
docommand(char *arg)
{
	char *options;
	int i;

	options = gettrailer(arg);

	for (i = 0; bootcmds[i].c_name != NULL; i++) {
		if (strcmp(arg, bootcmds[i].c_name) == 0) {
			(*bootcmds[i].c_fn)(options);
			return;
		}
	}

	printf("unknown command\n");
	bootcmd_help(NULL);
}

void
bootmenu(void)
{
	char input[256];
	char *c;

	for (;;) {
		c = input;

		input[0] = '\0';
		printf("> ");
		gets(input);

		/*
		 * Skip leading whitespace.
		 */
		while (*c == ' ') {
			c++;
		}
		if (*c != '\0') {
			docommand(c);
		}
	}
}

/*
 * chops the head from the arguments and returns the arguments if any,
 * or possibly an empty string.
 */
static char *
gettrailer(char *arg)
{
	char *options;

	if ((options = strchr(arg, ' ')) == NULL)
		return ("");
	else
		*options++ = '\0';

	/* trim leading blanks */
	while (*options && *options == ' ')
		options++;

	return options;
}

static int
parseopts(const char *opts, int *howto)
{
	int r, tmpopt = 0;

	opts++; 	/* skip - */
	while (*opts && *opts != ' ') {
		r = 0;
		BOOT_FLAG(*opts, r);
		if (r == 0) {
			printf("-%c: unknown flag\n", *opts);
			bootcmd_help(NULL);
			return 0;
		}
		tmpopt |= r;
		opts++;
	}

	*howto = tmpopt;
	return 1;
}

static int
parseboot(char *arg, char **filename, int *howto)
{
	char *opts = NULL;

	*filename = 0;
	*howto = 0;

	/* if there were no arguments */
	if (arg == NULL || *arg == '\0')
		return 1;

	/* format is... */
	/* [[xxNx:]filename] [-adqsv] */

	/* check for just args */
	if (arg[0] == '-') {
		opts = arg;
	} else {
		/* there's a file name */
		*filename = arg;

		opts = gettrailer(arg);
		if (opts == NULL || *opts == '\0') {
			opts = NULL;
		} else if (*opts != '-') {
			printf("invalid arguments\n");
			bootcmd_help(NULL);
			return 0;
		}
	}

	/* at this point, we have dealt with filenames. */

	/* now, deal with options */
	if (opts) {
		if (parseopts(opts, howto) == 0) {
			return 0;
		}
	}
	return 1;
}
