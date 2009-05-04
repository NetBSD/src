#	$NetBSD: Makefile,v 1.1.34.1 2009/05/04 08:14:26 yamt Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/hfs

LIB=	rumpfs_hfs

SRCS=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c unicode.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
