#	$NetBSD: Makefile,v 1.3 2015/10/19 16:16:36 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../miscfs/nullfs

LIB=	rumpfs_null
COMMENT=Loopback file system

SRCS=	null_vfsops.c null_vnops.c

CWARNFLAGS.clang+=	-Wno-conversion

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
