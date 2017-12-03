/*	$NetBSD: monitor.c,v 1.3.22.1 2017/12/03 11:36:22 jdolecek Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Kazuki Sakamoto.
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

#if defined(DBMONITOR)

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "boot.h"

#ifndef	NULL
#define NULL	(void *)0
#endif

static void db_cmd_dump(int, char **);
static void db_cmd_get(int, char **);
static void db_cmd_put(int, char **);
static void db_cmd_help(int, char **);

static int db_atob(char *);

static const struct db_cmd {
	char *name;
	void (*fcn)(int, char **);
} db_cmd[] = {
	{ "dump",	db_cmd_dump },
	{ "get",	db_cmd_get },
	{ "put",	db_cmd_put },
	{ "help",	db_cmd_help },
	{ NULL,		NULL },
};

int
db_monitor(void)
{
	char line[1024];
	char *p, *argv[16];
	int argc, flag;
	int tmp;

	for (;;) {
		printf("db> ");
		kgets(line, sizeof(line));

		flag = 0;
		argc = 0;
		for (p = line; *p != '\0'; p++) {
			if (*p != ' ' && *p != '\t') {
				if (!flag) {
					flag++;
					argv[argc++] = p;
				}
			} else {
				if (flag) {
					*p = '\0';
					flag = 0;
				}
			}
		}
		if (argc == 0)
			continue;

		tmp = 0;
		while (db_cmd[tmp].name != NULL) {
			if (strcmp("continue", argv[0]) == 0)
				return 0;
			if (strcmp(db_cmd[tmp].name, argv[0]) == 0) {
				(db_cmd[tmp].fcn)(argc, argv);
				break;
			}
			tmp++;
		}
		if (db_cmd[tmp].name == NULL)
			db_cmd_help(argc, argv);
	}

	return 0;
}

static int
db_atob(char *p)
{
	int b = 0, width, tmp, exp, x = 0;
	
	if (p[1] == 'x') {
		p += 2;
		x = 1;
	}

	width = strlen(p);
	while (width--) {
		exp = 1;
		for (tmp = 1; tmp <= width; tmp++)
			exp *= (x ? 16 : 10);
		if (*p >= '0' && *p <= '9') {
			tmp = *p - '0';
		} else {
			tmp = *p - 'a' + 10;
		}
		b += tmp * exp;
		p++;
	}
	return b;
}

static void
db_cmd_dump(int argc, char **argv)
{
	char *p, *r, *pp;
	int mode, add, size, i;

	switch (argc) {
	case 4:
		r = argv[1];
		switch (r[1]) {
		case 'b':
			mode = 1;
			break;
		case 'h':
			mode = 2;
			break;
		case 'w':
			mode = 4;
			break;
		default:
			goto out;
		}
		p = argv[2];
		pp = argv[3];
		break;
	case 3:
		mode = 4;
		p = argv[1];
		pp = argv[2];
		break;
	default:
		goto out;
	}

	add = db_atob(p);
	size = db_atob(pp);
	i = 0;
	for (; size > 0;) {
		if (i == 0) {
			printf("\n0x%x:", add);
		}
		switch (mode) {
		case 1:
			printf(" ");
			puthex(*(unsigned char *)add, 1);
			add += 1;
			size -= 1;
			if (++i == 16)
				i = 0;
			break;
		case 2:
			printf(" ");
			puthex(*(unsigned short *)add, 2);
			add += 2;
			size -= 2;
			if (++i == 8)
				i = 0;
			break;
		case 4:
			printf(" ");
			puthex(*(unsigned int *)add, 4);
			add += 4;
			size -= 4;
			if (++i == 4)
				i = 0;
			break;
		}
	}
	printf("\n");
	return;

out:	
	printf("dump [-b][-h][-w] address size\n");
	return;
}

static void
db_cmd_get(int argc, char **argv)
{
	char *p, *r;
	int mode, add;
	int val;

	switch (argc) {
	case 3:
		r = argv[1];
		switch (r[1]) {
		case 'b':
			mode = 1;
			break;
		case 'h':
			mode = 2;
			break;
		case 'w':
			mode = 4;
			break;
		default:
			goto out;
		}
		p = argv[2];
		break;
	case 2:
		mode = 4;
		p = argv[1];
		break;
	default:
		goto out;
	}

	add = db_atob(p);
	printf("0x%x: 0x", add);
	switch (mode) {
	case 1:
		val = *(char *)add;
		break;
	case 2:
		val = *(short *)add;
		break;
	case 4:
		val = *(int *)add;
		break;
	default:
		val = 0;
		break;
	}
	puthex(val, mode);
	printf("\n");
	return;

out:	
	printf("get [-b][-h][-w] address\n");
	return;
}

static void
db_cmd_put(int argc, char **argv)
{
	char *p, *r, *pp;
	int mode, add, data;

	switch (argc) {
	case 4:
		r = argv[1];
		switch (r[1]) {
		case 'b':
			mode = 1;
			break;
		case 'h':
			mode = 2;
			break;
		case 'w':
			mode = 4;
			break;
		default:
			goto out;
		}
		p = argv[2];
		pp = argv[3];
		break;
	case 3:
		mode = 4;
		p = argv[1];
		pp = argv[2];
		break;
	default:
		goto out;
	}

	add = db_atob(p);
	data = db_atob(pp);
	printf("0x%x: 0x", add);
	puthex(data, mode);
	switch (mode) {
	case 1:
		*(char *)add = data;
		break;
	case 2:
		*(short *)add = data;
		break;
	case 4:
		*(int *)add = data;
		break;
	}
	printf("\n");
	return;

out:	
	printf("put [-b][-h][-w] address data\n");
	return;
}

static void
db_cmd_help(int argc, char **argv)
{
	int i = 0;

	while (db_cmd[i].name != NULL)
		printf("%s, ", db_cmd[i++].name);
	printf("continue\n");
}

#endif	/* DBMONITOR */
