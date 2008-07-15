/*
 * dmfs-lv.c
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

/* Heavily based upon ramfs */

#include "dm.h"
#include "dmfs.h"

#include <linux/seq_file.h>

struct dmfs_inode_info {
	const char *name;
	struct inode *(*create)(struct inode *, int, struct seq_operations *,
				int);
	struct seq_operations *seq_ops;
	int type;
};

#define DMFS_SEQ(inode) ((struct seq_operations *)(inode)->u.generic_ip)

extern struct inode *dmfs_create_table(struct inode *, int,
				       struct seq_operations *, int);
extern struct seq_operations dmfs_error_seq_ops;
extern struct seq_operations dmfs_status_seq_ops;
extern struct seq_operations dmfs_suspend_seq_ops;
extern ssize_t dmfs_suspend_write(struct file *file, const char *buf,
				  size_t size, loff_t * ppos);
extern int dmfs_error_revalidate(struct dentry *dentry);

static int dmfs_seq_open(struct inode *inode, struct file *file)
{
	int ret = seq_open(file, DMFS_SEQ(inode));

	if (ret >= 0) {
		struct seq_file *seq = file->private_data;
		seq->context = DMFS_I(file->f_dentry->d_parent->d_inode);
	}
	return ret;
}

static int dmfs_no_fsync(struct file *file, struct dentry *dentry, int datasync)
{
	return 0;
};

static struct file_operations dmfs_suspend_file_operations = {
	open:		dmfs_seq_open,
	read:		seq_read,
	llseek:		seq_lseek,
	release:	seq_release,
	write:		dmfs_suspend_write,
	fsync:		dmfs_no_fsync,
};

static struct inode_operations dmfs_null_inode_operations = {
};

static struct inode_operations dmfs_error_inode_operations = {
	revalidate:	dmfs_error_revalidate
};

static struct file_operations dmfs_seq_ro_file_operations = {
	open:		dmfs_seq_open,
	read:		seq_read,
	llseek:		seq_lseek,
	release:	seq_release,
	fsync:		dmfs_no_fsync,
};

static struct inode *dmfs_create_seq_ro(struct inode *dir, int mode,
					struct seq_operations *seq_ops, int dev)
{
	struct inode *inode = dmfs_new_inode(dir->i_sb, mode | S_IFREG);
	if (inode) {
		inode->i_fop = &dmfs_seq_ro_file_operations;
		inode->i_op = &dmfs_null_inode_operations;
		DMFS_SEQ(inode) = seq_ops;
	}
	return inode;
}

static struct inode *dmfs_create_error(struct inode *dir, int mode,
					struct seq_operations *seq_ops, int dev)
{
	struct inode *inode = dmfs_new_inode(dir->i_sb, mode | S_IFREG);
	if (inode) {
		inode->i_fop = &dmfs_seq_ro_file_operations;
		inode->i_op = &dmfs_error_inode_operations;
		DMFS_SEQ(inode) = seq_ops;
	}
	return inode;
}

static struct inode *dmfs_create_device(struct inode *dir, int mode,
					struct seq_operations *seq_ops, int dev)
{
	struct inode *inode = dmfs_new_inode(dir->i_sb, mode | S_IFBLK);
	if (inode) {
		init_special_inode(inode, mode | S_IFBLK, dev);
	}
	return inode;
}

static struct inode *dmfs_create_suspend(struct inode *dir, int mode,
					 struct seq_operations *seq_ops,
					 int dev)
{
	struct inode *inode = dmfs_create_seq_ro(dir, mode, seq_ops, dev);
	if (inode) {
		inode->i_fop = &dmfs_suspend_file_operations;
	}
	return inode;
}

static int dmfs_lv_unlink(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;

	inode->i_mapping = &inode->i_data;
	inode->i_nlink--;
	return 0;
}

static struct dmfs_inode_info dmfs_ii[] = {
	{".", NULL, NULL, DT_DIR},
	{"..", NULL, NULL, DT_DIR},
	{"table", dmfs_create_table, NULL, DT_REG},
	{"error", dmfs_create_error, &dmfs_error_seq_ops, DT_REG},
	{"status", dmfs_create_seq_ro, &dmfs_status_seq_ops, DT_REG},
	{"device", dmfs_create_device, NULL, DT_BLK},
	{"suspend", dmfs_create_suspend, &dmfs_suspend_seq_ops, DT_REG},
};

#define NR_DMFS_II (sizeof(dmfs_ii)/sizeof(struct dmfs_inode_info))

static struct dmfs_inode_info *dmfs_find_by_name(const char *n, int len)
{
	int i;

	for (i = 2; i < NR_DMFS_II; i++) {
		if (strlen(dmfs_ii[i].name) != len)
			continue;
		if (memcmp(dmfs_ii[i].name, n, len) == 0)
			return &dmfs_ii[i];
	}
	return NULL;
}

static struct dentry *dmfs_lv_lookup(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = NULL;
	struct dmfs_inode_info *ii;

	ii = dmfs_find_by_name(dentry->d_name.name, dentry->d_name.len);
	if (ii) {
		int dev = kdev_t_to_nr(DMFS_I(dir)->md->dev);
		inode = ii->create(dir, 0600, ii->seq_ops, dev);
	}

	d_add(dentry, inode);
	return NULL;
}

static int dmfs_inum(int entry, struct dentry *dentry)
{
	if (entry == 0)
		return dentry->d_inode->i_ino;
	if (entry == 1)
		return dentry->d_parent->d_inode->i_ino;

	return entry;
}

static int dmfs_lv_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
	struct dentry *dentry = filp->f_dentry;
	struct dmfs_inode_info *ii;

	while (filp->f_pos < NR_DMFS_II) {
		ii = &dmfs_ii[filp->f_pos];
		if (filldir(dirent, ii->name, strlen(ii->name), filp->f_pos,
			    dmfs_inum(filp->f_pos, dentry), ii->type) < 0)
			break;
		filp->f_pos++;
	}

	return 0;
}

static int dmfs_lv_sync(struct file *file, struct dentry *dentry, int datasync)
{
	return 0;
}

static struct file_operations dmfs_lv_file_operations = {
	read:		generic_read_dir,
	readdir:	dmfs_lv_readdir,
	fsync:		dmfs_lv_sync,
};

static struct inode_operations dmfs_lv_inode_operations = {
	lookup:		dmfs_lv_lookup,
	unlink:		dmfs_lv_unlink,
};

struct inode *dmfs_create_lv(struct super_block *sb, int mode,
			     struct dentry *dentry)
{
	struct inode *inode = dmfs_new_private_inode(sb, mode | S_IFDIR);
	struct mapped_device *md;
	const char *name = dentry->d_name.name;
	char tmp_name[DM_NAME_LEN + 1];
	struct dm_table *table;
	int ret = -ENOMEM;

	if (inode) {
		ret = dm_table_create(&table);
		if (!ret) {
			ret = dm_table_complete(table);
			if (!ret) {
				inode->i_fop = &dmfs_lv_file_operations;
				inode->i_op = &dmfs_lv_inode_operations;
				memcpy(tmp_name, name, dentry->d_name.len);
				tmp_name[dentry->d_name.len] = 0;
				ret = dm_create(tmp_name, -1, table, &md);
				if (!ret) {
					DMFS_I(inode)->md = md;
					md->suspended = 1;
					return inode;
				}
			}
			dm_table_destroy(table);
		}
		iput(inode);
	}

	return ERR_PTR(ret);
}
