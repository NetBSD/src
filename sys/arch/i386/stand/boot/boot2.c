/*	$NetBSD: boot2.c,v 1.20 2008/01/02 10:39:39 sborrill Exp $	*/

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

#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <lib/libkern/libkern.h>

#include <libi386.h>
#include "devopen.h"

#ifdef SUPPORT_USTARFS
#include "ustarfs.h"
#endif
#ifdef SUPPORT_PS2
#include <biosmca.h>
#endif

extern struct x86_boot_params boot_params;

extern	const char bootprog_name[], bootprog_rev[], bootprog_date[],
	bootprog_maker[];

int errno;

int boot_biosdev;
u_int boot_biossector;

static const char * const names[][2] = {
	{ "netbsd", "netbsd.gz" },
	{ "onetbsd", "onetbsd.gz" },
	{ "netbsd.old", "netbsd.old.gz" },
};

#define NUMNAMES (sizeof(names)/sizeof(names[0]))
#define DEFFILENAME names[0][0]

#define MAXDEVNAME 16

#ifndef SMALL
#define BOOTCONF "boot.cfg"
#define MAXMENU 10
#define MAXBANNER 10
#endif /* !SMALL */

static char *default_devname;
static int default_unit, default_partition;
static const char *default_filename;

char *sprint_bootsel(const char *);
void bootit(const char *, int, int);
void print_banner(void);
void boot2(int, u_int);

#ifndef SMALL
void parsebootconf(const char *);
void doboottypemenu(void);
int atoi(const char *);
#endif /* !SMALL */

void	command_help(char *);
void	command_ls(char *);
void	command_quit(char *);
void	command_boot(char *);
void	command_dev(char *);
void	command_consdev(char *);

const struct bootblk_command commands[] = {
	{ "help",	command_help },
	{ "?",		command_help },
	{ "ls",		command_ls },
	{ "quit",	command_quit },
	{ "boot",	command_boot },
	{ "dev",	command_dev },
	{ "consdev",	command_consdev },
	{ NULL,		NULL },
};

#ifndef SMALL
struct bootconf_def {
	char *banner[MAXBANNER];	/* Banner text */
	char *command[MAXMENU];		/* Menu commands per entry*/
	char *consdev;			/* Console device */
	int def;			/* Default menu option */
	char *desc[MAXMENU];		/* Menu text per entry */
	int nummenu;			/* Number of menu items */
	int timeout;		 	/* Timeout in seconds */
} bootconf;
#endif /* !SMALL */

