/*	$NetBSD: trap_stellix.h,v 1.1.1.1.4.2 2000/06/07 00:52:22 dogcow Exp $ */
/* $srcdir/conf/trap/trap_stellix.h */
extern int mount_stellix(char *fsname, char *dir, int flags, int type, void *data);
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_stellix(mnt->mnt_fsname, mnt->mnt_dir, flags, type, mnt_data)
