/*	$NetBSD: trap_linux.h,v 1.1.1.2 2000/11/19 23:43:14 wiz Exp $	*/

/* $srcdir/conf/trap/trap_linux.h */
extern int mount_linux(MTYPE_TYPE type, mntent_t *mnt, int flags, caddr_t data);
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_linux(type, mnt, flags, mnt_data)
