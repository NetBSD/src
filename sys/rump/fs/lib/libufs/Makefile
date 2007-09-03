#	$NetBSD: Makefile,v 1.2.2.3 2007/09/03 10:23:53 skrll Exp $
#

.include <bsd.own.mk>

LIB=	ufs

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ufs

SRCS=	ufs_bmap.c ufs_dirhash.c ufs_ihash.c ufs_inode.c ufs_lookup.c	\
	ufs_vfsops.c ufs_vnops.c

CPPFLAGS+=	-DUFS_DIRHASH

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
