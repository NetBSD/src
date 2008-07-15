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

#include "dev-cache.h"
#include "log.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	int i;
	struct device *dev;
	struct dev_iter *iter;
	struct list_head *tmp;
	struct str_list *sl;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <dir>\n", argv[0]);
		exit(1);
	}

	init_log(stderr);
	init_debug(_LOG_INFO);

	if (!dev_cache_init()) {
		log_err("couldn't initialise dev_cache_init failed");
		exit(1);
	}

	for (i = 1; i < argc; i++) {
		if (!dev_cache_add_dir(argv[i])) {
			log_err("couldn't add '%s' to dev_cache", argv[i]);
			exit(1);
		}
	}

	if (!(iter = dev_iter_create(NULL))) {
		log_err("couldn't create iterator");
		exit(1);
	}

	while ((dev = dev_iter_get(iter))) {
		printf("%s", dev->name);

		list_for_each(tmp, &dev->aliases) {
			sl = list_entry(tmp, struct str_list, list);
			printf(", %s", sl->str);
		}
		printf("\n");
	}

	dev_iter_destroy(iter);
	dev_cache_exit();

	dump_memory();
	fin_log();
	return 0;
}
