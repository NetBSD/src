/*	$NetBSD: memswitch.c,v 1.14 2018/01/23 22:40:06 sevan Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
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

/* memswitch.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/ioctl.h>

#ifndef DEBUG
#include <machine/sram.h>
#else
/*
 * DEBUG -- works on other (faster) platforms;
 *   store in a regular file instead of actual non-volatile static RAM.
 */
#define PATH_RAMFILE "/tmp/sramfile"
#endif

#include "memswitch.h"

char *progname;
int nflag = 0;
u_int8_t *current_values = 0;
u_int8_t *modified_values = 0;

static void
usage(void)
{
	fprintf(stderr, "usage: %s -a\n", progname);
	fprintf(stderr, "       %s [-h] variable ...\n", progname);
	fprintf(stderr, "       %s -w variable=value ...\n", progname);
	fprintf(stderr, "       %s [-rs] filename\n", progname);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int ch;
	enum md {
		MD_NONE, MD_WRITE, MD_HELP, MD_SHOWALL, MD_SAVE, MD_RESTORE
	} mode = MD_NONE;

	progname = argv[0];

	while ((ch = getopt(argc, argv, "whanrs")) != -1) {
		switch (ch) {
		case 'w':	/* write */
			mode = MD_WRITE;
			break;
		case 'h':
			mode = MD_HELP;
			break;
		case 'a':
			mode = MD_SHOWALL;
			break;
		case 'n':
			nflag = 1;
			break;
		case 's':
			mode = MD_SAVE;
			break;
		case 'r':
			mode = MD_RESTORE;
			break;
		}
	}
	argc -= optind;
	argv += optind;

	switch (mode) {
	case MD_NONE:
		if (argc == 0)
			usage();
		while (argv[0]) {
			show_single(argv[0]);
			argv++;
		}
		break;
	case MD_SHOWALL:
		if (argc)
			usage();
		show_all();
		break;
	case MD_WRITE:
		if (argc == 0)
			usage();
		while (argv[0]) {
			modify_single (argv[0]);
			argv++;
		}
		flush();
		break;
	case MD_HELP:
		if (argc == 0)
			usage();
		while (argv[0]) {
			help_single(argv[0]);
			argv++;
		}
		break;
	case MD_SAVE:
		if (argc != 1)
			usage();
		save(argv[0]);
		break;
	case MD_RESTORE:
		if (argc != 1)
			usage();
		restore(argv[0]);
		break;
		
	}

	return 0;
}

void
show_single(const char *name)
{
	int i;
	int n = 0;
	char fullname[50];
	char valuestr[MAXVALUELEN];

	for (i = 0; i < number_of_props; i++) {
		snprintf(fullname, sizeof(fullname), "%s.%s",
			 properties[i].class, properties[i].node);
		if (strcmp(name, fullname) == 0 || strcmp(name, properties[i].class) == 0) {
			properties[i].print(&properties[i], valuestr);
			if (!nflag)
				printf("%s=%s\n", fullname, valuestr);
			n++;
		}
	}
	if (n == 0) {
		errx(1, "No such %s: %s", strstr(name, ".")?"property":"class", name);
	}

	return;
}

void
show_all(void)
{
	int i;
	char valuestr[MAXVALUELEN];

	for (i = 0; i < number_of_props; i++) {
		properties[i].print(&properties[i], valuestr);
		if (!nflag)
			printf("%s.%s=",
			       properties[i].class, properties[i].node);
		printf("%s\n", valuestr);
	}

	return;
}

void
modify_single(const char *expr)
{
	int i, l, n;
	char *class = NULL, *node = NULL;
	const char *value;
	char valuestr[MAXVALUELEN];

	l = 0;
	n = strlen(expr);
	for (i = 0; i < n; i++) {
		if (expr[i] == '.') {
			l = i + 1;
			class = alloca(l);
			if (class == 0)
				err(1, "alloca");
			strncpy(class, expr, i);
			class[i] = 0;
			break;
		}
	}
	if (i >= n)
		errx(1, "Invalid expression: %s", expr);

	for ( ; i < n; i++) {
		if (expr[i] == '=') {
			node = alloca(i - l + 1);
			if (node == 0)
				err(1, "alloca");
			strncpy(node, &(expr[l]), i - l);
			node[i - l] = 0;
			break;
		}
	}
	if (i >= n)
		errx(1, "Invalid expression: %s", expr);

	value = &(expr[++i]);

	for (i = 0; i < number_of_props; i++) {
		if (strcmp(properties[i].class, class) == 0 &&
		    strcmp(properties[i].node, node) == 0) {
			if (properties[i].parse(&properties[i], value) < 0) {
				/* error: do nothing */
			} else {
				properties[i].print(&properties[i], valuestr);
				printf("%s.%s -> %s\n", class, node, valuestr);
			}
			break;
		}
	}
	if (i >= number_of_props) {
		errx(1, "No such property: %s.%s", class, node);
	}

	return;
}

void
help_single(const char *name)
{
	int i;
	char fullname[50];
	char valuestr[MAXVALUELEN];

	for (i = 0; i < number_of_props; i++) {
		snprintf(fullname, sizeof(fullname), "%s.%s",
		    properties[i].class, properties[i].node);
		if (strcmp(name, fullname) == 0) {
			properties[i].print(&properties[i], valuestr);
			if (!nflag)
				printf("%s=", fullname);
			printf("%s\n", valuestr);
			printf("%s", properties[i].descr);
			break;
		}
	}
	if (i >= number_of_props) {
		errx(1, "No such property: %s", name);
	}

	return;
}

void
alloc_modified_values(void)
{
	if (current_values == 0)
		alloc_current_values();
	modified_values = malloc(256);
	if (modified_values == 0)
		err(1, "malloc");
	memcpy(modified_values, current_values, 256);
}

