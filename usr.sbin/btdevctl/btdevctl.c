/*	$NetBSD: btdevctl.c,v 1.1 2006/08/13 09:03:23 plunky Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2006 Itronix, Inc.\n"
	    "All rights reserved.\n");
__RCSID("$NetBSD: btdevctl.c,v 1.1 2006/08/13 09:03:23 plunky Exp $");

#include <prop/proplib.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "btdevctl.h"

const char *config_file = "/var/db/btdev.xml";
const char *control_file = NULL;

struct command {
	const char	*command;
	int		(*handler)(int, char **);
	const char	*description;
} commands[] = {
	{ "Attach",	dev_attach,	"attach device"		},
	{ "Detach",	dev_detach,	"detach device"		},
	{ "Parse",	hid_parse,	"parse HID descriptor"	},
	{ "Print",	cfg_print,	"print config entry"	},
	{ "Query",	cfg_query,	"make config entry"	},
	{ "Remove",	cfg_remove,	"remove config entry"	},
	{ NULL }
};

static void
usage(void)
{
	struct command *cmd;

	fprintf(stderr, "Usage: %s <btdev> <command> [parameters]\n"
			"Commands:\n",
			getprogname());

	for (cmd = commands ; cmd->command != NULL ; cmd++)
		fprintf(stderr, "\t%-13s%s\n", cmd->command, cmd->description);

	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct command *cmd;

	if (argc < 3)
		usage();

	control_file = argv[1];

	for (cmd = commands ; cmd->command != NULL ; cmd++) {
		if (strcasecmp(argv[2], cmd->command) == 0)
			return (cmd->handler)(argc - 2, argv + 2);
	}

	errx(EXIT_FAILURE, "%s: unknown command", argv[2]);
}

prop_dictionary_t
read_config(void)
{
	prop_dictionary_t dict;
	char *xml;
	off_t len;
	int fd;

	fd = open(config_file, O_RDONLY, 0);
	if (fd < 0)
		return NULL;

	len = lseek(fd, 0, SEEK_END);
	if (len == 0) {
		close(fd);
		return NULL;
	}

	xml = malloc(len);
	if (xml == NULL) {
		close(fd);
		return NULL;
	}

	(void)lseek(fd, 0, SEEK_SET);
	if (read(fd, xml, len) != len) {
		close(fd);
		free(xml);
		return NULL;
	}

	dict = prop_dictionary_internalize(xml);

	free(xml);
	close(fd);

	return dict;
}

int
write_config(prop_dictionary_t dict)
{
	char *xml;
	size_t len;
	int fd;

	fd = open(config_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
	if (fd < 0)
		return 0;

	xml = prop_dictionary_externalize(dict);
	if (xml == NULL) {
		close(fd);
		return 0;
	}

	len = strlen(xml);
	if (write(fd, xml, len) != len) {
		free(xml);
		close(fd);
		return 0;
	}

	free(xml);
	close(fd);

	return 1;
}
