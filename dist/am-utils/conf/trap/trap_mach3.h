/*	$NetBSD: trap_mach3.h,v 1.1.1.4 2001/05/13 17:50:23 veego Exp $	*/

/* $srcdir/conf/trap/trap_mach3.h */
extern int mount_mach3(char *type, char *mnt, int flags, caddr_t mnt_data);
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_mach(type, mnt, flags, mnt_data)
