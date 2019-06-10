/*	$NetBSD: main.c,v 1.2.4.2 2019/06/10 22:04:33 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <config.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "fuzz.h"

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION

#include <dirent.h>

static void test_all_from(const char *dirname)
{
	DIR *dirp;
	struct dirent *dp;

	dirp = opendir(dirname);
	if (dirp == NULL) {
		return;
	}

	while ((dp = readdir(dirp)) != NULL) {
		char filename[strlen(dirname) + strlen(dp->d_name) + 2];
		int fd;
		struct stat st;
		char *data;
		ssize_t n;

		if (dp->d_name[0] == '.') {
			continue;
		}
		snprintf(filename, sizeof(filename), "%s/%s",
			 dirname, dp->d_name);

		if ((fd = open(filename, O_RDONLY)) == -1) {
			fprintf(stderr, "Failed to open %s: %s\n", filename,
				strerror(errno));
			continue;
		}

		if (fstat(fd, &st) != 0) {
			fprintf(stderr, "Failed to stat %s: %s\n", filename,
				strerror(errno));
			goto closefd;
		}

		data = malloc(st.st_size);
		n = read(fd, data, st.st_size);
		if (n == st.st_size) {
			printf("testing %zd bytes from %s\n",
			       n, filename);
			fflush(stdout);
			LLVMFuzzerTestOneInput((const uint8_t *)data, n);
			fflush(stderr);
		} else {
			if (n < 0) {
				fprintf(stderr,
					"Failed to read %zd bytes from %s: %s\n",
					(ssize_t) st.st_size, filename,
					strerror(errno));
			} else {
				fprintf(stderr,
					"Failed to read %zd bytes from %s"
					", got %zd\n",
					(ssize_t) st.st_size, filename,
					n);
			}
		}
		free(data);
	closefd:
		close(fd);
	}

	closedir(dirp);
}

int main(int argc, char **argv)
{
	char corpusdir[PATH_MAX];
	const char *target = strrchr(argv[0], '/');

	UNUSED(argc);
	UNUSED(argv);

	target = target ? target + 1 : argv[0];
	if (strncmp(target, "lt-", 3) == 0) {
		target += 3;
	}

	snprintf(corpusdir, sizeof(corpusdir), FUZZDIR "/%s.in", target);

	test_all_from(corpusdir);

	return 0;
}

#elif __AFL_COMPILER

int main(int argc, char **argv)
{
	int ret;
	unsigned char buf[64 * 1024];

	UNUSED(argc);
	UNUSED(argv);

#ifdef __AFL_LOOP
	while (__AFL_LOOP(10000)) { // only works with afl-clang-fast
#else
	{
#endif
		ret = fread(buf, 1, sizeof(buf), stdin);
		if (ret < 0) {
			return 0;
		}

		LLVMFuzzerTestOneInput(buf, ret);
	}

	return 0;
}

#endif /* FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION */
