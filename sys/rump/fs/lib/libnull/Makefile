#	$NetBSD: Makefile,v 1.1.2.1 2011/06/06 09:10:05 jruoho Exp $
#

.PATH:  ${.CURDIR}/../../../../miscfs/nullfs

LIB=	rumpfs_null

SRCS=	null_vfsops.c null_vnops.c

CWARNFLAGS.clang+=	-Wno-conversion

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
