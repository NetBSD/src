/*	$NetBSD: memswitch.h,v 1.4 2018/01/24 14:45:44 sevan Exp $	*/

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

/* memswitch.h */

#include <paths.h>

#ifndef _PATH_DEVSRAM
#define _PATH_DEVSRAM "/dev/sram"
#endif

#define PROP_MAGIC1 (0)
#define PROP_MAGIC2 (1)
#define MAGIC1	(0x82773638)
#define MAGIC2	(0x30303057)
#define MAXVALUELEN 100

struct property;

typedef int (*parse_t)(struct property*, const char*);
typedef int (*print_t)(struct property*, char*);
typedef int (*fill_t)(struct property*);
typedef int (*flush_t)(struct property*);

struct property {
	const char *class;
	const char *node;
	int offset;
	int size;
	int value_valid;
	union value {
		unsigned char byte[4];
		unsigned short word[2];
		unsigned long longword;
		void *pointer;
	} current_value;
	int modified;
	union value modified_value;
	union value default_value;
	parse_t parse;
	int min, max;
	print_t print;
	fill_t fill;
	flush_t flush;
	const char *descr;
};

extern int number_of_props;
extern struct property properties[];
extern char *progname;
extern u_int8_t *current_values;
extern u_int8_t *modified_values;

static void usage(void) __dead;
void show_single(const char*));
void show_all(void));
void modify_single(const char*));
void help_single(const char*));
void alloc_current_values(void));
void alloc_modified_values(void));
void flush(void));
int save(const char*));
int restore(const char*));
