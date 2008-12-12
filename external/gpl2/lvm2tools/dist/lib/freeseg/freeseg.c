/*	$NetBSD: freeseg.c,v 1.1.1.1.2.2 2008/12/12 16:33:00 haad Exp $	*/

/*
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
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
#include "toolcontext.h"
#include "segtype.h"
#include "display.h"
#include "text_export.h"
#include "text_import.h"
#include "config.h"
#include "str_list.h"
#include "targets.h"
#include "lvm-string.h"
#include "activate.h"
#include "str_list.h"
#include "metadata.h"

static const char *_freeseg_name(const struct lv_segment *seg)
{
	return seg->segtype->name;
}

static void _freeseg_destroy(const struct segment_type *segtype)
{
	dm_free((void *)segtype);
}

static struct segtype_handler _freeseg_ops = {
	.name = _freeseg_name,
	.destroy = _freeseg_destroy,
};

struct segment_type *init_free_segtype(struct cmd_context *cmd)
{
	struct segment_type *segtype = dm_malloc(sizeof(*segtype));

	if (!segtype)
		return_NULL;

	segtype->cmd = cmd;
	segtype->ops = &_freeseg_ops;
	segtype->name = "free";
	segtype->private = NULL;
	segtype->flags = SEG_VIRTUAL | SEG_CANNOT_BE_ZEROED;

	log_very_verbose("Initialised segtype: %s", segtype->name);

	return segtype;
}
