/*	$NetBSD: trap_mach3.h,v 1.1.1.1.4.2 2000/06/07 00:52:22 dogcow Exp $ */
/* $srcdir/conf/trap/trap_mach3.h */
extern int mount_mach3(char *type, char *mnt, int flags, caddr_t mnt_data);
#define MOUNT_TRAP(type, mnt, flags, mnt_data) 	mount_mach(type, mnt, flags, mnt_data)
