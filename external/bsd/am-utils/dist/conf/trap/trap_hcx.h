/*	$NetBSD: trap_hcx.h,v 1.1.1.1.4.2 2008/10/19 22:39:38 haad Exp $	*/

/* $srcdir/conf/trap/trap_hcx.h */
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) 	mountsyscall(type, mnt->mnt_dir, flags, mnt_data)
