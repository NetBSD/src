/* $NetBSD: main.c,v 1.4 2010/09/02 02:08:30 jmcneill Exp $ */

/*
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "audiodev.h"
#include "drvctl.h"

static void
usage(const char *p)
{
	fprintf(stderr, "usage: %s list\n", p);
	fprintf(stderr, "       %s default <index>\n", p);
	fprintf(stderr, "       %s test <index>\n", p);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct audiodev *adev;
	unsigned int n, i;

	if (audiodev_refresh() == -1)
		return EXIT_FAILURE;

	if (argc < 2)
		usage(argv[0]);
		/* NOTREACHED */

	if (strcmp(argv[1], "list") == 0) {
		n = audiodev_count();
		for (i = 0; i < n; i++) {
			adev = audiodev_get(i);
			printf("%u: [%c] %s: ",
			    i, adev->defaultdev ? '*' : ' ', adev->xname);
			printf("%s", adev->audio_device.name);
			if (strlen(adev->audio_device.version) > 0)
				printf(" %s", adev->audio_device.version);
			printf(", %u playback channel%s\n",
			    adev->pchan, adev->pchan == 1 ? "" : "s");
		}
	} else if (strcmp(argv[1], "default") == 0 && argc == 3) {
		if (*argv[2] < '0' || *argv[2] > '9')
			usage(argv[0]);
			/* NOTREACHED */
		errno = 0;
		i = strtoul(argv[2], NULL, 10);
		if (errno)
			usage(argv[0]);
			/* NOTREACHED */
		adev = audiodev_get(i);
		if (adev == NULL) {
			fprintf(stderr, "no such device\n");
			return EXIT_FAILURE;
		}
		printf("setting default audio device to %s\n", adev->xname);
		if (audiodev_set_default(adev) == -1) {
			perror("couldn't set default device");
			return EXIT_FAILURE;
		}
	} else if (strcmp(argv[1], "test") == 0 && argc == 3) {
		if (*argv[2] < '0' || *argv[2] > '9')
			usage(argv[0]);
			/* NOTREACHED */
		errno = 0;
		i = strtoul(argv[2], NULL, 10);
		if (errno)
			usage(argv[0]);
			/* NOTREACHED */
		adev = audiodev_get(i);
		if (adev == NULL) {
			fprintf(stderr, "no such device\n");
			return EXIT_FAILURE;
		}
		for (i = 0; i < adev->pchan; i++) {
			printf("testing channel %d...\n", i);
			audiodev_test(adev, 1 << i);
		}
	} else
		usage(argv[0]);
		/* NOTREACHED */

	return EXIT_SUCCESS;
}
