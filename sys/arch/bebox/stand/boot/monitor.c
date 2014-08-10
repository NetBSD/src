/*	$NetBSD: monitor.c,v 1.9.58.1 2014/08/10 06:53:53 tls Exp $	*/

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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "boot.h"

extern int errno;
extern char *name;

void db_cmd_dump(int, char **);
void db_cmd_get(int, char **);
void db_cmd_mf(int, char **);
void db_cmd_mt(int, char **);
void db_cmd_put(int, char **);
void db_cmd_help(int, char **);

uint32_t db_atob(char *);

struct {
	char *name;
	void (*fcn)(int, char **);
} db_cmd[] = {
	{ "dump",	db_cmd_dump },
	{ "get",	db_cmd_get },
	{ "mf",		db_cmd_mf },
	{ "mt",		db_cmd_mt },
	{ "put",	db_cmd_put },
	{ "help",	db_cmd_help },
	{ NULL,		NULL },
};

int
db_monitor(void)
{
	int tmp;
	int argc, flag;
	char *p, *argv[16];
	char line[1024];

	while (1) {
		printf("db> ");
		gets(line);

		flag = 0;
		for (p = line, argc = 0; *p != '\0'; p++) {
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
			if (!strcmp("continue", argv[0]))
				return 0;
			if (!strcmp(db_cmd[tmp].name, argv[0])) {
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

uint32_t
db_atob(char *p)
{
	uint32_t b = 0;
	int width, tmp, exp, x = 0;

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

void
db_cmd_dump(int argc, char **argv)
{
	char *p, *r, *pp;
	int mode, size, i;
	uint32_t add;

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
		if (!i)
			printf("\n0x%x:", add);
		switch (mode) {
		case 1:
			printf(" %x", *(uint8_t *)add);
			add += 1;
			size -= 1;
			if (++i == 16)
				i = 0;
			break;
		case 2:
			printf(" %x", *(uint16_t *)add);
			add += 2;
			size -= 2;
			if (++i == 8)
				i = 0;
			break;
		case 4:
			printf(" %x", *(uint32_t *)add);
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

void
db_cmd_get(int argc, char **argv)
{
	char *p, *r;
	uint32_t add;
	int mode;

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
	printf("0x%x: ", add);
	switch (mode) {
	case 1:
		printf("0x%x", *(uint8_t *)add);
		break;
	case 2:
		printf("0x%x", *(uint16_t *)add);
		break;
	case 4:
		printf("0x%x", *(uint32_t *)add);
		break;
	}
	printf("\n");
	return;

out:
	printf("get [-b][-h][-w] address\n");
	return;
}

void
db_cmd_put(int argc, char **argv)
{
	char *p, *r, *pp;
	uint32_t add, data;
	int mode;

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
	printf("0x%x: 0x%x", add, data);
	switch (mode) {
	case 1:
		*(uint8_t *)add = data;
		break;
	case 2:
		*(uint16_t *)add = data;
		break;
	case 4:
		*(uint32_t *)add = data;
		break;
	}
	printf("\n");
	return;

out:
	printf("put [-b][-h][-w] address data\n");
	return;
}

#define STR(x) #x

#define	FUNC(x) \
uint32_t mf ## x(void); \
void mt ## x(uint32_t); \
uint32_t mf ## x() { \
	uint32_t tmp; \
	__asm volatile (STR(mf ## x %0) : STR(=r)(tmp)); \
	return (tmp); \
} \
void mt ## x(uint32_t data) \
{ \
	__asm volatile (STR(mt ## x %0) :: STR(r)(data)); \
} \

#define DEF(x) \
	{ #x, mf ## x, mt ## x }

FUNC(msr)

struct {
	char *op;
	uint32_t (*mf)(void);
	void (*mt)(uint32_t);
} mreg [] = {
	DEF(msr),
	{ NULL, NULL, NULL },
};

void
db_cmd_mf(int argc, char **argv)
{
	int i = 0;

	if (argc != 2) {
		printf("mf register\nregister:");
		while (mreg[i].op != NULL)
			printf(" %s", mreg[i++].op);
		printf("\n");
		return;
	}

	while (mreg[i].op != NULL) {
		if (!strcmp(mreg[i].op, argv[1])) {
			printf(" 0x%x\n", (mreg[i].mf)());
			break;
		}
		i++;
	}
}

void
db_cmd_mt(int argc, char **argv)
{
	int i = 0;

	if (argc != 3) {
		printf("mt register data\nregister:");
		while (mreg[i].op != NULL)
			printf(" %s", mreg[i++].op);
		printf("\n");
		return;
	}

	while (mreg[i].op != NULL) {
		if (!strcmp(mreg[i].op, argv[1])) {
			(mreg[i].mt)(db_atob(argv[2]));
			printf(" 0x%x\n", db_atob(argv[2]));
			break;
		}
		i++;
	}
}

void
db_cmd_help(int argc, char **argv)
{
	int i = 0;

	while (db_cmd[i].name != NULL)
		printf("%s, ", db_cmd[i++].name);
	printf("\n");
}
