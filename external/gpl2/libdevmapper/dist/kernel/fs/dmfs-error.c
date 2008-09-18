/*
 * dmfs-error.c
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

#include <linux/list.h>
#include <linux/seq_file.h>

struct dmfs_error {
	struct list_head list;
	unsigned len;
	char *msg;
};

static struct dmfs_error oom_error;

static struct list_head oom_list = {
	next:	&oom_error.list,
	prev:	&oom_error.list,
};

static struct dmfs_error oom_error = {
	list:	{next: &oom_list, prev:&oom_list},
	len:	39,
	msg:	"Out of memory during creation of table\n",
};

int dmfs_error_revalidate(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	struct inode *parent = dentry->d_parent->d_inode;

	if (!list_empty(&DMFS_I(parent)->errors))
		inode->i_size = 1;
	else
		inode->i_size = 0;

	return 0;
}

void dmfs_add_error(struct inode *inode, unsigned num, char *str)
{
	struct dmfs_i *dmi = DMFS_I(inode);
	int len = strlen(str) + sizeof(struct dmfs_error) + 12;
	struct dmfs_error *e = kmalloc(len, GFP_KERNEL);

	if (e) {
		e->msg = (char *)(e + 1);
		e->len = sprintf(e->msg, "%8u: %s\n", num, str);
		list_add(&e->list, &dmi->errors);
	}
}

void dmfs_zap_errors(struct inode *inode)
{
	struct dmfs_i *dmi = DMFS_I(inode);
	struct dmfs_error *e;

	while (!list_empty(&dmi->errors)) {
		e = list_entry(dmi->errors.next, struct dmfs_error, list);
		list_del(&e->list);
		kfree(e);
	}
}

static void *e_start(struct seq_file *e, loff_t *pos)
{
	struct list_head *p;
	loff_t n = *pos;
	struct dmfs_i *dmi = e->context;

	down(&dmi->sem);

	if (dmi->status) {
		list_for_each(p, &oom_list)
			if (n-- == 0)
				return list_entry(p, struct dmfs_error, list);
	} else {
		list_for_each(p, &dmi->errors)
			if (n-- == 0)
				return list_entry(p, struct dmfs_error, list);
	}

	return NULL;
}

static void *e_next(struct seq_file *e, void *v, loff_t *pos)
{
	struct dmfs_i *dmi = e->context;
	struct list_head *p = ((struct dmfs_error *)v)->list.next;

	(*pos)++;

	return (p == &dmi->errors) || 
	       (p == &oom_list) ? NULL : list_entry(p, struct dmfs_error, list);
}

static void e_stop(struct seq_file *e, void *v)
{
	struct dmfs_i *dmi = e->context;
	up(&dmi->sem);
}

static int show_error(struct seq_file *e, void *v)
{
	struct dmfs_error *d = v;
	seq_puts(e, d->msg);

	return 0;
}

struct seq_operations dmfs_error_seq_ops = {
	start:	e_start,
	next:	e_next,
	stop:	e_stop,
	show:	show_error,
};
