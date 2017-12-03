#	$NetBSD: Makefile,v 1.2.20.1 2017/12/03 11:39:11 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../fs/cd9660

LIB=	rumpfs_cd9660
COMMENT=ISO9660

SRCS=	cd9660_bmap.c cd9660_node.c cd9660_util.c cd9660_vnops.c	\
	cd9660_lookup.c cd9660_rrip.c cd9660_vfsops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
