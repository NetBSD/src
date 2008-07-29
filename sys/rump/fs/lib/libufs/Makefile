#	$NetBSD: Makefile,v 1.5 2008/07/29 13:17:47 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/ufs

LIB=	rumpfs_ufs

SRCS=	ufs_bmap.c ufs_dirhash.c ufs_ihash.c ufs_inode.c ufs_lookup.c	\
	ufs_vfsops.c ufs_vnops.c

CPPFLAGS+=	-DUFS_DIRHASH -DFFS_EI

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
