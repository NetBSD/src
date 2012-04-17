/*	$NetBSD: bootmenu.c,v 1.1.18.1 2012/04/17 00:07:13 yamt Exp $	*/

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

struct bootconf_def bootconf;

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
 * The lines are in the format keyword=value. There should not be spaces
 * around the = sign.
 *
 * The recognised keywords are:
 * banner: text displayed instead of the normal welcome text
 * menu: Descriptive text:command to use
 * timeout: Timeout in seconds (overrides that set by installboot)
 * default: the default menu option to use if Return is pressed
 * consdev: the console device to use
 * format: how menu choices are displayed: (a)utomatic, (n)umbers or (l)etters
 * clear: whether to clear the screen or not
 *
 * Example boot.cfg file:
 * banner=Welcome to NetBSD
 * banner=Please choose the boot type from the following menu
 * menu=Boot NetBSD:boot netbsd
 * menu=Boot into single user mode:boot netbsd -s
 * menu=:boot hd1a:netbsd -cs
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
	char *key, *value, *v2;

	/* Clear bootconf structure */
	memset(&bootconf, 0, sizeof(bootconf));

	/* Set timeout to configured */
	bootconf.timeout = default_timeout;

	/* automatically switch between letter and numbers on menu */
	bootconf.menuformat = MENUFORMAT_AUTO;

	fd = open(conf, 0);
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
		close(fd);
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
	for (c = bc; *c; c++) {
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
			/*
			 * Parse "menu=<description>:<command>".  If the
			 * description is empty ("menu=:<command>)",
			 * then re-use the command as the description.
			 * Note that the command may contain embedded
			 * colons.
			 */
			if (cmenu >= MAXMENU)
				continue;
			bootconf.desc[cmenu] = value;
			for (v2 = value; *v2 && *v2 != ':'; v2++)
				continue;
			if (*v2) {
				*v2++ = 0;
				bootconf.command[cmenu] = v2;
				if (! *value)
					bootconf.desc[cmenu] = v2;
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
		} else if (!strncmp(key, "format", 6)) {
			printf("value:%c\n", *value);
			switch (*value) {
			case 'a':
			case 'A':
				bootconf.menuformat = MENUFORMAT_AUTO;
				break;

			case 'n':
			case 'N':
			case 'd':
			case 'D':
				bootconf.menuformat = MENUFORMAT_NUMBER;
				break;

			case 'l':
			case 'L':
				bootconf.menuformat = MENUFORMAT_LETTER;
				break;
			}
		} else if (!strncmp(key, "clear", 5)) {
			bootconf.clear = !!atoi(value);
		}
	}
	switch (bootconf.menuformat) {
	case MENUFORMAT_AUTO:
		if (cmenu > 9 && bootconf.timeout > 0)
			bootconf.menuformat = MENUFORMAT_LETTER;
		else
			bootconf.menuformat = MENUFORMAT_NUMBER;
		break;

	case MENUFORMAT_NUMBER:
		if (cmenu > 9 && bootconf.timeout > 0)
			cmenu = 9;
		break;
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
static int
getchoicefrominput(char *input, int def)
{
	int choice = -1;

	if (*input == '\0' || *input == '\r' || *input == '\n')
		choice = def;
	else if (*input >= 'A' && *input < bootconf.nummenu + 'A')
		choice = (*input) - 'A';
	else if (*input >= 'a' && *input < bootconf.nummenu + 'a')
		choice = (*input) - 'a';
	else if (isnum(*input)) {
		choice = atoi(input) - 1;
		if (choice < 0 || choice >= bootconf.nummenu)
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
	if (bootconf.menuformat == MENUFORMAT_LETTER) {
		for (choice = 0; choice < bootconf.nummenu; choice++)
			printf("    %c. %s\n", choice + 'A',
			    bootconf.desc[choice]);
	} else {
		/* Can't use %2d format string with libsa */
		for (choice = 0; choice < bootconf.nummenu; choice++)
			printf("    %s%d. %s\n",
			    (choice < 9) ?  " " : "",
			    choice + 1,
			    bootconf.desc[choice]);
	}
	choice = -1;
	for (;;) {
		input[0] = '\0';

		if (bootconf.timeout < 0) {
			if (bootconf.menuformat == MENUFORMAT_LETTER)
				printf("\nOption: [%c]:",
				    bootconf.def + 'A');
			else
				printf("\nOption: [%d]:",
				    bootconf.def + 1);

			gets(input);
			choice = getchoicefrominput(input, bootconf.def);
		} else if (bootconf.timeout == 0)
			choice = bootconf.def;
		else  {
			printf("\nChoose an option; RETURN for default; "
			       "SPACE to stop countdown.\n");
			if (bootconf.menuformat == MENUFORMAT_LETTER)
				printf("Option %c will be chosen in ",
				    bootconf.def + 'A');
			else
				printf("Option %d will be chosen in ",
				    bootconf.def + 1);
			input[0] = awaitkey(bootconf.timeout, 1);
			input[1] = '\0';
			choice = getchoicefrominput(input, bootconf.def);
			/* If invalid key pressed, drop to menu */
			if (choice == -1)
				bootconf.timeout = -1;
		}
		if (choice < 0)
			continue;
		if (!strcmp(bootconf.command[choice], "prompt")) {
			printf("type \"?\" or \"help\" for help.\n");
			bootmenu(); /* does not return */
		} else {
			ic = bootconf.command[choice];
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
