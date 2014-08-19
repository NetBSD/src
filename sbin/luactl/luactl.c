/*	$NetBSD: luactl.c,v 1.2.8.2 2014/08/20 00:02:25 tls Exp $ */

/*
 * Copyright (c) 2011, Marc Balmer <mbalmer@NetBSD.org>.
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
 * 3. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Program to control Lua devices.
 */

#include <stdbool.h>
#include <sys/param.h>
#include <sys/lua.h>
#include <sys/ioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int devfd = -1;
int quiet = 0;
int docreate = 0;

static void getinfo(void);
static void create(char *, char *);
static void destroy(char *);

static void require(char *, char *);
static void load(char *, char *);

static void usage(void) __dead;

#define _PATH_DEV_LUA	"/dev/lua"

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "cq")) != -1)
		switch (ch) {
		case 'c':
			docreate = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if ((devfd = open(_PATH_DEV_LUA, O_RDWR)) == -1)
		err(EXIT_FAILURE, "%s", _PATH_DEV_LUA);

	if (argc == 0)
		getinfo();
	else if (!strcmp(argv[0], "create")) {
		if (argc == 2)
			create(argv[1], NULL);
		else if (argc == 3)
			create(argv[1], argv[2]);
		else
			usage();
	} else if (!strcmp(argv[0], "destroy")) {
		if (argc != 2)
			usage();
		destroy(argv[1]);
	} else if (!strcmp(argv[0], "require")) {
		if (argc != 3)
			usage();
		if (docreate)
			create(argv[1], NULL);
		require(argv[1], argv[2]);
	} else if (!strcmp(argv[0], "load")) {
		if (argc != 3)
			usage();
		if (docreate)
			create(argv[1], NULL);
		load(argv[1], argv[2]);
	} else
		usage();

	return EXIT_SUCCESS;
}

static void
getinfo(void)
{
	struct lua_info info;
	int n;

	info.states = NULL;
	if (ioctl(devfd, LUAINFO, &info) == -1)
		err(EXIT_FAILURE, "LUAINFO");

	if (info.num_states > 0) {
		info.states = calloc(info.num_states,
		    sizeof(struct lua_state_info));
		if (info.states == NULL)
			err(EXIT_FAILURE, "calloc");
		if (ioctl(devfd, LUAINFO, &info) == -1)
			err(EXIT_FAILURE, "LUAINFO");
	}
	printf("%d active state%s:\n", info.num_states,
	    info.num_states == 1 ? "" : "s");
	if (info.num_states > 0)
		printf("%-16s %-8s Description\n", "Name", "Creator");
	for (n = 0; n < info.num_states; n++)
		printf("%-16s %-8s %s\n", info.states[n].name,
		    info.states[n].user == true ? "user" : "kernel",
		    info.states[n].desc);
}

static void
create(char *name, char *desc)
{
	struct lua_create cr;

	strlcpy(cr.name, name, sizeof(cr.name));
	if (desc != NULL)
		strlcpy(cr.desc, desc, sizeof(cr.desc));
	else
		cr.desc[0] = '\0';

	if (ioctl(devfd, LUACREATE, &cr) == -1)
		err(EXIT_FAILURE, "LUACREATE");

	if (quiet)
		return;

	printf("%s created\n", name);
}

static void
destroy(char *name)
{
	struct lua_create cr;

	strlcpy(cr.name, name, sizeof(cr.name));

	if (ioctl(devfd, LUADESTROY, &cr) == -1)
		err(EXIT_FAILURE, "LUADESTROY");

	if (quiet)
		return;

	printf("%s destroyed\n", name);
}

static void
require(char *name, char *module)
{
	struct lua_require r;

	strlcpy(r.state, name, sizeof(r.state));
	strlcpy(r.module, module, sizeof(r.module));

	if (ioctl(devfd, LUAREQUIRE, &r) == -1)
		err(EXIT_FAILURE, "LUAREQUIRE");

	if (quiet)
		return;

	printf("%s required by %s\n", module, name);
}

static void
load(char *name, char *path)
{
	struct lua_load l;

	strlcpy(l.state, name, sizeof(l.state));
	strlcpy(l.path, path, sizeof(l.path));

	if (ioctl(devfd, LUALOAD, &l) == -1)
		err(EXIT_FAILURE, "LUALOAD");

	if (quiet)
		return;

	printf("%s loaded into %s\n", path, name);
}

static void
usage(void)
{
	const char *p;

	p = getprogname();
	fprintf(stderr, "usage: %s [-cq]\n", p);
	fprintf(stderr, "       %s [-cq] create name [desc]\n", p);
	fprintf(stderr, "       %s [-cq] destroy name\n", p);
	fprintf(stderr, "       %s [-cq] require name module\n", p);
	fprintf(stderr, "       %s [-cq] load name path\n", p);

	exit(EXIT_FAILURE);
}
