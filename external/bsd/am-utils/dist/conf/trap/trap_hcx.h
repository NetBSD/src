/*	$NetBSD: trap_hcx.h,v 1.1.1.1 2008/09/19 20:07:19 christos Exp $	*/

/* $srcdir/conf/trap/trap_hcx.h */
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) 	mountsyscall(type, mnt->mnt_dir, flags, mnt_data)
