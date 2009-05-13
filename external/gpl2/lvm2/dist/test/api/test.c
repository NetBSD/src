/*	$NetBSD: test.c,v 1.1.1.1.2.2 2009/05/13 18:52:47 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include "lvm2.h"

#define MAX_ARGS 64

static int lvm_split(char *str, int *argc, char **argv, int max)
{
	char *b = str, *e;
	*argc = 0;

	while (*b) {
		while (*b && isspace(*b))
			b++;

		if ((!*b) || (*b == '#'))
			break;

		e = b;
		while (*e && !isspace(*e))
			e++;

		argv[(*argc)++] = b;
		if (!*e)
			break;
		*e++ = '\0';
		b = e;
		if (*argc == max)
			break;
	}

	return *argc;
}

static int lvmapi_test_shell(void *h)
{
	int argc, i;
	char *input = NULL, *args[MAX_ARGS], **argv;

	while (1) {
		free(input);
		input = readline("lvm> ");

		/* EOF */
		if (!input) {
			printf("\n");
			break;
		}

		/* empty line */
		if (!*input)
			continue;

		argv = args;

		if (lvm_split(input, &argc, argv, MAX_ARGS) == MAX_ARGS) {
			printf("Too many arguments, sorry.");
			continue;
		}

		if (!strcmp(argv[0], "lvm")) {
			argv++;
			argc--;
		}

		if (!argc)
			continue;

		if (!strcmp(argv[0], "quit") || !strcmp(argv[0], "exit")) {
			printf("Exiting.\n");
			break;
		} else if (!strcmp(argv[0], "?") || !strcmp(argv[0], "help")) {
			printf("No commands defined\n");
		} else if (!strcmp(argv[0], "scan")) {
			for (i=1; i < argc; i++)
				printf("Scan a path!\n");
		}
	}

	free(input);
	return 0;
}
		      
int main (int argc, char *argv[])
{
	void *h;

	h = lvm2_create();
	if (!h) {
		printf("Unable to open lvm library instance\n");
		return 1;
	}

	lvmapi_test_shell(h);

	if (h)
		lvm2_destroy(h);
	return 0;
}

