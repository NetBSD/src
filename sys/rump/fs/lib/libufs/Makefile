#	$NetBSD: Makefile,v 1.3.2.2 2007/08/20 22:07:22 ad Exp $
#

.include <bsd.own.mk>

LIB=	ufs

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ufs

SRCS=	ufs_bmap.c ufs_dirhash.c ufs_ihash.c ufs_inode.c ufs_lookup.c	\
	ufs_vfsops.c ufs_vnops.c

CPPFLAGS+=	-DUFS_DIRHASH

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
