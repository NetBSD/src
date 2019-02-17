#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:44 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/cd9660

KMOD=	cd9660
SRCS=	cd9660_bmap.c cd9660_lookup.c cd9660_node.c cd9660_rrip.c \
	cd9660_util.c cd9660_vfsops.c cd9660_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
