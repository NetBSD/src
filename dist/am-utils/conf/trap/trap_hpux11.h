/*	$NetBSD: trap_hpux11.h,v 1.1.1.1 2000/06/07 00:52:21 dogcow Exp $ */
/* $srcdir/conf/trap/trap_hpux11.h */
extern int mount_hpux11(const char *fsname, const char *dir, int flags, MTYPE_TYPE type, caddr_t data);
#define MOUNT_TRAP(type, mnt, flags, mnt_data) mount_hpux11(mnt->mnt_fsname, mnt->mnt_dir, flags, type, mnt_data)
