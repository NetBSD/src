/*	$NetBSD: hn_dref_linux.h,v 1.1.1.1 2000/06/07 00:52:20 dogcow Exp $ */
/* $srcdir/conf/hn_dref/hn_dref_linux.h */
#define NFS_HN_DREF(dst, src) strncpy((dst), (src), MAXHOSTNAMELEN)
