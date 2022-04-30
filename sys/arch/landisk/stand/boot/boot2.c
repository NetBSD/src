/*	$NetBSD: boot2.c,v 1.7 2022/04/30 03:37:09 rin Exp $	*/

/*
 * Copyright (c) 2003
 *	David Laight.  All rights reserved
 * Copyright (c) 1996, 1997, 1999
 * 	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996, 1997
 * 	Perry E. Metzger.  All rights reserved.
 * Copyright (c) 1997
 *	Jason R. Thorpe.  All rights reserved
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

/* Based on stand/biosboot/main.c */

#include <sys/types.h>
#include <sys/reboot.h>
#include <sys/bootblock.h>
#include <sys/boot_flag.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libsa/ufs.h>
#include <lib/libkern/libkern.h>

#include "biosdisk.h"

#include "boot.h"
#include "bootinfo.h"
#include "cons.h"

int errno;

extern struct landisk_boot_params boot_params;

static const char * const names[][2] = {
	{ "netbsd", "netbsd.gz" },
	{ "netbsd.old", "netbsd.old.gz", },
	{ "onetbsd", "onetbsd.gz" },
};

#define	NUMNAMES	(sizeof(names) / sizeof(names[0]))
#define DEFFILENAME	names[0][0]

#define	MAXDEVNAME	16

static char *default_devname;
static uint default_unit, default_partition;
static const char *default_filename;

char *sprint_bootsel(const char *filename);
void bootit(const char *filename, int howto, int tell);
void print_banner(void);
void boot2(uint32_t boot_biossector);

int exec_netbsd(const char *file, int howto);

static char *gettrailer(char *arg);
static int parseopts(const char *opts, int *howto);
static int parseboot(char *arg, char **filename, int *howto);

void bootmenu(void);
static void bootcmd_help(char *);
static void bootcmd_ls(char *);
static void bootcmd_quit(char *);
static void bootcmd_halt(char *);
static void bootcmd_boot(char *);
static void bootcmd_monitor(char *);

static const struct bootblk_command {
	const char *c_name;
	void (*c_fn)(char *arg);
} bootcmds[] = {
	{ "help",	bootcmd_help },
	{ "?",		bootcmd_help },
	{ "ls",		bootcmd_ls },
	{ "quit",	bootcmd_quit },
	{ "halt",	bootcmd_halt },
	{ "boot",	bootcmd_boot },
	{ "!",		bootcmd_monitor },
	{ NULL,		NULL },
};

int
parsebootfile(const char *fname, char **devname,
	uint *unit, uint *partition, const char **file)
{
	const char *col;

	*devname = default_devname;
	*unit = default_unit;
	*partition = default_partition;
	*file = default_filename;

	if (fname == NULL)
		return (0);

	if((col = strchr(fname, ':'))) {	/* device given */
		static char savedevname[MAXDEVNAME+1];
		int devlen;
		unsigned int u = 0, p = 0;
		int i = 0;

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

#define isvalidpart(c) ((c) >= 'a' && (c) <= 'p')
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
		fname = col + 1;
	}

	if (*fname)
		*file = fname;

	return (0);
}

