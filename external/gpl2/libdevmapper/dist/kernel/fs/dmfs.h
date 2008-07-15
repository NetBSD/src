#ifndef LINUX_DMFS_H
#define LINUX_DMFS_H

struct dmfs_i {
	struct semaphore sem;
	struct mapped_device *md;
	struct list_head errors;
	int status;
};

#define DMFS_I(inode) ((struct dmfs_i *)(inode)->u.generic_ip)

int get_exclusive_write_access(struct inode *inode);

extern struct inode *dmfs_new_inode(struct super_block *sb, int mode);
extern struct inode *dmfs_new_private_inode(struct super_block *sb, int mode);

extern void dmfs_add_error(struct inode *inode, unsigned num, char *str);
extern void dmfs_zap_errors(struct inode *inode);

#endif				/* LINUX_DMFS_H */
