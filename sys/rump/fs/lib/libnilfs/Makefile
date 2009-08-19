#	$NetBSD: Makefile,v 1.1.4.2 2009/08/19 18:48:27 yamt Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/nilfs

LIB=	rumpfs_nilfs

SRCS=	nilfs_subr.c nilfs_vfsops.c nilfs_vnops.c

CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
