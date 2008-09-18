/*
 * dmfs-status.c
 *
 * Copyright (C) 2001 Sistina Software
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU CC; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "dm.h"
#include "dmfs.h"

#include <linux/seq_file.h>

static void *s_start(struct seq_file *s, loff_t *pos)
{
	return NULL;
}

static void *s_next(struct seq_file *s, void *v, loff_t *pos)
{
	return NULL;
}

static void s_stop(struct seq_file *s, void *v)
{
	return;
}

static int s_show(struct seq_file *s, void *v)
{
	return 0;
}

struct seq_operations dmfs_status_seq_ops = {
	start:	s_start,
	next:	s_next,
	stop:	s_stop,
	show:	s_show,
};
