/*	$NetBSD: main.c,v 1.2 1997/03/22 09:18:12 thorpej Exp $	 */

/*
 * Copyright (c) 1996
 * 	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996
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
 *
 */


#include <lib/libkern/libkern.h>

#include <lib/libsa/stand.h>

#include <libi386.h>

extern char    *strerror __P((int));	/* XXX missing in stand.h */

int             errno;
static char    *consdev;

extern char     version[];
extern char     etherdev[];

#ifdef SUPPORT_NFS		/* XXX */
int             debug = 0;
#endif

#define TIMEOUT 5
#define POLL_FREQ 10

#define MAXBOOTFILE 20

#ifdef COMPAT_OLDBOOT
int
parsebootfile(fname, fsname, devname, unit, partition, file)
	const char     *fname;
	char          **fsname;	/* out */
	char          **devname;/* out */
	unsigned int   *unit, *partition;	/* out */
	const char    **file;	/* out */
{
	return (EINVAL);
}

int 
biosdisk_gettype(f)
	struct open_file *f;
{
	return (0);
}
#endif

int 
bootit(filename, howto)
	const char     *filename;
	int             howto;
{
	if (exec_netbsd(filename, 0, howto, etherdev, "pc") < 0)
		printf("boot: %s\n", strerror(errno));
	else
		printf("boot returned\n");
	return (-1);
}

static void 
helpme()
{
	printf("commands are:\n"
	       "boot [filename] [-adrs]\n"
	       "     (ex. \"netbsd.old -s\"\n"
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
	if (strcmp("quit", arg) == 0) {
		printf("Exiting... goodbye...\n");
		exit(0);
	}
	if (strcmp("boot", arg) == 0) {
		char           *filename;
		int             howto;
		if (parseboot(options, &filename, &howto))
			bootit(filename, howto);
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

static int
awaitkey(void)
{
	int             i;

	i = TIMEOUT * POLL_FREQ;

	while (i--) {
		if (iskey()) {
			/* flush input buffer */
			while (iskey())
				getchar();

			return (1);
		}
		delay(1000000 / POLL_FREQ);
		if (!(i % POLL_FREQ))
			printf("%d\b", i / POLL_FREQ);
	}
	printf("0\n");
	return (0);
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

int
main()
{
	consdev = initio(CONSDEV_AUTO);
	gateA20();

	print_banner();

	printf("press any key for boot menu\n"
	       "starting in %d\b", TIMEOUT);

	if (awaitkey())
		bootmenu();	/* does not return */

	bootit("netbsd", 0);

	/* if that fails, let BIOS look for boot device */
	return (1);
}