int
parsebootfile(const char *fname, char **fsname, char **devname,
	      int *unit, int *partition, const char **file)
{
	const char *col;

	*fsname = "ufs";
	*devname = default_devname;
	*unit = default_unit;
	*partition = default_partition;
	*file = default_filename;

	if (fname == NULL)
		return 0;

	if ((col = strchr(fname, ':')) != NULL) {	/* device given */
		static char savedevname[MAXDEVNAME+1];
		int devlen;
		int u = 0, p = 0;
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
				return EUNIT;
			do {
				u *= 10;
				u += fname[i++] - '0';
			} while (isnum(fname[i]));
		}

#define isvalidpart(c) ((c) >= 'a' && (c) <= 'z')
		if (i < devlen) {
			if (!isvalidpart(fname[i]))
				return EPART;
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
	char *fsname, *devname;
	int unit, partition;
	const char *file;
	static char buf[80];

	if (parsebootfile(filename, &fsname, &devname, &unit,
			  &partition, &file) == 0) {
		sprintf(buf, "%s%d%c:%s", devname, unit, 'a' + partition, file);
		return buf;
	}
	return "(invalid)";
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

	if (exec_netbsd(filename, 0, howto) < 0)
		printf("boot: %s: %s\n", sprint_bootsel(filename),
		       strerror(errno));
	else
		printf("boot returned\n");
}

void
print_banner(void)
{
#ifndef SMALL
	int n;
	if (bootconf.banner[0]) {
		for (n = 0; bootconf.banner[n]; n++) 
			printf("%s\n", bootconf.banner[n]);
	} else {
#endif /* !SMALL */
		printf("\n");
		printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
		printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
		printf(">> Memory: %d/%d k\n", getbasemem(), getextmem());

#ifndef SMALL
	}
#endif /* !SMALL */
}

#ifndef SMALL
int
atoi(const char *in)
{
	char *c;
	int ret;

	ret = 0;
	c = (char *)in;
	if (*c == '-')
		c++;
	for (; isnum(*c); c++)
		ret = (ret * 10) + (*c - '0');

	return (*in == '-') ? -ret : ret;
}

/*
 * This function parses a boot.cfg file in the root of the filesystem
 * (if present) and populates the global boot configuration.
 * 
 * The file consists of a number of lines each terminated by \n
 * The lines are in the format keyword=value. There should be spaces
 * around the = sign.
 *
 * The recognised keywords are:
 * banner: text displayed instead of the normal welcome text
 * menu: Descriptive text:command to use
 * timeout: Timeout in seconds (overrides that set by installboot)
 * default: the default menu option to use if Return is pressed
 * consdev: the console device to use
 *
 * Example boot.cfg file:
 * banner=Welcome to NetBSD
 * banner=Please choose the boot type from the following menu
 * menu=Boot NetBSD:boot netbsd
 * menu=Boot into single user mode:boot netbsd -s
 * menu=Goto boot comand line:prompt
 * timeout=10
 * consdev=com0
 * default=1
*/
void
parsebootconf(const char *conf)
{
	char *bc, *c;
	int cmenu, cbanner, len;
	int fd, err, off;
	struct stat st;
	char *value, *key;
#ifdef SUPPORT_USTARFS
	void *op_open;
#endif

	/* Clear bootconf structure */
	bzero((void *)&bootconf, sizeof(bootconf));
	
	/* Set timeout to configured */
	bootconf.timeout = boot_params.bp_timeout;

	/* don't try to open BOOTCONF if the target fs is ustarfs */
#ifdef SUPPORT_USTARFS
#if !defined(LIBSA_SINGLE_FILESYSTEM)
	fd = open("boot", 0);	/* assume we are loaded as "boot" from here */
	if (fd < 0)
		op_open = NULL;	/* XXX */
	else {
		op_open = files[fd].f_ops->open;
		close(fd);
	}
#else
	op_open = file_system[0].open;
#endif	/* !LIBSA_SINGLE_FILESYSTEM */
	if (op_open == ustarfs_open)
		return;
#endif	/* SUPPORT_USTARFS */

	fd = open(BOOTCONF, 0);
	if (fd < 0)
		return;
	
	err = fstat(fd, &st);
	if (err == -1) {
		close(fd);
		return;
	}

	bc = alloc(st.st_size + 1);
	if (bc == NULL) {
		printf("Could not allocate memory for boot configuration\n");
		return;
	}
	
	off = 0;
	do {
		len = read(fd, bc + off, 1024);
		if (len <= 0)
			break;
		off += len;
	} while (len > 0);
	bc[off] = '\0';
	
	close(fd);
	/* bc now contains the whole boot.cfg file */
	
	cmenu = 0;
	cbanner = 0;
	for(c = bc; *c; c++) {
		key = c;
		/* Look for = separator between key and value */
		for (; *c && *c != '='; c++)
			continue;
		if (*c == '\0')
			break; /* break if at end of data */
		
		/* zero terminate key which points to keyword */
		*c++ = 0;
		value = c;
		/* Look for end of line (or file) and zero terminate value */
		for (; *c && *c != '\n'; c++)
			continue;
		*c = 0;
		
		if (!strncmp(key, "menu", 4)) {
			if (cmenu >= MAXMENU)
				continue;
			bootconf.desc[cmenu] = value;
			/* Look for : between description and command */
			for (; *value && *value != ':'; value++)
				continue;
			if(*value) {
				*value++ = 0;
				bootconf.command[cmenu] = value;
				cmenu++;
			} else {
				/* No delimiter means invalid line */
				bootconf.desc[cmenu] = NULL;
			}
		} else if (!strncmp(key, "banner", 6)) {
			if (cbanner < MAXBANNER)
				bootconf.banner[cbanner++] = value;
		} else if (!strncmp(key, "timeout", 7)) {
			if (!isnum(*value))
				bootconf.timeout = -1;
			else
				bootconf.timeout = atoi(value);
		} else if (!strncmp(key, "default", 7)) {
			bootconf.def = atoi(value) - 1;
		} else if (!strncmp(key, "consdev", 7)) {
			bootconf.consdev = value;
		}
	}
	bootconf.nummenu = cmenu;
	if (bootconf.def < 0)
		bootconf.def = 0;
	if (bootconf.def >= cmenu)
		bootconf.def = cmenu - 1;
}

/*
 * doboottypemenu will render the menu and parse any user input
 */

void
doboottypemenu(void)
{
	int choice;
	char input[80], c;
		
	printf("\n");
	/* Display menu */
	for (choice = 0; bootconf.desc[choice]; choice++)
		printf("    %d. %s\n", choice+1, bootconf.desc[choice]);

	choice = -1;
	for(;;) {
		input[0] = '\0';
		
		if (bootconf.timeout < 0) {
			printf("\nOption: [%d]:", bootconf.def + 1);
			gets(input);
			if (input[0] == '\0') choice = bootconf.def;
			if (input[0] >= '1' && 
				input[0] <= bootconf.nummenu + '0')
				    choice = input[0] - '1';
		} else if (bootconf.timeout == 0)
			choice = bootconf.def;
		else  {
			printf("\nPress the key for your chosen option or ");
			printf("Return to choose the default (%d)\n",
			      bootconf.def + 1);
			printf("Option %d will be chosen in ",
			      bootconf.def + 1);
			c = awaitkey(bootconf.timeout, 1);
			if (c >= '1' && c <= bootconf.nummenu + '0')
				choice = c - '1';
			else if (c ==  '\r' || c == '\n' || c == '\0')
				/* default if timed out or Return pressed */
				choice = bootconf.def;
			else {
				/* If any other key pressed, drop to menu */
				bootconf.timeout = -1;
				choice = -1;
			}
		}
		if (choice < 0)
			continue;
		if (!strcmp(bootconf.command[choice], "prompt") && 
		    ((boot_params.bp_flags & X86_BP_FLAGS_PASSWORD) == 0 ||
		    check_password(boot_params.bp_password))) {
			printf("type \"?\" or \"help\" for help.\n");
			bootmenu(); /* does not return */
		} else
			docommand(bootconf.command[choice]);
			
	}
}
#endif /* !SMALL */

/*
 * Called from the initial entry point boot_start in biosboot.S
 *
 * biosdev: BIOS drive number the system booted from
 * biossector: Sector number of the NetBSD partition
 */
void
boot2(int biosdev, u_int biossector)
{
	int currname;
	char c;

	initio(boot_params.bp_consdev);

#ifdef SUPPORT_PS2
	biosmca();
#endif
	gateA20();

	if (boot_params.bp_flags & X86_BP_FLAGS_RESET_VIDEO)
		biosvideomode();

	/* need to remember these */
	boot_biosdev = biosdev;
	boot_biossector = biossector;

	/* try to set default device to what BIOS tells us */
	bios2dev(biosdev, biossector, &default_devname, &default_unit,
		 &default_partition);

	/* if the user types "boot" without filename */
	default_filename = DEFFILENAME;

#ifndef SMALL
	parsebootconf(BOOTCONF);

	/*
	 * If console set in boot.cfg, switch to it.
	 * This will print the banner, so we don't need to explicitly do it
	 */
	if (bootconf.consdev)
		command_consdev(bootconf.consdev);
	else 
		print_banner();

	/* Display the menu, if applicable */
	if (bootconf.nummenu > 0) {
		/* Does not return */
		doboottypemenu();
	}
#else
		print_banner();
#endif

	printf("Press return to boot now, any other key for boot menu\n");
	for (currname = 0; currname < NUMNAMES; currname++) {
		printf("booting %s - starting in ",
		       sprint_bootsel(names[currname][0]));

#ifdef SMALL
		c = awaitkey(boot_params.bp_timeout, 1);
#else
		c = awaitkey((bootconf.timeout < 0) ? 0 : bootconf.timeout, 1);
#endif
		if ((c != '\r') && (c != '\n') && (c != '\0') &&
		    ((boot_params.bp_flags & X86_BP_FLAGS_PASSWORD) == 0
		     || check_password(boot_params.bp_password))) {
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

	bootmenu();	/* does not return */
}

/* ARGSUSED */
void
command_help(char *arg)
{

	printf("commands are:\n"
	       "boot [xdNx:][filename] [-acdqsvxz]\n"
	       "     (ex. \"hd0a:netbsd.old -s\"\n"
	       "ls [path]\n"
	       "dev xd[N[x]]:\n"
	       "consdev {pc|com[0123]|com[0123]kbd|auto}\n"
	       "help|?\n"
	       "quit\n");
}

void
command_ls(char *arg)
{
	const char *save = default_filename;

	default_filename = "/";
	ufs_ls(arg);
	default_filename = save;
}

/* ARGSUSED */
void
command_quit(char *arg)
{

	printf("Exiting...\n");
	delay(1000000);
	reboot();
	/* Note: we shouldn't get to this point! */
	panic("Could not reboot!");
	exit(0);
}

void
command_boot(char *arg)
{
	char *filename;
	int howto;

	if (parseboot(arg, &filename, &howto))
		bootit(filename, howto, 1);
}

void
command_dev(char *arg)
{
	static char savedevname[MAXDEVNAME + 1];
	char *fsname, *devname;
	const char *file; /* dummy */

	if (*arg == '\0') {
		printf("%s%d%c:\n", default_devname, default_unit,
		       'a' + default_partition);
		return;
	}

	if (strchr(arg, ':') == NULL ||
	    parsebootfile(arg, &fsname, &devname, &default_unit,
			  &default_partition, &file)) {
		command_help(NULL);
		return;
	}

	/* put to own static storage */
	strncpy(savedevname, devname, MAXDEVNAME + 1);
	default_devname = savedevname;
}

static const struct cons_devs {
	const char	*name;
	u_int		tag;
} cons_devs[] = {
	{ "pc",		CONSDEV_PC },
	{ "com0",	CONSDEV_COM0 },
	{ "com1",	CONSDEV_COM1 },
	{ "com2",	CONSDEV_COM2 },
	{ "com3",	CONSDEV_COM3 },
	{ "com0kbd",	CONSDEV_COM0KBD },
	{ "com1kbd",	CONSDEV_COM1KBD },
	{ "com2kbd",	CONSDEV_COM2KBD },
	{ "com3kbd",	CONSDEV_COM3KBD },
	{ "auto",	CONSDEV_AUTO },
	{ NULL,		0 }
};

void
command_consdev(char *arg)
{
	const struct cons_devs *cdp;

	for (cdp = cons_devs; cdp->name; cdp++) {
		if (strcmp(arg, cdp->name) == 0) {
			initio(cdp->tag);
			print_banner();
			return;
		}
	}
	printf("invalid console device.\n");
}
