#	$NetBSD: Makefile,v 1.1.38.1 2008/07/31 04:51:04 simonb Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/hfs

LIB=	rumpfs_hfs

SRCS=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c unicode.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
