#	$NetBSD: Makefile,v 1.1.6.2 2008/10/19 22:18:00 haad Exp $
#

.PATH:	${.CURDIR}/../../../../fs/cd9660

LIB=	rumpfs_cd9660

SRCS=	cd9660_bmap.c cd9660_node.c cd9660_util.c cd9660_vnops.c	\
	cd9660_lookup.c cd9660_rrip.c cd9660_vfsops.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
