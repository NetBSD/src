/*	$NetBSD: trap_rtu6.h,v 1.1.1.2 2000/11/19 23:43:14 wiz Exp $	*/

/* $srcdir/conf/trap/trap_rtu6.h */
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) 	vmount(type, mnt->mnt_dir, flags, mnt_data)
