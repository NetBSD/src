#	$NetBSD: Makefile,v 1.1 2009/07/18 16:31:43 reinoud Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/nilfs

LIB=	rumpfs_nilfs

SRCS=	nilfs_subr.c nilfs_vfsops.c nilfs_vnops.c

CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
