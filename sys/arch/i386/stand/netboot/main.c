/*	$NetBSD: main.c,v 1.13.2.2 2009/11/01 13:58:44 jym Exp $	 */

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

int errno;

extern char	bootprog_name[], bootprog_rev[], bootprog_kernrev[];

#define TIMEOUT 5

void	command_help(char *);
void	command_quit(char *);
void	command_boot(char *);

const struct bootblk_command commands[] = {
	{ "help",	command_help },
	{ "?",		command_help },
	{ "quit",	command_quit },
	{ "boot",	command_boot },
	{ NULL,		NULL },
};

int 
bootit(const char *filename, int howto)
{
	if (exec_netbsd(filename, 0, howto, 0, clear_pc_screen) < 0)
		printf("boot: %s\n", strerror(errno));
	else
		printf("boot returned\n");
	return (-1);
}

static void
print_banner(void)
{
	clear_pc_screen();

	printf("\n"
	       ">> %s, Revision %s (from NetBSD %s)\n"
	       ">> Memory: %d/%d k\n"
	       "Press return to boot now, any other key for boot menu\n"
	       "starting in ",
	       bootprog_name, bootprog_rev, bootprog_kernrev,
	       getbasemem(), getextmem());
}

int
main(void)
{
        char c;

	initio(CONSDEV_AUTO);
	gateA20();

	print_banner();

	c = awaitkey(TIMEOUT, 1);
	if ((c != '\r') && (c != '\n') && (c != '\0')) {
		printf("type \"?\" or \"help\" for help.\n");
		bootmenu();	/* does not return */
	}

	bootit("netbsd", 0);

	/* if that fails, let BIOS look for boot device */
	return (1);
}

/* ARGSUSED */
void
command_help(char *arg)
{
	printf("commands are:\n"
	       "boot [filename] [-acdqsv]\n"
	       "     (ex. \"netbsd.old -s\"\n"
	       "help|?\n"
	       "quit\n");
}

/* ARGSUSED */
void
command_quit(char *arg)
{
	printf("Exiting... goodbye...\n");
	exit(0);
}

void
command_boot(char *arg)
{
	char *filename;
	int howto;

	if (parseboot(arg, &filename, &howto))
		bootit(filename, howto);
}
