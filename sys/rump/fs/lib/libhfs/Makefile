#	$NetBSD: Makefile,v 1.1.30.1 2008/09/28 10:41:02 mjf Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/hfs

LIB=	rumpfs_hfs

SRCS=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c unicode.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
