/*	$NetBSD: dev.c,v 1.2 2006/07/26 10:31:00 tron Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
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
__RCSID("$NetBSD: dev.c,v 1.2 2006/07/26 10:31:00 tron Exp $");

#include <sys/ioctl.h>
#include <sys/param.h>
#include <prop/proplib.h>
#include <dev/bluetooth/btdev.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "btcontrol.h"

int
dev_attach(int ac, char **av)
{
	prop_dictionary_t	 cfg, dev;
	char			 path[MAXPATHLEN];
	int			 fd, ch, quiet;

	quiet = 0;

	while ((ch = getopt(ac, av, "q")) != -1) {
		switch(ch) {
		case 'q':
			quiet = 1;
			break;

		default:
			errx(EXIT_FAILURE, "unkown option -%c", ch);
		}
	}

	cfg = read_config();
	if (cfg == NULL)
		err(EXIT_FAILURE, "%s", config_file);

	dev = prop_dictionary_get(cfg, control_file);
	if (dev == NULL || prop_object_type(dev) != PROP_TYPE_DICTIONARY) {
		if (quiet)
			exit(EXIT_FAILURE);

		errx(EXIT_FAILURE, "%s: no config entry", control_file);
	}

	snprintf(path, sizeof(path), "/dev/%s", control_file);
	fd = open(path, O_WRONLY, 0);
	if (fd < 0)
		err(EXIT_FAILURE, "%s", path);

	if (prop_dictionary_send_ioctl(dev, fd, BTDEV_ATTACH)) {
		if (quiet && errno == ENXIO)
			exit(EXIT_FAILURE);

		errx(EXIT_FAILURE, "%s", path);
	}

	close(fd);
	prop_object_release(cfg);
	return EXIT_SUCCESS;
}

int
dev_detach(int ac, char **av)
{
	prop_dictionary_t	 dev;
	char			 path[MAXPATHLEN];
	int			 fd, ch, quiet;

	quiet = 0;

	while ((ch = getopt(ac, av, "q")) != -1) {
		switch(ch) {
		case 'q':
			quiet = 1;
			break;

		default:
			errx(EXIT_FAILURE, "unkown option -%c", ch);
		}
	}

	dev = prop_dictionary_create();
	if (dev == NULL)
		err(EXIT_FAILURE, "prop_dictionary_create");

	snprintf(path, sizeof(path), "/dev/%s", control_file);
	fd = open(path, O_WRONLY, 0);
	if (fd < 0)
		err(EXIT_FAILURE, "%s", path);

	if (prop_dictionary_send_ioctl(dev, fd, BTDEV_DETACH)) {
		if (quiet && errno == ENXIO)
			exit(EXIT_FAILURE);

		err(EXIT_FAILURE, "%s", path);
	}

	close(fd);
	prop_object_release(dev);
	return EXIT_SUCCESS;
}
