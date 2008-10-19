/*	$NetBSD: trap_irix.h,v 1.1.1.1.4.2 2008/10/19 22:39:38 haad Exp $	*/

/* $srcdir/conf/trap/trap_irix.h */
extern int mount_irix(char *fsname, char *dir, int flags, MTYPE_TYPE type, voidp data);
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_irix(mnt->mnt_fsname, mnt->mnt_dir, flags, type, mnt_data)
