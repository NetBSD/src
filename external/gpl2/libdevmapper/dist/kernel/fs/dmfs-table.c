/*
 * dmfs-table.c
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

#include <linux/mm.h>

static offset_t start_of_next_range(struct dm_table *t)
{
	offset_t n = 0;

	if (t->num_targets) {
		n = t->highs[t->num_targets - 1] + 1;
	}
	
	return n;
}

static char *dmfs_parse_line(struct dm_table *t, char *str)
{
	offset_t start, size, high;
	void *context;
	struct target_type *ttype;
	int rv = 0;
	char *msg;
	int pos = 0;
	char target[33];
	char *argv[MAX_ARGS];
	int argc;

	static char *err_table[] = {
		"Missing/Invalid start argument",
		"Missing/Invalid size argument",
		"Missing target type"
	};

	rv = sscanf(str, "%d %d %32s%n", &start, &size, target, &pos);
	if (rv < 3) {
		msg = err_table[rv];
		goto out;
	}
	str += pos;
	while (*str && isspace(*str))
		str++;

	msg = "Gap in table";
	if (start != start_of_next_range(t))
		goto out;

	msg = "Target type unknown";
	ttype = dm_get_target_type(target);
	if (ttype) {
		msg = "Too many arguments";
		rv = split_args(MAX_ARGS, &argc, argv, str);
		if (rv < 0)
			goto out;

		msg = "This message should never appear (constructor error)";
		rv = ttype->ctr(t, start, size, argc, argv, &context);
		msg = context;
		if (rv == 0) {
			msg = "Error adding target to table";
			high = start + (size - 1);
			if (dm_table_add_target(t, high, ttype, context) == 0)
				return NULL;
			ttype->dtr(t, context);
		}
		dm_put_target_type(ttype);
	}

      out:
	return msg;
}

static int dmfs_copy(char *dst, int dstlen, char *src, int srclen, int *flag)
{
	int len = min(dstlen, srclen);
	char *start = dst;

	while (len) {
		*dst = *src++;
		if (*dst == '\n')
			goto end_of_line;
		dst++;
		len--;
	}
      out:
	return (dst - start);
      end_of_line:
	dst++;
	*flag = 1;
	goto out;
}

static int dmfs_line_is_not_comment(char *str)
{
	while (*str) {
		if (*str == '#')
			break;
		if (!isspace(*str))
			return 1;
		str++;
	}
	return 0;
}

struct dmfs_desc {
	struct dm_table *table;
	struct inode *inode;
	char *tmp;
	loff_t tmpl;
	unsigned long lnum;
};

static int dmfs_read_actor(read_descriptor_t *desc, struct page *page,
			   unsigned long offset, unsigned long size)
{
	char *buf, *msg;
	unsigned long count = desc->count, len, copied;
	struct dmfs_desc *d = (struct dmfs_desc *) desc->buf;

	if (size > count)
		size = count;

	len = size;
	buf = kmap(page);
	do {
		int flag = 0;
		copied = dmfs_copy(d->tmp + d->tmpl, PAGE_SIZE - d->tmpl - 1,
				   buf + offset, len, &flag);
		offset += copied;
		len -= copied;
		if (d->tmpl + copied == PAGE_SIZE - 1)
			goto line_too_long;
		d->tmpl += copied;
		if (flag || (len == 0 && count == size)) {
			*(d->tmp + d->tmpl) = 0;
			if (dmfs_line_is_not_comment(d->tmp)) {
				msg = dmfs_parse_line(d->table, d->tmp);
				if (msg) {
					dmfs_add_error(d->inode, d->lnum, msg);
				}
			}
			d->lnum++;
			d->tmpl = 0;
		}
	} while (len > 0);
	kunmap(page);

	desc->count = count - size;
	desc->written += size;

	return size;

      line_too_long:
	printk(KERN_INFO "dmfs_read_actor: Line %lu too long\n", d->lnum);
	kunmap(page);
	return 0;
}

static struct dm_table *dmfs_parse(struct inode *inode, struct file *filp)
{
	struct dm_table *t = NULL;
	unsigned long page;
	struct dmfs_desc d;
	loff_t pos = 0;
	int r;

	if (inode->i_size == 0)
		return NULL;

	page = __get_free_page(GFP_NOFS);
	if (page) {
		r = dm_table_create(&t);
		if (!r) {
			read_descriptor_t desc;

			desc.written = 0;
			desc.count = inode->i_size;
			desc.buf = (char *) &d;
			d.table = t;
			d.inode = inode;
			d.tmp = (char *) page;
			d.tmpl = 0;
			d.lnum = 1;

			do_generic_file_read(filp, &pos, &desc,
					     dmfs_read_actor);
			if (desc.written != inode->i_size) {
				dm_table_destroy(t);
				t = NULL;
			} 
			if (!t || (t && !t->num_targets))
				dmfs_add_error(d.inode, 0, 
					       "No valid targets found");
		}
		free_page(page);
	}

	if (!list_empty(&DMFS_I(inode)->errors)) {
		dm_table_destroy(t);
		t = NULL;
	}

	return t;
}

static int dmfs_table_release(struct inode *inode, struct file *f)
{
	struct dentry *dentry = f->f_dentry;
	struct inode *parent = dentry->d_parent->d_inode;
	struct dmfs_i *dmi = DMFS_I(parent);
	struct dm_table *table;

	if (f->f_mode & FMODE_WRITE) {
		down(&dmi->sem);
		dmfs_zap_errors(dentry->d_parent->d_inode);
		table = dmfs_parse(dentry->d_parent->d_inode, f);

		if (table) {
			struct mapped_device *md = dmi->md;
			int need_resume = 0;

			if (md->suspended == 0) {
				dm_suspend(md);
				need_resume = 1;
			}
			dm_swap_table(md, table);
			if (need_resume) {
				dm_resume(md);
			}
		}
		up(&dmi->sem);

		put_write_access(parent);
	}

	return 0;
}

static int dmfs_readpage(struct file *file, struct page *page)
{
	if (!Page_Uptodate(page)) {
		memset(kmap(page), 0, PAGE_CACHE_SIZE);
		kunmap(page);
		flush_dcache_page(page);
		SetPageUptodate(page);
	}

	UnlockPage(page);
	return 0;
}

static int dmfs_prepare_write(struct file *file, struct page *page,
			      unsigned offset, unsigned to)
{
	void *addr = kmap(page);

	if (!Page_Uptodate(page)) {
		memset(addr, 0, PAGE_CACHE_SIZE);
		flush_dcache_page(page);
		SetPageUptodate(page);
	}

	SetPageDirty(page);
	return 0;
}

static int dmfs_commit_write(struct file *file, struct page *page,
			     unsigned offset, unsigned to)
{
	struct inode *inode = page->mapping->host;
	loff_t pos = ((loff_t) page->index << PAGE_CACHE_SHIFT) + to;

	kunmap(page);
	if (pos > inode->i_size)
		inode->i_size = pos;

	return 0;
}

/*
 * There is a small race here in that two processes might call this at
 * the same time and both fail. So its a fail safe race :-) This should
 * move into namei.c (and thus use the spinlock and do this properly)
 * at some stage if we continue to use this set of functions for ensuring
 * exclusive write access to the file
 */
