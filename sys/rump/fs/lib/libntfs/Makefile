#	$NetBSD: Makefile,v 1.3.2.2 2007/08/20 22:07:20 ad Exp $
#

.include <bsd.own.mk>

LIB=	ntfs

.PATH:  ${NETBSDSRCDIR}/sys/fs/ntfs

SRCS=	ntfs_compr.c ntfs_conv.c ntfs_ihash.c ntfs_subr.c		\
	ntfs_vfsops.c ntfs_vnops.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
