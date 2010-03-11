#	$NetBSD: Makefile,v 1.1.18.3 2010/03/11 15:04:35 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../fs/cd9660

LIB=	rumpfs_cd9660

SRCS=	cd9660_bmap.c cd9660_node.c cd9660_util.c cd9660_vnops.c	\
	cd9660_lookup.c cd9660_rrip.c cd9660_vfsops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
