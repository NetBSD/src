/*	$NetBSD: trap_aux.h,v 1.1.1.4 2001/05/13 17:50:23 veego Exp $	*/

/* $srcdir/conf/trap/trap_aux.h */
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	fsmount(type, mnt->mnt_dir, flags, mnt_data)
