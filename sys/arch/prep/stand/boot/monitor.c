/*	$NetBSD: monitor.c,v 1.1 2000/02/29 15:21:50 nonaka Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include "boot.h"

#define NULL	0

extern int errno;
extern char *name;

void db_cmd_dump __P((int, char **));
void db_cmd_get __P((int, char **));
void db_cmd_mf __P((int, char **));
void db_cmd_mt __P((int, char **));
void db_cmd_put __P((int, char **));
void db_cmd_help __P((int, char **));

unsigned int mfmsr __P((void));
void mtmsr __P((unsigned int));

int db_atob __P((char *));

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
db_monitor()
{
	int tmp;
	int argc, flag;
	char *p, *argv[16];
	char line[1024];

	while(1) {
		printf("db> ");
		gets(line);

		flag = 0;
		for(p = line, argc = 0; *p != '\0'; p++) {
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

int
db_atob(p)
	char *p;
{
	int b = 0, width, tmp, exp, x = 0;
	
	if (p[1] == 'x') {
		p += 2;
		x = 1;
	}
	width = strlen(p);
	while(width--) {
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
db_cmd_dump(argc, argv)
	int argc;
	char **argv;
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
		if (!i)
			printf("\n0x%x:", add);
		switch (mode) {
		case 1:
			printf(" %x", *(unsigned char *)add);
			add += 1;
			size -= 1;
			if (++i == 16)
				i = 0;
			break;
		case 2:
			printf(" %x", *(unsigned short *)add);
			add += 2;
			size -= 2;
			if (++i == 8)
				i = 0;
			break;
		case 4:
			printf(" %x", *(unsigned int *)add);
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
db_cmd_get(argc, argv)
	int argc;
	char **argv;
{
	char *p, *r;
	int mode, add;

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
		printf("0x%x", *(char *)add);
		break;
	case 2:
		printf("0x%x", *(short *)add);
		break;
	case 4:
		printf("0x%x", *(int *)add);
		break;
	}
	printf("\n");
	return;

out:	
	printf("get [-b][-h][-w] address\n");
	return;
}

void
db_cmd_put(argc, argv)
	int argc;
	char **argv;
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
	printf("0x%x: 0x%x", add, data);
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

#define STR(x) #x

#define	FUNC(x) \
unsigned int mf ## x() { \
	unsigned int tmp; \
	asm volatile (STR(mf ## x %0) : STR(=r)(tmp)); \
	return (tmp); \
} \
void mt ## x(data) \
unsigned int data; \
{ \
	asm volatile (STR(mt ## x %0) :: STR(r)(data)); \
} \

#define DEF(x) \
	{ #x, mf ## x, mt ## x }

FUNC(msr);

struct {
	char *op;
	unsigned int (*mf)(void);
	void (*mt)(unsigned int);
} mreg [] = {
	DEF(msr),
	{ NULL, NULL, NULL },
};

void
db_cmd_mf(argc, argv)
	int argc;
	char **argv;
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
db_cmd_mt(argc, argv)
	int argc;
	char **argv;
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
			(mreg[i].mt)((unsigned int)db_atob(argv[2]));
			printf(" 0x%x\n", db_atob(argv[2]));
			break;
		}
		i++;
	}
}

void
db_cmd_help(argc, argv)
	int argc;
	char **argv;
{
	int i = 0;

	while (db_cmd[i].name != NULL)
		printf("%s, ", db_cmd[i++].name);
	printf("continue\n");
}
