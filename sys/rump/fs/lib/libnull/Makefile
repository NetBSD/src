#	$NetBSD: Makefile,v 1.2.32.1 2015/12/27 12:10:14 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../miscfs/nullfs

LIB=	rumpfs_null
COMMENT=Loopback file system

SRCS=	null_vfsops.c null_vnops.c

CWARNFLAGS.clang+=	-Wno-conversion

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
