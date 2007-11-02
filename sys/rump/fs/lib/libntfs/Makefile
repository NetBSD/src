#	$NetBSD: Makefile,v 1.3.12.2 2007/11/02 12:43:39 joerg Exp $
#

.include <bsd.own.mk>

LIB=	ntfs

.PATH:  ${NETBSDSRCDIR}/sys/fs/ntfs

SRCS=	ntfs_compr.c ntfs_conv.c ntfs_ihash.c ntfs_subr.c		\
	ntfs_vfsops.c ntfs_vnops.c

CFLAGS+=-Wno-pointer-sign -D__NetBSD__

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
