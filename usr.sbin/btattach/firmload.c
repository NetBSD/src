/* $NetBSD: firmload.c,v 1.1 2017/08/10 20:43:12 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "firmload.h"

#define HW_FIRMWARE_PATH	"hw.firmware.path"
#define	HW_FIRMWARE_PATH_DELIM	":"

static int
firmware_search(char *paths, const char *drvname, const char *imgname)
{
	char *p, *f;
	const int debug = getenv("FIRMWARE_DEBUG") != NULL;
	int fd;

	p = strtok(paths, HW_FIRMWARE_PATH_DELIM);
	while (p) {
		if (asprintf(&f, "%s/%s/%s", p, drvname, imgname) == -1)
			return -1;
		if (debug)
			printf("%s: trying %s...\n", __func__, f);
		fd = open(f, O_RDONLY);
		free(f);
		if (fd != -1) {
			if (debug)
				printf("%s: using image at %s\n", __func__, f);
			return fd;
		}
		p = strtok(NULL, HW_FIRMWARE_PATH_DELIM);
	}

	/* Not found */
	return -1;
}

int
firmware_open(const char *drvname, const char *imgname)
{
	size_t len;
	char *paths;
	int fd;

	paths = asysctlbyname(HW_FIRMWARE_PATH, &len);
	if (paths == NULL)
		return -1;

	fd = firmware_search(paths, drvname, imgname);

	free(paths);

	return fd;
}
