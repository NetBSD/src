/*	$NetBSD: trap_ultrix.h,v 1.1.1.1.4.2 2000/06/07 00:52:22 dogcow Exp $ */
/* $srcdir/conf/trap/trap_ultrix.h */
/* arg 3 to mount(2) is rwflag */
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount(mnt->mnt_fsname, mnt->mnt_dir, flags & MNT2_GEN_OPT_RONLY, type, mnt_data)
