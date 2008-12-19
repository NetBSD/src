/*	$NetBSD: tags.c,v 1.2 2008/12/19 15:24:07 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2005 Red Hat, Inc. All rights reserved.
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
#include "metadata.h"
#include "import-export.h"
#include "str_list.h"
#include "lvm-string.h"

int print_tags(struct dm_list *tags, char *buffer, size_t size)
{
	struct str_list *sl;
	int first = 1;

	if (!emit_to_buffer(&buffer, &size, "["))
		return_0;

	dm_list_iterate_items(sl, tags) {
		if (!first) {
			if (!emit_to_buffer(&buffer, &size, ", "))
				return_0;
		} else
			first = 0;

		if (!emit_to_buffer(&buffer, &size, "\"%s\"", sl->str))
			return_0;
	}

	if (!emit_to_buffer(&buffer, &size, "]"))
		return_0;

	return 1;
}

int read_tags(struct dm_pool *mem, struct dm_list *tags, struct config_value *cv)
{
	if (cv->type == CFG_EMPTY_ARRAY)
		return 1;

	while (cv) {
		if (cv->type != CFG_STRING) {
			log_error("Found a tag that is not a string");
			return 0;
		}

		if (!str_list_add(mem, tags, dm_pool_strdup(mem, cv->v.str)))
			return_0;

		cv = cv->next;
	}

	return 1;
}
/*	$NetBSD: tags.c,v 1.2 2008/12/19 15:24:07 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2005 Red Hat, Inc. All rights reserved.
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
#include "metadata.h"
#include "import-export.h"
#include "str_list.h"
#include "lvm-string.h"

int print_tags(struct dm_list *tags, char *buffer, size_t size)
{
	struct str_list *sl;
	int first = 1;

	if (!emit_to_buffer(&buffer, &size, "["))
		return_0;

	dm_list_iterate_items(sl, tags) {
		if (!first) {
			if (!emit_to_buffer(&buffer, &size, ", "))
				return_0;
		} else
			first = 0;

		if (!emit_to_buffer(&buffer, &size, "\"%s\"", sl->str))
			return_0;
	}

	if (!emit_to_buffer(&buffer, &size, "]"))
		return_0;

	return 1;
}

int read_tags(struct dm_pool *mem, struct dm_list *tags, struct config_value *cv)
{
	if (cv->type == CFG_EMPTY_ARRAY)
		return 1;

	while (cv) {
		if (cv->type != CFG_STRING) {
			log_error("Found a tag that is not a string");
			return 0;
		}

		if (!str_list_add(mem, tags, dm_pool_strdup(mem, cv->v.str)))
			return_0;

		cv = cv->next;
	}

	return 1;
}
