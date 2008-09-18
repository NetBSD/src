/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lib.h"
#include "filter-composite.h"

#include <stdarg.h>

static int _and_p(struct dev_filter *f, struct device *dev)
{
	struct dev_filter **filters = (struct dev_filter **) f->private;

	while (*filters) {
		if (!(*filters)->passes_filter(*filters, dev))
			return 0;
		filters++;
	}

	log_debug("Using %s", dev_name(dev));

	return 1;
}

static void _composite_destroy(struct dev_filter *f)
{
	struct dev_filter **filters = (struct dev_filter **) f->private;

	while (*filters) {
		(*filters)->destroy(*filters);
		filters++;
	}

	dm_free(f->private);
	dm_free(f);
}

struct dev_filter *composite_filter_create(int n, struct dev_filter **filters)
{
	struct dev_filter **filters_copy, *cft;

	if (!filters)
		return_NULL;

	if (!(filters_copy = dm_malloc(sizeof(*filters) * (n + 1)))) {
		log_error("composite filters allocation failed");
		return NULL;
	}

	memcpy(filters_copy, filters, sizeof(*filters) * n);
	filters_copy[n] = NULL;

	if (!(cft = dm_malloc(sizeof(*cft)))) {
		log_error("compsoite filters allocation failed");
		dm_free(filters_copy);
		return NULL;
	}

	cft->passes_filter = _and_p;
	cft->destroy = _composite_destroy;
	cft->private = filters_copy;

	return cft;
}
