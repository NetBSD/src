/*	$NetBSD: trap_linux.h,v 1.1.1.1 2000/06/07 00:52:21 dogcow Exp $ */
/* $srcdir/conf/trap/trap_linux.h */
extern int mount_linux(MTYPE_TYPE type, mntent_t *mnt, int flags, caddr_t data);
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_linux(type, mnt, flags, mnt_data)
