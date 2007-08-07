#	$NetBSD: Makefile,v 1.2 2007/08/07 10:16:58 pooka Exp $
#

.include <bsd.own.mk>

LIB=	ufs

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ufs

SRCS=	ufs_bmap.c ufs_ihash.c ufs_inode.c ufs_lookup.c	ufs_vfsops.c	\
	ufs_vnops.c
#	ufs_dirhash.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
