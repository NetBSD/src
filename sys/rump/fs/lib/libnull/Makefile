#	$NetBSD: Makefile,v 1.1.6.3 2011/05/31 03:05:11 rmind Exp $
#

.PATH:  ${.CURDIR}/../../../../miscfs/nullfs

LIB=	rumpfs_null

SRCS=	null_vfsops.c null_vnops.c

CWARNFLAGS.clang+=	-Wno-conversion

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
