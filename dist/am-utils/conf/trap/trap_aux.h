/*	$NetBSD: trap_aux.h,v 1.1.1.1 2000/06/07 00:52:21 dogcow Exp $ */
/* $srcdir/conf/trap/trap_aux.h */
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	fsmount(type, mnt->mnt_dir, flags, mnt_data)
