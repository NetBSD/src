#	$NetBSD: Makefile,v 1.3 2015/10/19 16:16:35 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../fs/cd9660

LIB=	rumpfs_cd9660
COMMENT=ISO9660

SRCS=	cd9660_bmap.c cd9660_node.c cd9660_util.c cd9660_vnops.c	\
	cd9660_lookup.c cd9660_rrip.c cd9660_vfsops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
