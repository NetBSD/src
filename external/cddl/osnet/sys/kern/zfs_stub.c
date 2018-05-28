#include <sys/pathname.h>
#include <sys/zfs_context.h>
#include <sys/zfs_ctldir.h>

/* zfs_ctldir.c stub routtines */

void
zfsctl_init(void)
{
}

void
zfsctl_fini(void)
{
}

int
zfsctl_root(zfsvfs_t *zfsvfs, int flags, vnode_t **znode)
{

	return 0;
}

int
zfsctl_root_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
    int flags, vnode_t *rdir, cred_t *cr, caller_context_t *ct,
    int *direntflags, pathname_t *realpnp)
{

	return ENOENT;
}

void
zfsctl_create(zfsvfs_t *zfsvfs)
{
	
	return;
}

void
zfsctl_destroy(zfsvfs_t *zfsvfs)
{

	return;
}

int
zfsctl_lookup_objset(vfs_t *vfsp, uint64_t objsetid, zfsvfs_t **zfsvfsp)
{

	return EINVAL;
}

int
zfsctl_umount_snapshots(vfs_t *vfsp, int fflags, cred_t *cr)
{

	return 0;
}
