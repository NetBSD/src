/*	$NetBSD: trap_mach3.h,v 1.1.1.1.4.2 2008/10/19 22:39:39 haad Exp $	*/

/* $srcdir/conf/trap/trap_mach3.h */
extern int mount_mach3(char *type, char *mnt, int flags, caddr_t mnt_data);
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_mach(type, mnt, flags, mnt_data)
