/*
 * dmfs-root.c
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

extern struct inode *dmfs_create_lv(struct super_block *sb, int mode,
				    struct dentry *dentry);

static int is_identifier(const char *str, int len)
{
	while (len--) {
		if (!isalnum(*str) && *str != '_')
			return 0;
		str++;
	}
	return 1;
}

static int dmfs_root_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
	struct inode *inode;

	if (dentry->d_name.len >= DM_NAME_LEN)
		return -EINVAL;

	if (!is_identifier(dentry->d_name.name, dentry->d_name.len))
		return -EPERM;

	if (dentry->d_name.name[0] == '.')
		return -EINVAL;

	inode = dmfs_create_lv(dir->i_sb, mode, dentry);
	if (!IS_ERR(inode)) {
		d_instantiate(dentry, inode);
		dget(dentry);
		return 0;
	}

	return PTR_ERR(inode);
}

/*
 * if u.generic_ip is not NULL, then it indicates an inode which
 * represents a table. If it is NULL then the inode is a virtual
 * file and should be deleted along with the directory.
 */
static inline int positive(struct dentry *dentry)
{
	return dentry->d_inode && !d_unhashed(dentry);
}

static int empty(struct dentry *dentry)
{
	struct list_head *list;

	spin_lock(&dcache_lock);
	list = dentry->d_subdirs.next;

	while (list != &dentry->d_subdirs) {
		struct dentry *de = list_entry(list, struct dentry, d_child);

		if (positive(de)) {
			spin_unlock(&dcache_lock);
			return 0;
		}
		list = list->next;
	}
	spin_unlock(&dcache_lock);

	return 1;
}

static int dmfs_root_rmdir(struct inode *dir, struct dentry *dentry)
{
	int ret = -ENOTEMPTY;

	if (empty(dentry)) {
		struct inode *inode = dentry->d_inode;
		ret = dm_destroy(DMFS_I(inode)->md);
		if (ret == 0) {
			DMFS_I(inode)->md = NULL;
			inode->i_nlink--;
			dput(dentry);
		}
	}

	return ret;
}

static struct dentry *dmfs_root_lookup(struct inode *dir, struct dentry *dentry)
{
	d_add(dentry, NULL);
	return NULL;
}

static int dmfs_root_rename(struct inode *old_dir, struct dentry *old_dentry,
			    struct inode *new_dir, struct dentry *new_dentry)
{
	/* Can only rename - not move between directories! */
	if (old_dir != new_dir)
		return -EPERM;

	return -EINVAL;		/* FIXME: a change of LV name here */
}

static int dmfs_root_sync(struct file *file, struct dentry *dentry,
			  int datasync)
{
	return 0;
}

static struct file_operations dmfs_root_file_operations = {
	read:		generic_read_dir,
	readdir:	dcache_readdir,
	fsync:		dmfs_root_sync,
};

static struct inode_operations dmfs_root_inode_operations = {
	lookup:		dmfs_root_lookup,
	mkdir:		dmfs_root_mkdir,
	rmdir:		dmfs_root_rmdir,
	rename:		dmfs_root_rename,
};

struct inode *dmfs_create_root(struct super_block *sb, int mode)
{
	struct inode *inode = dmfs_new_inode(sb, mode | S_IFDIR);

	if (inode) {
		inode->i_fop = &dmfs_root_file_operations;
		inode->i_op = &dmfs_root_inode_operations;
	}

	return inode;
}

