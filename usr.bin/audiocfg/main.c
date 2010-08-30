/* $NetBSD: main.c,v 1.1.1.1 2010/08/30 02:19:47 mrg Exp $ */

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
	fprintf(stderr, "usage: %s [index]\n", p);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct audiodev *adev;
	unsigned int n, i;

	if (audiodev_refresh() == -1)
		return EXIT_FAILURE;

	switch (argc) {
	case 1:
		n = audiodev_count();
		for (i = 0; i < n; i++) {
			adev = audiodev_get(i);
			printf("%u: [%c] %s: %s\n",
			    i, adev->defaultdev ? '*' : ' ',
			    adev->xname, adev->audio_device.name);
		}
		break;
	case 2:
		if (*argv[1] < '0' || *argv[1] > '9')
			usage(argv[0]);
			/* NOTREACHED */
		errno = 0;
		i = strtoul(argv[1], NULL, 10);
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
		break;
	default:
		usage(argv[0]);
		/* NOTREACHED */
	}

	return EXIT_SUCCESS;
}
