/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "log.h"

#include <stdio.h>
#include <unistd.h>


static void _print_help(FILE *out, const char *prog)
{
	fprintf(out, "usage : %s [-hlbufd]\n\n", prog);
	fprintf(out, "  h : this message\n");
	fprintf(out, "  l : cause memory leak\n");
	fprintf(out, "  b : overrun memory block\n");
	fprintf(out, "  u : underrun memory block\n");
	fprintf(out, "  f : free random pointer\n");
	fprintf(out, "  d : free block twice\n");
}

struct block_list {
	struct block_list *next;
	char dummy[9];
};

static void _leak_memory(void)
{
	int i;
	struct block_list *b, *head, **l = &head, *n;

	/* allocate a list of blocks */
	for (i = 0; i < 1000; i++) {

		if (!(b = dbg_malloc(sizeof(*b)))) {
			log_fatal("Couldn't allocate memory");
			exit(1);
		}

		b->next = 0;
		*l = b;
		l = &b->next;
	}

	/* free off every other block */
	for (b = head, i = 0; b; b = n, i++) {
		n = b->next;
		if(i & 0x1)
			dbg_free(b);
	}
}

static void _bounds_overrun(void)
{
	char *b;

	/* allocate a block */
	b = dbg_malloc(534);

	/* overrun */
	b[534] = 56;

	/* free it, which should trigger the bounds error */
	dbg_free(b);
}

static void _bounds_underrun(void)
{
	char *b;

	/* allocate a block */
	b = dbg_malloc(534);

	/* underrun */
	*(b - 1) = 56;

	/* free it, which should trigger the bounds error */
	dbg_free(b);
}

static void _free_dud(void)
{
	char *b;

	/* allocate a block */
	b = dbg_malloc(534);

	/* free it, which should trigger the bounds error */
	dbg_free(b + 100);
}

static void _free_twice(void)
{
	char *b;

	/* allocate a block */
	b = dbg_malloc(534);

	/* free it, which should trigger the bounds error */
	dbg_free(b);
	dbg_free(b);
}

int main(int argc, char **argv)
{
	char opt;

	init_log(stderr);
	init_debug(_LOG_DEBUG);
	opt = getopt(argc, argv, "hlbufd");
	switch(opt) {
	case EOF:
	case 'h':
		_print_help(stdout, argv[0]);
		break;

	case 'l':
		_leak_memory();
		break;

	case 'b':
		_bounds_overrun();
		break;

	case 'u':
		_bounds_underrun();
		break;

	case 'f':
		_free_dud();
		break;

	case 'd':
		_free_twice();
		break;

	case '?':
		fprintf(stderr, "Unknown option -%c\n", opt);
		exit(1);
	}

	dump_memory();
	fin_log();
	return 0;
}
