/*	$NetBSD: pool_label.c,v 1.1.1.1 2008/12/22 00:17:51 haad Exp $	*/

/*
 * Copyright (C) 1997-2004 Sistina Software, Inc. All rights reserved.
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
#include "label.h"
#include "metadata.h"
#include "xlate.h"
#include "disk_rep.h"
#include "pool_label.h"

#include <sys/stat.h>
#include <fcntl.h>

static void _pool_not_supported(const char *op)
{
	log_error("The '%s' operation is not supported for the pool labeller.",
		  op);
}

static int _pool_can_handle(struct labeller *l __attribute((unused)), void *buf, uint64_t sector)
{

	struct pool_disk pd;

	/*
	 * POOL label must always be in first sector
	 */
	if (sector)
		return 0;

	pool_label_in(&pd, buf);

	/* can ignore 8 rightmost bits for ondisk format check */
	if ((pd.pl_magic == POOL_MAGIC) &&
	    (pd.pl_version >> 8 == POOL_VERSION >> 8))
		return 1;

	return 0;
}

static int _pool_write(struct label *label __attribute((unused)), void *buf __attribute((unused)))
{
	_pool_not_supported("write");
	return 0;
}

static int _pool_read(struct labeller *l, struct device *dev, void *buf,
		 struct label **label)
{
	struct pool_list pl;

	return read_pool_label(&pl, l, dev, buf, label);
}

static int _pool_initialise_label(struct labeller *l __attribute((unused)), struct label *label)
{
	strcpy(label->type, "POOL");

	return 1;
}

static void _pool_destroy_label(struct labeller *l __attribute((unused)), struct label *label __attribute((unused)))
{
	return;
}

static void _label_pool_destroy(struct labeller *l)
{
	dm_free(l);
}

struct label_ops _pool_ops = {
      .can_handle = _pool_can_handle,
      .write = _pool_write,
      .read = _pool_read,
      .verify = _pool_can_handle,
      .initialise_label = _pool_initialise_label,
      .destroy_label = _pool_destroy_label,
      .destroy = _label_pool_destroy,
};

struct labeller *pool_labeller_create(struct format_type *fmt)
{
	struct labeller *l;

	if (!(l = dm_malloc(sizeof(*l)))) {
		log_error("Couldn't allocate labeller object.");
		return NULL;
	}

	l->ops = &_pool_ops;
	l->private = (const void *) fmt;

	return l;
}
