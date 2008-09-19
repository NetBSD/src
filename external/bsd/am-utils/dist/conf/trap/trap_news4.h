/*	$NetBSD: trap_news4.h,v 1.1.1.1 2008/09/19 20:07:19 christos Exp $	*/

/* $srcdir/conf/trap/trap_news4.h */
#define MOUNT_TRAP(type, mnt, flags, mnt_data)         mount(type, mnt->mnt_dir, M_NEWTYPE | flags, mnt_data)
