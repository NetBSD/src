#	$NetBSD: Makefile,v 1.4.28.2 2008/07/31 04:51:04 simonb Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/ufs

LIB=	rumpfs_ufs

SRCS=	ufs_bmap.c ufs_dirhash.c ufs_ihash.c ufs_inode.c ufs_lookup.c	\
	ufs_vfsops.c ufs_vnops.c ufs_wapbl.c

CPPFLAGS+=	-DUFS_DIRHASH -DFFS_EI -DWAPBL

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
