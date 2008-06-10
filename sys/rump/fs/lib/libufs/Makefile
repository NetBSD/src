#	$NetBSD: Makefile,v 1.4.28.1 2008/06/10 14:51:22 simonb Exp $
#

.include <bsd.own.mk>

LIB=	ufs

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ufs

SRCS=	ufs_bmap.c ufs_dirhash.c ufs_ihash.c ufs_inode.c ufs_lookup.c	\
	ufs_vfsops.c ufs_vnops.c ufs_wapbl.c

CPPFLAGS+=	-DUFS_DIRHASH -DFFS_EI -DWAPBL

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
