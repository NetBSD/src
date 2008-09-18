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
#include "../../lib/regex/parse_rx.h"

#include <stdio.h>
#include <ctype.h>

static void _pretty_print(struct rx_node *rx, int depth)
{
	int i;
	for (i = 0; i < depth; i++)
		printf(" ");

	/* display info about the node */
	switch (rx->type) {
	case CAT:
		printf("Cat");
		break;

	case OR:
		printf("Or");
		break;

	case STAR:
		printf("Star");
		break;

	case PLUS:
		printf("Plus");
		break;

	case QUEST:
		printf("Quest");
		break;

	case CHARSET:
		printf("Charset : ");
		for (i = 0; i < 256; i++) {
			if (bit(rx->charset, i) && isprint(i))
				printf("%c", (char) i);
		}
		break;

	default:
		printf("Unknown type");
	}
	printf("\n");

	if (rx->left)
		_pretty_print(rx->left, depth + 1);

	if (rx->right)
		_pretty_print(rx->right, depth + 1);
}


int main(int argc, char **argv)
{
	struct dm_pool *mem;
	struct rx_node *rx;

	if (argc != 2) {
		fprintf(stderr, "Usage : %s <regex>\n", argv[0]);
		exit(0);
	}

	init_log(stderr);
	init_debug(_LOG_INFO);

	if (!(mem = dm_pool_create(1024))) {
		fprintf(stderr, "Couldn't create pool\n");
		exit(1);
	}

	if (!(rx = rx_parse_str(mem, argv[1]))) {
		fprintf(stderr, "Couldn't parse regex\n");
		exit(1);
	}

	_pretty_print(rx, 0);
	dm_pool_destroy(mem);

	dump_memory();
	fin_log();
	return 0;
}
