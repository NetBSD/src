#	$NetBSD: Makefile,v 1.1.14.2 2007/11/02 12:43:38 joerg Exp $
#

.include <bsd.own.mk>

LIB=	msdosfs

.PATH:  ${NETBSDSRCDIR}/sys/fs/msdosfs

SRCS=	msdosfs_conv.c msdosfs_fat.c msdosfs_vfsops.c msdosfs_denode.c	\
	msdosfs_lookup.c msdosfs_vnops.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
