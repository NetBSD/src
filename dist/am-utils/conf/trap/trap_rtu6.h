/*	$NetBSD: trap_rtu6.h,v 1.1.1.4 2001/05/13 17:50:23 veego Exp $	*/

/* $srcdir/conf/trap/trap_rtu6.h */
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) 	vmount(type, mnt->mnt_dir, flags, mnt_data)
