/*	$NetBSD: trap_hcx.h,v 1.1.1.4 2001/05/13 17:50:23 veego Exp $	*/

/* $srcdir/conf/trap/trap_hcx.h */
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) 	mountsyscall(type, mnt->mnt_dir, flags, mnt_data)
