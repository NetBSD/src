/*	$NetBSD: trap_news4.h,v 1.1.1.1.4.2 2000/06/07 00:52:22 dogcow Exp $ */
/* $srcdir/conf/trap/trap_news4.h */
#define MOUNT_TRAP(type, mnt, flags, mnt_data)         mount(type, mnt->mnt_dir, M_NEWTYPE | flags, mnt_data)
