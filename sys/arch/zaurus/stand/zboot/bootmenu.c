/*	$NetBSD: bootmenu.c,v 1.2.20.1 2014/08/10 06:54:12 tls Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/reboot.h>
#include <sys/bootblock.h>

#include "boot.h"
#include "unixdev.h"
#include "bootmenu.h"
#include "pathnames.h"

#define isnum(c) ((c) >= '0' && (c) <= '9')

#define MENUFORMAT_AUTO	  0
#define MENUFORMAT_NUMBER 1
#define MENUFORMAT_LETTER 2

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

void
parsebootconf(const char *conf)
{
	perform_bootcfg(conf, &bootcfg_do_noop, 0);
}

/*
 * doboottypemenu will render the menu and parse any user input
 */
static int
getchoicefrominput(char *input, int def)
{
	int choice = -1;

	if (*input == '\0' || *input == '\r' || *input == '\n')
		choice = def;
	else if (*input >= 'A' && *input < bootcfg_info.nummenu + 'A')
		choice = (*input) - 'A';
	else if (*input >= 'a' && *input < bootcfg_info.nummenu + 'a')
		choice = (*input) - 'a';
	else if (isnum(*input)) {
		choice = atoi(input) - 1;
		if (choice < 0 || choice >= bootcfg_info.nummenu)
			choice = -1;
	}
	return choice;
}

void
doboottypemenu(void)
{
	char input[80], *ic, *oc;
	int choice;

	printf("\n");
	/* Display menu */
	if (bootcfg_info.menuformat == MENUFORMAT_LETTER) {
		for (choice = 0; choice < bootcfg_info.nummenu; choice++)
			printf("    %c. %s\n", choice + 'A',
			    bootcfg_info.desc[choice]);
	} else {
		/* Can't use %2d format string with libsa */
		for (choice = 0; choice < bootcfg_info.nummenu; choice++)
			printf("    %s%d. %s\n",
			    (choice < 9) ?  " " : "",
			    choice + 1,
			    bootcfg_info.desc[choice]);
	}
	choice = -1;
	for (;;) {
		input[0] = '\0';

		if (bootcfg_info.timeout < 0) {
			if (bootcfg_info.menuformat == MENUFORMAT_LETTER)
				printf("\nOption: [%c]:",
				    bootcfg_info.def + 'A');
			else
				printf("\nOption: [%d]:",
				    bootcfg_info.def + 1);

			gets(input);
			choice = getchoicefrominput(input, bootcfg_info.def);
		} else if (bootcfg_info.timeout == 0)
			choice = bootcfg_info.def;
		else  {
			printf("\nChoose an option; RETURN for default; "
			       "SPACE to stop countdown.\n");
			if (bootcfg_info.menuformat == MENUFORMAT_LETTER)
				printf("Option %c will be chosen in ",
				    bootcfg_info.def + 'A');
			else
				printf("Option %d will be chosen in ",
				    bootcfg_info.def + 1);
			input[0] = awaitkey(bootcfg_info.timeout, 1);
			input[1] = '\0';
			choice = getchoicefrominput(input, bootcfg_info.def);
			/* If invalid key pressed, drop to menu */
			if (choice == -1)
				bootcfg_info.timeout = -1;
		}
		if (choice < 0)
			continue;
		if (!strcmp(bootcfg_info.command[choice], "prompt")) {
			printf("type \"?\" or \"help\" for help.\n");
			bootmenu(); /* does not return */
		} else {
			ic = bootcfg_info.command[choice];
			/* Split command string at ; into separate commands */
			do {
				oc = input;
				/* Look for ; separator */
				for (; *ic && *ic != COMMAND_SEPARATOR; ic++)
					*oc++ = *ic;
				if (*input == '\0')
					continue;
				/* Strip out any trailing spaces */
				oc--;
				for (; *oc == ' ' && oc > input; oc--);
				*++oc = '\0';
				if (*ic == COMMAND_SEPARATOR)
					ic++;
				/* Stop silly command strings like ;;; */
				if (*input != '\0')
					docommand(input);
				/* Skip leading spaces */
				for (; *ic == ' '; ic++);
			} while (*ic);
		}
	}
}
