/*	$NetBSD: trap_isc3.h,v 1.1.1.4 2001/05/13 17:50:23 veego Exp $	*/

/* $srcdir/conf/trap/trap_isc3.h */
extern int mount_isc3(char *fsname, char *dir, int flags, int type, void *data);
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_isc3(mnt->mnt_fsname, mnt->mnt_dir, flags, type, mnt_data)
