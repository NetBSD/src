#	$NetBSD: Makefile,v 1.1.2.2 2009/07/23 23:32:53 jym Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/nilfs

LIB=	rumpfs_nilfs

SRCS=	nilfs_subr.c nilfs_vfsops.c nilfs_vnops.c

CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