void
alloc_current_values(void)
{
#ifndef DEBUG
	int i;
	int sramfd = 0;
	struct sram_io buffer;

	current_values = malloc(256);
	if (current_values == 0)
		err(1, "malloc");

	sramfd = open(_PATH_DEVSRAM, O_RDONLY);
	if (sramfd < 0)
		err(1, "Opening %s", _PATH_DEVSRAM);

	/* Assume SRAM_IO_SIZE = n * 16. */
	for (i = 0; i < 256; i += SRAM_IO_SIZE) {
		buffer.offset = i;
		if (ioctl(sramfd, SIOGSRAM, &buffer) < 0)
			err(1, "ioctl");
		memcpy(&current_values[i], buffer.sram, SRAM_IO_SIZE);
	}

	close(sramfd);
#else
	int i;
	int fd;
	struct stat st;

	current_values = malloc(256);
	if (current_values == 0)
		err(1, "malloc");

	fd = open(PATH_RAMFILE, O_RDONLY);
	if (fd < 0 && errno == ENOENT) {
		modified_values = malloc(256);
		if (modified_values == 0)
			err(1, NULL);
		for (i = 0; i < number_of_props; i++) {
			properties[i].modified_value
			    = properties[i].default_value;
			properties[i].modified = 1;
			properties[i].flush(&properties[i]);
		}

		fd = creat(PATH_RAMFILE, 0666);
		if (fd < 0)
			err(1, "Creating %s", PATH_RAMFILE);
		if (write(fd, modified_values, 256) != 256)
			err(1, "Writing %s", PATH_RAMFILE);
		close(fd);
		free(modified_values);
		modified_values = 0;

		fd = open(PATH_RAMFILE, O_RDONLY);
	}
	if (fd < 0)
		err(1, "Opening %s", PATH_RAMFILE);
	if (fstat(fd, &st) < 0)
		err(1, "fstat");
	if (st.st_size != 256)
		errx(1, "PANIC! INVALID RAMFILE");
	if (read(fd, current_values, 256) != 256)
		err(1, "reading %s", PATH_RAMFILE);
	close(fd);
#endif

	properties[PROP_MAGIC1].fill(&properties[PROP_MAGIC1]);
	properties[PROP_MAGIC2].fill(&properties[PROP_MAGIC2]);
	if ((properties[PROP_MAGIC1].current_value.longword != MAGIC1) ||
	    (properties[PROP_MAGIC2].current_value.longword != MAGIC2))
		errx(1, "PANIC! INVALID MAGIC");
}

void
flush(void)
{
	int i;
	int sramfd = 0;
#ifndef DEBUG
	struct sram_io buffer;
#endif

	for (i = 0; i < number_of_props; i++) {
		if (properties[i].modified)
			properties[i].flush(&properties[i]);
	}

	if (modified_values == 0)
		/* Not modified at all. */
		return;

#ifndef DEBUG
	/* Assume SRAM_IO_SIZE = n * 16. */
	for (i = 0; i < 256; i += SRAM_IO_SIZE) {
		if (memcmp(&current_values[i], &modified_values[i],
			   SRAM_IO_SIZE) == 0)
			continue;

		if (sramfd == 0) {
			sramfd = open(_PATH_DEVSRAM, O_RDWR);
			if (sramfd < 0)
				err(1, "Opening %s", _PATH_DEVSRAM);
		}
		buffer.offset = i;
		memcpy(buffer.sram, &modified_values[i], SRAM_IO_SIZE);
		if (ioctl(sramfd, SIOPSRAM, &buffer) < 0)
			err(1, "ioctl");
	}
#else
	sramfd = open(PATH_RAMFILE, O_WRONLY);
	if (sramfd < 0)
		err(1, "Opening %s", PATH_RAMFILE);
	if (write(sramfd, modified_values, 256) != 256)
		err(1, "Writing %s", PATH_RAMFILE);
#endif

	if (sramfd != 0)
		close(sramfd);

	return;
}

int
save(const char *name)
{
#ifndef DEBUG
	int fd;

	alloc_current_values();

	if (strcmp(name, "-") == 0)
		fd = 1;		/* standard output */
	else {
		fd = open(name, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if (fd < 0)
			err(1, "Opening output file");
	}

	if (write(fd, current_values, 256) != 256)
		err(1, "Writing output file");

	if (fd != 1)
		close(fd);
#else
	fprintf(stderr, "Skipping save...\n");
#endif

	return 0;
}

int
restore(const char *name)
{
#ifndef DEBUG
	int sramfd, fd, i;
	struct sram_io buffer;

	modified_values = malloc(256);
	if (modified_values == 0)
		err(1, "Opening %s", _PATH_DEVSRAM);

	if (strcmp(name, "-") == 0)
		fd = 0;		/* standard input */
	else {
		fd = open(name, O_RDONLY);
		if (fd < 0)
			err(1, "Opening input file");
	}

	if (read(fd, modified_values, 256) != 256)
		err(1, "Reading input file");

	if (fd != 0)
		close(fd);

	sramfd = open(_PATH_DEVSRAM, O_RDWR);
	if (sramfd < 0)
		err(1, "Opening %s", _PATH_DEVSRAM);

	/* Assume SRAM_IO_SIZE = n * 16. */
	for (i = 0; i < 256; i += SRAM_IO_SIZE) {
		buffer.offset = i;
		memcpy(buffer.sram, &modified_values[i], SRAM_IO_SIZE);
		if (ioctl(sramfd, SIOPSRAM, &buffer) < 0)
			err(1, "ioctl");
	}

	close(sramfd);
#else
	fprintf(stderr, "Skipping restore...\n");
#endif

	return 0;
}
