/*	$NetBSD: main.c,v 1.15 1999/02/12 05:14:22 cjs Exp $	*/

/*
 * Copyright (c) 1996, 1997
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


#include <sys/reboot.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <libi386.h>

extern void ls __P((char*));
extern int bios2dev __P((int, char**, int*));

int errno;
extern int boot_biosdev;

extern	char bootprog_name[], bootprog_rev[], bootprog_date[],
	bootprog_maker[];

char *names[] = {
    "netbsd", "netbsd.gz",
    "netbsd.old", "netbsd.old.gz",
    "onetbsd", "onetbsd.gz",
#ifdef notyet    
    "netbsd.el", "netbsd.el.gz",
#endif /*notyet*/
};

#define NUMNAMES (sizeof(names)/sizeof(char *))
#define DEFFILENAME names[0]

#define MAXDEVNAME 16

#define TIMEOUT 5

static char *default_devname;
static int default_unit, default_partition;
static char *default_filename;

void	command_help __P((char *));
void	command_ls __P((char *));
void	command_quit __P((char *));
void	command_boot __P((char *));
void	command_dev __P((char *));

struct bootblk_command commands[] = {
	{ "help",	command_help },
	{ "?",		command_help },
	{ "ls",		command_ls },
	{ "quit",	command_quit },
	{ "boot",	command_boot },
	{ "dev",	command_dev },
	{ NULL,		NULL },
};

int
parsebootfile(fname, fsname, devname, unit, partition, file)
	const char *fname;
	char **fsname; /* out */
	char **devname; /* out */
	unsigned int *unit, *partition; /* out */
	const char **file; /* out */
{
	const char *col, *help;

	*fsname = "ufs";
	*devname = default_devname;
	*unit = default_unit;
	*partition = default_partition;
	*file = default_filename;

	if (fname == NULL)
		return(0);

	if((col = strchr(fname, ':'))) {	/* device given */
		static char savedevname[MAXDEVNAME+1];
		int devlen;
		unsigned int u = 0, p = 0;
		int i = 0;

		devlen = col - fname;
		if (devlen > MAXDEVNAME)
			return(EINVAL);

#define isvalidname(c) ((c) >= 'a' && (c) <= 'z')
		if (!isvalidname(fname[i]))
			return(EINVAL);
		do {
			savedevname[i] = fname[i];
			i++;
		} while (isvalidname(fname[i]));
		savedevname[i] = '\0';

#define isnum(c) ((c) >= '0' && (c) <= '9')
		if (i < devlen) {
			if (!isnum(fname[i]))
				return(EUNIT);
			do {
				u *= 10;
				u += fname[i++] - '0';
			} while (isnum(fname[i]));
		}

#define isvalidpart(c) ((c) >= 'a' && (c) <= 'z')
		if (i < devlen) {
			if (!isvalidpart(fname[i]))
				return(EPART);
			p = fname[i++] - 'a';
		}

		if (i != devlen)
			return(ENXIO);

		*devname = savedevname;
		*unit = u;
		*partition = p;
		help = col + 1;
	} else
		help = fname;

	if (*help)
		*file = help;

	return(0);
}

char *sprint_bootsel(filename)
const char *filename;
{
	char *fsname, *devname;
	int unit, partition;
	const char *file;
	static char buf[80];
	
	if (parsebootfile(filename, &fsname, &devname, &unit,
			  &partition, &file) == 0) {
		sprintf(buf, "%s%d%c:%s", devname, unit, 'a' + partition, file);
		return(buf);
	}
	return("(invalid)");
}

void
bootit(filename, howto, tell)
	const char *filename;
	int howto, tell;
{

	if (tell) {
		printf("booting %s", sprint_bootsel(filename));
		if (howto)
			printf(" (howto 0x%x)", howto);
		printf("\n");
	}

	if (exec_netbsd(filename, 0, howto) < 0)
		printf("boot: %s: %s\n", sprint_bootsel(filename),
		       strerror(errno));
	else
		printf("boot returned\n");
}

void
print_banner(void)
{

	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
	printf(">> Memory: %d/%d k\n", getbasemem(), getextmem());
	printf(
#ifdef COMPAT_OLDBOOT
	       "Use hd1a:netbsd to boot sd0 when wd0 is also installed\n"
#endif
	       "Press return to boot now, any other key for boot menu\n");
}


/* 
 * note: normally, void main() wouldn't be legal, but this isn't a
 * hosted environment...
 */
int
main()
{
	int currname;
	char c;

#ifdef	SUPPORT_SERIAL
	initio(SUPPORT_SERIAL);
#else
	initio(CONSDEV_PC);
#endif
	gateA20();

	print_banner();

	/* try to set default device to what BIOS tells us */
	bios2dev(boot_biosdev, &default_devname, &default_unit);
	default_partition = 0;

	/* if the user types "boot" without filename */
	default_filename = DEFFILENAME;

	currname = 0;
	for (;;) {
		printf("booting %s - starting in ",
		       sprint_bootsel(names[currname]));

		c = awaitkey(TIMEOUT, 1);
		if ((c != '\r') && (c != '\n') && (c != '\0')) {
			printf("type \"?\" or \"help\" for help.\n");
			bootmenu(); /* does not return */
		}

		/*
		 * try pairs of names[] entries, foo and foo.gz
		 */
		/* don't print "booting..." again */
		bootit(names[currname], 0, 0);
		/* since it failed, try switching bootfile. */
		currname = ++currname % NUMNAMES;

		/* now try the second of a pair, presumably the .gz
		   version. */
		/* XXX duped code sucks. */
		bootit(names[currname], 0, 1);
		/* since it failed, try switching bootfile. */
		currname = ++currname % NUMNAMES;
	}

	return (0);
}

/* ARGSUSED */
void
command_help(arg)
	char *arg;
{

	printf("commands are:\n"
	    "boot [xdNx:][filename] [-adrs]\n"
	    "     (ex. \"sd0a:netbsd.old -s\"\n"
	    "ls [path]\n"
	    "dev xd[N[x]]:\n"
	    "help|?\n"
	    "quit\n");
}

void
command_ls(arg)
	char *arg;
{
	char *save = default_filename;

	default_filename = "/";
	ls(arg);
	default_filename = save;
}

/* ARGSUSED */
void
command_quit(arg)
	char *arg;
{

	printf("Rebooting... goodbye...\n");
	delay(1000000);
	reboot();
	/* Note: we shouldn't get to this point! */
	panic("Could not reboot!");
	exit();
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
command_dev(arg)
	char *arg;
{
	static char savedevname[MAXDEVNAME + 1];
	char *fsname, *devname;
	const char *file; /* dummy */

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
