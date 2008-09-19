/*	$NetBSD: trap_dgux.h,v 1.1.1.1 2008/09/19 20:07:19 christos Exp $	*/

/* $srcdir/conf/trap/trap_dgux.h */
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_dgux(type, mnt->mnt_dir, flags, mnt_data)
