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

vnode_t *
zfsctl_root(znode_t *znode)
{

	return NULL;
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

/* zfs_acl.c */
#if 0
/*
 * Convert unix access mask to v4 access mask
 */
static uint32_t
zfs_unix_to_v4(uint32_t access_mask)
{
	uint32_t new_mask = 0;

/*	if (access_mask & S_IXOTH)
		new_mask |= ACE_EXECUTE;
	if (access_mask & S_IWOTH)
		new_mask |= ACE_WRITE_DATA;
	if (access_mask & S_IROTH)
	new_mask |= ACE_READ_DATA;*/
	return (new_mask);
}

int
zfs_zaccess_rename(znode_t *sdzp, znode_t *szp, znode_t *tdzp,
    znode_t *tzp, cred_t *cr)
{

	return 0;
}

int
zfs_setacl(znode_t *zp, vsecattr_t *vsecp, boolean_t skipaclchk, cred_t *cr)
{

	return ENOTSUP;
}

int
zfs_getacl(znode_t *zp, vsecattr_t *vsecp, boolean_t skipaclchk, cred_t *cr)
{

	return ENOTSUP;
}

int
zfs_aclset_common(znode_t *zp, zfs_acl_t *aclp, cred_t *cr,
    zfs_fuid_info_t **fuidp, dmu_tx_t *tx)
{

	return 0;
}

int
zfs_zaccess_rwx(znode_t *zp, mode_t mode, int flags, cred_t *cr)
{
	return (zfs_zaccess(zp, zfs_unix_to_v4(mode >> 6), flags, B_FALSE, cr));
}

int
zfs_zaccess_unix(znode_t *zp, mode_t mode, cred_t *cr)
{
	int v4_mode = zfs_unix_to_v4((uint32_t)mode >> 6);

	return (zfs_zaccess(zp, v4_mode, 0, B_FALSE, cr));
}

int
zfs_zaccess_delete(znode_t *dzp, znode_t *zp, cred_t *cr)
{

	return 0;
}

void
zfs_perm_init(znode_t *zp, znode_t *parent, int flag,
    vattr_t *vap, dmu_tx_t *tx, cred_t *cr,
    zfs_acl_t *setaclp, zfs_fuid_info_t **fuidp)
{
		
	return;
}

void
zfs_acl_free(zfs_acl_t *aclp)
{
	
	kmem_free(aclp, sizeof(zfs_acl_t));
	return;
}

int
zfs_zaccess(znode_t *zp, int mode, int flags, boolean_t skipaclchk, cred_t *cr)
{
	
	/* return ok or EACCES here ? */
	return 0;
}

int
zfs_vsec_2_aclp(zfsvfs_t *zfsvfs, vtype_t obj_type,
    vsecattr_t *vsecp, zfs_acl_t **zaclp)
{
	zfs_acl_t *aclp;
	*zaclp = NULL;

	aclp = kmem_zalloc(sizeof (zfs_acl_t), KM_SLEEP);

	*zaclp = aclp;

	return 0;
}

int
zfs_acl_chmod_setattr(znode_t *zp, zfs_acl_t **aclp, uint64_t mode)
{
	int error;
	error = 0;

	*aclp = kmem_zalloc(sizeof (zfs_acl_t), KM_SLEEP);;

	return (error);
}
#endif /* 0 */
