/*	$NetBSD: trap_dgux.h,v 1.1.1.2 2000/11/19 23:43:14 wiz Exp $	*/

/* $srcdir/conf/trap/trap_dgux.h */
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_dgux(type, mnt->mnt_dir, flags, mnt_data)
