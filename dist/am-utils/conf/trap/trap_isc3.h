/*	$NetBSD: trap_isc3.h,v 1.1.1.2 2000/11/19 23:43:14 wiz Exp $	*/

/* $srcdir/conf/trap/trap_isc3.h */
extern int mount_isc3(char *fsname, char *dir, int flags, int type, void *data);
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_isc3(mnt->mnt_fsname, mnt->mnt_dir, flags, type, mnt_data)
