/*	$NetBSD: trap_dgux.h,v 1.1.1.1.4.2 2008/10/19 22:39:38 haad Exp $	*/

/* $srcdir/conf/trap/trap_dgux.h */
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_dgux(type, mnt->mnt_dir, flags, mnt_data)
