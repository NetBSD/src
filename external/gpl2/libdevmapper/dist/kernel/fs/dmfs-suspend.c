/*
 * dmfs-suspend.c
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
	struct dmfs_i *dmi = s->context;

	if (*pos > 0)
		return NULL;

	down(&dmi->sem);

	return (void *)1;
}

static void *s_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;

	return NULL;
}

static void s_stop(struct seq_file *s, void *v)
{
	struct dmfs_i *dmi = s->context;
	up(&dmi->sem);
}

static int s_show(struct seq_file *s, void *v)
{
	struct dmfs_i *dmi = s->context;
	char msg[3] = "1\n";

	if (dmi->md->suspended == 0) {
		msg[0] = '0';
	}

	seq_puts(s, msg);

	return 0;
}

struct seq_operations dmfs_suspend_seq_ops = {
	start:	s_start,
	next:	s_next,
	stop:	s_stop,
	show:	s_show,
};

ssize_t dmfs_suspend_write(struct file *file, const char *buf, size_t count,
			   loff_t * ppos)
{
	struct inode *dir = file->f_dentry->d_parent->d_inode;
	struct dmfs_i *dmi = DMFS_I(dir);
	int written = 0;

	if (count == 0)
		goto out;
	if (count != 1 && count != 2)
		return -EINVAL;
	if (buf[0] != '0' && buf[0] != '1')
		return -EINVAL;

	down(&dmi->sem);
	if (buf[0] == '0') {
		if (get_exclusive_write_access(dir)) {
			written = -EPERM;
			goto out_unlock;
		}
		if (!list_empty(&dmi->errors)) {
			put_write_access(dir);
			written = -EPERM;
			goto out_unlock;
		}
		written = dm_resume(dmi->md);
		put_write_access(dir);
	}
	if (buf[0] == '1')
		written = dm_suspend(dmi->md);
	if (written >= 0)
		written = count;

      out_unlock:
	up(&dmi->sem);

      out:
	return written;
}
