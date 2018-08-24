/*	$NetBSD: boot.c,v 1.2 2018/08/24 23:22:10 jmcneill Exp $	*/

/*-
 * Copyright (c) 2016 Kimihiro Nonaka <nonaka@netbsd.org>
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "efiboot.h"

#include <sys/bootblock.h>
#include <sys/boot_flag.h>
#include <machine/limits.h>

#include <loadfile.h>

extern const char bootprog_name[], bootprog_rev[], bootprog_kernrev[];

extern char twiddle_toggle;

static const char * const names[][2] = {
	{ "\\netbsd", "\\netbsd.gz" },
	{ "\\onetbsd", "\\onetbsd.gz" },
	{ "\\netbsd.old", "\\netbsd.old.gz" },
};

#define NUMNAMES	__arraycount(names)
#define DEFFILENAME	names[0][0]

#define	DEFTIMEOUT	5

void	command_boot(char *);
void	command_reset(char *);
void	command_version(char *);
void	command_quit(char *);

const struct boot_command commands[] = {
	{ "boot",	command_boot,		"boot [fsN:][filename] [args]\n     (ex. \"fs0:\\netbsd.old -s\"" },
	{ "version",	command_version,	"version" },
	{ "help",	command_help,		"help|?" },
	{ "?",		command_help,		NULL },
	{ "quit",	command_quit,		"quit" },
	{ NULL,		NULL },
};

void
command_help(char *arg)
{
	int n;

	printf("commands are:\n");
	for (n = 0; commands[n].c_name; n++) {
		if (commands[n].c_help)
			printf("%s\n", commands[n].c_help);
	}
}

void
command_boot(char *arg)
{
	char *fname = arg;
	char *bootargs = gettrailer(arg);

	exec_netbsd(*fname ? fname : DEFFILENAME, bootargs);
}

void
command_version(char *arg)
{
	char *ufirmware;
	int rv;

	printf("EFI version: %d.%02d\n",
	    ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xffff);
	ufirmware = NULL;
	rv = ucs2_to_utf8(ST->FirmwareVendor, &ufirmware);
	if (rv == 0) {
		printf("EFI Firmware: %s (rev %d.%02d)\n", ufirmware,
		    ST->FirmwareRevision >> 16,
		    ST->FirmwareRevision & 0xffff);
		FreePool(ufirmware);
	}
}

void
command_quit(char *arg)
{
	efi_exit();
}

void
print_banner(void)
{
	printf("\n\n"
	    ">> %s, Revision %s (from NetBSD %s)\n",
	    bootprog_name, bootprog_rev, bootprog_kernrev);
}

void
boot(void)
{
	int currname, c;

	print_banner();

	printf("Press return to boot now, any other key for boot prompt\n");
	for (currname = 0; currname < NUMNAMES; currname++) {
		printf("booting %s - starting in ", names[currname][0]);

		c = awaitkey(DEFTIMEOUT, 1);
		if ((c != '\r') && (c != '\n') && (c != '\0')) {
			bootprompt(); /* does not return */
		}

		/*
		 * try pairs of names[] entries, foo and foo.gz
		 */
		exec_netbsd(names[currname][0], "");
		exec_netbsd(names[currname][1], "");
	}

	bootprompt();	/* does not return */
}