int get_exclusive_write_access(struct inode *inode)
{
	if (get_write_access(inode))
		return -1;
	if (atomic_read(&inode->i_writecount) != 1) {
		put_write_access(inode);
		return -1;
	}
	return 0;
}

static int dmfs_table_open(struct inode *inode, struct file *file)
{
	struct dentry *dentry = file->f_dentry;
	struct inode *parent = dentry->d_parent->d_inode;
	struct dmfs_i *dmi = DMFS_I(parent);

	if (file->f_mode & FMODE_WRITE) {
		if (get_exclusive_write_access(parent))
			return -EPERM;

		if (!dmi->md->suspended) {
			put_write_access(parent);
			return -EPERM;
		}
	}

	return 0;
}

static int dmfs_table_sync(struct file *file, struct dentry *dentry,
			   int datasync)
{
	return 0;
}

static int dmfs_table_revalidate(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	struct inode *parent = dentry->d_parent->d_inode;

	inode->i_size = parent->i_size;

	return 0;
}

struct address_space_operations dmfs_address_space_operations = {
	readpage:	dmfs_readpage,
	writepage:	fail_writepage,
	prepare_write:	dmfs_prepare_write,
	commit_write:	dmfs_commit_write
};

static struct file_operations dmfs_table_file_operations = {
	llseek:		generic_file_llseek,
	read:		generic_file_read,
	write:		generic_file_write,
	open:		dmfs_table_open,
	release:	dmfs_table_release,
	fsync:		dmfs_table_sync
};

static struct inode_operations dmfs_table_inode_operations = {
	revalidate:	dmfs_table_revalidate
};

struct inode *dmfs_create_table(struct inode *dir, int mode)
{
	struct inode *inode = dmfs_new_inode(dir->i_sb, mode | S_IFREG);

	if (inode) {
		inode->i_mapping = dir->i_mapping;
		inode->i_mapping->a_ops = &dmfs_address_space_operations;
		inode->i_fop = &dmfs_table_file_operations;
		inode->i_op = &dmfs_table_inode_operations;
	}

	return inode;
}
