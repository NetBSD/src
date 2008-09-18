#	$NetBSD: Makefile,v 1.1.36.1 2008/09/18 04:37:03 wrstuden Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/hfs

LIB=	rumpfs_hfs

SRCS=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c unicode.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
