/*	$NetBSD: trap_ultrix.h,v 1.1.1.4 2001/05/13 17:50:23 veego Exp $	*/

/* $srcdir/conf/trap/trap_ultrix.h */
/* arg 3 to mount(2) is rwflag */
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount(mnt->mnt_fsname, mnt->mnt_dir, flags & MNT2_GEN_OPT_RONLY, type, mnt_data)