char *
sprint_bootsel(const char *filename)
{
	static char buf[80];
	char *devname;
	uint unit, partition;
	const char *file;

	if (parsebootfile(filename, &devname, &unit, &partition, &file) == 0) {
		snprintf(buf, sizeof(buf), "%s%d%c:%s", devname, unit,
		    'a' + partition, file);
		return (buf);
	}
	return ("(invalid)");
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

void
print_banner(void)
{
	extern const char bootprog_name[];
	extern const char bootprog_rev[];

	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
}

void
boot2(uint32_t boot_biossector)
{
	int currname;
	int c;

	/* Initialize hardware */
	tick_init();

	/* Initialize console */
	cninit(boot_params.bp_consdev);

	print_banner();

	/* try to set default device to what BIOS tells us */
	bios2dev(0x40, &default_devname, &default_unit,
		boot_biossector, &default_partition);

	/* if the user types "boot" without filename */
	default_filename = DEFFILENAME;

	printf("Press return to boot now, any other key for boot menu\n");
	currname = 0;
	for (;;) {
		printf("booting %s - starting in ", 
		    sprint_bootsel(names[currname][0]));

		c = awaitkey(boot_params.bp_timeout, 1);
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
		/* since it failed, try switching bootfile. */
		currname = (currname + 1) % NUMNAMES;
	}
}

int
exec_netbsd(const char *file, int howto)
{
	static char bibuf[BOOTINFO_MAXSIZE];
	u_long marks[MARK_MAX];
	int fd;

	BI_ALLOC(6);	/* XXX */

	marks[MARK_START] = 0;	/* loadaddr */
	if ((fd = loadfile(file, marks, LOAD_KERNEL)) == -1)
		goto out;

	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n", marks[MARK_ENTRY],
	    marks[MARK_NSYM], marks[MARK_SYM], marks[MARK_END]);

	{
		struct btinfo_common *help;
		char *p;
		int i;

		p = bibuf;
		memcpy(p, &bootinfo->nentries, sizeof(bootinfo->nentries));
		p += sizeof(bootinfo->nentries);
		for (i = 0; i < bootinfo->nentries; i++) {
			help = (struct btinfo_common *)(bootinfo->entry[i]);
			memcpy(p, help, help->len);
			p += help->len;
		}
	}

	cache_flush();
	cache_disable();

	(*(void (*)(int, void *))marks[MARK_ENTRY])(howto, bibuf);
	panic("exec returned");

out:
	BI_FREE();
	bootinfo = 0;
	return (-1);
}

/*
 * boot menu
 */
/* ARGSUSED */
static void
bootcmd_help(char *arg)
{

	printf("commands are:\n"
	    "boot [xdNx:][filename] [-acdqsv]\n"
	    "     (ex. \"hd0a:netbsd.old -s\"\n"
	    "ls [path]\n"
	    "help|?\n"
	    "halt\n"
	    "quit\n");
}

static void
bootcmd_ls(char *arg)
{
	const char *save = default_filename;

	default_filename = "/";
	ls(arg);
	default_filename = save;
}

/* ARGSUSED */
static void
bootcmd_quit(char *arg)
{

	printf("Exiting...\n");
	delay(1000);
	reboot();
	/* Note: we shouldn't get to this point! */
	panic("Could not reboot!");
	exit(0);
}

/* ARGSUSED */
static void
bootcmd_halt(char *arg)
{

	printf("Exiting...\n");
	delay(1000);
	halt();
	/* Note: we shouldn't get to this point! */
	panic("Could not halt!");
	exit(0);
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
bootcmd_monitor(char *arg)
{

	db_monitor();
	printf("\n");
}

static void
docommand(const struct bootblk_command * const cmds, char *arg)
{
	char *options;
	int i;

	options = gettrailer(arg);

	for (i = 0; cmds[i].c_name != NULL; i++) {
		if (strcmp(arg, cmds[i].c_name) == 0) {
			(*cmds[i].c_fn)(options);
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
		kgets(input, sizeof(input));

		/*
		 * Skip leading whitespace.
		 */
		while (*c == ' ') {
			c++;
		}
		if (*c != '\0') {
			docommand(bootcmds, c);
		}
	}
}

/*
 * from arch/i386/stand/lib/parseutil.c
 */
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
	while (*options == ' ')
		options++;

	return (options);
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
			return (0);
		}
		tmpopt |= r;
		opts++;
	}

	*howto = tmpopt;
	return (1);
}

static int
parseboot(char *arg, char **filename, int *howto)
{
	char *opts = NULL;

	*filename = 0;
	*howto = 0;

	/* if there were no arguments */
	if (*arg == '\0')
		return (1);

	/* format is... */
	/* [[xxNx:]filename] [-adqsv] */

	/* check for just args */
	if (arg[0] == '-') {
		opts = arg;
	} else {
		/* there's a file name */
		*filename = arg;

		opts = gettrailer(arg);
		if (*opts == '\0') {
			opts = NULL;
		} else if (*opts != '-') {
			printf("invalid arguments\n");
			bootcmd_help(NULL);
			return (0);
		}
	}

	/* at this point, we have dealt with filenames. */

	/* now, deal with options */
	if (opts) {
		if (parseopts(opts, howto) == 0) {
			return (0);
		}
	}
	return (1);
}

/*
 * for common/lib/libc/arch/sh3/gen/udivsi3.S
 */
int raise(int sig);

/*ARGSUSED*/
int
raise(int sig)
{

	return 0;
}
